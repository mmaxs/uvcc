
#ifndef UVCC_REQUEST_IO__HPP
#define UVCC_REQUEST_IO__HPP

#include "uvcc/utility.hpp"
#include "uvcc/request-base.hpp"
#include "uvcc/handle-io.hpp"
#include "uvcc/request-fs.hpp"
#include "uvcc/request-stream.hpp"
#include "uvcc/request-udp.hpp"
#include "uvcc/buffer.hpp"

#include <uv.h>
#include <functional>   // function
#include <utility>      // forward(), declval
#include <type_traits>  // enable_if_t


namespace uv
{


/*! \ingroup doxy_group_request
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
  union uv_t
  {
    fs::write::uv_t uv_file_write_req;
    write::uv_t uv_stream_write_req;
    udp_send::uv_t uv_udp_send_req;
  };
  using on_request_t = std::function< void(output _request, buffer _buffer) >;
  /*!< \brief The function type of the callback called after data was written/sent to I/O endpoint.
       \sa `uv::fs::write::on_request_t`,\n `uv::write::on_request_t`,\n `uv::udp_send::on_request_t`. */

private: /*types*/
  using instance = request::instance< output >;

  template< class _T_, typename _Res_, typename... _Args_ > struct has_run_method
  {
    template< typename _U_, typename = _Res_ >
    struct test
    { static constexpr const bool value = false; };
    template< typename _U_ >
    struct test< _U_, decltype(std::declval< _U_ >().run(std::declval< _Args_ >()...)) >
    { static constexpr const bool value = true; };

    static constexpr const bool value = test< _T_ >::value;
  };

  template< class _T_, typename _Res_, typename... _Args_ > struct has_try_write_method
  {
    template< typename _U_, typename = _Res_ >
    struct test
    { static constexpr const bool value = false; };
    template< typename _U_ >
    struct test< _U_, decltype(std::declval< _U_ >().try_write(std::declval< _Args_ >()...)) >
    { static constexpr const bool value = true; };

    static constexpr const bool value = test< _T_ >::value;
  };

  template< class _T_, typename _Res_, typename... _Args_ > struct has_try_send_method
  {
    template< typename _U_, typename = _Res_ >
    struct test
    { static constexpr const bool value = false; };
    template< typename _U_ >
    struct test< _U_, decltype(std::declval< _U_ >().try_send(std::declval< _Args_ >()...)) >
    { static constexpr const bool value = true; };

    static constexpr const bool value = test< _T_ >::value;
  };

private: /*constructors*/
  explicit output(uv_t *_uv_req)
  {
    if (_uv_req)  instance::from(_uv_req)->ref();
    uv_req = _uv_req;
  }

private: /*functions*/
  template< typename... _Args_ >
  std::enable_if_t< has_run_method< write, int, pipe&, const buffer&, _Args_... >::value, int >
  run_(pipe &_io, const buffer &_buf, _Args_&&... _args)
  { return reinterpret_cast< write* >(this)->run(_io, _buf, std::forward< _Args_ >(_args)...); }

  template< typename... _Args_ >
  std::enable_if_t< has_run_method< write, int, tcp&, const buffer&, _Args_... >::value, int >
  run_(tcp &_io, const buffer &_buf, _Args_&&... _args)
  { return reinterpret_cast< write* >(this)->run(_io, _buf, std::forward< _Args_ >(_args)...); }

  template< typename... _Args_ >
  std::enable_if_t< has_run_method< write, int, tty&, const buffer&, _Args_... >::value, int >
  run_(tty &_io, const buffer &_buf, _Args_&&... _args)
  { return reinterpret_cast< write* >(this)->run(_io, _buf, std::forward< _Args_ >(_args)...); }

  template< typename... _Args_ >
  std::enable_if_t< has_run_method< udp_send, int, udp&, const buffer&, _Args_... >::value, int >
  run_(udp &_io, const buffer &_buf, _Args_&&... _args)
  { return reinterpret_cast< udp_send* >(this)->run(_io, _buf, std::forward< _Args_ >(_args)...); }

  template< typename... _Args_ >
  std::enable_if_t< has_run_method< fs::write, int, file&, const buffer&, _Args_... >::value, int >
  run_(file &_io, const buffer &_buf, _Args_&&... _args)
  { return reinterpret_cast< fs::write* >(this)->run(_io, _buf, std::forward< _Args_ >(_args)...); }

  template< typename... _Args_ > int run_(_Args_&&...)  { return uv_status(UV_EINVAL); }


