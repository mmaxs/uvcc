
#ifndef UVCC_HANDLE_MISC__HPP
#define UVCC_HANDLE_MISC__HPP

#include "uvcc/utility.hpp"
#include "uvcc/handle-base.hpp"
#include "uvcc/handle-io.hpp"

#include <cstdint>      // uint64_t
#include <uv.h>

#include <functional>   // function bind placeholders::
#include <vector>


namespace uv
{


/*! \ingroup doxy_group__handle
    \brief Async handle.
    \sa libuv API documentation: [`uv_async_t — Async handle`](http://docs.libuv.org/en/v1.x/async.html#uv-async-t-async-handle). */
class async : public handle
{
  //! \cond
  friend class handle::uv_interface;
  friend class handle::instance< async >;
  //! \endcond

public: /*types*/
  using uv_t = ::uv_async_t;
  template< typename... _Args_ >
  using on_send_t = std::function< void(async _handle, _Args_&&... _args) >;
  /*!< \brief The function type of the callback called by the `async` event.
       \note
        1. All the additional templated arguments are passed and stored by value along with the function object
           when the `async` handle variable is created.
        2. The `async` event is not a facility for executing a given callback function on the target loop on every `async::send()` call.
       \sa libuv API documentation: [`uv_async_cb`](http://docs.libuv.org/en/v1.x/async.html#c.uv_async_cb),
                                    [`uv_async_send()`](http://docs.libuv.org/en/v1.x/async.html#c.uv_async_send). */

protected: /*types*/
  //! \cond internals
  //! \addtogroup doxy_group__internals
  //! \{

  struct properties : handle::properties
  {
    std::function< void(async) > cb;
  };

  struct uv_interface : handle::uv_handle_interface  {};

  //! \}
  //! \endcond

private: /*types*/
  using instance = handle::instance< async >;

private: /*functions*/
  template < typename = void > static void async_cb(::uv_async_t*);

protected: /*constructors*/
  //! \cond
  explicit async(uv_t *_uv_handle) : handle(reinterpret_cast< handle::uv_t* >(_uv_handle))  {}
  //! \endcond

public: /*constructors*/
  ~async() = default;

  async(const async&) = default;
  async& operator =(const async&) = default;

  async(async&&) noexcept = default;
  async& operator =(async&&) noexcept = default;

  /*! \brief Create an `async` handle with no callback function. */
  explicit async(uv::loop &_loop)
  {
    uv_handle = instance::create();

    auto uv_ret = ::uv_async_init(static_cast< uv::loop::uv_t* >(_loop), static_cast< uv_t* >(uv_handle), nullptr);
    if (uv_status(uv_ret) < 0)  return;

    instance::from(uv_handle)->book_loop();
  }

  /*! \brief Create an `async` handle with specified callback function.
      \note All arguments are copied (or moved) to the internal callback function object. For passing arguments by reference
      (when parameters are used as output ones), wrap them with `std::ref()` or use raw pointers.
      \sa libuv API documentation: [`uv_async_init()`](http://docs.libuv.org/en/v1.x/async.html#c.uv_async_init). */
  template< class _Cb_, typename... _Args_,
      typename = std::enable_if_t< std::is_convertible< _Cb_, on_send_t< _Args_&&... > >::value >
  >
  async(uv::loop &_loop, _Cb_ &&_cb, _Args_&&... _args)
  {
    uv_handle = instance::create();

    auto uv_ret = ::uv_async_init(static_cast< uv::loop::uv_t* >(_loop), static_cast< uv_t* >(uv_handle), async_cb);
    if (uv_status(uv_ret) < 0)  return;

    instance::from(uv_handle)->properties().cb = std::bind(
        std::forward< _Cb_ >(_cb), std::placeholders::_1, std::forward< _Args_ >(_args)...
    );
    instance::from(uv_handle)->book_loop();
  }

public: /*interface*/
  /*! \brief Wakeup the event loop and call the `async` handle’s callback.
      \sa libuv API documentation: [`uv_async_send()`](http://docs.libuv.org/en/v1.x/async.html#c.uv_async_send). */
  int send() const noexcept
  {
    return uv_status(
        ::uv_async_send(static_cast< uv_t* >(uv_handle))
    );
  }

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_handle); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_handle); }
};

