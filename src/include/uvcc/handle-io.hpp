
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
  using on_read_t = std::function< void(io _handle, ssize_t _nread, buffer _buffer, void *_data) >;
  /*!< \brief The function type of the callback called by `read_start()` when data was read from an I/O endpoint.
       \details The `_data` pointer is valid for the duration of the callback only and refers to the following
       supplemental data:
        I/O endpoint  | `_data`                     | Description
       :--------------|:----------------------------|:----------------------------------------------------
        `uv::file`    | `int64_t*`                  | the offset the read operation has been performed at
        `uv::stream`  | `nullptr`                   | no additional data
        `uv::udp`     | `struct uv::udp_recv_info*` | information on sender and received message status

       \sa libuv API documentation: [`uv_read_cb`](http://docs.libuv.org/en/v1.x/stream.html#c.uv_read_cb),
                                    [`uv_udp_recv_cb`](http://docs.libuv.org/en/v1.x/udp.html#c.uv_udp_recv_cb).
       \note On error and EOF state the this callback is supplied with a dummy _null-initialized_ `_buffer`.
       \internal
       On error and EOF state the libuv API calls the user provided
       [`uv_read_cb`](http://docs.libuv.org/en/v1.x/stream.html#c.uv_read_cb) function with
       a _null-initialized_ [`uv_buf_t`](http://docs.libuv.org/en/v1.x/misc.html#c.uv_buf_t) buffer structure
       (where `buf->base = nullptr` and `buf->len = 0`) and does not try to retrieve  something from the
       [`uv_alloc_cb`](http://docs.libuv.org/en/v1.x/handle.html#c.uv_alloc_cb) callback in such a cases.
       So the uvcc `io::on_read_t` callback is supplied with a dummy _null-initialized_ `_buffer`.
       \endinternal */

protected: /*types*/
  //! \cond
  struct properties
  {
    spinlock rdstate_switch;
    bool rdstate_flag = false;
    on_buffer_alloc_t alloc_cb;
    on_read_t read_cb;
  };

  struct uv_interface : virtual handle::uv_interface
  {
    virtual int read_start(void*) const noexcept = 0;
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

private: /*functions*/
#if 0
  template< typename = void > static void alloc_cb(::uv_handle_t*, std::size_t, ::uv_buf_t*);
  template< typename = void > static void file_read_cb(::uv_fs_t*);
  template< typename = void > static void stream_read_cb(::uv_stream_t*, ssize_t, const ::uv_buf_t*);
  template< typename = void > static void udp_recv_cb(::uv_udp_t*, ssize_t, const ::uv_buf_t*, const ::sockaddr*, unsigned);
#endif

public: /*interface*/
  on_buffer_alloc_t& on_alloc() const noexcept  { return instance::from(uv_handle)->properties().alloc_cb; }
  on_read_t& on_read() const noexcept  { return instance::from(uv_handle)->properties().read_cb; }

  /*! \brief Start reading incoming data from the I/O endpoint.
      \details The handle is tried to be set for reading if only nonempty `_alloc_cb` and `_read_cb` functions
      are provided, or else `UV_EINVAL` is returned with no involving any libuv API or uvcc function.
      Repeated call to this function results in the automatic call to `read_stop()` firstly.
      In the repeated calls `_alloc_cb` and/or `_read_cb` functions can be empty values, which means that
      they aren't changed from the previous call.
      \note This function adds an extra reference to the handle instance, which is released when the
      counterpart function `read_stop()` is called.
      \sa libuv API documentation: [`uv_fs_read()`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_read),
                                   [`uv_read_start()`](http://docs.libuv.org/en/v1.x/stream.html#c.uv_read_start),
                                   [`uv_udp_recv_start()`](http://docs.libuv.org/en/v1.x/udp.html#c.uv_udp_recv_start). */
  int read_start(const on_buffer_alloc_t &_alloc_cb, const on_read_t &_read_cb) const
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

    uv_status(0);
    int o = instance_ptr->uv_interface()->read_start(uv_handle);
    if (!o)  uv_status(o);
    return o;
  }
  /*! \brief Restart reading incoming data from the I/O endpoint using `_alloc_cb` and `_read_cb`
      functions having been explicitly set before or provided with the previous `read_start()` call.
      Repeated call to this function results in the automatic call to `read_stop()` firstly.
      \note This function adds an extra reference to the handle instance, which is released when the
      counterpart function `read_stop()` is called. */
  int read_start() const
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

    uv_status(0);
    int o = instance_ptr->uv_interface()->read_start(uv_handle);
    if (!o)  uv_status(o);
    return o;
  }

  /*! \brief Stop reading data from the I/O endpoint.
      \sa libuv API documentation: [`uv_read_stop()`](http://docs.libuv.org/en/v1.x/stream.html#c.uv_read_stop). */
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

    return uv_status();
  }
};


}


#endif
