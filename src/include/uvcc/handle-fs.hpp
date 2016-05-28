
#ifndef UVCC_HANDLE_FS__HPP
#define UVCC_HANDLE_FS__HPP

#include "uvcc/utility.hpp"
#include "uvcc/handle-base.hpp"
#include "uvcc/handle-io.hpp"
#include "uvcc/loop.hpp"

#include <uv.h>
#include <functional>   // function


namespace uv
{


/*! \ingroup doxy_group_handle
    \brief The open file handle. */
class file : public io
{
  //! \cond
  friend class handle::instance< file >;
  friend class fs;
  //! \endcond

public: /*types*/
  using uv_t = ::uv_fs_t;
  using on_open_t = std::function< void(file) >;
  /*!< \brief The function type of the callback called after the asynchronous file open/create operation has been completed. */

protected: /*types*/
  //! \cond
  struct properties : io::properties
  {
    on_open_t open_cb;
    struct
    {
      ::uv_buf_t uv_buf_struct = { 0,};
      ::uv_fs_t  uv_req_struct = { 0,};
      int64_t offset = -1;
    } rd;
    std::size_t write_queue_size = 0;
    int is_closing = 0;
  };

  struct uv_interface : uv_fs_interface, io::uv_interface
  {
    int is_closing(void *_uv_fs) const noexcept override
    { return instance::from(_uv_fs)->properties().is_closing; }

    std::size_t write_queue_size(void *_uv_handle) const noexcept override
    { return instance::from(_uv_handle)->properties().write_queue_size; }

    int read_start(void *_uv_handle, int64_t _offset) const noexcept override
    {
      auto instance_ptr = instance::from(_uv_handle);
      auto &rd = instance_ptr->properties().rd;

      rd.uv_req_struct.data = instance_ptr;
      rd.offset = _offset;

      return file_read_start(instance_ptr);
    }

    int read_stop(void *_uv_handle) const noexcept override  { return 0; }
  };
  //! \endcond

private: /*types*/
  using instance = handle::instance< file >;

private: /*constructors*/
  explicit file(uv_t *_uv_handle)
  {
    if (_uv_handle)  instance::from(_uv_handle)->ref();
    uv_handle = _uv_handle;
  }

public: /*constructors*/
  ~file() = default;

  file(const file&) = default;
  file& operator =(const file&) = default;

  file(file&&) noexcept = default;
  file& operator =(file&&) noexcept = default;

  /*! \name File handle constructors - open and possibly create a file:
      \sa libuv API documentation: [`uv_fs_open()`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_open).
      \sa Linux: [`open()`](http://man7.org/linux/man-pages/man2/open.2.html).
          Windows: [`_open()`](https://msdn.microsoft.com/en-us/library/z0kc8e3z.aspx).

      The file descriptor will be closed automatically when the file handle reference count has became zero. */
  //! \{
  /*! \brief Open and possibly create a file _synchronously_. */
  file(uv::loop &_loop, const char *_path, int _flags, int _mode)
  {
    uv_handle = instance::create();
    uv_status(::uv_fs_open(
        static_cast< uv::loop::uv_t* >(_loop), static_cast< uv_t* >(uv_handle),
        _path, _flags, _mode,
        nullptr
    ));
  }
  /*! \brief Open and possibly create a file _asynchronously_.
      \note If the `_open_cb` callback is empty the operation is completed _synchronously_. */
  file(uv::loop &_loop, const char *_path, int _flags, int _mode, const on_open_t &_open_cb)
  {
    if (!_open_cb)
    {
      new (this) file(_loop, _path, _flags, _mode);
      return;
    };

    uv_handle = instance::create();
    instance::from(uv_handle)->properties().open_cb = _open_cb;

    instance::from(uv_handle)->ref();

    uv_status(0);
    int ret = ::uv_fs_open(
        static_cast< uv::loop::uv_t* >(_loop), static_cast< uv_t* >(uv_handle),
        _path, _flags, _mode,
        open_cb
    );
    if (!ret)  uv_status(ret);
  }
  /*! \brief Create a file object from an existing file descriptor. */
  file(uv::loop &_loop, ::uv_file _fd)
  {
    uv_handle = instance::create();
    static_cast< uv_t* >(uv_handle)->loop = static_cast< uv::loop::uv_t* >(_loop);
    static_cast< uv_t* >(uv_handle)->result = _fd;
    static_cast< uv_t* >(uv_handle)->path = nullptr;
  }
  //! \}

private: /*functions*/
  template< typename = void > static void open_cb(::uv_fs_t*);
  template< typename = void > static void read_cb(::uv_fs_t*);

