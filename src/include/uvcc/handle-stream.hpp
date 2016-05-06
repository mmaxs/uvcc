
#ifndef UVCC_HANDLE_STREAM__HPP
#define UVCC_HANDLE_STREAM__HPP

#include "uvcc/utility.hpp"
#include "uvcc/handle-base.hpp"
#include "uvcc/handle-io.hpp"
#include "uvcc/buffer.hpp"
#include "uvcc/loop.hpp"

#include <uv.h>
#include <cstddef>      // size_t
#include <functional>   // function
#include <string>       // string


namespace uv
{


/*! \ingroup doxy_group_handle
    \brief Stream handle type.
    \sa libuv API documentation: [`uv_stream_t`](http://docs.libuv.org/en/v1.x/stream.html#uv-stream-t-stream-handle).
    \note `read_start()` and `read_stop()` functions are mutually exclusive and thread-safe. */
class stream : public io
{
  //! \cond
  friend class handle::instance< stream >;
  friend class connect;
  friend class write;
  friend class shutdown;
  //! \endcond

public: /*types*/
  using uv_t = ::uv_stream_t;
  using on_connection_t = std::function< void(stream _server) >;
  /*!< \brief The function type of the callback called when a stream server has received an incoming connection.
       \details The user can accept the connection by calling accept().
       \sa libuv API documentation: [`uv_connection_cb`](http://docs.libuv.org/en/v1.x/stream.html#c.uv_connection_cb). */

protected: /*types*/
  //! \cond
  struct properties : io::properties
  {
    on_connection_t connection_cb;
  };

  struct uv_interface : uv_handle_interface, io::uv_interface
  {
    int read_start(void *_uv_handle) const noexcept override
    { return ::uv_read_start(static_cast< ::uv_stream_t* >(_uv_handle), alloc_cb, read_cb); }

    int read_stop(void *_uv_handle) const noexcept override
    { return ::uv_read_stop(static_cast< ::uv_stream_t* >(_uv_handle)); }
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
  template< typename = void > static void connection_cb(::uv_stream_t*, int);

public: /*interface*/
  on_connection_t& on_connection() const noexcept  { return instance::from(uv_handle)->properties().connection_cb; }

  /*! \brief Start listening for incoming connections.
      \sa libuv API documentation: [`uv_listen()`](http://docs.libuv.org/en/v1.x/stream.html#c.uv_listen).*/
  int listen(int _backlog, const on_connection_t &_connection_cb) const
  {
    instance::from(uv_handle)->properties().connection_cb = _connection_cb;
    uv_status(0);
    int o = ::uv_listen(static_cast< uv_t* >(uv_handle), _backlog, connection_cb);
    if (!o)  uv_status(o);
    return o;
  }
#if 0
  /*! \brief Accept incoming connections.
      \details There are specializations of this function for every `stream` subtype.
      ```
      template<> tcp accept< tcp >() const;
      template<> pipe accept< pipe >() const;
      template<> tty accept< tty >() const;
      ```
      \sa libuv API documentation: [`uv_accept()`](http://docs.libuv.org/en/v1.x/stream.html#c.uv_accept). */
  template< class _STREAM_ > _STREAM_ accept() const;
#endif
  /*! \brief Accept incoming connections.
      \details The function returns `stream` instance that actually is an object of one of the stream subtype:
      `tcp`, `pipe`, or `tty` depending on the actual subtype of the stream object which this function is applied to.
      \sa libuv API documentation: [`uv_accept()`](http://docs.libuv.org/en/v1.x/stream.html#c.uv_accept). */
  stream accept() const;

