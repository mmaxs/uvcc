
#ifndef UVCC_HANDLE__HPP
#define UVCC_HANDLE__HPP

#include "uvcc/utility.hpp"
#include "uvcc/buffer.hpp"
#include "uvcc/loop.hpp"

#include <uv.h>
#include <cstddef>      // offsetof size_t
#include <functional>   // function
#include <type_traits>  // is_standard_layout enable_if_t
#include <utility>      // swap() move() forward()
#include <memory>       // addressof()
#include <string>       // string


namespace uv
{
/*! \defgroup g__handle Handles
    \brief The classes representing libuv handles.
    \note uvcc handle objects have no interface functions corresponding to the following libuv
    API functions that control whether a handle is
    [referenced by the event loop](http://docs.libuv.org/en/v1.x/handle.html#reference-counting)
    which it has been attached to while being created and where it is running on:
    [`uv_ref()`](http://docs.libuv.org/en/v1.x/handle.html#c.uv_ref),
    [`uv_unref()`](http://docs.libuv.org/en/v1.x/handle.html#c.uv_unref),
    [`uv_has_ref()`](http://docs.libuv.org/en/v1.x/handle.html#c.uv_has_ref).
    These libuv functions can be directly applied to uvcc handle objects
    (with explicit casting the handle object to `uv_handle_t*`) if necessary. */
//! \{


/* libuv handles */
class async;
class check;
class fs_event;
class fs_poll;
class handle;
class idle;
class pipe;
class poll;
class prepare;
class process;
class stream;
class tcp;
class timer;
class tty;
class udp;
class signal;



/*! \defgroup g__handle_traits uv_handle_traits< typename >
    \brief The correspondence between libuv handle data types and C++ classes representing them. */
//! \{
#define BUGGY_DOXYGEN
#undef BUGGY_DOXYGEN
//! \cond
template< typename _UV_T_ > struct uv_handle_traits  {};
//! \endcond
#define XX(X, x) template<> struct uv_handle_traits< uv_##x##_t > { using type = x; };
UV_HANDLE_TYPE_MAP(XX)
#undef XX
//! \}



/*! \brief The base class for the libuv handles.
    \details Derived classes conceptually are just interfaces to the data stored
    in the base class, so there are no any virtual member functions.
    \sa libuv documentation: [`uv_handle_t`](http://docs.libuv.org/en/v1.x/handle.html#uv-handle-t-base-handle). */
class handle
{
  //! \cond
  friend class loop;
  friend class request;
  //! \endcond

public: /*types*/
  using uv_t = ::uv_handle_t;
  using on_destroy_t = std::function< void(void *_data) >;
  /*!< \brief The function type of the callback called when the handle is about to be closed and destroyed.
       \sa libuv documentation: [`uv_close_cb`](http://docs.libuv.org/en/v1.x/handle.html#c.uv_close_cb),
                                [`uv_close()`](http://docs.libuv.org/en/v1.x/handle.html#c.uv_close). */

protected: /*types*/
  //! \cond
  using supplemental_data_t = struct {};