  static int file_read_start(instance *_instance_ptr)
  {
    auto &rd = _instance_ptr->properties().rd;

    io_alloc_cb(&_instance_ptr->uv_handle_struct, 65536, &rd.uv_buf_struct);

    return ::uv_fs_read(
      _instance_ptr->uv_handle_struct.loop, &rd.uv_req_struct,
      _instance_ptr->uv_handle_struct.result,
      &rd.uv_buf_struct, 1,
      rd.offset,
      read_cb
    );
  }

public: /*interface*/
  /*! \brief The amount of bytes waiting to be written to the file. */
  std::size_t write_queue_size() const noexcept  { return instance::from(uv_handle)->properties().write_queue_size; }

  /*! \brief Get the cross platform representation of the file handle.
      \details On Windows this function returns _a C run-time file descriptor_ which differs from the
      _operating-system file handle_ that is returned by `handle::fileno()` function.
      On Unicies both functions return the same value.
      \sa Windows: [`_open_osfhandle()`](https://msdn.microsoft.com/en-us/library/bdts1c9x.aspx). */
  ::uv_file fd() const noexcept  { return static_cast< uv_t* >(uv_handle)->result; }

  /*! \brief The file path.
      \sa libuv API documentation: [`uv_fs_t.path`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_t.path). */
  const char* path() const noexcept  { return static_cast< uv_t* >(uv_handle)->path; }

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_handle); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_handle); }
};

template< typename >
void file::open_cb(::uv_fs_t *_uv_handle)
{
  auto instance_ptr = instance::from(_uv_handle);
  instance_ptr->uv_error = _uv_handle->result;

  ref_guard< instance > unref_req(*instance_ptr, adopt_ref);

  auto &open_cb = instance_ptr->properties().open_cb;
  if (open_cb)  open_cb(file(_uv_handle));
}

template< typename >
void file::read_cb(::uv_fs_t *_uv_req)
{
  auto instance_ptr = static_cast< instance* >(_uv_req->data);
  auto &properties = instance_ptr->properties();

  ssize_t nread = _uv_req->result == 0 ? UV_EOF : _uv_req->result;
  if (nread < 0)  // on error or EOF release the unused buffer and replace it with a null-initialized structure
  {
    buffer::instance::from(buffer::instance::uv_buf::from(properties.rd.uv_buf_struct.base))->unref();
    properties.rd.uv_buf_struct = ::uv_buf_init(nullptr, 0);
  };

  io_read_cb(&instance_ptr->uv_handle_struct, nread , &properties.rd.uv_buf_struct, &properties.rd.offset);

  ::uv_fs_req_cleanup(_uv_req);

  switch (properties.rdcmd_state)
  {
  case rdcmd::NOP:
  case rdcmd::STOP:
  case rdcmd::PAUSE:
      break;
  case rdcmd::START:
  case rdcmd::CONTINUE:
      {
        if (properties.rd.offset >= 0 and nread > 0)  properties.rd.offset += nread;

        instance_ptr->uv_error = 0;
        int ret = file_read_start(instance_ptr);
        if (!ret)  instance_ptr->uv_error = ret;
      }
      break;
  }
}


}


#endif
