
#ifndef UVCC_REQUEST_FS__HPP
#define UVCC_REQUEST_FS__HPP

#include "uvcc/utility.hpp"
#include "uvcc/request-base.hpp"
#include "uvcc/handle-fs.hpp"
#include "uvcc/buffer.hpp"

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
    \brief ...
    \sa libuv API documentation: [Filesystem operations](http://docs.libuv.org/en/v1.x/fs.html#filesystem-operations). */
class fs : public request
{
  //! \cond
  friend class request::instance< fs >;
  //! \endcond

public: /*types*/
  using uv_t = ::uv_fs_t;
  using on_request_t = std::function< void(int) >;

private: /*types*/
  using instance = request::instance< fs >;

public: /*constructors*/
  ~fs() = default;

  fs(const fs&) = default;
  fs& operator =(const fs&) = default;

  fs(fs&&) noexcept = default;
  fs& operator =(fs&&) noexcept = default;

public: /*interface*/
  const on_request_t& on_request() const noexcept  { return instance::from(uv_req)->on_request(); }
        on_request_t& on_request()       noexcept  { return instance::from(uv_req)->on_request(); }

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_req); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_req); }
};



/*! \ingroup doxy_request
    \brief The request type for storing a file handle and performing operations on it in synchronous mode. */
class file : public request
{
  //! \cond
  friend class request::instance< file >;
  //! \endcond

public: /*types*/
  using uv_t = ::uv_fs_t;
  using on_request_t = empty_t;

protected: /*types*/
  //! \cond
  struct supplemental_data_t
  {
    uv_t *uv_req = nullptr;
    uv_t *req_fstat = nullptr;
    ~supplemental_data_t()
    {
      if (req_fstat)  ::uv_fs_req_cleanup(req_fstat);
      if (uv_req)
      {
        if (uv_req->result >= 0)  // assuming that uv_req->result has been initialized with value < 0
        {
          uv_t req_close;
          ::uv_fs_close(nullptr, &req_close, uv_req->result, nullptr);
          ::uv_fs_req_cleanup(&req_close);
        };
        ::uv_fs_req_cleanup(uv_req);
      };
    }
  };
  //! \endcond

private: /*types*/
  using instance = request::instance< file >;

private: /*constructors*/
  explicit file(uv_t *_uv_req)
  {
    if (_uv_req)  instance::from(_uv_req)->ref();
    uv_req = _uv_req;
  }

public: /*constructors*/
  ~file() = default;
  /*! \brief Open and possibly create a file.
      \sa libuv API documentation: [`uv_fs_open()`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_open).
      \sa Linux: [`open()`](http://man7.org/linux/man-pages/man2/open.2.html).
          Windows: [`_open()`](https://msdn.microsoft.com/en-us/library/z0kc8e3z.aspx).

      The file descriptor will be closed automatically when the file instance reference count has became zero. */
  file(const char *_path, int _flags, int _mode)
  {
    uv_req = instance::create();
    instance::from(uv_req)->supplemental_data().uv_req = static_cast< uv_t* >(uv_req);
    uv_status(::uv_fs_open(
        nullptr, static_cast< uv_t* >(uv_req),
        _path, _flags, _mode,
        nullptr
    ));
  }

  file(const file&) = default;
  file& operator =(const file&) = default;

  file(file&&) noexcept = default;
  file& operator =(file&&) noexcept = default;

public: /*interface*/
  /*! \brief Get the cross platform representation of a file handle (mostly being a POSIX-like file descriptor). */
  ::uv_file fd() const noexcept  { return static_cast< uv_t* >(uv_req)->result; }
  /*! \brief The file path. */
  const char* path() const noexcept  { return static_cast< uv_t* >(uv_req)->path; }

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

  /*! \brief Read data from the file into the buffers described by `_buf` object.
      \returns The number of bytes read or relevant [libuv error constant](http://docs.libuv.org/en/v1.x/errors.html#error-constants).
      \sa libuv API documentation: [`uv_fs_read()`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_read).
      \sa Linux: [`preadv()`](http://man7.org/linux/man-pages/man2/preadv.2.html). */
  int read(buffer &_buf, int64_t _offset = -1) const noexcept
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
  /*! \brief Write data to the file from the buffers described by `_buf` object.
      \returns The number of bytes written or relevant [libuv error constant](http://docs.libuv.org/en/v1.x/errors.html#error-constants).
      \sa libuv API documentation: [`uv_fs_write()`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_write).
      \sa Linux: [`pwritev()`](http://man7.org/linux/man-pages/man2/pwritev.2.html). */
  int write(const buffer &_buf, int64_t _offset = -1) noexcept
  {
    uv_t req_write;
    uv_status(::uv_fs_write(
        nullptr, &req_write,
        fd(),
        static_cast< const buffer::uv_t* >(_buf), _buf.count(),
        _offset,
        nullptr
    ));
    ::uv_fs_req_cleanup(&req_write);
    return uv_status();
  }

