
#ifndef UVCC_HANDLE_MISC__HPP
#define UVCC_HANDLE_MISC__HPP

#include "uvcc/utility.hpp"
#include "uvcc/handle-base.hpp"

#include <cstdint>      // uint64_t
#include <uv.h>

#include <functional>   // function bind placeholders::


namespace uv
{


/*! \ingroup doxy_group__handle
    \brief Async handle.
    \sa libuv API documentation: [`uv_async_t — Async handle`](http://docs.libuv.org/en/v1.x/async.html#uv-async-t-async-handle). */
class async : public handle
{
  friend class handle::uv_interface;
  friend class handle::instance< async >;

public: /*types*/
  using uv_t = ::uv_async_t;
  template< typename... _Args_ >
  using on_send_t = std::function< void(async _handle, _Args_&&... _args) >;
  /*!< \brief The function type of the callback called by the [`async`](http://docs.libuv.org/en/v1.x/async.html#uv-async-t-async-handle) event.
       \note
        1. All the arguments are passed and stored by value along with the function object when the `async` handle variable is created.
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
  friend class handle::uv_interface;
  friend class handle::instance< timer >;

public: /*types*/
  using uv_t = ::uv_timer_t;
  template< typename... _Args_ >
  using on_timer_t = std::function< void(timer _handle, _Args_&&... _args) >;
  /*!< \brief The function type of the callback called by the timer scheduled with `timer::start()` function.
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


}


#endif
