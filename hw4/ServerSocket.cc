/*
 * Copyright ©2019 Aaron Johnston.  All rights reserved.  Permission is
 * hereby granted to students registered for University of Washington
 * CSE 333 for use solely during Summer Quarter 2019 for purposes of
 * the course.  No other use, copying, distribution, or modification
 * is permitted without prior written consent. Copyrights for
 * third-party components of this work must be honored.  Instructors
 * interested in reusing these course materials should contact the
 * author.
 */

#include <stdio.h>       // for snprintf()
#include <unistd.h>      // for close(), fcntl()
#include <sys/types.h>   // for socket(), getaddrinfo(), etc.
#include <sys/socket.h>  // for socket(), getaddrinfo(), etc.
#include <arpa/inet.h>   // for inet_ntop()
#include <netdb.h>       // for getaddrinfo()
#include <errno.h>       // for errno, used by strerror()
#include <string.h>      // for memset, strerror()
#include <iostream>      // for std::cerr, etc.

#include "./ServerSocket.h"

extern "C" {
  #include "libhw1/CSE333.h"
}

namespace hw4 {

ServerSocket::ServerSocket(uint16_t port) {
  port_ = port;
  listen_sock_fd_ = -1;
}

ServerSocket::~ServerSocket() {
  // Close the listening socket if it's not zero.  The rest of this
  // class will make sure to zero out the socket if it is closed
  // elsewhere.
  if (listen_sock_fd_ != -1)
    close(listen_sock_fd_);
  listen_sock_fd_ = -1;
}

bool ServerSocket::BindAndListen(int ai_family, int *listen_fd) {
  // Use "getaddrinfo," "socket," "bind," and "listen" to
  // create a listening socket on port port_.  Return the
  // listening socket through the output parameter "listen_fd".

  // STEP 1:
  struct addrinfo hints, *result;
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = ai_family;
  hints.ai_socktype = SOCK_STREAM;  // stream
  hints.ai_flags = AI_PASSIVE;      // use wildcard "in6addr_any" address
  hints.ai_flags |= AI_V4MAPPED;    // use v4-mapped v6 if no v6 found
  hints.ai_protocol = IPPROTO_TCP;  // tcp protocol
  hints.ai_canonname = nullptr;
  hints.ai_addr = nullptr;
  hints.ai_next = nullptr;

  int res = getaddrinfo(nullptr,
                        std::to_string(port_).c_str(),
                        &hints,
                        &result);
  if (res != 0) {
    std::cerr << "getaddrinfo() failed: ";
    std::cerr << gai_strerror(res) << std::endl;
    return false;
  }

  int ret_fd = -1;
  for (struct addrinfo *rp = result; rp != nullptr; rp = rp->ai_next) {
    ret_fd = socket(rp->ai_family,
                    rp->ai_socktype,
                    rp->ai_protocol);
    if (ret_fd == -1) {
      std::cerr << "socket() failed: " << gai_strerror(errno) << std::endl;
      ret_fd = -1;
      continue;
    }

    // Configure the socket; we're setting a socket "option."  In
    // particular, we set "SO_REUSEADDR", which tells the TCP stack
    // so make the port we bind to available again as soon as we
    // exit, rather than waiting for a few tens of seconds to recycle it.
    int optval = 1;
    setsockopt(ret_fd, SOL_SOCKET, SO_REUSEADDR,
                &optval, sizeof(optval));

    if (bind(ret_fd, rp->ai_addr, rp->ai_addrlen) == 0)
      break;

    // clean up.
    close(ret_fd);
    ret_fd = -1;
  }

  freeaddrinfo(result);

  if (ret_fd == -1)
    return false;

  if (listen(ret_fd, SOMAXCONN) != 0) {
    std::cerr << "Failed to mark socket as listening: ";
    std::cerr << gai_strerror(errno) << std::endl;
    return false;
  }

  listen_sock_fd_ = ret_fd;
  sock_family_ = ai_family;
  *listen_fd = ret_fd;
  return true;
}

bool ServerSocket::Accept(int *accepted_fd,
                          std::string *client_addr,
                          uint16_t *client_port,
                          std::string *client_dnsname,
                          std::string *server_addr,
                          std::string *server_dnsname) {
  // Accept a new connection on the listening socket listen_sock_fd_.
  // (Block until a new connection arrives.)  Return the newly accepted
  // socket, as well as information about both ends of the new connection,
  // through the various output parameters.

  // STEP 2:
  struct sockaddr_storage caddr;
  socklen_t caddr_len = sizeof(caddr);
  int client_fd = -1;
  // Block until a new connection arrives.
  while (1) {
    client_fd = accept(listen_sock_fd_,
                          reinterpret_cast<struct sockaddr *>(&caddr),
                          &caddr_len);
    if (client_fd < 0) {
      if ((errno == EINTR) || (errno == EAGAIN) || (errno == EWOULDBLOCK))
        continue;
      std::cerr << "Failure on accept: " << strerror(errno) << std::endl;
      return false;
    }
    // successfully accept the connection.
    break;
  }
  *accepted_fd = client_fd;
  struct sockaddr *addr_ptr = reinterpret_cast<struct sockaddr *>(&caddr);
  // Handle IPv4 and IPv6 respectively
  // Get the corresponding type of IP address and
  // extract the client_addr & client_port.
  if (addr_ptr->sa_family == AF_INET) {
    struct sockaddr_in IPv4 = *(reinterpret_cast<struct sockaddr_in *>
                                (&caddr));
    char str[INET_ADDRSTRLEN];
    *client_addr = std::string(inet_ntop(AF_INET,
                              &(IPv4.sin_addr),
                              str,
                              INET_ADDRSTRLEN));
    *client_port = IPv4.sin_port;
  } else if (addr_ptr->sa_family == AF_INET6) {
    struct sockaddr_in6 IPv6 = *(reinterpret_cast<struct sockaddr_in6 *>
                                (&caddr));
    char str[INET6_ADDRSTRLEN];
    *client_addr = std::string(inet_ntop(AF_INET6,
                              &(IPv6.sin6_addr),
                              str,
                              INET6_ADDRSTRLEN));
    *client_port = IPv6.sin6_port;
  }
  // Use getnameinfo() to reverse look up DNS
  // of the client.
  int res;
  char host_name_buf[NI_MAXHOST];
  res = getnameinfo(addr_ptr, caddr_len, host_name_buf,
                    sizeof(host_name_buf), NULL, 0, 0);
  Verify333(res == 0);
  *client_dnsname = std::string(host_name_buf);

  // Again, handle IPv4 and IPv6 respectively.
  // Invoke getsockname() to get the current address
  // to which the socket is bound to which is the server
  // address. Extract the server_addr from the address.
  // Using similar strategy applied to client, invoke
  // getnameinfo() to reverse look up the DNS of the server.
  if (sock_family_ == AF_INET) {
    struct sockaddr_in addr;
    socklen_t addr_length = sizeof(addr);
    res = getsockname(client_fd,
                      reinterpret_cast<struct sockaddr *>(&addr),
                      &addr_length);
    Verify333(res == 0);
    char str[INET_ADDRSTRLEN];
    *server_addr = std::string(inet_ntop(AF_INET,
                              &(addr.sin_addr),
                              str,
                              INET_ADDRSTRLEN));
    res = getnameinfo(reinterpret_cast<struct sockaddr *>(&addr),
                      addr_length,
                      host_name_buf,
                      sizeof(host_name_buf),
                      NULL,
                      0,
                      0);
    Verify333(res == 0);
    *server_dnsname = std::string(host_name_buf);
  } else if (sock_family_ == AF_INET6) {
    struct sockaddr_in6 addr;
    socklen_t addr_length = sizeof(addr);
    res = getsockname(client_fd,
                      reinterpret_cast<struct sockaddr *>(&addr),
                      &addr_length);
    Verify333(res == 0);
    char str[INET6_ADDRSTRLEN];
    *server_addr = std::string(inet_ntop(AF_INET,
                              &(addr.sin6_addr),
                              str,
                              INET6_ADDRSTRLEN));
    res = getnameinfo(reinterpret_cast<struct sockaddr *>(&addr),
                      addr_length,
                      host_name_buf,
                      sizeof(host_name_buf),
                      NULL,
                      0,
                      0);
    Verify333(res == 0);
    *server_dnsname = std::string(host_name_buf);
  }
  return true;
}

}  // namespace hw4
