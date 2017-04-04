
#ifndef UVCC_HANDLE_UDP__HPP
#define UVCC_HANDLE_UDP__HPP

#include "uvcc/utility.hpp"
#include "uvcc/handle-base.hpp"
#include "uvcc/handle-io.hpp"
#include "uvcc/loop.hpp"

#include <cstddef>      // size_t
#include <uv.h>

#include <type_traits>  // enable_if_t


namespace uv
{


/*! \ingroup doxy_group__handle
    \brief UDP handle type.
    \sa libuv API documentation: [`uv_udp_t — UDP handle`](http://docs.libuv.org/en/v1.x/udp.html#uv-udp-t-udp-handle). */
class udp : public io
{
  friend class handle::instance< udp >;
  friend class udp_send;

public: /*types*/
  using uv_t = ::uv_udp_t;

  /*! \brief Supplemental data passed as the last argument to `io::on_read_t` callback function
      called by `recv_start()`.
      \sa libuv API documentation: [`uv_udp_recv_cb`](http://docs.libuv.org/en/v1.x/udp.html#c.uv_udp_recv_cb). */
  struct io_info
  {
    /*! \brief The address of the remote peer. Can be `nullptr`. The pointer is valid for the duration of the callback only.
        \note If the receive callback is called with zero number of bytes that have been received, then `(peer == nullptr)`
        indicates that there is nothing to read, and `(peer != nullptr)` indicates an empty UDP packet is received. */
    const ::sockaddr *peer;
    /*! \brief One or more or’ed [`uv_udp_flags`](http://docs.libuv.org/en/v1.x/udp.html#c.uv_udp_flags) constants.
        \sa libuv API documentation: [`uv_udp_recv_cb`](http://docs.libuv.org/en/v1.x/udp.html#c.uv_udp_recv_cb),
                                     [`uv_udp_flags`](http://docs.libuv.org/en/v1.x/udp.html#c.uv_udp_flags). */
    unsigned int flags;
  };

protected: /*types*/
  //! \cond
  struct properties : io::properties  {};
  //! \endcond

  //! \cond
  struct uv_interface : handle::uv_handle_interface, io::uv_interface
  {
    static uv_interface& instance()  { static uv_interface instance;  return instance; }

    std::size_t write_queue_size(void *_uv_handle) const noexcept override
    { return static_cast< ::uv_udp_t* >(_uv_handle)->send_queue_size; }

    int read_start(void *_uv_handle, int64_t _offset) const noexcept override
    {
      if (_offset >= 0)  instance::from(_uv_handle)->properties().rdoffset = _offset;
      return ::uv_udp_recv_start(static_cast< ::uv_udp_t* >(_uv_handle), alloc_cb, recv_cb);
    }

    int read_stop(void *_uv_handle) const noexcept override
    { return ::uv_udp_recv_stop(static_cast< ::uv_udp_t* >(_uv_handle)); }
  };
  //! \endcond

private: /*types*/
  using instance = handle::instance< udp >;

protected: /*constructors*/
  //! \cond
  explicit udp(uv_t *_uv_handle) : io(static_cast< io::uv_t* >(_uv_handle))  {}
  //! \endcond

public: /*constructors*/
  ~udp() = default;

  udp(const udp&) = default;
  udp& operator =(const udp&) = default;

  udp(udp&&) noexcept = default;
  udp& operator =(udp&&) noexcept = default;

  /*! \details Create a UDP socket with the specified flags.
      \note With `AF_UNSPEC` flag no socket is actually created on the system.
      \sa libuv API documentation: [`uv_udp_init_ex()`](http://docs.libuv.org/en/v1.x/udp.html#c.uv_udp_init_ex).\n
          libuv enhancement proposals: <https://github.com/libuv/leps/blob/master/003-create-sockets-early.md>. */
  udp(uv::loop &_loop, unsigned int _flags = AF_UNSPEC)
  {
    uv_handle = instance::create();
    uv_status(::uv_udp_init_ex(static_cast< uv::loop::uv_t* >(_loop), static_cast< uv_t* >(uv_handle), _flags));
    instance::from(uv_handle)->book_loop();
  }
  /*! \details Create a handle object from an existing native platform depended datagram socket descriptor.
      \sa libuv API documentation: [`uv_udp_open()`](http://docs.libuv.org/en/v1.x/udp.html#c.uv_udp_open),
                                   [`uv_udp_init()`](http://docs.libuv.org/en/v1.x/udp.html#c.uv_udp_init). */
  udp(uv::loop &_loop, ::uv_os_sock_t _socket)
  {
    uv_handle = instance::create();

    auto uv_err = ::uv_udp_init(static_cast< uv::loop::uv_t* >(_loop), static_cast< uv_t* >(uv_handle));
    if (uv_status(uv_err) < 0)  return;

    instance::from(uv_handle)->book_loop();

    uv_status(::uv_udp_open(static_cast< uv_t* >(uv_handle), _socket));
  }

private: /*functions*/
  template< typename = void > static void alloc_cb(::uv_handle_t*, std::size_t, ::uv_buf_t*);
  template< typename = void > static void recv_cb(::uv_udp_t*, ssize_t, const ::uv_buf_t*, const ::sockaddr*, unsigned int);

public: /*interface*/
  /*! \brief Get the platform dependent socket descriptor. The alias for `handle::fileno()`. */
  ::uv_os_sock_t socket() const noexcept  { return (::uv_os_sock_t)fileno(); }

