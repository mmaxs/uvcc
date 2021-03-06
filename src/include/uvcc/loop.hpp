
#ifndef UVCC_LOOP__HPP
#define UVCC_LOOP__HPP

#include "uvcc/debug.hpp"
#include "uvcc/utility.hpp"

#include <cstddef>      // offsetof
#include <uv.h>

#include <functional>   // function bind placeholders::
#include <type_traits>  // is_standard_layout enable_if is_convertible
#include <utility>      // swap() forward()
#include <exception>    // uncaught_exception()
#include <stdexcept>    // runtime_error logic_error


namespace uv
{


class handle;


/*! \defgroup doxy_group__loop Event loop
    \brief The I/O event loop.
    \sa libuv API documentation: [the I/O loop](http://docs.libuv.org/en/v1.x/design.html#the-i-o-loop). */

/*! \ingroup doxy_group__loop
    \brief The I/O event loop class.
    \details All event loops (including the default one) are the instances of this class.
    \sa libuv API documentation: [`uv_loop_t`](http://docs.libuv.org/en/v1.x/loop.html#uv-loop-t-event-loop). */
class loop
{
  //! \cond
  friend class handle;
  friend class fs;
  friend class getaddrinfo;
  friend class getnameinfo;
  template< typename > friend class work;
  //! \endcond

public: /*types*/
  using uv_t = ::uv_loop_t;
  using on_destroy_t = std::function< void(void *_data) >;
  /*!< \brief The function type of the callback called when the loop instance is about to be destroyed. */
  using on_exit_t = std::function< void(loop _loop) >;
  /*!< \brief The function type of the callback called after the loop exit. */
  template< typename... _Args_ >
  using on_walk_t = std::function< void(handle _handle, _Args_&&... _args) >;
  /*!< \brief The function type of the callback called by the `walk()` function.
       \sa libuv API documentation: [`uv_walk_cb`](http://docs.libuv.org/en/v1.x/loop.html#c.uv_walk_cb),
                                    [`uv_walk()`](http://docs.libuv.org/en/v1.x/loop.html#c.uv_walk). */

private: /*types*/
  class instance
  {
  public: /*data*/
    mutable int uv_error = 0;
    ref_count refs;
    type_storage< on_destroy_t > destroy_cb_storage;
    type_storage< on_exit_t > exit_cb_storage;
    uv_t uv_loop_struct = { 0,};

  private: /*constructors*/
    instance()
    {
      uv_error = ::uv_loop_init(&uv_loop_struct);
      uvcc_debug_function_return("instance [0x%08tX] for loop [0x%08tX] (uv_error=%i)", (ptrdiff_t)this, (ptrdiff_t)&uv_loop_struct, uv_error);
    }

  public: /*constructors*/
    ~instance() noexcept(false)
    {
      uvcc_debug_function_enter("instance [0x%08tX] for loop [0x%08tX] (is_alive=%i)", (ptrdiff_t)this, (ptrdiff_t)&uv_loop_struct, ::uv_loop_alive(&uv_loop_struct));
      uvcc_debug_do_if(true, {
          uvcc_debug_log_if(true, "walk on loop [0x%08tX] destroying...", (ptrdiff_t)&uv_loop_struct);
          debug::print_loop_handles(&uv_loop_struct);
      });

      uv_error = ::uv_loop_close(&uv_loop_struct);
      auto loop_closed = (uv_error == 0);
      uvcc_debug_condition(loop_closed, "loop [0x%08tX] (is_alive=%i)", (ptrdiff_t)&uv_loop_struct, ::uv_loop_alive(&uv_loop_struct));

      if (!loop_closed)  // this may be because of:
      {
        // 1) there are handles associated with the loop that are in deed not closed by the time the loop instance destroying
        unsigned num_open_handles = 0;
        ::uv_walk(
            &uv_loop_struct,
            [](::uv_handle_t *_h, void *_n){ if (!::uv_is_closing(_h))  ++*static_cast< unsigned* >(_n); },
            &num_open_handles
        );
        uvcc_debug_condition(num_open_handles == 0, "loop [0x%08tX] (num_open_handles=%u)", (ptrdiff_t)&uv_loop_struct, num_open_handles);
        if (num_open_handles)
        {
          // it is not a proper circumstances for the libuv loop to be closed
          // but the uvcc loop instance is destroying because of some weird things;
          // due to loop booking all the uvcc-managed handles are closed before the loop which they are associated with;
          // so, if this is a consequence of some exception, try to not make the bad situation even worse
          if (std::uncaught_exception())
          {
            uvcc_debug_log_if(true, "loop [0x%08tX] is being destroyed during stack unwinding", (ptrdiff_t)&uv_loop_struct);
            return;
          }
          else
            throw std::logic_error(__PRETTY_FUNCTION__);
        }
        // 2) there are registered callbacks from closed handles (that should be nullptrs by the way),
        //    or some libuv internal requests; so, simply try to dispose of them
        do {
          uvcc_debug_log_if(true, "try at loop [0x%08tX] premortal one shot nonblocking run", (ptrdiff_t)&uv_loop_struct);
          uv_error = ::uv_run(&uv_loop_struct, UV_RUN_NOWAIT);
        } while (uv_error);

        uv_error = ::uv_loop_close(&uv_loop_struct);
        loop_closed = (uv_error == 0);
        uvcc_debug_condition(loop_closed, "loop [0x%08tX] (is_alive=%i)", (ptrdiff_t)&uv_loop_struct, ::uv_loop_alive(&uv_loop_struct));
        if (!loop_closed)  throw std::runtime_error(__PRETTY_FUNCTION__);
      }
    }

