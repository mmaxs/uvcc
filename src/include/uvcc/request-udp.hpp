
#ifndef UVCC_REQUEST_UDP__HPP
#define UVCC_REQUEST_UDP__HPP

#include "uvcc/utility.hpp"
#include "uvcc/request-base.hpp"
#include "uvcc/handle-udp.hpp"
#include "uvcc/buffer.hpp"

#include <uv.h>
#include <functional>   // function


namespace uv
{


/*! \ingroup doxy_group_request
    \brief UDP send request type.
    \sa libuv API documentation: [`uv_udp_t — UDP handle`](http://docs.libuv.org/en/v1.x/udp.html#uv-udp-t-udp-handle). */
class udp_send : public request
{
  //! \cond
  friend class request::instance< udp_send >;
  //! \endcond

public: /*types*/
  using uv_t = ::uv_udp_send_t;
  using on_request_t = std::function< void(udp_send _request, buffer _buffer) >;
  /*!< \brief The function type of the callback called after data was sent.
       \sa libuv API documentation: [`uv_udp_send_cb`](http://docs.libuv.org/en/v1.x/udp.html#c.uv_udp_send_cb). */

protected: /*types*/
  //! \cond
  struct properties
  {
    buffer::uv_t *uv_buf = nullptr;
    ::sockaddr_storage peer = { 0,};
  };
  //! \endcond

private: /*types*/
  using instance = request::instance< udp_send >;

public: /*constructors*/
  ~udp_send() = default;

  udp_send(const udp_send&) = default;
  udp_send& operator =(const udp_send&) = default;

  udp_send(udp_send&&) noexcept = default;
  udp_send& operator =(udp_send&&) noexcept = default;

public: /*interface*/
  const on_request_t& on_request() const noexcept  { return instance::from(uv_req)->request_cb_storage.value(); }
        on_request_t& on_request()       noexcept  { return instance::from(uv_req)->request_cb_storage.value(); }

  /*! \brief UDP handle where this send request is taking place. */
  udp handle() const noexcept  { return udp(static_cast< uv_t* >(uv_req)->handle); }

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_req); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_req); }
};


}


#endif