template< typename >
void async::async_cb(::uv_async_t *_uv_handle)
{
  auto &cb = instance::from(_uv_handle)->properties().cb;
  if (cb)  cb(async(_uv_handle));
}



/*! \ingroup doxy_group__handle
    \brief Timer handle.
    \sa libuv API documentation: [`uv_timer_t — Timer handle`](http://docs.libuv.org/en/v1.x/timer.html#uv-timer-t-timer-handle). */
class timer : public handle
{
  //! \cond
  friend class handle::uv_interface;
  friend class handle::instance< timer >;
  //! \endcond

public: /*types*/
  using uv_t = ::uv_timer_t;
  template< typename... _Args_ >
  using on_timer_t = std::function< void(timer _handle, _Args_&&... _args) >;
  /*!< \brief The function type of the callback called by the timer scheduled with `timer::start()` function.
       \note All the additional templated arguments are passed and stored by value along with the function object
       when the `timer` handle is started with `timer::start()` functions.
       \sa libuv API documentation: [`uv_timer_cb`](http://docs.libuv.org/en/v1.x/timer.html#c.uv_timer_cb). */

protected: /*types*/
  //! \cond internals
  //! \addtogroup doxy_group__internals
  //! \{

  struct properties : handle::properties
  {
    std::function< void(timer) > cb;
  };

  struct uv_interface : handle::uv_handle_interface  {};

  //! \}
  //! \endcond

private: /*types*/
  using instance = handle::instance< timer >;

private: /*functions*/
  template < typename = void > static void timer_cb(::uv_timer_t*);

protected: /*constructors*/
  //! \cond
  explicit timer(uv_t *_uv_handle) : handle(reinterpret_cast< handle::uv_t* >(_uv_handle))  {}
  //! \endcond

public: /*constructors*/
  ~timer() = default;

  timer(const timer&) = default;
  timer& operator =(const timer&) = default;

  timer(timer&&) noexcept = default;
  timer& operator =(timer&&) noexcept = default;

  /*! \brief Create a `timer` handle. */
  timer(uv::loop &_loop)
  {
    uv_handle = instance::create();

    auto uv_ret = ::uv_timer_init(static_cast< uv::loop::uv_t* >(_loop), static_cast< uv_t* >(uv_handle));
    if (uv_status(uv_ret) < 0)  return;

    instance::from(uv_handle)->book_loop();
  }

public: /*interface*/
  /*! \brief Start the timer and schedule the specified callback function.
      \details `_timeout` and `_repeat` parameters are in milliseconds.
      \note All arguments are copied (or moved) to the internal callback function object. For passing arguments by reference
      (when parameters are used as output ones), wrap them with `std::ref()` or use raw pointers.
      \sa libuv API documentation: [`uv_timer_start()`](http://docs.libuv.org/en/v1.x/timer.html#c.uv_timer_start). */
  template< class _Cb_, typename... _Args_,
      typename = std::enable_if_t< std::is_convertible< _Cb_, on_timer_t< _Args_&&... > >::value >
  >
  int start(uint64_t _timeout_value, uint64_t _repeat_value, _Cb_ &&_cb, _Args_&&... _args) const
  {
    instance::from(uv_handle)->properties().cb = std::bind(
        std::forward< _Cb_ >(_cb), std::placeholders::_1, std::forward< _Args_ >(_args)...
    );

    return uv_status(
        ::uv_timer_start(static_cast< uv_t* >(uv_handle), timer_cb, _timeout_value, _repeat_value)
    );
  }

  /*! \brief Stop the timer, the callback will not be called anymore. */
  int stop() const noexcept
  {
    return uv_status(
        ::uv_timer_stop(static_cast< uv_t* >(uv_handle))
    );
  }

  /*! \brief Stop the timer, and if it is repeating reschedule it using its repeat value as the timeout.
      \details If the timer has never been started before it returns `UV_EINVAL`. */
  int again() const noexcept
  {
    return uv_status(
        ::uv_timer_again(static_cast< uv_t* >(uv_handle))
    );
  }