  template< class _HANDLE_ > class instance
  {
  private: /*types*/
    using uv_t = typename _HANDLE_::uv_t;
    using supplemental_data_t = typename _HANDLE_::supplemental_data_t;

  private: /*data*/
    mutable int uv_error;
    void (*Delete)(void*);  // store a proper delete operator
    ref_count rc;
    type_storage< on_destroy_t > on_destroy_storage;
    alignas(64) mutable type_storage< supplemental_data_t > supplemental_data_storage;
    static_assert(sizeof(supplemental_data_storage) <= 64, "non-static layout structure");
    alignas(64) uv_t uv_handle;

  private: /*constructors*/
    instance() : uv_error(0), Delete(default_delete< instance >::Delete)  {}

  public: /*constructors*/
    ~instance() = default;

    instance(const instance&) = delete;
    instance& operator =(const instance&) = delete;

    instance(instance&&) = delete;
    instance& operator =(instance&&) = delete;

  private: /*functions*/
    static void close_cb(::uv_handle_t*);

    void destroy()
    {
      auto t = reinterpret_cast< ::uv_handle_t* >(&uv_handle);
      if (::uv_is_active(t))
        ::uv_close(t, close_cb);
      else
      {
        ::uv_close(t, nullptr);
        close_cb(t);
      }
    }

  public: /*interface*/
    static void* create()  { return std::addressof((new instance())->uv_handle); }

    constexpr static instance* from(void *_uv_handle) noexcept
    {
      static_assert(std::is_standard_layout< instance >::value, "not a standard layout type");
      return reinterpret_cast< instance* >(static_cast< char* >(_uv_handle) - offsetof(instance, uv_handle));
    }

    on_destroy_t& on_destroy() noexcept  { return on_destroy_storage.value(); }
    supplemental_data_t& supplemental_data() const noexcept  { return supplemental_data_storage.value(); }

    void ref()  { rc.inc(); }
    void unref() noexcept  { if (rc.dec() == 0)  destroy(); }
    ref_count::type nrefs() const noexcept  { return rc.value(); }

    int& uv_status() const noexcept  { return uv_error; }
  };
  //! \endcond

protected: /*data*/
  //! \cond
  void *uv_handle;
  //! \endcond

private: /*constructors*/
  explicit handle(uv_t *_uv_handle)
  {
    if (_uv_handle)  instance< handle >::from(_uv_handle)->ref();
    uv_handle = _uv_handle;
  }

protected: /*constructors*/
  handle() noexcept : uv_handle(nullptr)  {}

public: /*constructors*/
  ~handle()  { if (uv_handle)  instance< handle >::from(uv_handle)->unref(); }

  handle(const handle &_that) : handle(static_cast< uv_t* >(_that.uv_handle))  {}
  handle& operator =(const handle &_that)
  {
    if (this != &_that)
    {
      if (_that.uv_handle)  instance< handle >::from(_that.uv_handle)->ref();
      auto t = uv_handle;
      uv_handle = _that.uv_handle;
      if (t)  instance< handle >::from(t)->unref();
    };
    return *this;
  }

  handle(handle &&_that) noexcept : uv_handle(_that.uv_handle)  { _that.uv_handle = nullptr; }
  handle& operator =(handle &&_that) noexcept
  {
    if (this != &_that)
    {
      auto t = uv_handle;
      uv_handle = _that.uv_handle;
      _that.uv_handle = nullptr;
      if (t)  instance< handle >::from(t)->unref();
    };
    return *this;
  }

protected: /*functions*/
  //! \cond
  int uv_status(int _value) const noexcept  { return (instance< handle >::from(uv_handle)->uv_status() = _value); }
  //! \endcond

public: /*interface*/
  void swap(handle &_that) noexcept  { std::swap(uv_handle, _that.uv_handle); }
  /*! \brief The current number of existing references to the same object as this handle variable refers to. */
  long nrefs() const noexcept  { return instance< handle >::from(uv_handle)->nrefs(); }
  /*! \brief The status value returned by the last executed libuv API function. */
  int uv_status() const noexcept  { return instance< handle >::from(uv_handle)->uv_status(); }

  const on_destroy_t& on_destroy() const noexcept  { return instance< handle >::from(uv_handle)->on_destroy(); }
        on_destroy_t& on_destroy()       noexcept  { return instance< handle >::from(uv_handle)->on_destroy(); }

  /*! \brief The tag indicating the libuv type of the handle. */
  ::uv_handle_type type() const noexcept  { return static_cast< uv_t* >(uv_handle)->type; }
  /*! \brief The libuv loop where the handle is running on. */
  uv::loop loop() const noexcept  { return uv::loop{static_cast< uv_t* >(uv_handle)->loop}; }

