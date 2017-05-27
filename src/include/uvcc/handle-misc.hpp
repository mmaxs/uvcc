
#ifndef UVCC_HANDLE_MISC__HPP
#define UVCC_HANDLE_MISC__HPP

#include "uvcc/utility.hpp"
#include "uvcc/handle-base.hpp"
#include "uvcc/handle-io.hpp"

#include <cstdint>      // uint64_t
#include <uv.h>

#include <functional>   // function bind placeholders::
#include <utility>      // forward() declval()
#include <vector>       // vector
#include <stdexcept>    // invalid_argument


namespace uv
{


/*! \ingroup doxy_group__handle
    \brief Async handle.
    \sa libuv API documentation: [`uv_async_t` — Async handle](http://docs.libuv.org/en/v1.x/async.html#uv-async-t-async-handle). */
class async : public handle
{
  //! \cond
  friend class handle::uv_interface;
  friend class handle::instance< async >;
  //! \endcond

public: /*types*/
  using uv_t = ::uv_async_t;
  using on_send_t = std::function< void(async _handle) >;
  /*!< \brief The function type of the callback called on the event raised by `async::send()` function.
       \note The `async` event is not a facility for executing the given callback function
       on the target loop on every `async::send()` call.
       \sa libuv API documentation: [`uv_async_cb`](http://docs.libuv.org/en/v1.x/async.html#c.uv_async_cb),
                                    [`uv_async_send()`](http://docs.libuv.org/en/v1.x/async.html#c.uv_async_send). */

protected: /*types*/
  //! \cond internals
  //! \addtogroup doxy_group__internals
  //! \{

  struct properties : handle::properties
  {
    on_send_t async_cb;
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

  /*! \brief Create an `async` handle.
      \sa libuv API documentation: [`uv_async_init()`](http://docs.libuv.org/en/v1.x/async.html#c.uv_async_init). */
  explicit async(uv::loop &_loop)
  {
    uv_handle = instance::create();

    auto uv_ret = ::uv_async_init(static_cast< uv::loop::uv_t* >(_loop), static_cast< uv_t* >(uv_handle), async_cb);
    if (uv_status(uv_ret) < 0)  return;

    instance::from(uv_handle)->book_loop();
  }

public: /*interface*/
  /*! \brief Set the `async` event callback. */
  on_send_t& on_send() const noexcept  { return instance::from(uv_handle)->properties().async_cb; }

  /*! \brief Wakeup the event loop and call the `async` callback.
      \sa libuv API documentation: [`uv_async_send()`](http://docs.libuv.org/en/v1.x/async.html#c.uv_async_send). */
  int send() const
  {
    auto instance_ptr = instance::from(uv_handle);

    instance_ptr->ref();  // the reference must be added before async event sending and possible executing
                          // of the callback in another thread that will release this reference

    uv_status(0);
    auto uv_ret = ::uv_async_send(static_cast< uv_t* >(uv_handle));
    if (uv_ret < 0)
    {
      uv_status(uv_ret);
      instance_ptr->unref();
    }

    return uv_ret;
  }

  /*! \brief Set the given `async` callback and send wakeup event to the target loop.
      \details This is equivalent for
      ```
      async.on_send() = std::bind(std::forward< _Cb_ >(_cb), std::placeholders::_1, std::forward< _Args_ >(_args)...);
      async.send();
      ```
      \sa `async::send()` */
  template< class _Cb_, typename... _Args_, typename = std::enable_if_t<
      std::is_convertible<
          decltype(std::bind(std::declval< _Cb_ >(), std::placeholders::_1, static_cast< _Args_&& >(std::declval< _Args_ >())...)),
          on_send_t
      >::value
  > >
  int send(_Cb_ &&_cb, _Args_&&... _args) const
  {
    instance::from(uv_handle)->properties().async_cb = std::bind(
        std::forward< _Cb_ >(_cb), std::placeholders::_1, std::forward< _Args_ >(_args)...
    );
    return send();
  }

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_handle); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_handle); }
};

template< typename >
void async::async_cb(::uv_async_t *_uv_handle)
{
  auto instance_ptr = instance::from(_uv_handle);
  auto &async_cb = instance_ptr->properties().async_cb;

  ref_guard< instance > unref_handle(*instance_ptr, adopt_ref);

  if (async_cb)  async_cb(async(_uv_handle));
}



/*! \ingroup doxy_group__handle
    \brief Timer handle.
    \sa libuv API documentation: [`uv_timer_t` — Timer handle](http://docs.libuv.org/en/v1.x/timer.html#uv-timer-t-timer-handle). */
class timer : public handle
{
  //! \cond
  friend class handle::uv_interface;
  friend class handle::instance< timer >;
  //! \endcond

public: /*types*/
  using uv_t = ::uv_timer_t;
  using on_timer_t = std::function< void(timer _handle) >;
  /*!< \brief The function type of the callback called by the timer event. */

protected: /*types*/
  //! \cond internals
  //! \addtogroup doxy_group__internals
  //! \{

  struct properties : handle::properties
  {
    bool has_extra_ref = false;
    on_timer_t timer_cb;
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

  /*! \brief Create a `timer` handle which has a given repeat interval.
      \details `_repeat_interval` is in milliseconds. */
  explicit timer(uv::loop &_loop, uint64_t _repeat_interval = 0)
  {
    uv_handle = instance::create();

    auto uv_ret = ::uv_timer_init(static_cast< uv::loop::uv_t* >(_loop), static_cast< uv_t* >(uv_handle));
    if (uv_status(uv_ret) < 0)  return;

    ::uv_timer_set_repeat(static_cast< uv_t* >(uv_handle), _repeat_interval);

    instance::from(uv_handle)->book_loop();
  }

public: /*interface*/
  /*! \brief _Get_ the timer repeat interval in milliseconds. */
  uint64_t repeat_interval() const noexcept  { return ::uv_timer_get_repeat(static_cast< uv_t* >(uv_handle)); }
  /*! \brief _Set_ the timer repeat interval in milliseconds.
      \note Setting the repeat value to zero turns the timer to be non-repeating.
      \sa [`uv_timer_set_repeat()`](http://docs.libuv.org/en/v1.x/timer.html#c.uv_timer_set_repeat). */
  void repeat_interval(uint64_t _value) noexcept  { ::uv_timer_set_repeat(static_cast< uv_t* >(uv_handle), _value); }

