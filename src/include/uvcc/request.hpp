
#ifndef UVCC_REQUEST__HPP
#define UVCC_REQUEST__HPP

#include "uvcc/request-base.hpp"
#include "uvcc/request-stream.hpp"
#include "uvcc/request-fs.hpp"
//#include "uvcc/request-udp.hpp"
//#include "uvcc/request-dns.hpp"
//#include "uvcc/request-misc.hpp"

#include <uv.h>


namespace uv
{
/*! \defgroup doxy_group_request Requests
    \brief The classes representing libuv requests. */


/* libuv requests */
class request;
class connect;
class write;
class shutdown;
class udp_send;
class fs;
class work;
class getaddrinfo;
class getnameinfo;



/*! \defgroup doxy_group_request_traits uv_req_traits< typename >
    \ingroup doxy_group_request
    \brief The correspondence between libuv request data types and C++ classes representing them. */
//! \{
//! \cond
template< typename _UV_T_ > struct uv_req_traits  {};
//! \endcond
#define req request  // redefine the UV_REQ_TYPE_MAP() entry
#define XX(X, x) template<> struct uv_req_traits< uv_##x##_t >  { using type = x; };
UV_REQ_TYPE_MAP(XX)
#undef XX
#undef req
//! \}


}


#endif
