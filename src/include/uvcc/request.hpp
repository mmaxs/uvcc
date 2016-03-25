
#ifndef UVCC_REQUEST__HPP
#define UVCC_REQUEST__HPP

#include "uvcc/utility.hpp"
#include "uvcc/handle.hpp"
#include "uvcc/buffer.hpp"

#include <uv.h>
#include <cstddef>      // offsetof
#include <memory>       // addressof()
#include <functional>   // function
#include <utility>      // swap() move()
#include <type_traits>  // aligned_storage is_standard_layout conditional_t is_void


namespace uv
{
/*! \defgroup g__request Requests
    \brief The classes representing libuv requests. */
//! \{


/* libuv requests */
class request;
class connect;
class write;
class shutdown;
class udp_send;
class fs;
class work;
class getaddrinfo;
class getnameinfo;



/*! \defgroup g__request_traits uv_req_traits< typename >
    \brief The correspondence between libuv request data types and C++ classes representing them. */
//! \{
#define BUGGY_DOXYGEN
#undef BUGGY_DOXYGEN
//! \cond
template< typename _UV_T_ > struct uv_req_traits  {};
//! \endcond
#define req request  // redefine the UV_REQ_TYPE_MAP() entry
#define XX(X, x) template<> struct uv_req_traits< uv_##x##_t >  { using type = x; };
UV_REQ_TYPE_MAP(XX)
#undef XX
#undef req
//! \}



/*! \brief The base class for the libuv requests.
    \details Derived classes conceptually are just interfaces to the data stored
    in the base class, so there are no any virtual member functions.
    \sa libuv documentation: [`uv_req_t`](http://docs.libuv.org/en/v1.x/request.html#uv-req-t-base-request). */
class request
{
public: /*types*/
  using uv_t = ::uv_req_t;
  using on_destroy_t = std::function< void(void *_data) >;
  /*!< \brief The function type of the callback called when the request object is about to be destroyed. */
  using on_request_t = void;

protected: /*types*/
  //! \cond
  using supplemental_data_t = struct {};

  template< class _REQUEST_ > class instance
  {
  private: /*types*/
    using uv_t = typename _REQUEST_::uv_t;
    using on_request_t = std::conditional_t< std::is_void< typename _REQUEST_::on_request_t >::value, null_t, typename _REQUEST_::on_request_t >;
    using supplemental_data_t = typename _REQUEST_::supplemental_data_t;

  private: /*data*/
    mutable int uv_error;
    void (*Delete)(void*);  // store a proper delete operator
    ref_count rc;
    type_storage< on_destroy_t > on_destroy_storage;
    alignas(32) type_storage< on_request_t > on_request_storage;
    static_assert(sizeof(on_request_storage) <= 32, "non-static layout structure");
    alignas(32) mutable type_storage< supplemental_data_t > supplemental_data_storage;
    static_assert(sizeof(supplemental_data_storage) <= 32, "non-static layout structure");
    alignas(32) uv_t uv_req;

  private: /*constructors*/
    instance() : uv_error(0), Delete(default_delete< instance >::Delete)  {}

  public: /*constructors*/
    ~instance() = default;

    instance(const instance&) = delete;
    instance& operator =(const instance&) = delete;

    instance(instance&&) = delete;
    instance& operator =(instance&&) = delete;

  private: /*functions*/
    void destroy()
    {
      on_destroy_t &destroy_cb = on_destroy_storage.value();
      if (destroy_cb)  destroy_cb(uv_req.data);
      Delete(this);
    }

  public: /*interface*/
    static void* create()  { return std::addressof((new instance())->uv_req); }

    constexpr static instance* from(void *_uv_req) noexcept
    {
      static_assert(std::is_standard_layout< instance >::value, "not a standard layout type");
      return reinterpret_cast< instance* >(static_cast< char* >(_uv_req) - offsetof(instance, uv_req));
    }

    on_destroy_t& on_destroy() noexcept  { return on_destroy_storage.value(); }
    on_request_t& on_request() noexcept  { return on_request_storage.value(); }
    supplemental_data_t& supplemental_data() const noexcept  { return supplemental_data_storage.value(); }

    void ref()  { rc.inc(); }
    void unref()  { if (rc.dec() == 0)  destroy(); }
    ref_count::type nrefs() const noexcept  { return rc.value(); }

    int& uv_status() const noexcept  { return uv_error; }
  };
  //! \endcond

protected: /*data*/
  //! \cond
  void *uv_req;
  //! \endcond

private: /*constructors*/
  explicit request(uv_t *_uv_req)
  {
    if (_uv_req)  instance< request >::from(_uv_req)->ref();
    uv_req = _uv_req;
  }

protected: /*constructors*/
  request() noexcept : uv_req(nullptr)  {}

public: /*constructors*/
  ~request()  { if (uv_req)  instance< request >::from(uv_req)->unref(); }

