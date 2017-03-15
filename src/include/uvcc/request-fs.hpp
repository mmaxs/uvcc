
#ifndef UVCC_REQUEST_FS__HPP
#define UVCC_REQUEST_FS__HPP

#include "uvcc/utility.hpp"
#include "uvcc/request-base.hpp"
#include "uvcc/handle-fs.hpp"
#include "uvcc/handle-io.hpp"
#include "uvcc/buffer.hpp"
#include "uvcc/loop.hpp"

#include <uv.h>

#ifdef _WIN32
#include <io.h>         // _telli64()
#else
#include <unistd.h>     // lseek64()
#endif

#include <functional>   // function
#include <type_traits>  // enable_if_t is_convertible
#include <string>       // string


namespace uv
{


/*! \ingroup doxy_group__request
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
  class readlink;
  class realpath;

protected: /*types*/
  //! \cond
  struct properties
  {
    uv_t *uv_req = nullptr;
    ~properties()  { if (uv_req)  ::uv_fs_req_cleanup(uv_req); }
  };
  //! \endcond

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

protected: /*interface*/
  //! \cond
  template< ::uv_fs_type _UV_FS_TYPE_ > void init()
  {
    static_cast< uv_t* >(uv_req)->type = UV_FS;
    static_cast< uv_t* >(uv_req)->fs_type = _UV_FS_TYPE_;
    instance::from(uv_req)->properties().uv_req = static_cast< uv_t* >(uv_req);
  }
  //! \endcond

public: /*interface*/
  /*! \brief The tag indicating a subtype of the filesystem request.
      \sa libuv API documentation: [`uv_fs_type`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_type). */
  ::uv_fs_type fs_type() const noexcept  { return static_cast< uv_t* >(uv_req)->fs_type; }

  /*! \brief The libuv loop that started this filesystem request and where completion will be reported. */
  uv::loop loop() const noexcept  { return uv::loop(static_cast< uv_t* >(uv_req)->loop); }

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_req); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_req); }
};



/*! \brief Close a file handle. */
class fs::close : public fs
{
  //! \cond
  friend class request::instance< close >;
  //! \endcond

public: /*types*/
  using on_request_t = std::function< void(close _request) >;
  /*!< \brief The function type of the callback called when the `close` request has completed. */

protected: /*types*/
  //! \cond
  struct properties : public fs::properties
  {
    file::uv_t *uv_handle = nullptr;
  };
  //! \endcond

private: /*types*/
  using instance = request::instance< close >;

private: /*constructors*/
  explicit close(uv_t *_uv_req)
  {
    if (_uv_req)  instance::from(_uv_req)->ref();
    uv_req = _uv_req;
  }

public: /*constructors*/
  ~close() = default;
  close()
  {
    uv_req = instance::create();
    init< UV_FS_CLOSE >();
  }

  close(const close&) = default;
  close& operator =(const close&) = default;

  close(close&&) noexcept = default;
  close& operator =(close&&) noexcept = default;

private: /*functions*/
  template< typename = void > static void close_cb(::uv_fs_t*);

public: /*interface*/
  const on_request_t& on_request() const noexcept  { return instance::from(uv_req)->request_cb_storage.value(); }
        on_request_t& on_request()       noexcept  { return instance::from(uv_req)->request_cb_storage.value(); }

  /*! \brief The file which this close request has been running on.
      \details It is guaranteed that it will be a valid instance at least within the request callback. */
  file handle() const noexcept  { return file(instance::from(uv_req)->properties().uv_handle); }

  /*! \brief Run the request. Close a `_file` handle.
      \sa libuv API documentation: [`uv_fs_close()`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_close).
      \sa Linux: [`close()`](http://man7.org/linux/man-pages/man2/close.2.html).
      \note If the request callback is empty (has not been set), the request runs _synchronously_. */
  int run(file &_file)
  {
    file::instance::from(_file.uv_handle)->properties().is_closing = true;

    auto instance_ptr = instance::from(uv_req);

    ::uv_fs_req_cleanup(static_cast< uv_t* >(uv_req));  // assuming that *uv_req has initially been nulled

    if (!instance_ptr->request_cb_storage.value())
    {
      instance_ptr->properties().uv_handle = static_cast< file::uv_t* >(_file);

      return uv_status(::uv_fs_close(
          static_cast< file::uv_t* >(_file)->loop, static_cast< uv_t* >(uv_req),
          _file.fd(),
          nullptr
      ));
    }


    file::instance::from(_file.uv_handle)->ref();
    instance_ptr->ref();

    // instance_ptr->properties() = { static_cast< file::uv_t* >(_file) };
    {
      auto &properties = instance_ptr->properties();
      properties.uv_handle = static_cast< file::uv_t* >(_file);
    }

    uv_status(0);
    int ret = ::uv_fs_close(
        static_cast< file::uv_t* >(_file)->loop, static_cast< uv_t* >(uv_req),
        _file.fd(),
        close_cb
    );
    if (!ret)  uv_status(ret);
    return ret;
  }

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_req); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_req); }
};

template< typename >
void fs::close::close_cb(::uv_fs_t *_uv_req)
{
  auto instance_ptr = instance::from(_uv_req);
  instance_ptr->uv_error = _uv_req->result;

  auto &properties = instance_ptr->properties();

  ref_guard< file::instance > unref_file(*file::instance::from(properties.uv_handle), adopt_ref);
  ref_guard< instance > unref_req(*instance_ptr, adopt_ref);

  auto &close_cb = instance_ptr->request_cb_storage.value();
  if (close_cb)  close_cb(close(_uv_req));
}



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
  struct properties : public fs::properties
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
  read()
  {
    uv_req = instance::create();
    init< UV_FS_READ >();
  }

  read(const read&) = default;
  read& operator =(const read&) = default;

  read(read&&) noexcept = default;
  read& operator =(read&&) noexcept = default;

private: /*functions*/
  template< typename = void > static void read_cb(::uv_fs_t*);

public: /*interface*/
  const on_request_t& on_request() const noexcept  { return instance::from(uv_req)->request_cb_storage.value(); }
        on_request_t& on_request()       noexcept  { return instance::from(uv_req)->request_cb_storage.value(); }

  /*! \brief The file which this read request has been running on.
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
    if (_offset < 0)
    {
#ifdef _WIN32
      /*! \sa Windows: [`_tell()`, `_telli64()`](https://msdn.microsoft.com/en-us/library/c3kc5e7a.aspx). */
      _offset = _telli64(_file.fd());
#else
      _offset = lseek64(_file.fd(), 0, SEEK_CUR);
#endif
    }

    auto instance_ptr = instance::from(uv_req);

    ::uv_fs_req_cleanup(static_cast< uv_t* >(uv_req));  // assuming that *uv_req has initially been nulled

    if (!instance_ptr->request_cb_storage.value())
    {
      auto &properties = instance_ptr->properties();
      properties.uv_handle = static_cast< file::uv_t* >(_file);
      properties.offset = _offset;

      return uv_status(::uv_fs_read(
          static_cast< file::uv_t* >(_file)->loop, static_cast< uv_t* >(uv_req),
          _file.fd(),
          static_cast< const buffer::uv_t* >(_buf), _buf.count(),
          _offset,
          nullptr
      ));
    }


    file::instance::from(_file.uv_handle)->ref();
    buffer::instance::from(_buf.uv_buf)->ref();
    instance_ptr->ref();

    // instance_ptr->properties() = { static_cast< file::uv_t* >(_file), _buf.uv_buf, _offset };
    {
      auto &properties = instance_ptr->properties();
      properties.uv_handle = static_cast< file::uv_t* >(_file);
      properties.uv_buf = _buf.uv_buf;
      properties.offset = _offset;
    }

    uv_status(0);
    int ret = ::uv_fs_read(
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
  struct properties  // since `fs::write` can be static-casted from an `output` request it appears to be an exception from the common scheme
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
  write()
  {
    uv_req = instance::create();
    static_cast< uv_t* >(uv_req)->type = UV_FS;
    static_cast< uv_t* >(uv_req)->fs_type = UV_FS_WRITE;
  }

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
    }

    auto instance_ptr = instance::from(uv_req);

    if (!instance_ptr->request_cb_storage.value())
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
    }


    file::instance::from(_file.uv_handle)->ref();
    buffer::instance::from(_buf.uv_buf)->ref();
    instance_ptr->ref();

    std::size_t wr_size = 0;
    for (std::size_t i = 0, buf_count = _buf.count(); i < buf_count; ++i)  wr_size += _buf.len(i);

    // instance_ptr->properties() = { static_cast< file::uv_t* >(_file), _buf.uv_buf, _offset, wr_size };
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

  // if (_uv_req->result > 0)  file_instance_ptr->properties().write_queue_size -= _uv_req->result;  // don't mistakenly use the actual written bytes amount
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
  /*!< \brief The function type of the callback called when the `sync` request has completed. */

protected: /*types*/
  //! \cond
  struct properties : public fs::properties
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
  sync()
  {
    uv_req = instance::create();
    init< UV_FS_FSYNC >();
  }

  sync(const sync&) = default;
  sync& operator =(const sync&) = default;

  sync(sync&&) noexcept = default;
  sync& operator =(sync&&) noexcept = default;

private: /*functions*/
  template< typename = void > static void sync_cb(::uv_fs_t*);

