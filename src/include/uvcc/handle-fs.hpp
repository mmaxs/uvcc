
#ifndef UVCC_HANDLE_FS__HPP
#define UVCC_HANDLE_FS__HPP

#include "uvcc/utility.hpp"
#include "uvcc/handle-base.hpp"
#include "uvcc/handle-io.hpp"
#include "uvcc/loop.hpp"

#include <uv.h>

#ifdef _WIN32
#include <io.h>         // _telli64()
#else
#include <unistd.h>     // lseek64()
#endif

#include <functional>   // function
#include <string>       // string
#include <utility>      // move()


namespace uv
{


/*! \ingroup doxy_group__handle
    \brief The open file handle. */
class file : public io
{
  //! \cond
  friend class handle::uv_interface;
  friend class handle::instance< file >;
  friend class fs;
  //! \endcond

public: /*types*/
  using uv_t = ::uv_fs_t;
  using on_open_t = std::function< void(file) >;
  /*!< \brief The function type of the callback called after the asynchronous file open/create operation has been completed. */

protected: /*types*/
  //! \cond internals
  //! \addtogroup doxy_group__internals
  //! \{

  struct properties : io::properties
  {
    on_open_t open_cb;
    struct
    {
      ::uv_buf_t uv_buf_struct = { 0,};
      ::uv_fs_t  uv_req_struct = { 0,};
    } rd;
    std::size_t write_queue_size = 0;
    int is_closing = 0;
  };

  struct uv_interface : handle::uv_fs_interface, io::uv_interface
  {
    int is_closing(void *_uv_fs) const noexcept override
    { return instance::from(_uv_fs)->properties().is_closing; }

    std::size_t write_queue_size(void *_uv_handle) const noexcept override
    { return instance::from(_uv_handle)->properties().write_queue_size; }

    int read_start(void *_uv_handle, int64_t _offset) const noexcept override
    {
      auto instance_ptr = instance::from(_uv_handle);
      auto &properties = instance_ptr->properties();

      properties.rd.uv_req_struct.data = instance_ptr;

      if (_offset < 0)
      {
#ifdef _WIN32
        /*! \sa Windows: [`_tell()`, `_telli64()`](https://msdn.microsoft.com/en-us/library/c3kc5e7a.aspx). */
        _offset = _telli64(instance_ptr->uv_handle_struct.result);
#else
        _offset = lseek64(instance_ptr->uv_handle_struct.result, 0, SEEK_CUR);
#endif
      }
      properties.rdoffset = _offset;

      return file_read(instance_ptr);
    }

    int read_stop(void *_uv_handle) const noexcept override  { return 0; }
  };

  //! \}
  //! \endcond

private: /*types*/
  using instance = handle::instance< file >;

protected: /*constructors*/
  //! \cond
  explicit file(uv::loop::uv_t *_loop, ::uv_file _fd, const char *_path)
  {
    uv_handle = instance::create();
    static_cast< uv_t* >(uv_handle)->loop = _loop;
    static_cast< uv_t* >(uv_handle)->result = _fd;
    static_cast< uv_t* >(uv_handle)->path = _path;
    if (_fd < 0)  instance::from(uv_handle)->properties().is_closing = 1;
    instance::from(uv_handle)->book_loop();
  }

  explicit file(uv_t *_uv_handle) : io(static_cast< io::uv_t* >(_uv_handle))  {}
  //! \endcond

public: /*constructors*/
  ~file() = default;

  file(const file&) = default;
  file& operator =(const file&) = default;

  file(file&&) noexcept = default;
  file& operator =(file&&) noexcept = default;

  /*! \name File handle constructors - open and possibly create a file:
      \sa libuv API documentation: [`uv_fs_open()`](http://docs.libuv.org/en/v1.x/fs.html#c.uv_fs_open).
      \sa Linux: [`open()`](http://man7.org/linux/man-pages/man2/open.2.html).\n
          Windows: [`_open()`](https://msdn.microsoft.com/en-us/library/z0kc8e3z.aspx).

      The file descriptor will be closed automatically when the file handle reference count has became zero. */
  //! \{
  /*! \brief Open and possibly create a file _synchronously_. */
  file(uv::loop &_loop, const char *_path, int _flags, int _mode)
  {
    uv_handle = instance::create();

    auto uv_ret = uv_status(::uv_fs_open(
        static_cast< uv::loop::uv_t* >(_loop), static_cast< uv_t* >(uv_handle),
        _path, _flags, _mode,
        nullptr
    ));
    if (uv_ret >= 0)  instance::from(uv_handle)->book_loop();
  }
  /*! \brief Open and possibly create a file _asynchronously_.
      \note If the `_open_cb` callback is empty the operation is completed _synchronously_. */
  file(uv::loop &_loop, const char *_path, int _flags, int _mode, const on_open_t &_open_cb)
  {
    if (!_open_cb)
    {
      new (this) file(_loop, _path, _flags, _mode);
      return;
    }

    uv_handle = instance::create();

    auto instance_ptr = instance::from(uv_handle);
    instance_ptr->ref();

    instance_ptr->properties().open_cb = _open_cb;

    uv_status(0);
    auto uv_ret = ::uv_fs_open(
        static_cast< uv::loop::uv_t* >(_loop), static_cast< uv_t* >(uv_handle),
        _path, _flags, _mode,
        open_cb
    );
    if (uv_ret >= 0)
      instance_ptr->book_loop();
    else
    {
      uv_status(uv_ret);
      instance_ptr->unref();
    }
  }
  /*! \brief Create a file object from an existing file descriptor. */
  file(uv::loop &_loop, ::uv_file _fd) : file(static_cast< uv::loop::uv_t* >(_loop), _fd, nullptr)  {}
  //! \}

private: /*functions*/
  template< typename = void > static void open_cb(::uv_fs_t*);
  template< typename = void > static void read_cb(::uv_fs_t*);

