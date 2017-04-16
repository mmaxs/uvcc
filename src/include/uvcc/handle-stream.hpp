
#ifndef UVCC_HANDLE_STREAM__HPP
#define UVCC_HANDLE_STREAM__HPP

#include "uvcc/utility.hpp"
#include "uvcc/handle-base.hpp"
#include "uvcc/handle-io.hpp"
#include "uvcc/buffer.hpp"
#include "uvcc/loop.hpp"

#include <cstddef>      // size_t
#include <uv.h>

#include <functional>   // function
#include <string>       // string
#include <type_traits>  // enable_if_t


namespace uv
{


/*! \ingroup doxy_group__handle
    \brief Stream handle type.
    \sa libuv API documentation: [`uv_stream_t — Stream handle`](http://docs.libuv.org/en/v1.x/stream.html#uv-stream-t-stream-handle). */
class stream : public io
{
  friend class handle::uv_interface;
  friend class handle::instance< stream >;
  friend class connect;
  friend class write;
  friend class shutdown;
  friend class pipe;

public: /*types*/
  using uv_t = ::uv_stream_t;
  using on_connection_t = std::function< void(stream _server) >;
  /*!< \brief The function type of the callback called when a stream server has received an incoming connection.
       \details The user can accept the connection by calling accept().
       \sa libuv API documentation: [`uv_connection_cb`](http://docs.libuv.org/en/v1.x/stream.html#c.uv_connection_cb). */

protected: /*types*/
  //! \cond internals
  //! \addtogroup doxy_group__internals
  //! \{

  struct properties : io::properties
  {
    on_connection_t connection_cb;
  };

  struct uv_interface : handle::uv_handle_interface, io::uv_interface
  {
    std::size_t write_queue_size(void *_uv_handle) const noexcept override
    { return static_cast< ::uv_stream_t* >(_uv_handle)->write_queue_size; }

    int read_start(void *_uv_handle, int64_t _offset) const noexcept override
    {
      if (_offset >= 0)  instance::from(_uv_handle)->properties().rdoffset = _offset;
      return ::uv_read_start(static_cast< ::uv_stream_t* >(_uv_handle), alloc_cb, read_cb);
    }

    int read_stop(void *_uv_handle) const noexcept override
    { return ::uv_read_stop(static_cast< ::uv_stream_t* >(_uv_handle)); }
  };

  //! \}
  //! \endcond

private: /*types*/
  using instance = handle::instance< stream >;

protected: /*constructors*/
  //! \cond
  stream() noexcept = default;

  explicit stream(uv_t *_uv_handle) : io(static_cast< io::uv_t* >(_uv_handle))  {}
  //! \endcond

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
    auto uv_ret = ::uv_listen(static_cast< uv_t* >(uv_handle), _backlog, connection_cb);
    if (uv_ret < 0)  uv_status(uv_ret);
    return uv_ret;
  }
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

  /*! \brief Enable or disable blocking mode for the stream.
      \sa libuv API documentation: [`uv_stream_set_blocking()`](http://docs.libuv.org/en/v1.x/stream.html#c.uv_stream_set_blocking). */
  int set_blocking(bool _enable) noexcept  { return uv_status(::uv_stream_set_blocking(static_cast< uv_t* >(uv_handle), _enable)); }

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_handle); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_handle); }
};

template< typename >
void stream::alloc_cb(::uv_handle_t *_uv_handle, std::size_t _suggested_size, ::uv_buf_t *_uv_buf)
{ io_alloc_cb(_uv_handle, _suggested_size, _uv_buf); }

template< typename >
void stream::read_cb(::uv_stream_t *_uv_stream, ssize_t _nread, const ::uv_buf_t *_uv_buf)
{ io_read_cb(_uv_stream, _nread, _uv_buf, nullptr); }

template< typename >
void stream::connection_cb(::uv_stream_t *_uv_stream, int _status)
{
  auto instance_ptr = instance::from(_uv_stream);
  instance_ptr->uv_error = _status;
  auto &connection_cb = instance_ptr->properties().connection_cb;
  if (connection_cb)  connection_cb(stream(_uv_stream));
}



/*! \ingroup doxy_group__handle
    \brief TCP handle type.
    \sa libuv API documentation: [`uv_tcp_t — TCP handle`](http://docs.libuv.org/en/v1.x/tcp.html#uv-tcp-t-tcp-handle). */