public: /*interface*/
  const on_request_t& on_request() const noexcept  { return instance::from(uv_req)->request_cb_storage.value(); }
        on_request_t& on_request()       noexcept  { return instance::from(uv_req)->request_cb_storage.value(); }

  /*! \brief The file which this sync request has been running on.
      \details It is guaranteed that it will be a valid instance at least within the request callback. */
  file handle() const noexcept  { return file(instance::from(uv_req)->properties().uv_handle); }

  /*! \brief Run the request. Flush the data of the `_file` to the storage device.
      \sa libuv API documentation: [`uv_fs_fsync()`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_fsync),
                                   [`uv_fs_fdatasync()`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_fdatasync).
      \sa Linux: [`fsync()`](http://man7.org/linux/man-pages/man2/fsync.2.html),
                 [`fdatasync()`](http://man7.org/linux/man-pages/man2/fdatasync.2.html).

      By default [`uv_fs_fdatasync()`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_fdatasync) libuv API function is used.
      \note If the request callback is empty (has not been set), the request runs _synchronously_. */
  int run(file &_file, bool _flush_all_metadata = false)
  {
    auto uv_fsync_function = _flush_all_metadata ? ::uv_fs_fsync : uv_fs_fdatasync;
    auto instance_ptr = instance::from(uv_req);

    ::uv_fs_req_cleanup(static_cast< uv_t* >(uv_req));  // assuming that *uv_req has initially been nulled

    if (!instance_ptr->request_cb_storage.value())
    {
      instance_ptr->properties().uv_handle = static_cast< file::uv_t* >(_file);

      return uv_status(uv_fsync_function(
          static_cast< file::uv_t* >(_file)->loop, static_cast< uv_t* >(uv_req),
          _file.fd(),
          nullptr
      ));
    }


    file::instance::from(_file.uv_handle)->ref();
    instance_ptr->ref();

    // instance_ptr->properties() = { static_cast< file::uv_t* >(_file) };
    {
      auto &properties = instance_ptr->properties();
      properties.uv_handle = static_cast< file::uv_t* >(_file);
    }

    uv_status(0);
    int ret = uv_fsync_function(
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
}



/*! \brief Truncate a file to a specified length. */
class fs::truncate : public fs
{
  //! \cond
  friend class request::instance< truncate >;
  //! \endcond

public: /*types*/
  using on_request_t = std::function< void(truncate _request) >;
  /*!< \brief The function type of the callback called when the `truncate` request has completed. */

protected: /*types*/
  //! \cond
  struct properties : public fs::properties
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
  truncate()
  {
    uv_req = instance::create();
    init< UV_FS_FTRUNCATE >();
  }

  truncate(const truncate&) = default;
  truncate& operator =(const truncate&) = default;

  truncate(truncate&&) noexcept = default;
  truncate& operator =(truncate&&) noexcept = default;

private: /*functions*/
  template< typename = void > static void truncate_cb(::uv_fs_t*);

public: /*interface*/
  const on_request_t& on_request() const noexcept  { return instance::from(uv_req)->request_cb_storage.value(); }
        on_request_t& on_request()       noexcept  { return instance::from(uv_req)->request_cb_storage.value(); }

  /*! \brief The file which this truncate request has been running on.
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
    if (_offset < 0)
    {
#ifdef _WIN32
      /*! \sa Windows: [`_tell()`, `_telli64()`](https://msdn.microsoft.com/en-us/library/c3kc5e7a.aspx). */
      _offset = _telli64(_file.fd());
#else
      _offset = lseek64(_file.fd(), 0, SEEK_CUR);
#endif
    }

    auto instance_ptr = instance::from(uv_req);

    ::uv_fs_req_cleanup(static_cast< uv_t* >(uv_req));  // assuming that *uv_req has initially been nulled

    if (!instance_ptr->request_cb_storage.value())
    {
      auto &properties = instance_ptr->properties();
      properties.uv_handle = static_cast< file::uv_t* >(_file);
      properties.offset = _offset;

      return uv_status(::uv_fs_ftruncate(
          static_cast< file::uv_t* >(_file)->loop, static_cast< uv_t* >(uv_req),
          _file.fd(),
          _offset,
          nullptr
      ));
    }


    file::instance::from(_file.uv_handle)->ref();
    instance_ptr->ref();

    // instance_ptr->properties() = { static_cast< file::uv_t* >(_file), _offset };
    {
      auto &properties = instance_ptr->properties();
      properties.uv_handle = static_cast< file::uv_t* >(_file);
      properties.offset = _offset;
    }

    uv_status(0);
    int ret = ::uv_fs_ftruncate(
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
  /*!< \brief The function type of the callback called when the `sendfile` request has completed. */

protected: /*types*/
  //! \cond
  struct properties : public fs::properties
  {
    io::uv_t *uv_handle_out = nullptr;
    file::uv_t *uv_handle_in = nullptr;
    int64_t offset = 0;
    std::size_t pending_size = 0;
  };
  //! \endcond

private: /*types*/
  using instance = request::instance< sendfile >;

  struct fd
  {
    static constexpr const bool is_convertible_v = std::is_convertible< ::uv_os_fd_t, ::uv_file >::value;

    template< bool _is_convertible_ = is_convertible_v >
    static std::enable_if_t< !_is_convertible_, ::uv_file > try_convert(::uv_os_fd_t)  { return -1; }
    template< bool _is_convertible_ = is_convertible_v >
    static std::enable_if_t<  _is_convertible_, ::uv_file > try_convert(::uv_os_fd_t _os_fd)  { return _os_fd; }
  };

private: /*constructors*/
  explicit sendfile(uv_t *_uv_req)
  {
    if (_uv_req)  instance::from(_uv_req)->ref();
    uv_req = _uv_req;
  }

public: /*constructors*/
  ~sendfile() = default;
  sendfile()
  {
    uv_req = instance::create();
    init< UV_FS_SENDFILE >();
  }

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
    ::uv_file out = _out.type() == UV_FILE ? static_cast< file& >(_out).fd() : fd::try_convert(_out.fileno());
    if (out == -1)  return UV_EBADF;

    if (_offset < 0)
    {
#ifdef _WIN32
      /*! \sa Windows: [`_tell()`, `_telli64()`](https://msdn.microsoft.com/en-us/library/c3kc5e7a.aspx). */
      _offset = _telli64(_in.fd());
#else
      _offset = lseek64(_in.fd(), 0, SEEK_CUR);
#endif
    }

    auto instance_ptr = instance::from(uv_req);

    ::uv_fs_req_cleanup(static_cast< uv_t* >(uv_req));  // assuming that *uv_req has initially been nulled

    if (!instance_ptr->request_cb_storage.value())
    {
      auto &properties = instance_ptr->properties();
      properties.uv_handle_out = static_cast< io::uv_t* >(_out);
      properties.uv_handle_in = static_cast< file::uv_t* >(_in);
      properties.offset = _offset;

      return uv_status(::uv_fs_sendfile(
          static_cast< file::uv_t* >(_in)->loop, static_cast< uv_t* >(uv_req),
          out, _in.fd(),
          _offset, _length,
          nullptr
      ));
    }


    io::instance::from(_out.uv_handle)->ref();
    file::instance::from(_in.uv_handle)->ref();
    instance_ptr->ref();

    // instance_ptr->properties() = { static_cast< io::uv_t* >(_out), static_cast< file::uv_t* >(_in), _offset, _length };
    {
      auto &properties = instance_ptr->properties();
      properties.uv_handle_out = static_cast< io::uv_t* >(_out);
      properties.uv_handle_in = static_cast< file::uv_t* >(_in);
      properties.offset = _offset;
      properties.pending_size = _length;
    }

    if (_out.type() == UV_FILE)  file::instance::from(_out.uv_handle)->properties().write_queue_size += _length;

    uv_status(0);
    int ret = ::uv_fs_sendfile(
        static_cast< file::uv_t* >(_in)->loop, static_cast< uv_t* >(uv_req),
        out, _in.fd(),
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
{
  auto instance_ptr = instance::from(_uv_req);
  instance_ptr->uv_error = _uv_req->result;

  auto &properties = instance_ptr->properties();
  auto handle_out_instance_ptr = io::instance::from(properties.uv_handle_out);
  auto handle_in_instance_ptr = file::instance::from(properties.uv_handle_in);

  ref_guard< io::instance > unref_out(*handle_out_instance_ptr, adopt_ref);
  ref_guard< file::instance > unref_in(*handle_in_instance_ptr, adopt_ref);
  ref_guard< instance > unref_req(*instance_ptr, adopt_ref);

  if (handle_out_instance_ptr->uv_interface()->type(properties.uv_handle_out) == UV_FILE)
    reinterpret_cast< file::instance* >(handle_out_instance_ptr)->properties().write_queue_size -= properties.pending_size;

  auto &sendfile_cb = instance_ptr->request_cb_storage.value();
  if (sendfile_cb)  sendfile_cb(sendfile(_uv_req));
}



/*! \brief Get information about a file. */
class fs::stat : public fs
{
  //! \cond
  friend class request::instance< stat >;
  //! \endcond

public: /*types*/
  using on_request_t = std::function< void(stat _request) >;
  /*!< \brief The function type of the callback called when the `stat` request has completed. */

protected: /*types*/
  //! \cond
  struct properties : public fs::properties
  {
    file::uv_t *uv_handle = nullptr;
  };
  //! \endcond

private: /*types*/
  using instance = request::instance< stat >;

private: /*constructors*/
  explicit stat(uv_t *_uv_req)
  {
    if (_uv_req)  instance::from(_uv_req)->ref();
    uv_req = _uv_req;
  }

public: /*constructors*/
  ~stat() = default;
  stat()
  {
    uv_req = instance::create();
    init< UV_FS_STAT >();
  }

  stat(const stat&) = default;
  stat& operator =(const stat&) = default;

  stat(stat&&) noexcept = default;
  stat& operator =(stat&&) noexcept = default;

private: /*functions*/
  template< typename = void > static void stat_cb(::uv_fs_t*);

public: /*interface*/
  const on_request_t& on_request() const noexcept  { return instance::from(uv_req)->request_cb_storage.value(); }
        on_request_t& on_request()       noexcept  { return instance::from(uv_req)->request_cb_storage.value(); }

  /*! \brief The file which this stat request has been running on.
      \details It is guaranteed that it will be a valid instance at least within the request callback. */
  file handle() const noexcept
  {
    auto uv_handle = instance::from(uv_req)->properties().uv_handle;
    return uv_handle ? file(uv_handle) : file(static_cast< uv_t* >(uv_req)->loop, -1, static_cast< uv_t* >(uv_req)->path);
  }

  /*! \brief The file path affected by request.
      \sa libuv API documentation: [`uv_fs_t.path`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_t.path). */
  const char* path() const noexcept  { return static_cast< uv_t* >(uv_req)->path; }

  /*! \brief The result of the stat request.
      \sa libuv API documentation: [`uv_stat_t`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_stat_t),
                                   [`uv_timespec_t`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_timespec_t),
                                   [`uv_fs_t.statbuf`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_t.statbuf).
      \sa Linux: [`stat()`](http://man7.org/linux/man-pages/man2/stat.2.html),
                 [`fstat()`](http://man7.org/linux/man-pages/man2/fstat.2.html),
                 [`lstat()`](http://man7.org/linux/man-pages/man2/lstat.2.html). */
  const ::uv_stat_t& result() const noexcept  { return static_cast< uv_t* >(uv_req)->statbuf; }

  /*! \brief Run the request. Get status information on a file or directory specified by `_path`.
      \sa libuv API documentation: [`uv_fs_stat()`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_stat),
                                   [`uv_fs_lstat()`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_lstat).
      \sa Linux: [`stat()`](http://man7.org/linux/man-pages/man2/stat.2.html),
                 [`lstat()`](http://man7.org/linux/man-pages/man2/lstat.2.html).
      \note If the request callback is empty (has not been set), the request runs _synchronously_. */
  int run(uv::loop &_loop, const char* _path, bool _follow_symlinks = false)
  {
    auto uv_stat_function = _follow_symlinks ? ::uv_fs_stat : uv_fs_lstat;
    auto instance_ptr = instance::from(uv_req);

    ::uv_fs_req_cleanup(static_cast< uv_t* >(uv_req));  // assuming that *uv_req has initially been nulled

    instance_ptr->properties().uv_handle = nullptr;

    if (!instance_ptr->request_cb_storage.value())
    {
      return uv_status(uv_stat_function(
          static_cast< uv::loop::uv_t* >(_loop), static_cast< uv_t* >(uv_req),
          _path,
          nullptr
      ));
    }


    instance_ptr->ref();

    uv_status(0);
    int ret = uv_stat_function(
        static_cast< uv::loop::uv_t* >(_loop), static_cast< uv_t* >(uv_req),
        _path,
        stat_cb
    );
    if (!ret)  uv_status(ret);
    return ret;
  }

  /*! \brief Run the request. Get status information about the open `_file`.
      \sa libuv API documentation: [`uv_fs_fstat()`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_fstat).
      \sa Linux: [`fstat()`](http://man7.org/linux/man-pages/man2/fstat.2.html).
      \note If the request callback is empty (has not been set), the request runs _synchronously_. */
  int run(file &_file)
  {
    auto instance_ptr = instance::from(uv_req);

    ::uv_fs_req_cleanup(static_cast< uv_t* >(uv_req));  // assuming that *uv_req has initially been nulled

    if (!instance_ptr->request_cb_storage.value())
    {
      instance_ptr->properties().uv_handle = static_cast< file::uv_t* >(_file);

      return uv_status(::uv_fs_fstat(
          static_cast< file::uv_t* >(_file)->loop, static_cast< uv_t* >(uv_req),
          _file.fd(),
          nullptr
      ));
    }


    file::instance::from(_file.uv_handle)->ref();
    instance_ptr->ref();

    // instance_ptr->properties() = { static_cast< file::uv_t* >(_file) };
    {
      auto &properties = instance_ptr->properties();
      properties.uv_handle = static_cast< file::uv_t* >(_file);
    }

    uv_status(0);
    int ret = ::uv_fs_fstat(
        static_cast< file::uv_t* >(_file)->loop, static_cast< uv_t* >(uv_req),
        _file.fd(),
        stat_cb
    );
    if (!ret)  uv_status(ret);
    return ret;
  }

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_req); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_req); }
};