  request(const request &_that) : request(static_cast< uv_t* >(_that.uv_req))  {}
  request& operator =(const request &_that)
  {
    if (this != &_that)
    {
      if (_that.uv_req)  instance< request >::from(_that.uv_req)->ref();
      auto t = uv_req;
      uv_req = _that.uv_req;
      if (t)  instance< request >::from(t)->unref();
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
      if (t)  instance< request >::from(t)->unref();
    };
    return *this;
  }

protected: /*functions*/
  //! \cond
  int uv_status(int _value) const noexcept  { return (instance< request >::from(uv_req)->uv_status() = _value); }
  //! \endcond

public: /*interface*/
  void swap(request &_that) noexcept  { std::swap(uv_req, _that.uv_req); }
  /*! \brief The current number of existing references to the same object as this request variable refers to. */
  long nrefs() const noexcept  { return instance< request >::from(uv_req)->nrefs(); }
  /*! \brief The status value returned by the last executed libuv API function. */
  int uv_status() const noexcept  { return instance< request >::from(uv_req)->uv_status(); }

  const on_destroy_t& on_destroy() const noexcept  { return instance< request >::from(uv_req)->on_destroy(); }
        on_destroy_t& on_destroy()       noexcept  { return instance< request >::from(uv_req)->on_destroy(); }

  /*! \brief The tag indicating the libuv type of the request. */
  ::uv_req_type type() const noexcept  { return static_cast< uv_t* >(uv_req)->type; }

  /*! \brief The pointer to the user-defined arbitrary data. libuv and uvcc does not use this field. */
  void* const& data() const noexcept  { return static_cast< uv_t* >(uv_req)->data; }
  void*      & data()       noexcept  { return static_cast< uv_t* >(uv_req)->data; }

  /*! \details Cancel a pending request.
      \sa libuv documentation: [`uv_cancel()`](http://docs.libuv.org/en/v1.x/request.html#c.uv_cancel).*/
  int cancel() noexcept  { return ::uv_cancel(static_cast< uv_t* >(uv_req)); }

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_req); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_req); }

  explicit operator bool() const noexcept  { return (uv_status() >= 0); }  /*!< \brief Equivalent to `(uv_status() >= 0)`. */
};



/*! \brief Connect request type. */
class connect : public request
{
  //! \cond
  friend class request::instance< connect >;
  //! \endcond

public: /*types*/
  using uv_t = ::uv_connect_t;
  using on_request_t = std::function< void(connect) >;
  /*!< \brief The function type of the callback called after a connection request is done.
       \sa libuv documentation: [`uv_connect_cb`](http://docs.libuv.org/en/v1.x/stream.html#c.uv_connect_cb). */

private: /*types*/
  using instance = request::instance< connect >;

private: /*constructors*/
  explicit connect(uv_t *_uv_req)
  {
    if (_uv_req)  instance::from(_uv_req)->ref();
    uv_req = _uv_req;
  }

public: /*constructors*/
  ~connect() = default;
  connect()  { uv_req = instance::create(); }

  connect(const connect&) = default;
  connect& operator =(const connect&) = default;