  /*! \brief Set the timer callback. */
  on_timer_t& on_timer() const noexcept  { return instance::from(uv_handle)->properties().timer_cb; }

  /*! \brief Start the timer by scheduling a timer event after the `_timeout` interval expires.
      \details The timer callback function will be called after `_timeout` milliseconds or, if it is equal to **0**,
      on the next event loop iteration and then repeatedly after each `repeat_interval()`, if the latter is non-zero.

      Repeated call to this function results in the automatic call to `stop()` first.
      \note On successful start this function adds an extra reference to the handle instance,
      which is automatically released when the timer stops because of being non-repeating or
      the counterpart function `stop()` is called.
      \sa libuv API documentation: [`uv_timer_start()`](http://docs.libuv.org/en/v1.x/timer.html#c.uv_timer_start). */
  int start(uint64_t _timeout) const
  {
    auto instance_ptr = instance::from(uv_handle);
    auto &properties = instance_ptr->properties();

    if (!properties.has_extra_ref)
    {
      instance_ptr->ref();
      properties.has_extra_ref = true;
    }

    uv_status(0);
    auto uv_ret = ::uv_timer_start(static_cast< uv_t* >(uv_handle), timer_cb,
        _timeout, ::uv_timer_get_repeat(static_cast< uv_t* >(uv_handle))
    );
    if (uv_ret < 0)
    {
      uv_status(uv_ret);
      properties.has_extra_ref = false;
      instance_ptr->unref();
    }

    return uv_ret;
  }

  /*! \brief Start the timer with the given callback.
      \details This is equivalent for
      ```
      timer.on_timer() = std::bind(
          std::forward< _Cb_ >(_cb), std::placeholders::_1, std::forward< _Args_ >(_args)...
      );
      timer.start(_timeout);
      ```
      \sa `timer::start()` */
  template< class _Cb_, typename... _Args_, typename = std::enable_if_t<
      std::is_convertible<
          decltype(std::bind(std::declval< _Cb_ >(), std::placeholders::_1, static_cast< _Args_&& >(std::declval< _Args_ >())...)),
          on_timer_t
      >::value
  > >
  int start(uint64_t _timeout, _Cb_ &&_cb, _Args_&&... _args) const
  {
    instance::from(uv_handle)->properties().timer_cb = std::bind(
        std::forward< _Cb_ >(_cb), std::placeholders::_1, std::forward< _Args_ >(_args)...
    );

    return start(_timeout);
  }

  /*! \brief Stop the timer, the callback will not be called anymore. */
  int stop() const noexcept
  {
    auto instance_ptr = instance::from(uv_handle);
    auto &properties = instance_ptr->properties();

    auto uv_ret = uv_status(::uv_timer_stop(static_cast< uv_t* >(uv_handle)));

    if (properties.has_extra_ref)
    {
      properties.has_extra_ref = false;
      instance_ptr->unref();
    }

    return uv_ret;
  }

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_handle); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_handle); }
};

template< typename >
void timer::timer_cb(::uv_timer_t *_uv_handle)
{
  auto instance_ptr = instance::from(_uv_handle); 
  auto &properties = instance_ptr->properties();

#if 0
  uvcc_debug_log_if(true, "timer [0x%08tX]: repeat_interval=%llu is_active=%i\n",
    (ptrdiff_t)_uv_handle, ::uv_timer_get_repeat(_uv_handle), ::uv_is_active((::uv_handle_t*)_uv_handle)
  );
#endif

  if (::uv_timer_get_repeat(_uv_handle) == 0 and properties.has_extra_ref)
  {
    ref_guard< instance > unref_handle(*instance_ptr, adopt_ref);
    if (properties.timer_cb)  properties.timer_cb(timer(_uv_handle));
  }
  else
    if (properties.timer_cb)  properties.timer_cb(timer(_uv_handle));
}



/*! \defgroup doxy_group__idle_prepare_check  idle, prepare, check
    \ingroup doxy_group__handle
    \brief `uv::idle`, `uv::prepare`, and `uv::check` handles. */
//! \{
#if 0
//{

/*! \brief The class template for `uv::idle`, `uv::prepare`, and `uv::check` handles.
    \sa libuv API documentation:
            \li [`uv_idle_t` — Idle handle](http://docs.libuv.org/en/v1.x/idle.html#uv-idle-t-idle-handle),
            \li [`uv_prepare_t` — Prepare handle](http://docs.libuv.org/en/v1.x/prepare.html#uv-prepare-t-prepare-handle),
            \li [`uv_check_t` — Check handle](http://docs.libuv.org/en/v1.x/check.html#uv-check-t-check-handle). */
template<
    typename _uvHandleTp_,
    int (*_uvInitFn_)(::uv_loop_t*, _uvHandleTp_*),
    int (*_uvStartFn_)(_uvHandleTp_*, void (*)(_uvHandleTp_*)),
    int (*_uvStopFn_)(_uvHandleTp_*)
>
class loop_watcher : public handle
{
  //! \cond
  friend class handle::uv_interface;
  friend class handle::instance< loop_watcher >;
  //! \endcond

public: /*types*/
  using uv_t = _uvHandleTp_;
  using cb_t = std::function< void(loop_watcher _handle) >;
  /*!< \brief The function type of the handle's callback. */

protected: /*types*/
  //! \cond internals
  //! \addtogroup doxy_group__internals
  //! \{

  enum class opcmd  { UNKNOWN, STOP, START };

  struct properties : handle::properties
  {
    opcmd opcmd_state = opcmd::UNKNOWN;
    cb_t cb;
  };

  struct uv_interface : handle::uv_handle_interface  {};

  //! \}
  //! \endcond

private: /*types*/
  using instance = handle::instance< loop_watcher >;

private: /*functions*/
  static void cb(uv_t *_uv_handle)
  {
    auto &cb = instance::from(_uv_handle)->properties().cb;
    if (cb)  cb(idle(_uv_handle));
  }

protected: /*constructors*/
  //! \cond
  explicit loop_watcher(uv_t *_uv_handle) : handle(reinterpret_cast< handle::uv_t* >(_uv_handle))  {}
  //! \endcond

public: /*constructors*/
  ~loop_watcher() = default;

