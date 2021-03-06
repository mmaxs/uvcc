
#ifndef UVCC_REQUEST_DNS__HPP
#define UVCC_REQUEST_DNS__HPP

#include "uvcc/utility.hpp"
#include "uvcc/request-base.hpp"
#include "uvcc/loop.hpp"

#include <uv.h>

#include <functional>   // function
#include <type_traits>  // enable_if_t


namespace uv
{


/*! \ingroup doxy_group__request
    \brief Getaddrinfo request type.
    \sa libuv API documentation: [DNS utility functions](http://docs.libuv.org/en/v1.x/dns.html#dns-utility-functions). */
class getaddrinfo : public request
{
  //! \cond
  friend class request::instance< getaddrinfo >;
  //! \endcond

public: /*types*/
  using uv_t = ::uv_getaddrinfo_t;
  using on_request_t = std::function< void(getaddrinfo _request) >;
  /*!< \brief The function type of the callback called with the `getaddrinfo` request result once complete.
       \sa libuv API documentation: [`uv_getaddrinfo_cb`](http://docs.libuv.org/en/v1.x/dns.html#c.uv_getaddrinfo_cb). */

protected: /*types*/
  //! \cond internals
  //! \addtogroup doxy_group__internals
  //! \{
  struct properties : request::properties
  {
    uv_t *uv_req = nullptr;
    ~properties()  { if (uv_req)  ::uv_freeaddrinfo(uv_req->addrinfo); }
  };
  //! \}
  //! \endcond

private: /*types*/
  using instance = request::instance< getaddrinfo >;

protected: /*constructors*/
  //! \cond
  explicit getaddrinfo(uv_t *_uv_req) : request(reinterpret_cast< request::uv_t* >(_uv_req))  {}
  //! \endcond

public: /*constructors*/
  ~getaddrinfo() = default;
  getaddrinfo()
  {
    uv_req = instance::create();
    static_cast< uv_t* >(uv_req)->type = UV_GETADDRINFO;
    static_cast< uv_t* >(uv_req)->addrinfo = nullptr;  // ensure for desired initial value
    instance::from(uv_req)->properties().uv_req = static_cast< uv_t* >(uv_req);
  }

  getaddrinfo(const getaddrinfo&) = default;
  getaddrinfo& operator =(const getaddrinfo&) = default;

  getaddrinfo(getaddrinfo&&) noexcept = default;
  getaddrinfo& operator =(getaddrinfo&&) noexcept = default;

private: /*functions*/
  template< typename = void > static void getaddrinfo_cb(::uv_getaddrinfo_t*, int, ::addrinfo*);

  int run_(uv::loop &_loop, const char *_hostname, const char *_service, const ::addrinfo *_hints)
  {
    ::uv_freeaddrinfo(static_cast< uv_t* >(uv_req)->addrinfo);  // assuming that *uv_req or this particular field has initially been nulled

    auto instance_ptr = instance::from(uv_req);

    if (!instance_ptr->request_cb_storage.value())
    {
      return uv_status(::uv_getaddrinfo(
          static_cast< uv::loop::uv_t* >(_loop), static_cast< uv_t* >(uv_req),
          nullptr,
          _hostname, _service, _hints
      ));
    }
    else
    {
      instance_ptr->ref();

      uv_status(0);
      auto uv_ret = ::uv_getaddrinfo(
          static_cast< uv::loop::uv_t* >(_loop), static_cast< uv_t* >(uv_req),
          getaddrinfo_cb,
          _hostname, _service, _hints
      );
      if (uv_ret < 0)
      {
        uv_status(uv_ret);
        instance_ptr->unref();
      }

      return uv_ret;
    }
  }

public: /*interface*/
  on_request_t& on_request() const noexcept  { return instance::from(uv_req)->request_cb_storage.value(); }

  /*! \brief The libuv loop that started this `getaddrinfo` request and where completion will be reported.
      \details It is guaranteed that it will be a valid instance at least within the request callback. */
  uv::loop loop() const noexcept  { return uv::loop(static_cast< uv_t* >(uv_req)->loop); }

  /*! \brief The pointer to a `struct addrinfo` containing the request result. */
  const ::addrinfo* addrinfo() const noexcept  { return static_cast< uv_t* >(uv_req)->addrinfo; }

  /*! \brief Run the request.
      \details For supplying `_hints` argument the appropriate helper function for \ref doxy_group__netstruct can be utilized.
      \sa libuv API documentation: [`uv_getaddrinfo()`](http://docs.libuv.org/en/v1.x/dns.html#c.uv_getaddrinfo).
      \note If the request callback is empty (has not been set), the request runs _synchronously_
      (and `_loop` parameter is ignored). */
  int run(uv::loop &_loop, const char *_hostname, const char *_service, const ::addrinfo &_hints)
  {
    return run_(_loop, _hostname, _service, &_hints);
  }
  /*! \brief Idem with empty `_hints`. */
  int run(uv::loop &_loop, const char *_hostname, const char *_service)
  {
    return run_(_loop, _hostname, _service, nullptr);
  }

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_req); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_req); }
};

