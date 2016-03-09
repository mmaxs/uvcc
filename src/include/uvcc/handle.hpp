
#ifndef UVCC_HANDLE__HPP
#define UVCC_HANDLE__HPP

#include "uvcc/utility.hpp"
#include "uvcc/loop.hpp"

#include <uv.h>
#include <cstddef>      // offsetof
#include <functional>   // function
#include <type_traits>  // is_standard_layout enable_if_t
#include <utility>      // swap() move()
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
    These libuv functions can be directly applied to uvcc handle objects if necessary. */
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
    \brief Defines the correspondence between libuv handle data types and C++ classes representing them. */
//! \{
#define BUGGY_DOXYGEN
#undef BUGGY_DOXYGEN
//! \cond
template< typename _UV_T_ > struct uv_handle_traits  {};
//! \endcond
#define XX(_, x) template<> struct uv_handle_traits< uv_##x##_t > { using type = x; };
UV_HANDLE_TYPE_MAP(XX)
#undef XX
//! \}


/*! \brief The base class for the libuv handles.
    \details Derived classes conceptually are just interfaces to the data stored
    in the base class, so there are no any virtual member functions.
    \sa libuv documentation: [`uv_handle_t`](http://docs.libuv.org/en/v1.x/handle.html#uv-handle-t-base-handle). */
class handle
{
  friend class loop;
  friend class request;

public: /*types*/
  using uv_t = ::uv_handle_t;
  using on_destroy_t = std::function< void(void*) >;
  /*!< \brief The function type of the callback called when the handle is about to be closed and destroyed.
       \sa libuv documentation: [`uv_close_cb`](http://docs.libuv.org/en/v1.x/handle.html#c.uv_close_cb),
                                [`uv_close()`](http://docs.libuv.org/en/v1.x/handle.html#c.uv_close). */

protected: /*types*/
  //! \cond
  template< typename _UV_T_ > class base
  {
  private: /*data*/
    void (*Delete)(void*);  // store a proper delete operator
    ref_count rc;
    type_storage< on_destroy_t > on_destroy_storage;
    alignas(::uv_any_handle) _UV_T_ uv_handle;

  private: /*constructors*/
    base() : Delete(default_delete< base >::Delete)  {}

  public: /*constructors*/
    ~base() = default;

    base(const base&) = delete;
    base& operator =(const base&) = delete;

    base(base&&) = delete;
    base& operator =(base&&) = delete;

  private: /*functions*/
    static void close_cb(::uv_handle_t*);

    void destroy()
    {
      auto t = reinterpret_cast< uv_t* >(&uv_handle);
      if (::uv_is_active(t))
        ::uv_close(t, close_cb);
      else
      {
        ::uv_close(t, nullptr);
        close_cb(t);
      }
    }

  public: /*interface*/
    static void* create()  { return std::addressof((new base())->uv_handle); }

    constexpr static base* from(void *_uv_handle) noexcept
    {
      static_assert(std::is_standard_layout< base >::value, "not a standard layout type");
      return reinterpret_cast< base* >(static_cast< char* >(_uv_handle) - offsetof(base, uv_handle));
    }

    on_destroy_t& on_destroy() noexcept  { return on_destroy_storage.value(); }

    void ref()  { rc.inc(); }
    void unref() noexcept  { if (rc.dec() == 0)  destroy(); }
    ref_count::type nrefs() const noexcept  { return rc.value(); }
  };
  //! \endcond

protected: /*data*/
  //! \cond
  void *uv_handle;
  //! \endcond

private: /*constructors*/
  explicit handle(uv_t *_uv_handle)
  {
    base< uv_t >::from(_uv_handle)->ref();
    uv_handle = _uv_handle;
  }

protected: /*constructors*/
  handle() noexcept : uv_handle(nullptr)  {}

public: /*constructors*/
  ~handle()  { if (uv_handle)  base< uv_t >::from(uv_handle)->unref(); }

  handle(const handle &_that)
  {
    base< uv_t >::from(_that.uv_handle)->ref();
    uv_handle = _that.uv_handle;
  }
  handle& operator =(const handle &_that)
  {
    if (this != &_that)
    {
      base< uv_t >::from(_that.uv_handle)->ref();
      auto t = uv_handle;
      uv_handle = _that.uv_handle;
      base< uv_t >::from(t)->unref();
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
      base< uv_t >::from(t)->unref();
    };
    return *this;
  }

public: /*interface*/
  void swap(handle &_that) noexcept  { std::swap(uv_handle, _that.uv_handle); }
  /*! \brief The current number of existing references to the same object as this handle variable refers to. */
  long nrefs() const noexcept  { return base< uv_t >::from(uv_handle)->nrefs(); }

