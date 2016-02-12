
#ifndef UVCC_REQUEST__HPP
#define UVCC_REQUEST__HPP

#include "uvcc/utility.hpp"
#include "uvcc/buffer.hpp"
#include "uvcc/thread.hpp"
#include "uvcc/handle.hpp"

#include <uv.h>
#include <cstddef>      // offsetof
#include <cstdlib>      // malloc() free()
#include <memory>       // shared_ptr
#include <functional>   // function
#include <utility>      // move()
#include <type_traits>  // aligned_storage is_standard_layout
#include <vector>       // vector
#include <mutex>        // lock_guard adopt_lock


namespace uv
{
/*! \defgroup __request Requests
    \brief The classes represnting libuv requests. */
//! \{

/*! \defgroup __request_traits uv_req_traits< typename >
    \brief Defines the correspondence between libuv request data types and C++ classes representing them. */
//! \{

/* libuv requests */
class connect;
#define ON_CONNECT_T std::function< void(connect, int) >

class write;
#define ON_WRITE_T std::function< void(write, int) >

class shutdown;
#define ON_SHUTDOWN_T std::function< void(shutdown, int) >

class udp_send;
#define ON_UDP_SEND_T std::function< void(int) >

class fs;
#define ON_FS_T std::function< void(int) >

class work;
#define ON_WORK_T std::function< void(int) >

class getaddrinfo;
#define ON_GETADDRINFO_T std::function< void(int) >

class getnameinfo;
#define ON_GETNAMEINFO_T std::function< void(int) >

class request;
#define req request  // redefine UV_REQ_TYPE_MAP() entry
#define ON_REQ_T std::function< void() >  // a dummy declaration for UV_REQ_TYPE_MAP() being able to be used

//! \cond
/*! See \ref __request_traits */
template< typename _UV_T_ > struct uv_req_traits  {};
//! \endcond
#define XX(X, x) template<> struct uv_req_traits< uv_##x##_t >\
{\
  using type = x;\
  using on_request_t = ON_##X##_T;\
};
UV_REQ_TYPE_MAP(XX)
#undef XX

#undef req
#undef ON_CONNECT_T
#undef ON_WRITE_T
#undef ON_SHUTDOWN_T
#undef ON_UDP_SEND_T
#undef ON_FS_T
#undef ON_WORK_T
#undef ON_GETADDRINFO_T
#undef ON_GETNAMEINFO_T
//! \}


/*! \brief The base class for the libuv's requests.
    \details Derived classes conceptually are just interfaces to the data stored
    in the base class, so there are no any virtual member functions. */
class request
{
public: /*types*/
  using uv_t = ::uv_req_t;
  using on_destroy_t = std::function< void(void*) >;

protected: /*types*/
  //! \cond
  template< typename _UV_T_ > class base
  {
  private: /*types*/
    using on_request_t = typename uv_req_traits< _UV_T_ >::on_request_t;
    using on_request_storage_t = union_storage<
#define XX(X, x) uv_req_traits< uv_##x##_t >::on_request_t,
        UV_REQ_TYPE_MAP(XX)
#undef XX
        on_request_t
    >;

  private: /*data*/
    void (*Delete)(void*);  // store a proper delete operator
    ref_count count;
    type_storage< on_destroy_t > on_destroy_storage;
    mutex busy;
    on_request_storage_t on_request_storage;
    alignas(::uv_any_req) _UV_T_ uv_req;

  private: /*constructors*/
    base() : Delete(default_delete< base >::Delete)
    {
      on_request_storage.template reset< on_request_t >();
    }

  public: /*constructors*/
    ~base() = default;

    base(const base&) = delete;
    base& operator =(const base&) = delete;

    base(base&&) = delete;
    base& operator =(base&&) = delete;

  private: /*functions*/
    void destroy()
    {
      on_destroy_t &f = on_destroy_storage.value();
      if (f)  f(uv_req.data);
      Delete(this);
    }

  public: /*interface*/
    static void* create()  { return std::addressof((new base())->uv_req); }

    constexpr static base* from(void *_uv_req) noexcept
    {
      static_assert(std::is_standard_layout< base >::value, "not a standard layout type");
      return reinterpret_cast< base* >(static_cast< char* >(_uv_req) - offsetof(base, uv_req));
    }

    on_destroy_t& on_destroy() noexcept  { return on_destroy_storage.value(); }
    on_request_t& on_request() noexcept  { return on_request_storage.template value< on_request_t >(); }

    void ref()  { count.inc(); }
    void unref()  { if (count.dec() == 0)  destroy(); }

    void lock() noexcept  { busy.lock(); }
    bool try_lock() noexcept { return busy.try_lock(); }
    void unlock() noexcept   { busy.unlock(); }
  };
  //! \endcond

protected: /*data*/
  void *uv_req;

protected: /*constructors*/
  request() noexcept : uv_req(nullptr)  {}

public: /*constructors*/
  ~request()  { if (uv_req)  base< uv_t >::from(uv_req)->unref(); }