template< typename >
void getaddrinfo::getaddrinfo_cb(::uv_getaddrinfo_t *_uv_req, int _status, ::addrinfo *_result)
{
  auto instance_ptr = instance::from(_uv_req);
  instance_ptr->uv_error = _status;

  ref_guard< instance > unref_req(*instance_ptr, adopt_ref);

  auto &getaddrinfo_cb = instance_ptr->request_cb_storage.value();
  if (getaddrinfo_cb)  getaddrinfo_cb(getaddrinfo(_uv_req));
}



/*! \ingroup doxy_group__request
    \brief Getnameinfo request type.
    \sa libuv API documentation: [DNS utility functions](http://docs.libuv.org/en/v1.x/dns.html#dns-utility-functions). */
class getnameinfo : public request
{
  //! \cond
  friend class request::instance< getnameinfo >;
  //! \endcond

public: /*types*/
  using uv_t = ::uv_getnameinfo_t;
  using on_request_t = std::function< void(getnameinfo _request) >;
  /*!< \brief The function type of the callback called with the `getnameinfo` request result once complete.
       \sa libuv API documentation: [`uv_getnameinfo_cb`](http://docs.libuv.org/en/v1.x/dns.html#c.uv_getnameinfo_cb). */

private: /*types*/
  using instance = request::instance< getnameinfo >;

protected: /*constructors*/
  //! \cond
  explicit getnameinfo(uv_t *_uv_req) : request(reinterpret_cast< request::uv_t* >(_uv_req))  {}
  //! \endcond

public: /*constructors*/
  ~getnameinfo() = default;
  getnameinfo()
  {
    uv_req = instance::create();
    static_cast< uv_t* >(uv_req)->type = UV_GETNAMEINFO;
  }

  getnameinfo(const getnameinfo&) = default;
  getnameinfo& operator =(const getnameinfo&) = default;

  getnameinfo(getnameinfo&&) noexcept = default;
  getnameinfo& operator =(getnameinfo&&) noexcept = default;

private: /*functions*/
  template< typename = void > static void getnameinfo_cb(::uv_getnameinfo_t*, int, const char*, const char*);

public: /*interface*/
  on_request_t& on_request() const noexcept  { return instance::from(uv_req)->request_cb_storage.value(); }

  /*! \brief The libuv loop that started this `getnameinfo` request and where completion will be reported.
      \details It is guaranteed that it will be a valid instance at least within the request callback. */
  uv::loop loop() const noexcept  { return uv::loop(static_cast< uv_t* >(uv_req)->loop); }

  /*! \brief The char array containing the resulting host. It’s null terminated. */
  const char (& host() const noexcept)[NI_MAXHOST]  { return static_cast< uv_t* >(uv_req)->host; }
  /*! \brief The char array containing the resulting service. It’s null terminated. */
  const char (& service() const noexcept)[NI_MAXSERV]  { return static_cast< uv_t* >(uv_req)->service; }

  /*! \brief Run the request.
      \sa libuv API documentation: [`uv_getnameinfo()`](http://docs.libuv.org/en/v1.x/dns.html#c.uv_getnameinfo).
      \note If the request callback is empty (has not been set), the request runs _synchronously_.

      Available `_NI_FLAGS` are:
      - NI_DGRAM
      - NI_NAMEREQD
      - NI_NOFQDN
      - NI_NUMERICHOST
      - NI_NUMERICSERV
      .
      \sa Linux: [`getnameinfo()`](http://man7.org/linux/man-pages/man3/getnameinfo.3.html).\n
          Windows: [`getnameinfo()`](https://msdn.microsoft.com/en-us/library/ms738532(v=vs.85).aspx). */
  template< typename _T_, typename = std::enable_if_t< is_one_of< _T_, ::sockaddr, ::sockaddr_in, ::sockaddr_in6, ::sockaddr_storage >::value > >
  int run(uv::loop &_loop, const _T_ &_sa, int _NI_FLAGS = 0)
  {
    auto instance_ptr = instance::from(uv_req);

    if (!instance_ptr->request_cb_storage.value())
    {
      return uv_status(::uv_getnameinfo(
          static_cast< uv::loop::uv_t* >(_loop), static_cast< uv_t* >(uv_req),
          nullptr,
          reinterpret_cast< const ::sockaddr* >(&_sa), _NI_FLAGS
      ));
    }
    else
    {
      instance_ptr->ref();

      uv_status(0);
      auto uv_ret = ::uv_getnameinfo(
          static_cast< uv::loop::uv_t* >(_loop), static_cast< uv_t* >(uv_req),
          getnameinfo_cb,
          reinterpret_cast< const ::sockaddr* >(&_sa), _NI_FLAGS
      );
      if (uv_ret < 0)
      {
        uv_status(uv_ret);
        instance_ptr->unref();
      }

      return uv_ret;
    }
  }

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_req); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_req); }
};

template< typename >
void getnameinfo::getnameinfo_cb(::uv_getnameinfo_t *_uv_req, int _status, const char* _hostname, const char* _service)
{
  auto instance_ptr = instance::from(_uv_req);
  instance_ptr->uv_error = _status;

  ref_guard< instance > unref_req(*instance_ptr, adopt_ref);

  auto &getnameinfo_cb = instance_ptr->request_cb_storage.value();
  if (getnameinfo_cb)  getnameinfo_cb(getnameinfo(_uv_req));
}



}


#endif