template< typename >
void fs::stat::stat_cb(::uv_fs_t *_uv_req)
{
  auto instance_ptr = instance::from(_uv_req);
  instance_ptr->uv_error = _uv_req->result;

  auto &properties = instance_ptr->properties();

  if (properties.uv_handle)
  {
    ref_guard< file::instance > unref_file(*file::instance::from(properties.uv_handle), adopt_ref);
    ref_guard< instance > unref_req(*instance_ptr, adopt_ref);

    auto &stat_cb = instance_ptr->request_cb_storage.value();
    if (stat_cb)  stat_cb(stat(_uv_req));
  }
  else
  {
    ref_guard< instance > unref_req(*instance_ptr, adopt_ref);

    auto &stat_cb = instance_ptr->request_cb_storage.value();
    if (stat_cb)  stat_cb(stat(_uv_req));
  }
}



/*! \brief Change a file mode bits. */
class fs::chmod : public fs
{
  //! \cond
  friend class request::instance< chmod >;
  //! \endcond

public: /*types*/
  using on_request_t = std::function< void(chmod _request) >;
  /*!< \brief The function type of the callback called when the `chmod` request has completed. */

protected: /*types*/
  //! \cond
  struct properties : public fs::properties
  {
    file::uv_t *uv_handle = nullptr;
  };
  //! \endcond

private: /*types*/
  using instance = request::instance< chmod >;

private: /*constructors*/
  explicit chmod(uv_t *_uv_req)
  {
    if (_uv_req)  instance::from(_uv_req)->ref();
    uv_req = _uv_req;
  }

public: /*constructors*/
  ~chmod() = default;
  chmod()
  {
    uv_req = instance::create();
    init< UV_FS_CHMOD >();
  }

  chmod(const chmod&) = default;
  chmod& operator =(const chmod&) = default;

  chmod(chmod&&) noexcept = default;
  chmod& operator =(chmod&&) noexcept = default;

private: /*functions*/
  template< typename = void > static void chmod_cb(::uv_fs_t*);

public: /*interface*/
  const on_request_t& on_request() const noexcept  { return instance::from(uv_req)->request_cb_storage.value(); }
        on_request_t& on_request()       noexcept  { return instance::from(uv_req)->request_cb_storage.value(); }

  /*! \brief The file which this chmod request has been running on.
      \details It is guaranteed that it will be a valid instance at least within the request callback. */
  file handle() const noexcept
  {
    auto uv_handle = instance::from(uv_req)->properties().uv_handle;
    return uv_handle ? file(uv_handle) : file(static_cast< uv_t* >(uv_req)->loop, -1, static_cast< uv_t* >(uv_req)->path);
  }

  /*! \brief The file path affected by request.
      \sa libuv API documentation: [`uv_fs_t.path`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_t.path). */
  const char* path() const noexcept  { return static_cast< uv_t* >(uv_req)->path; }

  /*! \brief Run the request. Change permissions of a file or directory specified by `_path`.
      \sa libuv API documentation: [`uv_fs_chmod()`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_chmod).
      \sa Linux: [`chmod()`](http://man7.org/linux/man-pages/man2/chmod.2.html),\n
          Windows: [`_chmod()`](https://msdn.microsoft.com/en-us/library/1z319a54.aspx).
      \note If the request callback is empty (has not been set), the request runs _synchronously_. */
  int run(uv::loop &_loop, const char* _path, int _mode)
  {
    auto instance_ptr = instance::from(uv_req);

    ::uv_fs_req_cleanup(static_cast< uv_t* >(uv_req));  // assuming that *uv_req has initially been nulled

    instance_ptr->properties().uv_handle = nullptr;

    if (!instance_ptr->request_cb_storage.value())
    {
      return uv_status(::uv_fs_chmod(
          static_cast< uv::loop::uv_t* >(_loop), static_cast< uv_t* >(uv_req),
          _path, _mode,
          nullptr
      ));
    }


    instance_ptr->ref();

    uv_status(0);
    int ret = ::uv_fs_chmod(
        static_cast< uv::loop::uv_t* >(_loop), static_cast< uv_t* >(uv_req),
        _path, _mode,
        chmod_cb
    );
    if (!ret)  uv_status(ret);
    return ret;
  }

