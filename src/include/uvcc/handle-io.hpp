
#ifndef UVCC_HANDLE_IO__HPP
#define UVCC_HANDLE_IO__HPP

#include "uvcc/utility.hpp"
#include "uvcc/handle-base.hpp"
#include "uvcc/buffer.hpp"

#include <uv.h>
#include <cstddef>      // size_t
#include <functional>   // function
#include <mutex>        // lock_guard


namespace uv
{


/*! \ingroup doxy_group_handle
    \brief The base class for handles representing I/O endpoints: a file, TCP/UDP socket, pipe, TTY.
    \details Encapsulates common I/O functions and properties.
    \note `read_start()` and `read_stop()` functions are mutually exclusive and thread-safe. */
class io : public handle
{
  //! \cond
  friend class handle::instance< io >;
  //! \endcond

public: /*types*/
  using uv_t = void;
  using on_read_t = std::function< void(io _handle, ssize_t _nread, buffer _buffer, void *_info) >;
  /*!< \brief The function type of the callback called by `read_start()` when data was read from an I/O endpoint.
       \details The `_info` pointer is valid for the duration of the callback only and refers to the following
       supplemental data:
        I/O endpoint  | `_info`                      | Description
       :--------------|:-----------------------------|:----------------------------------------------------------
        `uv::file`    | `int64_t*`                   | the offset the read operation has been performed at
        `uv::stream`  | `nullptr`                    | no additional data
        `uv::udp`     | `struct uv::udp::recv_info*` | information on sender address and received message status

       \sa libuv API documentation: [`uv_read_cb`](http://docs.libuv.org/en/v1.x/stream.html#c.uv_read_cb),
                                    [`uv_udp_recv_cb`](http://docs.libuv.org/en/v1.x/udp.html#c.uv_udp_recv_cb).
       \note On error and EOF state the this callback is supplied with a dummy _null-initialized_ `_buffer`.

       On error and EOF state the libuv API calls the user provided
       [`uv_read_cb`](http://docs.libuv.org/en/v1.x/stream.html#c.uv_read_cb) or
       [`uv_udp_recv_cb`](http://docs.libuv.org/en/v1.x/udp.html#c.uv_udp_recv_cb) functions with
       a _null-initialized_ [`uv_buf_t`](http://docs.libuv.org/en/v1.x/misc.html#c.uv_buf_t) buffer structure
       (where `buf->base = nullptr` and `buf->len = 0`) and does not try to retrieve  something from the
       [`uv_alloc_cb`](http://docs.libuv.org/en/v1.x/handle.html#c.uv_alloc_cb) callback in such a cases.
       So the uvcc `io::on_read_t` callback is supplied with a dummy _null-initialized_ `_buffer`. */

protected: /*types*/
  //! \cond
  struct properties
  {
    spinlock rdstate_switch;
    bool rdstate_flag = false;
    std::size_t rdsize = 0;
    on_buffer_alloc_t alloc_cb;
    on_read_t read_cb;
  };

  struct uv_interface : virtual handle::uv_interface
  {
    virtual int read_start(void*, int64_t) const noexcept = 0;
    virtual int read_stop(void*) const noexcept = 0;
  };
  //! \endcond

private: /*types*/
  using instance = handle::instance< io >;

private: /*constructors*/
  explicit io(uv_t *_uv_handle)
  {
    if (_uv_handle)  instance::from(_uv_handle)->ref();
    uv_handle = _uv_handle;
  }

protected: /*constructors*/
  io() noexcept = default;

public: /*constructors*/
  ~io() = default;

  io(const io&) = default;
  io& operator =(const io&) = default;

  io(io&&) noexcept = default;
  io& operator =(io&&) noexcept = default;

protected: /*functions*/
  //! \cond
  static void io_alloc_cb(uv_t *_uv_handle, std::size_t _suggested_size, ::uv_buf_t *_uv_buf)
  {
    auto &properties = instance::from(_uv_handle)->properties();

    auto &alloc_cb = properties.alloc_cb;
    buffer &&b = alloc_cb(io(_uv_handle), properties.rdsize ? properties.rdsize : _suggested_size);

    buffer::instance::from(b.uv_buf)->ref();  // add the reference for the future moving the buffer instance into io_read_cb() parameter
    *_uv_buf = b[0];
  }