  template< typename... _Args_ >
  std::enable_if_t< has_try_write_method< write, int, stream&, const buffer&, _Args_... >::value, int >
  try_output_(stream &_io, const buffer &_buf, _Args_&&... _args)
  { return reinterpret_cast< write* >(this)->try_write(_io, _buf, std::forward< _Args_ >(_args)...); }

  template< typename... _Args_ >
  std::enable_if_t< has_try_send_method< udp_send, int, udp&, const buffer&, _Args_... >::value, int >
  try_output_(udp &_io, const buffer &_buf, _Args_&&... _args)
  { return reinterpret_cast< udp_send* >(this)->try_send(_io, _buf, std::forward< _Args_ >(_args)...); }

  template< typename... _Args_ >
  std::enable_if_t< has_try_write_method< fs::write, int, file&, const buffer&, _Args_... >::value, int >
  try_output_(file &_io, const buffer &_buf, _Args_&&... _args)
  { return reinterpret_cast< fs::write* >(this)->try_write(_io, _buf, std::forward< _Args_ >(_args)...); }

  template< typename... _Args_ > int try_output_(_Args_&&...)  { return uv_status(UV_EINVAL); }

public: /*constructors*/
  ~output() = default;
  output()  { uv_req = instance::create(); }

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

  int run(io &_io, const buffer &_buf, void *_info)
  {
    switch (_io.type())
    {
    case UV_NAMED_PIPE:
    case UV_TCP:
    case UV_TTY:
        return reinterpret_cast< write* >(this)->run(static_cast< stream& >(_io), _buf);
    case UV_UDP:
        return reinterpret_cast< udp_send* >(this)->run(static_cast< udp& >(_io), _buf, *static_cast< udp::io_info* >(_info)->peer);
    case UV_FILE:
        return reinterpret_cast< fs::write* >(this)->run(static_cast< file& >(_io), _buf, *static_cast< int64_t* >(_info));
    default:
        return uv_status(UV_EBADF);
    }
  }

  /*! \brief Run the request
      \details Depending on the actual object type of the `_io` argument the call should conform to the one of
      the signatures of the _request_`::run()` functions available for the corresponding write/send _request_
      for that object, i.e.:
      - `uv::fs::write::run()`, or
      - `uv::write::run()`, or
      - `uv::udp_send::run()`.
      .
      A proper _request_`::run()` function is deduced for any `io` subclasses and the corresponding _request_ at
      compile time. If that deduction fails, a substituted stub function returns `UV_EINVAL` status at run time.
      To prevent this the user code should take care that the arguments to be passed would match the
      _request_`::run()` available signatures according to the actual `_io` object type gotten at run time. */
  template< typename... _Args_ > int run(io &_io, const buffer &_buf, _Args_&&... _args)
  {
    switch (_io.type())
    {
    case UV_NAMED_PIPE:
        return run_(static_cast< pipe& >(_io), _buf, std::forward< _Args_ >(_args)...);
    case UV_TCP:
        return run_(static_cast< tcp& >(_io), _buf, std::forward< _Args_ >(_args)...);
    case UV_TTY:
        return run_(static_cast< tty& >(_io), _buf, std::forward< _Args_ >(_args)...);
    case UV_UDP:
        return run_(static_cast< udp& >(_io), _buf, std::forward< _Args_ >(_args)...);
    case UV_FILE:
        return run_(static_cast< file& >(_io), _buf, std::forward< _Args_ >(_args)...);
    default:
        return uv_status(UV_EBADF);
    }
  }

  /*! \brief Same as `run()`, but won’t queue an output request if it can’t be completed immediately.
      \details Depending on the actual run-time type of the `_io` argument, the function appears to be an alias
      to the one of the following functon:
      - `uv::fs::write::try_write()`, or
      - `uv::write::try_write()`, or
      - `uv::udp_send::try_send()`.
      .
      If a set of arguments to be passed does't match the signature of the corresponding function,
      a substituted stub returns `UV_EINVAL` status at run time. */
  template< typename... _Args_ > int try_output(io &_io, const buffer &_buf, _Args_&&... _args)
  {
    switch (_io.type())
    {
    case UV_NAMED_PIPE:
    case UV_TCP:
    case UV_TTY:
        return try_output_(static_cast< stream& >(_io), _buf, std::forward< _Args_ >(_args)...);
    case UV_UDP:
        return try_output_(static_cast< udp& >(_io), _buf, std::forward< _Args_ >(_args)...);
    case UV_FILE:
        return try_output_(static_cast< file& >(_io), _buf, std::forward< _Args_ >(_args)...);
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