  loop_watcher(const loop_watcher&) = default;
  loop_watcher& operator =(const loop_watcher&) = default;

  loop_watcher(loop_watcher&&) noexcept = default;
  loop_watcher& operator =(loop_watcher&&) noexcept = default;

  /*! \brief Create a watcher handle. */
  explicit loop_watcher(uv::loop &_loop)
  {
    uv_handle = instance::create();

    auto uv_ret = (*_uvInitFn_)(static_cast< uv::loop::uv_t* >(_loop), static_cast< uv_t* >(uv_handle));
    if (uv_status(uv_ret) < 0)  return;

    instance::from(uv_handle)->book_loop();
  }

public: /*interface*/
  /*! \brief Set the handle's callback. */
  cb_t& cb() const noexcept  { return instance::from(uv_handle)->properties().cb; }

  /*! \brief Start the handle.
      \note On successful start this function adds an extra reference to the handle instance,
      which is released when the counterpart function `stop()` is called. Repeated calls don't
      change the reference count. */
  int start() const
  {
    auto instance_ptr = instance::from(uv_handle);
    auto &properties = instance_ptr->properties();

    auto opcmd_state0 = properties.opcmd_state;

    properties.opcmd_state = opcmd::START;
    instance_ptr->ref();  // REF:START -- make sure it will exist for the future idle_cb() calls until stop()

    switch (opcmd_state0)
    {
    case opcmd::UNKNOWN:
    case opcmd::STOP:
        break;
    case opcmd::START:
        // uv_status(::uv_idle_stop(static_cast< uv_t* >(uv_handle)));  // it does not need to stop
        instance_ptr->unref();  // UNREF:START -- emulate stop()
        break;
    }

    uv_status(0);
    auto uv_ret = (*_uvStartFn_)(static_cast< uv_t* >(uv_handle), cb);
    if (uv_ret < 0)
    {
      uv_status(uv_ret);
      properties.opcmd_state = opcmd::UNKNOWN;
      instance_ptr->unref();  // UNREF:START_FAILURE -- release the extra reference on failure
    }

    return uv_ret;
  }

  /*! \brief Start the handle with the given callback.
      \details This is equivalent for
      ```
      loop_watcher.cb() = std::bind(
          std::forward< _Cb_ >(_cb), std::placeholders::_1, std::forward< _Args_ >(_args)...
      );
      loop_watcher.start();
      ```
      \sa `loop_watcher::start()` */
  template< class _Cb_, typename... _Args_, typename = std::enable_if_t< std::is_convertible<
      decltype(std::bind(std::declval< _Cb_ >(), std::placeholders::_1, static_cast< _Args_&& >(std::declval< _Args_ >())...)),
      cb_t
  >::value > >
  int start(_Cb_ &&_cb, _Args_&&... _args) const
  {
    cb() = std::bind(std::forward< _Cb_ >(_cb), std::placeholders::_1, std::forward< _Args_ >(_args)...);
    return start();
  }

  /*! \brief Stop the handle, the callback will no longer be called. */
  int stop() const noexcept
  {
    auto instance_ptr = instance::from(uv_handle);
    auto &opcmd_state = instance_ptr->properties().opcmd_state;

    auto opcmd_state0 = opcmd_state;
    opcmd_state = opcmd::STOP;

    auto uv_ret = uv_status((*_uvStopFn_)(static_cast< uv_t* >(uv_handle)));

    switch (opcmd_state0)
    {
    case opcmd::UNKNOWN:
    case opcmd::STOP:
        break;
    case opcmd::START:
        instance_ptr->unref();  // UNREF:STOP -- release the reference from start()
        break;
    }

    return uv_ret;
  }

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_handle); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_handle); }
};

/*! \brief Idle handle.
    \sa libuv API documentation: [`uv_idle_t` — Idle handle](http://docs.libuv.org/en/v1.x/idle.html#uv-idle-t-idle-handle). */
using idle = loop_watcher< ::uv_idle_t, ::uv_idle_init, ::uv_idle_start, ::uv_idle_stop >;

/*! \brief Prepare handle.
    \sa libuv API documentation: [`uv_prepare_t` — Prepare handle](http://docs.libuv.org/en/v1.x/prepare.html#uv-prepare-t-prepare-handle). */
using prepare = loop_watcher< ::uv_prepare_t, ::uv_prepare_init, ::uv_prepare_start, ::uv_prepare_stop >;

/*! \brief Check handle.
    \sa libuv API documentation: [`uv_check_t` — Check handle](http://docs.libuv.org/en/v1.x/check.html#uv-check-t-check-handle). */
using check = loop_watcher< ::uv_check_t, ::uv_check_init, ::uv_check_start, ::uv_check_stop >;

//}
#else
//{

/*! \brief Idle handle.
    \sa libuv API documentation: [`uv_idle_t` — Idle handle](http://docs.libuv.org/en/v1.x/idle.html#uv-idle-t-idle-handle). */
class idle : public handle
{
  //! \cond
  friend class handle::uv_interface;
  friend class handle::instance< idle >;
  //! \endcond

public: /*types*/
  using uv_t = ::uv_idle_t;
  using on_idle_t = std::function< void(idle _handle) >;
  /*!< \brief The function type of the handle's callback. */

protected: /*types*/
  //! \cond internals
  //! \addtogroup doxy_group__internals
  //! \{

  enum class opcmd  { UNKNOWN, STOP, START };

  struct properties : handle::properties
  {
    opcmd opcmd_state = opcmd::UNKNOWN;
    on_idle_t idle_cb;
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
  explicit idle(uv::loop &_loop)
  {
    uv_handle = instance::create();

    auto uv_ret = ::uv_idle_init(static_cast< uv::loop::uv_t* >(_loop), static_cast< uv_t* >(uv_handle));
    if (uv_status(uv_ret) < 0)  return;

    instance::from(uv_handle)->book_loop();
  }

public: /*interface*/
  /*! \brief Set the handle's callback. */
  on_idle_t& on_idle() const noexcept  { return instance::from(uv_handle)->properties().idle_cb; }

