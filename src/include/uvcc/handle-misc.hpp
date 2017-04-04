
#ifndef UVCC_HANDLE_MISC__HPP
#define UVCC_HANDLE_MISC__HPP

#include "uvcc/utility.hpp"
#include "uvcc/handle-base.hpp"

#include <uv.h>


namespace uv
{


/*! \ingroup doxy_group__handle
    \brief Async handle.
    \sa libuv API documentation: [`uv_async_t — Async handle`](http://docs.libuv.org/en/v1.x/async.html#uv-async-t-async-handle). */
class async : public handle
{
public: /*types*/
  template< typename... _Args_ >
  using on_send_t = std::function< void(async _handle, _Args_&&... _args) >;
  /*!< */
};


}


#endif
