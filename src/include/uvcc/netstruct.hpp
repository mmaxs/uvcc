
#ifndef UVCC_NETSTRUCT__HPP
#define UVCC_NETSTRUCT__HPP

#include <uv.h>
#include <cstring>       // memset()
#include <cstdlib>       // atoi()
#if 0
#ifdef _WIN32
#include <Winsock2.h>    // sockaddr_storage sockaddr_in sockaddr_in6
#include <Ws2tcpip.h>    // addrinfo
#else
#include <sys/socket.h>  // sockaddr_storage
#include <netinet/in.h>  // sockaddr_in sockaddr_in6
#include <netdb.h>       // addrinfo
#endif
#endif


namespace uv
{
/*! \defgroup g__netstruct Network-related structures initialization
    \ingroup g__utility */
//! \{


//! \cond
template< typename _T_, typename... _Args_ > int init(_T_&, _Args_...);
//! \endcond


/*! \name sockaddr_storage
    \sa Linux: [`sockaddr_storage`](http://man7.org/linux/man-pages/man7/socket.7.html).
        Windows: [`sockaddr_storage`](https://msdn.microsoft.com/en-us/library/ms740504(v=vs.85).aspx). */
//! \{

template<> int init(::sockaddr_storage &_sa, decltype(::sockaddr_storage::ss_family) _family)
{
  std::memset(&_sa, 0, sizeof(_sa));
  _sa.ss_family = _family;
  return 0;
}

//! \}


/*! \name sockaddr_in
    \sa Linux: [`sockaddr_in`](http://man7.org/linux/man-pages/man7/ip.7.html).
        Windows: [`sockaddr_in`](https://msdn.microsoft.com/en-us/library/ms740496(v=vs.85).aspx). */
//! \{

/*! \brief Initialize a `sockaddr_in` structure. */
template<> int init(::sockaddr_in &_sa)
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
template<> int init(::sockaddr_in &_sa, const char *_addr, const char *_port = nullptr)
{
  int pnum = _port ? std::atoi(_port) : 0;
  return ::uv_ip4_addr(_addr, pnum, &_sa);
}

//! \}


/*! \name sockaddr_in6
    \sa Linux: [`sockaddr_in6`](http://man7.org/linux/man-pages/man7/ipv6.7.html).
        Windows: [`sockaddr_in6`](https://msdn.microsoft.com/en-us/library/ms740496(v=vs.85).aspx). */
//! \{

/*! \brief Initialize a `sockaddr_in6` structure. */
template<> int init(::sockaddr_in6 &_sa)
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
template<> int init(::sockaddr_in6 &_sa, const char *_addr, const char *_port = nullptr)
{
  int pnum = _port ? std::atoi(_port) : 0;
  return ::uv_ip6_addr(_addr, pnum, &_sa);
}

//! \}


/*! \name addrinfo
    \sa Linux: [`addrinfo`](http://man7.org/linux/man-pages/man3/getaddrinfo.3.html).
        Windows: [`addrinfo`](https://msdn.microsoft.com/en-us/library/ms737530(v=vs.85).aspx). */
//! \{

template<> int init(::addrinfo &_ai)
{
  std::memset(&_ai, 0, sizeof(_ai));
  return 0;
}

//! \}


//! \}
}

#endif
