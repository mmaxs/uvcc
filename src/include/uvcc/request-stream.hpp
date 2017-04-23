
#ifndef UVCC_REQUEST_STREAM__HPP
#define UVCC_REQUEST_STREAM__HPP

#include "uvcc/utility.hpp"
#include "uvcc/request-base.hpp"
#include "uvcc/handle-stream.hpp"
#include "uvcc/buffer.hpp"

#include <uv.h>

#include <functional>   // function
#include <type_traits>  // enable_if_t


namespace uv
{


/*! \ingroup doxy_group__request
    \brief Stream connect request type.
    \sa libuv API documentation: [`uv_stream_t — Stream handle`](http://docs.libuv.org/en/v1.x/stream.html#uv-stream-t-stream-handle),\n
                                 [`uv_tcp_t — TCP handle`](http://docs.libuv.org/en/v1.x/tcp.html#uv-tcp-t-tcp-handle),\n
                                 [`uv_pipe_t — Pipe handle`](http://docs.libuv.org/en/v1.x/pipe.html#uv-pipe-t-pipe-handle). */
class connect : public request
{
  //! \cond
  friend class request::instance< connect >;
  //! \endcond

public: /*types*/
  using uv_t = ::uv_connect_t;
  using on_request_t = std::function< void(connect _request) >;
  /*!< \brief The function type of the callback called after connection is done.
       \sa libuv API documentation: [`uv_connect_cb`](http://docs.libuv.org/en/v1.x/stream.html#c.uv_connect_cb). */

private: /*types*/
  using instance = request::instance< connect >;

protected: /*constructors*/
  //! \cond
  explicit connect(uv_t *_uv_req) : request(reinterpret_cast< request::uv_t* >(_uv_req))  {}
  //! \endcond

public: /*constructors*/
  ~connect() = default;
  connect()
  {
    uv_req = instance::create();
    static_cast< uv_t* >(uv_req)->type = UV_CONNECT;
  }

  connect(const connect&) = default;
  connect& operator =(const connect&) = default;

  connect(connect&&) noexcept = default;
  connect& operator =(connect&&) noexcept = default;

private: /*functions*/
  template< typename = void > static void connect_cb(::uv_connect_t*, int);

public: /*interface*/
  const on_request_t& on_request() const noexcept  { return instance::from(uv_req)->request_cb_storage.value(); }
        on_request_t& on_request()       noexcept  { return instance::from(uv_req)->request_cb_storage.value(); }

  /*! \brief The stream which this connect request has been running on. */
  stream handle() const noexcept  { return stream(static_cast< uv_t* >(uv_req)->handle); }

  /*! \brief Run the request for `uv::tcp` stream.
      \sa libuv API documentation: [`uv_tcp_connect()`](http://docs.libuv.org/en/v1.x/tcp.html#c.uv_tcp_connect). */
  template<
      typename _T_,
      typename = std::enable_if_t< is_one_of< _T_, ::sockaddr, ::sockaddr_in, ::sockaddr_in6, ::sockaddr_storage >::value >
  >
  int run(tcp &_tcp, const _T_ &_sockaddr)
  {
    tcp::instance::from(_tcp.uv_handle)->ref();
    instance::from(uv_req)->ref();

    uv_status(0);
    auto uv_ret = ::uv_tcp_connect(
        static_cast< uv_t* >(uv_req), static_cast< tcp::uv_t* >(_tcp),
        reinterpret_cast< const ::sockaddr* >(&_sockaddr),
        connect_cb
    );
    if (uv_ret < 0)
    {
      uv_status(uv_ret);
      tcp::instance::from(_tcp.uv_handle)->unref();
      instance::from(uv_req)->unref();
    }

    return uv_ret;
  }
  /*! \brief Run the request for `uv::pipe` stream.
      \sa libuv API documentation: [`uv_pipe_connect()`](http://docs.libuv.org/en/v1.x/pipe.html#c.uv_pipe_connect). */
  void run(pipe &_pipe, const char *_name)
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
  auto instance_ptr = instance::from(_uv_req);
  instance_ptr->uv_error = _status;

  ref_guard< stream::instance > unref_handle(*stream::instance::from(_uv_req->handle), adopt_ref);
  ref_guard< instance > unref_req(*instance_ptr, adopt_ref);

