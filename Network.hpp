#pragma once

#include <cassert>
#include <cstring>
#include <cstdint>

#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>

#include <netdb.h>
#include <arpa/inet.h>

#include <stdexcept>
#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <optional>
#include <type_traits>
#include <mutex>

#ifndef TERMINATE
#include "Debug.hpp"
#include "Logger.hpp"
#endif

namespace net {

typedef uint64_t ip_t;
typedef uint16_t port_t;

ip_t ip_from_ints(ip_t a, ip_t b, ip_t c, ip_t d) {
  return ((a << 24) | (b << 16) | (c << 8) | (d << 0));
}

struct Addr {
	ip_t ip;
	port_t port;

	constexpr Addr():
    ip(INADDR_ANY), port(0)
  {}

	constexpr Addr(ip_t ip, port_t port):
    ip(ip), port(port)
  {}

  constexpr Addr(sockaddr_in saddr_in):
    ip(ntohl(saddr_in.sin_addr.s_addr)), port(ntohs(saddr_in.sin_port))
  {}

  operator sockaddr_in() const {
    sockaddr_in saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = htonl(ip);
    saddr.sin_port = htons(port);
    return saddr;
    /* return (sockaddr_in){ */
    /*   .sin_family = AF_INET, */
    /*   .sin_addr = { */
    /*     .s_addr = htonl(ip) */
    /*   }, */
    /*   .sin_port = htons(port) */
    /* }; */
  }

	constexpr bool operator== (const Addr &other) const {
    return ip == other.ip && port == other.port;
  }

	constexpr bool operator!= (const Addr &other) const {
    return !(*this == other);
  }

  bool operator<(const Addr &other) const { return ip < other.ip; }

  std::string to_str() const {
    sockaddr_in saddr = (*this);
    return inet_ntoa(saddr.sin_addr) + std::string(":") + std::to_string(port);
  }
};

template <typename T>
struct Package {
	Addr addr;
	T data;

	Package():
    addr(), data()
  {
    static_assert(std::is_standard_layout_v<T>);
  }

	Package(const Addr &addr, const T data):
    addr(addr), data(data)
  {
    static_assert(std::is_standard_layout_v<T>);
  }
};

template <typename T>
decltype(auto) make_package(const net::Addr addr, const T data) {
  return Package<T>(addr, data);
}

struct Blob {
  Addr addr;
  std::vector<uint8_t> data_;

  Blob():
    addr(), data_()
  {}

  template <typename T>
  Blob(Package<T> package):
    addr(package.addr), data_(sizeof(T))
  {
    memcpy(data(), package.data, sizeof(T));
  }

  size_t size() const {
    return data_.size();
  }

  void resize(size_t new_size) {
    data_.resize(new_size);
  }

  void *data() {
    return data_.data();
  }

  const void *data() const {
    return data_.data();
  }

/*   template <typename T> */
/*   operator Package<T>() { */
/*     std::cout << "implicit conversion" << std::endl; */
/*     ASSERT(sizeof(T) == size()); */
/*     Package<T> packet; */
/*     packet.addr = addr; */
/*     memcpy(&packet.data, data(), sizeof(T)); */
/*     std::cout << "origin " << addr.to_str() << std::endl; */
/*     std::cout << "copied " << packet.addr.to_str() << std::endl; */
/*   } */

  template <typename T, typename F, typename CF>
  bool try_visit_as(F &&func, CF &&cond) const {
    if(!cond(*this)) {
      return false;
    }
    T t;
    memcpy(&t, data(), sizeof(T));
    func(t);
    return true;
  }

  template <typename T, typename F>
  bool try_visit_as(F &&func) const {
    return try_visit_as<T>(std::forward<F>(func), [&](const Blob &blob) {
      return blob.size() == sizeof(T);
    });
  }
};

enum class SocketType {
  TCP_SERVER, TCP_CLIENT, UDP
};

template <SocketType Proto> class Socket;

template <>
class Socket<SocketType::UDP> {
	static const int MAX_PACKET_SIZE = 256;

	int handle_;
	port_t port_;
public:
	Socket(port_t port):
    port_(port)
  {
    handle_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(handle_ <= 0) {
      perror("error");
      TERMINATE("Can't create socket\n");
    }

    sockaddr_in address = Addr(INADDR_ANY, port_);

    if(bind(handle_, (const sockaddr *)&address, sizeof(sockaddr_in)) < 0) {
      perror("error");
      TERMINATE("Can't bind socket\n");
    }

    int nonBlocking = 1;
    if(fcntl(handle_, F_SETFL, O_NONBLOCK, nonBlocking) == -1) {
      perror("error");
      TERMINATE("Can't set non-blocking socket\n");
    }
  }

	~Socket() {
    close(handle_);
  }

  template <typename T>
	void send(const Package<T> package) const {
    if(sizeof(package.data) > MAX_PACKET_SIZE) {
      perror("error");
      TERMINATE("Package too big");
    }

    sockaddr_in address = package.addr;

    int sent_bytes = sendto(handle_, &package.data, sizeof(T), 0, (sockaddr *) &address, sizeof(sockaddr_in));

    if(sent_bytes != sizeof(T)) {
      std::cout << package.addr.to_str() << std::endl;
      perror("error");
      TERMINATE("Can't send packet\n");
    }

    /* std::cerr << "Send: " << send.text << std::endl; */
  }

	std::optional<Blob> receive() const {
    sockaddr_in saddr_from;
    socklen_t saddr_from_length = sizeof(saddr_from);

    Blob blob;
    blob.resize(MAX_PACKET_SIZE);

    int received_bytes = recvfrom(handle_, blob.data(), MAX_PACKET_SIZE, 0, (sockaddr *)&saddr_from, &saddr_from_length);

    if(received_bytes <= 0) {
      return std::optional<Blob>();
    }

    blob.resize(received_bytes);
    blob.addr = Addr(saddr_from);
    std::cout << blob.addr.to_str() << std::endl;

    return blob;
  }

