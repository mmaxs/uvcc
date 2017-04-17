
#ifndef UVCC_HANDLE_BASE__HPP
#define UVCC_HANDLE_BASE__HPP

#include "uvcc/debug.hpp"
#include "uvcc/utility.hpp"
#include "uvcc/loop.hpp"

#include <cstddef>      // size_t offsetof
#include <cstdint>      // uintptr_t
#include <uv.h>

#ifdef _WIN32
#include <io.h>         // _get_osfhandle()
#endif

#include <functional>   // function
#include <type_traits>  // is_standard_layout enable_if_t is_same
#include <utility>      // forward() swap()


#define HACK_UV_INTERFACE_PTR  1

namespace uv
{


/*! \ingroup doxy_group__handle
    \brief The base class for the libuv handles.
    \details Derived classes conceptually are just interfaces to the data stored
    in the base class, so there are no any virtual member functions.
    \sa libuv API documentation: [`uv_handle_t — Base handle`](http://docs.libuv.org/en/v1.x/handle.html#uv-handle-t-base-handle). */
class handle
{
  friend class loop;
  friend class request;

public: /*types*/
  using uv_t = ::uv_handle_t;
  using on_destroy_t = std::function< void(void *_data) >;
  /*!< \brief The function type of the callback called when the handle has been closed and about to be destroyed.
       \sa libuv API documentation: [`uv_close_cb`](http://docs.libuv.org/en/v1.x/handle.html#c.uv_close_cb),
                                    [`uv_close()`](http://docs.libuv.org/en/v1.x/handle.html#c.uv_close). */

protected: /*types*/
  //! \cond internals
  //! \addtogroup doxy_group__internals
  //! \{

  struct properties  {};
  constexpr static const std::size_t MAX_PROPERTY_SIZE = 136 + sizeof(::uv_buf_t) + sizeof(::uv_fs_t);
  constexpr static const std::size_t MAX_PROPERTY_ALIGN = 8;

  struct uv_interface
  {
    template< class _Handle_ >
    static typename _Handle_::uv_interface& instance()
    {
      static typename _Handle_::uv_interface instance;
      return instance;
    }

    virtual ~uv_interface() = default;

    virtual void close(void*) noexcept = 0;
    virtual ::uv_handle_type type(void*) const noexcept = 0;
    virtual ::uv_loop_t* loop(void*) const noexcept = 0;
    virtual void*& data(void*) noexcept = 0;
    virtual int fileno(void*, ::uv_os_fd_t&) const noexcept = 0;
    virtual int is_active(void*) const noexcept = 0;
    virtual int is_closing(void*) const noexcept = 0;
  };
  struct uv_handle_interface;
  struct uv_fs_interface;

