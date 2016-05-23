
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


/*! \ingroup doxy_group_request
    \brief The base calss for filesystem requests.
    \sa libuv API documentation: [Filesystem operations](http://docs.libuv.org/en/v1.x/fs.html#filesystem-operations). */
class fs : public request
{
  //! \cond
  friend class request::instance< fs >;
  //! \endcond

public: /*types*/
  using uv_t = ::uv_fs_t;

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



/*! \brief Read data from a file. */
class fs::read : public fs
{
  //! \cond
  friend class request::instance< read >;
  //! \endcond

public: /*types*/
  using on_request_t = std::function< void(read _request, buffer _buffer) >;
  /*!< \brief The function type of the callback called when data was read from the file. */

protected: /*types*/
  //! \cond
  struct properties
  {
    uv::file::uv_t *uv_handle = nullptr;
    buffer::uv_t *uv_buf = nullptr;
    int64_t offset = -1;
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
  const on_request_t& on_request() const noexcept  { return instance::from(uv_req)->request_cb_storage.value(); }
        on_request_t& on_request()       noexcept  { return instance::from(uv_req)->request_cb_storage.value(); }

  /*! \brief The file which this read request is running on.
      \details It is guaranteed that it will be a valid instance at least within the request callback. */
  uv::file file() const noexcept  { return uv::file(instance::from(uv_req)->properties().uv_handle); }
  /*! \brief The offset this read request has been performed at. */
  int64_t offset() const noexcept  { return instance::from(uv_req)->properties().offset; }

  /*! \brief Run the request. Read data from the `_file` into the buffers described by `_buf` object.
      \sa libuv API documentation: [`uv_fs_read()`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_read).
      \sa Linux: [`preadv()`](http://man7.org/linux/man-pages/man2/preadv.2.html).
      \note If the request callback is empty (has not been set), the request runs _synchronously_
      (and the `_loop` parameter is ignored).
      In this case the function returns a number of bytes read or relevant
      [libuv error constant](http://docs.libuv.org/en/v1.x/errors.html#error-constants).*/
  int run(uv::file _file, buffer &_buf, int64_t _offset = -1)
  {
    auto instance_ptr = instance::from(uv_req);

    auto &request_cb = instance_ptr->request_cb_storage.value();
    if (request_cb)
    {
      file::instance::from(_file.uv_handle)->ref();
      buffer::instance::from(_buf.uv_buf)->ref();
      instance_ptr->ref();

      // instance_ptr->properties() = {static_cast< file::uv_t* >(_file), _buf.uv_buf, _offset};
      {
        auto &properties = instance_ptr->properties();
        properties.uv_handle = static_cast< file::uv_t* >(_file);
        properties.uv_buf = _buf.uv_buf;
        properties.offset = _offset;
      }
    };

    uv_status(0);
    int ret = ::uv_fs_read(
        static_cast< uv::file::uv_t* >(_file)->loop, static_cast< uv_t* >(uv_req),
        _file.fd(),
        static_cast< const buffer::uv_t* >(_buf), _buf.count(),
        _offset,
        request_cb ? static_cast< ::uv_fs_cb >(read_cb) : nullptr
    );
    if (!ret)  uv_status(ret);
    return ret;
  }

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_req); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_req); }
};

template< typename >
void fs::read::read_cb(::uv_fs_t *_uv_req)
{
  auto instance_ptr = instance::from(_uv_req);
  instance_ptr->uv_error = _uv_req->result;

  auto &properties = instance_ptr->properties();

  ref_guard< file::instance > unref_file(*file::instance::from(properties.uv_handle), adopt_ref);
  ref_guard< instance > unref_req(*instance_ptr, adopt_ref);

  auto &read_cb = instance_ptr->request_cb_storage.value();
  if (read_cb)
    read_cb(read(_uv_req), buffer(properties.uv_buf, adopt_ref));
  else
    buffer::instance::from(properties.uv_buf)->unref();
}



