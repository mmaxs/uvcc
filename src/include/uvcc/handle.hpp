
#ifndef UVCC_HANDLE__HPP
#define UVCC_HANDLE__HPP

#include "uvcc/utility.hpp"

#include <uv.h>
#include <cstddef>      // offsetof
#include <functional>   // function
#include <type_traits>  // is_standard_layout
#include <utility>      // swap() move()
#include <memory>       // addressof()


namespace uv
{
/*! \defgroup g__handle Handles
    \brief The classes representing libuv handles. */
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
    in the base class, so there are no any virtual member functions. */
class handle
{
  friend class request;

public: /*types*/
  using uv_t = ::uv_handle_t;
  using on_destroy_t = std::function< void(void*) >;

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

    void destroy()
    {
      uv_t *uv_h = reinterpret_cast< uv_t* >(&uv_handle);
      if (::uv_is_active(uv_h))
        ::uv_close(uv_h, destroy_cb);
      else
      {
        ::uv_close(uv_h, nullptr);
        destroy_cb(uv_h);
      }
    }

    static void destroy_cb(uv_t *_uv_h)
    {
      base *b = from(_uv_h);
      on_destroy_t &f = b->on_destroy_storage.value();
      if (f)  f(_uv_h->data);
      b->Delete(b);
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
  };
  //! \endcond

protected: /*data*/
  void *uv_handle;

protected: /*constructors*/
  handle() noexcept : uv_handle(nullptr)  {}

public: /*constructors*/
  ~handle()  { if (uv_handle)  base< uv_t >::from(uv_handle)->unref(); }

  handle(const handle &_h)
  {
    base< uv_t >::from(_h.uv_handle)->ref();
    uv_handle = _h.uv_handle; 
  }
  handle& operator =(const handle &_h)
  {
    if (this != &_h)
    {
      base< uv_t >::from(_h.uv_handle)->ref();
      void *uv_h = uv_handle;
      uv_handle = _h.uv_handle; 
      base< uv_t >::from(uv_h)->unref();
    };
    return *this;
  }

  handle(handle &&_h) noexcept : uv_handle(_h.uv_handle)  { _h.uv_handle = nullptr; }
  handle& operator =(handle &&_h) noexcept
  {
    if (this != &_h)
    {
      void *uv_h = uv_handle;
      uv_handle = _h.uv_handle;
      _h.uv_handle = nullptr;
      base< uv_t >::from(uv_h)->unref();
    };
    return *this;
  }

public: /*interface*/
  void swap(handle &_h) noexcept  { std::swap(uv_handle, _h.uv_handle); }

  const on_destroy_t& on_destroy() const noexcept  { return base< uv_t >::from(uv_handle)->on_destroy(); }
        on_destroy_t& on_destroy()       noexcept  { return base< uv_t >::from(uv_handle)->on_destroy(); }

  ::uv_handle_type type() const noexcept  { return static_cast< uv_t* >(uv_handle)->type; }
  ::uv_loop_t* loop() const noexcept  { return static_cast< uv_t* >(uv_handle)->loop; }

  void* data() const noexcept  { return static_cast< uv_t* >(uv_handle)->data; }
  void data(void *_data) noexcept { static_cast< uv_t* >(uv_handle)->data = _data; }

  int is_active() const noexcept  { return ::uv_is_active(*this); }
  int is_closing() const noexcept { return ::uv_is_closing(*this); }

  unsigned int send_buffer_size() const noexcept
  {
    unsigned int v = 0;
    ::uv_send_buffer_size(static_cast< uv_t* >(uv_handle), (int*)&v);
    return v;
  }
  void send_buffer_size(const unsigned int _v) noexcept  { ::uv_send_buffer_size(static_cast< uv_t* >(uv_handle), (int*)&_v); }

  unsigned int recv_buffer_size() const noexcept
  {
    unsigned int v = 0;
    ::uv_recv_buffer_size(static_cast< uv_t* >(uv_handle), (int*)&v);
    return v;
  }
  void recv_buffer_size(const unsigned int _v) noexcept  { ::uv_recv_buffer_size(static_cast< uv_t* >(uv_handle), (int*)&_v); }

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

  explicit operator bool() const noexcept  { return is_active(); }
};




class stream : public handle
{
  friend class write;

public: /*types*/
  using uv_t = ::uv_stream_t;

private: /*constructors*/
  explicit stream(stream::uv_t *_uv_h)
  {
    base< uv_t >::from(_uv_h)->ref();
    uv_handle = _uv_h;
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
  bool is_readable()  { return ::uv_is_readable(static_cast< uv_t* >(uv_handle)); }
  bool is_writable()  { return ::uv_is_writable(static_cast< uv_t* >(uv_handle)); }
  int set_blocking(bool _b)  { return ::uv_stream_set_blocking(static_cast< uv_t* >(uv_handle), _b); }

public: /*conversion operators*/
  operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_handle); }
  operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_handle); }
};


class tcp : public stream
{
  friend class connect;

public: /*types*/
  using uv_t = ::uv_tcp_t;

private: /*constructors*/
  explicit tcp(stream::uv_t *_uv_h)
  {
    base< uv_t >::from(_uv_h)->ref();
    uv_handle = _uv_h;
  }

public: /*constructors*/
  ~tcp() = default;

  tcp(const tcp&) = default;
  tcp& operator =(const tcp&) = default;

  tcp(tcp&&) noexcept = default;
  tcp& operator =(tcp&&) noexcept = default;

  tcp(::uv_loop_t *_loop)
  {
    uv_handle = base< uv_t >::create();
    ::uv_tcp_init_ex(_loop, static_cast< uv_t* >(uv_handle), AF_INET);
  }

public: /*intreface*/
  ::uv_os_sock_t socket() const noexcept  { return (::uv_os_sock_t)fileno(); }

public: /*conversion operators*/
  operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_handle); }
  operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_handle); }
};


class pipe : public stream
{
public: /*types*/
  using uv_t = ::uv_pipe_t;

public: /*constructors*/
  ~pipe() = default;

  pipe(const pipe&) = default;
  pipe& operator =(const pipe&) = default;

  pipe(pipe&&) noexcept = default;
  pipe& operator =(pipe&&) noexcept = default;

public: /*intreface*/

public: /*conversion operators*/
  operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_handle); }
  operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_handle); }
};


class udp : public handle
{
public: /*types*/
  using uv_t = ::uv_udp_t;

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
template<> void swap(uv::handle &_this, uv::handle &_that) noexcept  { _this.swap(_that); }

}


#endif