  /*! \brief Run the request. Change permissions of the open `_file`.
      \sa libuv API documentation: [`uv_fs_fchmod()`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_fchmod).
      \sa Linux: [`fchmod()`](http://man7.org/linux/man-pages/man2/fchmod.2.html).
      \note If the request callback is empty (has not been set), the request runs _synchronously_. */
  int run(file &_file, int _mode)
  {
    auto instance_ptr = instance::from(uv_req);

    ::uv_fs_req_cleanup(static_cast< uv_t* >(uv_req));  // assuming that *uv_req has initially been nulled

    if (!instance_ptr->request_cb_storage.value())
    {
      instance_ptr->properties().uv_handle = static_cast< file::uv_t* >(_file);

      return uv_status(::uv_fs_fchmod(
          static_cast< file::uv_t* >(_file)->loop, static_cast< uv_t* >(uv_req),
          _file.fd(), _mode,
          nullptr
      ));
    }


    file::instance::from(_file.uv_handle)->ref();
    instance_ptr->ref();

    // instance_ptr->properties() = { static_cast< file::uv_t* >(_file) };
    {
      auto &properties = instance_ptr->properties();
      properties.uv_handle = static_cast< file::uv_t* >(_file);
    }

    uv_status(0);
    int ret = ::uv_fs_fchmod(
        static_cast< file::uv_t* >(_file)->loop, static_cast< uv_t* >(uv_req),
        _file.fd(), _mode,
        chmod_cb
    );
    if (!ret)  uv_status(ret);
    return ret;
  }

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_req); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_req); }
};

template< typename >
void fs::chmod::chmod_cb(::uv_fs_t *_uv_req)
{
  auto instance_ptr = instance::from(_uv_req);
  instance_ptr->uv_error = _uv_req->result;

  auto &properties = instance_ptr->properties();

  if (properties.uv_handle)
  {
    ref_guard< file::instance > unref_file(*file::instance::from(properties.uv_handle), adopt_ref);
    ref_guard< instance > unref_req(*instance_ptr, adopt_ref);

    auto &chmod_cb = instance_ptr->request_cb_storage.value();
    if (chmod_cb)  chmod_cb(chmod(_uv_req));
  }
  else
  {
    ref_guard< instance > unref_req(*instance_ptr, adopt_ref);

    auto &chmod_cb = instance_ptr->request_cb_storage.value();
    if (chmod_cb)  chmod_cb(chmod(_uv_req));
  }
}



/*! \brief Change ownership of a file. (_Not implemented on Windows._) */
class fs::chown : public fs
{
  //! \cond
  friend class request::instance< chown >;
  //! \endcond

public: /*types*/
  using on_request_t = std::function< void(chown _request) >;
  /*!< \brief The function type of the callback called when the `chown` request has completed. */

protected: /*types*/
  //! \cond
  struct properties : public fs::properties
  {
    file::uv_t *uv_handle = nullptr;
  };
  //! \endcond

private: /*types*/
  using instance = request::instance< chown >;

private: /*constructors*/
  explicit chown(uv_t *_uv_req)
  {
    if (_uv_req)  instance::from(_uv_req)->ref();
    uv_req = _uv_req;
  }

public: /*constructors*/
  ~chown() = default;
  chown()
  {
    uv_req = instance::create();
    init< UV_FS_CHOWN >();
  }

  chown(const chown&) = default;
  chown& operator =(const chown&) = default;

  chown(chown&&) noexcept = default;
  chown& operator =(chown&&) noexcept = default;

private: /*functions*/
  template< typename = void > static void chown_cb(::uv_fs_t*);

public: /*interface*/
  const on_request_t& on_request() const noexcept  { return instance::from(uv_req)->request_cb_storage.value(); }
        on_request_t& on_request()       noexcept  { return instance::from(uv_req)->request_cb_storage.value(); }

  /*! \brief The file which this chown request has been running on.
      \details It is guaranteed that it will be a valid instance at least within the request callback. */
  file handle() const noexcept
  {
    auto uv_handle = instance::from(uv_req)->properties().uv_handle;
    return uv_handle ? file(uv_handle) : file(static_cast< uv_t* >(uv_req)->loop, -1, static_cast< uv_t* >(uv_req)->path);
  }

  /*! \brief The file path affected by request.
      \sa libuv API documentation: [`uv_fs_t.path`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_t.path). */
  const char* path() const noexcept  { return static_cast< uv_t* >(uv_req)->path; }

  /*! \brief Run the request. Change the owner and group of a file or directory specified by `_path`.
      \sa libuv API documentation: [`uv_fs_chown()`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_chown).
      \sa Linux: [`chown()`](http://man7.org/linux/man-pages/man2/chown.2.html).
      \note If the request callback is empty (has not been set), the request runs _synchronously_. */
  int run(uv::loop &_loop, const char* _path, ::uv_uid_t _uid, ::uv_gid_t _gid)
  {
    auto instance_ptr = instance::from(uv_req);

    ::uv_fs_req_cleanup(static_cast< uv_t* >(uv_req));  // assuming that *uv_req has initially been nulled

    instance_ptr->properties().uv_handle = nullptr;

    if (!instance_ptr->request_cb_storage.value())
    {
      return uv_status(::uv_fs_chown(
          static_cast< uv::loop::uv_t* >(_loop), static_cast< uv_t* >(uv_req),
          _path, _uid, _gid,
          nullptr
      ));
    }


    instance_ptr->ref();

    uv_status(0);
    int ret = ::uv_fs_chown(
        static_cast< uv::loop::uv_t* >(_loop), static_cast< uv_t* >(uv_req),
        _path, _uid, _gid,
        chown_cb
    );
    if (!ret)  uv_status(ret);
    return ret;
  }

  /*! \brief Run the request. Change the owner and group of the open `_file`.
      \sa libuv API documentation: [`uv_fs_fchown()`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_fchown).
      \sa Linux: [`fchown()`](http://man7.org/linux/man-pages/man2/fchown.2.html).
      \note If the request callback is empty (has not been set), the request runs _synchronously_. */
  int run(file &_file, ::uv_uid_t _uid, ::uv_gid_t _gid)
  {
    auto instance_ptr = instance::from(uv_req);

    ::uv_fs_req_cleanup(static_cast< uv_t* >(uv_req));  // assuming that *uv_req has initially been nulled

    if (!instance_ptr->request_cb_storage.value())
    {
      instance_ptr->properties().uv_handle = static_cast< file::uv_t* >(_file);

      return uv_status(::uv_fs_fchown(
          static_cast< file::uv_t* >(_file)->loop, static_cast< uv_t* >(uv_req),
          _file.fd(), _uid, _gid,
          nullptr
      ));
    }


    file::instance::from(_file.uv_handle)->ref();
    instance_ptr->ref();

    // instance_ptr->properties() = { static_cast< file::uv_t* >(_file) };
    {
      auto &properties = instance_ptr->properties();
      properties.uv_handle = static_cast< file::uv_t* >(_file);
    }

    uv_status(0);
    int ret = ::uv_fs_fchown(
        static_cast< file::uv_t* >(_file)->loop, static_cast< uv_t* >(uv_req),
        _file.fd(), _uid, _gid,
        chown_cb
    );
    if (!ret)  uv_status(ret);
    return ret;
  }

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_req); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_req); }
};

template< typename >
void fs::chown::chown_cb(::uv_fs_t *_uv_req)
{
  auto instance_ptr = instance::from(_uv_req);
  instance_ptr->uv_error = _uv_req->result;

  auto &properties = instance_ptr->properties();

  if (properties.uv_handle)
  {
    ref_guard< file::instance > unref_file(*file::instance::from(properties.uv_handle), adopt_ref);
    ref_guard< instance > unref_req(*instance_ptr, adopt_ref);

    auto &chown_cb = instance_ptr->request_cb_storage.value();
    if (chown_cb)  chown_cb(chown(_uv_req));
  }
  else
  {
    ref_guard< instance > unref_req(*instance_ptr, adopt_ref);

    auto &chown_cb = instance_ptr->request_cb_storage.value();
    if (chown_cb)  chown_cb(chown(_uv_req));
  }
}



/*! \brief Change file timestamps. */
class fs::utime : public fs
{
  //! \cond
  friend class request::instance< utime >;
  //! \endcond

public: /*types*/
  using on_request_t = std::function< void(utime _request) >;
  /*!< \brief The function type of the callback called when the `utime` request has completed. */

protected: /*types*/
  //! \cond
  struct properties : public fs::properties
  {
    file::uv_t *uv_handle = nullptr;
  };
  //! \endcond

private: /*types*/
  using instance = request::instance< utime >;

private: /*constructors*/
  explicit utime(uv_t *_uv_req)
  {
    if (_uv_req)  instance::from(_uv_req)->ref();
    uv_req = _uv_req;
  }

public: /*constructors*/
  ~utime() = default;
  utime()
  {
    uv_req = instance::create();
    init< UV_FS_UTIME >();
  }

  utime(const utime&) = default;
  utime& operator =(const utime&) = default;

  utime(utime&&) noexcept = default;
  utime& operator =(utime&&) noexcept = default;

private: /*functions*/
  template< typename = void > static void utime_cb(::uv_fs_t*);

public: /*interface*/
  const on_request_t& on_request() const noexcept  { return instance::from(uv_req)->request_cb_storage.value(); }
        on_request_t& on_request()       noexcept  { return instance::from(uv_req)->request_cb_storage.value(); }

  /*! \brief The file which this utime request has been running on.
      \details It is guaranteed that it will be a valid instance at least within the request callback. */
  file handle() const noexcept
  {
    auto uv_handle = instance::from(uv_req)->properties().uv_handle;
    return uv_handle ? file(uv_handle) : file(static_cast< uv_t* >(uv_req)->loop, -1, static_cast< uv_t* >(uv_req)->path);
  }

