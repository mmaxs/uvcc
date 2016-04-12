
#ifndef UVCC_REQUEST_FS__HPP
#define UVCC_REQUEST_FS__HPP

#include "uvcc/utility.hpp"
#include "uvcc/request-base.hpp"
#include "uvcc/handle-fs.hpp"
#include "uvcc/buffer.hpp"
#include "uvcc/loop.hpp"
#include "uvcc/thread.hpp"

#include <uv.h>
#include <cstring>      // memset()
#include <functional>   // function
#ifdef _WIN32
#include <io.h>         // _dup()
#else
#include <unistd.h>     // dup()
#endif


namespace uv
{


/*! \ingroup doxy_request
    \brief The base calss for filesystem requests.
    \sa libuv API documentation: [Filesystem operations](http://docs.libuv.org/en/v1.x/fs.html#filesystem-operations). */
class fs : public request
{
  //! \cond
  friend class request::instance< fs >;
  //! \endcond

public: /*types*/
  using uv_t = ::uv_fs_t;

  class file;

private: /*types*/
  using instance = request::instance< fs >;

private: /*constructors*/
  explicit fs(uv_t *_uv_req)
  {
    if (_uv_req)  instance::from(_uv_req)->ref();
    uv_req = _uv_req;
  }

protected: /*constructors*/
  fs() noexcept = default;

public: /*constructors*/
  ~fs() = default;

  fs(const fs&) = default;
  fs& operator =(const fs&) = default;

  fs(fs&&) noexcept = default;
  fs& operator =(fs&&) noexcept = default;

public: /*interface*/
  /*! \brief The tag indicating a libuv subtype of the filesystem request.
      \sa libuv API documentation: [`uv_fs_type`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_type). */
  ::uv_fs_type fs_type() const noexcept  { return static_cast< uv_t* >(uv_req)->fs_type; }
  /*! \brief The libuv loop that started this filesystem request and where completion will be reported. */
  uv::loop loop() const noexcept  { return uv::loop(static_cast< uv_t* >(uv_req)->loop); }

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_req); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_req); }
};


/*! \brief The open file handle. */
class fs::file
{
public: /*types*/
  using uv_t = ::uv_fs_t;
  using on_destroy_t = std::function< void(void *_data) >;
  /*!< \brief The function type of the callback called when the file handle has been closed and is about to be destroyed. */
  using on_open_t = std::function< void(file) >;
  /*!< \brief The function type of the callback called after the asynchronous open operation has been completed. */
  using on_read_t = std::function< void(file _file, ssize_t _nread, buffer _buffer) >;
  /*!< \brief The function type of the callback called by `read_start()` when data was read from the file. */

private: /*types*/
  class instance
  {
  private: /*data*/
    mutable tls_int uv_error;
    ref_count rc;
    type_storage< on_destroy_t > on_destroy_storage;
    union_storage< on_open_t, on_read_t > cb_storage;
    uv_t uv_fs = { 0,};

  private: /*constructors*/
    instance()  { uv_fs.result = -1; }

  public: /*constructors*/
    ~instance() = default;

    instance(const instance&) = delete;
    instance& operator =(const instance&) = delete;

    instance(instance&&) = delete;
    instance& operator =(instance&&) = delete;

  private: /*functions*/
    void destroy()
    {
      if (uv_fs.result >= 0)  // assuming that uv_fs.result has been initialized with value < 0
      {
        uv_t req_close;
        ::uv_fs_close(nullptr, &req_close, uv_fs.result, nullptr);
        ::uv_fs_req_cleanup(&req_close);
      };

      auto &destroy_cb = on_destroy_storage.value();
      if (destroy_cb)  destroy_cb(uv_fs.data);

      ::uv_fs_req_cleanup(&uv_fs);
      delete this;
    }

  public: /*interface*/
    static uv_t* create()  { return std::addressof((new instance())->uv_fs); }

    constexpr static instance* from(uv_t *_uv_fs) noexcept
    {
      static_assert(std::is_standard_layout< instance >::value, "not a standard layout type");
      return reinterpret_cast< instance* >(reinterpret_cast< char* >(_uv_fs) - offsetof(instance, uv_fs));
    }

    on_destroy_t& on_destroy() noexcept  { return on_destroy_storage.value(); }
    decltype(cb_storage)& cb() noexcept  { return cb_storage; }

    void ref()  { rc.inc(); }
    void unref() noexcept  { if (rc.dec() == 0)  destroy(); }
    ref_count::type nrefs() const noexcept  { return rc.value(); }

    tls_int& uv_status() const noexcept  { return uv_error; }
  };

private: /*data*/
  uv_t *uv_fs;

private: /*constructors*/
  explicit file(uv_t *_uv_fs)
  {
    if (_uv_fs)  instance::from(_uv_fs)->ref();
    uv_fs = _uv_fs;
  }

public: /*constructors*/
  ~file() = default;

