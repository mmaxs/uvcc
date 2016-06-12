
#ifndef UVCC_REQUEST_FS__HPP
#define UVCC_REQUEST_FS__HPP

#include "uvcc/utility.hpp"
#include "uvcc/request-base.hpp"
#include "uvcc/handle-fs.hpp"
#include "uvcc/handle-io.hpp"
#include "uvcc/buffer.hpp"
#include "uvcc/loop.hpp"

#include <uv.h>
#include <functional>   // function
#ifdef _WIN32
#include <io.h>         // _telli64()
#else
#include <unistd.h>     // lseek64()
#endif


namespace uv
{


/*! \ingroup doxy_group_request
    \brief The base calss for filesystem requests.
    \sa libuv API documentation: [Filesystem operations](http://docs.libuv.org/en/v1.x/fs.html#filesystem-operations).

    \warning As far as all _asynchronous_ file operations in libuv are run on the threadpool, i.e. in separate threads,
    pay attention when running _asynchronous_ requests for sequential reading/writing from/to a file with the `_offset`
    value specified as of < 0 which means using of the "current file position". When the next read/write request on
    a file is scheduled after the previous one has completed and its callback has been called, everything will be OK.
    If several _asynchronous_ requests intended for performing sequential input/output are scheduled as unchained
    operations, some or all of them can be actually preformed simultaneously in parallel threads and the result will
    most probably turn out to be not what was expected. Don't use the "current file position" dummy value in such a case,
    always designate a real effective file offset for each request run. */
class fs : public request
{
  //! \cond
  friend class request::instance< fs >;
  //! \endcond

public: /*types*/
  using uv_t = ::uv_fs_t;

  class close;
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
    file::uv_t *uv_handle = nullptr;
    buffer::uv_t *uv_buf = nullptr;
    int64_t offset = 0;
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
  file handle() const noexcept  { return file(instance::from(uv_req)->properties().uv_handle); }

  /*! \brief The offset this read request has been performed at. */
  int64_t offset() const noexcept  { return instance::from(uv_req)->properties().offset; }