  template< class _Handle_ > class instance
  {
    struct uv_t
    {
      template< typename _T_, typename = std::size_t > struct substitute  { using type = void; };
      template< typename _T_ > struct substitute< _T_, decltype(sizeof(typename _T_::uv_t)) >  { using type = typename _T_::uv_t; };
      template< typename _T_ > struct substitute< _T_, std::enable_if_t< std::is_same< typename _T_::uv_t, void >::value, std::size_t > >
      {
        using type = union {
            ::uv_any_handle uv_handle_data;
            ::uv_fs_t uv_fs_data;
        };
      };  // a specialization for `io` handle class
      using type = typename substitute< _Handle_ >::type;
    };

  public: /*data*/
    mutable int uv_error = 0;
    ref_count refs;
    type_storage< on_destroy_t > destroy_cb_storage;
    aligned_storage< MAX_PROPERTY_SIZE, MAX_PROPERTY_ALIGN > property_storage;
#ifndef HACK_UV_INTERFACE_PTR
    handle::uv_interface *uv_interface_ptr = nullptr;
#else
    typename _Handle_::uv_interface *uv_interface_ptr = nullptr;
#endif
    loop::instance *loop_instance_ptr = nullptr;
    //* all the fields placed before should have immutable layout size across the handle class hierarchy *//
    alignas(greatest(alignof(::uv_any_handle), alignof(::uv_fs_t))) typename uv_t::type uv_handle_struct = { 0,};

  private: /*constructors*/
    instance()
    {
      property_storage.reset< typename _Handle_::properties >();
      uv_interface_ptr = &uv_interface::instance< _Handle_ >();
      uvcc_debug_function_return("instance [0x%08tX] for handle [0x%08tX]", (ptrdiff_t)this, (ptrdiff_t)&uv_handle_struct);
    }
    template< typename... _Args_ > instance(_Args_&&... _args)
    {
      property_storage.reset< typename _Handle_::properties >(std::forward< _Args_ >(_args)...);
      uv_interface_ptr = &uv_interface::instance< _Handle_ >();
      uvcc_debug_function_return("instance [0x%08tX] for handle [0x%08tX]", (ptrdiff_t)this, (ptrdiff_t)&uv_handle_struct);
    }

  public: /* constructors*/
    ~instance()
    {
      uvcc_debug_function_enter("instance [0x%08tX] for handle [0x%08tX]", (ptrdiff_t)this, (ptrdiff_t)&uv_handle_struct);
      unbook_loop();
    }

    instance(const instance&) = delete;
    instance& operator =(const instance&) = delete;

    instance(instance&&) = delete;
    instance& operator =(instance&&) = delete;

  public: /*interface*/
    static void* create()  { return &(new instance)->uv_handle_struct; }
    template< typename... _Args_ > static void* create(_Args_&&... _args)
    {
      return &(new instance(std::forward< _Args_ >(_args)...))->uv_handle_struct;
    }

    constexpr static instance* from(void *_uv_handle) noexcept
    {
      static_assert(std::is_standard_layout< instance >::value, "not a standard layout type");
      return reinterpret_cast< instance* >(static_cast< char* >(_uv_handle) - offsetof(instance, uv_handle_struct));
    }

    typename _Handle_::properties& properties() noexcept
    { return property_storage.get< typename _Handle_::properties >(); }

    typename _Handle_::uv_interface* uv_interface() const noexcept
#ifndef HACK_UV_INTERFACE_PTR
    { return dynamic_cast/* from a virtual base */< typename _Handle_::uv_interface* >(uv_interface_ptr); }
#else
    /* any uv_interface subclass is a __vptr only object with no any data members,
       there is also no needs to switch between the actual __vtable targets
       as far as uv_interface subclasses override only not defined pure virtual functions from their base classes,
       so we can just use __vptr from the concrete uv_interface leaf subclass object
       (and	simply use it according to the one of the desired uv_interface base class context if ever necessary) */
    { return uv_interface_ptr; }
#endif

    void ref()
    {
      uvcc_debug_function_enter("handle [0x%08tX]", (ptrdiff_t)&uv_handle_struct);
      refs.inc();
    }
    void unref()
    {
      uvcc_debug_function_enter("handle [0x%08tX]", (ptrdiff_t)&uv_handle_struct);
      auto nrefs = refs.dec();
      uvcc_debug_condition(nrefs == 0, "{nrefs to handle [0x%08tX]}=%li", (ptrdiff_t)&uv_handle_struct, nrefs);
      if (nrefs == 0)  uv_interface_ptr->close(&uv_handle_struct);
    }

    void book_loop()
    {
      uvcc_debug_function_enter(
          "handle [0x%08tX], loop [0x%08tX]", (ptrdiff_t)&uv_handle_struct, (ptrdiff_t)uv_interface_ptr->loop(&uv_handle_struct)
      );
      unbook_loop();
      loop_instance_ptr = loop::instance::from(uv_interface_ptr->loop(&uv_handle_struct));
      loop_instance_ptr->ref();
    }
    void unbook_loop()
    {
      uvcc_debug_do_if(loop_instance_ptr,
          uvcc_debug_function_enter("handle [0x%08tX], loop [0x%08tX]", (ptrdiff_t)&uv_handle_struct, (ptrdiff_t)&loop_instance_ptr->uv_loop_struct)
      );
      if (loop_instance_ptr)
      {
        loop_instance_ptr->unref();
        loop_instance_ptr = nullptr;
      }
    }
  };
  //! \cond
  template< class _Handle_ > friend typename _Handle_::instance* debug::instance(_Handle_&) noexcept;
  //! \endcond

  //! \}
  //! \endcond

protected: /*data*/
  //! \cond
  void *uv_handle;
  //! \endcond

protected: /*constructors*/
  //! \cond
  handle() noexcept : uv_handle(nullptr)  {}