/*! \brief Write data to a file. */
class fs::write : public fs
{
  //! \cond
  friend class request::instance< write >;
  //! \endcond

public: /*types*/
  using on_request_t = std::function< void(write _request, buffer _buffer) >;
  /*!< \brief The function type of the callback called when data was written to the file. */

protected: /*types*/
  //! \cond
  struct properties
  {
    uv::file::uv_t *uv_handle = nullptr;
    buffer::uv_t *uv_buf = nullptr;
    int64_t offset = -1;
  };
  //! \endcond

private: /*types*/
  using instance = request::instance< write >;

private: /*constructors*/
  explicit write(uv_t *_uv_req)
  {
    if (_uv_req)  instance::from(_uv_req)->ref();
    uv_req = _uv_req;
  }

public: /*constructors*/
  ~write() = default;
  write()  { uv_req = instance::create(); }

  write(const write&) = default;
  write& operator =(const write&) = default;

  write(write&&) noexcept = default;
  write& operator =(write&&) noexcept = default;

private: /*functions*/
  template< typename = void > static void write_cb(::uv_fs_t*);

public: /*interface*/
  const on_request_t& on_request() const noexcept  { return instance::from(uv_req)->request_cb_storage.value(); }
        on_request_t& on_request()       noexcept  { return instance::from(uv_req)->request_cb_storage.value(); }

  /*! \brief The file which this write request has been running on.
      \details It is guaranteed that it will be a valid instance at least within the request callback. */
  uv::file file() const noexcept  { return uv::file(instance::from(uv_req)->properties().uv_handle); }
  /*! \brief The offset this write request has been performed at. */
  int64_t offset() const noexcept  { return instance::from(uv_req)->properties().offset; }

  /*! \brief Run the request. Write data to the `_file` from the buffers described by `_buf` object.
      \sa libuv API documentation: [`uv_fs_write()`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_write).
      \sa Linux: [`pwritev()`](http://man7.org/linux/man-pages/man2/pwritev.2.html).
      \note If the request callback is empty (has not been set), the request runs _synchronously_
      (and the `_loop` parameter is ignored).
      In this case the function returns a number of bytes written or relevant
      [libuv error constant](http://docs.libuv.org/en/v1.x/errors.html#error-constants).*/
  int run(uv::file _file, const buffer &_buf, int64_t _offset = -1)
  {
    auto instance_ptr = instance::from(uv_req);

    auto &request_cb = instance_ptr->request_cb_storage.value();
    if (request_cb)
    {
      file::instance::from(_file.uv_handle)->ref();
      buffer::instance::from(_buf.uv_buf)->ref();
      instance_ptr->ref();

      // instance_ptr->properties() = {static_cast< file::uv_t* >(_file), _buf.uv_buf, _offset};
      {
        auto &properties = instance_ptr->properties();
        properties.uv_handle = static_cast< file::uv_t* >(_file);
        properties.uv_buf = _buf.uv_buf;
        properties.offset = _offset;
      }

      std::size_t wr_size = 0;
      for (std::size_t i = 0, buf_count = _buf.count(); i < buf_count; ++i)  wr_size += _buf.len(i);
      file::instance::from(_file.uv_handle)->properties().write_queue_size += wr_size;
    };

    uv_status(0);
    int ret = ::uv_fs_write(
        static_cast< uv::file::uv_t* >(_file)->loop, static_cast< uv_t* >(uv_req),
        _file.fd(),
        static_cast< const buffer::uv_t* >(_buf), _buf.count(),
        _offset,
        request_cb ? static_cast< ::uv_fs_cb >(write_cb) : nullptr
    );
    if (!ret)  uv_status(ret);
    return ret;
  }

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_req); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_req); }
};

template< typename >
void fs::write::write_cb(::uv_fs_t *_uv_req)
{
  auto instance_ptr = instance::from(_uv_req);
  instance_ptr->uv_error = _uv_req->result;

  auto &properties = instance_ptr->properties();
  auto file_instance_ptr = file::instance::from(properties.uv_handle);

  ref_guard< file::instance > unref_file(*file_instance_ptr, adopt_ref);
  ref_guard< instance > unref_req(*instance_ptr, adopt_ref);

  if (_uv_req->result > 0)  file_instance_ptr->properties().write_queue_size -= _uv_req->result;

  auto &write_cb = instance_ptr->request_cb_storage.value();
  if (write_cb)
    write_cb(write(_uv_req), buffer(properties.uv_buf, adopt_ref));
  else
    buffer::instance::from(properties.uv_buf)->unref();
}


}


#endif