class tcp : public stream
{
  friend class handle::instance< tcp >;
  friend class connect;

public: /*types*/
  using uv_t = ::uv_tcp_t;

private: /*types*/
  using instance = handle::instance< tcp >;

protected: /*constructors*/
  //! \cond
  explicit tcp(uv_t *_uv_handle) : stream(reinterpret_cast< stream::uv_t* >(_uv_handle))  {}
  //! \endcond

public: /*constructors*/
  ~tcp() = default;

  tcp(const tcp&) = default;
  tcp& operator =(const tcp&) = default;

  tcp(tcp&&) noexcept = default;
  tcp& operator =(tcp&&) noexcept = default;

  /*! \brief Create a TCP socket with the specified flags.
      \note With `AF_UNSPEC` flag no socket is actually created on the system.
      \sa libuv API documentation: [`uv_tcp_init_ex()`](http://docs.libuv.org/en/v1.x/tcp.html#c.uv_tcp_init_ex).\n
          libuv enhancement proposals: <https://github.com/libuv/leps/blob/master/003-create-sockets-early.md>. */
  tcp(uv::loop &_loop, unsigned int _flags = AF_UNSPEC)
  {
    uv_handle = instance::create();

    auto uv_err = ::uv_tcp_init_ex(static_cast< uv::loop::uv_t* >(_loop), static_cast< uv_t* >(uv_handle), _flags);
    if (uv_status(uv_err) < 0)  return;

    instance::from(uv_handle)->book_loop();
  }
  /*! \brief Create a handle object from an existing native platform depended TCP socket descriptor.
      \sa libuv API documentation: [`uv_tcp_open()`](http://docs.libuv.org/en/v1.x/tcp.html#c.uv_tcp_open),
                                   [`uv_tcp_init()`](http://docs.libuv.org/en/v1.x/tcp.html#c.uv_tcp_init). */
  tcp(uv::loop &_loop, ::uv_os_sock_t _socket, bool _set_blocking)
  {
    uv_handle = instance::create();

    auto uv_err = ::uv_tcp_init(static_cast< uv::loop::uv_t* >(_loop), static_cast< uv_t* >(uv_handle));
    if (uv_status(uv_err) < 0)  return;

    instance::from(uv_handle)->book_loop();

    uv_err = ::uv_tcp_open(static_cast< uv_t* >(uv_handle), _socket);
    if (uv_status(uv_err) < 0)  return;

    if (_set_blocking)  set_blocking(_set_blocking);
  }

public: /*interface*/
  /*! \brief Get the platform dependent socket descriptor. The alias for `handle::fileno()`. */
  ::uv_os_sock_t socket() const noexcept  { return (::uv_os_sock_t)fileno(); }

  /*! \brief Enable or disable Nagle’s algorithm on the socket. */
  int nodelay(bool _enable) noexcept  { return uv_status(::uv_tcp_nodelay(static_cast< uv_t* >(uv_handle), _enable)); }
  /*! \brief Enable or disable TCP keep-alive.
      \arg `_delay` is the initial delay in seconds, ignored when `_enable = false`. */
  int keepalive(bool _enable, unsigned int _delay) noexcept  { return uv_status(::uv_tcp_keepalive(static_cast< uv_t* >(uv_handle), _enable, _delay)); }
  /*! \details Enable or disable simultaneous asynchronous accept requests that are queued by the operating system when listening for new TCP connections.
      \sa libuv API documentation: [`uv_tcp_simultaneous_accepts()`](http://docs.libuv.org/en/v1.x/tcp.html#c.uv_tcp_simultaneous_accepts). */
  int simultaneous_accepts(bool _enable) noexcept  { return uv_status(::uv_tcp_simultaneous_accepts(static_cast< uv_t* >(uv_handle), _enable)); }

  /*! \brief Bind the handle to an address and port.
      \sa libuv API documentation: [`uv_tcp_bind()`](http://docs.libuv.org/en/v1.x/tcp.html#c.uv_tcp_bind). */
  template<
      typename _T_,
      typename = std::enable_if_t< is_one_of< _T_, ::sockaddr, ::sockaddr_in, ::sockaddr_in6, ::sockaddr_storage >::value >
  >
  int bind(const _T_ &_sockaddr, unsigned int _flags = 0) noexcept
  {
    return uv_status(::uv_tcp_bind(static_cast< uv_t* >(uv_handle), reinterpret_cast< const ::sockaddr* >(&_sockaddr), _flags));
  }

