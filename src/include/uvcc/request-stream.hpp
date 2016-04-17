
#ifndef UVCC_REQUEST_STREAM__HPP
#define UVCC_REQUEST_STREAM__HPP

#include "uvcc/utility.hpp"
#include "uvcc/request-base.hpp"
#include "uvcc/handle-stream.hpp"
#include "uvcc/buffer.hpp"

#include <uv.h>
#include <functional>   // function
#include <utility>      // move()
#include <type_traits>  // enable_if_t


namespace uv
{


/*! \ingroup doxy_request
    \brief Connect request type.
    \sa libuv API documentation: [`uv_stream_t`](http://docs.libuv.org/en/v1.x/stream.html#uv-stream-t-stream-handle),
                                 [`uv_tcp_t`](http://docs.libuv.org/en/v1.x/tcp.html#uv-tcp-t-tcp-handle),
                                 [`uv_pipe_t`](http://docs.libuv.org/en/v1.x/pipe.html#uv-pipe-t-pipe-handle). */
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

  /*! \brief The stream which this connect request is running on. */
  stream handle() const noexcept  { return stream(static_cast< uv_t* >(uv_req)->handle); }

  /*! \brief Run the request for `uv::tcp` stream.
      \sa libuv API documentation: [`uv_tcp_connect()`](http://docs.libuv.org/en/v1.x/tcp.html#c.uv_tcp_connect). */
  template<
      typename _T_,
      typename = std::enable_if_t< is_one_of< _T_, ::sockaddr, ::sockaddr_in, ::sockaddr_in6, ::sockaddr_storage >::value >
  >
  int run(tcp _tcp, const _T_ &_sockaddr)
  {
    tcp::instance::from(_tcp.uv_handle)->ref();
    instance::from(uv_req)->ref();
    uv_status(0);
    int o = ::uv_tcp_connect(
        static_cast< uv_t* >(uv_req), static_cast< tcp::uv_t* >(_tcp),
        reinterpret_cast< const ::sockaddr* >(&_sockaddr),
        connect_cb
    );
    if (!o)  uv_status(o);
    return o;
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
  self->uv_status() = _status;

  ref_guard< stream::instance > unref_handle(*stream::instance::from(_uv_req->handle), adopt_ref);
  ref_guard< instance > unref_req(*self, adopt_ref);

  auto &connect_cb = self->on_request();
  if (connect_cb)  connect_cb(connect(_uv_req));
}



/*! \ingroup doxy_request
    \brief Write request type.
    \sa libuv API documentation: [`uv_stream_t`](http://docs.libuv.org/en/v1.x/stream.html#uv-stream-t-stream-handle). */
class write : public request
{
  //! \cond
  friend class request::instance< write >;
  //! \endcond

public: /*types*/
  using uv_t = ::uv_write_t;
  using on_request_t = std::function< void(write, buffer) >;
  /*!< \brief The function type of the callback called after data was written on a stream.
       \sa libuv API documentation: [`uv_write_cb`](http://docs.libuv.org/en/v1.x/stream.html#c.uv_write_cb). */

protected: /*types*/
  //! \cond
  using supplemental_data_t = buffer::uv_t*;
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
  int run(stream _stream, const buffer &_buf)
  {
    auto self = instance::from(uv_req);

    stream::instance::from(_stream.uv_handle)->ref();
    buffer::instance::from(_buf.uv_buf)->ref();
    self->ref();
    self->supplemental_data() = _buf.uv_buf;

    uv_status(0);
    int o = ::uv_write(
        static_cast< uv_t* >(uv_req), static_cast< stream::uv_t* >(_stream),
        static_cast< const buffer::uv_t* >(_buf), _buf.count(),
        write_cb
    );
    if (!o)  uv_status(o);
    return o;
  }
  /*! \brief The overload for sending handles over a pipe.
      \sa libuv API documentation: [`uv_write2()`](http://docs.libuv.org/en/v1.x/stream.html#c.uv_write2). */
  int run(pipe _pipe, const buffer &_buf, stream _send_handle)
  {
    auto self = instance::from(uv_req);

    pipe::instance::from(_pipe.uv_handle)->ref();
    buffer::instance::from(_buf.uv_buf)->ref();
    stream::instance::from(_send_handle.uv_handle)->ref();
    self->ref();
    self->supplemental_data() = _buf.uv_buf;

    uv_status(0);
    int o = ::uv_write2(
        static_cast< uv_t* >(uv_req), static_cast< stream::uv_t* >(_pipe),
        static_cast< const buffer::uv_t* >(_buf), _buf.count(),
        static_cast< stream::uv_t* >(_send_handle),
        write2_cb
    );
    if (!o)  uv_status(o);
    return o;
  }

  /*! \details The wrapper for corresponding libuv function.
      \note It tries to execute and complete immediately and does not call the request callback.
      \sa libuv API documentation: [`uv_try_write()`](http://docs.libuv.org/en/v1.x/stream.html#c.uv_try_write). */
  int try_write(stream _stream, const buffer _buf)
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
  auto self = instance::from(_uv_req);
  self->uv_status() = _status;

  ref_guard< stream::instance > unref_handle(*stream::instance::from(_uv_req->handle), adopt_ref);
  ref_guard< instance > unref_req(*self, adopt_ref);

  auto &write_cb = self->on_request();
  if (write_cb)
    write_cb(write(_uv_req), buffer(self->supplemental_data(), adopt_ref));
  else
    buffer::instance::from(self->supplemental_data())->unref();
}
template< typename >
void write::write2_cb(::uv_write_t *_uv_req, int _status)
{
  ref_guard< stream::instance > unref_send_handle(*stream::instance::from(_uv_req->send_handle), adopt_ref);
  write_cb(_uv_req, _status);
}



/*! \ingroup doxy_request
    \brief Shutdown request type.
    \sa libuv API documentation: [`uv_stream_t`](http://docs.libuv.org/en/v1.x/stream.html#uv-stream-t-stream-handle). */
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
    uv_status(0);
    int o = ::uv_shutdown(static_cast< uv_t* >(uv_req), static_cast< stream::uv_t* >(_stream), shutdown_cb);
    if (!o)  uv_status(o);
    return o;
  }

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_req); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_req); }
};

template< typename >
void shutdown::shutdown_cb(::uv_shutdown_t *_uv_req, int _status)
{
  auto self = instance::from(_uv_req);
  self->uv_status() = _status;

  ref_guard< stream::instance > unref_handle(*stream::instance::from(_uv_req->handle), adopt_ref);
  ref_guard< instance > unref_req(*self, adopt_ref);

  auto &shutdown_cb = self->on_request();
  if (shutdown_cb)  shutdown_cb(shutdown(_uv_req));
}


}


#endif
