
#ifndef UVCC_HANDLE_IO__HPP
#define UVCC_HANDLE_IO__HPP

#include "uvcc/utility.hpp"
#include "uvcc/handle-base.hpp"
#include "uvcc/buffer.hpp"
#include "uvcc/loop.hpp"

#include <uv.h>
#include <cstddef>      // size_t
#include <functional>   // function
#include <mutex>        // lock_guard


namespace uv
{


/*! \ingroup doxy_group_handle
    \brief The base class for handles representing I/O endpoints: a file, TCP/UDP socket, pipe, TTY.
    \details Encapsulates common I/O functions and properties.
    \note `read_start()`/`read_stop()` and `read_pause()`/`read_resume()` functions are mutually exclusive and thread-safe. */
class io : public handle
{
  //! \cond
  friend class handle::instance< io >;
  friend class output;
  //! \endcond

public: /*types*/
  using uv_t = void;
  using on_read_t = std::function< void(io _handle, ssize_t _nread, buffer _buffer, void *_info) >;
  /*!< \brief The function type of the callback called by `read_start()` when data was read from an I/O endpoint.
       \details The `_info` pointer is valid for the duration of the callback only and refers to the following
       supplemental data:
        I/O endpoint  | `_info`                    | Description
       :--------------|:---------------------------|:--------------------------------------------------------------
        `uv::file`    | `int64_t*`                 | the offset the read operation has been performed at
        `uv::stream`  | `nullptr`                  | no additional data
        `uv::udp`     | `struct uv::udp::io_info*` | information on remote peer address and received message flags

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
  enum class rdcmd  { UNKNOWN, STOP, PAUSE, START, RESUME };

  struct properties
  {
    spinlock rdstate_switch;
    rdcmd rdcmd_state = rdcmd::UNKNOWN;
    std::size_t rdsize = 0;
    on_buffer_alloc_t alloc_cb;
    on_read_t read_cb;
  };

  struct uv_interface : virtual handle::uv_interface
  {
    virtual std::size_t write_queue_size(void*) const noexcept = 0;
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
  template< typename = void > static void udp_recv_cb(::uv_udp_t*, ssize_t, const ::uv_buf_t*, const ::sockaddr*, unsigned int);
#endif
  //! \endcond

public: /*interface*/
  /*! \brief The amount of queued bytes waiting to be written/sent to the I/O endpoint.
      \sa libuv API documentation: [`uv_stream_t.write_queue_size`](http://docs.libuv.org/en/v1.x/stream.html#c.uv_stream_t.write_queue_size),
                                   [`uv_udp_t.send_queue_size`](http://docs.libuv.org/en/v1.x/udp.html#c.uv_udp_t.send_queue_size). */
  std::size_t write_queue_size() const noexcept  { return instance::from(uv_handle)->uv_interface()->write_queue_size(uv_handle); }

  on_buffer_alloc_t& on_alloc() const noexcept  { return instance::from(uv_handle)->properties().alloc_cb; }
  on_read_t& on_read() const noexcept  { return instance::from(uv_handle)->properties().read_cb; }

  /*! \brief Start reading incoming data from the I/O endpoint.
      \details The handle is tried to be set for reading if only nonempty `_alloc_cb` and `_read_cb` functions
      are provided, or else `UV_EINVAL` is returned with no involving any libuv API or uvcc function.
      Repeated call to this function results in the automatic call to `read_stop()` firstly.
      In the repeated calls `_alloc_cb` and/or `_read_cb` functions may be empty values, which means that
      they aren't changed from the previous call.
      \arg `_size` parameter can be set to specify suggested length of the read buffer.
      \arg `_offset` - the starting offset for reading (intended for `uv::file` I/O endpoint);
                       the default value of \b -1 means using of the current file position.

      \note On successful start this function adds an extra reference to the handle instance,
      which is released when the counterpart function `read_stop()` is called.

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

    auto rdcmd_state = properties.rdcmd_state;
    properties.rdcmd_state = rdcmd::START;

    switch (rdcmd_state)
    {
    case rdcmd::UNKNOWN:
    case rdcmd::STOP:
    case rdcmd::PAUSE:
        instance_ptr->ref();  // make sure it will exist for the future _read_cb() calls until read_stop()/read_pause()
        break;
    case rdcmd::START:
    case rdcmd::RESUME:
        uv_status(instance_ptr->uv_interface()->read_stop(uv_handle));
        break;
    }

    if (_alloc_cb)  properties.alloc_cb = _alloc_cb;
    if (_read_cb)  properties.read_cb = _read_cb;
    properties.rdsize = _size;

    uv_status(0);
    int ret = instance_ptr->uv_interface()->read_start(uv_handle, _offset);
    if (!ret)  uv_status(ret);

    if (ret < 0)
    {
      properties.rdcmd_state = rdcmd::UNKNOWN;
      instance_ptr->unref();  // release the reference on start failure
    };

    return ret;
  }
  /*! \brief Restart reading incoming data from the I/O endpoint using `_alloc_cb` and `_read_cb`
      functions having been explicitly set before or provided with the previous `read_start()` call.
      Repeated call to this function results in the automatic call to `read_stop()` firstly.
      \note On successful restart this function adds an extra reference to the handle instance,
      which is released when the counterpart function `read_stop()` is called. */
  int read_start(std::size_t _size = 0, int64_t _offset = -1) const
  {
    auto instance_ptr = instance::from(uv_handle);
    auto &properties = instance_ptr->properties();

    std::lock_guard< decltype(properties.rdstate_switch) > lk(properties.rdstate_switch);

    if (!properties.alloc_cb or !properties.read_cb)  return uv_status(UV_EINVAL);

    auto rdcmd_state = properties.rdcmd_state;
    properties.rdcmd_state = rdcmd::START;

    switch (rdcmd_state)
    {
    case rdcmd::UNKNOWN:
    case rdcmd::STOP:
    case rdcmd::PAUSE:
        instance_ptr->ref();
        break;
    case rdcmd::START:
    case rdcmd::RESUME:
        uv_status(instance_ptr->uv_interface()->read_stop(uv_handle));
        break;
    }

    properties.rdsize = _size;

    uv_status(0);
    int ret = instance_ptr->uv_interface()->read_start(uv_handle, _offset);
    if (!ret)  uv_status(ret);

    if (ret < 0)
    {
      properties.rdcmd_state = rdcmd::UNKNOWN;
      instance_ptr->unref();
    };

    return ret;
  }

