
#ifndef UVCC_BUFFER__HPP
#define UVCC_BUFFER__HPP

#include "uvcc/utility.hpp"

#include <uv.h>
#include <cstddef>           // size_t offsetof max_align_t
#include <type_traits>       // is_standard_layout
#include <utility>           // swap()
#include <initializer_list>  // initializer_list
#include <functional>        // function


namespace uv
{


class handle;


/*! \defgroup doxy_group_buffer Buffer for I/O operations */

/*! \ingroup doxy_group_buffer
    \brief Encapsulates `uv_buf_t` data type and provides `uv_buf_t[]` functionality. */
class buffer
{
  //! \cond
  friend class io;
  friend class write;
  friend class udp;
  friend class udp_send;
  friend class fs;
  //! \endcond

public: /*types*/
  using uv_t = ::uv_buf_t;

private: /*types*/
  class instance
  {
  public: /*data*/
    ref_count refs;
    std::size_t buf_count;
    uv_t uv_buf_struct;

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
    instance(const std::initializer_list< std::size_t > &_len_values) : buf_count(_len_values.size())
    {
      if (buf_count == 0)
      {
        buf_count = 1;
        uv_buf_struct.base = nullptr;
        uv_buf_struct.len = 0;
      }
      else
      {
        uv_t *buf = &uv_buf_struct;
        std::size_t total_buf_len = 0;
        for (auto len : _len_values)  total_buf_len += ((buf++)->len = len);
        if (total_buf_len == 0)
        {
          buf = &uv_buf_struct;
          for (decltype(buf_count) i = 0; i < buf_count; ++i)  { buf[i].base = nullptr; buf[i].len = 0; }
        }
        else
        {
          uv_buf_struct.base = reinterpret_cast< char* >(buf) + alignment_padding(buf_count - 1);
          buf = &uv_buf_struct;
          for (decltype(buf_count) i = 1; i < buf_count; ++i)  buf[i].base = &buf[i-1].base[buf[i-1].len];
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
      const std::size_t base_size = sizeof(instance) + _extra_buf_count*sizeof(uv_t);
      const std::size_t proper_size = (base_size + alignof(std::max_align_t) - 1) & ~(alignof(std::max_align_t) - 1);
      return proper_size - base_size;
    }

    void destroy()  { delete this; }

  public: /*interface*/
    static uv_t* create(const std::initializer_list< std::size_t > &_len_values)
    { return &(new(_len_values) instance(_len_values))->uv_buf_struct; }
    static uv_t* create()  { return create({}); }

    constexpr static instance* from(uv_t *_uv_buf) noexcept
    {
      static_assert(std::is_standard_layout< instance >::value, "not a standard layout type");
      return reinterpret_cast< instance* >(reinterpret_cast< char* >(_uv_buf) - offsetof(instance, uv_buf_struct));
    }

    struct uv_buf
    {
      static uv_t* from(decltype(uv_t::base) _base) noexcept
      {
        return reinterpret_cast< uv_t* >(reinterpret_cast< char* >(_base) - alignment_padding(0) - sizeof(uv_t));
      }
    };

    void ref()  { refs.inc(); }
    void unref() noexcept  { if (refs.dec() == 0)  destroy(); }
  };

private: /*data*/
  uv_t *uv_buf;

private: /*constructors*/
  explicit buffer(uv_t *_uv_buf)
  {
    if (_uv_buf)  instance::from(_uv_buf)->ref();
    uv_buf = _uv_buf;
  }
  explicit buffer(uv_t *_uv_buf, const adopt_ref_t) : uv_buf(_uv_buf)  {}

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

  buffer(const buffer &_that) : buffer(_that.uv_buf)  {}
  buffer& operator =(const buffer &_that)
  {
    if (this != &_that)
    {
      if (_that.uv_buf)  instance::from(_that.uv_buf)->ref();
      auto t = uv_buf;
      uv_buf = _that.uv_buf;
      if (t)  instance::from(t)->unref();
    };
    return *this;
  }

  buffer(buffer &&_that) noexcept : uv_buf(_that.uv_buf)  { _that.uv_buf = nullptr; }
  buffer& operator =(buffer &&_that) noexcept
  {
    if (this != &_that)
    {
      auto t = uv_buf;
      uv_buf = _that.uv_buf;
      _that.uv_buf = nullptr;
      if (t)  instance::from(t)->unref();
    };
    return *this;
  }

public: /*interface*/
  void swap(buffer &_that) noexcept  { std::swap(uv_buf, _that.uv_buf); }
  /*! \brief The current number of existing references to the same buffer as this variable refers to. */
  long nrefs() const noexcept  { return instance::from(uv_buf)->refs.value(); }

  /*! \brief The number of the `uv_buf_t` structures in the array. */
  std::size_t count() const noexcept  { return instance::from(uv_buf)->buf_count; }

  /*! \brief Access to the `_i`-th `uv_buf_t` buffer structure in the array. */
  uv_t& operator [](const std::size_t _i) const noexcept  { return uv_buf[_i]; }
  /*! \brief The `.base` field of the `_i`-th buffer structure. */
  decltype(uv_t::base)& base(const std::size_t _i = 0) const noexcept  { return uv_buf[_i].base; }
  /*! \brief The `.len` field of the `_i`-th buffer structure. */
  decltype(uv_t::len)& len(const std::size_t _i = 0) const noexcept  { return uv_buf[_i].len; }

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return uv_buf; }
  explicit operator       uv_t*()       noexcept  { return uv_buf; }

  explicit operator bool() const noexcept  { return base(); }  /*!< \brief Equivalent to `(base() != nullptr)`. */
};


/*! \ingroup doxy_group_buffer
    \brief The function type of the callback called by `io::read_start()` and `udp::recv_start()`...
    \details ...to supply the input operation with a preallocated buffer. The callback should return a `uv::buffer`
    instance initialized with a `_suggested_size` (the value provided by libuv API is a constant of _65536_ bytes)
    or with whatever size, as long as it’s > 0.
    \sa libuv API documentation: [`uv_alloc_cb`](http://docs.libuv.org/en/v1.x/handle.html#c.uv_alloc_cb).
    \details The following is an example of the trivial ready for general use callback:
    ```
    buffer alloc_cb(handle, std::size_t _suggested_size)
    {
      return buffer{_suggested_size};
    }
    ```
    */
using on_buffer_alloc_t = std::function< buffer(handle _handle, std::size_t _suggested_size) >;


}


namespace std
{

//! \ingroup doxy_buffer
template<> inline void swap(uv::buffer &_this, uv::buffer &_that) noexcept  { _this.swap(_that); }

}


#endif