  /*! \brief The pointer to the user-defined arbitrary data. libuv and uvcc does not use this field. */
  void* const& data() const noexcept  { return static_cast< uv_t* >(uv_handle)->data; }
  void*      & data()       noexcept  { return static_cast< uv_t* >(uv_handle)->data; }

  /*! \details Check if the handle is active.
      \sa libuv documentation: [`uv_is_active()`](http://docs.libuv.org/en/v1.x/handle.html#c.uv_is_active). */
  int is_active() const noexcept  { return uv_status(::uv_is_active(static_cast< uv_t* >(uv_handle))); }
  /*! \details Check if the handle is closing or closed.
      \sa libuv documentation: [`uv_is_closing()`](http://docs.libuv.org/en/v1.x/handle.html#c.uv_is_closing). */
  int is_closing() const noexcept { return uv_status(::uv_is_closing(static_cast< uv_t* >(uv_handle))); }

  /*! \details _Get_ the size of the send buffer that the operating system uses for the socket.
      \sa libuv documentation: [`uv_send_buffer_size()`](http://docs.libuv.org/en/v1.x/handle.html#c.uv_send_buffer_size). */
  unsigned int send_buffer_size() const noexcept
  {
    unsigned int v = 0;
    uv_status(::uv_send_buffer_size(static_cast< uv_t* >(uv_handle), (int*)&v));
    return v;
  }
  /*! \details _Set_ the size of the send buffer that the operating system uses for the socket.
      \sa libuv documentation: [`uv_send_buffer_size()`](http://docs.libuv.org/en/v1.x/handle.html#c.uv_send_buffer_size). */
  void send_buffer_size(const unsigned int _value) noexcept  { uv_status(::uv_send_buffer_size(static_cast< uv_t* >(uv_handle), (int*)&_value)); }

  /*! \details _Get_ the size of the receive buffer that the operating system uses for the socket.
      \sa libuv documentation: [`uv_recv_buffer_size()`](http://docs.libuv.org/en/v1.x/handle.html#c.uv_recv_buffer_size). */
  unsigned int recv_buffer_size() const noexcept
  {
    unsigned int v = 0;
    uv_status(::uv_recv_buffer_size(static_cast< uv_t* >(uv_handle), (int*)&v));
    return v;
  }
  /*! \details _Set_ the size of the receive buffer that the operating system uses for the socket.
      \sa libuv documentation: [`uv_recv_buffer_size()`](http://docs.libuv.org/en/v1.x/handle.html#c.uv_recv_buffer_size). */
  void recv_buffer_size(const unsigned int _value) noexcept  { uv_status(::uv_recv_buffer_size(static_cast< uv_t* >(uv_handle), (int*)&_value)); }

  /*! \details Get the platform dependent file descriptor.
      \sa libuv documentation: [`uv_fileno()`](http://docs.libuv.org/en/v1.x/handle.html#c.uv_fileno). */
  ::uv_os_fd_t fileno() const noexcept
  {
#ifdef _WIN32
    ::uv_os_fd_t fd = INVALID_HANDLE_VALUE;
#else
    ::uv_os_fd_t fd = -1;
#endif
    uv_status(::uv_fileno(static_cast< uv_t* >(uv_handle), &fd));
    return fd;
  }

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_handle); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_handle); }

  explicit operator bool() const noexcept  { return (uv_status() == 0); }  /*!< \brief Equivalent to `(uv_status() == 0)`. */
};

template< class _HANDLE_ >
void handle::instance< _HANDLE_ >::close_cb(::uv_handle_t *_uv_handle)
{
  auto instance = from(_uv_handle);
  auto &destroy_cb = instance->on_destroy();
  if (destroy_cb)  destroy_cb(_uv_handle->data);
  instance->Delete(instance);
}



/*! \brief Stream handle type.
    \sa libuv documentation: [`uv_stream_t`](http://docs.libuv.org/en/v1.x/stream.html#uv-stream-t-stream-handle). */