  /*! \brief Get the local address which this handle is bound to.
      \returns `true` if the operation has completed successfully (can be checked with `uv_status()`) and
      the size of the passed argument (i.e. `sizeof(_T_)`) is enough to hold the returned socket address structure.
      \sa libuv API documentation: [`uv_tcp_getsockname()`](http://docs.libuv.org/en/v1.x/tcp.html#c.uv_tcp_getsockname). */
  template< typename _T_, typename = std::enable_if_t< is_one_of< _T_, ::sockaddr_in, ::sockaddr_in6, ::sockaddr_storage >::value > >
  bool getsockname(_T_ &_sockaddr) const noexcept
  {
    int z = sizeof(_T_);
    return
        uv_status(::uv_tcp_getsockname(static_cast< uv_t* >(uv_handle), reinterpret_cast< ::sockaddr* >(&_sockaddr), &z))
      and
        sizeof(_T_) >= z;
  }
  /*! \brief Get the address of the remote peer connected to this handle.
      \returns `true` if the operation has completed successfully (can be checked with `uv_status()`) and
      the size of the passed argument (i.e. `sizeof(_T_)`) is enough to hold the returned socket address structure.
      \sa libuv API documentation: [`uv_tcp_getpeername()`](http://docs.libuv.org/en/v1.x/tcp.html#c.uv_tcp_getpeername). */
  template< typename _T_, typename = std::enable_if_t< is_one_of< _T_, ::sockaddr_in, ::sockaddr_in6, ::sockaddr_storage >::value > >
  bool getpeername(_T_ &_sockaddr) const noexcept
  {
    int z = sizeof(_T_);
    return
        uv_status(::uv_tcp_getpeername(static_cast< uv_t* >(uv_handle), reinterpret_cast< ::sockaddr* >(&_sockaddr), &z))
      and
        sizeof(_T_) >= z;
  }

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



/*! \ingroup doxy_group__handle
    \brief Pipe handle type.
    \sa libuv API documentation: [`uv_pipe_t — Pipe handle`](http://docs.libuv.org/en/v1.x/pipe.html#uv-pipe-t-pipe-handle). */
class pipe : public stream
{
  friend class handle::instance< pipe >;
  friend class connect;
  friend class write;

public: /*types*/
  using uv_t = ::uv_pipe_t;

private: /*types*/
  using instance = handle::instance< pipe >;

protected: /*constructors*/
  //! \cond
  explicit pipe(uv_t *_uv_handle) : stream(reinterpret_cast< stream::uv_t* >(_uv_handle))  {}
  //! \endcond

public: /*constructors*/
  ~pipe() = default;

  pipe(const pipe&) = default;
  pipe& operator =(const pipe&) = default;

  pipe(pipe&&) noexcept = default;
  pipe& operator =(pipe&&) noexcept = default;

  /*! \brief Create a pipe bound to a file path (Unix domain socket) or a name (Windows named pipe).
      \sa libuv API documentation: [`uv_pipe_init()`](http://docs.libuv.org/en/v1.x/pipe.html#c.uv_pipe_init),
                                   [`uv_pipe_bind()`](http://docs.libuv.org/en/v1.x/pipe.html#c.uv_pipe_bind). */
  pipe(uv::loop &_loop, const char* _name, bool _ipc = false)
  {
    uv_handle = instance::create();

    auto uv_err = ::uv_pipe_init(static_cast< uv::loop::uv_t* >(_loop), static_cast< uv_t* >(uv_handle), _ipc);
    if (uv_status(uv_err) < 0)  return;

    instance::from(uv_handle)->book_loop();

    uv_status(::uv_pipe_bind(static_cast< uv_t* >(uv_handle), _name));
  }
  /*! \brief Create a pipe object from an existing OS native pipe descriptor.
      \sa libuv API documentation: [`uv_pipe_open()`](http://docs.libuv.org/en/v1.x/pipe.html#c.uv_pipe_open). */
  pipe(uv::loop &_loop, ::uv_file _fd, bool _ipc = false, bool _set_blocking = false)
  {
    uv_handle = instance::create();

    auto uv_err = ::uv_pipe_init(static_cast< uv::loop::uv_t* >(_loop), static_cast< uv_t* >(uv_handle), _ipc);
    if (uv_status(uv_err) < 0)  return;

    instance::from(uv_handle)->book_loop();

    uv_err = ::uv_pipe_open(static_cast< uv_t* >(uv_handle), _fd);
    if (uv_status(uv_err) < 0)  return;

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
    while (
        uv_status(::uv_pipe_getsockname(static_cast< uv_t* >(uv_handle), const_cast< char* >(name.c_str()), &len)) < 0
      and
        uv_status() == UV_ENOBUFS
    )  name.resize(len, '\0');
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
    while (
        uv_status(::uv_pipe_getpeername(static_cast< uv_t* >(uv_handle), const_cast< char* >(name.c_str()), &len)) < 0
      and
        uv_status() == UV_ENOBUFS
    )  name.resize(len, '\0');
    return name;
  }
  /*! \brief Set the number of pending pipe instances when this pipe server is waiting for connections. (_Windows only._) */
  void pending_instances(int _count) noexcept  { ::uv_pipe_pending_instances(static_cast< uv_t* >(uv_handle), _count); }