  request(const request &_r)
  {
    base< uv_t >::from(_r.uv_req)->ref();
    uv_req = _r.uv_req; 
  }
  request& operator =(const request &_r)
  {
    if (this != &_r)
    {
      base< uv_t >::from(_r.uv_req)->ref();
      void *uv_r = uv_req;
      uv_req = _r.uv_req; 
      base< uv_t >::from(uv_r)->unref();
    };
    return *this;
  }

  request(request &&_r) noexcept : uv_req(_r.uv_req)  { _r.uv_req = nullptr; }
  request& operator =(request &&_r) noexcept
  {
    if (this != &_r)
    {
      void *uv_r = uv_req;
      uv_req = _r.uv_req;
      _r.uv_req = nullptr;
      base< uv_t >::from(uv_r)->unref();
    };
    return *this;
  }

public: /*interface*/
  void swap(request &_r) noexcept  { std::swap(uv_req, _r.uv_req); }

  const on_destroy_t& on_destroy() const noexcept  { return base< uv_t >::from(uv_req)->on_destroy(); }
        on_destroy_t& on_destroy()       noexcept  { return base< uv_t >::from(uv_req)->on_destroy(); }

  ::uv_req_type type() const noexcept  { return static_cast< uv_t* >(uv_req)->type; }

  void* const& user_data() const noexcept  { return static_cast< uv_t* >(uv_req)->data; }
  void*      & user_data()       noexcept  { return static_cast< uv_t* >(uv_req)->data; }

  int cancel() noexcept  { return ::uv_cancel(static_cast< uv_t* >(uv_req)); }

public: /*conversion operators*/
  operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_req); }
  operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_req); }
};




class connect : public request
{
public: /*types*/
  using uv_t = ::uv_connect_t;
  using on_request_t = typename uv_req_traits< uv_t >::on_request_t;

private: /*constructors*/
  explicit connect(uv_t *_uv_r)
  {
    base< uv_t >::from(_uv_r)->ref();
    uv_req = _uv_r;
  }

public: /*constructors*/
  ~connect() = default;
  connect()  { uv_req = base< uv_t >::create(); }

  connect(const connect&) = default;
  connect& operator =(const connect&) = default;

  connect(connect&&) noexcept = default;
  connect& operator =(connect&&) noexcept = default;

private: /*functions*/
  static void run_cb(uv_t *_uv_r, int _status)
  {
    using handle_base = handle::base< tcp::uv_t >;
    ref_guard< handle_base > unref_handle(*handle_base::from(_uv_r->handle), adopt_ref);
    ref_guard< base< uv_t > > unref(*base< uv_t >::from(_uv_r), adopt_ref);

    on_request_t &f = base< uv_t >::from(_uv_r)->on_request();
    if (f)  f(connect(_uv_r), _status);
  }
  static void run_protected_cb(uv_t *_uv_r, int _status)
  {
    using handle_base = handle::base< tcp::uv_t >;
    ref_guard< handle_base > unref_handle(*handle_base::from(_uv_r->handle), adopt_ref);
    ref_guard< base< uv_t > > unref(*base< uv_t >::from(_uv_r), adopt_ref);
    std::lock_guard< base< uv_t > > unprotect(*base< uv_t >::from(_uv_r), std::adopt_lock);

    on_request_t &f = base< uv_t >::from(_uv_r)->on_request();
    if (f)  f(connect(_uv_r), _status);
  }

public: /*interface*/
  const on_request_t& on_request() const noexcept  { return base< uv_t >::from(uv_req)->on_request(); }
        on_request_t& on_request()       noexcept  { return base< uv_t >::from(uv_req)->on_request(); }

