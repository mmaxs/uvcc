
#ifndef UVCC_REQUEST_BASE__HPP
#define UVCC_REQUEST_BASE__HPP

#include "uvcc/utility.hpp"
#include "uvcc/thread.hpp"

#include <uv.h>
#include <cstddef>      // offsetof
#include <memory>       // addressof()
#include <functional>   // function
#include <utility>      // swap()
#include <type_traits>  // is_standard_layout


namespace uv
{


/*! \ingroup doxy_request
    \brief The base class for the libuv requests.
    \details Derived classes conceptually are just interfaces to the data stored
    in the base class, so there are no any virtual member functions.
    \sa libuv API documentation: [`uv_req_t`](http://docs.libuv.org/en/v1.x/request.html#uv-req-t-base-request). */
class request
{
public: /*types*/
  using uv_t = ::uv_req_t;
  using on_destroy_t = std::function< void(void *_data) >;
  /*!< \brief The function type of the callback called when the request object is about to be destroyed. */
  using on_request_t = null_t;

protected: /*types*/
  //! \cond
  using supplemental_data_t = empty_t;

  template< class _REQUEST_ > class instance
  {
  public: /*types*/
    using uv_t = typename _REQUEST_::uv_t;
    using on_request_t = typename _REQUEST_::on_request_t;

  private: /*types*/
    using supplemental_data_t = typename _REQUEST_::supplemental_data_t;

  private: /*data*/
    mutable tls_int uv_error;
    void (*Delete)(void*) = default_delete< instance >::Delete;  // store a proper delete operator
    ref_count rc;
    type_storage< on_destroy_t > on_destroy_storage;
    alignas(32) type_storage< on_request_t > on_request_storage;
    static_assert(sizeof(on_request_storage) <= 32, "non-static layout structure");
    alignas(32) mutable type_storage< supplemental_data_t > supplemental_data_storage;
    static_assert(sizeof(supplemental_data_storage) <= 32, "non-static layout structure");
    alignas(32) uv_t uv_req = { 0,};  // must be zeroed!

  private: /*constructors*/
    instance() = default;

  public: /*constructors*/
    ~instance() = default;

    instance(const instance&) = delete;
    instance& operator =(const instance&) = delete;

    instance(instance&&) = delete;
    instance& operator =(instance&&) = delete;

  private: /*functions*/
    void destroy()
    {
      auto &destroy_cb = on_destroy_storage.value();
      if (destroy_cb)  destroy_cb(uv_req.data);
      Delete(this);
    }

  public: /*interface*/
    static void* create()  { return std::addressof((new instance())->uv_req); }

    constexpr static instance* from(void *_uv_req) noexcept
    {
      static_assert(std::is_standard_layout< instance >::value, "not a standard layout type");
      return reinterpret_cast< instance* >(static_cast< char* >(_uv_req) - offsetof(instance, uv_req));
    }

    on_destroy_t& on_destroy() noexcept  { return on_destroy_storage.value(); }
    on_request_t& on_request() noexcept  { return on_request_storage.value(); }
    supplemental_data_t& supplemental_data() const noexcept  { return supplemental_data_storage.value(); }

    void ref()  { rc.inc(); }
    void unref()  { if (rc.dec() == 0)  destroy(); }
    ref_count::type nrefs() const noexcept  { return rc.value(); }

    tls_int& uv_status() const noexcept  { return uv_error; }
  };
  //! \endcond

protected: /*data*/
  //! \cond
  void *uv_req;
  //! \endcond

private: /*constructors*/
  explicit request(uv_t *_uv_req)
  {
    if (_uv_req)  instance< request >::from(_uv_req)->ref();
    uv_req = _uv_req;
  }

protected: /*constructors*/
  request() noexcept : uv_req(nullptr)  {}

public: /*constructors*/
  ~request()  { if (uv_req)  instance< request >::from(uv_req)->unref(); }

  request(const request &_that) : request(static_cast< uv_t* >(_that.uv_req))  {}
  request& operator =(const request &_that)
  {
    if (this != &_that)
    {
      if (_that.uv_req)  instance< request >::from(_that.uv_req)->ref();
      auto t = uv_req;
      uv_req = _that.uv_req;
      if (t)  instance< request >::from(t)->unref();
    };
    return *this;
  }

  request(request &&_that) noexcept : uv_req(_that.uv_req)  { _that.uv_req = nullptr; }
  request& operator =(request &&_that) noexcept
  {
    if (this != &_that)
    {
      auto t = uv_req;
      uv_req = _that.uv_req;
      _that.uv_req = nullptr;
      if (t)  instance< request >::from(t)->unref();
    };
    return *this;
  }

protected: /*functions*/
  //! \cond
  int uv_status(int _value) const noexcept
  {
    instance< request >::from(uv_req)->uv_status() = _value;
    return _value;
  }
  //! \endcond

public: /*interface*/
  void swap(request &_that) noexcept  { std::swap(uv_req, _that.uv_req); }
  /*! \brief The current number of existing references to the same object as this request variable refers to. */
  long nrefs() const noexcept  { return instance< request >::from(uv_req)->nrefs(); }
  /*! \brief The status value returned by the last executed libuv API function on this request. */
  int uv_status() const noexcept  { return instance< request >::from(uv_req)->uv_status(); }

  const on_destroy_t& on_destroy() const noexcept  { return instance< request >::from(uv_req)->on_destroy(); }
        on_destroy_t& on_destroy()       noexcept  { return instance< request >::from(uv_req)->on_destroy(); }

  /*! \brief The tag indicating a libuv type of the request.
      \sa libuv API documentation: [`uv_req_t.type`](http://docs.libuv.org/en/v1.x/request.html#c.uv_req_t.type). */
  ::uv_req_type type() const noexcept  { return static_cast< uv_t* >(uv_req)->type; }

  /*! \brief The pointer to the user-defined arbitrary data. libuv and uvcc does not use this field. */
  void* const& data() const noexcept  { return static_cast< uv_t* >(uv_req)->data; }
  void*      & data()       noexcept  { return static_cast< uv_t* >(uv_req)->data; }

  /*! \details Cancel a pending request.
      \sa libuv API documentation: [`uv_cancel()`](http://docs.libuv.org/en/v1.x/request.html#c.uv_cancel).*/
  int cancel() noexcept  { return ::uv_cancel(static_cast< uv_t* >(uv_req)); }

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_req); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_req); }

  explicit operator bool() const noexcept  { return (uv_status() >= 0); }  /*!< \brief Equivalent to `(uv_status() >= 0)`. */
};


}


namespace std
{

//! \ingroup doxy_request
template<> inline void swap(uv::request &_this, uv::request &_that) noexcept  { _this.swap(_that); }

}


#endif