  /*! \brief The number of pending handles being sent over the IPC pipe.
      \details Used in conjunction with `accept_pending_handle()`. */
  int pending_handle_count() const noexcept  { return ::uv_pipe_pending_count(static_cast< uv_t* >(uv_handle)); }
  /*! \brief Used to receive handles over the IPC pipe.
      \sa libuv API documentation: [`uv_pipe_pending_type()`](http://docs.libuv.org/en/v1.x/pipe.html#c.uv_pipe_pending_type). */
  stream accept_pending_handle() const;

#if 1
  /*! \brief _Get_ the size of the send buffer that the operating system uses for the pipe.
      \sa libuv API documentation: [`uv_send_buffer_size()`](http://docs.libuv.org/en/v1.x/handle.html#c.uv_send_buffer_size). */
  unsigned int send_buffer_size() const noexcept
  {
    unsigned int v = 0;
    uv_status(::uv_send_buffer_size(static_cast< handle::uv_t* >(uv_handle), (int*)&v));
    return v;
  }
  /*! \brief _Set_ the size of the send buffer that the operating system uses for the pipe.
      \sa libuv API documentation: [`uv_send_buffer_size()`](http://docs.libuv.org/en/v1.x/handle.html#c.uv_send_buffer_size). */
  void send_buffer_size(const unsigned int _value) noexcept  { uv_status(::uv_send_buffer_size(static_cast< handle::uv_t* >(uv_handle), (int*)&_value)); }

  /*! \brief _Get_ the size of the receive buffer that the operating system uses for the pipe.
      \sa libuv API documentation: [`uv_recv_buffer_size()`](http://docs.libuv.org/en/v1.x/handle.html#c.uv_recv_buffer_size). */
  unsigned int recv_buffer_size() const noexcept
  {
    unsigned int v = 0;
    uv_status(::uv_recv_buffer_size(static_cast< handle::uv_t* >(uv_handle), (int*)&v));
    return v;
  }
  /*! \brief _Set_ the size of the receive buffer that the operating system uses for the pipe.
      \sa libuv API documentation: [`uv_recv_buffer_size()`](http://docs.libuv.org/en/v1.x/handle.html#c.uv_recv_buffer_size). */
  void recv_buffer_size(const unsigned int _value) noexcept  { uv_status(::uv_recv_buffer_size(static_cast< handle::uv_t* >(uv_handle), (int*)&_value)); }
#endif

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_handle); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_handle); }
};



/*! \ingroup doxy_group__handle
    \brief TTY handle type.
    \sa libuv API documentation: [`uv_tty_t — TTY handle`](http://docs.libuv.org/en/v1.x/tty.html#uv-tty-t-tty-handle). */
class tty : public stream
{
  friend class handle::instance< tty >;

public: /*types*/
  using uv_t = ::uv_tty_t;

private: /*types*/
  using instance = handle::instance< pipe >;

protected: /*constructors*/
  //! \cond
  explicit tty(uv_t *_uv_handle) : stream(reinterpret_cast< stream::uv_t* >(_uv_handle))  {}
  //! \endcond

public: /*constructors*/
  ~tty() = default;

  tty(const tty&) = default;
  tty& operator =(const tty&) = default;

  tty(tty&&) noexcept = default;
  tty& operator =(tty&&) noexcept = default;

