
#ifndef UVCC_REQUEST_FS__HPP
#define UVCC_REQUEST_FS__HPP

#include "uvcc/utility.hpp"
#include "uvcc/request-base.hpp"
#include "uvcc/handle-fs.hpp"
#include "uvcc/buffer.hpp"
#include "uvcc/loop.hpp"

#include <uv.h>
#include <cstddef>      // offsetof
#include <cstring>      // memset()
#include <string>       // string
#include <functional>   // function
#include <memory>       // addressof() unique_ptr
#include <utility>      // forward()


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

  class read;
  class write;
  class sync;
  class datasync;
  class truncate;
  class sendfile;

  class stat;
  class chmod;
  class chown;
  class utime;

  class unlink;
  class mkdir;
  class mkdtemp;
  class rmdir;
  class scandir;
  class rename;
  class access;
  class link;
  class symlink;
  class readlink;
  class realpath;

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
  //! \cond
  friend class fs;
  //! \endcond

public: /*types*/
  using uv_t = ::uv_file;
  using on_destroy_t = std::function< void(void *_data) >;
  /*!< \brief The function type of the callback called when the file handle has been closed and is about to be destroyed. */
  using on_open_t = std::function< void(file) >;
  /*!< \brief The function type of the callback called after the asynchronous open operation has been completed. */

private: /*types*/
  class instance
  {
  private: /*data*/
    mutable int uv_error;
    ref_count rc;
    type_storage< on_destroy_t > on_destroy_storage;
    void *user_data = nullptr;
    std::string file_path;
    uv_t uv_file = -1;

  private: /*constructors*/
    instance() = default;

  public: /*constructors*/
    ~instance() = default;

    instance(const instance&) = delete;
    instance& operator =(const instance&) = delete;

    instance(instance&&) = delete;
    instance& operator =(instance&&) = delete;

  private: /*functions*/
    void destroy()
    {
      if (uv_file >= 0)
      {
        ::uv_fs_t req_close;
        ::uv_fs_close(nullptr, &req_close, uv_file, nullptr);
        ::uv_fs_req_cleanup(&req_close);
      };

      auto &destroy_cb = on_destroy_storage.value();
      if (destroy_cb)  destroy_cb(user_data);

      delete this;
    }

  public: /*interface*/
    static uv_t* create()  { return std::addressof((new instance())->uv_file); }

    constexpr static instance* from(uv_t *_uv_file) noexcept
    {
      static_assert(std::is_standard_layout< instance >::value, "not a standard layout type");
      return reinterpret_cast< instance* >(reinterpret_cast< char* >(_uv_file) - offsetof(instance, uv_file));
    }

    on_destroy_t& on_destroy() noexcept  { return on_destroy_storage.value(); }
    void*& data() noexcept  { return user_data; }
    std::string& path() noexcept  { return file_path; }

    void ref()  { rc.inc(); }
    void unref() noexcept  { if (rc.dec() == 0)  destroy(); }
    ref_count::type nrefs() const noexcept  { return rc.value(); }

    decltype(uv_error)& uv_status() const noexcept  { return uv_error; }
  };

  struct open_cb_pack
  {
    uv_t* const uv_file;
    const type_storage< on_open_t > on_open_storage;
    ::uv_fs_t req_open;

    constexpr static open_cb_pack* from(::uv_fs_t *_req_open) noexcept
    {
      static_assert(std::is_standard_layout< open_cb_pack >::value, "not a standard layout type");
      return reinterpret_cast< open_cb_pack* >(reinterpret_cast< char* >(_req_open) - offsetof(open_cb_pack, req_open));
    }
  };

private: /*data*/
  uv_t *uv_file;