  const on_destroy_t& on_destroy() const noexcept  { return base< uv_t >::from(uv_handle)->on_destroy(); }
        on_destroy_t& on_destroy()       noexcept  { return base< uv_t >::from(uv_handle)->on_destroy(); }

  /*! \brief The tag indicating the libuv type of the handle. */
  ::uv_handle_type type() const noexcept  { return static_cast< uv_t* >(uv_handle)->type; }
  /*! \brief The libuv loop where the handle is running on. */
  uv::loop loop() const noexcept  { return uv::loop{static_cast< uv_t* >(uv_handle)->loop}; }

  /*! \brief The pointer to the user-defined arbitrary data. libuv and uvcc does not use this field. */
  void* const& data() const noexcept  { return static_cast< uv_t* >(uv_handle)->data; }
  void*      & data()       noexcept  { return static_cast< uv_t* >(uv_handle)->data; }

  /*! \details Check if the handle is active.
      \sa libuv documentation: [`uv_is_active()`](http://docs.libuv.org/en/v1.x/handle.html#c.uv_is_active). */
  int is_active() const noexcept  { return ::uv_is_active(*this); }
  /*! \details Check if the handle is closing or closed.
      \sa libuv documentation: [`uv_is_closing()`](http://docs.libuv.org/en/v1.x/handle.html#c.uv_is_closing). */
  int is_closing() const noexcept { return ::uv_is_closing(*this); }

  /*! \details _Get_ the size of the send buffer that the operating system uses for the socket.
      \sa libuv documentation: [`uv_send_buffer_size()`](http://docs.libuv.org/en/v1.x/handle.html#c.uv_send_buffer_size). */
  unsigned int send_buffer_size() const noexcept
  {
    unsigned int v = 0;
    ::uv_send_buffer_size(static_cast< uv_t* >(uv_handle), (int*)&v);
    return v;
  }
  /*! \details _Set_ the size of the send buffer that the operating system uses for the socket.
      \sa libuv documentation: [`uv_send_buffer_size()`](http://docs.libuv.org/en/v1.x/handle.html#c.uv_send_buffer_size). */
  void send_buffer_size(const unsigned int _v) noexcept  { ::uv_send_buffer_size(static_cast< uv_t* >(uv_handle), (int*)&_v); }

  /*! \details _Get_ the size of the receive buffer that the operating system uses for the socket.
      \sa libuv documentation: [`uv_recv_buffer_size()`](http://docs.libuv.org/en/v1.x/handle.html#c.uv_recv_buffer_size). */
  unsigned int recv_buffer_size() const noexcept
  {
    unsigned int v = 0;
    ::uv_recv_buffer_size(static_cast< uv_t* >(uv_handle), (int*)&v);
    return v;
  }
  /*! \details _Set_ the size of the receive buffer that the operating system uses for the socket.
      \sa libuv documentation: [`uv_recv_buffer_size()`](http://docs.libuv.org/en/v1.x/handle.html#c.uv_recv_buffer_size). */
  void recv_buffer_size(const unsigned int _v) noexcept  { ::uv_recv_buffer_size(static_cast< uv_t* >(uv_handle), (int*)&_v); }

  /*! \details Get the platform dependent file descriptor.
      \sa libuv documentation: [`uv_fileno()`](http://docs.libuv.org/en/v1.x/handle.html#c.uv_fileno). */
  ::uv_os_fd_t fileno() const noexcept
  {
#ifdef _WIN32
    ::uv_os_fd_t fd = INVALID_HANDLE_VALUE;
#else
    ::uv_os_fd_t fd = -1;
#endif
    ::uv_fileno(*this, &fd);
    return fd;
  }

public: /*conversion operators*/
  operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_handle); }
  operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_handle); }

  explicit operator bool() const noexcept  { return is_active(); }  /*!< \brief Equivalent to `is_active()`. */
};

template< typename _UV_T_ >
void handle::base< _UV_T_ >::close_cb(::uv_handle_t *_uv_handle)
{
  base *b = from(_uv_handle);
  on_destroy_t &f = b->on_destroy_storage.value();
  if (f)  f(_uv_handle->data);
  b->Delete(b);
}