  auto &connect_cb = instance_ptr->request_cb_storage.value();
  if (connect_cb)  connect_cb(connect(_uv_req));
}



/*! \ingroup doxy_group__request
    \brief Stream write request type.
    \sa libuv API documentation: [`uv_stream_t — Stream handle`](http://docs.libuv.org/en/v1.x/stream.html#uv-stream-t-stream-handle). */
class write : public request
{
  //! \cond
  friend class request::instance< write >;
  friend class output;
  //! \endcond

public: /*types*/
  using uv_t = ::uv_write_t;
  using on_request_t = std::function< void(write _request, buffer _buffer) >;
  /*!< \brief The function type of the callback called after data was written on a stream.
       \sa libuv API documentation: [`uv_write_cb`](http://docs.libuv.org/en/v1.x/stream.html#c.uv_write_cb). */

protected: /*types*/
  //! \cond internals
  //! \addtogroup doxy_group__internals
  //! \{
  struct properties : request::properties
  {
    buffer::uv_t *uv_buf = nullptr;
  };
  //! \}
  //! \endcond

private: /*types*/
  using instance = request::instance< write >;

protected: /*constructors*/
  //! \cond
  explicit write(uv_t *_uv_req) : request(reinterpret_cast< request::uv_t* >(_uv_req))  {}
  //! \endcond

public: /*constructors*/
  ~write() = default;
  write()
  {
    uv_req = instance::create();
    static_cast< uv_t* >(uv_req)->type = UV_WRITE;
  }

  write(const write&) = default;
  write& operator =(const write&) = default;

  write(write&&) noexcept = default;
  write& operator =(write&&) noexcept = default;

private: /*functions*/
  template< typename = void > static void write_cb(::uv_write_t*, int);
  template< typename = void > static void write2_cb(::uv_write_t*, int);

public: /*interface*/
  const on_request_t& on_request() const noexcept  { return instance::from(uv_req)->request_cb_storage.value(); }
        on_request_t& on_request()       noexcept  { return instance::from(uv_req)->request_cb_storage.value(); }

  /*! \brief The stream which this write request has been running on. */
  stream handle() const noexcept  { return stream(static_cast< uv_t* >(uv_req)->handle); }
  /*! \brief The handle of the stream to be sent over a pipe using this write request. */
  stream send_handle() const noexcept  { return stream(static_cast< uv_t* >(uv_req)->send_handle); }

  /*! \brief Run the request.
      \sa libuv API documentation: [`uv_write()`](http://docs.libuv.org/en/v1.x/stream.html#c.uv_write). */
  int run(stream &_stream, const buffer &_buf)
  {
    auto instance_ptr = instance::from(uv_req);

    stream::instance::from(_stream.uv_handle)->ref();
    buffer::instance::from(_buf.uv_buf)->ref();
    instance_ptr->ref();

    // instance_ptr->properties() = { _buf.uv_buf };
    {
      auto &properties = instance_ptr->properties();
      properties.uv_buf = _buf.uv_buf;
    }

    uv_status(0);
    auto uv_ret = ::uv_write(
        static_cast< uv_t* >(uv_req), static_cast< stream::uv_t* >(_stream),
        static_cast< const buffer::uv_t* >(_buf), _buf.count(),
        write_cb
    );
    if (uv_ret < 0)
    {
      uv_status(uv_ret);
      stream::instance::from(_stream.uv_handle)->unref();
      buffer::instance::from(_buf.uv_buf)->unref();
      instance_ptr->unref();
    }

    return uv_ret;
  }
  /*! \brief The overload for sending handles over a pipe.
      \sa libuv API documentation: [`uv_write2()`](http://docs.libuv.org/en/v1.x/stream.html#c.uv_write2). */
  int run(pipe &_pipe, const buffer &_buf, stream &_send_handle)
  {
    auto instance_ptr = instance::from(uv_req);

    pipe::instance::from(_pipe.uv_handle)->ref();
    buffer::instance::from(_buf.uv_buf)->ref();
    stream::instance::from(_send_handle.uv_handle)->ref();
    instance_ptr->ref();

    // instance_ptr->properties() = { _buf.uv_buf };
    {
      auto &properties = instance_ptr->properties();
      properties.uv_buf = _buf.uv_buf;
    }

    uv_status(0);
    auto uv_ret = ::uv_write2(
        static_cast< uv_t* >(uv_req), static_cast< stream::uv_t* >(_pipe),
        static_cast< const buffer::uv_t* >(_buf), _buf.count(),
        static_cast< stream::uv_t* >(_send_handle),
        write2_cb
    );
    if (uv_ret < 0)
    {
      uv_status(uv_ret);
      pipe::instance::from(_pipe.uv_handle)->unref();
      buffer::instance::from(_buf.uv_buf)->unref();
      stream::instance::from(_send_handle.uv_handle)->unref();
      instance_ptr->unref();
    }

    return uv_ret;
  }