  /*! \brief The amount of queued bytes waiting to be sent. */
  std::size_t write_queue_size() const noexcept  { return static_cast< uv_t* >(uv_handle)->write_queue_size; }
  /*! \brief Check if the stream is readable. */
  bool is_readable() const noexcept  { return uv_status(::uv_is_readable(static_cast< uv_t* >(uv_handle))); }
  /*! \brief Check if the stream is writable. */
  bool is_writable() const noexcept  { return uv_status(::uv_is_writable(static_cast< uv_t* >(uv_handle))); }
  /*! \details Enable or disable blocking mode for the stream.
      \sa libuv API documentation: [`uv_stream_set_blocking()`](http://docs.libuv.org/en/v1.x/stream.html#c.uv_stream_set_blocking). */
  int set_blocking(bool _enable) noexcept  { return uv_status(::uv_stream_set_blocking(static_cast< uv_t* >(uv_handle), _enable)); }

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_handle); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_handle); }
};

template< typename >
void stream::alloc_cb(::uv_handle_t *_uv_handle, std::size_t _suggested_size, ::uv_buf_t *_uv_buf)
{
#if 0
  auto &alloc_cb = instance::from(_uv_handle)->properties().alloc_cb;
  buffer &&b = alloc_cb(stream(reinterpret_cast< uv_t* >(_uv_handle)), _suggested_size);
  buffer::instance::from(b.uv_buf)->ref();  // add the reference for the future moving the buffer instance into read_cb() parameter
  *_uv_buf = b[0];
#endif
  io_alloc(_uv_handle, _suggested_size, _uv_buf);
}
template< typename >
void stream::read_cb(::uv_stream_t *_uv_stream, ssize_t _nread, const ::uv_buf_t *_uv_buf)
{
#if 0
  auto instance_ptr = instance::from(_uv_stream);
  instance_ptr->uv_error = _nread;

  auto &read_cb = instance_ptr->properties().read_cb;
  if (_uv_buf->base)
    read_cb(stream(_uv_stream), _nread, buffer(buffer::instance::from_base(_uv_buf->base), adopt_ref), nullptr);
    // don't forget to specify adopt_ref flag when using ref_guard to unref the object
    // don't use ref_guard unless it really needs to hold on the object until the scope end
    // use move/transfer semantics instead if you need just pass the object to another function for further processing
  else
    read_cb(stream(_uv_stream), _nread, buffer(), nullptr);
#endif
  io_read(_uv_stream, _nread, _uv_buf, nullptr);
}
template< typename >
void stream::connection_cb(::uv_stream_t *_uv_stream, int _status)
{
  auto instance_ptr = instance::from(_uv_stream);
  instance_ptr->uv_error = _status;
  auto &connection_cb = instance_ptr->properties().connection_cb;
  if (connection_cb)  connection_cb(stream(_uv_stream));
}



