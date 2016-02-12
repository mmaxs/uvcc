
#ifndef UVCC_THREAD__HPP
#define UVCC_THREAD__HPP

#include <uv.h>
#include <stdexcept>    // runtime_error


namespace uv
{


class mutex
{
public: /*types*/
  using native_handle_type = ::uv_mutex_t;

private: /*data*/
  native_handle_type uv_mutex;

public: /*constructors*/
  ~mutex() noexcept  { ::uv_mutex_destroy(&uv_mutex); }
  mutex()  { if (::uv_mutex_init(&uv_mutex) < 0) throw std::runtime_error(__PRETTY_FUNCTION__); }

  mutex(const mutex&) = delete;
  mutex& operator =(const mutex&) = delete;

  mutex(mutex&&) = delete;
  mutex& operator =(mutex&&) = delete;

public: /*interface*/
  void lock() noexcept     { ::uv_mutex_lock(&uv_mutex); }
  bool try_lock() noexcept { return ::uv_mutex_trylock(&uv_mutex) == 0; }
  void unlock() noexcept   { ::uv_mutex_unlock(&uv_mutex); }

  native_handle_type native_handle() const noexcept  { return uv_mutex; }
};


}


#endif
