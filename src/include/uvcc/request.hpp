
#ifndef UVCC_REQUEST__HPP
#define UVCC_REQUEST__HPP

#include "uvcc/utility.hpp"
#include "uvcc/buffer.hpp"
#include "uvcc/handle.hpp"

#include <uv.h>
#include <cstddef>      // offsetof
#include <cstdlib>      // malloc() free()
#include <memory>       // shared_ptr
#include <functional>   // function
#include <utility>      // move()
#include <type_traits>  // aligned_storage is_standard_layout conditional_t is_void
#include <mutex>        // lock_guard adopt_lock


namespace uv
{
/*! \defgroup g__request Requests
    \brief The classes representing libuv requests. */
//! \{

/*! \defgroup g__request_traits uv_req_traits< typename >
    \brief Defines signatures of request callback functions and the correspondence
    between libuv request data types and C++ classes representing them. */
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
#define req request  // redefine the UV_REQ_TYPE_MAP() entry
#define ON_REQ_T void  // a dummy declaration for UV_REQ_TYPE_MAP() being able to be used


//! \cond
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


/*! \brief The base class for the libuv requests.
    \details Derived classes conceptually are just interfaces to the data stored
    in the base class, so there are no any virtual member functions.
    \sa libuv documentation: [`uv_req_t`](http://docs.libuv.org/en/v1.x/request.html#uv-req-t-base-request). */
class request
{
public: /*types*/
  using uv_t = ::uv_req_t;
  using on_destroy_t = std::function< void(void*) >;
  /*!< \brief The function type of the callback called when the request object is about to be destroyed. */

protected: /*types*/
  //! \cond
  template< typename _UV_T_ > class base
  {
  private: /*types*/
    using on_request_t = typename uv_req_traits< _UV_T_ >::on_request_t;
    using on_request_storage_t = union_storage<
#define XX(X, x) std::conditional_t< std::is_void< uv_req_traits< uv_##x##_t >::on_request_t >::value, null_t, uv_req_traits< uv_##x##_t >::on_request_t >,
        UV_REQ_TYPE_MAP(XX)
#undef XX
        null_t
    >;

  private: /*data*/
    void (*Delete)(void*);  // store a proper delete operator
    ref_count rc;
    type_storage< on_destroy_t > on_destroy_storage;
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
    template< typename _ON_REQUEST_T_ = on_request_t >  // make it to be a separate template to prevent unconditional erroneous generating this member for invalid (void) on_request_t
    _ON_REQUEST_T_& on_request() noexcept  { return on_request_storage.template value< _ON_REQUEST_T_ >(); }

    void ref()  { rc.inc(); }
    void unref()  { if (rc.dec() == 0)  destroy(); }
    ref_count::type nrefs() const noexcept  { return rc.value(); }
  };
  //! \endcond

protected: /*data*/
  //! \cond
  void *uv_req;
  //! \endcond

protected: /*constructors*/
  request() noexcept : uv_req(nullptr)  {}

public: /*constructors*/
  ~request()  { if (uv_req)  base< uv_t >::from(uv_req)->unref(); }

  request(const request &_that)
  {
    base< uv_t >::from(_that.uv_req)->ref();
    uv_req = _that.uv_req;
  }
  request& operator =(const request &_that)
  {
    if (this != &_that)
    {
      base< uv_t >::from(_that.uv_req)->ref();
      auto t = uv_req;
      uv_req = _that.uv_req;
      base< uv_t >::from(t)->unref();
    };
    return *this;
  }

  request(request &&_that) noexcept : uv_req(_that.uv_req)  { _that.uv_req = nullptr; }
  request& operator =(request &&_that) noexcept
  {
    if (this != &_that)
    {
      auto t = uv_req;
      uv_req = _that.uv_req;
      _that.uv_req = nullptr;
      base< uv_t >::from(t)->unref();
    };
    return *this;
  }

public: /*interface*/
  void swap(request &_that) noexcept  { std::swap(uv_req, _that.uv_req); }
  /*! \brief The current number of existing references to the same object as this request variable refers to. */
  long nrefs() const noexcept  { return base< uv_t >::from(uv_req)->nrefs(); }

  const on_destroy_t& on_destroy() const noexcept  { return base< uv_t >::from(uv_req)->on_destroy(); }
        on_destroy_t& on_destroy()       noexcept  { return base< uv_t >::from(uv_req)->on_destroy(); }

  /*! \brief The tag indicating the libuv type of the request. */
  ::uv_req_type type() const noexcept  { return static_cast< uv_t* >(uv_req)->type; }

