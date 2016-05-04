
#ifndef UVCC_HANDLE_BASE__HPP
#define UVCC_HANDLE_BASE__HPP

#include "uvcc/utility.hpp"
#include "uvcc/loop.hpp"

#include <uv.h>
#include <cstddef>      // offsetof
#include <functional>   // function
#include <type_traits>  // is_standard_layout
#include <utility>      // forward() swap()
#ifdef _WIN32
#include <io.h>         // _get_osfhandle()
#endif


namespace uv
{


/*! \ingroup doxy_group_handle 
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
  using uv_t = ::uv_handle_t;
  using on_destroy_t = std::function< void(void *_data) >;
  /*!< \brief The function type of the callback called when the handle has been closed and about to be destroyed.
       \sa libuv API documentation: [`uv_close_cb`](http://docs.libuv.org/en/v1.x/handle.html#c.uv_close_cb),
                                    [`uv_close()`](http://docs.libuv.org/en/v1.x/handle.html#c.uv_close). */

protected: /*types*/
  //! \cond
  using properties = empty_t;

  struct uv_interface
  {
    virtual ~uv_interface() = default;

    virtual void destroy_instance(void*) noexcept = 0;
    virtual ::uv_handle_type type(void*) const noexcept = 0;
    virtual ::uv_loop_t* loop(void*) const noexcept = 0;
    virtual void*& data(void*) noexcept = 0;
    virtual int fileno(void*, ::uv_os_fd_t&) const noexcept = 0;
  };
  struct uv_handle_interface;
  struct uv_fs_interface;

  template< class _HANDLE_ > class instance
  {
    struct uv_t
    {
      template< typename _T_, typename = std::size_t > struct substitute  { using type = empty_t; };
      template< typename _T_ > struct substitute< _T_, decltype(sizeof(typename _T_::uv_t)) >  { using type = typename _T_::uv_t; };
      using type = typename substitute< _HANDLE_ >::type;
    };

  public: /*data*/
    mutable int uv_error = 0;
    ref_count refs;
    type_storage< on_destroy_t > destroy_cb_storage;
    aligned_storage< 64, 2 > property_storage;
    handle::uv_interface *uv_interface = nullptr;
    alignas(static_cast< const int >(
        greatest(alignof(::uv_any_handle), alignof(::uv_fs_t))
    )) typename uv_t::type uv_handle_struct = { 0,};

  private: /*constructors*/
    instance()
    {
      property_storage.reset< typename _HANDLE_::properties >();
      uv_interface = new typename _HANDLE_::uv_interface;
    }
    template< typename... _Args_ > instance(_Args_&&... _args)
    {
      property_storage.reset< typename _HANDLE_::properties >(std::forward< _Args_ >(_args)...);
      uv_interface = new typename _HANDLE_::uv_interface;
    }

  public: /* constructors*/
    ~instance()  { delete uv_interface; }

    instance(const instance&) = delete;
    instance& operator =(const instance&) = delete;

    instance(instance&&) = delete;
    instance& operator =(instance&&) = delete;

  public: /*interface*/
    static void* create()  { return &(new instance())->uv_handle_struct; }
    template< typename... _Args_ > static void* create(_Args_&&... _args)
    {
      return &(new instance(std::forward< _Args_ >(_args)...))->uv_handle_struct;
    }

    constexpr static instance* from(void *_uv_handle) noexcept
    {
      static_assert(std::is_standard_layout< instance >::value, "not a standard layout type");
      return reinterpret_cast< instance* >(static_cast< char* >(_uv_handle) - offsetof(instance, uv_handle_struct));
    }

    void ref()  { refs.inc(); }
    void unref() noexcept
    {
      if (refs.dec() == 0)  static_cast< typename _HANDLE_::uv_interface* >(uv_interface)->destroy_instance(&uv_handle_struct);
    }
  };
  //! \endcond

protected: /*data*/
  //! \cond
  void *uv_handle;
  //! \endcond

private: /*constructors*/
  explicit handle(uv_t *_uv_handle)
  {
    if (_uv_handle)  instance< handle >::from(_uv_handle)->ref();
    uv_handle = _uv_handle;
  }

protected: /*constructors*/
  handle() noexcept : uv_handle(nullptr)  {}

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
    };
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
    };
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
  /*! \brief The current number of existing references to the same object as this handle variable refers to. */
  long nrefs() const noexcept  { return instance< handle >::from(uv_handle)->refs.value(); }
  /*! \brief The status value returned by the last executed libuv API function on this handle. */
  int uv_status() const noexcept  { return instance< handle >::from(uv_handle)->uv_error; }

  const on_destroy_t& on_destroy() const noexcept  { return instance< handle >::from(uv_handle)->destroy_cb_storage.value(); }
        on_destroy_t& on_destroy()       noexcept  { return instance< handle >::from(uv_handle)->destroy_cb_storage.value(); }

  /*! \brief The tag indicating the libuv type of the handle. */
  ::uv_handle_type type() const noexcept  { return static_cast< uv_interface* >(instance< handle >::from(uv_handle)->uv_interface)->type(uv_handle); }
  /*! \brief The libuv loop where the handle is running on.
      \details It is guaranteed that it will be a valid instance at least within the callback of the requests
      running with the handle. */
  uv::loop loop() const noexcept  { return uv::loop(static_cast< uv_interface* >(instance< handle >::from(uv_handle)->uv_interface)->loop(uv_handle)); }

  /*! \brief The pointer to the user-defined arbitrary data. libuv and uvcc does not use this field. */
  void* const& data() const noexcept  { return static_cast< uv_interface* >(instance< handle >::from(uv_handle)->uv_interface)->data(uv_handle); }
  void*      & data()       noexcept  { return static_cast< uv_interface* >(instance< handle >::from(uv_handle)->uv_interface)->data(uv_handle); }