  /*! \brief Number of bytes queued for sending. This field strictly shows how much information is currently queued. */
  std::size_t send_queue_size() const noexcept  { return static_cast< uv_t* >(uv_handle)->send_queue_size; }
  /*! \brief Number of send requests currently in the queue awaiting to be processed. */
  std::size_t send_queue_count() const noexcept  { return static_cast< uv_t* >(uv_handle)->send_queue_count; }

  /*! \name UDP multicast/broadcast features: */
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

  /*! \details Bind the handle to an address and port.
      \sa libuv API documentation: [`uv_udp_bind()`](http://docs.libuv.org/en/v1.x/udp.html#c.uv_udp_bind),
                                   [`uv_udp_flags`](http://docs.libuv.org/en/v1.x/udp.html#c.uv_udp_flags). */
  template<
      typename _T_,
      typename = std::enable_if_t< is_one_of< _T_, ::sockaddr, ::sockaddr_in, ::sockaddr_in6, ::sockaddr_storage >::value >
  >
  int bind(const _T_ &_sockaddr, unsigned int _flags = 0) noexcept
  {
    return uv_status(::uv_udp_bind(static_cast< uv_t* >(uv_handle), reinterpret_cast< const ::sockaddr* >(&_sockaddr), _flags));
  }

  /*! \brief Get the local address and port which this handle is bound to.
      \returns `true` if the operation has completed successfully (can be checked with `uv_status()`) and
      the size of the passed argument (i.e. `sizeof(_T_)`) is enough to hold the returned socket address structure.
      \sa libuv API documentation: [`uv_udp_getsockname()`](http://docs.libuv.org/en/v1.x/udp.html#c.uv_udp_getsockname). */
  template< typename _T_, typename = std::enable_if_t< is_one_of< _T_, ::sockaddr_in, ::sockaddr_in6, ::sockaddr_storage >::value > >
  bool getsockname(_T_ &_sockaddr) const noexcept
  {
    int z = sizeof(_T_);
    return
        uv_status(::uv_udp_getsockname(static_cast< uv_t* >(uv_handle), reinterpret_cast< ::sockaddr* >(&_sockaddr), &z)) >= 0
      and
        sizeof(_T_) >= z;
  }

  /*! \brief Set the time to live value.
      \sa libuv API documentation: [`uv_udp_set_ttl()`](http://docs.libuv.org/en/v1.x/udp.html#c.uv_udp_set_ttl). */
  int set_ttl(int _value) noexcept  { return uv_status(::uv_udp_set_ttl(static_cast< uv_t* >(uv_handle), _value)); }

  /*! \brief Alias for `io::read_start()`. */
  int recv_start(const on_buffer_alloc_t &_alloc_cb, const on_read_t &_recv_cb, std::size_t _size = 0) const
  { return read_start(_alloc_cb, _recv_cb, _size); }
  /*! \brief Idem. */
  int recv_start(std::size_t _size = 0) const  { return read_start(_size); }
  /*! \brief Alias for `io::read_stop()`. */
  int recv_stop() const  { return read_stop(); }

#if 1
  /*! \brief _Get_ the size of the send buffer that the operating system uses for the socket.
      \sa libuv API documentation: [`uv_send_buffer_size()`](http://docs.libuv.org/en/v1.x/handle.html#c.uv_send_buffer_size). */
  unsigned int send_buffer_size() const noexcept
  {
    unsigned int v = 0;
    uv_status(::uv_send_buffer_size(static_cast< handle::uv_t* >(uv_handle), (int*)&v));
    return v;
  }
  /*! \brief _Set_ the size of the send buffer that the operating system uses for the socket.
      \sa libuv API documentation: [`uv_send_buffer_size()`](http://docs.libuv.org/en/v1.x/handle.html#c.uv_send_buffer_size). */
  void send_buffer_size(const unsigned int _value) noexcept  { uv_status(::uv_send_buffer_size(static_cast< handle::uv_t* >(uv_handle), (int*)&_value)); }

  /*! \brief _Get_ the size of the receive buffer that the operating system uses for the socket.
      \sa libuv API documentation: [`uv_recv_buffer_size()`](http://docs.libuv.org/en/v1.x/handle.html#c.uv_recv_buffer_size). */
  unsigned int recv_buffer_size() const noexcept
  {
    unsigned int v = 0;
    uv_status(::uv_recv_buffer_size(static_cast< handle::uv_t* >(uv_handle), (int*)&v));
    return v;
  }
  /*! \brief _Set_ the size of the receive buffer that the operating system uses for the socket.
      \sa libuv API documentation: [`uv_recv_buffer_size()`](http://docs.libuv.org/en/v1.x/handle.html#c.uv_recv_buffer_size). */
  void recv_buffer_size(const unsigned int _value) noexcept  { uv_status(::uv_recv_buffer_size(static_cast< handle::uv_t* >(uv_handle), (int*)&_value)); }
#endif

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_handle); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_handle); }
};

template< typename >
void udp::alloc_cb(::uv_handle_t *_uv_handle, std::size_t _suggested_size, ::uv_buf_t *_uv_buf)
{ io_alloc_cb(_uv_handle, _suggested_size, _uv_buf); }

template< typename >
void udp::recv_cb(::uv_udp_t *_uv_handle, ssize_t _nread, const ::uv_buf_t *_uv_buf, const ::sockaddr *_sockaddr, unsigned int _flags)
{
  io_info supplemental_data = { _sockaddr, _flags };
  io_read_cb(_uv_handle, _nread, _uv_buf, &supplemental_data);
}


}


#endif
