
#ifndef UVCC_LOOP__HPP
#define UVCC_LOOP__HPP

#include "uvcc/utility.hpp"

#include <uv.h>
#include <cstddef>      // offsetof
#include <functional>   // function
#include <type_traits>  // is_standard_layout enable_if_t
#include <utility>      // swap() forward()
#include <memory>       // addressof()


namespace uv
{


class handle;


/*! \defgroup g__loop Event loop
    \brief The I/O event loop.
    \sa libuv documentation: [the I/O loop](http://docs.libuv.org/en/v1.x/design.html#the-i-o-loop). */
//! \{


/*! \brief The I/O event loop class.
    \details All event loops (including the default one) are the instances of this class.
    \sa libuv documentation: [`uv_loop_t`](http://docs.libuv.org/en/v1.x/loop.html#uv-loop-t-event-loop). */
class loop
{
  friend class handle;

public: /*types*/
  using uv_t = ::uv_loop_t;
  using on_destroy_t = std::function< void(void *_data) >;  /*!< \brief The function type of the callback called when the loop instance is about to be destroyed. */
  using on_walk_t = std::function< void(handle _handle, void *_arg) >;
  /*!< \brief The function type of the callback called by the `walk()` function.
       \sa libuv documentation: [`uv_walk_cb`](http://docs.libuv.org/en/v1.x/loop.html#c.uv_walk_cb),
                                [`uv_walk()`](http://docs.libuv.org/en/v1.x/loop.html#c.uv_walk). */

private: /*types*/
  class instance
  {
  private: /*data*/
    mutable int last_error;
    ref_count rc;
    type_storage< on_destroy_t > on_destroy_storage;
    uv_t uv_loop;

  private: /*constructors*/
    instance()  { last_error = ::uv_loop_init(&uv_loop); }

  public: /*constructors*/
    ~instance()  { last_error = ::uv_loop_close(&uv_loop); }

    instance(const instance&) = delete;
    instance& operator =(const instance&) = delete;

    instance(instance&&) = delete;
    instance& operator =(instance&&) = delete;

  private: /*functions*/
    void destroy()
    {
      on_destroy_t &f = on_destroy_storage.value();
      if (f)  f(uv_loop.data);
      delete this;
    }

  public: /*interface*/
    static uv_t* create()  { return std::addressof((new instance())->uv_loop); }

    constexpr static instance* from(uv_t *_uv_loop) noexcept
    {
      static_assert(std::is_standard_layout< instance >::value, "not a standard layout type");
      return reinterpret_cast< instance* >(reinterpret_cast< char* >(_uv_loop) - offsetof(instance, uv_loop));
    }

    on_destroy_t& on_destroy() noexcept  { return on_destroy_storage.value(); }

    void ref()  { rc.inc(); }
    void unref()  { if (rc.dec() == 0)  destroy(); }
    ref_count::type nrefs() const noexcept  { return rc.value(); }

    int& status() const noexcept  { return last_error; }
  };

  struct walk_cb_pack
  {
    const on_walk_t &on_walk;
    void *arg;
  };

private: /*data*/
  uv_t *uv_loop;

private: /*constructors*/
  explicit loop(uv_t *_uv_loop)
  {
    instance::from(_uv_loop)->ref();
    uv_loop = _uv_loop;
  }

public: /*constructors*/
  ~loop()  { if (uv_loop)  instance::from(uv_loop)->unref(); }

  /*! \brief Create a new event loop. */
  loop() : uv_loop(instance::create())  {}

  loop(const loop &_that)
  {
    instance::from(_that.uv_loop)->ref();
    uv_loop = _that.uv_loop;
  }
  loop& operator =(const loop &_that)
  {
    if (this != &_that)
    {
      instance::from(_that.uv_loop)->ref();
      auto t = uv_loop;
      uv_loop = _that.uv_loop;
      instance::from(t)->unref();
    };
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
      instance::from(t)->unref();
    };
    return *this;
  }

private: /*functions*/
  template< typename = void > static void walk_cb(::uv_handle_t*, void*);

protected: /*functions*/
  //! \cond
  int status(int _last_error) const noexcept  { return (instance::from(uv_loop)->status() = _last_error); }
  //! \endcond

public: /*interface*/
  /*! \brief Returns the initialized loop that can be used as a global default loop throughout the program. */
  /*! \internal \note This function does not need to use the libuv function
      [`uv_default_loop()`](http://docs.libuv.org/en/v1.x/loop.html#c.uv_default_loop)
      to create, initialize, and get the default loop instance as far as that one is just an ordinary loop
      instance stored in the global static variable. */
  static loop Default() noexcept
  {
    static loop default_loop;
    return default_loop;
  }