  /*! \brief _Get_ the timer repeat value. */
  uint64_t repeat_value() const noexcept  { return ::uv_timer_get_repeat(static_cast< uv_t* >(uv_handle)); }
  /*! \brief _Set_ the repeat interval value in milliseconds.
      \note Setting the repeat value to zero turns the timer to be non-repeating.
      \sa [`uv_timer_set_repeat()`](http://docs.libuv.org/en/v1.x/timer.html#c.uv_timer_set_repeat). */
  void repeat_value(uint64_t _interval) noexcept  { ::uv_timer_set_repeat(static_cast< uv_t* >(uv_handle), _interval); }

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_handle); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_handle); }
};

template< typename >
void timer::timer_cb(::uv_timer_t *_uv_handle)
{
  auto &cb = instance::from(_uv_handle)->properties().cb;
  if (cb)  cb(timer(_uv_handle));
}



/*! \defgroup doxy_group__idle_prepare_check  idle, prepare, check
    \ingroup doxy_group__handle
    \brief `uv::idle`, `uv::prepare`, and `uv::check` handles. */
//! \{

/*! \brief Idle handle.
    \sa libuv API documentation: [`uv_idle_t — Idle handle`](http://docs.libuv.org/en/v1.x/idle.html#uv-idle-t-idle-handle). */
class idle : public handle
{
  //! \cond
  friend class handle::uv_interface;
  friend class handle::instance< idle >;
  //! \endcond

public: /*types*/
  using uv_t = ::uv_idle_t;
  template< typename... _Args_ >
  using on_idle_t = std::function< void(idle _handle, _Args_&&... _args) >;
  /*!< \brief The function type of the callback being set with `idle::start()` function.
       \sa libuv API documentation: [`uv_idle_cb`](http://docs.libuv.org/en/v1.x/idle.html#c.uv_idle_cb). */

protected: /*types*/
  //! \cond internals
  //! \addtogroup doxy_group__internals
  //! \{

  struct properties : handle::properties
  {
    std::function< void(idle) > cb;
  };

  struct uv_interface : handle::uv_handle_interface  {};

  //! \}
  //! \endcond

private: /*types*/
  using instance = handle::instance< idle >;

private: /*functions*/
  template < typename = void > static void idle_cb(::uv_idle_t*);

protected: /*constructors*/
  //! \cond
  explicit idle(uv_t *_uv_handle) : handle(reinterpret_cast< handle::uv_t* >(_uv_handle))  {}
  //! \endcond

public: /*constructors*/
  ~idle() = default;

  idle(const idle&) = default;
  idle& operator =(const idle&) = default;

  idle(idle&&) noexcept = default;
  idle& operator =(idle&&) noexcept = default;

  /*! \brief Create an `idle` handle. */
  idle(uv::loop &_loop)
  {
    uv_handle = instance::create();

    auto uv_ret = ::uv_idle_init(static_cast< uv::loop::uv_t* >(_loop), static_cast< uv_t* >(uv_handle));
    if (uv_status(uv_ret) < 0)  return;

    instance::from(uv_handle)->book_loop();
  }

public: /*interface*/
  /*! \brief Start the handle with the given callback.
      \note All arguments are copied (or moved) to the internal callback function object. For passing arguments by reference
      (when parameters are used as output ones), wrap them with `std::ref()` or use raw pointers. */
  template< class _Cb_, typename... _Args_,
      typename = std::enable_if_t< std::is_convertible< _Cb_, on_idle_t< _Args_&&... > >::value >
  >
  int start(_Cb_ &&_cb, _Args_&&... _args) const
  {
    instance::from(uv_handle)->properties().cb = std::bind(
        std::forward< _Cb_ >(_cb), std::placeholders::_1, std::forward< _Args_ >(_args)...
    );

    return uv_status(
        ::uv_idle_start(static_cast< uv_t* >(uv_handle), idle_cb)
    );
  }

  /*! \brief Stop the handle, the callback will no longer be called. */
  int stop() const noexcept
  {
    return uv_status(
        ::uv_idle_stop(static_cast< uv_t* >(uv_handle))
    );
  }

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_handle); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_handle); }
};

template< typename >
void idle::idle_cb(::uv_idle_t *_uv_handle)
{
  auto &cb = instance::from(_uv_handle)->properties().cb;
  if (cb)  cb(idle(_uv_handle));
}