/*! \brief Stream handle type.
    \sa libuv documentation: [`uv_stream_t`](http://docs.libuv.org/en/v1.x/stream.html#uv-stream-t-stream-handle). */
class stream : public handle
{
  friend class connect;
  friend class write;
  friend class shutdown;

public: /*types*/
  using uv_t = ::uv_stream_t;

private: /*types*/
  using base = handle::base< uv_t >;

private: /*constructors*/
  explicit stream(uv_t *_uv_handle)
  {
    base::from(_uv_handle)->ref();
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

public: /*interface*/
  /*! \brief The amount of queued bytes waiting to be sent. */
  std::size_t write_queue_size() const noexcept  { return static_cast< uv_t* >(uv_handle)->write_queue_size; }
  /*! \brief Check if the stream is readable. */
  bool is_readable() const noexcept  { return ::uv_is_readable(static_cast< uv_t* >(uv_handle)); }
  /*! \brief Check if the stream is writable. */
  bool is_writable() const noexcept  { return ::uv_is_writable(static_cast< uv_t* >(uv_handle)); }
  /*! \details Enable or disable blocking mode for the stream.
      \sa libuv documentation: [`uv_stream_set_blocking()`](http://docs.libuv.org/en/v1.x/stream.html#c.uv_stream_set_blocking). */
  int set_blocking(bool _enable) noexcept  { return ::uv_stream_set_blocking(static_cast< uv_t* >(uv_handle), _enable); }

public: /*conversion operators*/
  operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_handle); }
  operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_handle); }
};


/*! \brief TCP handle type.
    \sa libuv documentation: [`uv_tcp_t`](http://docs.libuv.org/en/v1.x/tcp.html#uv-tcp-t-tcp-handle). */
class tcp : public stream
{
  friend class connect;

public: /*types*/
  using uv_t = ::uv_tcp_t;

private: /*types*/
  using base = handle::base< uv_t >;

private: /*constructors*/
  explicit tcp(uv_t *_uv_handle)
  {
    base::from(_uv_handle)->ref();
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
    uv_handle = base::create();
    ::uv_tcp_init_ex(_loop, static_cast< uv_t* >(uv_handle), _flags);
  }
  /*! \details Create a socket object from an existing OS native socket descriptor.
      \sa libuv documentation: [`uv_tcp_open()`](http://docs.libuv.org/en/v1.x/tcp.html#c.uv_tcp_open),
                               [`uv_tcp_init()`](http://docs.libuv.org/en/v1.x/tcp.html#c.uv_tcp_init). */
  tcp(uv::loop _loop, ::uv_os_sock_t _sock)
  {
    uv_handle = base::create();
    ::uv_tcp_init(_loop, static_cast< uv_t* >(uv_handle));
    ::uv_tcp_open(static_cast< uv_t* >(uv_handle), _sock);
  }

public: /*intreface*/
  /*! \brief Get the platform dependent socket descriptor. The alias for `handle::fileno()`. */
  ::uv_os_sock_t socket() const noexcept  { return (::uv_os_sock_t)fileno(); }

