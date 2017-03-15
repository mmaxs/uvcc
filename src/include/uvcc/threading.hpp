
#ifndef UVCC_THREAD__HPP
#define UVCC_THREAD__HPP

#include <cstdint>    // intptr_t
#include <uv.h>

#include <stdexcept>  // runtime_error


namespace uv
{


class mutex
{
public: /*types*/
  using uv_t = ::uv_mutex_t;

private: /*data*/
  uv_t uv_mutex;

public: /*constructors*/
  ~mutex() noexcept  { ::uv_mutex_destroy(&uv_mutex); }
  mutex()  { if (::uv_mutex_init(&uv_mutex) < 0)  throw std::runtime_error(__PRETTY_FUNCTION__); }

  mutex(const mutex&) = delete;
  mutex& operator =(const mutex&) = delete;

  mutex(mutex&&) = delete;
  mutex& operator =(mutex&&) = delete;

public: /*interface*/
  void lock() noexcept     { ::uv_mutex_lock(&uv_mutex); }
  bool try_lock() noexcept { return ::uv_mutex_trylock(&uv_mutex) == 0; }
  void unlock() noexcept   { ::uv_mutex_unlock(&uv_mutex); }

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return &uv_mutex; }
  explicit operator       uv_t*()       noexcept  { return &uv_mutex; }
};


class tls_int
{
private: /*data*/
  ::uv_key_t tls_key;

public: /*constructors*/
  ~tls_int() noexcept  { ::uv_key_delete(&tls_key); }

  explicit tls_int(const int _value = 0)
  {
    if (::uv_key_create(&tls_key) < 0)  throw std::runtime_error(__PRETTY_FUNCTION__);
    ::uv_key_set(&tls_key, reinterpret_cast< void* >(_value));
  }
  tls_int& operator =(const int _value) noexcept
  {
    ::uv_key_set(&tls_key, reinterpret_cast< void* >(_value));
    return *this;
  }

  tls_int(const tls_int &_that) : tls_int((int)_that)  {}
  tls_int& operator =(const tls_int &_that) noexcept  { return this == &_that ? *this : operator =((int)_that); }

public: /*conversion opertors*/
  operator int() const noexcept
  {
    return reinterpret_cast< std::intptr_t >(::uv_key_get(const_cast< ::uv_key_t* >(&tls_key)));
  }
};


}


#endif