  /*! \brief Start the handle.
      \note On successful start this function adds an extra reference to the handle instance,
      which is released when the counterpart function `stop()` is called. Repeated calls don't
      change the reference count.
      \sa libuv API documentation: [`uv_idle_t` — Idle handle](http://docs.libuv.org/en/v1.x/idle.html#uv-idle-t-idle-handle). */
  int start() const
  {
    auto instance_ptr = instance::from(uv_handle);
    auto &properties = instance_ptr->properties();

    auto opcmd_state0 = properties.opcmd_state;

    properties.opcmd_state = opcmd::START;
    instance_ptr->ref();  // REF:START -- make sure it will exist for the future idle_cb() calls until stop()

    switch (opcmd_state0)
    {
    case opcmd::UNKNOWN:
    case opcmd::STOP:
        break;
    case opcmd::START:
        // uv_status(::uv_idle_stop(static_cast< uv_t* >(uv_handle)));  // it does not need to stop
        instance_ptr->unref();  // UNREF:START -- emulate stop()
        break;
    }

    uv_status(0);
    auto uv_ret = ::uv_idle_start(static_cast< uv_t* >(uv_handle), idle_cb);
    if (uv_ret < 0)
    {
      uv_status(uv_ret);
      properties.opcmd_state = opcmd::UNKNOWN;
      instance_ptr->unref();  // UNREF:START_FAILURE -- release the extra reference on failure
    }

    return uv_ret;
  }

  /*! \brief Start the handle with the given callback.
      \details This is equivalent for
      ```
      idle.on_idle() = std::bind(
          std::forward< _Cb_ >(_cb), std::placeholders::_1, std::forward< _Args_ >(_args)...
      );
      idle.start();
      ```
      \sa `idle::start()` */
  template< class _Cb_, typename... _Args_, typename = std::enable_if_t< std::is_convertible<
      decltype(std::bind(std::declval< _Cb_ >(), std::placeholders::_1, static_cast< _Args_&& >(std::declval< _Args_ >())...)),
      on_idle_t
  >::value > >
  int start(_Cb_ &&_cb, _Args_&&... _args) const
  {
    on_idle() = std::bind(std::forward< _Cb_ >(_cb), std::placeholders::_1, std::forward< _Args_ >(_args)...);
    return start();
  }

  /*! \brief Stop the handle, the callback will no longer be called. */
  int stop() const noexcept
  {
    auto instance_ptr = instance::from(uv_handle);
    auto &opcmd_state = instance_ptr->properties().opcmd_state;

    auto opcmd_state0 = opcmd_state;
    opcmd_state = opcmd::STOP;

    auto uv_ret = uv_status(::uv_idle_stop(static_cast< uv_t* >(uv_handle)));

    switch (opcmd_state0)
    {
    case opcmd::UNKNOWN:
    case opcmd::STOP:
        break;
    case opcmd::START:
        instance_ptr->unref();  // UNREF:STOP -- release the reference from start()
        break;
    }

    return uv_ret;
  }

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_handle); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_handle); }
};

template< typename >
void idle::idle_cb(::uv_idle_t *_uv_handle)
{
  auto &idle_cb = instance::from(_uv_handle)->properties().idle_cb;
  if (idle_cb)  idle_cb(idle(_uv_handle));
}


/*! \brief Prepare handle.
    \sa libuv API documentation: [`uv_prepare_t` — Prepare handle](http://docs.libuv.org/en/v1.x/prepare.html#uv-prepare-t-prepare-handle). */
class prepare : public handle
{
  //! \cond
  friend class handle::uv_interface;
  friend class handle::instance< prepare >;
  //! \endcond

public: /*types*/
  using uv_t = ::uv_prepare_t;
  using on_prepare_t = std::function< void(prepare _handle) >;
  /*!< \brief The function type of the handle's callback. */

protected: /*types*/
  //! \cond internals
  //! \addtogroup doxy_group__internals
  //! \{

  enum class opcmd  { UNKNOWN, STOP, START };

  struct properties : handle::properties
  {
    opcmd opcmd_state = opcmd::UNKNOWN;
    on_prepare_t prepare_cb;
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
  explicit prepare(uv::loop &_loop)
  {
    uv_handle = instance::create();

    auto uv_ret = ::uv_prepare_init(static_cast< uv::loop::uv_t* >(_loop), static_cast< uv_t* >(uv_handle));
    if (uv_status(uv_ret) < 0)  return;

    instance::from(uv_handle)->book_loop();
  }

public: /*interface*/
  /*! \brief Set the handle's callback. */
  on_prepare_t& on_prepare() const noexcept  { return instance::from(uv_handle)->properties().prepare_cb; }

  /*! \brief Start the handle.
      \note On successful start this function adds an extra reference to the handle instance,
      which is released when the counterpart function `stop()` is called. Repeated calls don't
      change the reference count.
      \sa libuv API documentation: [`uv_prepare_t` — Prepare handle](http://docs.libuv.org/en/v1.x/prepare.html#uv-prepare-t-prepare-handle). */
  int start() const
  {
    auto instance_ptr = instance::from(uv_handle);
    auto &properties = instance_ptr->properties();

    auto opcmd_state0 = properties.opcmd_state;

    properties.opcmd_state = opcmd::START;
    instance_ptr->ref();  // REF:START -- make sure it will exist for the future prepare_cb() calls until stop()

    switch (opcmd_state0)
    {
    case opcmd::UNKNOWN:
    case opcmd::STOP:
        break;
    case opcmd::START:
        // uv_status(::uv_prepare_stop(static_cast< uv_t* >(uv_handle)));  // it does not need to stop
        instance_ptr->unref();  // UNREF:START -- emulate stop()
        break;
    }

    uv_status(0);
    auto uv_ret = ::uv_prepare_start(static_cast< uv_t* >(uv_handle), prepare_cb);
    if (uv_ret < 0)
    {
      uv_status(uv_ret);
      properties.opcmd_state = opcmd::UNKNOWN;
      instance_ptr->unref();  // UNREF:START_FAILURE -- release the extra reference on failure
    }

    return uv_ret;
  }