private: /*constructors*/
  explicit file(uv_t *_uv_file)
  {
    if (_uv_file)  instance::from(_uv_file)->ref();
    uv_file = _uv_file;
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
    uv_file = instance::create();
    ::uv_fs_t req_open;
    uv_status(::uv_fs_open(
        nullptr, &req_open,
        _path, _flags, _mode,
        nullptr
    ));
    if (req_open.result >= 0)  *uv_file = req_open.result;
    instance::from(uv_file)->path().assign(req_open.path);

    ::uv_fs_req_cleanup(&req_open);
  }
  /*! \brief Open and possibly create a file.
      \note If the `_open_cb` callback is empty the operation is completed _synchronously_
       (and `_loop` parameter is ignored), otherwise it will be performed _asynchronously_. */
  file(uv::loop _loop, const char *_path, int _flags, int _mode, const on_open_t &_open_cb)
  {
    if (!_open_cb)
    {
      new (this) file(_path, _flags, _mode);
      return;
    };

    uv_file = instance::create();
    auto t = new open_cb_pack{uv_file, {_open_cb}, { 0,}};

    instance::from(uv_file)->ref();

    uv_status(0);
    int o = ::uv_fs_open(
        static_cast< uv::loop::uv_t* >(_loop), std::addressof(t->req_open),
        _path, _flags, _mode,
        open_cb
    );
    if (!o)  uv_status(o);
  }
  /*! \brief Create a file object from an existing file descriptor.
      \details Optionally the name of the file the descriptor is associated with can be specified. */
  file(::uv_file _fd, const char *_name = nullptr)
  {
    uv_file = instance::create();
    *uv_file = _fd;
    if (_name)  instance::from(uv_file)->path().assign(_name);
  }
  //! \}

  file(const file &_that) : file(_that.uv_file)  {}
  file& operator =(const file &_that)
  {
    if (this != &_that)
    {
      if (_that.uv_file)  instance::from(_that.uv_file)->ref();
      auto t = uv_file;
      uv_file = _that.uv_file;
      if (t)  instance::from(t)->unref();
    };
    return *this;
  }

  file(file &&_that) noexcept : uv_file(_that.uv_file)  { _that.uv_file = nullptr; }
  file& operator =(file &&_that) noexcept
  {
    if (this != &_that)
    {
      auto t = uv_file;
      uv_file = _that.uv_file;
      _that.uv_file = nullptr;
      if (t)  instance::from(t)->unref();
    };
    return *this;
  }

private: /*functions*/
  template< typename = void > static void open_cb(::uv_fs_t*);

  int uv_status(int _value) const noexcept
  {
    instance::from(uv_file)->uv_status() = _value;
    return _value;
  }

public: /*interface*/
  void swap(file &_that) noexcept  { std::swap(uv_file, _that.uv_file); }
  /*! \brief The current number of existing references to the same object as this request variable refers to. */
  long nrefs() const noexcept  { return instance::from(uv_file)->nrefs(); }
  /*! \brief The status value returned by the last executed libuv API function on this handle. */
  int uv_status() const noexcept  { return instance::from(uv_file)->uv_status(); }

  const on_destroy_t& on_destroy() const noexcept  { return instance::from(uv_file)->on_destroy(); }
        on_destroy_t& on_destroy()       noexcept  { return instance::from(uv_file)->on_destroy(); }

  /*! \brief The pointer to the user-defined arbitrary data. libuv and uvcc does not use this field. */
  void* const& data() const noexcept  { return instance::from(uv_file)->data(); }
  void*      & data()       noexcept  { return instance::from(uv_file)->data(); }

  /*! \brief Get the cross platform representation of a file handle (mostly being a POSIX-like file descriptor). */
  uv_t fd() const noexcept  { return *uv_file; }
  /*! \brief The file path. */
  const std::string& path() const noexcept  { return instance::from(uv_file)->path(); }

public: /*conversion operators*/
  explicit operator const uv_t() const noexcept  { return *uv_file; }

  explicit operator bool() const noexcept  { return (uv_status() >= 0); }  /*!< \brief Equivalent to `(uv_status() >= 0)`. */
};

template< typename >
void fs::file::open_cb(::uv_fs_t *_req_open)
{
  std::unique_ptr< open_cb_pack > t(open_cb_pack::from(_req_open));

  auto self = instance::from(t->uv_file);
  self->uv_status() = _req_open->result;

  ref_guard< instance > unref_req(*self, adopt_ref);

  if (_req_open->result >= 0)  *t->uv_file = _req_open->result;
  self->path().assign(_req_open->path);

  auto &open_cb = t->on_open_storage.value();
  if (open_cb)  open_cb(file(t->uv_file));
}