  /*! \brief Stop reading data from the I/O endpoint and clear all read parameters.
      \sa libuv API documentation: [`uv_read_stop()`](http://docs.libuv.org/en/v1.x/stream.html#c.uv_read_stop),
                                   [`uv_udp_recv_stop()`](http://docs.libuv.org/en/v1.x/udp.html#c.uv_udp_recv_stop). */
  int read_stop() const
  {
    auto instance_ptr = instance::from(uv_handle);
    auto &properties = instance_ptr->properties();

    std::lock_guard< decltype(properties.rdstate_switch) > lk(properties.rdstate_switch);

    auto rdcmd_state = properties.rdcmd_state;
    properties.rdcmd_state = rdcmd::STOP;

    int ret = uv_status(instance_ptr->uv_interface()->read_stop(uv_handle));

    properties.rdsize = 0;

    switch (rdcmd_state)
    {
    case rdcmd::UNKNOWN:
    case rdcmd::STOP:
    case rdcmd::PAUSE:
        break;
    case rdcmd::START:
    case rdcmd::RESUME:
        instance_ptr->unref();  // release the reference from read_start()/read_resume()
        break;
    }

    return ret;
  }

  /*! \brief Pause reading data from the I/O endpoint.
      \details
      \arg `read_pause(true)` - a \e "pause" command that is functionally equivalent to `read_stop()`
      without clearing read parameters.
      \arg `read_pause(false)` - a _no op_ command that returns immediately.

      To be used in conjunction with `read_resume()` control command.
      \sa `read_resume()` */
  int read_pause(bool _trigger_condition) const
  {
    if (!_trigger_condition)  return 0;

    auto instance_ptr = instance::from(uv_handle);
    auto &properties = instance_ptr->properties();

    std::lock_guard< decltype(properties.rdstate_switch) > lk(properties.rdstate_switch);

    int ret = 0;
    switch (properties.rdcmd_state)
    {
    case rdcmd::UNKNOWN:
    case rdcmd::STOP:
    case rdcmd::PAUSE:
        break;
    case rdcmd::START:
    case rdcmd::RESUME:
        properties.rdcmd_state = rdcmd::PAUSE;
        ret = uv_status(instance_ptr->uv_interface()->read_stop(uv_handle));
        instance_ptr->unref();
        break;
    }
    return ret;
  }

