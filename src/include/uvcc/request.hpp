
#ifndef UVCC_REQUEST__HPP
#define UVCC_REQUEST__HPP

#include "uvcc/utility.hpp"
#include "uvcc/handle.hpp"
#include "uvcc/buffer.hpp"
#include "uvcc/loop.hpp"

#include <uv.h>
#include <cstddef>      // offsetof
#include <cstring>      // memset()
#include <memory>       // addressof()
#include <functional>   // function
#include <utility>      // swap() move()
#include <type_traits>  // aligned_storage is_standard_layout conditional_t is_void enable_if_t
#ifdef _WIN32
#include <io.h>         // _dup()
#else
#include <unistd.h>     // dup()
#endif


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
    \sa libuv API documentation: [`uv_req_t`](http://docs.libuv.org/en/v1.x/request.html#uv-req-t-base-request). */
class request
{
public: /*types*/
  using uv_t = ::uv_req_t;
  using on_destroy_t = std::function< void(void *_data) >;
  /*!< \brief The function type of the callback called when the request object is about to be destroyed. */
  using on_request_t = null_t;

protected: /*types*/
  //! \cond
  using supplemental_data_t = empty_t;

  template< class _REQUEST_ > class instance
  {
  public: /*types*/
    using uv_t = typename _REQUEST_::uv_t;
    using on_request_t = typename _REQUEST_::on_request_t;

  private: /*types*/
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
    alignas(32) uv_t uv_req = {0,};  // must be zeroed!

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
      auto &destroy_cb = on_destroy_storage.value();
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
      \sa libuv API documentation: [`uv_cancel()`](http://docs.libuv.org/en/v1.x/request.html#c.uv_cancel).*/
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
       \sa libuv API documentation: [`uv_connect_cb`](http://docs.libuv.org/en/v1.x/stream.html#c.uv_connect_cb). */

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
      \sa libuv API documentation: [`uv_tcp_connect()`](http://docs.libuv.org/en/v1.x/tcp.html#c.uv_tcp_connect). */
  template< typename _T_, typename = std::enable_if_t< is_one_of< _T_, ::sockaddr, ::sockaddr_in, ::sockaddr_in6, ::sockaddr_storage >::value > >
  int run(tcp _tcp, const _T_ &_sockaddr)
  {
    tcp::instance::from(_tcp.uv_handle)->ref();
    instance::from(uv_req)->ref();
    return uv_status(::uv_tcp_connect(static_cast< uv_t* >(uv_req), static_cast< tcp::uv_t* >(_tcp), reinterpret_cast< const ::sockaddr* >(&_sockaddr), connect_cb));
  }
  /*! \brief Run the request for `uv::pipe` stream.
      \sa libuv API documentation: [`uv_pipe_connect()`](http://docs.libuv.org/en/v1.x/pipe.html#c.uv_pipe_connect). */
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
  auto self = instance::from(_uv_req);

  ref_guard< stream::instance > unref_handle(*stream::instance::from(_uv_req->handle), adopt_ref);
  ref_guard< instance > unref_req(*self, adopt_ref);

  self->uv_status() = _status;
  auto &connect_cb = self->on_request();
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
       \sa libuv API documentation: [`uv_write_cb`](http://docs.libuv.org/en/v1.x/stream.html#c.uv_write_cb). */

protected: /*types*/
  //! \cond
  using supplemental_data_t = buffer;
  //! \endcond

private: /*types*/
  using instance = request::instance< write >;

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
      \sa libuv API documentation: [`uv_write()`](http://docs.libuv.org/en/v1.x/stream.html#c.uv_write). */
  int run(stream _stream, const buffer _buf)
  {
    auto self = instance::from(uv_req);

    stream::instance::from(_stream.uv_handle)->ref();
    buffer &b = (self->supplemental_data() = std::move(_buf));
    self->ref();

    return uv_status(::uv_write(
        static_cast< uv_t* >(uv_req), static_cast< stream::uv_t* >(_stream),
        static_cast< const buffer::uv_t* >(b), b.count(),
        write_cb
    ));
  }
  /*! \brief The overload for sending handles over a pipe.
      \sa libuv API documentation: [`uv_write2()`](http://docs.libuv.org/en/v1.x/stream.html#c.uv_write2). */
  int run(pipe _pipe, const buffer _buf, stream _send_handle)
  {
    auto self = instance::from(uv_req);

    pipe::instance::from(_pipe.uv_handle)->ref();
    buffer &b = (self->supplemental_data() = std::move(_buf));
    stream::instance::from(_send_handle.uv_handle)->ref();
    self->ref();

    return uv_status(::uv_write2(static_cast< uv_t* >(uv_req), static_cast< stream::uv_t* >(_pipe), static_cast< const buffer::uv_t* >(b), b.count(), static_cast< stream::uv_t* >(_send_handle), write2_cb));
  }

  /*! \details The wrapper for corresponding libuv function.
      \note It tries to execute and complete immediately and does not call the request callback.
      \sa libuv API documentation: [`uv_try_write()`](http://docs.libuv.org/en/v1.x/stream.html#c.uv_try_write). */
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
  auto self = instance::from(_uv_req);

  ref_guard< stream::instance > unref_handle(*stream::instance::from(_uv_req->handle), adopt_ref);
  buffer b(std::move(self->supplemental_data()));
  ref_guard< instance > unref_req(*self, adopt_ref);

  self->uv_status() = _status;
  auto &write_cb = self->on_request();
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
       \sa libuv API documentation: [`uv_shutdown_cb`](http://docs.libuv.org/en/v1.x/stream.html#c.uv_shutdown_cb). */

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
  auto self = instance::from(_uv_req);

  ref_guard< stream::instance > unref_handle(*stream::instance::from(_uv_req->handle), adopt_ref);
  ref_guard< instance > unref_req(*self, adopt_ref);

  self->uv_status() = _status;
  auto &shutdown_cb = self->on_request();
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



/*! \brief The request type for storing a file handle and performing operations on it in synchronous mode. */
class file : public request
{
  //! \cond
  friend class request::instance< file >;
  //! \endcond

public: /*types*/
  using uv_t = ::uv_fs_t;
  using on_request_t = empty_t;

protected: /*types*/
  //! \cond
  struct supplemental_data_t
  {
    uv_t *uv_req = nullptr;
    uv_t *req_fstat = nullptr;
    ~supplemental_data_t()
    {
      if (req_fstat)  ::uv_fs_req_cleanup(req_fstat);
      if (uv_req)
      {
        if (uv_req->result >= 0)  // assuming that uv_req->result has been initialized with value < 0
        {
          uv_t req_close;
          ::uv_fs_close(nullptr, &req_close, uv_req->result, nullptr);
          ::uv_fs_req_cleanup(&req_close);
        };
        ::uv_fs_req_cleanup(uv_req);
      };
    }
  };
  //! \endcond

private: /*types*/
  using instance = request::instance< file >;

private: /*constructors*/
  explicit file(uv_t *_uv_req)
  {
    if (_uv_req)  instance::from(_uv_req)->ref();
    uv_req = _uv_req;
  }

public: /*constructors*/
  ~file() = default;
  /*! \brief Open and possibly create a file.
      \sa libuv API documentation: [`uv_fs_open()`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_open).
      \sa Linux: [`open()`](http://man7.org/linux/man-pages/man2/open.2.html).
          Windows: [`_open()`](https://msdn.microsoft.com/en-us/library/z0kc8e3z.aspx).

      The file descriptor will be closed automatically when the file instance reference count has became zero. */
  file(const char *_path, int _flags, int _mode)
  {
    uv_req = instance::create();
    instance::from(uv_req)->supplemental_data().uv_req = static_cast< uv_t* >(uv_req);
    uv_status(::uv_fs_open(
        nullptr, static_cast< uv_t* >(uv_req),
        _path, _flags, _mode,
        nullptr
    ));
  }

  file(const file&) = default;
  file& operator =(const file&) = default;

  file(file&&) noexcept = default;
  file& operator =(file&&) noexcept = default;

public: /*interface*/
  /*! \brief Get the cross platform representation of a file handle (mostly being a POSIX-like file descriptor). */
  ::uv_file fd() const noexcept  { return static_cast< uv_t* >(uv_req)->result; }
  /*! \brief The file path. */
  const char* path() const noexcept  { return static_cast< uv_t* >(uv_req)->path; }

  /*! \brief Duplicate this file descriptor. */
  ::uv_file dup() const noexcept
  {
    if (fd() < 0)  return -1;
#ifdef _WIN32
    return ::_dup(fd());
#else
    return ::dup(fd());
#endif
  }

  /*! \brief Read data from the file into the buffers described by `_buf` object.
      \returns The number of bytes read or relevant [libuv error constant](http://docs.libuv.org/en/v1.x/errors.html#error-constants).
      \sa libuv API documentation: [`uv_fs_read()`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_read).
      \sa Linux: [`preadv()`](http://man7.org/linux/man-pages/man2/preadv.2.html). */
  int read(buffer &_buf, int64_t _offset = -1) const noexcept
  {
    uv_t req_read;
    uv_status(::uv_fs_read(
        nullptr, &req_read,
        fd(),
        static_cast< const buffer::uv_t* >(_buf), _buf.count(),
        _offset,
        nullptr
    ));
    ::uv_fs_req_cleanup(&req_read);
    return uv_status();
  }
  /*! \brief Write data to the file from the buffers described by `_buf` object.
      \returns The number of bytes written or relevant [libuv error constant](http://docs.libuv.org/en/v1.x/errors.html#error-constants).
      \sa libuv API documentation: [`uv_fs_write()`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_write).
      \sa Linux: [`pwritev()`](http://man7.org/linux/man-pages/man2/pwritev.2.html). */
  int write(const buffer &_buf, int64_t _offset = -1) noexcept
  {
    uv_t req_write;
    uv_status(::uv_fs_write(
        nullptr, &req_write,
        fd(),
        static_cast< const buffer::uv_t* >(_buf), _buf.count(),
        _offset,
        nullptr
    ));
    ::uv_fs_req_cleanup(&req_write);
    return uv_status();
  }

  /*! \brief Get information about the file.
      \details The result of the request is stored internally in the libuv request description structure.
      Use `uv_status()` member function to check the status of the request completion.
      \sa libuv API documentation: [`uv_fs_t.statbuf`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_t.statbuf),
                                   [`uv_stat_t`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_stat_t),
                                   [`uv_fs_fstat()`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_fstat).
      \sa Linux: [`fstat()`](http://man7.org/linux/man-pages/man2/fstat.2.html). */
  const ::uv_stat_t& fstat() const noexcept
  {
    uv_t* &req_fstat = instance::from(uv_req)->supplemental_data().req_fstat;
    if (!req_fstat)
    {
      req_fstat = new uv_t;
      std::memset(req_fstat, 0, sizeof(*req_fstat));
    }
    else
      ::uv_fs_req_cleanup(req_fstat);

    uv_status(::uv_fs_fstat(
        nullptr, req_fstat,
        fd(),
        nullptr
    ));
    return req_fstat->statbuf;
  }

  /*! \brief Synchronize all modified file's in-core data with storage.
      \sa libuv API documentation: [`uv_fs_fsync()`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_fsync).
      \sa Linux: [`fsync()`](http://man7.org/linux/man-pages/man2/fsync.2.html). */
  int fsync() noexcept
  {
    uv_t req_fsync;
    uv_status(::uv_fs_fsync(
        nullptr, &req_fsync,
        fd(),
        nullptr
    ));
    ::uv_fs_req_cleanup(&req_fsync);
    return uv_status();
  }
  /*! \brief Synchronize modified file's data with storage excluding
      flushing unnecessary metadata to reduce disk activity.
      \sa libuv API documentation: [`uv_fs_fdatasync()`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_fdatasync).
      \sa Linux: [`fdatasync()`](http://man7.org/linux/man-pages/man2/fdatasync.2.html). */
  int fdatasync() noexcept
  {
    uv_t req_fdatasync;
    uv_status(::uv_fs_fdatasync(
        nullptr, &req_fdatasync,
        fd(),
        nullptr
    ));
    ::uv_fs_req_cleanup(&req_fdatasync);
    return uv_status();
  }

  /*! \brief Truncate the file to a specified length.
      \sa libuv API documentation: [`uv_fs_ftruncate()`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_ftruncate).
      \sa Linux: [`ftruncate()`](http://man7.org/linux/man-pages/man2/ftruncate.2.html). */
  int ftruncate(int64_t _offset) noexcept
  {
    uv_t req_ftruncate;
    uv_status(::uv_fs_ftruncate(
        nullptr, &req_ftruncate,
        fd(),
        _offset,
        nullptr
    ));
    ::uv_fs_req_cleanup(&req_ftruncate);
    return uv_status();
  }

  /*! \brief Change permissions of the file.
      \sa libuv API documentation: [`uv_fs_fchmod()`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_fchmod).
      \sa Linux: [`fchmod()`](http://man7.org/linux/man-pages/man2/fchmod.2.html). */
  int fchmod(int _mode)
  {
    uv_t req_fchmod;
    uv_status(::uv_fs_fchmod(
        nullptr, &req_fchmod,
        fd(),
        _mode,
        nullptr
    ));
    ::uv_fs_req_cleanup(&req_fchmod);
    return uv_status();
  }
  /*! \brief Change ownership of the file.
      \note Not implemented on Windows.
      \sa libuv API documentation: [`uv_fs_fchown()`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_fchown).
      \sa Linux: [`fchown()`](http://man7.org/linux/man-pages/man2/fchown.2.html). */
  int fchown(::uv_uid_t _uid, ::uv_gid_t _gid)
  {
    uv_t req_fchown;
    uv_status(::uv_fs_fchown(
        nullptr, &req_fchown,
        fd(),
        _uid, _gid,
        nullptr
    ));
    ::uv_fs_req_cleanup(&req_fchown);
    return uv_status();
  }

  /*! \brief Change file timestamps.
      \sa libuv API documentation: [`uv_fs_futime()`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_futime).
      \sa Linux: [`futimes()`](http://man7.org/linux/man-pages/man3/futimes.3.html). */
  int futime(double _atime, double _mtime)
  {
    uv_t req_futime;
    uv_status(::uv_fs_futime(
        nullptr, &req_futime,
        fd(),
        _atime, _mtime,
        nullptr
    ));
    ::uv_fs_req_cleanup(&req_futime);
    return uv_status();
  }

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



/*! \brief Getaddrinfo request type. */
class getaddrinfo : public request
{
  //! \cond
  friend class request::instance< getaddrinfo >;
  //! \endcond

public: /*types*/
  using uv_t = ::uv_getaddrinfo_t;
  using on_request_t = std::function< void(getaddrinfo, const ::addrinfo *_result) >;
  /*!< \brief The function type of the callback that is called with the `getaddrinfo` request result once complete.
       \sa libuv API documentation: [`uv_getaddrinfo_cb`](http://docs.libuv.org/en/v1.x/dns.html#c.uv_getaddrinfo_cb). */

protected: /*types*/
  //! \cond
  struct supplemental_data_t
  {
    uv_t *uv_req = nullptr;
    ~supplemental_data_t()  { if (uv_req)  ::uv_freeaddrinfo(uv_req->addrinfo); }
  };
  //! \endcond

private: /*types*/
  using instance = request::instance< getaddrinfo >;

private: /*constructors*/
  explicit getaddrinfo(uv_t *_uv_req)
  {
    if (_uv_req)  instance::from(_uv_req)->ref();
    uv_req = _uv_req;
  }

public: /*constructors*/
  ~getaddrinfo() = default;
  getaddrinfo()
  {
    uv_req = instance::create();
    instance::from(uv_req)->supplemental_data().uv_req = static_cast< uv_t* >(uv_req);
  }

  getaddrinfo(const getaddrinfo&) = default;
  getaddrinfo& operator =(const getaddrinfo&) = default;

  getaddrinfo(getaddrinfo&&) noexcept = default;
  getaddrinfo& operator =(getaddrinfo&&) noexcept = default;

private: /*functions*/
  template< typename = void > static void getaddrinfo_cb(::uv_getaddrinfo_t*, int, ::addrinfo*);

  int run(uv::loop _loop, const char *_hostname, const char *_service, const ::addrinfo *_hints)
  {
    auto &request_cb = on_request();
    if (request_cb)
    {
      uv::loop::instance::from(_loop.uv_loop)->ref();
      instance::from(uv_req)->ref();
    };

    ::uv_freeaddrinfo(static_cast< uv_t* >(uv_req)->addrinfo);  // assuming that *uv_req has initially been zeroed

    return uv_status(::uv_getaddrinfo(
        static_cast< loop::uv_t* >(_loop), static_cast< uv_t* >(uv_req),
        request_cb ? static_cast< ::uv_getaddrinfo_cb >(getaddrinfo_cb) : nullptr,
        _hostname, _service, _hints
    ));
  }

public: /*interface*/
  const on_request_t& on_request() const noexcept  { return instance::from(uv_req)->on_request(); }
        on_request_t& on_request()       noexcept  { return instance::from(uv_req)->on_request(); }

  /*! \brief The libuv loop that started this `getaddrinfo` request and where completion will be reported.
      \details It is guaranteed that it will be a valid instance at least within the request callback. */
  uv::loop loop() const noexcept  { return uv::loop(static_cast< uv_t* >(uv_req)->loop); }

  /*! \brief The pointer to a `struct addrinfo` containing the request result. */
  const ::addrinfo* addrinfo() const noexcept  { return static_cast< uv_t* >(uv_req)->addrinfo; }

  /*! \brief Run the request.
      \details For supplying `_hints` argument the appropriate helper function for \ref g__netstruct can be utilized.
      \sa libuv API documentation: [`uv_getaddrinfo()`](http://docs.libuv.org/en/v1.x/dns.html#c.uv_getaddrinfo).
      \note If the request callback is empty (has not been set), the request runs **synchronously**. */
  int run(uv::loop _loop, const char *_hostname, const char *_service, const ::addrinfo &_hints)
  {
    return run(std::move(_loop), _hostname, _service, &_hints);
  }
  /*! \brief Idem with empty `_hints`. */
  int run(uv::loop _loop, const char *_hostname, const char *_service)
  {
    return run(std::move(_loop), _hostname, _service, nullptr);
  }

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_req); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_req); }
};

template< typename >
void getaddrinfo::getaddrinfo_cb(::uv_getaddrinfo_t *_uv_req, int _status, ::addrinfo *_result)
{
  auto self = instance::from(_uv_req);

  ref_guard< uv::loop::instance > unref_loop(*uv::loop::instance::from(_uv_req->loop), adopt_ref);
  ref_guard< instance > unref_req(*self, adopt_ref);

  self->uv_status() = _status;
  auto &getaddrinfo_cb = self->on_request();
  if (getaddrinfo_cb)  getaddrinfo_cb(getaddrinfo(_uv_req), _result);
}



/*! \brief Getnameinfo request type. */
class getnameinfo : public request
{
  //! \cond
  friend class request::instance< getnameinfo >;
  //! \endcond

public: /*types*/
  using uv_t = ::uv_getnameinfo_t;
  using on_request_t = std::function< void(getnameinfo, const char *_hostname, const char *_service) >;
  /*!< \brief The function type of the callback that is called with the `getnameinfo` request result once complete.
       \sa libuv API documentation: [`uv_getnameinfo_cb`](http://docs.libuv.org/en/v1.x/dns.html#c.uv_getnameinfo_cb). */

private: /*types*/
  using instance = request::instance< getnameinfo >;

private: /*constructors*/
  explicit getnameinfo(uv_t *_uv_req)
  {
    if (_uv_req)  instance::from(_uv_req)->ref();
    uv_req = _uv_req;
  }

public: /*constructors*/
  ~getnameinfo() = default;
  getnameinfo()  { uv_req = instance::create(); }

  getnameinfo(const getnameinfo&) = default;
  getnameinfo& operator =(const getnameinfo&) = default;

  getnameinfo(getnameinfo&&) noexcept = default;
  getnameinfo& operator =(getnameinfo&&) noexcept = default;

private: /*functions*/
  template< typename = void > static void getnameinfo_cb(::uv_getnameinfo_t*, int, const char*, const char*);

public: /*interface*/
  const on_request_t& on_request() const noexcept  { return instance::from(uv_req)->on_request(); }
        on_request_t& on_request()       noexcept  { return instance::from(uv_req)->on_request(); }

  /*! \brief The libuv loop that started this `getnameinfo` request and where completion will be reported.
      \details It is guaranteed that it will be a valid instance at least within the request callback. */
  uv::loop loop() const noexcept  { return uv::loop(static_cast< uv_t* >(uv_req)->loop); }

  /*! \brief The char array containing the resulting host. It’s null terminated. */
  const char (& host() const noexcept)[NI_MAXHOST]  { return static_cast< uv_t* >(uv_req)->host; }
  /*! \brief The char array containing the resulting service. It’s null terminated. */
  const char (& service() const noexcept)[NI_MAXSERV]  { return static_cast< uv_t* >(uv_req)->service; }

  /*! \brief Run the request.
      \sa libuv API documentation: [`uv_getnameinfo()`](http://docs.libuv.org/en/v1.x/dns.html#c.uv_getnameinfo).
      \note If the request callback is empty, the request runs **synchronously**.

      Available `_NI_FLAGS` are:
+ NI_DGRAM
+ NI_NAMEREQD
+ NI_NOFQDN
+ NI_NUMERICHOST
+ NI_NUMERICSERV
.
      \sa Linux: [`getnameinfo()`](http://man7.org/linux/man-pages/man3/getnameinfo.3.html).
          Windows: [`getnameinfo()`](https://msdn.microsoft.com/en-us/library/ms738532(v=vs.85).aspx). */
  template< typename _T_, typename = std::enable_if_t< is_one_of< _T_, ::sockaddr, ::sockaddr_in, ::sockaddr_in6, ::sockaddr_storage >::value > >
  int run(uv::loop _loop, const _T_ &_sa, int _NI_FLAGS = 0)
  {
    auto &request_cb = on_request();
    if (request_cb)
    {
      uv::loop::instance::from(_loop.uv_loop)->ref();
      instance::from(uv_req)->ref();
    };

    return uv_status(::uv_getnameinfo(
        static_cast< loop::uv_t* >(_loop), static_cast< uv_t* >(uv_req),
        request_cb ? static_cast< ::uv_getnameinfo_cb >(getnameinfo_cb) : nullptr,
        reinterpret_cast< const ::sockaddr* >(&_sa), _NI_FLAGS
    ));
  }

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_req); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_req); }
};

template< typename >
void getnameinfo::getnameinfo_cb(::uv_getnameinfo_t *_uv_req, int _status, const char* _hostname, const char* _service)
{
  auto self = instance::from(_uv_req);

  ref_guard< uv::loop::instance > unref_loop(*uv::loop::instance::from(_uv_req->loop), adopt_ref);
  ref_guard< instance > unref_req(*self, adopt_ref);

  self->uv_status() = _status;
  auto &getnameinfo_cb = self->on_request();
  if (getnameinfo_cb)  getnameinfo_cb(getnameinfo(_uv_req), _hostname, _service);
}


//! \}
}


namespace std
{

//! \ingroup g__request
template<> inline void swap(uv::request &_this, uv::request &_that) noexcept  { _this.swap(_that); }

}



#endif