  /*! \details The wrapper for a corresponding libuv function.
      \note It tries to execute and complete immediately and does not call the request callback.
      \sa libuv API documentation: [`uv_try_write()`](http://docs.libuv.org/en/v1.x/stream.html#c.uv_try_write). */
  int try_write(stream &_stream, const buffer &_buf)
  {
    return uv_status(
        ::uv_try_write(static_cast< stream::uv_t* >(_stream), static_cast< const buffer::uv_t* >(_buf), _buf.count())
    );
  }

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_req); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_req); }
};

template< typename >
void write::write_cb(::uv_write_t *_uv_req, int _status)
{
  auto instance_ptr = instance::from(_uv_req);
  instance_ptr->uv_error = _status;

  ref_guard< stream::instance > unref_handle(*stream::instance::from(_uv_req->handle), adopt_ref);
  ref_guard< instance > unref_req(*instance_ptr, adopt_ref);

  auto &write_cb = instance_ptr->request_cb_storage.value();
  if (write_cb)
    write_cb(write(_uv_req), buffer(instance_ptr->properties().uv_buf, adopt_ref));
  else
    buffer::instance::from(instance_ptr->properties().uv_buf)->unref();
}
template< typename >
void write::write2_cb(::uv_write_t *_uv_req, int _status)
{
  ref_guard< stream::instance > unref_send_handle(*stream::instance::from(_uv_req->send_handle), adopt_ref);
  write_cb(_uv_req, _status);
}



/*! \ingroup doxy_group__request
    \brief Stream shutdown request type.
    \sa libuv API documentation: [`uv_stream_t — Stream handle`](http://docs.libuv.org/en/v1.x/stream.html#uv-stream-t-stream-handle). */
class shutdown : public request
{
  //! \cond
  friend class request::instance< shutdown >;
  //! \endcond

public: /*types*/
  using uv_t = ::uv_shutdown_t;
  using on_request_t = std::function< void(shutdown _request) >;
  /*!< \brief The function type of the callback called after shutdown is done.
       \sa libuv API documentation: [`uv_shutdown_cb`](http://docs.libuv.org/en/v1.x/stream.html#c.uv_shutdown_cb). */

private: /*types*/
  using instance = request::instance< shutdown >;

protected: /*constructors*/
  //! \cond
  explicit shutdown(uv_t *_uv_req) : request(reinterpret_cast< request::uv_t* >(_uv_req))  {}
  //! \endcond

public: /*constructors*/
  ~shutdown() = default;
  shutdown()
  {
    uv_req = instance::create();
    static_cast< uv_t* >(uv_req)->type = UV_SHUTDOWN;
  }

  shutdown(const shutdown&) = default;
  shutdown& operator =(const shutdown&) = default;

  shutdown(shutdown&&) noexcept = default;
  shutdown& operator =(shutdown&&) noexcept = default;

private: /*functions*/
  template< typename = void > static void shutdown_cb(::uv_shutdown_t*, int);

public: /*interface*/
  const on_request_t& on_request() const noexcept  { return instance::from(uv_req)->request_cb_storage.value(); }
        on_request_t& on_request()       noexcept  { return instance::from(uv_req)->request_cb_storage.value(); }

  /*! \brief The stream which this shutdown request has been running on. */
  stream handle() const noexcept  { return stream(static_cast< uv_t* >(uv_req)->handle); }

  /*! \brief Run the request. */
  int run(stream &_stream)
  {
    stream::instance::from(_stream.uv_handle)->ref();
    instance::from(uv_req)->ref();

    uv_status(0);
    auto uv_ret = ::uv_shutdown(static_cast< uv_t* >(uv_req), static_cast< stream::uv_t* >(_stream), shutdown_cb);
    if (uv_ret < 0)
    {
      uv_status(uv_ret);
      stream::instance::from(_stream.uv_handle)->unref();
      instance::from(uv_req)->unref();
    }

    return uv_ret;
  }

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_req); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_req); }
};

template< typename >
void shutdown::shutdown_cb(::uv_shutdown_t *_uv_req, int _status)
{
  auto instance_ptr = instance::from(_uv_req);
  instance_ptr->uv_error = _status;

  ref_guard< stream::instance > unref_handle(*stream::instance::from(_uv_req->handle), adopt_ref);
  ref_guard< instance > unref_req(*instance_ptr, adopt_ref);

  auto &shutdown_cb = instance_ptr->request_cb_storage.value();
  if (shutdown_cb)  shutdown_cb(shutdown(_uv_req));
}


}


#endif
