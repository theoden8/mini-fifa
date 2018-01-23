#pragma once

#include <cassert>
#include <cstring>
#include <cstdint>
#include <ctime>

#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>

/* #include <sys/select.h> */
/* #include <netinet/ip_icmp.h> */

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

ip_t ipv4_from_ints(ip_t a, ip_t b, ip_t c, ip_t d) {
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

  bool operator<(const Addr &other) const { return ip < other.ip || (ip == other.ip && port < other.port); }

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
  ICMP,
  UDP,
  TCP_SERVER,
  TCP_CLIENT
};

template <SocketType Proto> class Socket;

/* // https://stackoverflow.com/a/20105379/4811285 */
/* template <> */
/* class Socket<SocketType::ICMP> { */
/*   static constexpr int MAX_PACKET_SIZE = 256; */

/*   int sequence = 0; */
/*   int handle_; */
/*   icmphdr icmp_hdr; */
/*   port_t port; */
/* public: */
/*   Socket(port_t port): */
/*     port_(port) */
/*   { */
/*     handle_ = socket(PF_INET, SOCK_DGRAM, IPPROTO_ICMP); */
/*     if(handle_ <= 0) { */
/*       perror("error"); */
/*       TERMINATE("Can't create socket\n"); */
/*     } */

/*     memset(&icmp_hdr, 0, sizeof(icmphdr)); */
/*     icmp_hdr.type = ICMP_ECHO; */
/*     icmp_hdr.un.echo.id = 1234; // arbitrary id */
/*     sockaddr_in address = Addr(INADDR_ANY, port_); */

/*     if(bind(handle_, (const sockaddr *)&address, sizeof(sockaddr_in)) < 0) { */
/*       perror("error"); */
/*       TERMINATE("Can't bind socket\n"); */
/*     } */

/*     int nonBlocking = 1; */
/*     if(fcntl(handle_, F_SETFL, O_NONBLOCK, nonBlocking) == -1) { */
/*       perror("error"); */
/*       TERMINATE("Can't set non-blocking socket\n"); */
/*     } */
/*   } */

/*   void send(const char *msg) { */
/*     unsigned char data[MAX_PACKET_SIZE]; */

/*     icmp_hdr.un.echo.sequence = sequence++; */
/*     memcpy(data, &icmp_hdr, sizeof(icmphdr)); */
/*     memcpy(data + sizeof(icmphdr), msg, strlen(msg)); //icmp payload */
/*     sockaddr_in addr; */
/*     int sent_bytes = sendto(handle_, data, sizeof(icmphdr) + strlen(msg), 0, (sockaddr_in *)&addr, sizeof(sockaddr_in)); */
/*     if(rc <= 0) { */
/*       perror("error"): */
/*       TERMINATE("Can't send packet\n"); */
/*     } */
/*   } */

/*   // wait for reply with a timeout */
/*   bool select(int sec=3) { */
/*     fd_set read_set; */

/*     timeval timeout = { sec, 0 }; */

/*     memset(read_set, 0, sizeof(fd_set)); */
/*     FD_SET(handle_, &read_set); */

/*     int rc = select(handle_ + 1, &read_set, nullptr, nullptr, &timeout); */
/*     if(rc < 0) { */
/*       perror("error"); */
/*       TERMINATE("Can't select\n"); */
/*     } */

/*     return rc != 0; */
/*   } */

/*   bool receive() { */
/*     unsigned char data[MAX_PACKET_SIZE]; */

/*     socklen_t slen; */
/*     icmphdr rcv_hdr; */

/*     int received_bytes = recvfrom(handle_, data, MAX_PACKET_SIZE, 0, nullptr, &slen); */
/*     if(received_bytes <= 0) { */
/*       perror("error"); */
/*       TERMINATE("Can't receive packet\n"); */
/*     } */

/*     if(received_bytes < sizeof(icmphdr)) { */
/*       TERMINATE("Recieved too short ICMP packet\n"); */
/*     } */

/*     memcpy(&rcv_hdr, data, sizeof(icmphdr)); */
/*     if(rcv_hdr.type == ICMP_ECHOREPLY) { */
/*       // id = icmp_hdr.un.echo.id */
/*       // sequence = icmp_hdr.un.echo.sequence */
/*     } else { */
/*       // type = rcv_hdr.type */
/*     } */
/*   } */