class stream : public handle
{
  //! \cond
  friend class handle::instance< stream >;
  friend class connect;
  friend class write;
  friend class shutdown;
  //! \endcond

public: /*types*/
  using uv_t = ::uv_stream_t;
  using on_read_t = std::function< void(stream _stream, ssize_t _nread, buffer _buffer) >;
  /*!< \brief The function type of the callback called by `read_start()`.
       \sa libuv documentation: [`uv_read_cb`](http://docs.libuv.org/en/v1.x/stream.html#c.uv_read_cb). */

protected: /*types*/
  //! \cond
  struct supplemental_data_t
  {
    on_buffer_t on_buffer;
    on_read_t on_read;
  };
  //! \endcond

private: /*types*/
  using instance = handle::instance< stream >;

private: /*constructors*/
  explicit stream(uv_t *_uv_handle)
  {
    if (_uv_handle)  instance::from(_uv_handle)->ref();
    uv_handle = _uv_handle;
  }

protected: /*constructors*/
  stream() noexcept = default;

public: /*constructors*/
  ~stream() = default;

  stream(const stream&) = default;
  stream& operator =(const stream&) = default;

  stream(stream&&) noexcept = default;
  stream& operator =(stream&&) noexcept = default;

private: /*functions*/
  template< typename = void > static void alloc_cb(::uv_handle_t*, std::size_t, ::uv_buf_t*);
  template< typename = void > static void read_cb(::uv_stream_t*, ssize_t, const ::uv_buf_t*);

public: /*interface*/
  /*! \details Start reading incoming data from the stream.
      \sa libuv documentation: [`uv_read_start()`](http://docs.libuv.org/en/v1.x/stream.html#c.uv_read_start). */
  int read_start(const on_buffer_t &_alloc_cb, const on_read_t &_read_cb)
  {
    auto &cb = instance::from(uv_handle)->supplemental_data();
    if (cb.on_read)  read_stop();
    cb = {_alloc_cb, _read_cb};
    instance::from(uv_handle)->ref();
    return uv_status(::uv_read_start(static_cast< uv_t* >(uv_handle), alloc_cb, read_cb));
  }
  /*! \details Stop reading data from the stream.
      \sa libuv documentation: [`uv_read_stop()`](http://docs.libuv.org/en/v1.x/stream.html#c.uv_read_stop). */
  int read_stop() noexcept
  {
    uv_status(::uv_read_stop(static_cast< uv_t* >(uv_handle)));
    auto &cb = instance::from(uv_handle)->supplemental_data();
    if (cb.on_read)
    {
      cb.on_read = on_read_t();
      instance::from(uv_handle)->unref();
    };
    return uv_status();
  }

  /*! \brief The amount of queued bytes waiting to be sent. */
  std::size_t write_queue_size() const noexcept  { return static_cast< uv_t* >(uv_handle)->write_queue_size; }
  /*! \brief Check if the stream is readable. */
  bool is_readable() const noexcept  { return uv_status(::uv_is_readable(static_cast< uv_t* >(uv_handle))); }
  /*! \brief Check if the stream is writable. */
  bool is_writable() const noexcept  { return uv_status(::uv_is_writable(static_cast< uv_t* >(uv_handle))); }
  /*! \details Enable or disable blocking mode for the stream.
      \sa libuv documentation: [`uv_stream_set_blocking()`](http://docs.libuv.org/en/v1.x/stream.html#c.uv_stream_set_blocking). */
  int set_blocking(bool _enable) noexcept  { return uv_status(::uv_stream_set_blocking(static_cast< uv_t* >(uv_handle), _enable)); }

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_handle); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_handle); }
};