#if 0
  /*! \details Check if the handle is active.
      \sa libuv API documentation: [`uv_is_active()`](http://docs.libuv.org/en/v1.x/handle.html#c.uv_is_active). */
  int is_active() const noexcept  { return uv_status(::uv_is_active(static_cast< uv_t* >(uv_handle))); }
  /*! \details Check if the handle is closing or closed.
      \sa libuv API documentation: [`uv_is_closing()`](http://docs.libuv.org/en/v1.x/handle.html#c.uv_is_closing). */
  int is_closing() const noexcept { return uv_status(::uv_is_closing(static_cast< uv_t* >(uv_handle))); }

  /*! \details _Get_ the size of the send buffer that the operating system uses for the socket.
      \sa libuv API documentation: [`uv_send_buffer_size()`](http://docs.libuv.org/en/v1.x/handle.html#c.uv_send_buffer_size). */
  unsigned int send_buffer_size() const noexcept
  {
    unsigned int v = 0;
    uv_status(::uv_send_buffer_size(static_cast< uv_t* >(uv_handle), (int*)&v));
    return v;
  }
  /*! \details _Set_ the size of the send buffer that the operating system uses for the socket.
      \sa libuv API documentation: [`uv_send_buffer_size()`](http://docs.libuv.org/en/v1.x/handle.html#c.uv_send_buffer_size). */
  void send_buffer_size(const unsigned int _value) noexcept  { uv_status(::uv_send_buffer_size(static_cast< uv_t* >(uv_handle), (int*)&_value)); }

  /*! \details _Get_ the size of the receive buffer that the operating system uses for the socket.
      \sa libuv API documentation: [`uv_recv_buffer_size()`](http://docs.libuv.org/en/v1.x/handle.html#c.uv_recv_buffer_size). */
  unsigned int recv_buffer_size() const noexcept
  {
    unsigned int v = 0;
    uv_status(::uv_recv_buffer_size(static_cast< uv_t* >(uv_handle), (int*)&v));
    return v;
  }
  /*! \details _Set_ the size of the receive buffer that the operating system uses for the socket.
      \sa libuv API documentation: [`uv_recv_buffer_size()`](http://docs.libuv.org/en/v1.x/handle.html#c.uv_recv_buffer_size). */
  void recv_buffer_size(const unsigned int _value) noexcept  { uv_status(::uv_recv_buffer_size(static_cast< uv_t* >(uv_handle), (int*)&_value)); }
#endif
  /*! \details Get the platform dependent handle/file descriptor.
      \sa libuv API documentation: [`uv_fileno()`](http://docs.libuv.org/en/v1.x/handle.html#c.uv_fileno). */
  ::uv_os_fd_t fileno() const noexcept
  {
    ::uv_os_fd_t h;
    uv_status(static_cast< uv_interface* >(instance< handle >::from(uv_handle)->uv_interface)->fileno(uv_handle, h));
    return h;
  }

public: /*conversion operators*/
  explicit operator bool() const noexcept  { return (uv_status() >= 0); }  /*!< \brief Equivalent to `(uv_status() >= 0)`. */
};


//! \cond
struct handle::uv_handle_interface : uv_interface
{
  template< typename = void > static void close_cb(::uv_handle_t*);

  void destroy_instance(void *_uv_handle) noexcept override
  {
    auto uv_handle = static_cast< ::uv_handle_t* >(_uv_handle);
    if (::uv_is_active(uv_handle))
      ::uv_close(uv_handle, close_cb);
    else
    {
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
};

template< typename >
void handle::uv_handle_interface::close_cb(::uv_handle_t *_uv_handle)
{
  auto instance = handle::instance< handle >::from(_uv_handle);
  auto &destroy_cb = instance->destroy_cb_storage.value();
  if (destroy_cb)  destroy_cb(_uv_handle->data);
  delete instance;
}//! \endcond



//! \cond
struct handle::uv_fs_interface : uv_interface
{
  void destroy_instance(void *_uv_fs) noexcept override
  {
    auto instance = handle::instance< handle >::from(_uv_fs);
    auto uv_fs = static_cast< ::uv_fs_t* >(_uv_fs);
    auto fd = static_cast< ::uv_file >(uv_fs->result);
    
    if (fd >= 0)
    {
      ::uv_fs_t req_close;
      ::uv_fs_close(nullptr, &req_close, fd, nullptr);
      ::uv_fs_req_cleanup(&req_close);
    };

    auto &destroy_cb = instance->destroy_cb_storage.value();
    if (destroy_cb)  destroy_cb(uv_fs->data);
    delete instance;
  }

  ::uv_handle_type type(void *_uv_fs) const noexcept override  { return UV_FILE; }
  ::uv_loop_t* loop(void *_uv_fs) const noexcept override  { return static_cast< ::uv_fs_t* >(_uv_fs)->loop; }
  void*& data(void *_uv_fs) noexcept override  { return static_cast< ::uv_fs_t* >(_uv_fs)->data; }

  int fileno(void *_uv_fs, ::uv_os_fd_t &_h) const noexcept override
  {
    auto fd = static_cast< ::uv_file >(static_cast< ::uv_fs_t* >(_uv_fs)->result);
    
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
