
#ifndef UVCC_SOCKADDR__HPP
#define UVCC_SOCKADDR__HPP

#include <cstring>       // memset()
#ifdef _WIN32
#include <Winsock2.h>    // sockaddr_storage sockaddr_in sockaddr_in6
#include <Ws2tcpip.h>    // addrinfo
#else
#include <sys/socket.h>  // sockaddr_storage
#include <netinet/in.h>  // sockaddr_in sockaddr_in6
#include <netdb.h>       // addrinfo
#endif


namespace uv
{
/*! \defgroup g__netstruct Network related structures initialization
    \ingroup g__utility */
//! \{


//! \cond
template< typename _T_, typename... _Args_ > _T_& init(_T_&, _Args_...);
//! \endcond

/*! \sa Linux: [`sockaddr_storage`](http://man7.org/linux/man-pages/man7/socket.7.html)
    \sa Windows: [SOCKADDR_STORAGE structure](https://msdn.microsoft.com/en-us/library/ms740504(v=vs.85).aspx) */
template<> ::sockaddr_storage& init(::sockaddr_storage &_sockaddr, decltype(::sockaddr_storage::ss_family) _family)
{
  std::memset(&_sockaddr, 0, sizeof(_sockaddr));
  _sockaddr.ss_family = _family;
  return _sockaddr;
}

/*! \sa Linux: [`sockaddr_in`](http://man7.org/linux/man-pages/man7/ip.7.html)
    \sa Windows: [`sockaddr_in`](https://msdn.microsoft.com/en-us/library/ms740496(v=vs.85).aspx) */
template<> ::sockaddr_in& init(::sockaddr_in &_sockaddr)
{
  std::memset(&_sockaddr, 0, sizeof(_sockaddr));
  _sockaddr.sin_family = AF_INET;
  return _sockaddr;
}

/*! \sa Linux: [`sockaddr_in6`](http://man7.org/linux/man-pages/man7/ipv6.7.html)
    \sa Windows: [`sockaddr_in6`](https://msdn.microsoft.com/en-us/library/ms740496(v=vs.85).aspx) */
template<> ::sockaddr_in6& init(::sockaddr_in6 &_sockaddr)
{
  std::memset(&_sockaddr, 0, sizeof(_sockaddr));
  _sockaddr.sin6_family = AF_INET6;
  return _sockaddr;
}

/*! \sa Linux: [`addrinfo`](http://man7.org/linux/man-pages/man3/getaddrinfo.3.html)
    \sa Windows: [addrinfo structure](https://msdn.microsoft.com/en-us/library/ms737530(v=vs.85).aspx) */
template<> ::addrinfo& init(::addrinfo &_addrinfo)
{
  std::memset(&_addrinfo, 0, sizeof(_addrinfo));
  return _addrinfo;
}


//! \}
}

#endif
