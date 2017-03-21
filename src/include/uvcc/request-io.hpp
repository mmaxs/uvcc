
#ifndef UVCC_REQUEST_IO__HPP
#define UVCC_REQUEST_IO__HPP

#include "uvcc/utility.hpp"
#include "uvcc/request-base.hpp"
#include "uvcc/handle-io.hpp"
#include "uvcc/request-fs.hpp"
#include "uvcc/request-stream.hpp"
#include "uvcc/request-udp.hpp"
#include "uvcc/buffer.hpp"

#include <cstring>      // memset()
#include <uv.h>

#include <functional>   // function


namespace uv
{


/*! \ingroup doxy_group__request
    \brief Generic write/send request type for I/O endpoints (files, TCP/UDP sockets, pipes, TTYs).
    \details Virtually it appears to be a one of the request:
    - `uv::fs::write`
    - `uv::write`
    - `uv::udp_send`
    .
    depending on the actual type of the `io` argument passed to the `run()` member function. */
class output : public request
{
  //! \cond
  friend class request::instance< output >;
  //! \endcond

public: /*types*/
  //! \cond
  union uv_t
  {
    fs::write::uv_t uv_file_write_req;
    write::uv_t uv_stream_write_req;
    udp_send::uv_t uv_udp_send_req;
  };
  //! \endcond
  using on_request_t = std::function< void(output _request, buffer _buffer) >;
  /*!< \brief The function type of the callback called after data was written/sent to I/O endpoint.
       \sa `fs::write::on_request_t`,\n `write::on_request_t`,\n `udp_send::on_request_t`. */

protected: /*types*/
  //! \cond
  struct properties
  {
    union request_properties
    {
      fs::write::properties file_write_properties;
      write::properties stream_write_properties;
      udp_send::properties udp_send_properties;

      ~request_properties()  {}
      request_properties()  { std::memset(this, 0, sizeof(*this)); }
    };

    request_properties property_storage;  // it must be the first field to align with property storage of the actual request type
    ::uv_req_t *uv_req = nullptr;

    ~properties()
    {
      if (uv_req)  switch (uv_req->type)
      {
      case UV_WRITE:
          property_storage.stream_write_properties.~properties();
          break;
      case UV_UDP_SEND:
          property_storage.udp_send_properties.~properties();
          break;
      case UV_FS:
          property_storage.file_write_properties.~properties();
          break;
      default:
          break;
      }
    }
  };
  //! \endcond

private: /*types*/
  using instance = request::instance< output >;

private: /*constructors*/
  explicit output(uv_t *_uv_req)
  {
    if (_uv_req)  instance::from(_uv_req)->ref();
    uv_req = _uv_req;
  }

public: /*constructors*/
  ~output() = default;
  output()
  {
    uv_req = instance::create();
    static_cast< ::uv_req_t* >(uv_req)->type = UV_REQ;
    instance::from(uv_req)->properties().uv_req = static_cast< ::uv_req_t* >(uv_req);
  }

  output(const output&) = default;
  output& operator =(const output&) = default;

  output(output&&) noexcept = default;
  output& operator =(output&&) noexcept = default;

public: /*interface*/
  const on_request_t& on_request() const noexcept  { return instance::from(uv_req)->request_cb_storage.value(); }
        on_request_t& on_request()       noexcept  { return instance::from(uv_req)->request_cb_storage.value(); }

  /*! \brief The I/O endpoint handle where this output request has been taking place. */
  io handle() const noexcept
  {
    switch (static_cast< ::uv_req_t* >(uv_req)->type)
    {
    case UV_WRITE:
        return reinterpret_cast< const write* >(this)->handle();
    case UV_UDP_SEND:
        return reinterpret_cast< const udp_send* >(this)->handle();
    case UV_FS:
        return reinterpret_cast< const fs::write* >(this)->handle();
    default:
        io ret;
        ret.uv_status(UV_EBADF);
        return ret;
    }
  }