  /*! \brief Run the request. Read data from the `_file` into the buffers described by `_buf` object.
      \sa libuv API documentation: [`uv_fs_read()`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_read).
      \sa Linux: [`preadv()`](http://man7.org/linux/man-pages/man2/preadv.2.html).
      \note If the request callback is empty (has not been set), the request runs _synchronously_.
      In this case the function returns a number of bytes read or relevant libuv error code.

      The `_offset` value of < 0 means using of the current file position. */
  int run(file &_file, buffer &_buf, int64_t _offset)
  {
    int ret = 0;

    if (_offset < 0)
    {
#ifdef _WIN32
      /*! \sa Windows: [`_tell()`, `_telli64()`](https://msdn.microsoft.com/en-us/library/c3kc5e7a.aspx). */
      _offset = _telli64(_file.fd());
#else
      _offset = lseek64(_file.fd(), 0, SEEK_CUR);
#endif
    };

    auto instance_ptr = instance::from(uv_req);

    auto &request_cb = instance_ptr->request_cb_storage.value();
    if (!request_cb)
    {
      auto &properties = instance_ptr->properties();
      properties.uv_handle = static_cast< file::uv_t* >(_file);
      properties.offset = _offset;

      ret = uv_status(::uv_fs_read(
          static_cast< file::uv_t* >(_file)->loop, static_cast< uv_t* >(uv_req),
          _file.fd(),
          static_cast< const buffer::uv_t* >(_buf), _buf.count(),
          _offset,
          nullptr
      ));

      ::uv_fs_req_cleanup(static_cast< uv_t* >(uv_req));
      return ret;
    };


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

    uv_status(0);
    ret = ::uv_fs_read(
        static_cast< file::uv_t* >(_file)->loop, static_cast< uv_t* >(uv_req),
        _file.fd(),
        static_cast< const buffer::uv_t* >(_buf), _buf.count(),
        _offset,
        read_cb
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

  ::uv_fs_req_cleanup(_uv_req);
}



/*! \brief Write data to a file. */
class fs::write : public fs
{
  //! \cond
  friend class request::instance< write >;
  friend class output;
  //! \endcond

public: /*types*/
  using on_request_t = std::function< void(write _request, buffer _buffer) >;
  /*!< \brief The function type of the callback called when data was written to the file. */

protected: /*types*/
  //! \cond
  struct properties
  {
    file::uv_t *uv_handle = nullptr;
    buffer::uv_t *uv_buf = nullptr;
    int64_t offset = 0;
    std::size_t pending_size = 0;
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
  file handle() const noexcept  { return file(instance::from(uv_req)->properties().uv_handle); }

  /*! \brief The offset this write request has been performed at. */
  int64_t offset() const noexcept  { return instance::from(uv_req)->properties().offset; }

  /*! \brief Run the request. Write data to the `_file` from the buffers described by `_buf` object.
      \sa libuv API documentation: [`uv_fs_write()`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_write).
      \sa Linux: [`pwritev()`](http://man7.org/linux/man-pages/man2/pwritev.2.html).
      \note If the request callback is empty (has not been set), the request runs _synchronously_.
      In this case the function returns a number of bytes written or relevant libuv error code.

      The `_offset` value of < 0 means using of the current file position. */
  int run(file &_file, const buffer &_buf, int64_t _offset)
  {
    int ret = 0;

    if (_offset < 0)
    {
#ifdef _WIN32
      /*! \sa Windows: [`_tell()`, `_telli64()`](https://msdn.microsoft.com/en-us/library/c3kc5e7a.aspx). */
      _offset = _telli64(_file.fd());
#else
      _offset = lseek64(_file.fd(), 0, SEEK_CUR);
#endif
    };

    auto instance_ptr = instance::from(uv_req);

    auto &request_cb = instance_ptr->request_cb_storage.value();
    if (!request_cb)
    {
      auto &properties = instance_ptr->properties();
      properties.uv_handle = static_cast< file::uv_t* >(_file);
      properties.offset = _offset;

      ret = uv_status(::uv_fs_write(
          static_cast< file::uv_t* >(_file)->loop, static_cast< uv_t* >(uv_req),
          _file.fd(),
          static_cast< const buffer::uv_t* >(_buf), _buf.count(),
          _offset,
          nullptr
      ));

      ::uv_fs_req_cleanup(static_cast< uv_t* >(uv_req));
      return ret;
    };


    file::instance::from(_file.uv_handle)->ref();
    buffer::instance::from(_buf.uv_buf)->ref();
    instance_ptr->ref();

    std::size_t wr_size = 0;
    for (std::size_t i = 0, buf_count = _buf.count(); i < buf_count; ++i)  wr_size += _buf.len(i);

    // instance_ptr->properties() = {static_cast< file::uv_t* >(_file), _buf.uv_buf, _offset, wr_size};
    {
      auto &properties = instance_ptr->properties();
      properties.uv_handle = static_cast< file::uv_t* >(_file);
      properties.uv_buf = _buf.uv_buf;
      properties.offset = _offset;
      properties.pending_size = wr_size;
    }

    file::instance::from(_file.uv_handle)->properties().write_queue_size += wr_size;

    uv_status(0);
    ret = ::uv_fs_write(
        static_cast< file::uv_t* >(_file)->loop, static_cast< uv_t* >(uv_req),
        _file.fd(),
        static_cast< const buffer::uv_t* >(_buf), _buf.count(),
        _offset,
        write_cb
    );
    if (!ret)  uv_status(ret);
    return ret;
  }

  /*! \brief Try to execute the request _synchronously_ if it can be completed immediately...
      \details ...i.e. if `(_file.write_queue_size() == 0)` and without calling the request callback.\n
      The `_offset` value of < 0 means using of the current file position.
      \returns A number of bytes written, or relevant libuv error code, or `UV_EAGAIN` error code when
      the data can’t be written immediately. */
  int try_write(file &_file, const buffer &_buf, int64_t _offset)
  {
    if (_file.write_queue_size() != 0)  return uv_status(UV_EAGAIN);

    int ret = uv_status(::uv_fs_write(
        static_cast< file::uv_t* >(_file)->loop, static_cast< uv_t* >(uv_req),
        _file.fd(),
        static_cast< const buffer::uv_t* >(_buf), _buf.count(),
        _offset,
        nullptr
    ));

    ::uv_fs_req_cleanup(static_cast< uv_t* >(uv_req));
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

  // if (_uv_req->result > 0)  file_instance_ptr->properties().write_queue_size -= _uv_req->result;
  file_instance_ptr->properties().write_queue_size -= properties.pending_size;

  auto &write_cb = instance_ptr->request_cb_storage.value();
  if (write_cb)
    write_cb(write(_uv_req), buffer(properties.uv_buf, adopt_ref));
  else
    buffer::instance::from(properties.uv_buf)->unref();

  ::uv_fs_req_cleanup(_uv_req);
}



/*! \brief Synchronize a file's state with storage device. */
class fs::sync : public fs
{
  //! \cond
  friend class request::instance< sync >;
  //! \endcond

public: /*types*/
  using on_request_t = std::function< void(sync _request) >;
  /*!< \brief The function type of the callback called when the sync request has completed. */

protected: /*types*/
  //! \cond
  struct properties
  {
    file::uv_t *uv_handle = nullptr;
  };
  //! \endcond

private: /*types*/
  using instance = request::instance< sync >;

private: /*constructors*/
  explicit sync(uv_t *_uv_req)
  {
    if (_uv_req)  instance::from(_uv_req)->ref();
    uv_req = _uv_req;
  }

public: /*constructors*/
  ~sync() = default;
  sync()  { uv_req = instance::create(); }

  sync(const sync&) = default;
  sync& operator =(const sync&) = default;

  sync(sync&&) noexcept = default;
  sync& operator =(sync&&) noexcept = default;

private: /*functions*/
  template< typename = void > static void sync_cb(::uv_fs_t*);

public: /*interface*/
  const on_request_t& on_request() const noexcept  { return instance::from(uv_req)->request_cb_storage.value(); }
        on_request_t& on_request()       noexcept  { return instance::from(uv_req)->request_cb_storage.value(); }

  /*! \brief The file which this sync request is running on.
      \details It is guaranteed that it will be a valid instance at least within the request callback. */
  file handle() const noexcept  { return file(instance::from(uv_req)->properties().uv_handle); }

  /*! \brief Run the request. Flush the data of the `_file` to the storage device.
      \sa libuv API documentation: [`uv_fs_fsync()`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_fsync),
                                   [`uv_fs_fdatasync()`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_fdatasync).
      \sa Linux: [`fsync()`](http://man7.org/linux/man-pages/man2/fsync.2.html),
                 [`fdatasync()`](http://man7.org/linux/man-pages/man2/fdatasync.2.html).
      \note If the request callback is empty (has not been set), the request runs _synchronously_. */
  int run(file &_file, bool _flush_metadata = false)
  {
    int ret = 0;
    auto uv_fsync_function = _flush_metadata ? ::uv_fs_fsync : uv_fs_fdatasync;

    auto instance_ptr = instance::from(uv_req);

    auto &request_cb = instance_ptr->request_cb_storage.value();
    if (!request_cb)
    {
      instance_ptr->properties().uv_handle = static_cast< file::uv_t* >(_file);

      ret = uv_status(uv_fsync_function(
          static_cast< file::uv_t* >(_file)->loop, static_cast< uv_t* >(uv_req),
          _file.fd(),
          nullptr
      ));

      ::uv_fs_req_cleanup(static_cast< uv_t* >(uv_req));
      return ret;
    };


    file::instance::from(_file.uv_handle)->ref();
    instance_ptr->ref();

    // instance_ptr->properties() = {static_cast< file::uv_t* >(_file)};
    {
      auto &properties = instance_ptr->properties();
      properties.uv_handle = static_cast< file::uv_t* >(_file);
    }

    uv_status(0);
    ret = uv_fsync_function(
        static_cast< file::uv_t* >(_file)->loop, static_cast< uv_t* >(uv_req),
        _file.fd(),
        sync_cb
    );
    if (!ret)  uv_status(ret);
    return ret;
  }

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_req); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_req); }
};

template< typename >
void fs::sync::sync_cb(::uv_fs_t *_uv_req)
{
  auto instance_ptr = instance::from(_uv_req);
  instance_ptr->uv_error = _uv_req->result;

  auto &properties = instance_ptr->properties();

  ref_guard< file::instance > unref_file(*file::instance::from(properties.uv_handle), adopt_ref);
  ref_guard< instance > unref_req(*instance_ptr, adopt_ref);

  auto &sync_cb = instance_ptr->request_cb_storage.value();
  if (sync_cb)  sync_cb(sync(_uv_req));

  ::uv_fs_req_cleanup(_uv_req);
}



/*! \brief Truncate a file to a specified length. */
class fs::truncate : public fs
{
  //! \cond
  friend class request::instance< truncate >;
  //! \endcond

public: /*types*/
  using on_request_t = std::function< void(truncate _request) >;
  /*!< \brief The function type of the callback called when the truncate request has completed. */

protected: /*types*/
  //! \cond
  struct properties
  {
    file::uv_t *uv_handle = nullptr;
    int64_t offset = 0;
  };
  //! \endcond

private: /*types*/
  using instance = request::instance< truncate >;

private: /*constructors*/
  explicit truncate(uv_t *_uv_req)
  {
    if (_uv_req)  instance::from(_uv_req)->ref();
    uv_req = _uv_req;
  }

public: /*constructors*/
  ~truncate() = default;
  truncate()  { uv_req = instance::create(); }

