
#ifndef UVCC_BUFFER__HPP
#define UVCC_BUFFER__HPP

#include "uvcc/utility.hpp"

#include <uv.h>
#include <cstddef>      // size_t offsetof
#include <type_traits>  // is_standard_layout
#include <utility>      // swap()
#include <memory>       // addressof()


namespace uv
{
/*! \defgroup g__buffer Buffer for I/O operations */
//! \{


/*! \brief Encapsulates `uv_buf_t` data type and provides `uv_buf_t[]` functionality. */
class buffer
{
public: /*types*/
  using uv_t = ::uv_buf_t;

private: /*types*/
  class instance
  {
  private: /*data*/
    ref_count rc;
    long buf_count;
    uv_t *uv_bufs;

  private: /*constructors*/
    instance()
    {
      buf_count = 1;
      uv_bufs = new uv_t[buf_count];
      uv_bufs[0].base = nullptr;
      uv_bufs[0].len = 0;
    }
    /* for gcc 6
    template< typename... _Args_ > instance(const _Args_ _args)
    {
      buf_count = sizeof...(_Args_);
      uv_bufs = new uv_t[buf_count];
      uv_bufs[0].base = new char[(... + _args)];

      auto buf = bufs;
      (..., ((buf++)->len = _args));
      buf = bufs;
      (..., ((++buf)->base = &buf[-1].base[buf[-1].len], (void)_args));
    }
    */
    template< typename... _Args_ > instance(const _Args_... _args)
    {
      buf_count = sizeof...(_Args_);
      uv_bufs = new uv_t[buf_count];
      init(uv_bufs, _args...);
    }

  public: /*constructors*/
    ~instance()
    {
      delete[] uv_bufs[0].base;
      delete[] uv_bufs;
    }

    instance(const instance&) = delete;
    instance& operator =(const instance&) = delete;

    instance(instance&&) = delete;
    instance& operator =(instance&&) = delete;

  private: /*functions*/
    void init(uv_t*)
    {
      std::size_t len = 0;
      for (auto i = buf_count; i--;)  len += uv_bufs[i].len;
      uv_bufs[0].base = new char[len];

      uv_t *buf = uv_bufs;
      for (decltype(buf_count) i = 1; i < buf_count; ++i)  buf[i].base = &buf[i-1].base[buf[i-1].len];
    }
    template< typename... _Args_ > void init(uv_t *_buf, const std::size_t _len, const _Args_... _args)
    {
      _buf->len = _len;
      init(++_buf, _args...);
    }

    void destroy()  { delete this; }

  public: /*interface*/
    static uv_t** create()  { return std::addressof((new instance)->uv_bufs); }
    static uv_t** create(std::size_t _len)  { return std::addressof((new instance(_len))->uv_bufs); }

    constexpr static instance* from(uv_t **_uv_bufs) noexcept
    {
      static_assert(std::is_standard_layout< instance >::value, "not a standard layout type");
      return reinterpret_cast< instance* >(reinterpret_cast< char* >(_uv_bufs) - offsetof(instance, uv_bufs));
    }

    void ref()  { rc.inc(); }
    void unref() noexcept  { if (rc.dec() == 0)  destroy(); }
  };

private: /*data*/
  uv_t **uv_buf;

public: /*constructors*/
  ~buffer()  { if (uv_buf)  instance::from(uv_buf)->unref(); }

  buffer() : uv_buf(instance::create())  {}
  buffer(std::size_t _len) : uv_buf(instance::create(_len))  {}

  buffer(const buffer &_b)
  {
    instance::from(_b.uv_buf)->ref();
    uv_buf = _b.uv_buf; 
  }
  buffer& operator =(const buffer &_b)
  {
    if (this != &_b)
    {
      instance::from(_b.uv_buf)->ref();
      uv_t **uv_b = uv_buf;
      uv_buf = _b.uv_buf; 
      instance::from(uv_b)->unref();
    };
    return *this;
  }

  buffer(buffer &&_b) noexcept : uv_buf(_b.uv_buf)  { _b.uv_buf = nullptr; }
  buffer& operator =(buffer &&_b) noexcept
  {
    if (this != &_b)
    {
      uv_t **uv_b = uv_buf;
      uv_buf = _b.uv_buf;
      _b.uv_buf = nullptr;
      instance::from(uv_b)->unref();
    };
    return *this;
  }

public: /*interface*/
  void swap(buffer &_b) noexcept  { std::swap(uv_buf, _b.uv_buf); }

  char* base(const long _i = 0) const noexcept  { return (*uv_buf)[_i].base; }
  std::size_t len(const long _i = 0) const noexcept  { return (*uv_buf)[_i].len; }

public: /*conversion operators*/
  operator const uv_t*() const noexcept  { return *uv_buf; }
  operator       uv_t*()       noexcept  { return *uv_buf; }

  explicit operator bool() const noexcept  { return base(); }
};


//! \}
}


namespace std
{

//! \ingroup g__buffer
template<> void swap(uv::buffer &_this, uv::buffer &_that) noexcept  { _this.swap(_that); }

}



#endif