  tcp handle() const  { return tcp(static_cast< uv_t* >(uv_req)->handle); }

  int run(tcp _tcp, const ::sockaddr *_sa)
  {
    handle::base< tcp::uv_t >::from(_tcp.uv_handle)->ref();
    base< uv_t >::from(uv_req)->ref();
    return ::uv_tcp_connect(static_cast< uv_t* >(uv_req), _tcp, _sa, run_cb);
  }
  int run_protected(tcp _tcp, const ::sockaddr *_sa)
  {
    handle::base< tcp::uv_t >::from(_tcp.uv_handle)->ref();
    base< uv_t >::from(uv_req)->ref();
    base< uv_t >::from(uv_req)->lock();
    return ::uv_tcp_connect(static_cast< uv_t* >(uv_req), _tcp, _sa, run_protected_cb);
  }
  int try_run_protected(tcp _tcp, const ::sockaddr *_sa)
  {
    int o = base< uv_t >::from(uv_req)->try_lock();
    if (o != 0)  return o;
    handle::base< tcp::uv_t >::from(_tcp.uv_handle)->ref();
    base< uv_t >::from(uv_req)->ref();
    return ::uv_tcp_connect(static_cast< uv_t* >(uv_req), _tcp, _sa, run_protected_cb);
  }

public: /*conversion operators*/
  operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_req); }
  operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_req); }
};


class write : public request
{
public: /*types*/
  using uv_t = ::uv_write_t;
  using on_request_t = typename uv_req_traits< uv_t >::on_request_t;

private: /*constructors*/
  explicit write(uv_t *_uv_r)
  {
    base< uv_t >::from(_uv_r)->ref();
    uv_req = _uv_r;
  }

public: /*constructors*/
  ~write() = default;
  write()  { uv_req = base< uv_t >::create(); }

  write(const write&) = default;
  write& operator =(const write&) = default;

  write(write&&) noexcept = default;
  write& operator =(write&&) noexcept = default;

private: /*functions*/
  static void run_cb(uv_t *_uv_r, int _status)
  {
    using handle_base = handle::base< stream::uv_t >;
    ref_guard< handle_base > unref_handle(*handle_base::from(_uv_r->handle), adopt_ref);
    ref_guard< base< uv_t > > unref(*base< uv_t >::from(_uv_r), adopt_ref);

    on_request_t &f = base< uv_t >::from(_uv_r)->on_request();
    if (f)  f(write(_uv_r), _status);
  }
  static void run_protected_cb(uv_t *_uv_r, int _status)
  {
    using handle_base = handle::base< stream::uv_t >;
    ref_guard< handle_base > unref_handle(*handle_base::from(_uv_r->handle), adopt_ref);
    ref_guard< base< uv_t > > unref(*base< uv_t >::from(_uv_r), adopt_ref);
    std::lock_guard< base< uv_t > > unprotect(*base< uv_t >::from(_uv_r), std::adopt_lock);

    on_request_t &f = base< uv_t >::from(_uv_r)->on_request();
    if (f)  f(write(_uv_r), _status);
  }

public: /*interface*/
  const on_request_t& on_request() const noexcept  { return base< uv_t >::from(uv_req)->on_request(); }
        on_request_t& on_request()       noexcept  { return base< uv_t >::from(uv_req)->on_request(); }

  stream handle() const  { return stream(static_cast< uv_t* >(uv_req)->handle); }
  stream send_handle() const  { return stream(static_cast< uv_t* >(uv_req)->send_handle); }

