
#ifndef UVCC_HANDLE_BASE__HPP
#define UVCC_HANDLE_BASE__HPP

#include "uvcc/utility.hpp"
#include "uvcc/loop.hpp"

#include <uv.h>
#include <cstddef>      // offsetof
#include <functional>   // function
#include <type_traits>  // is_standard_layout
#include <utility>      // forward() swap()
#include <string>       // string
#include <memory>       // addressof()
#ifdef _WIN32
#include <io.h>         // _get_osfhandle()
#endif


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
  using uv_t = polymorphic_data_t;
  using on_destroy_t = std::function< void(void *_data) >;
  /*!< \brief The function type of the callback called when the handle has been closed and about to be destroyed.
       \sa libuv API documentation: [`uv_close_cb`](http://docs.libuv.org/en/v1.x/handle.html#c.uv_close_cb),
                                    [`uv_close()`](http://docs.libuv.org/en/v1.x/handle.html#c.uv_close). */

protected: /*types*/
  //! \cond
  struct property  /*!< \brief Basic properties for all uvcc handles. */
  {
  /*data*/
    void *uv_ptr = nullptr;
    on_destroy_t destroy_cb;

  /*constructors*/
    virtual ~property() = default;

  /*interface*/
    virtual void destroy_instance() noexcept = 0;
    virtual ::uv_handle_type type() const noexcept = 0;
    virtual ::uv_loop_t* loop() const noexcept = 0;
    virtual void*& data() noexcept = 0;
    virtual int fileno(::uv_os_fd_t&) const noexcept = 0;
  };
  struct uv_handle_t__property;  /*!< \brief Common properties for uvcc handles based on `uv_handle_t` libuv type. */
  struct uv_fs_t__property;  /*!< \brief Common properties for uvcc handles based on `uv_fs_t` libuv type. */

  template< class _HANDLE_ > struct instance
  {
  /*types*/
    using uv_t = typename _HANDLE_::uv_t;
    using property = typename _HANDLE_::property;

  /*data*/
    mutable int uv_error = 0;
    ref_count refs;
    handle::property *prop_ptr = nullptr;
    alignas(static_cast< const int >(
        greatest(alignof(::uv_any_handle), alignof(::uv_fs_t))
    )) uv_t uv_handle = { 0,};  // this handle property is brought here for instance address reconstruction

  /*constructors*/
    ~instance()  { delete prop_ptr; }
    instance()
    {
      prop_ptr = new property;
      prop_ptr->uv_ptr = std::addressof(uv_handle);
    }

    template< typename... _Args_ > instance(_Args_&&... _args)
    {
      prop_ptr = new property(std::forward< _Args_ >(_args)...);
      prop_ptr->uv_ptr = std::addressof(uv_handle);
    }

  /*interface*/
    constexpr static instance* from(void *_uv_handle) noexcept
    {
      static_assert(std::is_standard_layout< instance >::value, "not a standard layout type");
      return reinterpret_cast< instance* >(static_cast< char* >(_uv_handle) - offsetof(instance, uv_handle));
    }

    property* prop() const noexcept  { return dynamic_cast< property* >(prop_ptr); }

    void ref()  { refs.inc(); }
    void unref() noexcept  { if (refs.dec() == 0)  prop_ptr->destroy_instance(); }
  };
  //! \endcond

protected: /*data*/
  //! \cond
  void *inst_ptr;
  //! \endcond

private: /*constructors*/
  explicit handle(void *_inst_ptr)
  {
    if (_inst_ptr)  static_cast< instance< handle >* >(_inst_ptr)->ref();
    inst_ptr = _inst_ptr;
  }

protected: /*constructors*/
  handle() noexcept : inst_ptr(nullptr)  {}

public: /*constructors*/
  ~handle()  { if (inst_ptr)  static_cast< instance< handle >* >(inst_ptr)->unref(); }

  handle(const handle &_that) : handle(_that.inst_ptr)  {}
  handle& operator =(const handle &_that)
  {
    if (this != &_that)
    {
      if (_that.inst_ptr)  static_cast< instance< handle >* >(_that.inst_ptr)->ref();
      auto t = inst_ptr;
      inst_ptr = _that.inst_ptr;
      if (t)  static_cast< instance< handle >* >(t)->unref();
    };
    return *this;
  }

  handle(handle &&_that) noexcept : inst_ptr(_that.inst_ptr)  { _that.inst_ptr = nullptr; }
  handle& operator =(handle &&_that) noexcept
  {
    if (this != &_that)
    {
      auto t = inst_ptr;
      inst_ptr = _that.inst_ptr;
      _that.inst_ptr = nullptr;
      if (t)  static_cast< instance< handle >* >(t)->unref();
    };
    return *this;
  }

protected: /*functions*/
  //! \cond
  int uv_status(int _value) const noexcept
  {
    static_cast< instance< handle >* >(inst_ptr)->uv_error = _value;
    return _value;
  }

  template< class _HANDLE_ >
  instance< _HANDLE_ >* inst() const noexcept  { return static_cast< instance< _HANDLE_ >* >(inst_ptr); }
  //! \endcond