  /*! \brief The file path affected by request.
      \sa libuv API documentation: [`uv_fs_t.path`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_t.path). */
  const char* path() const noexcept  { return static_cast< uv_t* >(uv_req)->path; }

  /*! \brief Run the request. Change last access and modification times for a file or directory specified by `_path`.
      \sa libuv API documentation: [`uv_fs_utime()`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_utime).
      \sa Linux: [`utime()`](http://man7.org/linux/man-pages/man2/utime.2.html),
                 [`utimensat()`](http://man7.org/linux/man-pages/man2/utimensat.2.html).
      \note If the request callback is empty (has not been set), the request runs _synchronously_. */
  int run(uv::loop &_loop, const char* _path, double _atime, double _mtime)
  {
    auto instance_ptr = instance::from(uv_req);

    ::uv_fs_req_cleanup(static_cast< uv_t* >(uv_req));  // assuming that *uv_req has initially been nulled

    instance_ptr->properties().uv_handle = nullptr;

    if (!instance_ptr->request_cb_storage.value())
    {
      return uv_status(::uv_fs_utime(
          static_cast< uv::loop::uv_t* >(_loop), static_cast< uv_t* >(uv_req),
          _path, _atime, _mtime,
          nullptr
      ));
    }


    instance_ptr->ref();

    uv_status(0);
    int ret = ::uv_fs_utime(
        static_cast< uv::loop::uv_t* >(_loop), static_cast< uv_t* >(uv_req),
        _path, _atime, _mtime,
        utime_cb
    );
    if (!ret)  uv_status(ret);
    return ret;
  }

  /*! \brief Run the request. Change last access and modification times for the open `_file`.
      \sa libuv API documentation: [`uv_fs_futime()`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_futime).
      \sa Linux: [`utimensat()`](http://man7.org/linux/man-pages/man2/utimensat.2.html).
      \note If the request callback is empty (has not been set), the request runs _synchronously_. */
  int run(file &_file, double _atime, double _mtime)
  {
    auto instance_ptr = instance::from(uv_req);

    ::uv_fs_req_cleanup(static_cast< uv_t* >(uv_req));  // assuming that *uv_req has initially been nulled

    if (!instance_ptr->request_cb_storage.value())
    {
      instance_ptr->properties().uv_handle = static_cast< file::uv_t* >(_file);

      return uv_status(::uv_fs_futime(
          static_cast< file::uv_t* >(_file)->loop, static_cast< uv_t* >(uv_req),
          _file.fd(), _atime, _mtime,
          nullptr
      ));
    }


    file::instance::from(_file.uv_handle)->ref();
    instance_ptr->ref();

    // instance_ptr->properties() = { static_cast< file::uv_t* >(_file) };
    {
      auto &properties = instance_ptr->properties();
      properties.uv_handle = static_cast< file::uv_t* >(_file);
    }

    uv_status(0);
    int ret = ::uv_fs_futime(
        static_cast< file::uv_t* >(_file)->loop, static_cast< uv_t* >(uv_req),
        _file.fd(), _atime, _mtime,
        utime_cb
    );
    if (!ret)  uv_status(ret);
    return ret;
  }

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_req); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_req); }
};

template< typename >
void fs::utime::utime_cb(::uv_fs_t *_uv_req)
{
  auto instance_ptr = instance::from(_uv_req);
  instance_ptr->uv_error = _uv_req->result;

  auto &properties = instance_ptr->properties();

  if (properties.uv_handle)
  {
    ref_guard< file::instance > unref_file(*file::instance::from(properties.uv_handle), adopt_ref);
    ref_guard< instance > unref_req(*instance_ptr, adopt_ref);

    auto &utime_cb = instance_ptr->request_cb_storage.value();
    if (utime_cb)  utime_cb(utime(_uv_req));
  }
  else
  {
    ref_guard< instance > unref_req(*instance_ptr, adopt_ref);

    auto &utime_cb = instance_ptr->request_cb_storage.value();
    if (utime_cb)  utime_cb(utime(_uv_req));
  }
}



/*! \brief Delete a file name and possibly the file itself that the name refers to. */
class fs::unlink : public fs
{
  //! \cond
  friend class request::instance< unlink >;
  //! \endcond

public: /*types*/
  using on_request_t = std::function< void(unlink _request) >;
  /*!< \brief The function type of the callback called when the `unlink` request has completed. */

private: /*types*/
  using instance = request::instance< unlink >;

private: /*constructors*/
  explicit unlink(uv_t *_uv_req)
  {
    if (_uv_req)  instance::from(_uv_req)->ref();
    uv_req = _uv_req;
  }

public: /*constructors*/
  ~unlink() = default;
  unlink()
  {
    uv_req = instance::create();
    init< UV_FS_UNLINK >();
  }

  unlink(const unlink&) = default;
  unlink& operator =(const unlink&) = default;

  unlink(unlink&&) noexcept = default;
  unlink& operator =(unlink&&) noexcept = default;

private: /*functions*/
  template< typename = void > static void unlink_cb(::uv_fs_t*);

public: /*interface*/
  const on_request_t& on_request() const noexcept  { return instance::from(uv_req)->request_cb_storage.value(); }
        on_request_t& on_request()       noexcept  { return instance::from(uv_req)->request_cb_storage.value(); }

  /*! \brief The file path affected by request.
      \sa libuv API documentation: [`uv_fs_t.path`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_t.path). */
  const char* path() const noexcept  { return static_cast< uv_t* >(uv_req)->path; }

  /*! \brief Run the request. Delete a file name specified by `_path`.
      \sa libuv API documentation: [`uv_fs_unlink()`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_unlink).
      \sa Linux: [`unlink()`](http://man7.org/linux/man-pages/man2/unlink.2.html).
      \note If the request callback is empty (has not been set), the request runs _synchronously_. */
  int run(uv::loop &_loop, const char* _path)
  {
    auto instance_ptr = instance::from(uv_req);

    ::uv_fs_req_cleanup(static_cast< uv_t* >(uv_req));  // assuming that *uv_req has initially been nulled

    if (!instance_ptr->request_cb_storage.value())
    {
      return uv_status(::uv_fs_unlink(
          static_cast< uv::loop::uv_t* >(_loop), static_cast< uv_t* >(uv_req),
          _path,
          nullptr
      ));
    }


    instance_ptr->ref();

    uv_status(0);
    int ret = ::uv_fs_unlink(
        static_cast< uv::loop::uv_t* >(_loop), static_cast< uv_t* >(uv_req),
        _path,
        unlink_cb
    );
    if (!ret)  uv_status(ret);
    return ret;
  }

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_req); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_req); }
};

template< typename >
void fs::unlink::unlink_cb(::uv_fs_t *_uv_req)
{
  auto instance_ptr = instance::from(_uv_req);
  instance_ptr->uv_error = _uv_req->result;

  ref_guard< instance > unref_req(*instance_ptr, adopt_ref);

  auto &unlink_cb = instance_ptr->request_cb_storage.value();
  if (unlink_cb)  unlink_cb(unlink(_uv_req));
}



/*! \brief Create a directory. */
class fs::mkdir : public fs
{
  //! \cond
  friend class request::instance< mkdir >;
  //! \endcond

public: /*types*/
  using on_request_t = std::function< void(mkdir _request) >;
  /*!< \brief The function type of the callback called when the `mkdir` request has completed. */

private: /*types*/
  using instance = request::instance< mkdir >;

private: /*constructors*/
  explicit mkdir(uv_t *_uv_req)
  {
    if (_uv_req)  instance::from(_uv_req)->ref();
    uv_req = _uv_req;
  }

public: /*constructors*/
  ~mkdir() = default;
  mkdir()
  {
    uv_req = instance::create();
    init< UV_FS_MKDIR >();
  }

  mkdir(const mkdir&) = default;
  mkdir& operator =(const mkdir&) = default;

  mkdir(mkdir&&) noexcept = default;
  mkdir& operator =(mkdir&&) noexcept = default;

private: /*functions*/
  template< typename = void > static void mkdir_cb(::uv_fs_t*);

public: /*interface*/
  const on_request_t& on_request() const noexcept  { return instance::from(uv_req)->request_cb_storage.value(); }
        on_request_t& on_request()       noexcept  { return instance::from(uv_req)->request_cb_storage.value(); }

  /*! \brief The file path affected by request.
      \sa libuv API documentation: [`uv_fs_t.path`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_t.path). */
  const char* path() const noexcept  { return static_cast< uv_t* >(uv_req)->path; }

  /*! \brief Run the request. Create a directory specified by `_path`.
      \sa libuv API documentation: [`uv_fs_mkdir()`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_mkdir).
      \sa Linux: [`mkdir()`](http://man7.org/linux/man-pages/man2/mkdir.2.html).
      \note If the request callback is empty (has not been set), the request runs _synchronously_. */
  int run(uv::loop &_loop, const char* _path, int _mode)
  {
    auto instance_ptr = instance::from(uv_req);

    ::uv_fs_req_cleanup(static_cast< uv_t* >(uv_req));  // assuming that *uv_req has initially been nulled

    if (!instance_ptr->request_cb_storage.value())
    {
      return uv_status(::uv_fs_mkdir(
          static_cast< uv::loop::uv_t* >(_loop), static_cast< uv_t* >(uv_req),
          _path, _mode,
          nullptr
      ));
    }


    instance_ptr->ref();

    uv_status(0);
    int ret = ::uv_fs_mkdir(
        static_cast< uv::loop::uv_t* >(_loop), static_cast< uv_t* >(uv_req),
        _path, _mode,
        mkdir_cb
    );
    if (!ret)  uv_status(ret);
    return ret;
  }

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_req); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_req); }
};