  connect(connect&&) noexcept = default;
  connect& operator =(connect&&) noexcept = default;

private: /*functions*/
  template< typename = void > static void connect_cb(::uv_connect_t*, int);

public: /*interface*/
  const on_request_t& on_request() const noexcept  { return instance::from(uv_req)->on_request(); }
        on_request_t& on_request()       noexcept  { return instance::from(uv_req)->on_request(); }

  /*! \brief The stream (`uv::tcp` or `uv::pipe`) which this connect request is running on. */
  stream handle() const noexcept  { return stream(static_cast< uv_t* >(uv_req)->handle); }

  /*! \brief Run the request for `uv::tcp` stream.
      \sa libuv documentation: [`uv_tcp_connect()`](http://docs.libuv.org/en/v1.x/tcp.html#c.uv_tcp_connect). */
  template< typename _T_, typename = std::enable_if_t< is_one_of< _T_, ::sockaddr_in, ::sockaddr_in6, ::sockaddr_storage >::value > >
  int run(tcp _tcp, const _T_ &_sockaddr)
  {
    tcp::instance::from(_tcp.uv_handle)->ref();
    instance::from(uv_req)->ref();
    return uv_status(::uv_tcp_connect(static_cast< uv_t* >(uv_req), static_cast< tcp::uv_t* >(_tcp), reinterpret_cast< const ::sockaddr* >(&_sockaddr), connect_cb));
  }
  /*! \brief Run the request for `uv::pipe` stream.
      \sa libuv documentation: [`uv_pipe_connect()`](http://docs.libuv.org/en/v1.x/pipe.html#c.uv_pipe_connect). */
  void run(pipe _pipe, const char *_name)
  {
    pipe::instance::from(_pipe.uv_handle)->ref();
    instance::from(uv_req)->ref();
    ::uv_pipe_connect(static_cast< uv_t* >(uv_req), static_cast< pipe::uv_t* >(_pipe), _name, connect_cb);
  }

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_req); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_req); }
};

template< typename >
void connect::connect_cb(::uv_connect_t *_uv_req, int _status)
{
  ref_guard< stream::instance > unref_handle(*stream::instance::from(_uv_req->handle), adopt_ref);
  auto t = instance::from(_uv_req);
  ref_guard< instance > unref_req(*t, adopt_ref);

  t->uv_status() = _status;
  auto &connect_cb = t->on_request();
  if (connect_cb)  connect_cb(connect(_uv_req));
}



/*! \brief Write request type. */
class write : public request
{
  //! \cond
  friend class request::instance< write >;
  //! \endcond

public: /*types*/
  using uv_t = ::uv_write_t;
  using on_request_t = std::function< void(write) >;
  /*!< \brief The function type of the callback called after data was written on a stream.
       See \ref g__request_traits
       \sa libuv documentation: [`uv_write_cb`](http://docs.libuv.org/en/v1.x/stream.html#c.uv_write_cb). */

private: /*types*/
  using instance = request::instance< write >;

protected: /*types*/
  //! \cond
  using supplemental_data_t = buffer;
  //! \endcond

private: /*constructors*/
  explicit write(uv_t *_uv_req)
  {
    if (_uv_req)  instance::from(_uv_req)->ref();
    uv_req = _uv_req;
  }

public: /*constructors*/
  ~write() = default;
  write()  { uv_req = instance::create(); }

  write(const write&) = default;
  write& operator =(const write&) = default;

  write(write&&) noexcept = default;
  write& operator =(write&&) noexcept = default;

private: /*functions*/
  template< typename = void > static void write_cb(::uv_write_t*, int);
  template< typename = void > static void write2_cb(::uv_write_t*, int);

public: /*interface*/
  const on_request_t& on_request() const noexcept  { return instance::from(uv_req)->on_request(); }
        on_request_t& on_request()       noexcept  { return instance::from(uv_req)->on_request(); }

  /*! \brief The stream which this write request is running on. */
  stream handle() const noexcept  { return stream(static_cast< uv_t* >(uv_req)->handle); }
  /*! \brief The handle of the stream being sent over a pipe using this write request. */
  stream send_handle() const noexcept  { return stream(static_cast< uv_t* >(uv_req)->send_handle); }