  /*! \brief Get information about the file.
      \details The result of the request is stored internally in the libuv request description structure.
      Use `uv_status()` member function to check the status of the request completion.
      \sa libuv API documentation: [`uv_fs_t.statbuf`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_t.statbuf),
                                   [`uv_stat_t`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_stat_t),
                                   [`uv_fs_fstat()`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_fstat).
      \sa Linux: [`fstat()`](http://man7.org/linux/man-pages/man2/fstat.2.html). */
  const ::uv_stat_t& fstat() const noexcept
  {
    uv_t* &req_fstat = instance::from(uv_req)->supplemental_data().req_fstat;
    if (!req_fstat)
    {
      req_fstat = new uv_t;
      std::memset(req_fstat, 0, sizeof(*req_fstat));
    }
    else
      ::uv_fs_req_cleanup(req_fstat);

    uv_status(::uv_fs_fstat(
        nullptr, req_fstat,
        fd(),
        nullptr
    ));
    return req_fstat->statbuf;
  }

  /*! \brief Synchronize all modified file's in-core data with storage.
      \sa libuv API documentation: [`uv_fs_fsync()`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_fsync).
      \sa Linux: [`fsync()`](http://man7.org/linux/man-pages/man2/fsync.2.html). */
  int fsync() noexcept
  {
    uv_t req_fsync;
    uv_status(::uv_fs_fsync(
        nullptr, &req_fsync,
        fd(),
        nullptr
    ));
    ::uv_fs_req_cleanup(&req_fsync);
    return uv_status();
  }
  /*! \brief Synchronize modified file's data with storage excluding
      flushing unnecessary metadata to reduce disk activity.
      \sa libuv API documentation: [`uv_fs_fdatasync()`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_fdatasync).
      \sa Linux: [`fdatasync()`](http://man7.org/linux/man-pages/man2/fdatasync.2.html). */
  int fdatasync() noexcept
  {
    uv_t req_fdatasync;
    uv_status(::uv_fs_fdatasync(
        nullptr, &req_fdatasync,
        fd(),
        nullptr
    ));
    ::uv_fs_req_cleanup(&req_fdatasync);
    return uv_status();
  }

  /*! \brief Truncate the file to a specified length.
      \sa libuv API documentation: [`uv_fs_ftruncate()`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_ftruncate).
      \sa Linux: [`ftruncate()`](http://man7.org/linux/man-pages/man2/ftruncate.2.html). */
  int ftruncate(int64_t _offset) noexcept
  {
    uv_t req_ftruncate;
    uv_status(::uv_fs_ftruncate(
        nullptr, &req_ftruncate,
        fd(),
        _offset,
        nullptr
    ));
    ::uv_fs_req_cleanup(&req_ftruncate);
    return uv_status();
  }

  /*! \brief Change permissions of the file.
      \sa libuv API documentation: [`uv_fs_fchmod()`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_fchmod).
      \sa Linux: [`fchmod()`](http://man7.org/linux/man-pages/man2/fchmod.2.html). */
  int fchmod(int _mode)
  {
    uv_t req_fchmod;
    uv_status(::uv_fs_fchmod(
        nullptr, &req_fchmod,
        fd(),
        _mode,
        nullptr
    ));
    ::uv_fs_req_cleanup(&req_fchmod);
    return uv_status();
  }
  /*! \brief Change ownership of the file.
      \note Not implemented on Windows.
      \sa libuv API documentation: [`uv_fs_fchown()`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_fchown).
      \sa Linux: [`fchown()`](http://man7.org/linux/man-pages/man2/fchown.2.html). */
  int fchown(::uv_uid_t _uid, ::uv_gid_t _gid)
  {
    uv_t req_fchown;
    uv_status(::uv_fs_fchown(
        nullptr, &req_fchown,
        fd(),
        _uid, _gid,
        nullptr
    ));
    ::uv_fs_req_cleanup(&req_fchown);
    return uv_status();
  }

  /*! \brief Change file timestamps.
      \sa libuv API documentation: [`uv_fs_futime()`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_futime).
      \sa Linux: [`futimes()`](http://man7.org/linux/man-pages/man3/futimes.3.html). */
  int futime(double _atime, double _mtime)
  {
    uv_t req_futime;
    uv_status(::uv_fs_futime(
        nullptr, &req_futime,
        fd(),
        _atime, _mtime,
        nullptr
    ));
    ::uv_fs_req_cleanup(&req_futime);
    return uv_status();
  }

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_req); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_req); }
};


}


#endif
