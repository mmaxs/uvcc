
#ifndef UVCC_HANDLE_BASE__HPP
#define UVCC_HANDLE_BASE__HPP

#include "uvcc/utility.hpp"
#include "uvcc/loop.hpp"

#include <uv.h>
#include <cstddef>      // offsetof
#include <functional>   // function
#include <type_traits>  // is_standard_layout
#include <utility>      // forward() swap()
#include <memory>       // addressof()


namespace uv
{


/*! \ingroup doxy_handle 
    \brief The base class for the libuv handles.
    \details Derived classes conceptually are just interfaces to the data stored
    in the base class, so there are no any virtual member functions.
    \sa libuv API documentation: [`uv_handle_t`](http://docs.libuv.org/en/v1.x/handle.html#uv-handle-t-base-handle). */
class handle
{
  //! \cond
  friend class loop;
  friend class request;
  //! \endcond

public: /*types*/
  using uv_t = null_t;
  using on_destroy_t = std::function< void(void *_data) >;
  /*!< \brief The function type of the callback called when the handle has been closed and about to be destroyed.
       \sa libuv API documentation: [`uv_close_cb`](http://docs.libuv.org/en/v1.x/handle.html#c.uv_close_cb),
                                    [`uv_close()`](http://docs.libuv.org/en/v1.x/handle.html#c.uv_close). */

protected: /*types*/
  //! \cond
  struct uv_property
  {
  /*data*/
    void *ptr = nullptr;

  /*constructors*/
    virtual ~uv_property() = default;

  /*interface*/
    virtual void destroy_instance() noexcept = 0;
    virtual ::uv_handle_type type() const noexcept = 0;
    virtual void*& data() const noexcept = 0;
  };
  struct uv_handle_property;

  template< class _HANDLE_ > struct instance
  {
  /*types*/
    using uv_t = typename _HANDLE_::uv_t;
    using property_t = typename _HANDLE_::uv_property;

  /*data*/
    mutable int uv_error = 0;
    ref_count refs;
    type_storage< on_destroy_t > on_destroy;
    property_t *property = nullptr;
    alignas(greatest(alignof(::uv_any_handle), alignof(::uv_fs_t))) uv_t uv_handle = { 0,};

  /*constructors*/
    ~instance()  { delete property; }

    template< typename... _Args_ > instance(_Args_&&... _args)
    {
      property = new property_t(std::forward< _Args_ >(_args));
      property->ptr = this;
    }

  /*interface*/
    constexpr static instance* from(void *_uv_handle) noexcept
    {
      static_assert(std::is_standard_layout< instance >::value, "not a standard layout type");
      return reinterpret_cast< instance* >(static_cast< char* >(_uv_handle) - offsetof(instance, uv_handle));
    }

    void ref()  { refs.inc(); }
    void unref() noexcept  { if (refs.dec() == 0)  property->destroy_instance(); }
  };
  //! \endcond

protected: /*data*/
  //! \cond
  void *ptr;
  //! \endcond

private: /*constructors*/
  explicit handle(void *_ptr)
  {
    if (_ptr)  static_cast< instance< handle >* >(_ptr)->ref();
    ptr = _ptr;
  }

protected: /*constructors*/
  handle() noexcept : ptr(nullptr)  {}

public: /*constructors*/
  ~handle()  { if (ptr)  static_cast< instance< handle >* >(ptr)->unref(); }

  handle(const handle &_that) : handle(_that.ptr)  {}
  handle& operator =(const handle &_that)
  {
    if (this != &_that)
    {
      if (_that.ptr)  static_cast< instance< handle >* >(_that.ptr)->ref();
      auto t = ptr;
      ptr = _that.ptr;
      if (t)  static_cast< instance< handle >* >(t)->unref();
    };
    return *this;
  }

  handle(handle &&_that) noexcept : ptr(_that.ptr)  { _that.ptr = nullptr; }
  handle& operator =(handle &&_that) noexcept
  {
    if (this != &_that)
    {
      auto t = ptr;
      ptr = _that.ptr;
      _that.ptr = nullptr;
      if (t)  static_cast< instance< handle >* >(t)->unref();
    };
    return *this;
  }

protected: /*functions*/
  //! \cond
  int uv_status(int _value) const noexcept
  {
    static_cast< instance< handle >* >(ptr)->uv_error = _value;
    return _value;
  }
  //! \endcond

public: /*interface*/
  void swap(handle &_that) noexcept  { std::swap(ptr, _that.ptr); }
  /*! \brief The current number of existing references to the same object as this handle variable refers to. */
  long nrefs() const noexcept  { return static_cast< instance< handle >* >(ptr)->refs.value(); }
  /*! \brief The status value returned by the last executed libuv API function on this handle. */
  int uv_status() const noexcept  { return static_cast< instance< handle >* >(ptr)->uv_error; }

  const on_destroy_t& on_destroy() const noexcept  { return static_cast< instance< handle >* >(ptr)->on_destroy.value(); }
        on_destroy_t& on_destroy()       noexcept  { return static_cast< instance< handle >* >(ptr)->on_destroy.value(); }

  /*! \brief The tag indicating the libuv type of the handle. */
  ::uv_handle_type type() const noexcept  { return static_cast< instance< handle >* >(ptr)->property->type(); }
  /*! \brief The libuv loop where the handle is running on.
      \details It is guaranteed that it will be a valid instance at least within the callback of the requests
      running with the handle. */
  uv::loop loop() const noexcept  { return uv::loop(static_cast< instance< handle >* >(ptr)->uv_handle.loop); }

  /*! \brief The pointer to the user-defined arbitrary data. libuv and uvcc does not use this field. */
  void* const& data() const noexcept  { return static_cast< instance< handle >* >(ptr)->property->data(); }
  void*      & data()       noexcept  { return static_cast< instance< handle >* >(ptr)->property->data(); }

public: /*conversion operators*/
  explicit operator bool() const noexcept  { return (uv_status() >= 0); }  /*!< \brief Equivalent to `(uv_status() >= 0)`. */
};



struct handle::uv_handle_property : virtual uv_property
{
  using instance = handle::instance< handle >;

  template< typename = void > static void close_cb(::uv_handle_t*);

  void destroy_instance() noexcept override
  {
    auto t = std::addressof(static_cast< instance* >(ptr)->uv_handle);
    if (::uv_is_active(t))
      ::uv_close(t, close_cb);
    else
    {
      ::uv_close(t, nullptr);
      close_cb(t);
    }
  }

  ::uv_handle_type type() const noexcept override  { return static_cast< instance* >(ptr)->uv_handle.type; }
  void*& data() const noexcept override  { return static_cast< instance* >(ptr)->uv_handle.data; }

  int is_active() const noexcept  { return ::uv_is_active(std::addressof(static_cast< instance* >(ptr)->uv_handle));  }
  int is_closing() const noexcept { return ::uv_is_closing(std::addressof(static_cast< instance* >(ptr)->uv_handle)); }
};

template< typename >
void handle::uv_handle_property::close_cb(::uv_handle_t *_uv_handle);
{
  auto ptr = instance::from(_uv_handle);
  auto &destroy_cb = ptr->on_destroy.value();
  if (destroy_cb)  destroy_cb(_uv_handle->data);
  delete ptr;
}


}


namespace std
{

//! \ingroup doxy_handle
template<> inline void swap(uv::handle &_this, uv::handle &_that) noexcept  { _this.swap(_that); }

}


#endif