/*! \ingroup doxy_group_handle
    \brief TCP handle type.
    \sa libuv API documentation: [`uv_tcp_t`](http://docs.libuv.org/en/v1.x/tcp.html#uv-tcp-t-tcp-handle). */
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
      \note With `AF_UNSPEC` flag no socket is actually created on the system.
      \sa libuv API documentation: [`uv_tcp_init_ex()`](http://docs.libuv.org/en/v1.x/tcp.html#c.uv_tcp_init_ex).
      \sa libuv enhancement proposals: <https://github.com/libuv/leps/blob/master/003-create-sockets-early.md>. */
  tcp(uv::loop _loop, unsigned int _flags = AF_UNSPEC)
  {
    uv_handle = instance::create();
    uv_status(::uv_tcp_init_ex(static_cast< uv::loop::uv_t* >(_loop), static_cast< uv_t* >(uv_handle), _flags));
  }
  /*! \details Create a socket object from an existing native platform depended socket descriptor.
      \sa libuv API documentation: [`uv_tcp_open()`](http://docs.libuv.org/en/v1.x/tcp.html#c.uv_tcp_open),
                                   [`uv_tcp_init()`](http://docs.libuv.org/en/v1.x/tcp.html#c.uv_tcp_init). */
  tcp(uv::loop _loop, ::uv_os_sock_t _sock, bool _set_blocking)
  {
    uv_handle = instance::create();
    if (uv_status(::uv_tcp_init(static_cast< uv::loop::uv_t* >(_loop), static_cast< uv_t* >(uv_handle))) != 0)  return;
    if (uv_status(::uv_tcp_open(static_cast< uv_t* >(uv_handle), _sock)) != 0)  return;
    if (_set_blocking)  set_blocking(_set_blocking);
  }

public: /*interface*/
  /*! \brief Get the platform dependent socket descriptor. The alias for `handle::fileno()`. */
  ::uv_os_sock_t socket() const noexcept  { return (::uv_os_sock_t)fileno(); }

  /*! \brief Enable or disable Nagle’s algorithm on the socket. */
  int nodelay(bool _enable) noexcept  { return uv_status(::uv_tcp_nodelay(static_cast< uv_t* >(uv_handle), _enable)); }
  /*! \details Enable or disable TCP keep-alive.
      \arg `_delay` is the initial delay in seconds, ignored when `_enable = false`. */
  int keepalive(bool _enable, unsigned int _delay) noexcept  { return uv_status(::uv_tcp_keepalive(static_cast< uv_t* >(uv_handle), _enable, _delay)); }
  /*! \details Enable or disable simultaneous asynchronous accept requests that are queued by the operating system when listening for new TCP connections.
      \sa libuv API documentation: [`uv_tcp_simultaneous_accepts()`](http://docs.libuv.org/en/v1.x/tcp.html#c.uv_tcp_simultaneous_accepts). */
  int simultaneous_accepts(bool _enable) noexcept  { return uv_status(::uv_tcp_simultaneous_accepts(static_cast< uv_t* >(uv_handle), _enable)); }

  /*! \details Bind the handle to an address and port.
      \sa libuv API documentation: [`uv_tcp_bind()`](http://docs.libuv.org/en/v1.x/tcp.html#c.uv_tcp_bind). */
  template< typename _T_, typename = std::enable_if_t< is_one_of< _T_, ::sockaddr_in, ::sockaddr_in6, ::sockaddr_storage >::value > >
  int bind(const _T_ &_sa, unsigned int _flags = 0) noexcept
  {
    return uv_status(::uv_tcp_bind(static_cast< uv_t* >(uv_handle), reinterpret_cast< const ::sockaddr* >(&_sa), _flags));
  }

  /*! \details Get the address which this handle is bound to.
      \sa libuv API documentation: [`uv_tcp_getsockname()`](http://docs.libuv.org/en/v1.x/tcp.html#c.uv_tcp_getsockname). */
  template< typename _T_, typename = std::enable_if_t< is_one_of< _T_, ::sockaddr_in, ::sockaddr_in6, ::sockaddr_storage >::value > >
  int getsockname(_T_ &_sockaddr) const noexcept
  {
    int z = sizeof(_T_);
    return uv_status(::uv_tcp_getsockname(static_cast< uv_t* >(uv_handle), reinterpret_cast< ::sockaddr* >(&_sockaddr), &z));
  }
  /*! \details Get the address of the remote peer connected to this handle.
      \sa libuv API documentation: [`uv_tcp_getpeername()`](http://docs.libuv.org/en/v1.x/tcp.html#c.uv_tcp_getpeername). */
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


#if 0
//! \cond
template<> tcp stream::accept< tcp >() const
{
  using instance = handle::instance< tcp >;

  stream client;
  client.uv_handle = instance::create();
  if (
    client.uv_status(::uv_tcp_init(
        static_cast< instance::uv_t* >(uv_handle)->loop,
        static_cast< instance::uv_t* >(client.uv_handle)
    )) < 0
  )
    uv_status(client.uv_status());
  else if (
    uv_status(::uv_accept(static_cast< stream::uv_t* >(uv_handle), static_cast< stream::uv_t* >(client))) < 0
  )
    client.uv_status(uv_status());

  return static_cast< tcp& >(client);
}
//! \endcond
#endif


/*! \ingroup doxy_group_handle
    \brief Pipe handle type.
    \sa libuv API documentation: [`uv_pipe_t`](http://docs.libuv.org/en/v1.x/pipe.html#uv-pipe-t-pipe-handle). */
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
      \sa libuv API documentation: [`uv_pipe_init()`](http://docs.libuv.org/en/v1.x/pipe.html#c.uv_pipe_init),
                               [`uv_pipe_bind()`](http://docs.libuv.org/en/v1.x/pipe.html#c.uv_pipe_bind). */
  pipe(uv::loop _loop, const char* _name, bool _ipc = false)
  {
    uv_handle = instance::create();
    if (uv_status(::uv_pipe_init(static_cast< uv::loop::uv_t* >(_loop), static_cast< uv_t* >(uv_handle), _ipc)) != 0)  return;
    uv_status(::uv_pipe_bind(static_cast< uv_t* >(uv_handle), _name));
  }
  /*! \details Create a pipe object from an existing OS native pipe descriptor.
      \sa libuv API documentation: [`uv_pipe_open()`](http://docs.libuv.org/en/v1.x/pipe.html#c.uv_pipe_open). */
  pipe(uv::loop _loop, ::uv_file _fd, bool _ipc = false, bool _set_blocking = false)
  {
    uv_handle = instance::create();
    if (uv_status(::uv_pipe_init(static_cast< uv::loop::uv_t* >(_loop), static_cast< uv_t* >(uv_handle), _ipc)) != 0)  return;
    if (uv_status(::uv_pipe_open(static_cast< uv_t* >(uv_handle), _fd)) != 0)  return;
    if (_set_blocking)  set_blocking(_set_blocking);
  }

public: /*interface*/
  /*! \brief Non-zero if this pipe is used for passing handles. */
  int ipc() const noexcept  { return static_cast< uv_t* >(uv_handle)->ipc; }

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
      \sa libuv API documentation: [`uv_pipe_pending_type()`](http://docs.libuv.org/en/v1.x/pipe.html#c.uv_pipe_pending_type). */
  ::uv_handle_type pending_type() const noexcept  { return ::uv_pipe_pending_type(static_cast< uv_t* >(uv_handle)); }

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_handle); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_handle); }
};