/*! \brief Prepare handle.
    \sa libuv API documentation: [`uv_prepare_t — Prepare handle`](http://docs.libuv.org/en/v1.x/prepare.html#uv-prepare-t-prepare-handle). */
class prepare : public handle
{
  //! \cond
  friend class handle::uv_interface;
  friend class handle::instance< prepare >;
  //! \endcond

public: /*types*/
  using uv_t = ::uv_prepare_t;
  template< typename... _Args_ >
  using on_prepare_t = std::function< void(prepare _handle, _Args_&&... _args) >;
  /*!< \brief The function type of the callback being set with `prepare::start()` function.
       \sa libuv API documentation: [`uv_prepare_cb`](http://docs.libuv.org/en/v1.x/prepare.html#c.uv_prepare_cb). */

protected: /*types*/
  //! \cond internals
  //! \addtogroup doxy_group__internals
  //! \{

  struct properties : handle::properties
  {
    std::function< void(prepare) > cb;
  };

  struct uv_interface : handle::uv_handle_interface  {};

  //! \}
  //! \endcond

private: /*types*/
  using instance = handle::instance< prepare >;

private: /*functions*/
  template < typename = void > static void prepare_cb(::uv_prepare_t*);

protected: /*constructors*/
  //! \cond
  explicit prepare(uv_t *_uv_handle) : handle(reinterpret_cast< handle::uv_t* >(_uv_handle))  {}
  //! \endcond

public: /*constructors*/
  ~prepare() = default;

  prepare(const prepare&) = default;
  prepare& operator =(const prepare&) = default;

  prepare(prepare&&) noexcept = default;
  prepare& operator =(prepare&&) noexcept = default;

  /*! \brief Create an `prepare` handle. */
  prepare(uv::loop &_loop)
  {
    uv_handle = instance::create();

    auto uv_ret = ::uv_prepare_init(static_cast< uv::loop::uv_t* >(_loop), static_cast< uv_t* >(uv_handle));
    if (uv_status(uv_ret) < 0)  return;

    instance::from(uv_handle)->book_loop();
  }

public: /*interface*/
  /*! \brief Start the handle with the given callback.
      \note All arguments are copied (or moved) to the internal callback function object. For passing arguments by reference
      (when parameters are used as output ones), wrap them with `std::ref()` or use raw pointers. */
  template< class _Cb_, typename... _Args_,
      typename = std::enable_if_t< std::is_convertible< _Cb_, on_prepare_t< _Args_&&... > >::value >
  >
  int start(_Cb_ &&_cb, _Args_&&... _args) const
  {
    instance::from(uv_handle)->properties().cb = std::bind(
        std::forward< _Cb_ >(_cb), std::placeholders::_1, std::forward< _Args_ >(_args)...
    );

    return uv_status(
        ::uv_prepare_start(static_cast< uv_t* >(uv_handle), prepare_cb)
    );
  }

  /*! \brief Stop the handle, the callback will no longer be called. */
  int stop() const noexcept
  {
    return uv_status(
        ::uv_prepare_stop(static_cast< uv_t* >(uv_handle))
    );
  }

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_handle); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_handle); }
};

template< typename >
void prepare::prepare_cb(::uv_prepare_t *_uv_handle)
{
  auto &cb = instance::from(_uv_handle)->properties().cb;
  if (cb)  cb(prepare(_uv_handle));
}


/*! \brief Check handle.
    \sa libuv API documentation: [`uv_check_t — Check handle`](http://docs.libuv.org/en/v1.x/check.html#uv-check-t-check-handle). */
class check : public handle
{
  //! \cond
  friend class handle::uv_interface;
  friend class handle::instance< check >;
  //! \endcond

public: /*types*/
  using uv_t = ::uv_check_t;
  template< typename... _Args_ >
  using on_check_t = std::function< void(check _handle, _Args_&&... _args) >;
  /*!< \brief The function type of the callback being set with `check::start()` function.
       \sa libuv API documentation: [`uv_check_cb`](http://docs.libuv.org/en/v1.x/check.html#c.uv_check_cb). */

protected: /*types*/
  //! \cond internals
  //! \addtogroup doxy_group__internals
  //! \{

  struct properties : handle::properties
  {
    std::function< void(check) > cb;
  };

  struct uv_interface : handle::uv_handle_interface  {};