  /*! \brief Enable or disable Nagle’s algorithm on the socket. */
  int nodelay(bool _enable) noexcept  { return ::uv_tcp_nodelay(static_cast< uv_t* >(uv_handle), _enable); }
  /*! \details Enable or disable TCP keep-alive.
      \arg `_delay` is the initial delay in seconds, ignored when `_enable = false`. */
  int keepalive(bool _enable, unsigned int _delay) noexcept  { return ::uv_tcp_keepalive(static_cast< uv_t* >(uv_handle), _enable, _delay); }
  /*! \details Enable or disable simultaneous asynchronous accept requests that are queued by the operating system when listening for new TCP connections.
      \sa libuv documentation: [`uv_tcp_simultaneous_accepts()`](http://docs.libuv.org/en/v1.x/tcp.html#c.uv_tcp_simultaneous_accepts). */
  int simultaneous_accepts(bool _enable) noexcept  { return ::uv_tcp_simultaneous_accepts(static_cast< uv_t* >(uv_handle), _enable); }
  /*! \details Bind the handle to an address and port.
      \sa libuv documentation: [`uv_tcp_bind()`](http://docs.libuv.org/en/v1.x/tcp.html#c.uv_tcp_bind). */
  template< typename _T_, typename = std::enable_if_t< is_one_of< _T_, ::sockaddr_in, ::sockaddr_in6, ::sockaddr_storage >::value > >
  int bind(const _T_ &_sa, unsigned int _flags) noexcept  { return ::uv_tcp_bind(static_cast< uv_t* >(uv_handle), reinterpret_cast< const ::sockaddr* >(&_sa), _flags); }
  /*! \details Get the address which this handle is bound to.
      \sa libuv documentation: [`uv_tcp_getsockname()`](http://docs.libuv.org/en/v1.x/tcp.html#c.uv_tcp_getsockname). */
  template< typename _T_, typename = std::enable_if_t< is_one_of< _T_, ::sockaddr_in, ::sockaddr_in6, ::sockaddr_storage >::value > >
  int getsockname(_T_ &_sa) const noexcept
  {
    int z = sizeof(_T_);
    return ::uv_tcp_getsockname(static_cast< uv_t* >(uv_handle), reinterpret_cast< ::sockaddr* >(&_sa), &z);
  }
  /*! \details Get the address of the remote peer connected to this handle.
      \sa libuv documentation: [`uv_tcp_getpeername()`](http://docs.libuv.org/en/v1.x/tcp.html#c.uv_tcp_getpeername). */
  template< typename _T_, typename = std::enable_if_t< is_one_of< _T_, ::sockaddr_in, ::sockaddr_in6, ::sockaddr_storage >::value > >
  int getpeername(_T_ &_sa) const noexcept
  {
    int z = sizeof(_T_);
    return ::uv_tcp_getpeername(static_cast< uv_t* >(uv_handle), reinterpret_cast< ::sockaddr* >(&_sa), &z);
  }

public: /*conversion operators*/
  operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_handle); }
  operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_handle); }
};



/*! \brief Pipe handle type.
    \sa libuv documentation: [`uv_pipe_t`](http://docs.libuv.org/en/v1.x/pipe.html#uv-pipe-t-pipe-handle). */
class pipe : public stream
{
  friend class connect;
  friend class write;

public: /*types*/
  using uv_t = ::uv_pipe_t;

private: /*types*/
  using base = handle::base< uv_t >;

private: /*constructors*/
  explicit pipe(uv_t *_uv_handle)
  {
    base::from(_uv_handle)->ref();
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
    uv_handle = base::create();
    ::uv_pipe_init(_loop, static_cast< uv_t* >(uv_handle), _ipc);
    ::uv_pipe_bind(static_cast< uv_t* >(uv_handle), _name);
  }
  /*! \details Create a pipe object from an existing OS native pipe descriptor.
      \sa libuv documentation: [`uv_pipe_open()`](http://docs.libuv.org/en/v1.x/pipe.html#c.uv_pipe_open). */
  pipe(uv::loop _loop, ::uv_file _fd, bool _ipc = false)
  {
    uv_handle = base::create();
    ::uv_pipe_init(_loop, static_cast< uv_t* >(uv_handle), _ipc);
    ::uv_pipe_open(static_cast< uv_t* >(uv_handle), _fd);
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
    int o = 0;
    while ( (o = ::uv_pipe_getsockname(static_cast< uv_t* >(uv_handle), const_cast< char* >(name.c_str()), &len)) != 0 and o == UV_ENOBUFS )
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
    int o = 0;
    while ( (o = ::uv_pipe_getpeername(static_cast< uv_t* >(uv_handle), const_cast< char* >(name.c_str()), &len)) != 0 and o == UV_ENOBUFS )
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
  operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_handle); }
  operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_handle); }
};


class udp : public handle
{
public: /*types*/
  using uv_t = ::uv_udp_t;

private: /*types*/
  using base = handle::base< uv_t >;

public: /*constructors*/
  ~udp() = default;

  udp(const udp&) = default;
  udp& operator =(const udp&) = default;

  udp(udp&&) noexcept = default;
  udp& operator =(udp&&) noexcept = default;

public: /*intreface*/
  ::uv_os_sock_t socket() const noexcept  { return (::uv_os_sock_t)fileno(); }

public: /*conversion operators*/
  operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_handle); }
  operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_handle); }
};


//! \}
}


namespace std
{

//! \ingroup g__handle
template<> inline void swap(uv::handle &_this, uv::handle &_that) noexcept  { _this.swap(_that); }

}


#endif