  /*! \brief Run the request.
      \sa libuv documentation: [`uv_write()`](http://docs.libuv.org/en/v1.x/stream.html#c.uv_write). */
  int run(stream _stream, const buffer _buf)
  {
    stream::instance::from(_stream.uv_handle)->ref();
    instance::from(uv_req)->supplemental_data() = _buf;
    instance::from(uv_req)->ref();

    return uv_status(::uv_write(static_cast< uv_t* >(uv_req), static_cast< stream::uv_t* >(_stream), static_cast< const buffer::uv_t* >(_buf), _buf.count(), write_cb));
  }
  /*! \brief The overload for sending handles over a pipe.
      \sa libuv documentation: [`uv_write2()`](http://docs.libuv.org/en/v1.x/stream.html#c.uv_write2). */
  int run(pipe _pipe, const buffer _buf, stream _send_handle)
  {
    pipe::instance::from(_pipe.uv_handle)->ref();
    instance::from(uv_req)->supplemental_data() = _buf;
    stream::instance::from(_send_handle.uv_handle)->ref();
    instance::from(uv_req)->ref();

    return uv_status(::uv_write2(static_cast< uv_t* >(uv_req), static_cast< stream::uv_t* >(_pipe), static_cast< const buffer::uv_t* >(_buf), _buf.count(), static_cast< stream::uv_t* >(_send_handle), write2_cb));
  }

  /*! \details The wrapper for corresponding libuv function.
      \note It tries to execute and complete immediately and does not call the request callback.
      \sa libuv documentation: [`uv_try_write()`](http://docs.libuv.org/en/v1.x/stream.html#c.uv_try_write). */
  int try_write(stream _stream, const buffer _buf)
  {
    return uv_status(::uv_try_write(static_cast< stream::uv_t* >(_stream), static_cast< const buffer::uv_t* >(_buf), _buf.count()));
  }

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_req); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_req); }
};

template< typename >
void write::write_cb(::uv_write_t *_uv_req, int _status)
{
  buffer b(std::move(instance::from(_uv_req)->supplemental_data()));
  ref_guard< stream::instance > unref_handle(*stream::instance::from(_uv_req->handle), adopt_ref);
  auto t = instance::from(_uv_req);
  ref_guard< instance > unref_req(*t, adopt_ref);

  t->uv_status() = _status;
  auto &write_cb = t->on_request();
  if (write_cb)  write_cb(write(_uv_req));
}
template< typename >
void write::write2_cb(::uv_write_t *_uv_req, int _status)
{
  ref_guard< stream::instance > unref_send_handle(*stream::instance::from(_uv_req->send_handle), adopt_ref);
  write_cb(_uv_req, _status);
}


/*! \brief Shutdown request type. */
class shutdown : public request
{
  //! \cond
  friend class request::instance< shutdown >;
  //! \endcond

public: /*types*/
  using uv_t = ::uv_shutdown_t;
  using on_request_t = std::function< void(shutdown) >;
  /*!< \brief The function type of the callback called after a shutdown request has been completed.
       See \ref g__request_traits
       \sa libuv documentation: [`uv_shutdown_cb`](http://docs.libuv.org/en/v1.x/stream.html#c.uv_shutdown_cb). */

private: /*types*/
  using instance = request::instance< shutdown >;

private: /*constructors*/
  explicit shutdown(uv_t *_uv_req)
  {
    if (_uv_req)  instance::from(_uv_req)->ref();
    uv_req = _uv_req;
  }

public: /*constructors*/
  ~shutdown() = default;
  shutdown()  { uv_req = instance::create(); }

  shutdown(const shutdown&) = default;
  shutdown& operator =(const shutdown&) = default;

  shutdown(shutdown&&) noexcept = default;
  shutdown& operator =(shutdown&&) noexcept = default;

private: /*functions*/
  template< typename = void > static void shutdown_cb(::uv_shutdown_t*, int);

public: /*interface*/
  const on_request_t& on_request() const noexcept  { return instance::from(uv_req)->on_request(); }
        on_request_t& on_request()       noexcept  { return instance::from(uv_req)->on_request(); }