template< typename >
void fs::mkdir::mkdir_cb(::uv_fs_t *_uv_req)
{
  auto instance_ptr = instance::from(_uv_req);
  instance_ptr->uv_error = _uv_req->result;

  ref_guard< instance > unref_req(*instance_ptr, adopt_ref);

  auto &mkdir_cb = instance_ptr->request_cb_storage.value();
  if (mkdir_cb)  mkdir_cb(mkdir(_uv_req));
}



/*! \brief Create a uniquely named temporary directory. */
class fs::mkdtemp : public fs
{
  //! \cond
  friend class request::instance< mkdtemp >;
  //! \endcond

public: /*types*/
  using on_request_t = std::function< void(mkdtemp _request) >;
  /*!< \brief The function type of the callback called when the `mkdtemp` request has completed. */

private: /*types*/
  using instance = request::instance< mkdtemp >;

private: /*constructors*/
  explicit mkdtemp(uv_t *_uv_req)
  {
    if (_uv_req)  instance::from(_uv_req)->ref();
    uv_req = _uv_req;
  }

public: /*constructors*/
  ~mkdtemp() = default;
  mkdtemp()
  {
    uv_req = instance::create();
    init< UV_FS_MKDTEMP >();
  }

  mkdtemp(const mkdtemp&) = default;
  mkdtemp& operator =(const mkdtemp&) = default;

  mkdtemp(mkdtemp&&) noexcept = default;
  mkdtemp& operator =(mkdtemp&&) noexcept = default;

private: /*functions*/
  template< typename = void > static void mkdtemp_cb(::uv_fs_t*);

public: /*interface*/
  const on_request_t& on_request() const noexcept  { return instance::from(uv_req)->request_cb_storage.value(); }
        on_request_t& on_request()       noexcept  { return instance::from(uv_req)->request_cb_storage.value(); }

  /*! \brief The path of the directory created by request.
      \sa libuv API documentation: [`uv_fs_t.path`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_t.path). */
  const char* path() const noexcept  { return static_cast< uv_t* >(uv_req)->path; }

  /*! \brief Run the request. Create a temporary directory with unique name generated from `_template` string.
      \sa libuv API documentation: [`uv_fs_mkdtemp()`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_mkdtemp).
      \sa Linux: [`mkdtemp()`](http://man7.org/linux/man-pages/man3/mkdtemp.3.html).
      \note If the request callback is empty (has not been set), the request runs _synchronously_. */
  int run(uv::loop &_loop, const char* _template)
  {
    auto instance_ptr = instance::from(uv_req);

    ::uv_fs_req_cleanup(static_cast< uv_t* >(uv_req));  // assuming that *uv_req has initially been nulled

    if (!instance_ptr->request_cb_storage.value())
    {
      return uv_status(::uv_fs_mkdtemp(
          static_cast< uv::loop::uv_t* >(_loop), static_cast< uv_t* >(uv_req),
          _template,
          nullptr
      ));
    }


    instance_ptr->ref();

    uv_status(0);
    int ret = ::uv_fs_mkdtemp(
        static_cast< uv::loop::uv_t* >(_loop), static_cast< uv_t* >(uv_req),
        _template,
        mkdtemp_cb
    );
    if (!ret)  uv_status(ret);
    return ret;
  }

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_req); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_req); }
};

template< typename >
void fs::mkdtemp::mkdtemp_cb(::uv_fs_t *_uv_req)
{
  auto instance_ptr = instance::from(_uv_req);
  instance_ptr->uv_error = _uv_req->result;

  ref_guard< instance > unref_req(*instance_ptr, adopt_ref);

  auto &mkdtemp_cb = instance_ptr->request_cb_storage.value();
  if (mkdtemp_cb)  mkdtemp_cb(mkdtemp(_uv_req));
}



/*! \brief Delete a directory. */
class fs::rmdir : public fs
{
  //! \cond
  friend class request::instance< rmdir >;
  //! \endcond

public: /*types*/
  using on_request_t = std::function< void(rmdir _request) >;
  /*!< \brief The function type of the callback called when the `rmdir` request has completed. */

private: /*types*/
  using instance = request::instance< rmdir >;

private: /*constructors*/
  explicit rmdir(uv_t *_uv_req)
  {
    if (_uv_req)  instance::from(_uv_req)->ref();
    uv_req = _uv_req;
  }

public: /*constructors*/
  ~rmdir() = default;
  rmdir()
  {
    uv_req = instance::create();
    init< UV_FS_RMDIR >();
  }

  rmdir(const rmdir&) = default;
  rmdir& operator =(const rmdir&) = default;

  rmdir(rmdir&&) noexcept = default;
  rmdir& operator =(rmdir&&) noexcept = default;

private: /*functions*/
  template< typename = void > static void rmdir_cb(::uv_fs_t*);

public: /*interface*/
  const on_request_t& on_request() const noexcept  { return instance::from(uv_req)->request_cb_storage.value(); }
        on_request_t& on_request()       noexcept  { return instance::from(uv_req)->request_cb_storage.value(); }

  /*! \brief The file path affected by request.
      \sa libuv API documentation: [`uv_fs_t.path`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_t.path). */
  const char* path() const noexcept  { return static_cast< uv_t* >(uv_req)->path; }

  /*! \brief Run the request. Delete a directory (which must be empty) specified by `_path`.
      \sa libuv API documentation: [`uv_fs_rmdir()`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_rmdir).
      \sa Linux: [`rmdir()`](http://man7.org/linux/man-pages/man2/rmdir.2.html).
      \note If the request callback is empty (has not been set), the request runs _synchronously_. */
  int run(uv::loop &_loop, const char* _path)
  {
    auto instance_ptr = instance::from(uv_req);

    ::uv_fs_req_cleanup(static_cast< uv_t* >(uv_req));  // assuming that *uv_req has initially been nulled

    if (!instance_ptr->request_cb_storage.value())
    {
      return uv_status(::uv_fs_rmdir(
          static_cast< uv::loop::uv_t* >(_loop), static_cast< uv_t* >(uv_req),
          _path,
          nullptr
      ));
    }


    instance_ptr->ref();

    uv_status(0);
    int ret = ::uv_fs_rmdir(
        static_cast< uv::loop::uv_t* >(_loop), static_cast< uv_t* >(uv_req),
        _path,
        rmdir_cb
    );
    if (!ret)  uv_status(ret);
    return ret;
  }

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_req); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_req); }
};

template< typename >
void fs::rmdir::rmdir_cb(::uv_fs_t *_uv_req)
{
  auto instance_ptr = instance::from(_uv_req);
  instance_ptr->uv_error = _uv_req->result;

  ref_guard< instance > unref_req(*instance_ptr, adopt_ref);

  auto &rmdir_cb = instance_ptr->request_cb_storage.value();
  if (rmdir_cb)  rmdir_cb(rmdir(_uv_req));
}



/*! \brief Scan a directory. */
class fs::scandir : public fs
{
  //! \cond
  friend class request::instance< scandir >;
  //! \endcond

public: /*types*/
  using on_request_t = std::function< void(scandir _request) >;
  /*!< \brief The function type of the callback called when the `scandir` request has completed. */

private: /*types*/
  using instance = request::instance< scandir >;

private: /*constructors*/
  explicit scandir(uv_t *_uv_req)
  {
    if (_uv_req)  instance::from(_uv_req)->ref();
    uv_req = _uv_req;
  }

public: /*constructors*/
  ~scandir() = default;
  scandir()
  {
    uv_req = instance::create();
    init< UV_FS_SCANDIR >();
  }

  scandir(const scandir&) = default;
  scandir& operator =(const scandir&) = default;

  scandir(scandir&&) noexcept = default;
  scandir& operator =(scandir&&) noexcept = default;

private: /*functions*/
  template< typename = void > static void scandir_cb(::uv_fs_t*);

public: /*interface*/
  const on_request_t& on_request() const noexcept  { return instance::from(uv_req)->request_cb_storage.value(); }
        on_request_t& on_request()       noexcept  { return instance::from(uv_req)->request_cb_storage.value(); }

  /*! \brief The file path affected by request.
      \sa libuv API documentation: [`uv_fs_t.path`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_t.path). */
  const char* path() const noexcept  { return static_cast< uv_t* >(uv_req)->path; }

  /*! \brief Get the results of the scandir request.
      \details It is simply a wrapper around `uv_fs_scandir_next()` libuv API function.
      \sa libuv API documentation: [`uv_dirent_t`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_dirent_t),
                                   [`uv_fs_scandir()`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_scandir),
                                   [`uv_fs_scandir_next()`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_scandir_next). */
  int scandir_next(::uv_dirent_t &_entry) const noexcept
  { return uv_status(uv_fs_scandir_next(static_cast< uv_t* >(uv_req), &_entry)); }

