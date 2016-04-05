
#ifndef UVCC_NETSTRUCT__HPP
#define UVCC_NETSTRUCT__HPP

#include <uv.h>
#include <cstring>       // memset()
#include <cstdlib>       // atoi()
#ifdef _WIN32
#include <Winsock2.h>    // sockaddr_storage sockaddr_in sockaddr_in6
#include <Ws2tcpip.h>    // addrinfo AI_ADDRCONFIG
#else
#include <sys/socket.h>  // sockaddr_storage
#include <netinet/in.h>  // sockaddr_in sockaddr_in6
#include <netdb.h>       // addrinfo AI_ADDRCONFIG
#endif


namespace uv
{
/*! \defgroup g__netstruct Network-related structures initialization
    \ingroup g__utility
    \sa Linux: [ip(7):`sockaddr_in`](http://man7.org/linux/man-pages/man7/ip.7.html),
               [ipv6(7):`sockaddr_in6`](http://man7.org/linux/man-pages/man7/ipv6.7.html),
               [socket(7):`sockaddr_storage`](http://man7.org/linux/man-pages/man7/socket.7.html).
    \sa Windows: [`sockaddr_in`](https://msdn.microsoft.com/en-us/library/ms740496(v=vs.85).aspx),
                 [`sockaddr_in6`](https://msdn.microsoft.com/en-us/library/ms740496(v=vs.85).aspx),
                 [`sockaddr_storage`](https://msdn.microsoft.com/en-us/library/ms740504(v=vs.85).aspx). */
//! \{

//! \name sockaddr_in
//! \{

/*! \brief Initialize a `sockaddr_in` structure. */
template< typename = void >
int init(::sockaddr_in &_sa)
{
  std::memset(&_sa, 0, sizeof(_sa));
  _sa.sin_family = AF_INET;
  return 0;
}
/*! \brief Initialize a `sockaddr_in` structure from strings containing an IPv4 address and (optionally) a port.
    \sa libuv API documentation: [`uv_ip4_addr()`](http://docs.libuv.org/en/v1.x/misc.html#c.uv_ip4_addr),
                                 [`uv_inet_pton()`](http://docs.libuv.org/en/v1.x/misc.html#c.uv_inet_pton).
    \sa Linux: [`inet_pton()`](http://man7.org/linux/man-pages/man3/inet_pton.3.html).
        Windows: [`RtlIpv4StringToAddressEx()`](https://msdn.microsoft.com/en-us/library/aa814459(v=vs.85).aspx),
                 [`InetPton()`](https://msdn.microsoft.com/en-us/library/cc805844(v=vs.85).aspx). */
template< typename = void >
int init(::sockaddr_in &_sa, const char *_ip, const char *_port = nullptr)
{
  int pnum = _port ? std::atoi(_port) : 0;
  return ::uv_ip4_addr(_ip, pnum, &_sa);
}

//! \}


//! \name sockaddr_in6
//! \{

/*! \brief Initialize a `sockaddr_in6` structure. */
template< typename = void >
int init(::sockaddr_in6 &_sa)
{
  std::memset(&_sa, 0, sizeof(_sa));
  _sa.sin6_family = AF_INET6;
  return 0;
}
/*! \brief Initialize a `sockaddr_in6` structure from strings containing an IPv6 address and (optionally) a port.
    \sa libuv API documentation: [`uv_ip6_addr()`](http://docs.libuv.org/en/v1.x/misc.html#c.uv_ip6_addr),
                                 [`uv_inet_pton()`](http://docs.libuv.org/en/v1.x/misc.html#c.uv_inet_pton).
    \sa Linux: [`inet_pton()`](http://man7.org/linux/man-pages/man3/inet_pton.3.html).
        Windows: [`RtlIpv6StringToAddressEx()`](https://msdn.microsoft.com/en-us/library/aa814463(v=vs.85).aspx),
                 [`InetPton()`](https://msdn.microsoft.com/en-us/library/cc805844(v=vs.85).aspx). */
template< typename = void >
int init(::sockaddr_in6 &_sa, const char *_ip, const char *_port = nullptr)
{
  int pnum = _port ? std::atoi(_port) : 0;
  return ::uv_ip6_addr(_ip, pnum, &_sa);
}

//! \}


//! \name sockaddr_storage
//! \{

template< typename = void >
int init(::sockaddr_storage &_sa, decltype(::sockaddr_storage::ss_family) _family = AF_UNSPEC)
{
  std::memset(&_sa, 0, sizeof(_sa));
  _sa.ss_family = _family;
  return 0;
}

template< typename = void >
int init(::sockaddr_storage &_sa, const char *_ip, const char *_port = nullptr)
{
  std::memset(&_sa, 0, sizeof(_sa));
  return (
      init(reinterpret_cast< ::sockaddr_in& >(_sa), _ip, _port) == 0
      or
      init(reinterpret_cast< ::sockaddr_in6& >(_sa), _ip, _port) == 0
  ) ? 0 : UV_EINVAL;
}

//! \}


//! \name addrinfo
//! \{

/*! \details Initialize an `addrinfo` structure for to be used as a hints argument in `uv::getaddrinfo` request.
    \sa Linux: [`getaddrinfo()`](http://man7.org/linux/man-pages/man3/getaddrinfo.3.html).
        Windows: [`addrinfo`](https://msdn.microsoft.com/en-us/library/ms737530(v=vs.85).aspx),
                 [`getaddrinfo()`](https://msdn.microsoft.com/en-us/library/ms738520(v=vs.85).aspx),
                 [`GetAddrInfoEx()`](https://msdn.microsoft.com/en-us/library/ms738518(v=vs.85).aspx). */
template< typename = void >
int init(::addrinfo &_ai,
    decltype(::addrinfo::ai_family)   _family = AF_UNSPEC,
    decltype(::addrinfo::ai_socktype) _socktype = 0,
    decltype(::addrinfo::ai_flags)    _flags = AI_ADDRCONFIG
)
{
  std::memset(&_ai, 0, sizeof(_ai));
  _ai.ai_family = _family;
  _ai.ai_socktype = _socktype;
  _ai.ai_flags = _flags;
  return 0;
}

//! \}


//! \}
}

#endif