  /*! \brief Start the handle with the given callback.
      \details This is equivalent for
      ```
      prepare.on_prepare() = std::bind(
          std::forward< _Cb_ >(_cb), std::placeholders::_1, std::forward< _Args_ >(_args)...
      );
      prepare.start();
      ```
      \sa `prepare::start()` */
  template< class _Cb_, typename... _Args_, typename = std::enable_if_t< std::is_convertible<
      decltype(std::bind(std::declval< _Cb_ >(), std::placeholders::_1, static_cast< _Args_&& >(std::declval< _Args_ >())...)),
      on_prepare_t
  >::value > >
  int start(_Cb_ &&_cb, _Args_&&... _args) const
  {
    on_prepare() = std::bind(std::forward< _Cb_ >(_cb), std::placeholders::_1, std::forward< _Args_ >(_args)...);
    return start();
  }

  /*! \brief Stop the handle, the callback will no longer be called. */
  int stop() const noexcept
  {
    auto instance_ptr = instance::from(uv_handle);
    auto &opcmd_state = instance_ptr->properties().opcmd_state;

    auto opcmd_state0 = opcmd_state;
    opcmd_state = opcmd::STOP;

    auto uv_ret = uv_status(::uv_prepare_stop(static_cast< uv_t* >(uv_handle)));

    switch (opcmd_state0)
    {
    case opcmd::UNKNOWN:
    case opcmd::STOP:
        break;
    case opcmd::START:
        instance_ptr->unref();  // UNREF:STOP -- release the reference from start()
        break;
    }

    return uv_ret;
  }

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_handle); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_handle); }
};

template< typename >
void prepare::prepare_cb(::uv_prepare_t *_uv_handle)
{
  auto &prepare_cb = instance::from(_uv_handle)->properties().prepare_cb;
  if (prepare_cb)  prepare_cb(prepare(_uv_handle));
}


/*! \brief Check handle.
    \sa libuv API documentation: [`uv_check_t` — Check handle](http://docs.libuv.org/en/v1.x/check.html#uv-check-t-check-handle). */
class check : public handle
{
  //! \cond
  friend class handle::uv_interface;
  friend class handle::instance< check >;
  //! \endcond

public: /*types*/
  using uv_t = ::uv_check_t;
  using on_check_t = std::function< void(check _handle) >;
  /*!< \brief The function type of the handle's callback. */

protected: /*types*/
  //! \cond internals
  //! \addtogroup doxy_group__internals
  //! \{

  enum class opcmd  { UNKNOWN, STOP, START };

  struct properties : handle::properties
  {
    opcmd opcmd_state = opcmd::UNKNOWN;
    on_check_t check_cb;
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
  explicit check(uv::loop &_loop)
  {
    uv_handle = instance::create();

    auto uv_ret = ::uv_check_init(static_cast< uv::loop::uv_t* >(_loop), static_cast< uv_t* >(uv_handle));
    if (uv_status(uv_ret) < 0)  return;

    instance::from(uv_handle)->book_loop();
  }

public: /*interface*/
  /*! \brief Set the handle's callback. */
  on_check_t& on_check() const noexcept  { return instance::from(uv_handle)->properties().check_cb; }

  /*! \brief Start the handle.
      \note On successful start this function adds an extra reference to the handle instance,
      which is released when the counterpart function `stop()` is called. Repeated calls don't
      change the reference count.
      \sa libuv API documentation: [`uv_check_t` — Check handle](http://docs.libuv.org/en/v1.x/check.html#uv-check-t-check-handle). */
  int start() const
  {
    auto instance_ptr = instance::from(uv_handle);
    auto &properties = instance_ptr->properties();

    auto opcmd_state0 = properties.opcmd_state;

    properties.opcmd_state = opcmd::START;
    instance_ptr->ref();  // REF:START -- make sure it will exist for the future check_cb() calls until stop()

    switch (opcmd_state0)
    {
    case opcmd::UNKNOWN:
    case opcmd::STOP:
        break;
    case opcmd::START:
        // uv_status(::uv_check_stop(static_cast< uv_t* >(uv_handle)));  // it does not need to stop
        instance_ptr->unref();  // UNREF:START -- emulate stop()
        break;
    }

    uv_status(0);
    auto uv_ret = ::uv_check_start(static_cast< uv_t* >(uv_handle), check_cb);
    if (uv_ret < 0)
    {
      uv_status(uv_ret);
      properties.opcmd_state = opcmd::UNKNOWN;
      instance_ptr->unref();  // UNREF:START_FAILURE -- release the extra reference on failure
    }

    return uv_ret;
  }

  /*! \brief Start the handle with the given callback.
      \details This is equivalent for
      ```
      check.on_check() = std::bind(
          std::forward< _Cb_ >(_cb), std::placeholders::_1, std::forward< _Args_ >(_args)...
      );
      check.start();
      ```
      \sa `check::start()` */
  template< class _Cb_, typename... _Args_, typename = std::enable_if_t< std::is_convertible<
      decltype(std::bind(std::declval< _Cb_ >(), std::placeholders::_1, static_cast< _Args_&& >(std::declval< _Args_ >())...)),
      on_check_t
  >::value > >
  int start(_Cb_ &&_cb, _Args_&&... _args) const
  {
    on_check() = std::bind(std::forward< _Cb_ >(_cb), std::placeholders::_1, std::forward< _Args_ >(_args)...);
    return start();
  }

  /*! \brief Stop the handle, the callback will no longer be called. */
  int stop() const noexcept
  {
    auto instance_ptr = instance::from(uv_handle);
    auto &opcmd_state = instance_ptr->properties().opcmd_state;

    auto opcmd_state0 = opcmd_state;
    opcmd_state = opcmd::STOP;

    auto uv_ret = uv_status(::uv_check_stop(static_cast< uv_t* >(uv_handle)));

    switch (opcmd_state0)
    {
    case opcmd::UNKNOWN:
    case opcmd::STOP:
        break;
    case opcmd::START:
        instance_ptr->unref();  // UNREF:STOP -- release the reference from start()
        break;
    }

    return uv_ret;
  }

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_handle); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_handle); }
};

template< typename >
void check::check_cb(::uv_check_t *_uv_handle)
{
  auto &check_cb = instance::from(_uv_handle)->properties().check_cb;
  if (check_cb)  check_cb(check(_uv_handle));
}

//}
#endif
//! \}



/*! \ingroup doxy_group__handle
    \brief Signal handle.
    \sa libuv API documentation: [`uv_signal_t` — Signal handle](http://docs.libuv.org/en/v1.x/signal.html#uv-signal-t-signal-handle). */
