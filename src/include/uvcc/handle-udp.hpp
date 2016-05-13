
#ifndef UVCC_HANDLE_UDP__HPP
#define UVCC_HANDLE_UDP__HPP

#include "uvcc/utility.hpp"
#include "uvcc/handle-base.hpp"
#include "uvcc/handle-io.hpp"

#include <uv.h>
#include <cstddef>      // size_t
#include <type_traits>  // enable_if_t


namespace uv
{


/*! \ingroup doxy_group_handle
    \brief UDP handle type.
    \sa libuv API documentation: [`uv_udp_t — UDP handle`](http://docs.libuv.org/en/v1.x/udp.html#uv-udp-t-udp-handle). */
class udp : public io
{
  //! \cond
  friend class handle::instance< udp >;
  //! \endcond

public: /*types*/
  using uv_t = ::uv_udp_t;
  //using on_recv_t = std::function< void(udp _udp, ssize_t _nread, buffer _buffer, const ::sockaddr *_sa, unsigned int _flags) >;
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
  /*! \brief Get the platform dependent socket descriptor. The alias for `handle::fileno()`. */
  ::uv_os_sock_t socket() const noexcept  { return (::uv_os_sock_t)fileno(); }

  /*! \brief Number of bytes queued for sending. This field strictly shows how much information is currently queued. */
  std::size_t send_queue_size() const noexcept  { return static_cast< uv_t* >(uv_handle)->send_queue_size; }
  /*! \brief Number of send requests currently in the queue awaiting to be processed. */
  std::size_t send_queue_count() const noexcept  { return static_cast< uv_t* >(uv_handle)->send_queue_count; }

  /*! \name UDP multicast/broadcast facilities: */
  //! \{
  /*! \brief Set membership for a multicast address.
      \sa libuv API documentation: [`uv_udp_set_membership()`](http://docs.libuv.org/en/v1.x/udp.html#c.uv_udp_set_membership),
                                   [`uv_membership`](http://docs.libuv.org/en/v1.x/udp.html#c.uv_membership). */
  int set_multicast_membership(const char *_multicast_addr, const char *_interface_addr, ::uv_membership _membership) noexcept
  {
    return uv_status(::uv_udp_set_membership(
        static_cast< uv_t* >(uv_handle),
        _multicast_addr, _interface_addr, _membership
    ));
  }
  /*! \brief Set IP multicast loop flag. Makes multicast packets loop back to local sockets.
      \sa libuv API documentation: [`uv_udp_set_multicast_loop()`](http://docs.libuv.org/en/v1.x/udp.html#c.uv_udp_set_multicast_loop). */
  int set_multicast_loop(bool _enable) noexcept
  { return uv_status(::uv_udp_set_multicast_loop(static_cast< uv_t* >(uv_handle), _enable)); }
  /*! \brief Set the multicast TTL value.
      \sa libuv API documentation: [`uv_udp_set_multicast_ttl()`](http://docs.libuv.org/en/v1.x/udp.html#c.uv_udp_set_multicast_ttl). */
  int set_multicast_ttl(int _value) noexcept
  { return uv_status(::uv_udp_set_multicast_ttl(static_cast< uv_t* >(uv_handle), _value)); }
  /*! \brief Set the multicast interface to send or receive data on.
      \sa libuv API documentation: [`uv_udp_set_multicast_interface()`](http://docs.libuv.org/en/v1.x/udp.html#c.uv_udp_set_multicast_interface). */
  int set_multicast_interface(const char *_interface_addr) noexcept
  { return uv_status(::uv_udp_set_multicast_interface(static_cast< uv_t* >(uv_handle), _interface_addr)); }
  /*! \brief Set broadcast on or off.
      \sa libuv API documentation: [`uv_udp_set_broadcast()`](http://docs.libuv.org/en/v1.x/udp.html#c.uv_udp_set_broadcast). */
  int set_broadcast(bool _enable) noexcept
  { return uv_status(::uv_udp_set_broadcast(static_cast< uv_t* >(uv_handle), _enable)); }
  //! \}

  /*! \details Get the local address which this handle is bound to.
      \sa libuv API documentation: [`uv_udp_getsockname()`](http://docs.libuv.org/en/v1.x/udp.html#c.uv_udp_getsockname). */
  template< typename _T_, typename = std::enable_if_t< is_one_of< _T_, ::sockaddr_in, ::sockaddr_in6, ::sockaddr_storage >::value > >
  int getsockname(_T_ &_sockaddr) const noexcept
  {
    int z = sizeof(_T_);
    return uv_status(::uv_udp_getsockname(static_cast< uv_t* >(uv_handle), reinterpret_cast< ::sockaddr* >(&_sockaddr), &z));
  }

  /*! \brief Set the time to live value.
      \sa libuv API documentation: [`uv_udp_set_ttl()`](http://docs.libuv.org/en/v1.x/udp.html#c.uv_udp_set_ttl). */
  int set_ttl(int _value) noexcept  { return uv_status(::uv_udp_set_ttl(static_cast< uv_t* >(uv_handle), _value)); }

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_handle); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_handle); }
};


}


#endif
