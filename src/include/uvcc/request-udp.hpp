
#ifndef UVCC_REQUEST_UDP__HPP
#define UVCC_REQUEST_UDP__HPP

#include "uvcc/utility.hpp"
#include "uvcc/request-base.hpp"
#include "uvcc/handle-udp.hpp"
#include "uvcc/buffer.hpp"
#include "uvcc/netstruct.hpp"

#include <uv.h>
#include <functional>   // function
#include <type_traits>  // enable_if_t


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

private: /*constructors*/
  explicit udp_send(uv_t *_uv_req)
  {
    if (_uv_req)  instance::from(_uv_req)->ref();
    uv_req = _uv_req;
  }

public: /*constructors*/
  ~udp_send() = default;
  udp_send()  { uv_req = instance::create(); }

  udp_send(const udp_send&) = default;
  udp_send& operator =(const udp_send&) = default;

  udp_send(udp_send&&) noexcept = default;
  udp_send& operator =(udp_send&&) noexcept = default;

private: /*functions*/
  template< typename = void > static void udp_send_cb(::uv_udp_send_t*, int);

public: /*interface*/
  const on_request_t& on_request() const noexcept  { return instance::from(uv_req)->request_cb_storage.value(); }
        on_request_t& on_request()       noexcept  { return instance::from(uv_req)->request_cb_storage.value(); }

  /*! \brief UDP handle where this send request has been taking place. */
  udp handle() const noexcept  { return udp(static_cast< uv_t* >(uv_req)->handle); }
  /*! \brief Get the address of the remote peer which this request has sent data to.
      \returns `true` if `sizeof(_T_)` is enough to hold the returned socket address structure. */
  template< typename _T_, typename = std::enable_if_t< is_one_of< _T_, ::sockaddr_in, ::sockaddr_in6, ::sockaddr_storage >::value > >
  bool getpeername(_T_ &_sockaddr) const noexcept
  {
    _sockaddr = reinterpret_cast< _T_& >(instance::from(uv_req)->properties().peer);
    switch (reinterpret_cast< ::sockaddr >(_sockaddr).sa_family)
    {
      case AF_INET:   return sizeof(_T_) >= sizeof(::sockaddr_in);
      case AF_INET6:  return sizeof(_T_) >= sizeof(::sockaddr_in6);
      default:        return sizeof(_T_) == sizeof(::sockaddr_storage);
    }
  }

  /*! \brief Run the request.
      \details Send data over the UDP socket. If the socket has not previously been bound with `udp::bind()`
      it will be bound to 0.0.0.0 (the “all interfaces” IPv4 address) and a random port number.
      \sa libuv API documentation: [`uv_udp_send()`](http://docs.libuv.org/en/v1.x/udp.html#c.uv_udp_send). */
  template<
      typename _T_,
      typename = std::enable_if_t< is_one_of< _T_, ::sockaddr, ::sockaddr_in, ::sockaddr_in6, ::sockaddr_storage >::value >
  >
  int run(udp _udp, const buffer &_buf, const _T_ &_sockaddr)
  {
    auto instance_ptr = instance::from(uv_req);

    udp::instance::from(_udp.uv_handle)->ref();
    buffer::instance::from(_buf.uv_buf)->ref();
    instance_ptr->ref();

    {
      auto &properties = instance_ptr->properties();
      properties.uv_buf = _buf.uv_buf;
      init(properties.peer, reinterpret_cast< const ::sockaddr& >(_sockaddr));
    }


    uv_status(0);
    int ret = ::uv_udp_send(
        static_cast< uv_t* >(uv_req), static_cast< udp::uv_t* >(_udp),
        static_cast< const buffer::uv_t* >(_buf), _buf.count(),
        reinterpret_cast< const ::sockaddr* >(&_sockaddr),
        udp_send_cb
    );
    if (!ret)  uv_status(ret);
    return ret;
  }

  /*! \details The wrapper for a corresponding libuv function.
      \note It tries to execute and complete immediately and does not call the request callback.
      \sa libuv API documentation: [`uv_udp_try_send()`](http://docs.libuv.org/en/v1.x/udp.html#c.uv_udp_try_send). */
  template<
      typename _T_,
      typename = std::enable_if_t< is_one_of< _T_, ::sockaddr, ::sockaddr_in, ::sockaddr_in6, ::sockaddr_storage >::value >
  >
  int try_send(udp _udp, const buffer &_buf, const _T_ &_sockaddr)
  {
    return uv_status(::uv_udp_try_send(
        static_cast< udp::uv_t* >(_udp),
        static_cast< const buffer::uv_t* >(_buf), _buf.count(),
        reinterpret_cast< const ::sockaddr* >(&_sockaddr)
    ));
  }

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_req); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_req); }
};

template< typename >
void udp_send::udp_send_cb(::uv_udp_send_t *_uv_req, int _status)
{
  auto instance_ptr = instance::from(_uv_req);
  instance_ptr->uv_error = _status;

  ref_guard< udp::instance > unref_handle(*udp::instance::from(_uv_req->handle), adopt_ref);
  ref_guard< instance > unref_req(*instance_ptr, adopt_ref);

  auto &udp_send_cb = instance_ptr->request_cb_storage.value();
  if (udp_send_cb)
    udp_send_cb(udp_send(_uv_req), buffer(instance_ptr->properties().uv_buf, adopt_ref));
  else
    buffer::instance::from(instance_ptr->properties().uv_buf)->unref();
}


}


#endif