class signal : public handle
{
  //! \cond
  friend class handle::uv_interface;
  friend class handle::instance< signal >;
  //! \endcond

public: /*types*/
  using uv_t = ::uv_signal_t;
  using on_signal_t = std::function< void(signal _handle, bool _oneshot) >;
  /*!< \brief The function type of the signal callback.
       \details The `_oneshot` parameter indicates that the signal handling was started in mode of `start_oneshot()` function.
       \sa libuv API documentation: [`uv_signal_cb`](http://docs.libuv.org/en/v1.x/signal.html#c.uv_signal_cb). */

protected: /*types*/
  //! \cond internals
  //! \addtogroup doxy_group__internals
  //! \{

  enum class opcmd  { UNKNOWN, STOP, START, START_ONESHOT };

  struct properties : handle::properties
  {
    opcmd opcmd_state = opcmd::UNKNOWN;
    int signum = 0;
    on_signal_t signal_cb;
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

  /*! \brief Create a `signal` handle watching for `_signum` signal. */
  signal(uv::loop &_loop, int _signum)
  {
    uv_handle = instance::create();

    auto uv_ret = ::uv_signal_init(static_cast< uv::loop::uv_t* >(_loop), static_cast< uv_t* >(uv_handle));
    if (uv_status(uv_ret) < 0)  return;

    instance::from(uv_handle)->properties().signum = _signum;

    instance::from(uv_handle)->book_loop();
  }

protected: /*functions*/
  //! \cond
  int start(opcmd _startcmd_state) const
  {
    switch (_startcmd_state)
    {
    case opcmd::UNKNOWN:
    case opcmd::STOP:
        throw std::invalid_argument(__PRETTY_FUNCTION__);
        break;
    case opcmd::START:
    case opcmd::START_ONESHOT:
        break;
    }

    auto instance_ptr = instance::from(uv_handle);
    auto &properties = instance_ptr->properties();

    auto opcmd_state0 = properties.opcmd_state;

    properties.opcmd_state = _startcmd_state;
    instance_ptr->ref();  // REF:START/START_ONESHOT -- make sure it will exist for the future signal_cb() calls until stop()

    switch (opcmd_state0)
    {
    case opcmd::UNKNOWN:
    case opcmd::STOP:
        break;
    case opcmd::START:
    case opcmd::START_ONESHOT:
        // uv_status(::uv_signal_stop(static_cast< uv_t* >(uv_handle)));  // ::uv_signal_start() does this when necessary
        instance_ptr->unref();  // UNREF:RESTART -- emulate stop()
        break;
    }

    uv_status(0);
    auto uv_ret = ::uv_signal_start(static_cast< uv_t* >(uv_handle), signal_cb, properties.signum);
    if (uv_ret < 0)
    {
      uv_status(uv_ret);
      properties.opcmd_state = opcmd::UNKNOWN;
      instance_ptr->unref();  // UNREF:START_FAILURE -- release the extra reference on failure
    }

    return uv_ret;
  }
  //! \endcond

public: /*interface*/
  /*! \brief Get the signal number being watched for by this handle. */
  int signum() const noexcept
  {
    auto signum = static_cast< uv_t* >(uv_handle)->signum;
    return  signum ? signum : instance::from(uv_handle)->properties().signum;
  }

  /*! \brief Set the signal callback. */
  on_signal_t& on_signal() const noexcept  { return instance::from(uv_handle)->properties().signal_cb; }

  /*! \brief Start the handle for watching for the signal.
      \details Repeated call to this function results in the automatic call to `stop()` first
      (possible signal events are not missed if the signal number has not changed).
      \note On successful start this function adds an extra reference to the handle instance,
      which is released when the counterpart function `stop()` is called.
      \sa libuv API documentation: [`uv_signal_t` — Signal handle](http://docs.libuv.org/en/v1.x/signal.html#uv-signal-t-signal-handle). */
  int start() const  { return start(opcmd::START); }

  /*! \brief Start the handle with the given signal callback.
      \details This is equivalent for
      ```
      signal.on_signal() = std::bind(
          std::forward< _Cb_ >(_cb), std::placeholders::_1, std::placeholders::_2, std::forward< _Args_ >(_args)...
      );
      signal.start();
      ```
      \sa `signal::start()` */
  template< class _Cb_, typename... _Args_, typename = std::enable_if_t< std::is_convertible<
      decltype(std::bind(std::declval< _Cb_ >(), std::placeholders::_1, std::placeholders::_2, static_cast< _Args_&& >(std::declval< _Args_ >())...)),
      on_signal_t
  >::value > >
  int start(_Cb_ &&_cb, _Args_&&... _args) const
  {
    instance::from(uv_handle)->properties().signal_cb = std::bind(
        std::forward< _Cb_ >(_cb), std::placeholders::_1, std::placeholders::_2, std::forward< _Args_ >(_args)...
    );
    return start(opcmd::START);
  }

  /*! \brief Start the handle with the given signal callback, watching for the given signal number.
      \details This function overload allows to change the signal number being watched for by the handle.
      This is equivalent for
      ```
      signal.signum() = _signum;  // (virtually) change the signal number being watched for
      signal.on_signal() = std::bind(
          std::forward< _Cb_ >(_cb), std::placeholders::_1, std::placeholders::_2, std::forward< _Args_ >(_args)...
      );
      signal.start();
      ```
      \sa `signal::start()` */
  template< class _Cb_, typename... _Args_, typename = std::enable_if_t< std::is_convertible<
      decltype(std::bind(std::declval< _Cb_ >(), std::placeholders::_1, std::placeholders::_2, static_cast< _Args_&& >(std::declval< _Args_ >())...)),
      on_signal_t
  >::value > >
  int start(int _signum, _Cb_ &&_cb, _Args_&&... _args) const
  {
    instance::from(uv_handle)->properties().signum = _signum;
    instance::from(uv_handle)->properties().signal_cb = std::bind(
        std::forward< _Cb_ >(_cb), std::placeholders::_1, std::placeholders::_2, std::forward< _Args_ >(_args)...
    );
    return start(opcmd::START);
  }

#if (UV_VERSION_MAJOR >= 1) && (UV_VERSION_MINOR >= 12)
  /*! \brief Start the handle for watching for the signal for the it first occurrence.
      \details The signal handler is called for the first occasion of the signal received,
      after which the handle is immediately stopped.

      Repeated call to this function results in the automatic call to `stop()` first.
      \note On successful start this function adds an extra reference to the handle instance,
      which is **automatically released** when the signal is received or if none has received,
      when the counterpart function `stop()` is called. */
  int start_oneshot() const  { return start(opcmd::START_ONESHOT); }
#endif