  explicit handle(uv_t *_uv_handle)
  {
    if (_uv_handle)  instance< handle >::from(_uv_handle)->ref();
    uv_handle = _uv_handle;
  }
  //! \endcond

public: /*constructors*/
  ~handle()  { if (uv_handle)  instance< handle >::from(uv_handle)->unref(); }

  handle(const handle &_that) : handle(static_cast< uv_t* >(_that.uv_handle))  {}
  handle& operator =(const handle &_that)
  {
    if (this != &_that)
    {
      if (_that.uv_handle)  instance< handle >::from(_that.uv_handle)->ref();
      auto t = uv_handle;
      uv_handle = _that.uv_handle;
      if (t)  instance< handle >::from(t)->unref();
    }
    return *this;
  }

  handle(handle &&_that) noexcept : uv_handle(_that.uv_handle)  { _that.uv_handle = nullptr; }
  handle& operator =(handle &&_that) noexcept
  {
    if (this != &_that)
    {
      auto t = uv_handle;
      uv_handle = _that.uv_handle;
      _that.uv_handle = nullptr;
      if (t)  instance< handle >::from(t)->unref();
    }
    return *this;
  }

protected: /*functions*/
  //! \cond
  int uv_status(int _value) const noexcept
  {
    instance< handle >::from(uv_handle)->uv_error = _value;
    return _value;
  }
  //! \endcond

public: /*interface*/
  void swap(handle &_that) noexcept  { std::swap(uv_handle, _that.uv_handle); }
  std::uintptr_t id() const noexcept  { return reinterpret_cast< std::uintptr_t >(instance< handle >::from(uv_handle)); }

  /*! \brief The current number of existing references to the same object as this handle variable refers to. */
  long nrefs() const noexcept  { return instance< handle >::from(uv_handle)->refs.get_value(); }

  /*! \brief The status value returned by the last executed libuv API function on this handle. */
  int uv_status() const noexcept  { return instance< handle >::from(uv_handle)->uv_error; }

  const on_destroy_t& on_destroy() const noexcept  { return instance< handle >::from(uv_handle)->destroy_cb_storage.value(); }
        on_destroy_t& on_destroy()       noexcept  { return instance< handle >::from(uv_handle)->destroy_cb_storage.value(); }

  /*! \brief The tag indicating the libuv type of the handle. */
  ::uv_handle_type type() const noexcept  { return instance< handle >::from(uv_handle)->uv_interface()->type(uv_handle); }
  /*! \brief A string containing the name of the handle type. */
  const char* type_name() const noexcept
  {
    const char *ret;

    switch (type())
    {
#define XX(X, x) case UV_##X: ret = #x; break;
      UV_HANDLE_TYPE_MAP(XX)
#undef XX
      case UV_FILE: ret = "file"; break;
      default: ret = "<unknown>"; break;
    }

    return ret;
  }

  /*! \brief The libuv loop where the handle is running on.
      \details It is guaranteed that it will be a valid instance at least within the callback of the requests
      running on the handle. */
  uv::loop loop() const noexcept  { return uv::loop(instance< handle >::from(uv_handle)->uv_interface()->loop(uv_handle)); }

  /*! \brief The pointer to the user-defined arbitrary data. libuv and uvcc does not use this field. */
  void* const& data() const noexcept  { return instance< handle >::from(uv_handle)->uv_interface()->data(uv_handle); }
  void*      & data()       noexcept  { return instance< handle >::from(uv_handle)->uv_interface()->data(uv_handle); }

  /*! \brief Check if the handle is active.
      \sa libuv API documentation: [`uv_is_active()`](http://docs.libuv.org/en/v1.x/handle.html#c.uv_is_active). */
  int is_active() const noexcept
  { return uv_status(instance< handle >::from(uv_handle)->uv_interface()->is_active(uv_handle)); }
  /*! \brief Check if the handle is closing or closed.
      \sa libuv API documentation: [`uv_is_closing()`](http://docs.libuv.org/en/v1.x/handle.html#c.uv_is_closing). */
  int is_closing() const noexcept
  { return uv_status(instance< handle >::from(uv_handle)->uv_interface()->is_closing(uv_handle)); }

#if 0
  /*! \brief _Get_ the size of the send buffer that the operating system uses for the socket.
      \sa libuv API documentation: [`uv_send_buffer_size()`](http://docs.libuv.org/en/v1.x/handle.html#c.uv_send_buffer_size). */
  unsigned int send_buffer_size() const noexcept
  {
    unsigned int v = 0;
    uv_status(::uv_send_buffer_size(static_cast< uv_t* >(uv_handle), (int*)&v));
    return v;
  }
  /*! \brief _Set_ the size of the send buffer that the operating system uses for the socket.
      \sa libuv API documentation: [`uv_send_buffer_size()`](http://docs.libuv.org/en/v1.x/handle.html#c.uv_send_buffer_size). */
  void send_buffer_size(unsigned int _value) noexcept  { uv_status(::uv_send_buffer_size(static_cast< uv_t* >(uv_handle), (int*)&_value)); }