  /*! \name File handle constructors - open and possibly create a file:
      \sa libuv API documentation: [`uv_fs_open()`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_open).
      \sa Linux: [`open()`](http://man7.org/linux/man-pages/man2/open.2.html).
          Windows: [`_open()`](https://msdn.microsoft.com/en-us/library/z0kc8e3z.aspx).

      The file descriptor will be closed automatically when the file handle reference count has became zero. */
  //! \{
  /*! \brief Open and possibly create a file _synchronously_. */
  file(const char *_path, int _flags, int _mode)
  {
    uv_fs = instance::create();
    uv_status(::uv_fs_open(
        nullptr, uv_fs,
        _path, _flags, _mode,
        nullptr
    ));
  }
  /*! \brief Open and possibly create a file.
      \note If the `_open_cb` callback is empty the operation is completed _synchronously_
       (and `_loop` parameter is ignored), otherwise it will be performed _asynchronously_. */
  file(uv::loop _loop, const char *_path, int _flags, int _mode, const on_open_t &_open_cb)
  {
    uv_fs = instance::create();

    if (_open_cb)
    {
      instance::from(uv_fs)->cb().reset(_open_cb);

      uv::loop::instance::from(_loop.uv_loop)->ref();
      instance::from(uv_fs)->ref();
    };

    uv_status(::uv_fs_open(
        static_cast< loop::uv_t* >(_loop), uv_fs,
        _path, _flags, _mode,
        _open_cb ? static_cast< ::uv_fs_cb >(open_cb) : nullptr
    ));
  }
  //! \}

  file(const file&) = default;
  file& operator =(const file&) = default;

  file(file&&) noexcept = default;
  file& operator =(file&&) noexcept = default;

private: /*functions*/
  template< typename = void > static void open_cb(::uv_fs_t*);

  int uv_status(int _value) const noexcept
  {
    instance::from(uv_fs)->uv_status() = _value;
    return _value;
  }

public: /*interface*/
  void swap(file &_that) noexcept  { std::swap(uv_fs, _that.uv_fs); }
  /*! \brief The current number of existing references to the same object as this request variable refers to. */
  long nrefs() const noexcept  { return instance::from(uv_fs)->nrefs(); }
  /*! \brief The status value returned by the last executed libuv API function on this handle. */
  int uv_status() const noexcept  { return instance::from(uv_fs)->uv_status(); }

  const on_destroy_t& on_destroy() const noexcept  { return instance::from(uv_fs)->on_destroy(); }
        on_destroy_t& on_destroy()       noexcept  { return instance::from(uv_fs)->on_destroy(); }

  /*! \brief The pointer to the user-defined arbitrary data. libuv and uvcc does not use this field. */
  void* const& data() const noexcept  { return static_cast< uv_t* >(uv_fs)->data; }
  void*      & data()       noexcept  { return static_cast< uv_t* >(uv_fs)->data; }

  /*! \brief Get the cross platform representation of a file handle (mostly being a POSIX-like file descriptor). */
  ::uv_file fd() const noexcept  { return static_cast< uv_t* >(uv_fs)->result; }
  /*! \brief The file path. */
  const char* path() const noexcept  { return static_cast< uv_t* >(uv_fs)->path; }

  /*! \brief Duplicate this file descriptor. */
  ::uv_file dup() const noexcept
  {
    if (fd() < 0)  return -1;
#ifdef _WIN32
    return ::_dup(fd());
#else
    return ::dup(fd());
#endif
  }
#if 0
  /*! \brief Read data from the file into the buffers described by `_buf` object.
      \returns The number of bytes read or relevant [libuv error constant](http://docs.libuv.org/en/v1.x/errors.html#error-constants).
      \sa libuv API documentation: [`uv_fs_read()`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_read).
      \sa Linux: [`preadv()`](http://man7.org/linux/man-pages/man2/preadv.2.html). */
  int read_start(buffer &_buf, int64_t _offset = -1) const noexcept
  {
    uv_t req_read;
    uv_status(::uv_fs_read(
        nullptr, &req_read,
        fd(),
        static_cast< const buffer::uv_t* >(_buf), _buf.count(),
        _offset,
        nullptr
    ));
    ::uv_fs_req_cleanup(&req_read);
    return uv_status();
  }
  int read_stop() const  { return 0; }
#endif
public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_fs); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_fs); }

  explicit operator bool() const noexcept  { return (uv_status() >= 0); }  /*!< \brief Equivalent to `(uv_status() >= 0)`. */
};

template< typename >
void fs::file::open_cb(::uv_fs_t *_uv_fs)
{
  auto self = instance::from(_uv_fs);
  self->uv_status() = _uv_fs->result;

  ref_guard< uv::loop::instance > unref_loop(*uv::loop::instance::from(_uv_fs->loop), adopt_ref);
  ref_guard< instance > unref_req(*self, adopt_ref);

  auto &open_cb = self->cb().get< on_open_t >();
  if (open_cb)  open_cb(file(_uv_fs));
}


}


namespace std
{

//! \ingroup doxy_request
template<> inline void swap(uv::fs::file &_this, uv::fs::file &_that) noexcept  { _this.swap(_that); }

}


#endif