template< typename >
void stream::alloc_cb(::uv_handle_t *_uv_handle, std::size_t _suggested_size, ::uv_buf_t *_uv_buf)
{
  auto &alloc_cb = instance::from(_uv_handle)->supplemental_data().on_buffer;
  buffer b = alloc_cb(stream(reinterpret_cast< uv_t* >(_uv_handle)), _suggested_size);
  buffer::instance::from(b.uv_buf)->ref();
  *_uv_buf = b[0];
}
template< typename >
void stream::read_cb(::uv_stream_t *_uv_stream, ssize_t _nread, const ::uv_buf_t *_uv_buf)
{
  auto &read_cb = instance::from(_uv_stream)->supplemental_data().on_read;
  if (_uv_buf->base)
  {
    ::uv_buf_t *uv_buf = buffer::instance::from(*_uv_buf);
    buffer b(uv_buf);
    buffer::instance::from(uv_buf)->unref();
    read_cb(stream(_uv_stream), _nread, std::move(b));
  }
  else
    read_cb(stream(_uv_stream), _nread, buffer());
}



/*! \brief TCP handle type.
    \sa libuv documentation: [`uv_tcp_t`](http://docs.libuv.org/en/v1.x/tcp.html#uv-tcp-t-tcp-handle). */
class tcp : public stream
{
  //! \cond
  friend class handle::instance< tcp >;
  friend class connect;
  //! \endcond

public: /*types*/
  using uv_t = ::uv_tcp_t;

private: /*types*/
  using instance = handle::instance< tcp >;

private: /*constructors*/
  explicit tcp(uv_t *_uv_handle)
  {
    if (_uv_handle)  instance::from(_uv_handle)->ref();
    uv_handle = _uv_handle;
  }

public: /*constructors*/
  ~tcp() = default;

  tcp(const tcp&) = default;
  tcp& operator =(const tcp&) = default;

  tcp(tcp&&) noexcept = default;
  tcp& operator =(tcp&&) noexcept = default;

  /*! \details Create a socket with the specified flags.
      \note With `AF_UNSPEC` flag no socket is created.
      \sa libuv documentation: [`uv_tcp_init_ex()`](http://docs.libuv.org/en/v1.x/tcp.html#c.uv_tcp_init_ex). */
  tcp(uv::loop _loop, unsigned int _flags = AF_INET)
  {
    uv_handle = instance::create();
    uv_status(::uv_tcp_init_ex(static_cast< uv::loop::uv_t* >(_loop), static_cast< uv_t* >(uv_handle), _flags));
  }
  /*! \details Create a socket object from an existing OS native socket descriptor.
      \sa libuv documentation: [`uv_tcp_open()`](http://docs.libuv.org/en/v1.x/tcp.html#c.uv_tcp_open),
                               [`uv_tcp_init()`](http://docs.libuv.org/en/v1.x/tcp.html#c.uv_tcp_init). */
  tcp(uv::loop _loop, ::uv_os_sock_t _sock)
  {
    uv_handle = instance::create();
    if (uv_status(::uv_tcp_init(static_cast< uv::loop::uv_t* >(_loop), static_cast< uv_t* >(uv_handle))) != 0)  return;
    uv_status(::uv_tcp_open(static_cast< uv_t* >(uv_handle), _sock));
  }

public: /*intreface*/
  /*! \brief Get the platform dependent socket descriptor. The alias for `handle::fileno()`. */
  ::uv_os_sock_t socket() const noexcept  { return (::uv_os_sock_t)fileno(); }

