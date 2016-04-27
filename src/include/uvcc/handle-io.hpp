
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
#include <mutex>        // lock_guard


namespace uv
{


/*! \ingroup doxy_handle
    \brief The base class for handles representing I/O endpoints: a file, TCP/UDP socket, pipe, TTY.
    Encapsulates common I/O functions and properties. */
class io : public handle
{
  //! \cond
  friend class handle::instance< io >;
  //! \endcond

public: /*types*/
  using on_read_t = std::function< void(io _handle, ssize_t _nread, buffer _buffer) >;
  /*!< \brief The function type of the callback called by `read_start()` when data was read from an I/O endpoint.
       \note On error and EOF state the libuv API calls the user provided
       [`uv_read_cb`](http://docs.libuv.org/en/v1.x/stream.html#c.uv_read_cb) function with
       a _null-initialized_ [`uv_buf_t`](http://docs.libuv.org/en/v1.x/misc.html#c.uv_buf_t) buffer structure
       (where `buf->base = nullptr` and `buf->len = 0`) and does not try to retrieve  something from the
       [`uv_alloc_cb`](http://docs.libuv.org/en/v1.x/handle.html#c.uv_alloc_cb) callback in such a cases.
       So the uvcc `io::on_read_t` callback is supplied with a dummy _null-initialized_ `_buffer`.
       \sa libuv API documentation: [`uv_read_cb`](http://docs.libuv.org/en/v1.x/stream.html#c.uv_read_cb) */

protected: /*types*/
  //! \cond
  struct property : virtual handle::property
  {
    spinlock rdstate_switch;
    bool rdstate_flag = false;
    on_alloc_t alloc_cb;
    on_read_t read_cb;
    virtual int read_start() const noexcept = 0;
    virtual int read_stop() const noexcept = 0;
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
#if 0
  template< typename = void > static void alloc_cb(::uv_handle_t*, std::size_t, ::uv_buf_t*);
  template< typename = void > static void file_read_cb(::uv_fs_t*);
  template< typename = void > static void stream_read_cb(::uv_stream_t*, ssize_t, const ::uv_buf_t*);
  template< typename = void > static void udp_recv_cb(::uv_udp_t*, ssize_t, const ::uv_buf_t*, const ::sockaddr*, unsigned);
#endif

public: /*interface*/
  on_alloc_t& on_alloc() const noexcept  { return static_cast< instance* >(ptr)->property->alloc_cb; }
  on_read_t& on_read() const noexcept  { return static_cast< instance* >(ptr)->property->read_cb; }

  /*! \brief Start reading incoming data from the I/O endpoint.
      \details The handle is tried to be set for reading if only nonempty `_alloc_cb` and `_read_cb` functions
      are provided, or else `UV_EINVAL` is returned with no involving any libuv API or uvcc function.
      Repeated call to this function results in the automatic call to `read_stop()` firstly.
      In the repeated calls `_alloc_cb` and/or `_read_cb` functions can be empty values, which means that
      they aren't changed from the previous call.
      \note This function adds an extra reference to the handle instance, which is released when the
      counterpart function `read_stop()` is called. */
  int read_start(const on_alloc_t &_alloc_cb, const on_read_t &_read_cb) const
  {
    auto p = static_cast< instance* >(ptr)->property;

    std::lock_guard< decltype(p->rdstate_switch) > lk(p->rdstate_switch);

    if (!_alloc_cb and !p->alloc_cb)  return uv_status(UV_EINVAL);
    if (!_read_cb and !p->read_cb)  return uv_status(UV_EINVAL);

    static_cast< instance* >(ptr)->ref();  // first, make sure it would exist for the future _read_cb() calls until read_stop()

    if (p->rdstate_flag)
    {
      uv_status(p->read_stop());
      static_cast< instance* >(ptr)->unref();  // release the excess reference from the repeated read_start()
    }
    else p->rdstate_flag = true;

    if (_alloc_cb)  p->alloc_cb = _alloc_cb;
    if (_read_cb)  p->read_cb = _read_cb;

    uv_status(0);
    int o = p->read_start();
    if (!o)  uv_status(o);
    return o;
  }
  /*! \brief Restart reading incoming data from the I/O endpoint using `_alloc_cb` and `_read_cb`
      functions provided with the previous `read_start()` call.
      Repeated call to this function results in the automatic call to `read_stop()` firstly.
      \note This function adds an extra reference to the handle instance, which is released when the
      counterpart function `read_stop()` is called. */
  int read_start() const
  {
    auto p = static_cast< instance* >(ptr)->property;

    std::lock_guard< decltype(p->rdstate_switch) > lk(p->rdstate_switch);

    if (!p->alloc_cb or !p->read_cb)  return uv_status(UV_EINVAL);

    static_cast< instance* >(ptr)->ref();

    if (p->rdstate_flag)
    {
      uv_status(p->read_stop());
      static_cast< instance* >(ptr)->unref();
    }
    else p->rdstate_flag = true;

    uv_status(0);
    int o = p->read_start();
    if (!o)  uv_status(o);
    return o;
  }

  /*! \brief Stop reading data from the I/O endpoint. */
  int read_stop() const
  {
    auto p = static_cast< instance* >(ptr)->property;

    std::lock_guard< decltype(p->rdstate_switch) > lk(p->rdstate_switch);

    uv_status(p->read_stop());

    if (p->rdstate_flag)
    {
      p->rdstate_flag = false;
      static_cast< instance* >(ptr)->unref();  // release the excess reference from read_start()
    };

    return uv_status();
  }
};


}


#endif