/*   ~Socket() { */
/*     close(handle_); */
/*   } */
/* }; */

template <>
class Socket<SocketType::UDP> {
	static constexpr int MAX_PACKET_SIZE = 256;

	int handle_;
	port_t port_;
  std::mutex mtx;
public:
	Socket(port_t port):
    port_(port)
  {
    std::lock_guard<std::mutex> guard(mtx);
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
	void send(const Package<T> package) {
    std::lock_guard<std::mutex> guard(mtx);
    if(sizeof(T) > MAX_PACKET_SIZE) {
      perror("error");
      TERMINATE("The packet to be sent is too big\n");
    }

    sockaddr_in address = package.addr;

    int sent_bytes = sendto(handle_, &package.data, sizeof(T), 0, (sockaddr *) &address, sizeof(sockaddr_in));

    if(sent_bytes != sizeof(T)) {
      std::cout << package.addr.to_str() << std::endl;
      perror("error");
      TERMINATE("Can't send packet\n");
    }
  }

	std::optional<Blob> receive() {
    std::lock_guard<std::mutex> guard(mtx);
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

    return blob;
  }

	constexpr port_t port() const {
    return port_;
  }

  template <typename G, typename F>
  void listen(G &&idle, F &&func) {
    std::optional<Blob> opt_blob;
    bool cond = 1;
    while(cond) {
      if(!idle()) {
        break;
      }
      if((opt_blob = receive()).has_value()) {
        cond = func(*opt_blob);
      }
    }
  }

  template <typename F>
  void listen(F &&func) {
    listen([](){return true;}, std::forward<F>(func));
  }
};

/* template <> */
/* class Socket<SocketType::TCP_SERVER> { */
/* 	static constexpr int MAX_PACKET_SIZE = 256; */

/* 	int handle_; */
/* 	port_t port_; */

/*   struct Connection { */
/*     Socket<SocketType::TCP_SERVER> &socket; */
/*     int handle_ = -1; */
/*     Addr client; */

/*     Connection(Socket<SocketType::TCP_SERVER> &socket): */
/*       socket(socket) */
/*     {} */

/*     bool try_connect() { */
/*       sockaddr_in cl_saddr; */
/*       socklen_t bytes = sizeof(sockaddr_in); */
/*       handle_ = accept(socket.handle_, (sockaddr *)&cl_saddr, &bytes); */
/*       if(errno == EWOULDBLOCK) { */
/*         errno = 0; */
/*         return false;; */
/*       } */
/*       if(handle_ <= 0) { */
/*         perror("error"); */
/*         TERMINATE("Can't accept connection\n"); */
/*       } */
/*       client = Addr(cl_saddr); */
/*       return true; */
/*     } */

/*     bool is_open() { */
/*       return handle_ != -1; */
/*     } */

/*     template <typename T> */
/*     void send(Package<T> &package) { */
/*       if(!is_open()) { */
/*         TERMINATE("Can't send packet: connection is not established\n"); */
/*       } */

/*       package.addr = client; */

/*       int sent_bytes = write(handle_, &package.data, sizeof(T)); */

/*       if(sent_bytes != sizeof(T)) { */
/*         perror("error"); */
/*         TERMINATE("Can't send packet: invalid number of bytes"); */
/*       } */
/*     } */

/*     template <typename T> */
/*     bool receive(Package <T> &package) { */
/*       if(!is_open()) { */
/*         TERMINATE("Can't send packaet: connection is not established\n"); */
/*       } */

/*       int received_bytes = read(handle_, &package.data, sizeof(T)); */

/*       if(received_bytes != sizeof(T)) { */
/*         return false; */
/*       } */

/*       package.addr = client; */
/*       return true; */
/*     } */

/*     void close_connection() { */
/*       close(handle_); */
/*       handle_ = -1; */
/*     } */

/*     ~Connection() { */
/*       if(is_open()) { */
/*         close_connection(); */
/*       } */
/*     } */
/*   }; */

/*   std::map<net::Addr, Connection> clients; */
/* public: */
/* 	Socket(port_t port): */
/*     port_(port) */
/*   { */
/*     handle_ = socket(AF_INET, SOCK_STREAM, 0); */
/*     if(handle_ <= 0) { */
/*       TERMINATE("Can't create socket\n"); */
/*     } */

/*     sockaddr_in address = Addr(INADDR_ANY, port_); */

/*     if(bind(handle_, (const sockaddr *)&address, sizeof(sockaddr_in)) < 0) { */
/*       perror("error"); */
/*       TERMINATE("Can't bind socket\n"); */
/*     } */

/*     /1* int nonBlocking = 1; *1/ */
/*     /1* if(fcntl(handle_, F_SETFL, O_NONBLOCK, nonBlocking) == -1) { *1/ */
/*     /1*   perror("error"); *1/ */
/*     /1*   TERMINATE("Can't set non-blocking socket\n"); *1/ */
/*     /1* } *1/ */

/*     listen(handle_, 5); */
/*     if(errno)perror("error:"); */
/*   } */

/*   ~Socket() { */
/*     close(handle_); */
/*   } */

/*   void wait_for_connection() { */
/*     Connection conn(*this); */
/*     while(!conn.try_connect()) */
/*       ; */
/*     clients.insert(std::make_pair(conn.client, conn)); */
/*   } */

/*   template <typename T> */
/*   void send_all(net::Package<T> &package) { */
/*     for(auto c : clients) { */
/*       auto &conn = c.second; */
/*       conn.send(package); */
/*     } */
/*   } */

/*   template <typename T, typename F> */
/*   std::optional<Blob> receive_all(net::Package<T> &package, F checker) { */
/*     std::optional<Blob> opt_blob; */
/*     for(auto c : clients) { */
/*       auto &conn = c.second; */
/*       if((opt_blob = conn.receive(package)).has_value()) { */
/*         break; */
/*       } */
/*     } */
/*     return opt_blob; */
/*   } */

/* 	port_t port() const { */
/*     return port_; */
/*   } */
/* }; */

/* template <> */
/* class Socket<SocketType::TCP_CLIENT> { */
/*   static constexpr int MAX_PACKET_SIZE = 256; */
/*   int handle_; */
/*   port_t port_; */
/* public: */
/*   Addr server; */

/*   Socket(port_t port): */
/*     port_(port) */
/*   { */
/*     handle_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); */
/*     if(handle_ <= 0) { */
/*       TERMINATE("Can't create socket\n"); */
/*     } */

/*     sockaddr_in address = Addr(INADDR_ANY, port_); */

/*     if(bind(handle_, (const sockaddr *)&address, sizeof(sockaddr_in)) < 0) { */
/*       perror("error"); */
/*       TERMINATE("Can't bind socket\n"); */
/*     } */

/* /1*     int nonBlocking = 1; *1/ */
/* /1*     if(fcntl(handle_, F_SETFL, O_NONBLOCK, nonBlocking) == -1) { *1/ */
/* /1*       perror("error"); *1/ */
/* /1*       TERMINATE("Can't set non-blocking socket\n"); *1/ */
/* /1*     } *1/ */
/*   } */

/*   ~Socket() { */
/*     close(handle_); */
/*   } */

/*   bool try_connect(Addr addr) { */
/*     server = addr; */
/*     if(connect(handle_, (sockaddr *)&server, sizeof(server)) < 0) { */
/*       if(errno == EALREADY) { */
/*         return false; */
/*       } */
/*       perror("error"); */
/*       return false; */
/*     } */
/*     return true; */
/*   } */

/*   template <typename T> */
/*   void send(const Package<T> &package) { */
/*     if(package.addr != server) { */
/*       do { */
/*         errno = 0; */
/*         if(!try_connect(package.addr)) { */
/*           TERMINATE("Can't set up connection to server\n"); */
/*         } */
/*       } while(errno == EALREADY); */
/*     } */
/*     int sent_bytes = write(handle_, &package.data, sizeof(T)); */
/*     if(sent_bytes < sizeof(T)) { */
/*       perror("error"); */
/*       TERMINATE("Can't sent package: sent less bytes\n"); */
/*     } */
/*   } */

/*   template <typename T> */
/*   std::optional<Blob> receive() { */
/*     Blob blob; */
/*     blob.resize(MAX_PACKET_SIZE); */

/*     int received_bytes = read(handle_, blob.data(), MAX_PACKET_SIZE); */

/*     if(received_bytes <= 0) { */
/*       return std::optional<Blob>(); */
/*     } */

/*     blob.resize(received_bytes); */

/*     return blob; */
/*   } */

/* 	port_t port() const { */
/*     return port_; */
/*   } */
/* }; */

}