  /*! \brief Stop the handle, the callback will no longer be called. */
  int stop() const noexcept
  {
    auto instance_ptr = instance::from(uv_handle);
    auto &opcmd_state = instance_ptr->properties().opcmd_state;

    auto opcmd_state0 = opcmd_state;
    opcmd_state = opcmd::STOP;

    auto uv_ret = uv_status(::uv_signal_stop(static_cast< uv_t* >(uv_handle)));

    switch (opcmd_state0)
    {
    case opcmd::UNKNOWN:
    case opcmd::STOP:
        break;
    case opcmd::START:
    case opcmd::START_ONESHOT:
        instance_ptr->unref();  // UNREF:STOP -- release the reference from start() or fruitless start_oneshot()
        break;
    }

    return uv_ret;
  }

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_handle); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_handle); }
};

template< typename >
void signal::signal_cb(::uv_signal_t *_uv_handle, int)
{
  auto instance_ptr = instance::from(_uv_handle);
  auto &signal_cb = instance_ptr->properties().signal_cb;

  if (instance_ptr->properties().opcmd_state == opcmd::START_ONESHOT)
  {
    instance_ptr->properties().opcmd_state = opcmd::UNKNOWN;
    ref_guard< instance > unref_handle(*instance_ptr, adopt_ref);  // UNREF:START_ONESHOT

    if (signal_cb)  signal_cb(signal(_uv_handle), true);
  }
  else
    if (signal_cb)  signal_cb(signal(_uv_handle), false);
}