	port_t port() const {
    return port_;
  }

  template <typename G, typename F>
  void listen(std::mutex &mtx, G &&idle, F &&func) {
    std::optional<Blob> opt_blob;
    bool cond = 1;
    while(cond) {
      idle();
      std::lock_guard<std::mutex> guard(mtx);
      if((opt_blob = receive()).has_value()) {
        cond = func(*opt_blob);
      }
    }
  }

  template <typename F>
  void listen(std::mutex &mtx, F &&func) {
    listen(mtx, [](){}, std::forward<F>(func));
  }
};

template <>
class Socket<SocketType::TCP_SERVER> {
	static const int MAX_PACKET_SIZE = 256;

	int handle_;
	port_t port_;

  struct Connection {
    Socket<SocketType::TCP_SERVER> &socket;
    int handle_ = -1;
    Addr client;

    Connection(Socket<SocketType::TCP_SERVER> &socket):
      socket(socket)
    {}

    bool try_connect() {
      sockaddr_in cl_saddr;
      socklen_t bytes = sizeof(sockaddr_in);
      handle_ = accept(socket.handle_, (sockaddr *)&cl_saddr, &bytes);
      if(errno == EWOULDBLOCK) {
        errno = 0;
        return false;;
      }
      if(handle_ <= 0) {
        perror("error");
        TERMINATE("Can't accept connection\n");
      }
      client = Addr(cl_saddr);
      return true;
    }

    bool is_open() {
      return handle_ != -1;
    }

    template <typename T>
    void send(Package<T> &package) {
      if(!is_open()) {
        TERMINATE("Can't send packet: connection is not established\n");
      }

      package.addr = client;

      int sent_bytes = write(handle_, &package.data, sizeof(T));

      if(sent_bytes != sizeof(T)) {
        perror("error");
        TERMINATE("Can't send packet: invalid number of bytes");
      }
    }

    template <typename T>
    bool receive(Package <T> &package) {
      if(!is_open()) {
        TERMINATE("Can't send packaet: connection is not established\n");
      }

      int received_bytes = read(handle_, &package.data, sizeof(T));

      if(received_bytes != sizeof(T)) {
        return false;
      }

      package.addr = client;
      return true;
    }

    void close_connection() {
      close(handle_);
      handle_ = -1;
    }

    ~Connection() {
      if(is_open()) {
        close_connection();
      }
    }
  };

  std::map<net::Addr, Connection> clients;
public:
	Socket(port_t port):
    port_(port)
  {
    handle_ = socket(AF_INET, SOCK_STREAM, 0);
    if(handle_ <= 0) {
      TERMINATE("Can't create socket\n");
    }

    sockaddr_in address = Addr(INADDR_ANY, port_);

    if(bind(handle_, (const sockaddr *)&address, sizeof(sockaddr_in)) < 0) {
      perror("error");
      TERMINATE("Can't bind socket\n");
    }

    int nonBlocking = 1;
    if(fcntl(handle_, F_SETFL, O_NONBLOCK, nonBlocking) == -1) {
      perror("error");
      TERMINATE("Can't set non-blocking socket\n");
    }

    listen(handle_, 5);
    if(errno)perror("error:");
  }

  ~Socket() {
    close(handle_);
  }

  void wait_for_connection() {
    Connection conn(*this);
    while(!conn.try_connect())
      ;
    clients.insert(std::make_pair(conn.client, conn));
  }

  template <typename T>
  void send_all(net::Package<T> &package) {
    for(auto c : clients) {
      auto &conn = c.second;
      conn.send(package);
    }
  }

  template <typename T, typename F>
  bool receive_all(net::Package<T> &package, F checker) {
    for(auto c : clients) {
      auto &conn = c.second;
      if(conn.receive(package)) {
        if(checker(package.data)) {
          return true;
        }
      }
    }
    return false;
  }

	port_t port() const {
    return port_;
  }
};

template <>
class Socket<SocketType::TCP_CLIENT> {
  int handle_;
  port_t port_;
public:
  Addr server;

  Socket(port_t port):
    port_(port)
  {
    handle_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(handle_ <= 0) {
      TERMINATE("Can't create socket\n");
    }

    sockaddr_in address = Addr(INADDR_ANY, port_);

    if(bind(handle_, (const sockaddr *)&address, sizeof(sockaddr_in)) < 0) {
      perror("error");
      TERMINATE("Can't bind socket\n");
    }

    int nonBlocking = 1;
    if(fcntl(handle_, F_SETFL, O_NONBLOCK, nonBlocking) == -1) {
      perror("error");
      TERMINATE("Can't set non-blocking socket\n");
    }
  }

  ~Socket() {
    close(handle_);
  }

  bool try_connect(Addr addr) {
    server = addr;
    if(connect(handle_, (sockaddr *)&server, sizeof(server)) < 0) {
      perror("error");
      return false;
    }
    return true;
  }

  template <typename T>
  void send(const Package<T> &package) {
    int sent_bytes = write(handle_, &package.data, sizeof(T));
    if(sent_bytes < sizeof(T)) {
      perror("error");
      TERMINATE("Can't sent package: sent less bytes\n");
    }
  }

  template <typename T>
  bool receive(Package<T> &package) {
    int received_bytes = read(handle_, &package.data, sizeof(T));
    if(received_bytes < sizeof(T)) {
      return false;
    }
    package.addr = server;
    return true;
  }

	port_t port() const {
    return port_;
  }
};

}