#if 0
//! \cond
template<> pipe stream::accept< pipe >() const
{
  using instance = handle::instance< pipe >;

  stream client;
  client.uv_handle = instance::create();
  if (
    client.uv_status(::uv_pipe_init(
        static_cast< instance::uv_t* >(uv_handle)->loop,
        static_cast< instance::uv_t* >(client.uv_handle),
        static_cast< instance::uv_t* >(uv_handle)->ipc
    )) < 0
  )
    uv_status(client.uv_status());
  else if (
    uv_status(::uv_accept(static_cast< stream::uv_t* >(uv_handle), static_cast< stream::uv_t* >(client))) < 0
  )
    client.uv_status(uv_status());

  return static_cast< pipe& >(client);
}
//! \endcond
#endif

inline stream stream::accept() const
{
  stream client;

  switch (static_cast< uv_t* >(uv_handle)->type)
  {
  case UV_NAMED_PIPE:
      client.uv_handle = handle::instance< pipe >::create();
      client.uv_status(::uv_pipe_init(
          static_cast< ::uv_pipe_t* >(uv_handle)->loop,
          static_cast< ::uv_pipe_t* >(client.uv_handle),
          static_cast< ::uv_pipe_t* >(uv_handle)->ipc
      ));
      break;
  case UV_TCP:
      client.uv_handle = handle::instance< tcp >::create();
      client.uv_status(::uv_tcp_init(
          static_cast< ::uv_tcp_t* >(uv_handle)->loop,
          static_cast< ::uv_tcp_t* >(client.uv_handle)
      ));
      break;
  case UV_TTY:
      break;
  default:
      client.uv_status(UV_EBADF);
      break;
  }

  if (!client)
    uv_status(client.uv_status());
  else if (uv_status(::uv_accept(static_cast< uv_t* >(uv_handle), static_cast< uv_t* >(client))) < 0)
    client.uv_status(uv_status());

  return client;
}


}


#endif