  /*! \brief _Get_ the size of the receive buffer that the operating system uses for the socket.
      \sa libuv API documentation: [`uv_recv_buffer_size()`](http://docs.libuv.org/en/v1.x/handle.html#c.uv_recv_buffer_size). */
  unsigned int recv_buffer_size() const noexcept
  {
    unsigned int v = 0;
    uv_status(::uv_recv_buffer_size(static_cast< uv_t* >(uv_handle), (int*)&v));
    return v;
  }
  /*! \brief _Set_ the size of the receive buffer that the operating system uses for the socket.
      \sa libuv API documentation: [`uv_recv_buffer_size()`](http://docs.libuv.org/en/v1.x/handle.html#c.uv_recv_buffer_size). */
  void recv_buffer_size(unsigned int _value) noexcept  { uv_status(::uv_recv_buffer_size(static_cast< uv_t* >(uv_handle), (int*)&_value)); }
#endif

  /*! \brief Get the platform dependent handle/file descriptor.
      \sa libuv API documentation: [`uv_fileno()`](http://docs.libuv.org/en/v1.x/handle.html#c.uv_fileno). */
  ::uv_os_fd_t fileno() const noexcept
  {
    ::uv_os_fd_t h;
    uv_status(instance< handle >::from(uv_handle)->uv_interface()->fileno(uv_handle, h));
    return h;
  }

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return instance< handle >::from(uv_handle)->uv_interface()->type(uv_handle) == UV_FILE ? nullptr : static_cast< const uv_t* >(uv_handle); }
  explicit operator       uv_t*()       noexcept  { return instance< handle >::from(uv_handle)->uv_interface()->type(uv_handle) == UV_FILE ? nullptr : static_cast<       uv_t* >(uv_handle); }

  explicit operator bool() const noexcept  { return (uv_status() >= 0); }  /*!< \brief Equivalent to `(uv_status() >= 0)`. */

public: /*comparison operators*/
  friend bool operator ==(const handle &_lhs, const handle _rhs) noexcept  { return (_lhs.uv_handle == _rhs.uv_handle); }
  friend bool operator !=(const handle &_lhs, const handle _rhs) noexcept  { return !(_lhs == _rhs); }
};


//! \cond internals
//! \addtogroup doxy_group__internals
//! \{
struct handle::uv_handle_interface : virtual uv_interface
{
  template< typename = void > static void close_cb(::uv_handle_t*);

  void close(void *_uv_handle) noexcept override
  {
    auto uv_handle = static_cast< ::uv_handle_t* >(_uv_handle);
    uvcc_debug_function_enter("%s handle [0x%08tX]", debug::handle_type_name(uv_handle), (ptrdiff_t)uv_handle);

    /*auto loop_alive = ::uv_loop_alive(uv_handle->loop);
    uvcc_debug_condition(loop_alive, "is loop [0x%08tX] (stop_flag=%u) associated with handle [0x%08tX] alive", (ptrdiff_t)uv_handle->loop, uv_handle->loop->stop_flag, (ptrdiff_t)uv_handle);
    if (loop_alive)
    {
      uvcc_debug_log_if(true, "handle [0x%08tX]: call close callback asynchronously", (ptrdiff_t)uv_handle);
      ::uv_close(uv_handle, close_cb);
    }
    else*/
    // the (loop_alive == 1) state or (stop_flag == 0) doesn't mean that loop is running or will be running,
    // therefore don't let the destroy procedure to rely on the loop at all
    {
      uvcc_debug_log_if(true, "handle [0x%08tX]: call close callback synchronously", (ptrdiff_t)uv_handle);
      ::uv_close(uv_handle, nullptr);
      close_cb(uv_handle);
    }
  }