  /*! \brief Resume reading data from the I/O endpoint after having been paused.
      \details
      \arg `read_resume(true)` - a \e "resume" command that is functionally equivalent to `read_start()` except
      that it cannot \e "start" and is refused if the previous control command was not \e "pause" (i.e. `read_pause()`).
      \arg `read_resume(false)` - a _no op_ command that returns immediately.

      To be used in conjunction with `read_pause()` control command.

      \note `read_pause()`/`read_resume()` commands are intended for temporary pausing the read process for example in
      such a cases when a consumer which the data being read is sent to becomes overwhelmed with them and its input queue
      (and/or a sender output queue) has been considerably increased.

      The different control command interoperating semantics is described as follows:
      \verbatim
                           ║ The command having been executed previously                                 
         The command to    ╟─────────┬──────────────┬───────────────────┬─────────────┬──────────────────
         be currently      ║ No      │ read_start() │ read_resume(true) │ read_stop() │ read_pause(true) 
         executed          ║ command │              │                   │             │                  
        ═══════════════════╬═════════╪══════════════╪═══════════════════╪═════════════╪══════════════════
         read_start()      ║ "start" │ "restart"    │ "restart"         │ "start"     │ "start"          
         read_resume(true) ║ -       │ -            │ -                 │ -           │ "resume"         
         read_stop()       ║ "stop"  │ "stop"       │ "stop"            │ "stop"      │ "stop"           
         read_pause(true)  ║ -       │ "pause"      │ "pause"           │ -           │ -                

        "resume" is functionally equivalent to "start"
        "pause" is functionally equivalent to "stop" without clearing read parameters
        "restart" is functionally equivalent to "stop" followed by "start"
      \endverbatim */
  int read_resume(bool _trigger_condition)
  {
    if (!_trigger_condition)  return 0;

    auto instance_ptr = instance::from(uv_handle);
    auto &properties = instance_ptr->properties();

    std::lock_guard< decltype(properties.rdstate_switch) > lk(properties.rdstate_switch);

    int ret = 0;
    switch (properties.rdcmd_state)
    {
    case rdcmd::UNKNOWN:
    case rdcmd::STOP:
        break;
    case rdcmd::PAUSE:
        properties.rdcmd_state = rdcmd::RESUME;

        instance_ptr->ref();

        uv_status(0);
        ret = instance_ptr->uv_interface()->read_start(uv_handle, -1);
        if (!ret)  uv_status(ret);

        if (ret < 0)
        {
          properties.rdcmd_state = rdcmd::UNKNOWN;
          instance_ptr->unref();
        };

        break;
    case rdcmd::START:
    case rdcmd::RESUME:
        break;
    }
    return ret;
  }

  /*! \brief Create an `io` handle object which actual type is derived from an existing file descriptor.
      \details Supported file desctriptors:
      - Windows: pipe, tty, file
      - Unicies: pipe, tty, file, tcp/udp socket
      .
      \sa [`uv_guess_handle()`](http://docs.libuv.org/en/v1.x/misc.html#c.uv_guess_handle). */
  static io guess_handle(uv::loop&, ::uv_file);

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_handle); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_handle); }
};


}


#include "uvcc/handle-stream.hpp"
#include "uvcc/handle-fs.hpp"
#include "uvcc/handle-udp.hpp"


namespace uv
{


inline io io::guess_handle(uv::loop &_loop, ::uv_file _fd)
{
  io ret;

  switch (::uv_guess_handle(_fd))
  {
  default:
  case UV_UNKNOWN_HANDLE:
      ret.uv_status(UV_EBADF);
      break;
  case UV_NAMED_PIPE:
      ret.uv_handle = handle::instance< pipe >::create();
      ret.uv_status(::uv_pipe_init(
          static_cast< uv::loop::uv_t* >(_loop),
          static_cast< ::uv_pipe_t* >(ret.uv_handle),
          0
      ));
      if (!ret) break;
      ret.uv_status(::uv_pipe_open(
          static_cast< uv_pipe_t* >(ret.uv_handle),
          _fd
      ));
      break;
  case UV_TCP:
      ret.uv_handle = handle::instance< tcp >::create();
      ret.uv_status(::uv_tcp_init(
          static_cast< uv::loop::uv_t* >(_loop),
          static_cast< ::uv_tcp_t* >(ret.uv_handle)
      ));
      if (!ret)  break;
      ret.uv_status(::uv_tcp_open(
          static_cast< ::uv_tcp_t* >(ret.uv_handle),
          _fd
      ));
      break;
  case UV_TTY:
      ret.uv_handle = handle::instance< tty >::create();
      ret.uv_status(::uv_tty_init(
          static_cast< uv::loop::uv_t* >(_loop),
          static_cast< ::uv_tty_t* >(ret.uv_handle),
          _fd,
          1
      ));
      break;
  case UV_UDP:
      ret.uv_handle = handle::instance< udp >::create();
      ret.uv_status(::uv_udp_init(
          static_cast< uv::loop::uv_t* >(_loop),
          static_cast< ::uv_udp_t* >(ret.uv_handle)
      ));
      if (!ret)  break;
      ret.uv_status(::uv_udp_open(
          static_cast< ::uv_udp_t* >(ret.uv_handle),
          _fd
      ));
      break;
  case UV_FILE:
      ret.uv_handle = handle::instance< file >::create();
      static_cast< file::uv_t* >(ret.uv_handle)->loop = static_cast< uv::loop::uv_t* >(_loop);
      static_cast< file::uv_t* >(ret.uv_handle)->result = _fd;
      static_cast< file::uv_t* >(ret.uv_handle)->path = nullptr;
      break;
  }

  return ret;
}


}


#endif