  /*! \brief Run the request. Scan a directory specified by `_path`.
      \sa libuv API documentation: [`uv_fs_scandir()`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_scandir),
                                   [`uv_fs_scandir_next()`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_scandir_next).
      \sa Linux: [`scandir()`](http://man7.org/linux/man-pages/man3/scandir.3.html).
      \note If the request callback is empty (has not been set), the request runs _synchronously_.
      In this case the function returns the number of directory entries or relevant libuv error code. */
  int run(uv::loop &_loop, const char* _path)
  {
    auto instance_ptr = instance::from(uv_req);

    ::uv_fs_req_cleanup(static_cast< uv_t* >(uv_req));  // assuming that *uv_req has initially been nulled

    if (!instance_ptr->request_cb_storage.value())
    {
      return uv_status(::uv_fs_scandir(
          static_cast< uv::loop::uv_t* >(_loop), static_cast< uv_t* >(uv_req),
          _path, 0,
          nullptr
      ));
    }


    instance_ptr->ref();

    uv_status(0);
    int ret = ::uv_fs_scandir(
        static_cast< uv::loop::uv_t* >(_loop), static_cast< uv_t* >(uv_req),
        _path, 0,
        scandir_cb
    );
    if (!ret)  uv_status(ret);
    return ret;
  }

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_req); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_req); }
};

template< typename >
void fs::scandir::scandir_cb(::uv_fs_t *_uv_req)
{
  auto instance_ptr = instance::from(_uv_req);
  instance_ptr->uv_error = _uv_req->result;

  ref_guard< instance > unref_req(*instance_ptr, adopt_ref);

  auto &scandir_cb = instance_ptr->request_cb_storage.value();
  if (scandir_cb)  scandir_cb(scandir(_uv_req));
}



/*! \brief Change the name or location of a file. */
class fs::rename : public fs
{
  //! \cond
  friend class request::instance< rename >;
  //! \endcond

public: /*types*/
  using on_request_t = std::function< void(rename _request) >;
  /*!< \brief The function type of the callback called when the `rename` request has completed. */

protected: /*types*/
  //! \cond
  struct properties : public fs::properties
  {
#ifdef _WIN32
    std::string new_path;
#endif
  };
  //! \endcond

private: /*types*/
  using instance = request::instance< rename >;

private: /*constructors*/
  explicit rename(uv_t *_uv_req)
  {
    if (_uv_req)  instance::from(_uv_req)->ref();
    uv_req = _uv_req;
  }

public: /*constructors*/
  ~rename() = default;
  rename()
  {
    uv_req = instance::create();
    init< UV_FS_RENAME >();
  }

  rename(const rename&) = default;
  rename& operator =(const rename&) = default;

  rename(rename&&) noexcept = default;
  rename& operator =(rename&&) noexcept = default;

private: /*functions*/
  template< typename = void > static void rename_cb(::uv_fs_t*);

public: /*interface*/
  const on_request_t& on_request() const noexcept  { return instance::from(uv_req)->request_cb_storage.value(); }
        on_request_t& on_request()       noexcept  { return instance::from(uv_req)->request_cb_storage.value(); }

  /*! \brief The file path affected by request.
      \sa libuv API documentation: [`uv_fs_t.path`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_t.path). */
  const char* path() const noexcept  { return static_cast< uv_t* >(uv_req)->path; }
  /*! \brief The file path affected by request. */
  const char* new_path() const noexcept
  {
#ifdef _WIN32
    return instance::from(uv_req)->properties().new_path.c_str();
#else
    return static_cast< uv_t* >(uv_req)->new_path;
#endif
  }

  /*! \brief Run the request. Renames a file, moving it between directories if required.
      \sa libuv API documentation: [`uv_fs_rename()`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_rename).
      \sa Linux: [`rename()`](http://man7.org/linux/man-pages/man2/rename.2.html).
      \note If the request callback is empty (has not been set), the request runs _synchronously_. */
  int run(uv::loop &_loop, const char* _path, const char* _new_path)
  {
    auto instance_ptr = instance::from(uv_req);

    ::uv_fs_req_cleanup(static_cast< uv_t* >(uv_req));  // assuming that *uv_req has initially been nulled

#ifdef _WIN32
    instance_ptr->properties().new_path.assign(_new_path);
#endif

    if (!instance_ptr->request_cb_storage.value())
    {
      return uv_status(::uv_fs_rename(
          static_cast< uv::loop::uv_t* >(_loop), static_cast< uv_t* >(uv_req),
          _path, _new_path,
          nullptr
      ));
    }


    instance_ptr->ref();

    uv_status(0);
    int ret = ::uv_fs_rename(
        static_cast< uv::loop::uv_t* >(_loop), static_cast< uv_t* >(uv_req),
        _path, _new_path,
        rename_cb
    );
    if (!ret)  uv_status(ret);
    return ret;
  }

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_req); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_req); }
};

template< typename >
void fs::rename::rename_cb(::uv_fs_t *_uv_req)
{
  auto instance_ptr = instance::from(_uv_req);
  instance_ptr->uv_error = _uv_req->result;

  ref_guard< instance > unref_req(*instance_ptr, adopt_ref);

  auto &rename_cb = instance_ptr->request_cb_storage.value();
  if (rename_cb)  rename_cb(rename(_uv_req));
}



/*! \brief Check user's permissions for a file. */
class fs::access : public fs
{
  //! \cond
  friend class request::instance< access >;
  //! \endcond

public: /*types*/
  using on_request_t = std::function< void(access _request) >;
  /*!< \brief The function type of the callback called when the `access` request has completed. */

private: /*types*/
  using instance = request::instance< access >;

private: /*constructors*/
  explicit access(uv_t *_uv_req)
  {
    if (_uv_req)  instance::from(_uv_req)->ref();
    uv_req = _uv_req;
  }

public: /*constructors*/
  ~access() = default;
  access()
  {
    uv_req = instance::create();
    init< UV_FS_ACCESS >();
  }

  access(const access&) = default;
  access& operator =(const access&) = default;

  access(access&&) noexcept = default;
  access& operator =(access&&) noexcept = default;

private: /*functions*/
  template< typename = void > static void access_cb(::uv_fs_t*);

public: /*interface*/
  const on_request_t& on_request() const noexcept  { return instance::from(uv_req)->request_cb_storage.value(); }
        on_request_t& on_request()       noexcept  { return instance::from(uv_req)->request_cb_storage.value(); }

  /*! \brief The file path affected by request.
      \sa libuv API documentation: [`uv_fs_t.path`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_t.path). */
  const char* path() const noexcept  { return static_cast< uv_t* >(uv_req)->path; }

  /*! \brief Run the request. Check if the calling process can access the file specified by `_path` using the access `_mode`.
      \sa libuv API documentation: [`uv_fs_access()`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_access).
      \sa Linux: [`access()`](http://man7.org/linux/man-pages/man2/access.2.html).
      \note If the request callback is empty (has not been set), the request runs _synchronously_. */
  int run(uv::loop &_loop, const char* _path, int _mode)
  {
    auto instance_ptr = instance::from(uv_req);

    ::uv_fs_req_cleanup(static_cast< uv_t* >(uv_req));  // assuming that *uv_req has initially been nulled

    if (!instance_ptr->request_cb_storage.value())
    {
      return uv_status(::uv_fs_access(
          static_cast< uv::loop::uv_t* >(_loop), static_cast< uv_t* >(uv_req),
          _path, _mode,
          nullptr
      ));
    }


    instance_ptr->ref();

    uv_status(0);
    int ret = ::uv_fs_access(
        static_cast< uv::loop::uv_t* >(_loop), static_cast< uv_t* >(uv_req),
        _path, _mode,
        access_cb
    );
    if (!ret)  uv_status(ret);
    return ret;
  }

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_req); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_req); }
};

template< typename >
void fs::access::access_cb(::uv_fs_t *_uv_req)
{
  auto instance_ptr = instance::from(_uv_req);
  instance_ptr->uv_error = _uv_req->result;

  ref_guard< instance > unref_req(*instance_ptr, adopt_ref);

  auto &access_cb = instance_ptr->request_cb_storage.value();
  if (access_cb)  access_cb(access(_uv_req));
}



/*! \brief Create a new link to a file. */
class fs::link : public fs
{
  //! \cond
  friend class request::instance< link >;
  //! \endcond

public: /*types*/
  using on_request_t = std::function< void(link _request) >;
  /*!< \brief The function type of the callback called when the `link` request has completed. */

protected: /*types*/
  //! \cond
  struct properties : public fs::properties
  {
#ifdef _WIN32
    std::string link_path;
#endif
  };
  //! \endcond

private: /*types*/
  using instance = request::instance< link >;

private: /*constructors*/
  explicit link(uv_t *_uv_req)
  {
    if (_uv_req)  instance::from(_uv_req)->ref();
    uv_req = _uv_req;
  }

public: /*constructors*/
  ~link() = default;
  link()
  {
    uv_req = instance::create();
    init< UV_FS_LINK >();
  }

  link(const link&) = default;
  link& operator =(const link&) = default;

  link(link&&) noexcept = default;
  link& operator =(link&&) noexcept = default;

private: /*functions*/
  template< typename = void > static void link_cb(::uv_fs_t*);

public: /*interface*/
  const on_request_t& on_request() const noexcept  { return instance::from(uv_req)->request_cb_storage.value(); }
        on_request_t& on_request()       noexcept  { return instance::from(uv_req)->request_cb_storage.value(); }

  /*! \brief The file path affected by request.
      \sa libuv API documentation: [`uv_fs_t.path`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_t.path). */
  const char* path() const noexcept  { return static_cast< uv_t* >(uv_req)->path; }
  /*! \brief The new name for the path affected by request. */
  const char* link_path() const noexcept
  {
#ifdef _WIN32
    return instance::from(uv_req)->properties().link_path.c_str();
#else
    return static_cast< uv_t* >(uv_req)->new_path;
#endif
  }