  int run(stream _stream, const std::vector< buffer > &_bufs)
  {
    handle::base< stream::uv_t >::from(_stream.uv_handle)->ref();
    base< uv_t >::from(uv_req)->ref();
    //return ::uv_write(static_cast< uv_t* >(uv_req), _stream, _bufs.data(), _bufs.size(), run_cb);
  }
  int run(pipe _pipe, stream _send_handle)
  {
  }
  int try_run(stream _stream, const std::vector< buffer > &_bufs)
  {
  }
  int run_protected(stream _stream, const std::vector< buffer > &_bufs)
  {
    handle::base< stream::uv_t >::from(_stream.uv_handle)->ref();
    base< uv_t >::from(uv_req)->ref();
    base< uv_t >::from(uv_req)->lock();
    //return ::uv_write(static_cast< uv_t* >(uv_req), _stream, _bufs.data(), _bufs.size(), run_protected_cb);
  }
  int run_protected(pipe _pipe, stream _send_handle)
  {
  }
  int try_run_protected(stream _stream, const std::vector< buffer > &_bufs)
  {
  }
  int try_run_protected(pipe _pipe, stream _send_handle)
  {
  }

public: /*conversion operators*/
  operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_req); }
  operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_req); }
};


class shutdown : public request
{
public: /*types*/
  using uv_t = ::uv_shutdown_t;
  using on_request_t = typename uv_req_traits< uv_t >::on_request_t;

public: /*constructors*/
  ~shutdown() = default;

  shutdown(const shutdown&) = default;
  shutdown& operator =(const shutdown&) = default;

  shutdown(shutdown&&) noexcept = default;
  shutdown& operator =(shutdown&&) noexcept = default;

public: /*conversion operators*/
  operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_req); }
  operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_req); }
};


class udp_send : public request
{
public: /*types*/
  using uv_t = ::uv_udp_send_t;
  using on_request_t = typename uv_req_traits< uv_t >::on_request_t;

public: /*constructors*/
  ~udp_send() = default;

  udp_send(const udp_send&) = default;
  udp_send& operator =(const udp_send&) = default;

  udp_send(udp_send&&) noexcept = default;
  udp_send& operator =(udp_send&&) noexcept = default;

public: /*conversion operators*/
  operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_req); }
  operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_req); }
};


class fs : public request
{
public: /*types*/
  using uv_t = ::uv_fs_t;
  using on_request_t = typename uv_req_traits< uv_t >::on_request_t;

public: /*constructors*/
  ~fs() = default;

  fs(const fs&) = default;
  fs& operator =(const fs&) = default;

  fs(fs&&) noexcept = default;
  fs& operator =(fs&&) noexcept = default;

public: /*conversion operators*/
  operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_req); }
  operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_req); }
};


class work : public request
{
public: /*types*/
  using uv_t = ::uv_work_t;
  using on_request_t = typename uv_req_traits< uv_t >::on_request_t;

public: /*constructors*/
  ~work() = default;

  work(const work&) = default;
  work& operator =(const work&) = default;

  work(work&&) noexcept = default;
  work& operator =(work&&) noexcept = default;

public: /*conversion operators*/
  operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_req); }
  operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_req); }
};


class getaddrinfo : public request
{
public: /*types*/
  using uv_t = ::uv_getaddrinfo_t;
  using on_request_t = typename uv_req_traits< uv_t >::on_request_t;

public: /*constructors*/
  ~getaddrinfo() = default;

  getaddrinfo(const getaddrinfo&) = default;
  getaddrinfo& operator =(const getaddrinfo&) = default;

  getaddrinfo(getaddrinfo&&) noexcept = default;
  getaddrinfo& operator =(getaddrinfo&&) noexcept = default;

public: /*conversion operators*/
  operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_req); }
  operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_req); }
};


class getnameinfo : public request
{
public: /*types*/
  using uv_t = ::uv_getnameinfo_t;
  using on_request_t = typename uv_req_traits< uv_t >::on_request_t;

public: /*constructors*/
  ~getnameinfo() = default;

  getnameinfo(const getnameinfo&) = default;
  getnameinfo& operator =(const getnameinfo&) = default;

  getnameinfo(getnameinfo&&) noexcept = default;
  getnameinfo& operator =(getnameinfo&&) noexcept = default;

public: /*conversion operators*/
  operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_req); }
  operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_req); }
};


//! \}
}


namespace std
{

/*! \brief \ingroup __request */
template<> void swap(uv::request &_this, uv::request &_that) noexcept  { _this.swap(_that); }

}


#endif