  /*! \brief Run the request with interpreting arguments as additional parameters for actual write/send
      request performed depending on what I/O endpoint the `_io` argument actually represents.
      \details
      The `output` request instance is static-casted to the corresponding write/send request for that endpoint and
      the following actual call is performed:
       `_io` endpoint type             | Actual output function
      :--------------------------------|:-----------------------
       `uv::file`                      | `fs::write::run(static_cast< uv::file& >(_io), _buf, _offset)`
       one of the `uv::stream` subtype | `write::run(static_cast< uv::stream& >(_io), _buf)`
       `uv::udp`                       | `udp_send::run(static_cast< uv::udp& >(_io), _buf, *static_cast< const udp::io_info* >(_info)->peer)`

      \note Employing this function can be practical within a `io::on_read_t` callback <em>when the type of the I/O
      endpoint that the output request is going to be run on is of the same type, as the one the `io::on_read_t` callback
      is called for</em> because the `_offset` and `_info` arguments are interpreted in the same way as when they have been
      passed into the `io::on_read_t` callback but for determining supplemental parameters for output operation.
      In any case the `output` request can be static-casted to the desired _request_ type corresponding to the output
      I/O endpoint and then the one of the _request_`::run()` available functions can be used. */
  int run(io &_io, const buffer &_buf, int64_t _offset = -1, void *_info = nullptr)
  {
    switch (_io.type())
    {
    case UV_NAMED_PIPE:
    case UV_TCP:
    case UV_TTY:
        static_cast< write::uv_t* >(uv_req)->type = UV_WRITE;
        return reinterpret_cast< write* >(this)->run(static_cast< stream& >(_io), _buf);
    case UV_UDP:
        static_cast< udp_send::uv_t* >(uv_req)->type = UV_UDP_SEND;
        return _info ?
            reinterpret_cast< udp_send* >(this)->run(
                static_cast< udp& >(_io), _buf,
                *static_cast< const udp::io_info* >(_info)->peer
            )
          :
            uv_status(UV_EINVAL);
    case UV_FILE:
        static_cast< fs::uv_t* >(uv_req)->type = UV_FS;
        static_cast< fs::uv_t* >(uv_req)->fs_type = UV_FS_WRITE;
        return reinterpret_cast< fs::write* >(this)->run(static_cast< file& >(_io), _buf, _offset);
    default:
        return uv_status(UV_EBADF);
    }
  }

  /*! \brief Same as `run()`, but won’t queue an output request if it can’t be completed immediately.
      \details Depending on the actual run-time type of the `_io` argument, the function appears to be an alias
      to the one of the following functon:
      - `fs::write::try_write()`, or
      - `write::try_write()`, or
      - `udp_send::try_send()`.
      . */
  int try_output(io &_io, const buffer &_buf, int64_t _offset = -1, void *_info = nullptr)
  {
    switch (_io.type())
    {
    case UV_NAMED_PIPE:
    case UV_TCP:
    case UV_TTY:
        static_cast< write::uv_t* >(uv_req)->type = UV_WRITE;
        return reinterpret_cast< write* >(this)->try_write(static_cast< stream& >(_io), _buf);
    case UV_UDP:
        static_cast< udp_send::uv_t* >(uv_req)->type = UV_UDP_SEND;
        return _info ?
            reinterpret_cast< udp_send* >(this)->try_send(
                static_cast< udp& >(_io), _buf,
                *static_cast< const udp::io_info* >(_info)->peer
            )
          :
            uv_status(UV_EINVAL);
    case UV_FILE:
        static_cast< fs::uv_t* >(uv_req)->type = UV_FS;
        static_cast< fs::uv_t* >(uv_req)->fs_type = UV_FS_WRITE;
        return reinterpret_cast< fs::write* >(this)->try_write(static_cast< file& >(_io), _buf, _offset);
    default:
        return uv_status(UV_EBADF);
    }
  }

public: /*conversion operators*/
  explicit operator const fs::write&() const noexcept  { return *reinterpret_cast< const fs::write* >(this); }
  explicit operator       fs::write&()       noexcept  { return *reinterpret_cast<       fs::write* >(this); }

  explicit operator const write&() const noexcept  { return *reinterpret_cast< const write* >(this); }
  explicit operator       write&()       noexcept  { return *reinterpret_cast<       write* >(this); }

  explicit operator const udp_send&() const noexcept  { return *reinterpret_cast< const udp_send* >(this); }
  explicit operator       udp_send&()       noexcept  { return *reinterpret_cast<       udp_send* >(this); }
};


}


#endif
