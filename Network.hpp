#pragma once

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

#include "Debug.hpp"
#include "Logger.hpp"

namespace net {

typedef uint64_t ip_t;
typedef uint16_t port_t;

ip_t ip_from_ints(ip_t a, ip_t b, ip_t c, ip_t d) {
  return (a << 24) | (b << 16) | (c << 8) | (d << 8);
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
};

template <typename T>
struct Package {
	Addr addr;
	T data;

	Package():
    addr(), data()
  {}

	Package(const Addr &addr, const T &data):
    addr(addr), data(data)
  {}
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
      TERMINATE("Can't create socket\n");
    }

    sockaddr_in address = Addr(INADDR_ANY, htons(port_));

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
	void send(const Package<T> &package) const {
    if(sizeof(package.data) > MAX_PACKET_SIZE) {
      TERMINATE("Package too big");
    }

    sockaddr_in address = package.addr;

    int sent_bytes = sendto(handle_, &package.data, sizeof(T), 0, (sockaddr *) &address, sizeof(sockaddr_in));

    if(sent_bytes != sizeof(T)) {
      TERMINATE("Can't send packet\n");
    }

    /* std::cerr << "Send: " << send.text << std::endl; */
  }

  template <typename T>
	bool receive(Package<T> &package) const {
    sockaddr_in from;
    socklen_t from_length = sizeof(from);

    int received_bytes = recvfrom(handle_, &package.data, sizeof(T), 0, (sockaddr *) &from, &from_length);

    if(received_bytes < sizeof(T)) {
      return false;
    }

    package.addr = Addr(from);

    return true;
  }

	port_t port() const {
    return port_;
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
    void send(const Package<T> &package) {
      if(!is_open()) {
        TERMINATE("Can't send packet: connection is not established\n");
      }

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
    handle_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(handle_ <= 0) {
      TERMINATE("Can't create socket\n");
    }

    sockaddr_in address = Addr(INADDR_ANY, htons(port_));

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

    sockaddr_in address = Addr(INADDR_ANY, htons(port_));

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
  bool receive(const Package<T> &package) {
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
