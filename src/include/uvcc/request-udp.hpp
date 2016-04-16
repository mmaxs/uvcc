
#ifndef UVCC_REQUEST_UDP__HPP
#define UVCC_REQUEST_UDP__HPP

#include "uvcc/utility.hpp"
#include "uvcc/request-base.hpp"
#include "uvcc/handle-udp.hpp"

#include <uv.h>
#include <functional>   // function


namespace uv
{


/*! \ingroup doxy_request
    \brief ...
    \sa libuv API documentation: [`uv_udp_t`](http://docs.libuv.org/en/v1.x/udp.html#uv-udp-t-udp-handle). */
class udp_send : public request
{
  friend class request::instance< udp_send >;

public: /*types*/
  using uv_t = ::uv_udp_send_t;
  using on_request_t = std::function< void(int) >;

private: /*types*/
  using instance = request::instance< udp_send >;

public: /*constructors*/
  ~udp_send() = default;

  udp_send(const udp_send&) = default;
  udp_send& operator =(const udp_send&) = default;

  udp_send(udp_send&&) noexcept = default;
  udp_send& operator =(udp_send&&) noexcept = default;

public: /*interface*/
  const on_request_t& on_request() const noexcept  { return instance::from(uv_req)->on_request(); }
        on_request_t& on_request()       noexcept  { return instance::from(uv_req)->on_request(); }

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_req); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_req); }
};


}


#endif