  truncate(const truncate&) = default;
  truncate& operator =(const truncate&) = default;

  truncate(truncate&&) noexcept = default;
  truncate& operator =(truncate&&) noexcept = default;

private: /*functions*/
  template< typename = void > static void truncate_cb(::uv_fs_t*);

public: /*interface*/
  const on_request_t& on_request() const noexcept  { return instance::from(uv_req)->request_cb_storage.value(); }
        on_request_t& on_request()       noexcept  { return instance::from(uv_req)->request_cb_storage.value(); }

  /*! \brief The file which this truncate request is running on.
      \details It is guaranteed that it will be a valid instance at least within the request callback. */
  file handle() const noexcept  { return file(instance::from(uv_req)->properties().uv_handle); }

  /*! \brief The offset this trucate request has been performed at. */
  int64_t offset() const noexcept  { return instance::from(uv_req)->properties().offset; }

  /*! \brief Run the request. Truncate the `_file` to a size of `_offset` bytes.
      \sa libuv API documentation: [`uv_fs_ftruncate()`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_ftruncate).
      \sa Linux: [`ftruncate()`](http://man7.org/linux/man-pages/man2/ftruncate.2.html).
      \note If the request callback is empty (has not been set), the request runs _synchronously_.

      The `_offset` value of < 0 means using of the current file position. */
  int run(file &_file, int64_t _offset)
  {
    int ret = 0;

    if (_offset < 0)
    {
#ifdef _WIN32
      /*! \sa Windows: [`_tell()`, `_telli64()`](https://msdn.microsoft.com/en-us/library/c3kc5e7a.aspx). */
      _offset = _telli64(_file.fd());
#else
      _offset = lseek64(_file.fd(), 0, SEEK_CUR);
#endif
    };

    auto instance_ptr = instance::from(uv_req);

    auto &request_cb = instance_ptr->request_cb_storage.value();
    if (!request_cb)
    {
      auto &properties = instance_ptr->properties();
      properties.uv_handle = static_cast< file::uv_t* >(_file);
      properties.offset = _offset;

      ret = uv_status(::uv_fs_ftruncate(
          static_cast< file::uv_t* >(_file)->loop, static_cast< uv_t* >(uv_req),
          _file.fd(),
          _offset,
          nullptr
      ));

      ::uv_fs_req_cleanup(static_cast< uv_t* >(uv_req));
      return ret;
    };


    file::instance::from(_file.uv_handle)->ref();
    instance_ptr->ref();

    // instance_ptr->properties() = {static_cast< file::uv_t* >(_file), _offset};
    {
      auto &properties = instance_ptr->properties();
      properties.uv_handle = static_cast< file::uv_t* >(_file);
      properties.offset = _offset;
    }

    uv_status(0);
    ret = ::uv_fs_ftruncate(
        static_cast< file::uv_t* >(_file)->loop, static_cast< uv_t* >(uv_req),
        _file.fd(),
        _offset,
        truncate_cb
    );
    if (!ret)  uv_status(ret);
    return ret;
  }

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_req); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_req); }
};