    instance(const instance&) = delete;
    instance& operator =(const instance&) = delete;

    instance(instance&&) = delete;
    instance& operator =(instance&&) = delete;

  private: /*functions*/
    void destroy()
    {
      auto &destroy_cb = destroy_cb_storage.value();
      if (destroy_cb)  destroy_cb(uv_loop_struct.data);
      delete this;
    }

  public: /*interface*/
    static uv_t* create()  { return &(new instance())->uv_loop_struct; }

    constexpr static instance* from(uv_t *_uv_loop) noexcept
    {
      static_assert(std::is_standard_layout< instance >::value, "not a standard layout type");
      return reinterpret_cast< instance* >(reinterpret_cast< char* >(_uv_loop) - offsetof(instance, uv_loop_struct));
    }

    void ref()
    {
      uvcc_debug_function_enter("loop [0x%08tX]", (ptrdiff_t)&uv_loop_struct);
      refs.inc();
    }
    void unref()
    {
      uvcc_debug_function_enter("loop [0x%08tX]", (ptrdiff_t)&uv_loop_struct);
      auto nrefs = refs.dec();
      uvcc_debug_condition(nrefs == 0, "loop [0x%08tX] (nrefs=%li)", (ptrdiff_t)&uv_loop_struct, nrefs);
      if (nrefs == 0)  destroy();
    }
  };
  //! \cond
  friend typename loop::instance* debug::instance<>(loop&) noexcept;
  //! \endcond

private: /*data*/
  uv_t *uv_loop;

private: /*constructors*/
  explicit loop(uv_t *_uv_loop)
  {
    if (_uv_loop)  instance::from(_uv_loop)->ref();
    uv_loop = _uv_loop;
  }

public: /*constructors*/
  ~loop()  { if (uv_loop)  instance::from(uv_loop)->unref(); }

  /*! \brief Create a new event loop. */
  loop() : uv_loop(instance::create())  {}

  loop(const loop &_that) : loop(_that.uv_loop)  {}
  loop& operator =(const loop &_that)
  {
    if (this != &_that)
    {
      if (_that.uv_loop)  instance::from(_that.uv_loop)->ref();
      auto t = uv_loop;
      uv_loop = _that.uv_loop;
      if (t)  instance::from(t)->unref();
    }
    return *this;
  }

  loop(loop &&_that) noexcept : uv_loop(_that.uv_loop)  { _that.uv_loop = nullptr; }
  loop& operator =(loop &&_that) noexcept
  {
    if (this != &_that)
    {
      auto t = uv_loop;
      uv_loop = _that.uv_loop;
      _that.uv_loop = nullptr;
      if (t)  instance::from(t)->unref();
    }
    return *this;
  }

private: /*functions*/
  template< typename = void > static void walk_cb(::uv_handle_t*, void*);

  int uv_status(int _value) const noexcept
  {
    instance::from(uv_loop)->uv_error = _value;
    return _value;
  }

public: /*interface*/
  /*! \brief Returns the initialized loop that can be used as a global default loop throughout the program. */
  /*! \internal \note This function does not need to use the libuv function
      [`uv_default_loop()`](http://docs.libuv.org/en/v1.x/loop.html#c.uv_default_loop)
      to create, initialize, and get the default loop instance as far as that one is just an ordinary loop
      instance stored in the global static variable. \endinternal */
  static loop& Default() noexcept
  {
    static loop default_loop;
    return default_loop;
  }

  void swap(loop &_that) noexcept  { std::swap(uv_loop, _that.uv_loop); }
  /*! \brief The current number of existing references to the same loop as this variable refers to. */
  long nrefs() const noexcept  { return instance::from(uv_loop)->refs.get_value(); }
  /*! \brief The status value returned by the last executed libuv API function. */
  int uv_status() const noexcept  { return instance::from(uv_loop)->uv_error; }

  on_destroy_t& on_destroy() const noexcept  { return instance::from(uv_loop)->destroy_cb_storage.value(); }

  on_exit_t& on_exit() const noexcept  { return instance::from(uv_loop)->exit_cb_storage.value(); }

