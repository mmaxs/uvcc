
#ifndef UVCC_SOCKADDR__HPP
#define UVCC_SOCKADDR__HPP

#include <cstring>       // memset()
#ifdef _WIN32
#include <Winsock2.h>    // sockaddr_in sockaddr_in6
#else
#include <netinet/in.h>  // sockaddr_in sockaddr_in6
#endif


namespace uv
{
/*! \defgroup g__sockaddr Utility functions for socket address structure initialization
    \ingroup g__utility */
//! \{


//! \cond
template< typename _T_ > _T_& reset(_T_&);
//! \endcond

template<> ::sockaddr_in& reset(::sockaddr_in &_sa)
{
  std::memset(&_sa, 0, sizeof(_sa));
  _sa.sin_family = AF_INET;
  return _sa;
}

template<> ::sockaddr_in6& reset(::sockaddr_in6 &_sa)
{
  std::memset(&_sa, 0, sizeof(_sa));
  _sa.sin6_family = AF_INET6;
  return _sa;
}


template< typename _T_, typename... _Ts_ > void reset(_T_& _sa, _Ts_&... _args)
{
  reset(_sa);
  reset(_args...);
}


//! \}
}

#endif
