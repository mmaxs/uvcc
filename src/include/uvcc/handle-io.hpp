
#ifndef UVCC_HANDLE_IO__HPP
#define UVCC_HANDLE_IO__HPP

#include "uvcc/utility.hpp"
#include "uvcc/handle-base.hpp"
#include "uvcc/buffer.hpp"
#include "uvcc/loop.hpp"

#include <uv.h>
#include <cstddef>      // size_t
#include <functional>   // function
#include <string>       // string


namespace uv
{


/*! \ingroup doxy_handle
    \brief The base class for handles representing I/O endpoints. Encapsulates common I/O functions and properties. */
class io : public handle
{
  //! \cond
  friend class handle::instance< io >;
  //! \endcond

public: /*types*/
  using uv_t = null_t;
  using on_read_t = std::function< void(io _handle, ssize_t _nread, buffer _buffer) >;
  /*!< \brief The function type of the callback called by `read_start()` when data was read from an I/O endpoint.
       \note The libuv API calls user provided read callback functions with a _null-initialized_ `uv_buf_t`
       buffer structure (where `buf->base = nullptr` and `buf->len = 0`) on error and EOF and does not try to retrieve
       something from the [`uv_alloc_cb`](http://docs.libuv.org/en/v1.x/handle.html#c.uv_alloc_cb) callback in
       such a cases, so the uvcc `on_read_t` callback is supplied with a dummy _null-initialized_ `_buffer`. */

protected: /*types*/
  //! \cond
  struct uv_property : virtual handle::uv_property
  {
    on_buffer_t on_buffer;
    on_read_t on_read;
  };
  //! \endcond

private: /*types*/
  using instance = handle::instance< io >;

private: /*constructors*/
  explicit io(void *_ptr)
  {
    if (_ptr)  instance::from(_ptr)->ref();
    ptr = _ptr;
  }

protected: /*constructors*/
  io() noexcept = default;

public: /*constructors*/
  ~io() = default;

  io(const io&) = default;
  io& operator =(const io&) = default;

  io(io&&) noexcept = default;
  io& operator =(io&&) noexcept = default;

private: /*functions*/
  template< typename = void > static void alloc_cb(::uv_handle_t*, std::size_t, ::uv_buf_t*);
  template< typename = void > static void file_read_cb(::uv_fs_t*);
  template< typename = void > static void stream_read_cb(::uv_stream_t*, ssize_t, const ::uv_buf_t*);
  template< typename = void > static void udp_recv_cb(::uv_udp_t*, ssize_t, const ::uv_buf_t*, const ::sockaddr*, unsigned);

public: /*interface*/
  /*! \brief Start reading incoming data from the stream.
      \details The handle is tried to be set for reading if only nonempty `_alloc_cb` and `_read_cb` functions
      are  provided, or else `UV_EINVAL` is returned with no involving any libuv API or uvcc function.
      Repeated call to this function results in the automatic call to `read_stop()` firstly,
      and `_alloc_cb` function can be empty in this case, which means that it doesn't change from the previous call.
      \note This function adds an extra reference to the handle instance, which is released when the counterpart
      function `read_stop()` is called. */
  int read_start(const on_buffer_t &_alloc_cb, const on_read_t &_read_cb) const
  {
    if (!_read_cb)  return uv_status(UV_EINVAL);

    auto p = static_cast< instance* >(ptr);

    if (!_alloc_cb and !p->on_read)  return uv_status(UV_EINVAL);

    p->ref();  // first, make sure it would exist for the future _read_cb() calls until read_stop()
    if (p->on_read)  read_stop();

    p->on_buffer = _alloc_cb;
    if (_read_cb)  p->on_read = _read_cb;

    uv_status(0);
    int o = ::uv_read_start(static_cast< uv_t* >(uv_handle), alloc_cb, read_cb);
    if (!o)  uv_status(o);
    return o;
  }
  /*! \brief Stop reading data from the stream. */
  int read_stop() const
  {
    uv_status(::uv_read_stop(static_cast< uv_t* >(uv_handle)));

    auto self = instance::from(uv_handle);
    auto &read_cb = self->supplemental_data().on_read;
    if (read_cb)
    {
      read_cb = on_read_t();
      self->unref();  // release the excess reference from read_start()
    };

    return uv_status();
  }
};

template< typename >
void stream::alloc_cb(::uv_handle_t *_uv_handle, std::size_t _suggested_size, ::uv_buf_t *_uv_buf)
{
  auto &alloc_cb = instance::from(_uv_handle)->supplemental_data().on_buffer;
  buffer &&b = alloc_cb(stream(reinterpret_cast< uv_t* >(_uv_handle)), _suggested_size);
  buffer::instance::from(b.uv_buf)->ref();  // add the reference for the future moving the buffer instance into read_cb() parameter
  *_uv_buf = b[0];
}
template< typename >
void stream::read_cb(::uv_stream_t *_uv_stream, ssize_t _nread, const ::uv_buf_t *_uv_buf)
{
  auto self = instance::from(_uv_stream);
  self->uv_status() = _nread;

  auto &read_cb = self->supplemental_data().on_read;
  if (_uv_buf->base)
    read_cb(stream(_uv_stream), _nread, buffer(buffer::instance::from_base(_uv_buf->base), adopt_ref));
    // don't forget to specify adopt_ref flag when using ref_guard to unref the object
    // don't use ref_guard unless it really needs to hold on the object until the scope end
    // use move/transfer semantics instead if you need just pass the object to another function for further processing
  else
    read_cb(stream(_uv_stream), _nread, buffer());
}
template< typename >
void stream::connection_cb(::uv_stream_t *_uv_stream, int _status)
{
  auto self = instance::from(_uv_stream);
  self->uv_status() = _status;
  auto &connection_cb = self->supplemental_data().on_connection;
  if (connection_cb)  connection_cb(stream(_uv_stream));
}


}


#endif