  /*! \brief The pointer to the user-defined arbitrary data. libuv and uvcc does not use this field. */
  void* const& data() const noexcept  { return static_cast< uv_t* >(uv_req)->data; }
  void*      & data()       noexcept  { return static_cast< uv_t* >(uv_req)->data; }

  /*! \details Cancel a pending request.
      \sa libuv documentation: [`uv_cancel()`](http://docs.libuv.org/en/v1.x/request.html#c.uv_cancel).*/
  int cancel() noexcept  { return ::uv_cancel(static_cast< uv_t* >(uv_req)); }

public: /*conversion operators*/
  operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_req); }
  operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_req); }
};



/*! \brief Connect request type. */
class connect : public request
{
public: /*types*/
  using uv_t = ::uv_connect_t;
  using on_request_t = typename uv_req_traits< uv_t >::on_request_t;
  /*!< \brief The function type of the callback called after a connection request is done.
       See \ref g__request_traits
       \sa libuv documentation: [`uv_connect_cb`](http://docs.libuv.org/en/v1.x/stream.html#c.uv_connect_cb). */

private: /*types*/
  using base = request::base< uv_t >;

private: /*constructors*/
  explicit connect(uv_t *_uv_req)
  {
    base::from(_uv_req)->ref();
    uv_req = _uv_req;
  }

public: /*constructors*/
  ~connect() = default;
  connect()  { uv_req = base::create(); }

  connect(const connect&) = default;
  connect& operator =(const connect&) = default;

  connect(connect&&) noexcept = default;
  connect& operator =(connect&&) noexcept = default;

private: /*functions*/
  template< typename = void > static void connect_cb(::uv_connect_t*, int);

public: /*interface*/
  const on_request_t& on_request() const noexcept  { return base::from(uv_req)->on_request(); }
        on_request_t& on_request()       noexcept  { return base::from(uv_req)->on_request(); }

  /*! \brief The `uv::tcp` stream which this connect request is running on. */
  tcp handle() const noexcept  { return tcp(static_cast< uv_t* >(uv_req)->handle); }

  /*! \brief Run the request.
      \sa libuv documentation: [`uv_tcp_connect()`](http://docs.libuv.org/en/v1.x/tcp.html#c.uv_tcp_connect). */
  template< typename _T_, typename = std::enable_if_t< is_one_of< _T_, ::sockaddr_in, ::sockaddr_in6, ::sockaddr_storage >::value > >
  int run(tcp _tcp, const _T_ &_sa)
  {
    tcp::base::from(_tcp.uv_handle)->ref();
    base::from(uv_req)->ref();
    return ::uv_tcp_connect(static_cast< uv_t* >(uv_req), _tcp, reinterpret_cast< const ::sockaddr* >(&_sa), connect_cb);
  }

public: /*conversion operators*/
  operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_req); }
  operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_req); }
};

template< typename >
void connect::connect_cb(::uv_connect_t *_uv_req, int _status)
{
  ref_guard< tcp::base > unref_handle(*tcp::base::from(_uv_req->handle), adopt_ref);
  ref_guard< base > unref_req(*base::from(_uv_req), adopt_ref);

  on_request_t &f = base::from(_uv_req)->on_request();
  if (f)  f(connect(_uv_req), _status);
}



/*! \brief Write request type. */
class write : public request
{
public: /*types*/
  using uv_t = ::uv_write_t;
  using on_request_t = typename uv_req_traits< uv_t >::on_request_t;
  /*!< \brief The function type of the callback called after data was written on a stream.
       See \ref g__request_traits
       \sa libuv documentation: [`uv_write_cb`](http://docs.libuv.org/en/v1.x/stream.html#c.uv_write_cb). */

private: /*types*/
  using base = request::base< uv_t >;

private: /*constructors*/
  explicit write(uv_t *_uv_req)
  {
    base::from(_uv_req)->ref();
    uv_req = _uv_req;
  }

public: /*constructors*/
  ~write() = default;
  write()  { uv_req = base::create(); }

  write(const write&) = default;
  write& operator =(const write&) = default;

  write(write&&) noexcept = default;
  write& operator =(write&&) noexcept = default;

private: /*functions*/
  template< typename = void > static void write_cb(::uv_write_t*, int);

public: /*interface*/
  const on_request_t& on_request() const noexcept  { return base::from(uv_req)->on_request(); }
        on_request_t& on_request()       noexcept  { return base::from(uv_req)->on_request(); }

  /*! \brief The stream which this write request is running on. */
  stream handle() const noexcept  { return stream(static_cast< uv_t* >(uv_req)->handle); }
  /*! \brief The handle of the stream being sent over a pipe using this write request. */
  stream send_handle() const noexcept  { return stream(static_cast< uv_t* >(uv_req)->send_handle); }