  void swap(loop &_that) noexcept  { std::swap(uv_loop, _that.uv_loop); }
  /*! \brief The current number of existing references to the same loop as this variable refers to. */
  long nrefs() const noexcept  { return instance::from(uv_loop)->nrefs(); }
  /*! \brief The status value returned by the last executed libuv API function. */
  int status() const noexcept  { return instance::from(uv_loop)->status(); }

  const on_destroy_t& on_destroy() const noexcept  { return instance::from(uv_loop)->on_destroy(); }
        on_destroy_t& on_destroy()       noexcept  { return instance::from(uv_loop)->on_destroy(); }

  /*! \details The pointer to the user-defined arbitrary data.
      \sa libuv documentation: [`uv_loop_t.data`](http://docs.libuv.org/en/v1.x/loop.html#c.uv_loop_t.data). */
  void* const& data() const noexcept  { return uv_loop->data; }
  void*      & data()       noexcept  { return uv_loop->data; }

  /*! \details Set additional loop options.
      \sa libuv documentation: [`uv_loop_configure()`](http://docs.libuv.org/en/v1.x/loop.html#c.uv_loop_configure). */
  template< typename... _Args_ > int configure(::uv_loop_option _opt, _Args_&&... _args)
  {
    return status(::uv_loop_configure(uv_loop, _opt, std::forward< _Args_ >(_args)...));
  }

  /*! \details Start the event loop.
      \sa libuv documentation: [`uv_run()`](http://docs.libuv.org/en/v1.x/loop.html#c.uv_run). */
  int run(::uv_run_mode _mode)  { return status(::uv_run(uv_loop, _mode)); }
  /*! \details Stop the event loop.
      \sa libuv documentation: [`uv_stop()`](http://docs.libuv.org/en/v1.x/loop.html#c.uv_stop). */
  void stop()  { ::uv_stop(uv_loop); }

  /*! \brief Returns non-zero if there are active handles or request in the loop. */
  int is_alive() const noexcept  { return status(::uv_loop_alive(uv_loop)); }

  /*! \details Get backend file descriptor.
      \sa libuv documentation: [`uv_backend_fd()`](http://docs.libuv.org/en/v1.x/loop.html#c.uv_backend_fd). */
  int backend_fd() const noexcept  { return ::uv_backend_fd(uv_loop); }
  /*! \brief Get the poll timeout. The return value is in _milliseconds_, or \b -1 for no timeout. */
  int backend_timeout() const noexcept  { return ::uv_backend_timeout(uv_loop); }
  /*! \details Return the current timestamp in _milliseconds_.
      \sa libuv documentation: [`uv_now()`](http://docs.libuv.org/en/v1.x/loop.html#c.uv_now). */
  uint64_t now() const noexcept  { return ::uv_now(uv_loop); }

  /*! \details Update the event loop’s concept of “now”.
      \sa libuv documentation: [`uv_update_time()`](http://docs.libuv.org/en/v1.x/loop.html#c.uv_update_time). */
  void update_time() noexcept  { ::uv_update_time(uv_loop); }

  /*! \brief Walk the handles in the loop: for each handle in the loop `_walk_cb` will be executed with the given `_arg`. */
  void walk(const on_walk_t &_walk_cb, void *_arg)
  {
    if (!_walk_cb)  return;
    walk_cb_pack t{_walk_cb, _arg};
    ::uv_walk(uv_loop, walk_cb, &t);
  }

public: /*conversion operators*/
  operator const uv_t*() const noexcept  { return uv_loop; }
  operator       uv_t*()       noexcept  { return uv_loop; }

  explicit operator bool() const noexcept  { return (status() == 0); }  /*!< \brief Equivalent to `(status() == 0)`. */
};


//! \}
}


#include "uvcc/handle.hpp"


namespace uv
{

template< typename >
void loop::walk_cb(::uv_handle_t *_uv_handle, void *_arg)
{
  auto t = static_cast< walk_cb_pack* >(_arg);
  t->on_walk(handle(_uv_handle), t->arg);
}

}


namespace std
{

//! \ingroup g__loop
template<> inline void swap(uv::loop &_this, uv::loop &_that) noexcept  { _this.swap(_that); }

}


#endif