  ::uv_handle_type type(void *_uv_handle) const noexcept override  { return static_cast< ::uv_handle_t* >(_uv_handle)->type; }
  ::uv_loop_t* loop(void *_uv_handle) const noexcept override  { return static_cast< ::uv_handle_t* >(_uv_handle)->loop; }
  void*& data(void *_uv_handle) noexcept override  { return static_cast< ::uv_handle_t* >(_uv_handle)->data; }
  
  int fileno(void *_uv_handle, ::uv_os_fd_t &_h) const noexcept override
  {
#ifdef _WIN32
    _h = INVALID_HANDLE_VALUE;
#else
    _h = -1;
#endif
    return ::uv_fileno(static_cast< ::uv_handle_t* >(_uv_handle), &_h);
  }

  int is_active(void *_uv_handle) const noexcept override
  { return ::uv_is_active(static_cast< ::uv_handle_t* >(_uv_handle)); }
  int is_closing(void *_uv_handle) const noexcept override
  { return ::uv_is_closing(static_cast< ::uv_handle_t* >(_uv_handle)); }
};
//! \}
//! \endcond

template< typename >
void handle::uv_handle_interface::close_cb(::uv_handle_t *_uv_handle)
{
  uvcc_debug_function_enter("%s handle [0x%08tX]", debug::handle_type_name(_uv_handle), (ptrdiff_t)_uv_handle);

  auto instance_ptr = handle::instance< handle >::from(_uv_handle);

  auto &destroy_cb = instance_ptr->destroy_cb_storage.value();
  if (destroy_cb)  destroy_cb(_uv_handle->data);

  delete instance_ptr;
}


//! \cond internals
//! \addtogroup doxy_group__internals
//! \{
struct handle::uv_fs_interface : virtual uv_interface
{
  void close(void *_uv_fs) noexcept override
  {
    uvcc_debug_function_enter("fs handle [0x%08tX]", (ptrdiff_t)_uv_fs);

    auto instance_ptr = handle::instance< handle >::from(_uv_fs);
    auto uv_fs = static_cast< ::uv_fs_t* >(_uv_fs);
    auto fd = static_cast< ::uv_file >(uv_fs->result);
    
    if (fd >= 0)
    {
      ::uv_fs_t req_close;
      ::uv_fs_close(nullptr, &req_close, fd, nullptr);  // XXX : nullptr for loop
      ::uv_fs_req_cleanup(&req_close);
    }

    auto &destroy_cb = instance_ptr->destroy_cb_storage.value();
    if (destroy_cb)  destroy_cb(uv_fs->data);

    ::uv_fs_req_cleanup(uv_fs);
    delete instance_ptr;
  }

  ::uv_handle_type type(void *_uv_fs) const noexcept override  { return UV_FILE; }
  ::uv_loop_t* loop(void *_uv_fs) const noexcept override  { return static_cast< ::uv_fs_t* >(_uv_fs)->loop; }
  void*& data(void *_uv_fs) noexcept override  { return static_cast< ::uv_fs_t* >(_uv_fs)->data; }

  int fileno(void *_uv_fs, ::uv_os_fd_t &_h) const noexcept override
  {
    auto fd = static_cast< ::uv_file >(static_cast< ::uv_fs_t* >(_uv_fs)->result);
    
#ifdef _WIN32
    /*! \sa Windows: [`_get_osfhandle()`](https://msdn.microsoft.com/en-us/library/ks2530z6.aspx). */
    _h = fd >= 0 ? (HANDLE)::_get_osfhandle(fd) : INVALID_HANDLE_VALUE;
    return _h == INVALID_HANDLE_VALUE ? UV_EBADF : 0;
#else
    _h = static_cast< ::uv_os_fd_t >(fd);
    return _h == -1 ? UV_EBADF : 0;
#endif
  }

  int is_active(void *_uv_fs) const noexcept override  { return 0; }
};
//! \}
//! \endcond


}


namespace std
{

//! \ingroup doxy_group__handle
template<> inline void swap(uv::handle &_this, uv::handle &_that) noexcept  { _this.swap(_that); }

}


#endif