  /*! \brief Run the request.
      \sa libuv documentation: [`uv_write()`](http://docs.libuv.org/en/v1.x/stream.html#c.uv_write). */
  int run(stream _stream, const buffer _buf)
  {
    stream::base::from(_stream.uv_handle)->ref();
    base::from(uv_req)->ref();
    return ::uv_write(static_cast< uv_t* >(uv_req), _stream, _buf, _buf.count(), write_cb);
  }
  /*! \brief The overload for sending handles over a pipe.
      \sa libuv documentation: [`uv_write2()`](http://docs.libuv.org/en/v1.x/stream.html#c.uv_write2). */
  int run(pipe _pipe, const stream _send_handle)
  {
    return 0;
  }

  /*! \details The wrapper for corresponding libuv function.
      \note It tries to execute and complete immediately and does not call the request callback.
      \sa libuv documentation: [`uv_try_write()`](http://docs.libuv.org/en/v1.x/stream.html#c.uv_try_write). */
  int try_write(stream _stream, const buffer _buf)
  {
    return ::uv_try_write(_stream, _buf, _buf.count());
  }

public: /*conversion operators*/
  operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_req); }
  operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_req); }
};

template< typename >
void write::write_cb(::uv_write_t *_uv_req, int _status)
{
  ref_guard< stream::base > unref_handle(*stream::base::from(_uv_req->handle), adopt_ref);
  ref_guard< base > unref_req(*base::from(_uv_req), adopt_ref);

  on_request_t &f = base::from(_uv_req)->on_request();
  if (f)  f(write(_uv_req), _status);
}



/*! \brief Shutdown request type. */
class shutdown : public request
{
public: /*types*/
  using uv_t = ::uv_shutdown_t;
  using on_request_t = typename uv_req_traits< uv_t >::on_request_t;
  /*!< \brief The function type of the callback called after a shutdown request has been completed.
       See \ref g__request_traits
       \sa libuv documentation: [`uv_shutdown_cb`](http://docs.libuv.org/en/v1.x/stream.html#c.uv_shutdown_cb). */

private: /*types*/
  using base = request::base< uv_t >;

private: /*constructors*/
  explicit shutdown(uv_t *_uv_req)
  {
    base::from(_uv_req)->ref();
    uv_req = _uv_req;
  }

public: /*constructors*/
  ~shutdown() = default;
  shutdown()  { uv_req = base::create(); }

  shutdown(const shutdown&) = default;
  shutdown& operator =(const shutdown&) = default;

  shutdown(shutdown&&) noexcept = default;
  shutdown& operator =(shutdown&&) noexcept = default;

private: /*functions*/
  template< typename = void > static void shutdown_cb(::uv_shutdown_t*, int);

public: /*interface*/
  const on_request_t& on_request() const noexcept  { return base::from(uv_req)->on_request(); }
        on_request_t& on_request()       noexcept  { return base::from(uv_req)->on_request(); }

  /*! \brief The stream which this shutdown request is running on. */
  stream handle() const noexcept  { return stream(static_cast< uv_t* >(uv_req)->handle); }

  /*! \brief Run the request. */
  int run(stream _stream)
  {
    stream::base::from(_stream.uv_handle)->ref();
    base::from(uv_req)->ref();
    return ::uv_shutdown(static_cast< uv_t* >(uv_req), _stream, shutdown_cb);
  }

public: /*conversion operators*/
  operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_req); }
  operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_req); }
};

template< typename >
void shutdown::shutdown_cb(::uv_shutdown_t *_uv_req, int _status)
{
  ref_guard< stream::base > unref_handle(*stream::base::from(_uv_req->handle), adopt_ref);
  ref_guard< base > unref_req(*base::from(_uv_req), adopt_ref);

  on_request_t &f = base::from(_uv_req)->on_request();
  if (f)  f(shutdown(_uv_req), _status);
}



class udp_send : public request
{
public: /*types*/
  using uv_t = ::uv_udp_send_t;
  using on_request_t = typename uv_req_traits< uv_t >::on_request_t;

private: /*types*/
  using base = request::base< uv_t >;

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

private: /*types*/
  using base = request::base< uv_t >;

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

private: /*types*/
  using base = request::base< uv_t >;

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

private: /*types*/
  using base = request::base< uv_t >;

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

private: /*types*/
  using base = request::base< uv_t >;

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

//! \ingroup g__request
template<> inline void swap(uv::request &_this, uv::request &_that) noexcept  { _this.swap(_that); }

}


#endif