  /*! \brief Enable or disable Nagle’s algorithm on the socket. */
  int nodelay(bool _enable) noexcept  { return uv_status(::uv_tcp_nodelay(static_cast< uv_t* >(uv_handle), _enable)); }
  /*! \details Enable or disable TCP keep-alive.
      \arg `_delay` is the initial delay in seconds, ignored when `_enable = false`. */
  int keepalive(bool _enable, unsigned int _delay) noexcept  { return uv_status(::uv_tcp_keepalive(static_cast< uv_t* >(uv_handle), _enable, _delay)); }
  /*! \details Enable or disable simultaneous asynchronous accept requests that are queued by the operating system when listening for new TCP connections.
      \sa libuv documentation: [`uv_tcp_simultaneous_accepts()`](http://docs.libuv.org/en/v1.x/tcp.html#c.uv_tcp_simultaneous_accepts). */
  int simultaneous_accepts(bool _enable) noexcept  { return uv_status(::uv_tcp_simultaneous_accepts(static_cast< uv_t* >(uv_handle), _enable)); }
  /*! \details Bind the handle to an address and port.
      \sa libuv documentation: [`uv_tcp_bind()`](http://docs.libuv.org/en/v1.x/tcp.html#c.uv_tcp_bind). */
  template< typename _T_, typename = std::enable_if_t< is_one_of< _T_, ::sockaddr_in, ::sockaddr_in6, ::sockaddr_storage >::value > >
  int bind(const _T_ &_sa, unsigned int _flags) noexcept  { return uv_status(::uv_tcp_bind(static_cast< uv_t* >(uv_handle), reinterpret_cast< const ::sockaddr* >(&_sa), _flags)); }
  /*! \details Get the address which this handle is bound to.
      \sa libuv documentation: [`uv_tcp_getsockname()`](http://docs.libuv.org/en/v1.x/tcp.html#c.uv_tcp_getsockname). */
  template< typename _T_, typename = std::enable_if_t< is_one_of< _T_, ::sockaddr_in, ::sockaddr_in6, ::sockaddr_storage >::value > >
  int getsockname(_T_ &_sockaddr) const noexcept
  {
    int z = sizeof(_T_);
    return uv_status(::uv_tcp_getsockname(static_cast< uv_t* >(uv_handle), reinterpret_cast< ::sockaddr* >(&_sockaddr), &z));
  }
  /*! \details Get the address of the remote peer connected to this handle.
      \sa libuv documentation: [`uv_tcp_getpeername()`](http://docs.libuv.org/en/v1.x/tcp.html#c.uv_tcp_getpeername). */
  template< typename _T_, typename = std::enable_if_t< is_one_of< _T_, ::sockaddr_in, ::sockaddr_in6, ::sockaddr_storage >::value > >
  int getpeername(_T_ &_sockaddr) const noexcept
  {
    int z = sizeof(_T_);
    return uv_status(::uv_tcp_getpeername(static_cast< uv_t* >(uv_handle), reinterpret_cast< ::sockaddr* >(&_sockaddr), &z));
  }

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_handle); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_handle); }
};



/*! \brief Pipe handle type.
    \sa libuv documentation: [`uv_pipe_t`](http://docs.libuv.org/en/v1.x/pipe.html#uv-pipe-t-pipe-handle). */
class pipe : public stream
{
  //! \cond
  friend class handle::instance< pipe >;
  friend class connect;
  friend class write;
  //! \endcond

public: /*types*/
  using uv_t = ::uv_pipe_t;

private: /*types*/
  using instance = handle::instance< pipe >;

private: /*constructors*/
  explicit pipe(uv_t *_uv_handle)
  {
    if (_uv_handle)  instance::from(_uv_handle)->ref();
    uv_handle = _uv_handle;
  }

public: /*constructors*/
  ~pipe() = default;

  pipe(const pipe&) = default;
  pipe& operator =(const pipe&) = default;

  pipe(pipe&&) noexcept = default;
  pipe& operator =(pipe&&) noexcept = default;