  /*! \brief Run the request. Make a new name for a target specified by `_path`.
      \details
      \arg If `(_symbolic_link == false)`, create a new hard link to an existing `_path`.
      \arg If `(_symbolic_link == true)`, create a symbolic link which contains the string from `_path`
      as a link target.

      On _Windows_ the `_symlink_flags` parameter can be specified to control how the _link to a directory_
      will be created:
      \arg `UV_FS_SYMLINK_DIR` - request to create a symbolic link;
      \arg `UV_FS_SYMLINK_JUNCTION` - request to create a junction point.

      \sa libuv API documentation: [`uv_fs_link()`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_link),
                                   [`uv_fs_symlink()`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_symlink).
      \sa Linux: [`link()`](http://man7.org/linux/man-pages/man2/link.2.html),
                 [`symlink()`](http://man7.org/linux/man-pages/man2/symlink.2.html).
      \note If the request callback is empty (has not been set), the request runs _synchronously_. */
  int run(uv::loop &_loop, const char* _path, const char* _link_path, bool _symbolic_link, int _symlink_flags = 0)
  {
    auto instance_ptr = instance::from(uv_req);

    ::uv_fs_req_cleanup(static_cast< uv_t* >(uv_req));  // assuming that *uv_req has initially been nulled

#ifdef _WIN32
    instance_ptr->properties().link_path.assign(_link_path);
#endif

    if (!instance_ptr->request_cb_storage.value())
    {
      if (_symbolic_link)  return uv_status(::uv_fs_symlink(
          static_cast< uv::loop::uv_t* >(_loop), static_cast< uv_t* >(uv_req),
          _path, _link_path, _symlink_flags,
          nullptr
      ));
      else  return uv_status(::uv_fs_link(
          static_cast< uv::loop::uv_t* >(_loop), static_cast< uv_t* >(uv_req),
          _path, _link_path,
          nullptr
      ));
    }


    instance_ptr->ref();

    uv_status(0);
    int ret = _symbolic_link
      ? ::uv_fs_symlink(
            static_cast< uv::loop::uv_t* >(_loop), static_cast< uv_t* >(uv_req),
            _path, _link_path, _symlink_flags,
            link_cb
        )
      : ::uv_fs_link(
            static_cast< uv::loop::uv_t* >(_loop), static_cast< uv_t* >(uv_req),
            _path, _link_path,
            link_cb
        );
    if (!ret)  uv_status(ret);
    return ret;
  }

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_req); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_req); }
};

template< typename >
void fs::link::link_cb(::uv_fs_t *_uv_req)
{
  auto instance_ptr = instance::from(_uv_req);
  instance_ptr->uv_error = _uv_req->result;

  ref_guard< instance > unref_req(*instance_ptr, adopt_ref);

  auto &link_cb = instance_ptr->request_cb_storage.value();
  if (link_cb)  link_cb(link(_uv_req));
}



/*! \brief Read value of a symbolic link. */
class fs::readlink : public fs
{
  //! \cond
  friend class request::instance< readlink >;
  //! \endcond

public: /*types*/
  using on_request_t = std::function< void(readlink _request) >;
  /*!< \brief The function type of the callback called when the `readlink` request has completed. */

private: /*types*/
  using instance = request::instance< readlink >;

private: /*constructors*/
  explicit readlink(uv_t *_uv_req)
  {
    if (_uv_req)  instance::from(_uv_req)->ref();
    uv_req = _uv_req;
  }

public: /*constructors*/
  ~readlink() = default;
  readlink()
  {
    uv_req = instance::create();
    init< UV_FS_READLINK >();
  }

  readlink(const readlink&) = default;
  readlink& operator =(const readlink&) = default;

  readlink(readlink&&) noexcept = default;
  readlink& operator =(readlink&&) noexcept = default;

private: /*functions*/
  template< typename = void > static void readlink_cb(::uv_fs_t*);

public: /*interface*/
  const on_request_t& on_request() const noexcept  { return instance::from(uv_req)->request_cb_storage.value(); }
        on_request_t& on_request()       noexcept  { return instance::from(uv_req)->request_cb_storage.value(); }

  /*! \brief The file path affected by request.
      \sa libuv API documentation: [`uv_fs_t.path`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_t.path). */
  const char* path() const noexcept  { return static_cast< uv_t* >(uv_req)->path; }

  /*! \brief The result of the readlink request. */
  const char* result() const noexcept  { return static_cast< const char* >(static_cast< uv_t* >(uv_req)->ptr); }

  /*! \brief Run the request. Read value of a symbolic link specified by `_path`.
      \sa libuv API documentation: [`uv_fs_readlink()`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_readlink).
      \sa Linux: [`readlink()`](http://man7.org/linux/man-pages/man2/readlink.2.html).
      \note If the request callback is empty (has not been set), the request runs _synchronously_. */
  int run(uv::loop &_loop, const char* _path)
  {
    auto instance_ptr = instance::from(uv_req);

    ::uv_fs_req_cleanup(static_cast< uv_t* >(uv_req));  // assuming that *uv_req has initially been nulled

    if (!instance_ptr->request_cb_storage.value())
    {
      return uv_status(::uv_fs_readlink(
          static_cast< uv::loop::uv_t* >(_loop), static_cast< uv_t* >(uv_req),
          _path,
          nullptr
      ));
    }


    instance_ptr->ref();

    uv_status(0);
    int ret = ::uv_fs_readlink(
        static_cast< uv::loop::uv_t* >(_loop), static_cast< uv_t* >(uv_req),
        _path,
        readlink_cb
    );
    if (!ret)  uv_status(ret);
    return ret;
  }

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_req); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_req); }
};

template< typename >
void fs::readlink::readlink_cb(::uv_fs_t *_uv_req)
{
  auto instance_ptr = instance::from(_uv_req);
  instance_ptr->uv_error = _uv_req->result;

  ref_guard< instance > unref_req(*instance_ptr, adopt_ref);

  auto &readlink_cb = instance_ptr->request_cb_storage.value();
  if (readlink_cb)  readlink_cb(readlink(_uv_req));
}



/*! \brief Get canonicalized absolute pathname. */
class fs::realpath : public fs
{
  //! \cond
  friend class request::instance< realpath >;
  //! \endcond

public: /*types*/
  using on_request_t = std::function< void(realpath _request) >;
  /*!< \brief The function type of the callback called when the `realpath` request has completed. */

private: /*types*/
  using instance = request::instance< realpath >;

private: /*constructors*/
  explicit realpath(uv_t *_uv_req)
  {
    if (_uv_req)  instance::from(_uv_req)->ref();
    uv_req = _uv_req;
  }

public: /*constructors*/
  ~realpath() = default;
  realpath()
  {
    uv_req = instance::create();
    init< UV_FS_REALPATH >();
  }

  realpath(const realpath&) = default;
  realpath& operator =(const realpath&) = default;

  realpath(realpath&&) noexcept = default;
  realpath& operator =(realpath&&) noexcept = default;

private: /*functions*/
  template< typename = void > static void realpath_cb(::uv_fs_t*);

public: /*interface*/
  const on_request_t& on_request() const noexcept  { return instance::from(uv_req)->request_cb_storage.value(); }
        on_request_t& on_request()       noexcept  { return instance::from(uv_req)->request_cb_storage.value(); }

  /*! \brief The file path affected by request.
      \sa libuv API documentation: [`uv_fs_t.path`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_t.path). */
  const char* path() const noexcept  { return static_cast< uv_t* >(uv_req)->path; }

  /*! \brief The result of the realpath request. */
  const char* result() const noexcept  { return static_cast< const char* >(static_cast< uv_t* >(uv_req)->ptr); }

  /*! \brief Run the request. Get canonicalized absolute pathname.
      \sa libuv API documentation: [`uv_fs_realpath()`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_realpath).
      \sa Linux: [`realpath()`](http://man7.org/linux/man-pages/man3/realpath.3.html).
      \note If the request callback is empty (has not been set), the request runs _synchronously_. */
  int run(uv::loop &_loop, const char* _path)
  {
    auto instance_ptr = instance::from(uv_req);

    ::uv_fs_req_cleanup(static_cast< uv_t* >(uv_req));  // assuming that *uv_req has initially been nulled

    if (!instance_ptr->request_cb_storage.value())
    {
      return uv_status(::uv_fs_realpath(
          static_cast< uv::loop::uv_t* >(_loop), static_cast< uv_t* >(uv_req),
          _path,
          nullptr
      ));
    }


    instance_ptr->ref();

    uv_status(0);
    int ret = ::uv_fs_realpath(
        static_cast< uv::loop::uv_t* >(_loop), static_cast< uv_t* >(uv_req),
        _path,
        realpath_cb
    );
    if (!ret)  uv_status(ret);
    return ret;
  }

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_req); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_req); }
};

template< typename >
void fs::realpath::realpath_cb(::uv_fs_t *_uv_req)
{
  auto instance_ptr = instance::from(_uv_req);
  instance_ptr->uv_error = _uv_req->result;

  ref_guard< instance > unref_req(*instance_ptr, adopt_ref);

  auto &realpath_cb = instance_ptr->request_cb_storage.value();
  if (realpath_cb)  realpath_cb(realpath(_uv_req));
}


}


#endif
