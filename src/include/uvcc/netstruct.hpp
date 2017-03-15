
#ifndef UVCC_NETSTRUCT__HPP
#define UVCC_NETSTRUCT__HPP

#include <cstring>       // memset()
#include <cstdlib>       // atoi()
#include <uv.h>

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
/*! \defgroup doxy_group__netstruct  Network-related structures initialization
    \ingroup doxy_group__utility
    \sa Linux: [ip(7):`sockaddr_in`](http://man7.org/linux/man-pages/man7/ip.7.html),
               [ipv6(7):`sockaddr_in6`](http://man7.org/linux/man-pages/man7/ipv6.7.html),
               [socket(7):`sockaddr_storage`](http://man7.org/linux/man-pages/man7/socket.7.html).\n
        Windows: [`sockaddr_in`](https://msdn.microsoft.com/en-us/library/ms740496(v=vs.85).aspx),
                 [`sockaddr_in6`](https://msdn.microsoft.com/en-us/library/ms740496(v=vs.85).aspx),
                 [`sockaddr_storage`](https://msdn.microsoft.com/en-us/library/ms740504(v=vs.85).aspx). */
//! \{

//! \name sockaddr_in
//! \{

/*! \brief Initialize a `sockaddr_in` structure. */
inline int init(::sockaddr_in &_sin)
{
  std::memset(&_sin, 0, sizeof(_sin));
  _sin.sin_family = AF_INET;
  return 0;
}
/*! \brief Initialize a `sockaddr_in` structure from strings containing an IPv4 address and (optionally) a port.
    \sa libuv API documentation: [`uv_ip4_addr()`](http://docs.libuv.org/en/v1.x/misc.html#c.uv_ip4_addr),
                                 [`uv_inet_pton()`](http://docs.libuv.org/en/v1.x/misc.html#c.uv_inet_pton).
    \sa Linux: [`inet_pton()`](http://man7.org/linux/man-pages/man3/inet_pton.3.html).\n
        Windows: [`RtlIpv4StringToAddressEx()`](https://msdn.microsoft.com/en-us/library/aa814459(v=vs.85).aspx),
                 [`InetPton()`](https://msdn.microsoft.com/en-us/library/cc805844(v=vs.85).aspx). */
inline int init(::sockaddr_in &_sin, const char *_ip, const char *_port = nullptr)
{
  int pnum = _port ? std::atoi(_port) : 0;
  return ::uv_ip4_addr(_ip, pnum, &_sin);
}

//! \}


//! \name sockaddr_in6
//! \{

/*! \brief Initialize a `sockaddr_in6` structure. */
inline int init(::sockaddr_in6 &_sin6)
{
  std::memset(&_sin6, 0, sizeof(_sin6));
  _sin6.sin6_family = AF_INET6;
  return 0;
}
/*! \brief Initialize a `sockaddr_in6` structure from strings containing an IPv6 address and (optionally) a port.
    \sa libuv API documentation: [`uv_ip6_addr()`](http://docs.libuv.org/en/v1.x/misc.html#c.uv_ip6_addr),
                                 [`uv_inet_pton()`](http://docs.libuv.org/en/v1.x/misc.html#c.uv_inet_pton).
    \sa Linux: [`inet_pton()`](http://man7.org/linux/man-pages/man3/inet_pton.3.html).\n
        Windows: [`RtlIpv6StringToAddressEx()`](https://msdn.microsoft.com/en-us/library/aa814463(v=vs.85).aspx),
                 [`InetPton()`](https://msdn.microsoft.com/en-us/library/cc805844(v=vs.85).aspx). */
inline int init(::sockaddr_in6 &_sin6, const char *_ip, const char *_port = nullptr)
{
  int pnum = _port ? std::atoi(_port) : 0;
  return ::uv_ip6_addr(_ip, pnum, &_sin6);
}

//! \}


//! \name sockaddr_storage
//! \{

inline int init(::sockaddr_storage &_ss, decltype(::sockaddr::sa_family) _family = AF_UNSPEC)
{
  std::memset(&_ss, 0, sizeof(_ss));
  _ss.ss_family = _family;
  return 0;
}

inline int init(::sockaddr_storage &_ss, const ::sockaddr &_sa)
{
  std::memset(&_ss, 0, sizeof(_ss));
  switch (_sa.sa_family)
  {
  case AF_INET:
      reinterpret_cast< ::sockaddr_in& >(_ss) = reinterpret_cast< const ::sockaddr_in& >(_sa);
      break;
  case AF_INET6:
      reinterpret_cast< ::sockaddr_in6& >(_ss) = reinterpret_cast< const ::sockaddr_in6& >(_sa);
      break;
  default:
      return UV_EAFNOSUPPORT;
  }
  return 0;
}

inline int init(::sockaddr_storage &_ss, const char *_ip, const char *_port = nullptr)
{
  std::memset(&_ss, 0, sizeof(_ss));
  return (
      init(reinterpret_cast< ::sockaddr_in& >(_ss), _ip, _port) == 0
      or
      init(reinterpret_cast< ::sockaddr_in6& >(_ss), _ip, _port) == 0
  ) ? 0 : UV_EINVAL;
}

//! \}


//! \name addrinfo
//! \{

/*! \brief Initialize an `addrinfo` structure for to be used as a hints argument in `uv::getaddrinfo` request.
    \details
    The `_family` argument might be AF_UNSPEC or AF_INET or AF_INET6 or (Windows only) AF_NETBIOS.

    The `_socktype` argument might be SOCK_DGRAM or SOCK_STREAM.

    The `_flags` argument can be a combination of the following values:
    - AI_ADDRCONFIG
    - AI_ALL
    - AI_CANONNAME
    - AI_NUMERICHOST
    - AI_NUMERICSERV
    - AI_PASSIVE
    - AI_V4MAPPED
    .
    Linux only - extensions for Internationalized Domain Names:
    - AI_CANONIDN
    - AI_IDN
    - AI_IDN_ALLOW_UNASSIGNED
    - AI_IDN_USE_STD3_ASCII_RULES
    .
    Windows only:
    - AI_DISABLE_IDN_ENCODING
    - AI_FILESERVER
    - AI_FLAGS
    - AI_FQDN
    - AI_NON_AUTHORITATIVE
    - AI_RETURN_PREFERRED_NAMES
    - AI_SECURE
    .
    \sa Linux: [`getaddrinfo()`](http://man7.org/linux/man-pages/man3/getaddrinfo.3.html).\n
        Windows: [`addrinfo`](https://msdn.microsoft.com/en-us/library/ms737530(v=vs.85).aspx),
                 [`getaddrinfo()`](https://msdn.microsoft.com/en-us/library/ms738520(v=vs.85).aspx),
                 [`GetAddrInfoEx()`](https://msdn.microsoft.com/en-us/library/ms738518(v=vs.85).aspx). */
inline int init(::addrinfo &_ai,
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
