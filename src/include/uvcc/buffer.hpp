
#ifndef UVCC_BUFFER__HPP
#define UVCC_BUFFER__HPP

#include "uvcc/utility.hpp"

#include <uv.h>
#include <cstddef>           // size_t offsetof
#include <type_traits>       // is_standard_layout
#include <utility>           // swap()
#include <memory>            // addressof()
#include <initializer_list>  //


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
    uv_t uv_buf;

  private: /*new/delete*/
    static void* operator new(std::size_t _size, const std::initializer_list< std::size_t > &_len_values)
    {
      auto buf_ecount = _len_values.size();
      if (buf_ecount > 0)  --buf_ecount;  // extra count
      std::size_t buf_tlen = 0;
      for (auto len : _len_values)  buf_tlen += len;  // total length
      return ::operator new(_size + buf_ecount*sizeof(uv_t) + buf_tlen);
    }
    static void operator delete(void *_ptr, const std::initializer_list< std::size_t >&)  { ::operator delete(_ptr); }
    static void operator delete(void *_ptr)  { ::operator delete(_ptr); }

  private: /*constructors*/
    instance(const std::initializer_list< std::size_t > &_len_values) : buf_count(_len_values.size())
    {
      if (buf_count == 0)
      {
        buf_count = 1;
        uv_buf.base = nullptr;
        uv_buf.len = 0;
      }
      else
      {
        uv_t *buf = &uv_buf;
        for (auto len : _len_values)  (buf++)->len = len;
        uv_buf.base = reinterpret_cast< char* >(buf);
        buf = &uv_buf;
        for (decltype(buf_count) i = 1; i < buf_count; ++i)  buf[i].base = &buf[i-1].base[buf[i-1].len];
      }
    }

  public: /*constructors*/
    ~instance() = default;

    instance(const instance&) = delete;
    instance& operator =(const instance&) = delete;

    instance(instance&&) = delete;
    instance& operator =(instance&&) = delete;

  private: /*functions*/
    void destroy()  { delete this; }

  public: /*interface*/
    static uv_t* create(const std::initializer_list< std::size_t > &_len_values)  { return std::addressof((new(_len_values) instance(_len_values))->uv_buf); }
    static uv_t* create()  { return create({}); }
    static uv_t* create(std::size_t _len)  { return create({_len}); }

    constexpr static instance* from(uv_t *_uv_buf) noexcept
    {
      static_assert(std::is_standard_layout< instance >::value, "not a standard layout type");
      return reinterpret_cast< instance* >(reinterpret_cast< char* >(_uv_buf) - offsetof(instance, uv_buf));
    }

    auto bcount()  { return buf_count; }

    void ref()  { rc.inc(); }
    void unref() noexcept  { if (rc.dec() == 0)  destroy(); }
  };

private: /*data*/
  uv_t *uv_buf;

public: /*constructors*/
  ~buffer()  { if (uv_buf)  instance::from(uv_buf)->unref(); }

  buffer() : uv_buf(instance::create())  {}
  explicit buffer(std::size_t _len) : uv_buf(instance::create(_len))  {}
  explicit buffer(const std::initializer_list< std::size_t > &_len_values) : uv_buf(instance::create(_len_values))  {}

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

  auto count()  { return instance::from(uv_buf)->bcount(); }

  char* base(const long _i = 0) const noexcept  { return uv_buf[_i].base; }
  std::size_t len(const long _i = 0) const noexcept  { return uv_buf[_i].len; }

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