/*! \brief Read data from a file. */
class fs::read : public fs
{
  //! \cond
  friend class request::instance< read >;
  //! \endcond

public: /*types*/
  using on_request_t = std::function< void(file _file, ssize_t _nread, buffer _buffer, int64_t _offset) >;
  /*!< \brief The function type of the callback called when data was read from the file. */

protected: /*types*/
  //! \cond
  struct supplemental_data_t
  {
    fs::file::uv_t *uv_file;
    buffer::uv_t *uv_buf;
    int64_t offset;
  };
  //! \endcond

private: /*types*/
  using instance = request::instance< read >;

private: /*constructors*/
  explicit read(uv_t *_uv_req)
  {
    if (_uv_req)  instance::from(_uv_req)->ref();
    uv_req = _uv_req;
  }

public: /*constructors*/
  ~read() = default;
  read()  { uv_req = instance::create(); }

  read(const read&) = default;
  read& operator =(const read&) = default;

  read(read&&) noexcept = default;
  read& operator =(read&&) noexcept = default;

private: /*functions*/
  template< typename = void > static void read_cb(::uv_fs_t*);

public: /*interface*/
  const on_request_t& on_request() const noexcept  { return instance::from(uv_req)->on_request(); }
        on_request_t& on_request()       noexcept  { return instance::from(uv_req)->on_request(); }

  /*! \brief Run the request. Read data from the `_file` into the buffers described by `_buf` object.
      \sa libuv API documentation: [`uv_fs_read()`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_read).
      \sa Linux: [`preadv()`](http://man7.org/linux/man-pages/man2/preadv.2.html).
      \note If the request callback is empty (has not been set), the request runs **synchronously**
      (and `_loop` parameter is ignored).
      In this case the function returns the number of bytes read or the relevant
      [libuv error constant](http://docs.libuv.org/en/v1.x/errors.html#error-constants).*/
  int run(uv::loop _loop, fs::file _file, buffer &_buf, int64_t _offset = -1)
  {
    auto self = instance::from(uv_req);

    auto &request_cb = self->on_request();
    if (request_cb)
    {
      fs::file::instance::from(_file.uv_file)->ref();
      buffer::instance::from(_buf.uv_buf)->ref();
      self->ref();
      self->supplemental_data() = {_file.uv_file, _buf.uv_buf, _offset};
    };

    uv_status(0);
    int o = ::uv_fs_read(
        static_cast< uv::loop::uv_t* >(_loop), static_cast< uv_t* >(uv_req),
        static_cast< fs::file::uv_t >(_file),
        static_cast< const buffer::uv_t* >(_buf), _buf.count(),
        _offset,
        request_cb ? static_cast< ::uv_fs_cb >(read_cb) : nullptr
    );
    if (!o)  uv_status(o);
    return o;
  }

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_req); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_req); }
};

template< typename >
void fs::read::read_cb(::uv_fs_t *_uv_req)
{
  auto self = instance::from(_uv_req);
  self->uv_status() = _uv_req->result;

  ref_guard< fs::file::instance > unref_file(*fs::file::instance::from(self->supplemental_data().uv_file), adopt_ref);
  ref_guard< instance > unref_req(*self, adopt_ref);

  auto &read_cb = self->on_request();
  if (read_cb)
    read_cb(
        file(self->supplemental_data().uv_file),
        _uv_req->result,
        buffer(self->supplemental_data().uv_buf, adopt_ref),
        self->supplemental_data().offset
    );
  else
    buffer::instance::from(self->supplemental_data().uv_buf)->unref();
}



class fs::write : public fs
{
};


}


namespace std
{

//! \ingroup doxy_request
template<> inline void swap(uv::fs::file &_this, uv::fs::file &_that) noexcept  { _this.swap(_that); }

}


#endif