  //! \}
  //! \endcond

private: /*types*/
  using instance = handle::instance< check >;

private: /*functions*/
  template < typename = void > static void check_cb(::uv_check_t*);

protected: /*constructors*/
  //! \cond
  explicit check(uv_t *_uv_handle) : handle(reinterpret_cast< handle::uv_t* >(_uv_handle))  {}
  //! \endcond

public: /*constructors*/
  ~check() = default;

  check(const check&) = default;
  check& operator =(const check&) = default;

  check(check&&) noexcept = default;
  check& operator =(check&&) noexcept = default;

  /*! \brief Create an `check` handle. */
  check(uv::loop &_loop)
  {
    uv_handle = instance::create();

    auto uv_ret = ::uv_check_init(static_cast< uv::loop::uv_t* >(_loop), static_cast< uv_t* >(uv_handle));
    if (uv_status(uv_ret) < 0)  return;

    instance::from(uv_handle)->book_loop();
  }

public: /*interface*/
  /*! \brief Start the handle with the given callback.
      \note All arguments are copied (or moved) to the internal callback function object. For passing arguments by reference
      (when parameters are used as output ones), wrap them with `std::ref()` or use raw pointers. */
  template< class _Cb_, typename... _Args_,
      typename = std::enable_if_t< std::is_convertible< _Cb_, on_check_t< _Args_&&... > >::value >
  >
  int start(_Cb_ &&_cb, _Args_&&... _args) const
  {
    instance::from(uv_handle)->properties().cb = std::bind(
        std::forward< _Cb_ >(_cb), std::placeholders::_1, std::forward< _Args_ >(_args)...
    );

    return uv_status(
        ::uv_check_start(static_cast< uv_t* >(uv_handle), check_cb)
    );
  }

  /*! \brief Stop the handle, the callback will no longer be called. */
  int stop() const noexcept
  {
    return uv_status(
        ::uv_check_stop(static_cast< uv_t* >(uv_handle))
    );
  }

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_handle); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_handle); }
};

template< typename >
void check::check_cb(::uv_check_t *_uv_handle)
{
  auto &cb = instance::from(_uv_handle)->properties().cb;
  if (cb)  cb(check(_uv_handle));
}

//! \}



/*! \ingroup doxy_group__handle
    \brief Signal handle.
    \sa libuv API documentation: [`uv_signal_t — Signal handle`](http://docs.libuv.org/en/v1.x/signal.html#uv-signal-t-signal-handle). */
class signal : public handle
{
  //! \cond
  friend class handle::uv_interface;
  friend class handle::instance< signal >;
  //! \endcond

public: /*types*/
  using uv_t = ::uv_signal_t;
  template< typename... _Args_ >
  using on_signal_t = std::function< void(signal _handle, _Args_&&... _args) >;
  /*!< \brief The function type of the `signal` handler.
       \note All the additional templated arguments are passed and stored by value along with the function object
       when the `signal` handle is started with `signal::start()`/`signal::start_oneshot()` functions.
       \sa libuv API documentation: [`uv_signal_cb`](http://docs.libuv.org/en/v1.x/signal.html#c.uv_signal_cb). */

protected: /*types*/
  //! \cond internals
  //! \addtogroup doxy_group__internals
  //! \{

  struct properties : handle::properties
  {
    std::function< void(signal) > cb;
  };

  struct uv_interface : handle::uv_handle_interface  {};

  //! \}
  //! \endcond

private: /*types*/
  using instance = handle::instance< signal >;

private: /*functions*/
  template < typename = void > static void signal_cb(::uv_signal_t*, int);

protected: /*constructors*/
  //! \cond
  explicit signal(uv_t *_uv_handle) : handle(reinterpret_cast< handle::uv_t* >(_uv_handle))  {}
  //! \endcond

public: /*constructors*/
  ~signal() = default;

  signal(const signal&) = default;
  signal& operator =(const signal&) = default;

  signal(signal&&) noexcept = default;
  signal& operator =(signal&&) noexcept = default;

  /*! \brief Create a `signal` handle. */
  signal(uv::loop &_loop)
  {
    uv_handle = instance::create();

    auto uv_ret = ::uv_signal_init(static_cast< uv::loop::uv_t* >(_loop), static_cast< uv_t* >(uv_handle));
    if (uv_status(uv_ret) < 0)  return;

    instance::from(uv_handle)->book_loop();
  }

public: /*interface*/
  /*! \brief Get the signal number being monitored by this handle. */
  int signum() const noexcept  { return static_cast< uv_t* >(uv_handle)->signum; }

