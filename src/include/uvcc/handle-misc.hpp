
#ifndef UVCC_HANDLE_MISC__HPP
#define UVCC_HANDLE_MISC__HPP

#include "uvcc/utility.hpp"
#include "uvcc/handle-base.hpp"

#include <uv.h>

#include <functional>   // function bind placeholders::


namespace uv
{


/*! \ingroup doxy_group__handle
    \brief Async handle.
    \sa libuv API documentation: [`uv_async_t — Async handle`](http://docs.libuv.org/en/v1.x/async.html#uv-async-t-async-handle). */
class async : public handle
{
  friend class handle::instance< async >;

public: /*types*/
  using uv_t = ::uv_async_t;
  template< typename... _Args_ >
  using on_send_t = std::function< void(async _handle, _Args_&&... _args) >;
  /*!< */

protected: /*types*/
  //! \cond internals
  //! \addtogroup doxy_group__internals
  //! \{

  struct properties : handle::properties
  {
    std::function< void(async) > cb;
  };

  struct uv_interface : handle::uv_handle_interface
  {
    static uv_interface& instance()  { static uv_interface instance;  return instance; }
  };

  //! \}
  //! \endcond

private: /*types*/
  using instance = handle::instance< async >;

private: /*functions*/
  template < typename = void > static void async_cb(::uv_async_t*);

protected: /*constructors*/
  explicit async(uv_t *_uv_handle) : handle(reinterpret_cast< handle::uv_t* >(_uv_handle))  {}

public: /*constructors*/
  ~async() = default;

  async(const async&) = default;
  async& operator =(const async&) = default;

  async(async&&) noexcept = default;
  async& operator =(async&&) noexcept = default;

  explicit async(uv::loop &_loop)
  {
    uv_handle = instance::create();
    auto uv_ret = uv_status(
        ::uv_async_init(static_cast< uv::loop::uv_t* >(_loop), static_cast< uv_t* >(uv_handle), nullptr)
    );
    if (uv_ret >= 0)  instance::from(uv_handle)->book_loop();
  }

  template< class _Cb_, typename... _Args_,
      typename = std::enable_if_t< std::is_convertible< _Cb_, on_send_t< _Args_&&... > >::value >
  >
  async(uv::loop &_loop, _Cb_ &&_cb, _Args_&&... _args)
  {
    uv_handle = instance::create();
    instance::from(uv_handle)->properties().cb = std::bind(
        std::forward< _Cb_ >(_cb), std::placeholders::_1, std::forward< _Args_ >(_args)...
    );
    auto uv_ret = uv_status(
        ::uv_async_init(static_cast< uv::loop::uv_t* >(_loop), static_cast< uv_t* >(uv_handle), async_cb)
    );
    if (uv_ret >= 0)  instance::from(uv_handle)->book_loop();
  }

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_handle); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_handle); }
};

template< typename >
void async::async_cb(::uv_async_t *_uv_handle)
{
  auto &cb = instance::from(_uv_handle)->properties().cb;
  if (cb)  cb(async(_uv_handle));
}


}


#endif