  static void io_read_cb(uv_t *_uv_handle, ssize_t _nread, const ::uv_buf_t *_uv_buf, void *_info)
  {
    auto instance_ptr = instance::from(_uv_handle);
    instance_ptr->uv_error = _nread;

    auto &read_cb = instance_ptr->properties().read_cb;
    if (_uv_buf->base)
      read_cb(io(_uv_handle), _nread, buffer(buffer::instance::uv_buf::from(_uv_buf->base), adopt_ref), _info);
      // don't forget to specify adopt_ref flag when using ref_guard to unref the object
      // don't use ref_guard unless it really needs to hold on the object until the scope end
      // use move/transfer semantics instead if you need just pass the object to another function for further processing
    else
      read_cb(io(_uv_handle), _nread, buffer(), _info);
  }

#if 0
  template< typename = void > static void file_read_cb(::uv_fs_t*);
  template< typename = void > static void stream_read_cb(::uv_stream_t*, ssize_t, const ::uv_buf_t*);
  template< typename = void > static void udp_recv_cb(::uv_udp_t*, ssize_t, const ::uv_buf_t*, const ::sockaddr*, unsigned);
#endif
  //! \endcond

public: /*interface*/
  on_buffer_alloc_t& on_alloc() const noexcept  { return instance::from(uv_handle)->properties().alloc_cb; }
  on_read_t& on_read() const noexcept  { return instance::from(uv_handle)->properties().read_cb; }

  /*! \brief Start reading incoming data from the I/O endpoint.
      \details The handle is tried to be set for reading if only nonempty `_alloc_cb` and `_read_cb` functions
      are provided, or else `UV_EINVAL` is returned with no involving any libuv API or uvcc function.
      Repeated call to this function results in the automatic call to `read_stop()` firstly.
      In the repeated calls `_alloc_cb` and/or `_read_cb` functions may be empty values, which means that
      they aren't changed from the previous call.
      \arg `_size` parameter can be set to specify suggested length of the read buffer.
      \arg `_offset` - the starting offset for reading (intended for `uv::file` I/O endpoint).

      \note This function adds an extra reference to the handle instance, which is released when the
      counterpart function `read_stop()` is called.

      For `uv::stream` and `uv::udp` endpoints the function is just a wrapper around
      [`uv_read_start()`](http://docs.libuv.org/en/v1.x/stream.html#c.uv_read_start) and
      [`uv_udp_recv_start()`](http://docs.libuv.org/en/v1.x/udp.html#c.uv_udp_recv_start) libuv facilities.
      For `uv::file` endpoint it implements a chain of calls for
      [`uv_fs_read()`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_read) libuv API function with the user provided
      callback function. If the user callback does not stop the read loop by calling `read_stop()` function,
      the next call is automatically performed after the callback returns. The user callback receives a number of bytes
      read during the current iteration, or EOF or error state just like if it would be a
      [`uv_read_cb`](http://docs.libuv.org/en/v1.x/stream.html#c.uv_read_cb) callback for `uv::stream` endpoint.
      Note that even on EOF or error state having been reached the loop keeps trying to read from the file until `read_stop()`
      is explicitly called.

      \sa libuv API documentation: [`uv_fs_read()`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_read),
                                   [`uv_read_start()`](http://docs.libuv.org/en/v1.x/stream.html#c.uv_read_start),
                                   [`uv_udp_recv_start()`](http://docs.libuv.org/en/v1.x/udp.html#c.uv_udp_recv_start). */
  int read_start(const on_buffer_alloc_t &_alloc_cb, const on_read_t &_read_cb, std::size_t _size = 0, int64_t _offset = -1) const
  {
    auto instance_ptr = instance::from(uv_handle);
    auto &properties = instance_ptr->properties();

    std::lock_guard< decltype(properties.rdstate_switch) > lk(properties.rdstate_switch);

    if (!_alloc_cb and !properties.alloc_cb)  return uv_status(UV_EINVAL);
    if (!_read_cb and !properties.read_cb)  return uv_status(UV_EINVAL);

    instance_ptr->ref();  // first, make sure it would exist for the future _read_cb() calls until read_stop()

    if (properties.rdstate_flag)
    {
      uv_status(instance_ptr->uv_interface()->read_stop(uv_handle));
      instance_ptr->unref();  // release the excess reference from the repeated read_start()
    }
    else properties.rdstate_flag = true;

    if (_alloc_cb)  properties.alloc_cb = _alloc_cb;
    if (_read_cb)  properties.read_cb = _read_cb;
    properties.rdsize = _size;

    uv_status(0);
    int ret = instance_ptr->uv_interface()->read_start(uv_handle, _offset);
    if (!ret)  uv_status(ret);
    return ret;
  }
  /*! \brief Restart reading incoming data from the I/O endpoint using `_alloc_cb` and `_read_cb`
      functions having been explicitly set before or provided with the previous `read_start()` call.
      Repeated call to this function results in the automatic call to `read_stop()` firstly.
      \note This function adds an extra reference to the handle instance, which is released when the
      counterpart function `read_stop()` is called. */
  int read_start(std::size_t _size = 0, int64_t _offset = -1) const
  {
    auto instance_ptr = instance::from(uv_handle);
    auto &properties = instance_ptr->properties();

    std::lock_guard< decltype(properties.rdstate_switch) > lk(properties.rdstate_switch);

    if (!properties.alloc_cb or !properties.read_cb)  return uv_status(UV_EINVAL);

    instance_ptr->ref();

    if (properties.rdstate_flag)
    {
      uv_status(instance_ptr->uv_interface()->read_stop(uv_handle));
      instance_ptr->unref();
    }
    else properties.rdstate_flag = true;

    properties.rdsize = _size;

    uv_status(0);
    int ret = instance_ptr->uv_interface()->read_start(uv_handle, _offset);
    if (!ret)  uv_status(ret);
    return ret;
  }