  /*! \brief The stream which this shutdown request is running on. */
  stream handle() const noexcept  { return stream(static_cast< uv_t* >(uv_req)->handle); }

  /*! \brief Run the request. */
  int run(stream _stream)
  {
    stream::instance::from(_stream.uv_handle)->ref();
    instance::from(uv_req)->ref();
    return uv_status(::uv_shutdown(static_cast< uv_t* >(uv_req), static_cast< stream::uv_t* >(_stream), shutdown_cb));
  }

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_req); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_req); }
};

template< typename >
void shutdown::shutdown_cb(::uv_shutdown_t *_uv_req, int _status)
{
  ref_guard< stream::instance > unref_handle(*stream::instance::from(_uv_req->handle), adopt_ref);
  auto t = instance::from(_uv_req);
  ref_guard< instance > unref_req(*t, adopt_ref);

  t->uv_status() = _status;
  auto &shutdown_cb = t->on_request();
  if (shutdown_cb)  shutdown_cb(shutdown(_uv_req));
}



class udp_send : public request
{
  //! \cond
  friend class request::instance< udp_send >;
  //! \endcond

public: /*types*/
  using uv_t = ::uv_udp_send_t;
  using on_request_t = std::function< void(int) >;

private: /*types*/
  using instance = request::instance< udp_send >;

public: /*constructors*/
  ~udp_send() = default;

  udp_send(const udp_send&) = default;
  udp_send& operator =(const udp_send&) = default;

  udp_send(udp_send&&) noexcept = default;
  udp_send& operator =(udp_send&&) noexcept = default;

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_req); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_req); }
};



class fs : public request
{
  //! \cond
  friend class request::instance< fs >;
  //! \endcond

public: /*types*/
  using uv_t = ::uv_fs_t;
  using on_request_t = std::function< void(int) >;

private: /*types*/
  using instance = request::instance< fs >;

public: /*constructors*/
  ~fs() = default;

  fs(const fs&) = default;
  fs& operator =(const fs&) = default;

  fs(fs&&) noexcept = default;
  fs& operator =(fs&&) noexcept = default;

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_req); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_req); }
};



class work : public request
{
  //! \cond
  friend class request::instance< work >;
  //! \endcond

public: /*types*/
  using uv_t = ::uv_work_t;
  using on_request_t = std::function< void(int) >;

private: /*types*/
  using instance = request::instance< work >;

public: /*constructors*/
  ~work() = default;

  work(const work&) = default;
  work& operator =(const work&) = default;

  work(work&&) noexcept = default;
  work& operator =(work&&) noexcept = default;

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_req); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_req); }
};



class getaddrinfo : public request
{
  //! \cond
  friend class request::instance< getaddrinfo >;
  //! \endcond

public: /*types*/
  using uv_t = ::uv_getaddrinfo_t;
  using on_request_t = std::function< void(int) >;

private: /*types*/
  using instance = request::instance< getaddrinfo >;

public: /*constructors*/
  ~getaddrinfo() = default;

 
  getaddrinfo(const getaddrinfo&) = default;
  getaddrinfo& operator =(const getaddrinfo&) = default;

  getaddrinfo(getaddrinfo&&) noexcept = default;
  getaddrinfo& operator =(getaddrinfo&&) noexcept = default;

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_req); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_req); }
};



class getnameinfo : public request
{
  //! \cond
  friend class request::instance< getnameinfo >;
  //! \endcond

public: /*types*/
  using uv_t = ::uv_getnameinfo_t;
  using on_request_t = std::function< void(int) >;

private: /*types*/
  using instance = request::instance< getnameinfo >;

public: /*constructors*/
  ~getnameinfo() = default;

  getnameinfo(const getnameinfo&) = default;
  getnameinfo& operator =(const getnameinfo&) = default;

  getnameinfo(getnameinfo&&) noexcept = default;
  getnameinfo& operator =(getnameinfo&&) noexcept = default;

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_req); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_req); }
};


//! \}
}


namespace std
{

//! \ingroup g__request
template<> inline void swap(uv::request &_this, uv::request &_that) noexcept  { _this.swap(_that); }

}



#endif