  /*! \brief Create a tty object from the given TTY file descriptor.
      \sa libuv API documentation: [`uv_tty_init()`](http://docs.libuv.org/en/v1.x/tty.html#c.uv_tty_init). */
  tty(uv::loop &_loop, ::uv_file _fd, bool _readable, bool _set_blocking = false)
  {
    uv_handle = instance::create();

    auto uv_err = ::uv_tty_init(static_cast< uv::loop::uv_t* >(_loop), static_cast< uv_t* >(uv_handle), _fd, _readable);
    if (uv_status(uv_err) < 0)  return;

    instance::from(uv_handle)->book_loop();

    if (_set_blocking)  set_blocking(_set_blocking);
  }

public: /*interface*/
  /*! \brief Set the TTY using the specified terminal mode.
      \sa libuv API documentation: [`uv_tty_set_mode()`](http://docs.libuv.org/en/v1.x/tty.html#c.uv_tty_set_mode),
                                   [`uv_tty_mode_t`](http://docs.libuv.org/en/v1.x/tty.html#c.uv_tty_mode_t). */
  int set_mode(::uv_tty_mode_t _mode) noexcept
  {
    return uv_status(::uv_tty_set_mode(static_cast< uv_t* >(uv_handle), _mode));
  }

  /*! \brief Get the current window size.
      \sa libuv API documentation: [`uv_tty_get_winsize()`](http://docs.libuv.org/en/v1.x/tty.html#c.uv_tty_get_winsize). */
  int get_winsize(int &_width, int &_height) const noexcept
  {
    return uv_status(::uv_tty_get_winsize(static_cast< uv_t* >(uv_handle), &_width, &_height));
  }

  /*! \brief Reset TTY settings to default values. To be called when the program exits.
      \sa libuv API documentation: [`uv_tty_reset_mode()`](http://docs.libuv.org/en/v1.x/tty.html#c.uv_tty_reset_mode). */
  static int reset_mode() noexcept  { return ::uv_tty_reset_mode(); }

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_handle); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_handle); }
};



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
      if (client) handle::instance< pipe >::from(client.uv_handle)->book_loop();
      break;
  case UV_TCP:
      client.uv_handle = handle::instance< tcp >::create();
      client.uv_status(::uv_tcp_init(
          static_cast< ::uv_tcp_t* >(uv_handle)->loop,
          static_cast< ::uv_tcp_t* >(client.uv_handle)
      ));
      if (client) handle::instance< tcp >::from(client.uv_handle)->book_loop();
      break;
  case UV_TTY:
      client.uv_status(UV_ENOTSUP);
      break;
  default:
      client.uv_status(UV_EBADF);
      break;
  }

  if (!client)
    uv_status(client.uv_status());
  else
  {
    auto uv_ret = ::uv_accept(static_cast< uv_t* >(uv_handle), static_cast< uv_t* >(client));
    if (uv_status(uv_ret) < 0)  client.uv_status(uv_ret);
  }

  return client;
}


inline stream pipe::accept_pending_handle() const
{
  stream fd;

  switch (::uv_pipe_pending_type(static_cast< uv_t* >(uv_handle)))
  {
  case UV_NAMED_PIPE:
      fd.uv_handle = handle::instance< pipe >::create();
      fd.uv_status(::uv_pipe_init(
          static_cast< uv_t* >(uv_handle)->loop,
          static_cast< ::uv_pipe_t* >(fd.uv_handle),
          static_cast< uv_t* >(uv_handle)->ipc
      ));
      if (fd) handle::instance< pipe >::from(fd.uv_handle)->book_loop();
      break;
  case UV_TCP:
      fd.uv_handle = handle::instance< tcp >::create();
      fd.uv_status(::uv_tcp_init(
          static_cast< uv_t* >(uv_handle)->loop,
          static_cast< ::uv_tcp_t* >(fd.uv_handle)
      ));
      if (fd) handle::instance< tcp >::from(fd.uv_handle)->book_loop();
      break;
  default:
      fd.uv_status(UV_EBADF);
      break;
  }

  if (!fd)
    uv_status(fd.uv_status());
  else
  {
    auto uv_ret = ::uv_accept(static_cast< ::uv_stream_t* >(uv_handle), static_cast< ::uv_stream_t* >(fd));
    if (uv_status(uv_ret) < 0)  fd.uv_status(uv_ret);
  }

  return fd;
}


}


#endif