/*! \ingroup doxy_group__handle
    \brief Process handle.
    \sa libuv API documentation: [`uv_process_t` — Process handle](http://docs.libuv.org/en/v1.x/process.html#uv-process-t-process-handle). */
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
    struct stdio_container : ::uv_stdio_container_t
    {
      stdio_container() noexcept  // to provide proper initialization
      {
        flags = UV_IGNORE;
        data.fd = -1;
      }
    };
    struct stdio_endpoint : io
    {
      stdio_endpoint() noexcept = default;  // to provide demandable access to protected `io` default ctor
    };

    ::uv_process_options_t spawn_options = { 0,};
    on_exit_t exit_cb;
    std::vector< stdio_container > stdio_uv_containers;
    std::vector< stdio_endpoint > stdio_uvcc_endpoints;

    properties()
    {
      spawn_options.exit_cb = process::exit_cb;
    }

    void ensure_stdio_number(unsigned _target_fd_number)
    {
      if (_target_fd_number >= stdio_uv_containers.size())
      {
        stdio_uv_containers.resize(_target_fd_number + 1);
        stdio_uvcc_endpoints.resize(_target_fd_number + 1);

        spawn_options.stdio_count = stdio_uv_containers.size();
        spawn_options.stdio = stdio_uv_containers.data();
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
  explicit process(uv::loop &_loop)
  {
    uv_handle = instance::create();
    static_cast< uv_t* >(uv_handle)->loop = static_cast< loop::uv_t* >(_loop);
    instance::from(uv_handle)->book_loop();
  }

public: /*interface*/
  /*! \brief Force child processes spawned by this process not to inherit file descriptors/handles that this process has inherited from its parent.
      \sa libuv API documentation: [`uv_disable_stdio_inheritance()`](http://docs.libuv.org/en/v1.x/process.html#c.uv_disable_stdio_inheritance). */
  static void disable_stdio_inheritance() noexcept  { ::uv_disable_stdio_inheritance(); }

  /*! \brief Send the specified signal to the given PID.
      \sa libuv API documentation: [`uv_kill()`](http://docs.libuv.org/en/v1.x/process.html#c.uv_kill). */
  static int kill(int _pid, int _signum) noexcept  { return ::uv_kill(_pid, _signum); }

  /*! \brief The PID of the child process. It is set after calling `spawn()`. */
  int pid() const noexcept  { return static_cast< uv_t* >(uv_handle)->pid; }

  /*! \name Functions to prepare for spawning a child process:: */
  //! \{

  /*! \brief Set the callback function called when the child process exits. */
  on_exit_t& on_exit() const noexcept  { return instance::from(uv_handle)->properties().exit_cb; }

  /*! \brief Set pointer to environment for the child process. If `nullptr` the parents environment is used. */
  void set_environment(char *_envp[]) const noexcept
  {
    auto &spawn_options = instance::from(uv_handle)->properties().spawn_options;
    spawn_options.env = _envp;
  }

  /*! \brief Set current working directory when spawning the child process. */
  void set_working_dir(const char *_cwd) const noexcept
  {
    auto &spawn_options = instance::from(uv_handle)->properties().spawn_options;
    spawn_options.cwd = _cwd;
  }

  /*! \brief Set the io endpoint that should be available to the child process as the target stdio file descriptor number.
      \sa libuv API documentation: [`uv_process_options_t.stdio`](http://docs.libuv.org/en/v1.x/process.html#c.uv_process_options_t.stdio),
                                   [`uv_process_options_t`](http://docs.libuv.org/en/v1.x/process.html#c.uv_process_options_t),
                                   [`uv_stdio_container_t`](http://docs.libuv.org/en/v1.x/process.html#c.uv_stdio_container_t),
                                   [`uv_stdio_flags`](http://docs.libuv.org/en/v1.x/process.html#c.uv_stdio_flags). */
  void inherit_stdio(unsigned _target_fd_number, io _io)  const
  {
    auto &properties = instance::from(uv_handle)->properties();

    properties.ensure_stdio_number(_target_fd_number);

    if (_io.id())  switch (_io.type())
    {
      case UV_NAMED_PIPE:
      case UV_STREAM:
      case UV_TCP:
      case UV_TTY:
          properties.stdio_uv_containers[_target_fd_number].flags = UV_INHERIT_STREAM;
          properties.stdio_uv_containers[_target_fd_number].data.stream = static_cast< stream::uv_t* >(static_cast< stream& >(_io));
          break;
      case UV_FILE:
          properties.stdio_uv_containers[_target_fd_number].flags = UV_INHERIT_FD;
          properties.stdio_uv_containers[_target_fd_number].data.fd = static_cast< file& >(_io).fd();
          break;
      default:
          properties.stdio_uv_containers[_target_fd_number].flags = UV_IGNORE;
          properties.stdio_uv_containers[_target_fd_number].data.fd = -1;
    }

    properties.stdio_uvcc_endpoints[_target_fd_number] = static_cast< properties::stdio_endpoint&& >(_io);  // move assignment
  }

  /*! \brief Set the file descriptor that should be inherited by the child process as the target stdio descriptor number. */
  void inherit_stdio(unsigned _target_fd_number, ::uv_file _fd)  const
  {
    auto &properties = instance::from(uv_handle)->properties();

    properties.ensure_stdio_number(_target_fd_number);

    properties.stdio_uv_containers[_target_fd_number].flags = UV_INHERIT_FD;
    properties.stdio_uv_containers[_target_fd_number].data.fd = _fd;
  }

  /*! \brief Create a pipe to the child process' stdio fd number.
      \details Only `UV_READABLE_PIPE` and `UV_WRITABLE_PIPE` flags from the
      [`uv_stdio_flags`](http://docs.libuv.org/en/v1.x/process.html#c.uv_stdio_flags) enumeration take effect.\n
      Set `_ipc` parameter to `true` if the created pipe is going to be used for handle passing between processes.
      \sa libuv API documentation: [`uv_stdio_flags`](http://docs.libuv.org/en/v1.x/process.html#c.uv_stdio_flags),
                                   [`uv_process_options_t.stdio`](http://docs.libuv.org/en/v1.x/process.html#c.uv_process_options_t.stdio). */
  int create_stdio_pipe(unsigned _target_fd_number, uv::loop &_pipe_loop, ::uv_stdio_flags _pipe_flags, bool _ipc = false) const
  {
    auto &properties = instance::from(uv_handle)->properties();

    properties.ensure_stdio_number(_target_fd_number);

    pipe p(_pipe_loop, _ipc);
    if (!p)  return uv_status(p.uv_status());

    int flags = UV_CREATE_PIPE;
    if (_pipe_flags & UV_READABLE_PIPE)  flags |= UV_READABLE_PIPE;
    if (_pipe_flags & UV_WRITABLE_PIPE)  flags |= UV_WRITABLE_PIPE;
    properties.stdio_uv_containers[_target_fd_number].flags = static_cast< ::uv_stdio_flags >(flags);
    properties.stdio_uv_containers[_target_fd_number].data.stream = static_cast< stream::uv_t* >(p);

    properties.stdio_uvcc_endpoints[_target_fd_number] = static_cast< properties::stdio_endpoint&& >(static_cast< io& >(p));  // move assignment
    return 0;
  }

  /*! \brief The stdio endpoints to be set for the child process. */
  std::vector< io >& stdio() const noexcept
  {
    auto &properties = instance::from(uv_handle)->properties();
    return reinterpret_cast< std::vector< io >& >(properties.stdio_uvcc_endpoints);
  }

  /*! \brief Set the child process' user id.
      \details Specifying a value of **-1** clears the `UV_PROCESS_SETUID` flag.
      \note On Windows this function is no-op.
      \sa libuv API documentation: [`uv_process_options_t.uid`](http://docs.libuv.org/en/v1.x/process.html#c.uv_process_options_t.uid),
                                   [`uv_process_flags`](http://docs.libuv.org/en/v1.x/process.html#c.uv_process_flags), */
  void set_uid(::uv_uid_t _uid) const noexcept
  {
#ifndef _WIN32
    auto &spawn_options = instance::from(uv_handle)->properties().spawn_options;
    if (_uid == -1)
      spawn_options.flags &= ~UV_PROCESS_SETUID;
    else
      spawn_options.flags |= UV_PROCESS_SETUID;
    spawn_options.uid = _uid;
#endif
  }
  /*! \brief Set the child process' group id.
      \details Specifying a value of **-1** clears the `UV_PROCESS_SETGID` flag.
      \note On Windows this function is no-op.
      \sa libuv API documentation: [`uv_process_options_t.gid`](http://docs.libuv.org/en/v1.x/process.html#c.uv_process_options_t.gid),
                                   [`uv_process_flags`](http://docs.libuv.org/en/v1.x/process.html#c.uv_process_flags), */
  void set_gid(::uv_gid_t _gid) const noexcept
  {
#ifndef _WIN32
    auto &spawn_options = instance::from(uv_handle)->properties().spawn_options;
    if (_gid == -1)
      spawn_options.flags &= ~UV_PROCESS_SETGID;
    else
      spawn_options.flags |= UV_PROCESS_SETGID;
    spawn_options.gid = _gid;
#endif
  }

  //! \}

  /*! \brief Create and start a new child process.
      \sa libuv API documentation: [`uv_spawn()`](http://docs.libuv.org/en/v1.x/process.html#c.uv_spawn),
                                   [`uv_process_flags`](http://docs.libuv.org/en/v1.x/process.html#c.uv_process_flags). */
  int spawn(const char *_file, char *_argv[], ::uv_process_flags _flags = static_cast< ::uv_process_flags >(0)) const noexcept
  {
    auto &properties = instance::from(uv_handle)->properties();
    {
      properties.spawn_options.file = _file;
      properties.spawn_options.args = _argv;
      properties.spawn_options.flags |= _flags;
    }

    return uv_status(
        ::uv_spawn(static_cast< uv_t* >(uv_handle)->loop, static_cast< uv_t* >(uv_handle), &properties.spawn_options)
    );
  }
  int spawn(const char *_file, const char *_argv[], ::uv_process_flags _flags = static_cast< ::uv_process_flags >(0)) const noexcept
  {
    return spawn(_file, const_cast< char** >(_argv), _flags);
  }

  /*! \brief Send the specified signal to the child process. */
  int kill(int _signum) const noexcept
  {
    return uv_status(::uv_process_kill(static_cast< uv_t* >(uv_handle), _signum));
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
