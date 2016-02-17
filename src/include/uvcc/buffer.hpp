
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
    constexpr static instance* from(uv_t *_uv_buf) noexcept
    {
      static_assert(std::is_standard_layout< instance >::value, "not a standard layout type");
      return reinterpret_cast< instance* >(reinterpret_cast< char* >(_uv_buf) - offsetof(instance, uv_buf));
    }

    void destroy()  { delete this; }

  public: /*interface*/
    static uv_t* create(std::size_t _len)  { return std::addressof((new instance(_len))->uv_buf); }

    static void ref(uv_t *_uv_buf)  { from(_uv_buf)->count.inc(); }
    static void unref(uv_t *_uv_buf) noexcept  { instance *b = from(_uv_buf); if (b->count.dec() == 0)  b->destroy(); }
  };

private: /*data*/
  uv_t *uv_buf;

public: /*constructors*/
  ~buffer()  { if (uv_buf)  instance::unref(uv_buf); }
  buffer(std::size_t _len) : uv_buf(instance::create(_len))  {}

  buffer(const buffer &_b)
  {
    instance::ref(_b.uv_buf);
    uv_buf = _b.uv_buf; 
  }
  buffer& operator =(const buffer &_b)
  {
    if (this != &_b)
    {
      instance::ref(_b.uv_buf);
      uv_t *uv_b = uv_buf;
      uv_buf = _b.uv_buf; 
      instance::unref(uv_b);
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
      instance::unref(uv_b);
    };
    return *this;
  }

public: /*interface*/
  void swap(buffer &_b) noexcept  { std::swap(uv_buf, _b.uv_buf); }

  char* base() const noexcept  { return uv_buf->base; }
  std::size_t len() const noexcept  { return uv_buf->len; }

public: /*conversion operators*/
  operator       uv_t*()       noexcept  { return uv_buf; }
  operator const uv_t*() const noexcept  { return uv_buf; }

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
