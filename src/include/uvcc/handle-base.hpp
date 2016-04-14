
#ifndef UVCC_HANDLE_BASE__HPP
#define UVCC_HANDLE_BASE__HPP

#include "uvcc/utility.hpp"
#include "uvcc/loop.hpp"
#include "uvcc/thread.hpp"

#include <uv.h>
#include <cstddef>      // offsetof
#include <functional>   // function
#include <type_traits>  // is_standard_layout
#include <utility>      // swap()
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
  using uv_t = ::uv_handle_t;
  using on_destroy_t = std::function< void(void *_data) >;
  /*!< \brief The function type of the callback called when the handle has been closed and about to be destroyed.
       \sa libuv API documentation: [`uv_close_cb`](http://docs.libuv.org/en/v1.x/handle.html#c.uv_close_cb),
                                    [`uv_close()`](http://docs.libuv.org/en/v1.x/handle.html#c.uv_close). */

protected: /*types*/
  //! \cond
  using supplemental_data_t = empty_t;

  template< class _HANDLE_ > class instance
  {
  public: /*types*/
    using uv_t = typename _HANDLE_::uv_t;

  private: /*types*/
    using supplemental_data_t = typename _HANDLE_::supplemental_data_t;

  private: /*data*/
    mutable tls_int uv_error;
    void (*Delete)(void*) = default_delete< instance >::Delete;  // store a proper delete operator
    ref_count rc;
    type_storage< on_destroy_t > on_destroy_storage;
    mutable any_ptr supplemental_data_ptr;
    alignas(::uv_any_handle) uv_t uv_handle = { 0,};

  private: /*constructors*/
    instance() = default;

  public: /*constructors*/
    ~instance() = default;

    instance(const instance&) = delete;
    instance& operator =(const instance&) = delete;

    instance(instance&&) = delete;
    instance& operator =(instance&&) = delete;

  private: /*functions*/
    static void close_cb(::uv_handle_t*);

    void destroy()
    {
      auto t = reinterpret_cast< ::uv_handle_t* >(&uv_handle);
      if (::uv_is_active(t))
        ::uv_close(t, close_cb);
      else
      {
        ::uv_close(t, nullptr);
        close_cb(t);
      }
    }

  public: /*interface*/
    static void* create()  { return std::addressof((new instance())->uv_handle); }

    constexpr static instance* from(void *_uv_handle) noexcept
    {
      static_assert(std::is_standard_layout< instance >::value, "not a standard layout type");
      return reinterpret_cast< instance* >(static_cast< char* >(_uv_handle) - offsetof(instance, uv_handle));
    }

    on_destroy_t& on_destroy() noexcept  { return on_destroy_storage.value(); }
    supplemental_data_t& supplemental_data() const noexcept
    {
      if (!supplemental_data_ptr)  supplemental_data_ptr.reset(new supplemental_data_t);
      return *supplemental_data_ptr.get< supplemental_data_t >();
    }

    void ref()  { rc.inc(); }
    void unref() noexcept  { if (rc.dec() == 0)  destroy(); }
    ref_count::type nrefs() const noexcept  { return rc.value(); }

    tls_int& uv_status() const noexcept  { return uv_error; }
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
    instance< handle >::from(uv_handle)->uv_status() = _value;
    return _value;
  }
  //! \endcond

public: /*interface*/
  void swap(handle &_that) noexcept  { std::swap(uv_handle, _that.uv_handle); }
  /*! \brief The current number of existing references to the same object as this handle variable refers to. */
  long nrefs() const noexcept  { return instance< handle >::from(uv_handle)->nrefs(); }
  /*! \brief The status value returned by the last executed libuv API function on this handle. */
  int uv_status() const noexcept  { return instance< handle >::from(uv_handle)->uv_status(); }

  const on_destroy_t& on_destroy() const noexcept  { return instance< handle >::from(uv_handle)->on_destroy(); }
        on_destroy_t& on_destroy()       noexcept  { return instance< handle >::from(uv_handle)->on_destroy(); }

  /*! \brief The tag indicating the libuv type of the handle. */
  ::uv_handle_type type() const noexcept  { return static_cast< uv_t* >(uv_handle)->type; }
  /*! \brief The libuv loop where the handle is running on.
      \details It is guaranteed that it will be a valid instance at least within the callback of the requests
      running with the handle. */
  uv::loop loop() const noexcept  { return uv::loop(static_cast< uv_t* >(uv_handle)->loop); }

  /*! \brief The pointer to the user-defined arbitrary data. libuv and uvcc does not use this field. */
  void* const& data() const noexcept  { return static_cast< uv_t* >(uv_handle)->data; }
  void*      & data()       noexcept  { return static_cast< uv_t* >(uv_handle)->data; }

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

  /*! \details Get the platform dependent handle/file descriptor.
      \sa libuv API documentation: [`uv_fileno()`](http://docs.libuv.org/en/v1.x/handle.html#c.uv_fileno). */
  ::uv_os_fd_t fileno() const noexcept
  {
#ifdef _WIN32
    ::uv_os_fd_t h = INVALID_HANDLE_VALUE;
#else
    ::uv_os_fd_t h = -1;
#endif
    uv_status(::uv_fileno(static_cast< uv_t* >(uv_handle), &h));
    return h;
  }

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_handle); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_handle); }

  explicit operator bool() const noexcept  { return (uv_status() >= 0); }  /*!< \brief Equivalent to `(uv_status() >= 0)`. */
};


//! \cond
template< class _HANDLE_ >
void handle::instance< _HANDLE_ >::close_cb(::uv_handle_t *_uv_handle)
{
  auto self = from(_uv_handle);
  auto &destroy_cb = self->on_destroy();
  if (destroy_cb)  destroy_cb(_uv_handle->data);
  self->Delete(self);
}
//! \endcond


}


namespace std
{

//! \ingroup doxy_handle
template<> inline void swap(uv::handle &_this, uv::handle &_that) noexcept  { _this.swap(_that); }

}


#endif
