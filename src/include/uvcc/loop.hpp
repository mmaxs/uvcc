
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
/*! \defgroup g__loop Event loop
    \brief The I/O event loop.
    \sa libuv documentation: [the I/O loop](http://docs.libuv.org/en/v1.x/design.html#the-i-o-loop). */
//! \{


class handle;


/*! \brief The I/O event loop class.
    \details All event loops (including default one) are the instances of this class.
    \sa libuv documentation: [`uv_loop_t`](http://docs.libuv.org/en/v1.x/loop.html#uv-loop-t-event-loop). */
class loop
{
public: /*types*/
  using uv_t = ::uv_loop_t;
  using on_destroy_t = std::function< void(void*) >;  /*!< \brief The function type of the callback called when the loop instance is about to be destroyed. */
  using on_walk_t = std::function< void(handle, void*) >;
  /*!< \details The function type of the callback called by the `walk()` function.
       \sa libuv documentation: [`uv_walk_cb`](http://docs.libuv.org/en/v1.x/loop.html#c.uv_walk_cb),
                                [`uv_walk()`](http://docs.libuv.org/en/v1.x/loop.html#c.uv_walk). */

private: /*types*/
  class instance
  {
  public: /*types*/
    class uv_base
    {
    private:
      uv_t value;
      uv_t* ptr;

    public:
      ~uv_base()  { ::uv_loop_close(ptr); }

      uv_base() noexcept : ptr(&value)  { ::uv_loop_init(ptr); }
      explicit uv_base(uv_t *_uv_ptr) noexcept : ptr(_uv_ptr)  {}

      uv_t* operator ->() const noexcept  { return ptr; }

    public:
      operator const uv_t*() const noexcept  { return ptr; }
      operator       uv_t*()       noexcept  { return ptr; }
    };

  private: /*data*/
    ref_count rc;
    type_storage< on_destroy_t > on_destroy_storage;
    uv_base uv_loop;

  private: /*constructors*/
    instance() = default;
    explicit instance(uv_t *_uv_ptr) : uv_loop(_uv_ptr)  {};

  public: /*constructors*/
    ~instance() = default;

    instance(const instance&) = delete;
    instance& operator =(const instance&) = delete;

    instance(instance&&) = delete;
    instance& operator =(instance&&) = delete;

  private: /*functions*/
    void destroy()
    {
      on_destroy_t &f = on_destroy_storage.value();
      if (f)  f(uv_loop->data);
      delete this;
    }

  public: /*interface*/
    static uv_base* create()  { return std::addressof((new instance())->uv_loop); }
    static uv_base* create(uv_t *_uv_ptr)  { return std::addressof((new instance(_uv_ptr))->uv_loop); }

    constexpr static instance* from(uv_base *_uv_loop) noexcept
    {
      static_assert(std::is_standard_layout< instance >::value, "not a standard layout type");
      return reinterpret_cast< instance* >(reinterpret_cast< char* >(_uv_loop) - offsetof(instance, uv_loop));
    }

    on_destroy_t& on_destroy() noexcept  { return on_destroy_storage.value(); }

    void ref()  { rc.inc(); }
    void unref()  { if (rc.dec() == 0)  destroy(); }
    ref_count::type nrefs() const noexcept  { return rc.value(); }
  };

private: /*data*/
  instance::uv_base *uv_loop;

private: /*constructors*/
  explicit loop(instance::uv_base *_uv_loop)
  {
    instance::from(_uv_loop)->ref();
    uv_loop = _uv_loop;
  }
  explicit loop(uv_t *_uv_ptr) : uv_loop(instance::create(_uv_ptr))  {}

public: /*constructors*/
  ~loop()  { if (uv_loop)  instance::from(uv_loop)->unref(); }

  /*! \brief Create a new event loop. */
  loop() : uv_loop(instance::create())  {}

  loop(const loop &_l)
  {
    instance::from(_l.uv_loop)->ref();
    uv_loop = _l.uv_loop; 
  }
  loop& operator =(const loop &_l)
  {
    if (this != &_l)
    {
      instance::from(_l.uv_loop)->ref();
      auto uv_l = uv_loop;
      uv_loop = _l.uv_loop; 
      instance::from(uv_l)->unref();
    };
    return *this;
  }

  loop(loop &&_l) noexcept : uv_loop(_l.uv_loop)  { _l.uv_loop = nullptr; }
  loop& operator =(loop &&_l) noexcept
  {
    if (this != &_l)
    {
      auto uv_l = uv_loop;
      uv_loop = _l.uv_loop;
      _l.uv_loop = nullptr;
      instance::from(uv_l)->unref();
    };
    return *this;
  }

public: /*interface*/
  static loop Default()
  {
    static loop default_loop(::uv_default_loop());
    return default_loop;
  }

  void swap(loop &_l) noexcept  { std::swap(uv_loop, _l.uv_loop); }
  /*! \brief The current number of existing references to the same loop as this variable refers to. */
  long nrefs() const noexcept  { return instance::from(uv_loop)->nrefs(); }

  const on_destroy_t& on_destroy() const noexcept  { return instance::from(uv_loop)->on_destroy(); }
        on_destroy_t& on_destroy()       noexcept  { return instance::from(uv_loop)->on_destroy(); }

  /*! \details The pointer to the user-defined arbitrary data.
      \sa libuv documentation: [`uv_loop_t.data`](http://docs.libuv.org/en/v1.x/loop.html#c.uv_loop_t.data). */
  void* const& data() const noexcept  { return (*uv_loop)->data; }
  void*      & data()       noexcept  { return (*uv_loop)->data; }

  /*! \details Set additional loop options.
      \sa libuv documentation: [`uv_loop_configure()`](http://docs.libuv.org/en/v1.x/loop.html#c.uv_loop_configure). */
  template< typename... _Args_ > int configure(::uv_loop_option _opt, _Args_&&... _args)
  {
    return ::uv_loop_configure(uv_loop, _opt, std::forward< _Args_ >(_args)...);
  }

public: /*conversion operators*/
  operator const uv_t*() const noexcept  { return *uv_loop; }
  operator       uv_t*()       noexcept  { return *uv_loop; }

  //explicit operator bool() const noexcept  { return is_active(); }  /*!< \details Equivalent to `is_alive()`. */
};


//! \}
}


namespace std
{

//! \ingroup g__loop
template<> void swap(uv::loop &_this, uv::loop &_that) noexcept  { _this.swap(_that); }

}


#endif