  /*! \details Create a pipe bound to a file path (Unix domain socket) or a name (Windows named pipe).
      \sa libuv documentation: [`uv_pipe_init()`](http://docs.libuv.org/en/v1.x/pipe.html#c.uv_pipe_init),
                               [`uv_pipe_bind()`](http://docs.libuv.org/en/v1.x/pipe.html#c.uv_pipe_bind). */
  pipe(uv::loop _loop, const char* _name, bool _ipc = false)
  {
    uv_handle = instance::create();
    if (uv_status(::uv_pipe_init(static_cast< uv::loop::uv_t* >(_loop), static_cast< uv_t* >(uv_handle), _ipc)) != 0)  return;
    uv_status(::uv_pipe_bind(static_cast< uv_t* >(uv_handle), _name));
  }
  /*! \details Create a pipe object from an existing OS native pipe descriptor.
      \sa libuv documentation: [`uv_pipe_open()`](http://docs.libuv.org/en/v1.x/pipe.html#c.uv_pipe_open). */
  pipe(uv::loop _loop, ::uv_file _fd, bool _ipc = false)
  {
    uv_handle = instance::create();
    if (uv_status(::uv_pipe_init(static_cast< uv::loop::uv_t* >(_loop), static_cast< uv_t* >(uv_handle), _ipc)) != 0)  return;
    uv_status(::uv_pipe_open(static_cast< uv_t* >(uv_handle), _fd));
  }

public: /*intreface*/
  /*! \brief Get the name of the Unix domain socket or the Windows named pipe for this pipe object. */
  std::string getsockname() const noexcept
  {
#ifdef UNIX_PATH_MAX
    std::size_t len = UNIX_PATH_MAX;
#else
    std::size_t len = 108;
#endif
    std::string name(len, '\0');
    while ( uv_status(::uv_pipe_getsockname(static_cast< uv_t* >(uv_handle), const_cast< char* >(name.c_str()), &len)) != 0 and uv_status() == UV_ENOBUFS )
        name.resize(len, '\0');
    return name;
  }
  /*! \brief Get the name of the Unix domain socket or the Windows named pipe which this pipe object is connected to. */
  std::string getpeername() const noexcept
  {
#ifdef UNIX_PATH_MAX
    std::size_t len = UNIX_PATH_MAX;
#else
    std::size_t len = 108;
#endif
    std::string name(len, '\0');
    while ( uv_status(::uv_pipe_getpeername(static_cast< uv_t* >(uv_handle), const_cast< char* >(name.c_str()), &len)) != 0 and uv_status() == UV_ENOBUFS )
        name.resize(len, '\0');
    return name;
  }
  /*! \brief Set the number of pending pipe instances when this pipe server is waiting for connections. (_Windows only_) */
  void pending_instances(int _count) noexcept  { ::uv_pipe_pending_instances(static_cast< uv_t* >(uv_handle), _count); }

  /*! \brief Used in conjunction with `pending_type()`. */
  int pending_count() const noexcept  { return ::uv_pipe_pending_count(static_cast< uv_t* >(uv_handle)); }
  /*! \details Used to receive handles over IPC pipes.
      \sa libuv documentation: [`uv_pipe_pending_type()`](http://docs.libuv.org/en/v1.x/pipe.html#c.uv_pipe_pending_type). */
  ::uv_handle_type pending_type() const noexcept  { return ::uv_pipe_pending_type(static_cast< uv_t* >(uv_handle)); }

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_handle); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_handle); }
};



class udp : public handle
{
  //! \cond
  friend class handle::instance< udp >;
  //! \endcond

public: /*types*/
  using uv_t = ::uv_udp_t;
  using on_recv_t = std::function< void(udp _udp, ssize_t _nread, buffer _buffer, const ::sockaddr *_sa, unsigned int _flags) >;
  /*!< \brief The function type of the callback called by `recv_start()`.
       \sa libuv documentation: [`uv_udp_recv_cb`](http://docs.libuv.org/en/v1.x/udp.html#c.uv_udp_recv_cb). */

private: /*types*/
  using instance = handle::instance< udp >;

public: /*constructors*/
  ~udp() = default;

  udp(const udp&) = default;
  udp& operator =(const udp&) = default;

  udp(udp&&) noexcept = default;
  udp& operator =(udp&&) noexcept = default;

public: /*intreface*/
  ::uv_os_sock_t socket() const noexcept  { return (::uv_os_sock_t)fileno(); }

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_handle); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_handle); }
};


//! \}
}


namespace std
{

//! \ingroup g__handle
template<> inline void swap(uv::handle &_this, uv::handle &_that) noexcept  { _this.swap(_that); }

}


#endif