  /*! \details The pointer to the user-defined arbitrary data.
      \sa libuv API documentation: [`uv_loop_t.data`](http://docs.libuv.org/en/v1.x/loop.html#c.uv_loop_t.data). */
  void* const& data() const noexcept  { return uv_loop->data; }
  void*      & data()       noexcept  { return uv_loop->data; }

  /*! \brief Set additional loop options.
      \sa libuv API documentation: [`uv_loop_configure()`](http://docs.libuv.org/en/v1.x/loop.html#c.uv_loop_configure). */
  template< typename... _Args_ >
  int configure(::uv_loop_option _opt, _Args_&&... _args)
  {
    return uv_status(::uv_loop_configure(uv_loop, _opt, std::forward< _Args_ >(_args)...));
  }

  /*! \brief Go into a loop and process events and their callbacks with the current thread.
      \details The function acts and returns depending on circumstances which processing is defined by the `_mode` argument.
      \sa libuv API documentation: [`uv_run()`](http://docs.libuv.org/en/v1.x/loop.html#c.uv_run),
                                   [`uv_run_mode`](http://docs.libuv.org/en/v1.x/loop.html#c.uv_run_mode).
      \note If you start a loop with this function within a callback executed by another loop the first one will
      be "blocked" until the second started loop ends and the function returns. */
  int run(::uv_run_mode _mode)
  {
    auto uv_ret = uv_status(::uv_run(uv_loop, _mode));

    uvcc_debug_do_if(true, {
        uvcc_debug_log_if(true, "walk on loop [0x%08tX] (is_alive=%i) exiting (uv_error=%i)...", (ptrdiff_t)uv_loop, ::uv_loop_alive(uv_loop), uv_ret);
        debug::print_loop_handles(uv_loop);
    });

    auto &exit_cb = instance::from(uv_loop)->exit_cb_storage.value();
    if (exit_cb)  exit_cb(loop(uv_loop));

    return uv_ret;
  }

  /*! \brief Stop the event loop.
      \sa libuv API documentation: [`uv_stop()`](http://docs.libuv.org/en/v1.x/loop.html#c.uv_stop). */
  void stop()  { ::uv_stop(uv_loop); }

  /*! \brief Returns non-zero if there are active handles or request in the loop. */
  int is_alive() const noexcept  { return uv_status(::uv_loop_alive(uv_loop)); }

  /*! \details Get backend file descriptor.
      \sa libuv API documentation: [`uv_backend_fd()`](http://docs.libuv.org/en/v1.x/loop.html#c.uv_backend_fd). */
  int backend_fd() const noexcept  { return ::uv_backend_fd(uv_loop); }
  /*! \brief Get the poll timeout. The return value is in _milliseconds_, or \b -1 for no timeout. */
  int backend_timeout() const noexcept  { return ::uv_backend_timeout(uv_loop); }
  /*! \details Return the current timestamp in _milliseconds_.
      \sa libuv API documentation: [`uv_now()`](http://docs.libuv.org/en/v1.x/loop.html#c.uv_now). */
  uint64_t now() const noexcept  { return ::uv_now(uv_loop); }

  /*! \details Update the event loop’s concept of “now”.
      \sa libuv API documentation: [`uv_update_time()`](http://docs.libuv.org/en/v1.x/loop.html#c.uv_update_time). */
  void update_time() noexcept  { ::uv_update_time(uv_loop); }

  /*! \details Walk the list of active handles referenced by the loop: for each handle `_walk_cb`
      will be executed with the given `_args`.
      \note All arguments are copied (or moved) to the callback function object.
      For passing arguments by reference when some callback parameters are used as output ones,
      wrap corresponding arguments with `std::ref()` or pass them through raw pointer parameters. */
  template< class _Func_, typename... _Args_,
      typename = std::enable_if_t< std::is_convertible< _Func_, on_walk_t< _Args_&&... > >::value >
  >
  void walk(_Func_&& _walk_cb, _Args_&&... _args)
  {
    std::function< void(handle) > cb{ std::bind(std::forward< _Func_ >(_walk_cb), std::placeholders::_1, std::forward< _Args_ >(_args)...) };
    if (cb)  ::uv_walk(uv_loop, walk_cb, &cb);
  }

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return uv_loop; }
  explicit operator       uv_t*()       noexcept  { return uv_loop; }

  explicit operator bool() const noexcept  { return (uv_status() >= 0); }  /*!< \brief Equivalent to `(uv_status() >= 0)`. */
};


}


#include "uvcc/handle-base.hpp"


namespace uv
{
template< typename >
void loop::walk_cb(::uv_handle_t *_uv_handle, void *_arg)
{
  static_cast< std::function< void(handle) >* >(_arg)->operator()(handle(_uv_handle));
}
}


namespace std
{

//! \ingroup doxy_group__loop
template<> inline void swap(uv::loop &_this, uv::loop &_that) noexcept  { _this.swap(_that); }

}


#endif