template< typename >
void fs::truncate::truncate_cb(::uv_fs_t *_uv_req)
{
  auto instance_ptr = instance::from(_uv_req);
  instance_ptr->uv_error = _uv_req->result;

  auto &properties = instance_ptr->properties();

  ref_guard< file::instance > unref_file(*file::instance::from(properties.uv_handle), adopt_ref);
  ref_guard< instance > unref_req(*instance_ptr, adopt_ref);

  auto &truncate_cb = instance_ptr->request_cb_storage.value();
  if (truncate_cb)  truncate_cb(truncate(_uv_req));

  ::uv_fs_req_cleanup(_uv_req);
}



/*! \brief Transfer data between file descriptors. */
class fs::sendfile : public fs
{
  //! \cond
  friend class request::instance< sendfile >;
  friend class output;
  //! \endcond

public: /*types*/
  using on_request_t = std::function< void(sendfile _request) >;
  /*!< \brief The function type of the callback called when the sendfile request has completed. */

protected: /*types*/
  //! \cond
  struct properties
  {
    io::uv_t *uv_handle_out = nullptr;
    file::uv_t *uv_handle_in = nullptr;
    int64_t offset = 0;
  };
  //! \endcond

private: /*types*/
  using instance = request::instance< sendfile >;

private: /*constructors*/
  explicit sendfile(uv_t *_uv_req)
  {
    if (_uv_req)  instance::from(_uv_req)->ref();
    uv_req = _uv_req;
  }

public: /*constructors*/
  ~sendfile() = default;
  sendfile()  { uv_req = instance::create(); }

