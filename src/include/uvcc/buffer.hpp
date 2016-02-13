
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
    ref_count count;
    uv_t uv_buf;

  private: /*constructors*/
    instance()
    {
      uv_buf.base = nullptr;
      uv_buf.len = 0;
    }
    instance(std::size_t _len)
    {
      uv_buf.base = new char[_len];
      uv_buf.len = _len;
    }

  public: /*constructors*/
    ~instance()  { delete[] uv_buf.base; }

    instance(const instance&) = delete;
    instance& operator =(const instance&) = delete;

    instance(instance&&) = delete;
    instance& operator =(instance&&) = delete;

  private: /*functions*/

    void destroy()  { delete this; }

  public: /*interface*/
    static uv_t* create()  { return std::addressof((new instance)->uv_buf); }
    static uv_t* create(std::size_t _len)  { return std::addressof((new instance(_len))->uv_buf); }

    constexpr static instance* from(uv_t *_uv_buf) noexcept
    {
      static_assert(std::is_standard_layout< instance >::value, "not a standard layout type");
      return reinterpret_cast< instance* >(reinterpret_cast< char* >(_uv_buf) - offsetof(instance, uv_buf));
    }

    void ref()  { count.inc(); }
    void unref() noexcept  { if (count.dec() == 0)  destroy(); }
  };

private: /*data*/
  uv_t *uv_buf;

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
      uv_t *uv_b = uv_buf;
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
      uv_t *uv_b = uv_buf;
      uv_buf = _b.uv_buf;
      _b.uv_buf = nullptr;
      instance::from(uv_b)->unref();
    };
    return *this;
  }

public: /*interface*/
  void swap(buffer &_b) noexcept  { std::swap(uv_buf, _b.uv_buf); }

  char* base() const noexcept  { return uv_buf->base; }
  std::size_t len() const noexcept  { return uv_buf->len; }

public: /*conversion operators*/
  operator const uv_t*() const noexcept  { return uv_buf; }
  operator       uv_t*()       noexcept  { return uv_buf; }

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