  /*! \brief Stop reading data from the I/O endpoint.
      \sa libuv API documentation: [`uv_read_stop()`](http://docs.libuv.org/en/v1.x/stream.html#c.uv_read_stop),
                                   [`uv_udp_recv_stop()`](http://docs.libuv.org/en/v1.x/udp.html#c.uv_udp_recv_stop). */
  int read_stop() const
  {
    auto instance_ptr = instance::from(uv_handle);
    auto &properties = instance_ptr->properties();

    std::lock_guard< decltype(properties.rdstate_switch) > lk(properties.rdstate_switch);

    uv_status(instance_ptr->uv_interface()->read_stop(uv_handle));

    if (properties.rdstate_flag)
    {
      properties.rdstate_flag = false;
      instance_ptr->unref();  // release the excess reference from read_start()
    };

    properties.rdsize = 0;

    return uv_status();
  }

  static io guess_handle(::uv_file _fd);

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_handle); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_handle); }
};


}


#include "uvcc/loop.hpp"
#include "uvcc/handle-stream.hpp"
#include "uvcc/handle-fs.hpp"
#include "uvcc/handle-udp.hpp"


namespace uv
{


inline io io::guess_handle(::uv_file _fd)
{
  io handle;

  switch (::uv_guess_handle(_fd))
  {
  default:
  case UV_UNKNOWN_HANDLE:
      handle.uv_status(UV_EBADF);
      break;
  case UV_NAMED_PIPE:
      handle.uv_handle = handle::instance< pipe >::create();
      handle.uv_status(::uv_pipe_init(
          static_cast< loop::uv_t* >(loop::Default()),
          static_cast< ::uv_pipe_t* >(handle.uv_handle),
          0
      ));
      if (!handle) break;
      handle.uv_status(::uv_pipe_open(
          static_cast< uv_pipe_t* >(handle.uv_handle),
          _fd
      ));
      break;
  case UV_TCP:
      break;
  case UV_TTY:
      break;
  case UV_UDP:
      break;
  case UV_FILE:
      break;
  }

  return handle;
}


}


#endif
