
#ifndef UVCC_HANDLE__HPP
#define UVCC_HANDLE__HPP

#include "uvcc/handle-base.hpp"
#include "uvcc/handle-stream.hpp"
#include "uvcc/handle-fs.hpp"
#include "uvcc/handle-udp.hpp"
#include "uvcc/handle-misc.hpp"

#include <uv.h>


namespace uv
{
/*! \defgroup doxy_handle Handles
    \brief The classes representing libuv handles. */


/* libuv handles */
class async;
class check;
class fs_event;
class fs_poll;
class handle;
class idle;
class pipe;
class poll;
class prepare;
class process;
class stream;
class tcp;
class timer;
class tty;
class udp;
class signal;



/*! \defgroup doxy_handle_traits uv_handle_traits< typename >
    \ingroup doxy_handle
    \brief The correspondence between libuv handle data types and C++ classes representing them. */
//! \{
//! \cond
template< typename _UV_T_ > struct uv_handle_traits  {};
//! \endcond
#define XX(X, x) template<> struct uv_handle_traits< uv_##x##_t > { using type = x; };
UV_HANDLE_TYPE_MAP(XX)
#undef XX
//! \}


}


#endif
