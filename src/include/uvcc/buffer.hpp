
#ifndef UVCC_BUFFER__HPP
#define UVCC_BUFFER__HPP

#include "uvcc/utility.hpp"

#include <uv.h>
#include <cstddef>           // size_t offsetof max_align_t
#include <type_traits>       // is_standard_layout
#include <utility>           // swap()
#include <memory>            // addressof()
#include <initializer_list>  // initializer_list


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
    ref_count rc_;
    std::size_t buf_count_;
    uv_t uv_buf_;

  private: /*new/delete*/
    static void* operator new(std::size_t _size, const std::initializer_list< std::size_t > &_len_values)
    {
      auto extra_buf_count = _len_values.size();
      if (extra_buf_count > 0)  --extra_buf_count;
      std::size_t total_buf_len = 0;
      for (auto len : _len_values)  total_buf_len += len;
      return ::operator new(_size + extra_buf_count*sizeof(uv_t) + alignment_padding(extra_buf_count) + total_buf_len);
    }
    static void operator delete(void *_ptr, const std::initializer_list< std::size_t >&)  { ::operator delete(_ptr); }
    static void operator delete(void *_ptr)  { ::operator delete(_ptr); }

  private: /*constructors*/
    instance(const std::initializer_list< std::size_t > &_len_values) : buf_count_(_len_values.size())
    {
      if (buf_count_ == 0)
      {
        buf_count_ = 1;
        uv_buf_.base = nullptr;
        uv_buf_.len = 0;
      }
      else
      {
        uv_t *buf = &uv_buf_;
        std::size_t total_buf_len = 0;
        for (auto len : _len_values)  total_buf_len += ((buf++)->len = len);
        if (total_buf_len == 0)
        {
          buf = &uv_buf_;
          for (decltype(buf_count_) i = 0; i < buf_count_; ++i)  { buf[i].base = nullptr; buf[i].len = 0; }
        }
        else
        {
          uv_buf_.base = reinterpret_cast< char* >(buf) + alignment_padding(buf_count_-1);
          buf = &uv_buf_;
          for (decltype(buf_count_) i = 1; i < buf_count_; ++i)  buf[i].base = &buf[i-1].base[buf[i-1].len];
        }
      }
    }

  public: /*constructors*/
    ~instance() = default;

    instance(const instance&) = delete;
    instance& operator =(const instance&) = delete;

    instance(instance&&) = delete;
    instance& operator =(instance&&) = delete;

  private: /*functions*/
    static std::size_t alignment_padding(const std::size_t _extra_buf_count) noexcept
    {
      std::size_t base_size = sizeof(instance) + _extra_buf_count*sizeof(uv_t);
      std::size_t proper_size = (base_size + alignof(std::max_align_t) - 1) & ~(alignof(std::max_align_t) - 1);
      return proper_size - base_size;
    }

    void destroy()  { delete this; }

  public: /*interface*/
    static uv_t* create(const std::initializer_list< std::size_t > &_len_values)
    { return std::addressof((new(_len_values) instance(_len_values))->uv_buf_); }
    static uv_t* create()  { return create({}); }

    constexpr static instance* from(uv_t *_uv_buf) noexcept
    {
      static_assert(std::is_standard_layout< instance >::value, "not a standard layout type");
      return reinterpret_cast< instance* >(reinterpret_cast< char* >(_uv_buf) - offsetof(instance, uv_buf_));
    }

    std::size_t buf_count()  { return buf_count_; }

    void ref()  { rc_.inc(); }
    void unref() noexcept  { if (rc_.dec() == 0)  destroy(); }
  };

private: /*data*/
  uv_t *uv_buf;

public: /*constructors*/
  ~buffer()  { if (uv_buf)  instance::from(uv_buf)->unref(); }

  /*! \brief Create a single `uv_buf_t` _null-initialized_ buffer structure.
      \details That is:
      ```
      char*  uv_buf_t.base = nullptr;
      size_t uv_buf_t.len  = 0;
      ``` */
  buffer() : uv_buf(instance::create())  {}

  /*! \brief Create an array of `uv_buf_t` effectively initialized buffer structures.
      \details Each structure in the array is effectively initialized with an allocated memory chunk
      of the specified length. The number of structures in array is equal to the number of elements
      in the initializer list. The value of the `.len` field and the length of the each allocated
      chunk pointed by the `.base` field is equal to the corresponding value from the initializer list.

      All chunks are located seamlessly one after the next within a single continuous memory block.
      Therefore the `.base` field of the next buffer just points to the byte following the end
      of the previous buffer and the `.base` field of the first buffer in the array points to the
      whole memory area of the total length of all buffers.

      If some of the initializing values are zeros, the `.base` field of the such a buffer is not a `nullptr`.
      Instead it keeps pointing inside the continuous memory block and is considered as a zero-length chunk.

      All of the initializing values being zeros results in creating an array of _null-initialized_
      `uv_buf_t` buffer structures. */
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

  std::size_t count() const noexcept  { return instance::from(uv_buf)->buf_count(); }  /*!< \brief The number of the `uv_buf_t` structures in the array. */

  uv_t& operator [](const std::size_t _i) const noexcept  { return uv_buf[_i]; }  /*!< \brief Access to the `_i`-th `uv_buf_t` buffer structure in the array. */
  decltype(uv_t::base)& base(const std::size_t _i = 0) const noexcept  { return uv_buf[_i].base; }  /*!< \brief The `.base` field of the `_i`-th buffer structure. */
  decltype(uv_t::len)& len(const std::size_t _i = 0) const noexcept  { return uv_buf[_i].len; }  /*!< \brief The `.len` field of the `_i`-th buffer structure. */

public: /*conversion operators*/
  operator const uv_t*() const noexcept  { return uv_buf; }
  operator       uv_t*()       noexcept  { return uv_buf; }

  explicit operator bool() const noexcept  { return base(); }  /*!< \brief Equivalent to `( base() != nullptr )`. */
};


//! \}
}


namespace std
{

//! \ingroup g__buffer
template<> void swap(uv::buffer &_this, uv::buffer &_that) noexcept  { _this.swap(_that); }

}



#endif