  /*! \brief Start the handle with the given callback, watching for the given signal.
      \note All arguments are copied (or moved) to the internal callback function object. For passing arguments by reference
      (when parameters are used as output ones), wrap them with `std::ref()` or use raw pointers. */
  template< class _Cb_, typename... _Args_,
      typename = std::enable_if_t< std::is_convertible< _Cb_, on_signal_t< _Args_&&... > >::value >
  >
  int start(int _signum, _Cb_ &&_cb, _Args_&&... _args) const
  {
    instance::from(uv_handle)->properties().cb = std::bind(
        std::forward< _Cb_ >(_cb), std::placeholders::_1, std::forward< _Args_ >(_args)...
    );

    return uv_status(
        ::uv_signal_start(static_cast< uv_t* >(uv_handle), signal_cb, _signum)
    );
  }
#if (UV_VERSION_MAJOR >= 1) && (UV_VERSION_MINOR >= 12)
  /*! \brief Start the handle with the given callback, watching for the given signal.
      \details The signal handler is reset the moment the signal is received.
      \note All arguments are copied (or moved) to the internal callback function object. For passing arguments by reference
      (when parameters are used as output ones), wrap them with `std::ref()` or use raw pointers. */
  template< class _Cb_, typename... _Args_,
      typename = std::enable_if_t< std::is_convertible< _Cb_, on_signal_t< _Args_&&... > >::value >
  >
  int start_oneshot(int _signum, _Cb_ &&_cb, _Args_&&... _args) const
  {
    instance::from(uv_handle)->properties().cb = std::bind(
        std::forward< _Cb_ >(_cb), std::placeholders::_1, std::forward< _Args_ >(_args)...
    );

    return uv_status(
        ::uv_signal_start_oneshot(static_cast< uv_t* >(uv_handle), signal_cb, _signum)
    );
  }
#endif

  /*! \brief Stop the handle, the callback will no longer be called. */
  int stop() const noexcept
  {
    return uv_status(
        ::uv_signal_stop(static_cast< uv_t* >(uv_handle))
    );
  }

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_handle); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_handle); }
};

template< typename >
void signal::signal_cb(::uv_signal_t *_uv_handle, int)
{
  auto &cb = instance::from(_uv_handle)->properties().cb;
  if (cb)  cb(signal(_uv_handle));
}



/*! \ingroup doxy_group__handle
    \brief Process handle.
    \sa libuv API documentation: [`uv_process_t — Process handle`](http://docs.libuv.org/en/v1.x/process.html#uv-process-t-process-handle). */
class process : public handle
{
  //! \cond
  friend class handle::uv_interface;
  friend class handle::instance< process >;
  //! \endcond

public: /*types*/
  using uv_t = ::uv_process_t;
  using on_exit_t = std::function< void(process _handle, int64_t _exit_status, int _termination_signal) >;
  /*!< \brief The function type of the callback called when the child process exits.
       \sa libuv API documentation: [`uv_exit_cb`](http://docs.libuv.org/en/v1.x/process.html#c.uv_exit_cb). */

protected: /*types*/
  //! \cond internals
  //! \addtogroup doxy_group__internals
  //! \{

  struct properties : handle::properties
  {
    /*types*/
    struct stdio_container : ::uv_stdio_container_t
    {
      static_assert(sizeof(data.stream) == sizeof(io), "incompatible type sizes");

      ~stdio_container()  { reinterpret_cast< io& >(data.stream).~io(); }
      stdio_container()
      {
        flags = UV_IGNORE;
        new(&data.stream) io;
      }
    };

    /*data*/
    ::uv_process_options_t spawn_options = { 0,};
    on_exit_t exit_cb;
    std::vector< stdio_container > stdios;

    /*constructors*/
    properties()
    {
      spawn_options.exit_cb = process::exit_cb;
    }

    /*interface*/
    void prepare_stdio_container(unsigned _number)
    {
      if (_number >= stdios.size())
      {
        stdios.resize(_number + 1);
        spawn_options.stdio = stdios.data();
        spawn_options.stdio_count = stdios.size();
      }
    }
  };

