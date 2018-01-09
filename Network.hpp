#pragma once

#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>

#include <stdexcept>
#include <iostream>
#include <string>

namespace net {
typedef unsigned long ip_t;
typedef unsigned short port_t;

struct Addr {
	ip_t ip;
	port_t port;

	Addr():
    ip(0), port(0)
  {}

	Addr(ip_t ip, port_t port):
    ip(ip), port(port)
  {}

	bool operator== (const Addr &other) const {
    return ip == other.ip && port == other.port;
  }

	bool operator!= (const Addr &other) const {
    return !(*this == other);
  }
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

class Socket {
	static const int MAX_PACKET_SIZE = 256;

	int handle_;
	port_t port_;
public:
	Socket(port_t port):
    port_(port)
  {
    handle_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(handle_ <= 0) {
      throw std::runtime_error("Cant create socket");
    }

    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);

    if(bind(handle_, (const sockaddr *) &address, sizeof(sockaddr_in)) < 0) {
      throw std::runtime_error("Cant bind socket");
    }

    int nonBlocking = 1;
    if(fcntl(handle_, F_SETFL, O_NONBLOCK, nonBlocking) == -1) {
      throw std::runtime_error("Cant set non-blocking socket");
    }
  }

	~Socket() {
    close(handle_);
  }

  template <typename T>
	void send(const Package<T> &send) const {
    if(sizeof(send.data) > MAX_PACKET_SIZE) {
      throw std::runtime_error("Too big packet");
    }

    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(send.addr.ip);
    address.sin_port = htons(send.addr.port);

    int sent_bytes = sendto(handle_, &send.data, sizeof(T), 0, (sockaddr *) &address, sizeof(sockaddr_in));

    if(sent_bytes != sizeof(send.data)) {
      throw std::runtime_error("Cant send packet");
    }

    /* std::cerr << "Send: " << send.text << std::endl; */
  }

  template <typename T>
	bool receive(Package<T> &received) const {
    sockaddr_in from;
    socklen_t from_length = sizeof(from);

    int received_bytes = recvfrom(handle_, &received.data, sizeof(T), 0, (sockaddr *) &from, &from_length);

    if(received_bytes < sizeof(T)) {
      return false;
    }

    received.addr = Addr(ntohl(from.sin_addr.s_addr), ntohs(from.sin_port));

    return true;
  }

	port_t port() const {
    return port_;
  }
};
}