  static int file_read(instance *_instance_ptr)
  {
    auto &properties = _instance_ptr->properties();

    io_alloc_cb(&_instance_ptr->uv_handle_struct, 65536, &properties.rd.uv_buf_struct);

    return ::uv_fs_read(
      _instance_ptr->uv_handle_struct.loop, &properties.rd.uv_req_struct,
      _instance_ptr->uv_handle_struct.result,
      &properties.rd.uv_buf_struct, 1,
      properties.rdoffset,
      read_cb
    );
  }

public: /*interface*/
  /*! \brief The amount of bytes waiting to be written to the file. */
  std::size_t write_queue_size() const noexcept  { return instance::from(uv_handle)->properties().write_queue_size; }

  /*! \brief Get the cross platform representation of the file handle.
      \details On Windows this function returns _a C run-time file descriptor_ which differs from the
      _operating-system file handle_ that is returned by `handle::fileno()` function.
      On Unix-like systems both functions return the same value.
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
  }

  io_read_cb(&instance_ptr->uv_handle_struct, nread , &properties.rd.uv_buf_struct, nullptr);

  ::uv_fs_req_cleanup(_uv_req);

  switch (properties.rdcmd_state)
  {
  case rdcmd::UNKNOWN:
  case rdcmd::STOP:
  case rdcmd::PAUSE:
      break;
  case rdcmd::START:
  case rdcmd::RESUME:
      {
        instance_ptr->uv_error = 0;
        auto uv_ret = file_read(instance_ptr);
        if (uv_ret < 0)  instance_ptr->uv_error = uv_ret;
      }
      break;
  }
}



/*! \ingroup doxy_group__handle
    \brief FS Event handle.
    \sa libuv API documentation: [`uv_fs_event_t` — FS Event handle](http://docs.libuv.org/en/v1.x/fs_event.html#uv-fs-event-t-fs-event-handle). */
class fs_event : public handle
{
  //! \cond
  friend class handle::uv_interface;
  friend class handle::instance< fs_event >;
  //! \endcond

public: /*types*/
  using uv_t = ::uv_fs_event_t;
  using on_fs_event_t = std::function< void(fs_event _handle, const char *_filename, int _events) >;
  /*!< \brief The function type of the FS event callback.
       \sa libuv API documentation: [`uv_fs_event_cb`](http://docs.libuv.org/en/v1.x/fs_event.html#c.uv_fs_event_cb),
                                    [`uv_fs_event`](http://docs.libuv.org/en/v1.x/fs_event.html#c.uv_fs_event). */

protected: /*types*/
  //! \cond internals
  //! \addtogroup doxy_group__internals
  //! \{

  enum class opcmd  { UNKNOWN, STOP, START };

  struct properties : handle::properties
  {
    opcmd opcmd_state = opcmd::UNKNOWN;
    int event_flags = 0;
    std::string path;
    on_fs_event_t fs_event_cb;
  };

  struct uv_interface : handle::uv_handle_interface  {};

  //! \}
  //! \endcond

private: /*types*/
  using instance = handle::instance< fs_event >;

private: /*functions*/
  template < typename = void > static void fs_event_cb(::uv_fs_event_t*, const char*, int, int);

protected: /*constructors*/
  //! \cond
  explicit fs_event(uv_t *_uv_handle) : handle(reinterpret_cast< handle::uv_t* >(_uv_handle))  {}
  //! \endcond

public: /*constructors*/
  ~fs_event() = default;

  fs_event(const fs_event&) = default;
  fs_event& operator =(const fs_event&) = default;

  fs_event(fs_event&&) noexcept = default;
  fs_event& operator =(fs_event&&) noexcept = default;

  /*! \brief Create an `fs_event`. */
  explicit fs_event(uv::loop &_loop, int _event_flags = 0)
  {
    uv_handle = instance::create();

    auto uv_ret = ::uv_fs_event_init(static_cast< uv::loop::uv_t* >(_loop), static_cast< uv_t* >(uv_handle));
    instance::from(uv_handle)->properties().event_flags = _event_flags;

    if (uv_status(uv_ret) < 0)  return;

    instance::from(uv_handle)->book_loop();
  }

public: /*interface*/
  /*! \brief Set the FS path being monitored by this handle. */
  std::string& path() const noexcept  { return  instance::from(uv_handle)->properties().path; }