  sendfile(const sendfile&) = default;
  sendfile& operator =(const sendfile&) = default;

  sendfile(sendfile&&) noexcept = default;
  sendfile& operator =(sendfile&&) noexcept = default;

private: /*functions*/
  template< typename = void > static void sendfile_cb(::uv_fs_t*);

public: /*interface*/
  const on_request_t& on_request() const noexcept  { return instance::from(uv_req)->request_cb_storage.value(); }
        on_request_t& on_request()       noexcept  { return instance::from(uv_req)->request_cb_storage.value(); }

  /*! \brief The I/O endpoint which this sendfile request has been writing data to.
      \details It is guaranteed that it will be a valid instance at least within the request callback. */
  io handle_out() const noexcept  { return io(instance::from(uv_req)->properties().uv_handle_out); }
  /*! \brief The file which this sendfile request has been reading data from.
      \details It is guaranteed that it will be a valid instance at least within the request callback. */
  file handle_in() const noexcept  { return file(instance::from(uv_req)->properties().uv_handle_in); }

  /*! \brief The offset this sendfile request has been started to read data at from input file. */
  int64_t offset() const noexcept  { return instance::from(uv_req)->properties().offset; }

  /*! \brief Run the request. Read data from the `_in` file starting from the `_offset` and copy it to the `_out` endpoint.
      \details `_length` is the number of bytes to be transferred.
      \sa libuv API documentation: [`uv_fs_sendfile()`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_sendfile).
      \sa Linux: [`sendfile()`](http://man7.org/linux/man-pages/man2/sendfile.2.html).
      \note If the request callback is empty (has not been set), the request runs _synchronously_.
      In this case the function returns a number of bytes copied or relevant libuv error code.

      The `_offset` value of < 0 means using of the current file position. */
  int run(io &_out, file &_in, int64_t _offset, std::size_t _length)
  {
    int ret = 0;

    if (_offset < 0)
    {
#ifdef _WIN32
      /*! \sa Windows: [`_tell()`, `_telli64()`](https://msdn.microsoft.com/en-us/library/c3kc5e7a.aspx). */
      _offset = _telli64(_in.fd());
#else
      _offset = lseek64(_in.fd(), 0, SEEK_CUR);
#endif
    };

    auto instance_ptr = instance::from(uv_req);

    auto &request_cb = instance_ptr->request_cb_storage.value();
    if (!request_cb)
    {
      auto &properties = instance_ptr->properties();
      properties.uv_handle_out = static_cast< io::uv_t* >(_out);
      properties.uv_handle_in = static_cast< file::uv_t* >(_in);
      properties.offset = _offset;

      ret = uv_status(::uv_fs_sendfile(
          static_cast< file::uv_t* >(_in)->loop, static_cast< uv_t* >(uv_req),
          _out.fileno(), _in.fd(),
          _offset, _length,
          nullptr
      ));

      ::uv_fs_req_cleanup(static_cast< uv_t* >(uv_req));
      return ret;
    };


    io::instance::from(_out.uv_handle)->ref();
    file::instance::from(_in.uv_handle)->ref();
    instance_ptr->ref();

    // instance_ptr->properties() = {static_cast< io::uv_t* >(_out), static_cast< file::uv_t* >(_in), _offset};
    {
      auto &properties = instance_ptr->properties();
      properties.uv_handle_out = static_cast< io::uv_t* >(_out);
      properties.uv_handle_in = static_cast< file::uv_t* >(_in);
      properties.offset = _offset;
    }

    uv_status(0);
    ret = ::uv_fs_sendfile(
        static_cast< file::uv_t* >(_in)->loop, static_cast< uv_t* >(uv_req),
        _out.fileno(), _in.fd(),
        _offset, _length,
        sendfile_cb
    );
    if (!ret)  uv_status(ret);
    return ret;
  }

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_req); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_req); }
};

template< typename >
void fs::sendfile::sendfile_cb(::uv_fs_t *_uv_req)
{/*
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

  ::uv_fs_req_cleanup(_uv_req);
*/}


}


#endif
