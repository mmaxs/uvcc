
#ifndef UVCC_REQUEST_BASE__HPP
#define UVCC_REQUEST_BASE__HPP

#include "uvcc/utility.hpp"

#include <cstddef>      // size_t offsetof
#include <cstdint>      // uintptr_t
#include <cstring>      // memset()
#include <uv.h>

#include <functional>   // function
#include <type_traits>  // is_standard_layout
#include <utility>      // forward() swap()


namespace uv
{


/*! \ingroup doxy_group__request
    \brief The base class for the libuv requests.
    \details Derived classes conceptually are just interfaces to the data stored
    in the base class, so there are no any virtual member functions.
    \sa libuv API documentation: [`uv_req_t — Base request`](http://docs.libuv.org/en/v1.x/request.html#uv-req-t-base-request). */
class request
{
public: /*types*/
  using uv_t = ::uv_req_t;
  using on_destroy_t = std::function< void(void *_data) >;
  /*!< \brief The function type of the callback called when the request object is about to be destroyed. */

protected: /*types*/
  //! \cond
  using properties = empty_t;
  constexpr static const std::size_t MAX_PROPERTY_SIZE = 24 + sizeof(::sockaddr_storage);
  constexpr static const std::size_t MAX_PROPERTY_ALIGN = 8;

  template< class _Request_ > class instance
  {
    struct uv_t
    {
      template< typename _T_, typename = std::size_t > struct substitute  { using type = null_t; };
      template< typename _T_ > struct substitute< _T_, decltype(sizeof(typename _T_::uv_t)) >  { using type = typename _T_::uv_t; };
      using type = typename substitute< _Request_ >::type;
    };
    struct on_request_t
    {
      template< typename _T_, typename = std::size_t > struct substitute  { using type = std::function< void() >; };
      template< typename _T_ > struct substitute< _T_, decltype(sizeof(typename _T_::on_request_t)) >  { using type = typename _T_::on_request_t; };
      using type = typename substitute< _Request_ >::type;
    };

  public: /*data*/
    mutable int uv_error = 0;
    ref_count refs;
    type_storage< on_destroy_t > destroy_cb_storage;
    type_storage< typename on_request_t::type > request_cb_storage;  // XXX : ensure this field is of immutable layout size
    aligned_storage< MAX_PROPERTY_SIZE, MAX_PROPERTY_ALIGN > property_storage;
    //* all the fields placed before should have immutable layout size across the request class hierarchy *//
    alignas(::uv_any_req) typename uv_t::type uv_req_struct;

  private: /*constructors*/
    instance()
    {
      std::memset(&uv_req_struct, 0, sizeof(uv_req_struct));
      property_storage.reset< typename _Request_::properties >();
    }
    template< typename... _Args_ > instance(_Args_&&... _args)
    {
      std::memset(&uv_req_struct, 0, sizeof(uv_req_struct));
      property_storage.reset< typename _Request_::properties >(std::forward< _Args_ >(_args)...);
    }

  public: /*constructors*/
    ~instance() = default;

    instance(const instance&) = delete;
    instance& operator =(const instance&) = delete;

    instance(instance&&) = delete;
    instance& operator =(instance&&) = delete;

  private: /*functions*/
    void destroy_instance()
    {
      auto &destroy_cb = destroy_cb_storage.value();
      if (destroy_cb)  destroy_cb(uv_req_struct.data);

      delete this;
    }

  public: /*interface*/
    static void* create()  { return &(new instance)->uv_req_struct; }
    template< typename... _Args_ > static void* create(_Args_&&... _args)
    {
      return &(new instance(std::forward< _Args_ >(_args)...))->uv_req_struct;
    }

    constexpr static instance* from(void *_uv_req) noexcept
    {
      static_assert(std::is_standard_layout< instance >::value, "not a standard layout type");
      return reinterpret_cast< instance* >(static_cast< char* >(_uv_req) - offsetof(instance, uv_req_struct));
    }

    typename _Request_::properties& properties() noexcept
    { return property_storage.get< typename _Request_::properties >(); }

    void ref()  { refs.inc(); }
    void unref()  { if (refs.dec() == 0)  destroy_instance(); }
  };
  //! \endcond

protected: /*data*/
  //! \cond
  void *uv_req;
  //! \endcond

protected: /*constructors*/
  request() noexcept : uv_req(nullptr)  {}

  explicit request(uv_t *_uv_req)
  {
    if (_uv_req)  instance< request >::from(_uv_req)->ref();
    uv_req = _uv_req;
  }

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
    }
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
    }
    return *this;
  }

protected: /*functions*/
  //! \cond
  int uv_status(int _value) const noexcept
  {
    instance< request >::from(uv_req)->uv_error = _value;
    return _value;
  }
  //! \endcond

public: /*interface*/
  void swap(request &_that) noexcept  { std::swap(uv_req, _that.uv_req); }
  std::uintptr_t id() const noexcept  { return reinterpret_cast< std::uintptr_t >(instance< request >::from(uv_req)); }

  /*! \brief The current number of existing references to the same object as this request variable refers to. */
  long nrefs() const noexcept  { return instance< request >::from(uv_req)->refs.value(); }

  /*! \brief The status value returned by the last executed libuv API function on this request. */
  int uv_status() const noexcept  { return instance< request >::from(uv_req)->uv_error; }

  const on_destroy_t& on_destroy() const noexcept  { return instance< request >::from(uv_req)->destroy_cb_storage.value(); }
        on_destroy_t& on_destroy()       noexcept  { return instance< request >::from(uv_req)->destroy_cb_storage.value(); }

  /*! \brief The tag indicating a libuv type of the request.
      \sa libuv API documentation: [`uv_req_t.type`](http://docs.libuv.org/en/v1.x/request.html#c.uv_req_t.type). */
  ::uv_req_type type() const noexcept  { return static_cast< uv_t* >(uv_req)->type; }
  /*! \brief A string containing the name of the request type. */
  const char* type_name() const noexcept
  {
    const char *ret;

    switch (type())
    {
#define XX(X, x) case UV_##X: ret = #x; break;
      UV_REQ_TYPE_MAP(XX)
#undef XX
      default: ret = "<unknown>"; break;
    }

    return ret;
  }

  /*! \brief The pointer to the user-defined arbitrary data. libuv and uvcc does not use this field. */
  void* const& data() const noexcept  { return static_cast< uv_t* >(uv_req)->data; }
  void*      & data()       noexcept  { return static_cast< uv_t* >(uv_req)->data; }

  /*! \brief Cancel a pending request.
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

//! \ingroup doxy_group__request
template<> inline void swap(uv::request &_this, uv::request &_that) noexcept  { _this.swap(_that); }

}


#endif