  /*! \brief Set the FS event callback. */
  on_fs_event_t& on_fs_event() const noexcept  { return instance::from(uv_handle)->properties().fs_event_cb; }

  /*! \brief Start the handle.
      \details Repeated call to this function results in the automatic call to `stop()` first.
      \note On successful start this function adds an extra reference to the handle instance,
      which is released when the counterpart function `stop()` is called.
      \sa libuv API documentation: [`uv_fs_event_start()`](http://docs.libuv.org/en/v1.x/fs_event.html#c.uv_fs_event_start). */
  int start() const
  {
    auto instance_ptr = instance::from(uv_handle);
    auto &properties = instance_ptr->properties();

    auto opcmd_state0 = properties.opcmd_state;

    properties.opcmd_state = opcmd::START;
    instance_ptr->ref();  // REF:START -- make sure it will exist for the future fs_event_cb() calls until stop()

    switch (opcmd_state0)
    {
    case opcmd::UNKNOWN:
    case opcmd::STOP:
        break;
    case opcmd::START:
        uv_status(::uv_fs_event_stop(static_cast< uv_t* >(uv_handle)));
        instance_ptr->unref();  // UNREF:RESTART -- adjust extra reference number
        break;
    }

    uv_status(0);
    auto uv_ret = ::uv_fs_event_start(static_cast< uv_t* >(uv_handle), fs_event_cb, properties.path.c_str(), properties.event_flags);
    if (uv_ret < 0)
    {
      uv_status(uv_ret);
      properties.opcmd_state = opcmd::UNKNOWN;
      instance_ptr->unref();  // UNREF:START_FAILURE -- release the extra reference on failure
    }

    return uv_ret;
  }

  /*! \brief Start the handle with the given callback.
      \details This is equivalent for
      ```
      fs_event.on_fs_event() = std::bind(
          std::forward< _Cb_ >(_cb), std::placeholders::_1, std::placeholders::_2, std::forward< _Args_ >(_args)...
      );
      fs_event.start();
      ```
      \sa `fs_event::start()` */
  template< class _Cb_, typename... _Args_, typename = std::enable_if_t< std::is_convertible<
      decltype(std::bind(std::declval< _Cb_ >(), std::placeholders::_1, std::placeholders::_2, static_cast< _Args_&& >(std::declval< _Args_ >())...)),
      on_fs_event_t
  >::value > >
  int start(_Cb_ &&_cb, _Args_&&... _args) const
  {
    instance::from(uv_handle)->properties().fs_event_cb = std::bind(
        std::forward< _Cb_ >(_cb), std::placeholders::_1, std::placeholders::_2, std::forward< _Args_ >(_args)...
    );
    return start();
  }

  /*! \brief Start the handle with the given callback, watching for the specified FS path.
      \details This is equivalent for
      ```
      fs_event.path() = _path;
      fs_event.on_fs_event() = std::bind(
          std::forward< _Cb_ >(_cb), std::placeholders::_1, std::placeholders::_2, std::forward< _Args_ >(_args)...
      );
      fs_event.start();
      ```
      \sa `fs_event::start()` */
  template< class _Cb_, typename... _Args_, typename = std::enable_if_t< std::is_convertible<
      decltype(std::bind(std::declval< _Cb_ >(), std::placeholders::_1, std::placeholders::_2, static_cast< _Args_&& >(std::declval< _Args_ >())...)),
      on_fs_event_t
  >::value > >
  int start(std::string _path, _Cb_ &&_cb, _Args_&&... _args) const
  {
    instance::from(uv_handle)->properties().path = std::move(_path);
    instance::from(uv_handle)->properties().fs_event_cb = std::bind(
        std::forward< _Cb_ >(_cb), std::placeholders::_1, std::placeholders::_2, std::forward< _Args_ >(_args)...
    );
    return start();
  }

  /*! \brief Stop the handle, the callback will no longer be called. */
  int stop() const noexcept
  {
    auto instance_ptr = instance::from(uv_handle);
    auto &opcmd_state = instance_ptr->properties().opcmd_state;

    auto opcmd_state0 = opcmd_state;
    opcmd_state = opcmd::STOP;

    auto uv_ret = uv_status(::uv_fs_event_stop(static_cast< uv_t* >(uv_handle)));

    switch (opcmd_state0)
    {
    case opcmd::UNKNOWN:
    case opcmd::STOP:
        break;
    case opcmd::START:
        instance_ptr->unref();  // UNREF:STOP -- release the reference from start()
        break;
    }

    return uv_ret;
  }

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_handle); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_handle); }
};

template< typename >
void fs_event::fs_event_cb(::uv_fs_event_t *_uv_handle, const char *_filename, int _events, int _status)
{
  auto instance_ptr = instance::from(_uv_handle);
  instance_ptr->uv_error = _status;
  auto &fs_event_cb = instance_ptr->properties().fs_event_cb;
  if (fs_event_cb)  fs_event_cb(fs_event(_uv_handle), _filename, _events);
}


}


#endif
