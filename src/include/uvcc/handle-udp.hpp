
#ifndef UVCC_HANDLE_UDP__HPP
#define UVCC_HANDLE_UDP__HPP

#include "uvcc/utility.hpp"
#include "uvcc/handle-base.hpp"

#include <uv.h>


namespace uv
{


/*! \ingroup doxy_handle
    \brief UDP handle type.
    \sa libuv API documentation: [`uv_udp_t`](http://docs.libuv.org/en/v1.x/udp.html#uv-udp-t-udp-handle). */
class udp : public handle
{
  friend class handle::instance< udp >;

public: /*types*/
  using uv_t = ::uv_udp_t;
  using on_recv_t = std::function< void(udp _udp, ssize_t _nread, buffer _buffer, const ::sockaddr *_sa, unsigned int _flags) >;
  /*!< \brief The function type of the callback called by `recv_start()`.
       \sa libuv API documentation: [`uv_udp_recv_cb`](http://docs.libuv.org/en/v1.x/udp.html#c.uv_udp_recv_cb). */

private: /*types*/
  using instance = handle::instance< udp >;

public: /*constructors*/
  ~udp() = default;

  udp(const udp&) = default;
  udp& operator =(const udp&) = default;

  udp(udp&&) noexcept = default;
  udp& operator =(udp&&) noexcept = default;

public: /*interface*/
  ::uv_os_sock_t socket() const noexcept  { return (::uv_os_sock_t)fileno(); }

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_handle); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_handle); }
};


}


#endif