public: /*interface*/
  void swap(handle &_that) noexcept  { std::swap(inst_ptr, _that.inst_ptr); }
  /*! \brief The current number of existing references to the same object as this handle variable refers to. */
  long nrefs() const noexcept  { return inst< handle >()->refs.value(); }
  /*! \brief The status value returned by the last executed libuv API function on this handle. */
  int uv_status() const noexcept  { return inst< handle >()->uv_error; }

  const on_destroy_t& on_destroy() const noexcept  { return inst< handle >()->prop()->destroy_cb; }
        on_destroy_t& on_destroy()       noexcept  { return inst< handle >()->prop()->destroy_cb; }

  /*! \brief The tag indicating the libuv type of the handle. */
  ::uv_handle_type type() const noexcept  { return inst< handle >()->prop()->type(); }
  /*! \brief The libuv loop where the handle is running on.
      \details It is guaranteed that it will be a valid instance at least within the callback of the requests
      running with the handle. */
  uv::loop loop() const noexcept  { return uv::loop(inst< handle >()->prop()->loop()); }

  /*! \brief The pointer to the user-defined arbitrary data. libuv and uvcc does not use this field. */
  void* const& data() const noexcept  { return inst< handle >()->prop()->data(); }
  void*      & data()       noexcept  { return inst< handle >()->prop()->data(); }

  /*! \details Get the platform dependent handle/file descriptor.
      \sa libuv API documentation: [`uv_fileno()`](http://docs.libuv.org/en/v1.x/handle.html#c.uv_fileno). */
  ::uv_os_fd_t fileno() const noexcept
  {
    ::uv_os_fd_t h;
    uv_status(inst< handle >()->prop()->fileno(h));
    return h;
  }

public: /*conversion operators*/
  explicit operator bool() const noexcept  { return (uv_status() >= 0); }  /*!< \brief Equivalent to `(uv_status() >= 0)`. */
};


//! \cond
struct handle::uv_handle_t__property : virtual property
{
  template< typename = void > static void close_cb(::uv_handle_t*);

  void destroy_instance() noexcept override
  {
    auto t = static_cast< ::uv_handle_t* >(uv_ptr);
    if (::uv_is_active(t))
      ::uv_close(t, close_cb);
    else
    {
      ::uv_close(t, nullptr);
      close_cb(t);
    }
  }

  ::uv_handle_type type() const noexcept override  { return static_cast< ::uv_handle_t* >(uv_ptr)->type; }
  ::uv_loop_t* loop() const noexcept override  { return static_cast< ::uv_handle_t* >(uv_ptr)->loop; }
  void*& data() noexcept override  { return static_cast< ::uv_handle_t* >(uv_ptr)->data; }
  int fileno(::uv_os_fd_t &_h) const noexcept override
  {
#ifdef _WIN32
    _h = INVALID_HANDLE_VALUE;
#else
    _h = -1;
#endif
    return ::uv_fileno(static_cast< ::uv_handle_t* >(uv_ptr), &_h);
  }

  int is_active() const noexcept  { return ::uv_is_active(static_cast< ::uv_handle_t* >(uv_ptr));  }
  int is_closing() const noexcept { return ::uv_is_closing(static_cast< ::uv_handle_t* >(uv_ptr)); }
};

template< typename >
void handle::uv_handle_t__property::close_cb(::uv_handle_t *_uv_handle)
{
  auto inst = handle::instance< handle >::from(_uv_handle);
  auto &destroy_cb = inst->prop()->destroy_cb;
  if (destroy_cb)  destroy_cb(_uv_handle->data);
  delete inst;
}
//! \endcond



//! \cond
struct handle::uv_fs_t__property : virtual property
{
  void *user_data = nullptr;
  std::string file_path;
  ::uv_file fd = -1;

  void destroy_instance() noexcept override
  {
    if (fd >= 0)
    {
      ::uv_fs_t req_close;
      ::uv_fs_close(nullptr, &req_close, fd, nullptr);
      ::uv_fs_req_cleanup(&req_close);
    };

    if (destroy_cb)  destroy_cb(user_data);
    delete handle::instance< handle >::from(uv_ptr);
  }

  ::uv_handle_type type() const noexcept override  { return UV_FILE; }
  ::uv_loop_t* loop() const noexcept override  { return static_cast< ::uv_fs_t* >(uv_ptr)->loop; }
  void*& data() noexcept override  { return user_data; }
  int fileno(::uv_os_fd_t &_h) const noexcept override
  {
#ifdef _WIN32
    /*! \sa Windows: [`_get_osfhandle()`](https://msdn.microsoft.com/en-us/library/ks2530z6.aspx) */
    _h = fd >= 0 ? (HANDLE)::_get_osfhandle(fd) : INVALID_HANDLE_VALUE;
    return _h == INVALID_HANDLE_VALUE ? UV_EBADF : 0;
#else
    _h = static_cast< ::uv_os_fd_t >(fd);
    return _h == -1 ? UV_EBADF : 0;
#endif
  }
};
//! \endcond


}


namespace std
{

//! \ingroup doxy_handle
template<> inline void swap(uv::handle &_this, uv::handle &_that) noexcept  { _this.swap(_that); }

}


#endif