  struct uv_interface : handle::uv_handle_interface  {};

  //! \}
  //! \endcond

private: /*types*/
  using instance = handle::instance< process >;

private: /*functions*/
  template < typename = void > static void exit_cb(::uv_process_t*, int64_t, int);

protected: /*constructors*/
  //! \cond
  explicit process(uv_t *_uv_handle) : handle(reinterpret_cast< handle::uv_t* >(_uv_handle))  {}
  //! \endcond

public: /*constructors*/
  ~process() = default;

  process(const process&) = default;
  process& operator =(const process&) = default;

  process(process&&) noexcept = default;
  process& operator =(process&&) noexcept = default;

  /*! \brief Create a `process` handle. */
  process(uv::loop &_loop)
  {
    uv_handle = instance::create();
    static_cast< uv_t* >(uv_handle)->loop = static_cast< loop::uv_t* >(_loop);
    instance::from(uv_handle)->book_loop();
  }

public: /*interface*/
  /*! \brief The PID of the spawned process. It’s set after calling `spawn()`. */
  int pid() const noexcept  { return static_cast< uv_t* >(uv_handle)->pid; }

  /*! \brief Set the callback function called when the spawned process exits. */
  on_exit_t& on_exit() const noexcept  { return instance::from(uv_handle)->properties().exit_cb; }

  /*! \name Functions for specifying how stdio streams should be transmitted to the child process: */
  //! \{
  /*! \brief Set flags.
      \details `UV_INHERIT_FD` `UV_INHERIT_STREAM` `UVCC_IPC_PIPE`
      \sa libuv API documentation: [`uv_stdio_flags`](http://docs.libuv.org/en/v1.x/process.html#c.uv_stdio_flags). */
  void prepare_stdio_stream(unsigned _number, ::uv_stdio_flags _flags)  const
  {
    auto &properties = instance::from(uv_handle)->properties();
    properties.prepare_stdio_container(_number);
    properties.stdios[_number].flags = _flags;
  }
  /*! \brief Set the stream. */
  io& stdio_stream(unsigned _number) const
  {
    auto &properties = instance::from(uv_handle)->properties();
    properties.prepare_stdio_container(_number);
    return reinterpret_cast< io& >(properties.stdios[_number].data.stream);
  }
  //! \}

  /*! \brief Create and start a new child process.
      \sa libuv API documentation: [`uv_spawn()`](http://docs.libuv.org/en/v1.x/process.html#c.uv_spawn). */
  int spawn() const noexcept
  {
    auto &properties = instance::from(uv_handle)->properties();

    for (auto &stdio_container : properties.stdios)
    {
      auto &stdio_stream = reinterpret_cast< io& >(stdio_container.data.stream);
      if (stdio_container.flags & UV_CREATE_PIPE)
      {
      }
      else if (stdio_container.flags & UV_INHERIT_FD)
      {
        if (stdio_stream.id() and stdio_stream.type() != UV_FILE)
        {
          int flags = stdio_container.flags;
          flags ^= UV_INHERIT_FD;
          flags |= UV_INHERIT_STREAM;
          stdio_container.flags = static_cast< decltype(stdio_container.flags) >(flags);
        }
        // XXX: it must be turned to fd !
      }
      else if (stdio_container.flags & UV_INHERIT_STREAM)
      {
      }
      else
      {
        stdio_container.flags = UV_IGNORE;
      }
    }

    return uv_status(
        ::uv_spawn(static_cast< uv_t* >(uv_handle)->loop, static_cast< uv_t* >(uv_handle), &properties.spawn_options)
    );
  }
  int spawn(::uv_process_flags _flags) const noexcept
  {
    auto &spawn_options = instance::from(uv_handle)->properties().spawn_options;
    spawn_options.flags |= _flags;
    return spawn();
  }

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_handle); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_handle); }
};

template< typename >
void process::exit_cb(::uv_process_t* _uv_handle, int64_t _exit_status, int _termination_signal)
{
  auto &exit_cb = instance::from(_uv_handle)->properties().exit_cb;
  if (exit_cb)  exit_cb(process(_uv_handle), _exit_status, _termination_signal);
}


}


#endif
