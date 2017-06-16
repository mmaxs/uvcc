
#include "uvcc.hpp"

#include <cstdio>
#include <cstdlib>    // strtoll()
#include <cinttypes>  // PRI*
#include <fcntl.h>    // O_*


#ifndef _WIN32
#include <signal.h>
sighandler_t sigpipe_handler = signal(SIGPIPE, SIG_IGN);  // ignore SIGPIPE
#endif


#define PRINT_UV_ERR(code, printf_args...)  do {\
  fflush(stdout);\
  fprintf(stderr, "" printf_args);\
  fprintf(stderr, ": %s (%i): %s\n", ::uv_err_name(code), (int)(code), ::uv_strerror(code));\
  fflush(stderr);\
} while (0)


uv::file in = uv::file(uv::loop::Default(), -1);
uv::io out = uv::io::guess_handle(uv::loop::Default(), fileno(stdout));


constexpr std::size_t BUFFER_SIZE = 8192;
constexpr std::size_t WRITE_QUEUE_SIZE_UPPER_LIMIT = 128*BUFFER_SIZE,
                      WRITE_QUEUE_SIZE_LOWER_LIMIT =  16*BUFFER_SIZE;

bool wr_err_reported = false;


#if 0
uv::buffer alloc_cb(uv::handle, std::size_t);
#endif


void write_to_stdout_cb(uv::output, uv::buffer);


int main(int _argc, char *_argv[])
{
  if (_argc > 1)
  {
    in = uv::file(uv::loop::Default(),
        _argv[1],
#ifdef _WIN32
        _O_RDONLY|_O_BINARY,
#else
        O_RDONLY,
#endif
        0
    );
    if (!in)
    {
      PRINT_UV_ERR(in.uv_status(), "%s: input file open (%s)", _argv[0], in.path());
      return in.uv_status();
    }
  }
  else
  {
    fprintf(stderr, "%s: input file required\n", _argv[0]);
    return 1;
  }

  if (!out)
  {
    PRINT_UV_ERR(out.uv_status(), "%s: stdout open (%s)", _argv[0], out.type_name());
    return out.uv_status();
  }

  int64_t start_offset = 0;
  if (_argc > 2)  start_offset = std::strtoll(_argv[2], nullptr, 0);

  {
    uv::fs::stat fstat;
    fstat.run(in);
    if (!fstat)
    {
      PRINT_UV_ERR(fstat.uv_status(), "%s: input file stat request (%s)", _argv[0], in.path());
      return fstat.uv_status();
    }

    if (start_offset < 0)  start_offset = fstat.result().st_size + start_offset;
    if (start_offset < 0)  start_offset = 0;
  }

  in.read_start(
      [](uv::handle, std::size_t _suggested_size){ return uv::buffer{ _suggested_size }; },
      [](uv::io _io, ssize_t _nread, uv::buffer _buf, int64_t _offset, void *_info)
      {
        if (_nread < 0)
        {
          if (_nread != UV_EOF)  PRINT_UV_ERR(_nread, "input file reading (%s) at offset %" PRIi64, in.path(), _offset);
          _io.read_stop();
        }
        else if (_nread > 0)
        {
          // set the actual data size
          _buf.len() = _nread;

          // write to stdout
          uv::output io_wr;
          io_wr.on_request() = write_to_stdout_cb;

          io_wr.run(out, _buf, _offset, _info);
          if (!io_wr)
          {
            PRINT_UV_ERR(io_wr.uv_status(), "stdout write initiation (%s) at offset %" PRIi64, out.type_name(), _offset);
            _io.read_stop();
          }

          in.read_pause(out.write_queue_size() >= WRITE_QUEUE_SIZE_UPPER_LIMIT);
        }
      },
      BUFFER_SIZE,
      start_offset
  );
  if (!in)
  {
    PRINT_UV_ERR(in.uv_status(), "input file read initiation (%s)", in.path());
    return in.uv_status();
  }

#if 0
  if (!in)
  {
    PRINT_UV_ERR(in.uv_status(), "stdin open (%s)", in.type_name());
    return in.uv_status();
  }
  DEBUG_LOG(true, "[debug] stdin: %s handle [0x%08tX]\n", in.type_name(), (ptrdiff_t)static_cast< ::uv_handle_t* >(in));

  DEBUG_LOG(true, "[debug] stdout: %s handle [0x%08tX]\n", out.type_name(), (ptrdiff_t)static_cast< ::uv_handle_t* >(out));

  // open files specified as the program arguments on the command line
  constexpr int mode = 
  #ifdef _WIN32
      _S_IREAD|_S_IWRITE;
  #else
      S_IRWXU|S_IRWXG|S_IRWXO;
  #endif
  for (int i = 1; i < _argc; ++i)
  {
    uv::file f(uv::loop::Default(), _argv[i], O_CREAT|O_TRUNC|O_WRONLY, mode);
    if (f)
      files.emplace_back(std::move(f));
    else
      PRINT_UV_ERR(f.uv_status(), "file open (%s)", f.path());
  }

  // attach the handle to the loop for reading incoming data
  in.read_start(alloc_cb,
      [](uv::io _io, ssize_t _nread, uv::buffer _buf, int64_t _offset, void *_info) -> void
      {
        if (_nread < 0)
        {
          if (_nread != UV_EOF)  PRINT_UV_ERR(_nread, "read from stdin (%s)", in.type_name());
          _io.read_stop();
        }
        else if (_nread > 0)
        {
          // set the actual data size
          _buf.len() = _nread;

          // write to stdout
          uv::output io_wr;
          io_wr.on_request() = write_to_stdout_cb;

          io_wr.run(out, _buf, _offset, _info);  // note: UDP connected sockets are not supported by libuv
          if (!io_wr)
          {
            PRINT_UV_ERR(io_wr.uv_status(), "write initiation to stdout (%s) at offset %" PRIi64, out.type_name(), _offset);
            _io.read_stop();
          }
        }

        auto total_write_pending_bytes = out.write_queue_size() + file_write_queues_size;
        int ret = in.read_pause(total_write_pending_bytes >= WRITE_QUEUE_SIZE_UPPER_LIMIT);
        DEBUG_LOG(ret == 0, "[debug] read paused (total_write_pending_bytes=%zu)\n", total_write_pending_bytes);
      }
  );
  if (!in)
  {
    PRINT_UV_ERR(in.uv_status(), "read initiation from stdin (%s)", in.type_name());
    return in.uv_status();
  }
#endif
  return uv::loop::Default().run(UV_RUN_DEFAULT);
}

#if 0
class buffer_pool
{
private: /*data*/
  bool pool_destroying = false;
  std::size_t buf_size;
  std::size_t num_total_items;
  std::vector< uv::buffer > spare_items_pool;

public: /*constructors*/
  ~buffer_pool()
  {
    DEBUG_LOG(true, "[debug] buffer pool destroying: spare_items=%zu total_items=%zu\n", spare_items_pool.size(), num_total_items);
    pool_destroying = true;
  }

  buffer_pool(std::size_t _buffer_size, std::size_t _init_pool_size, std::size_t _init_pool_capacity)
  : buf_size(_buffer_size), num_total_items(0)
  {
    spare_items_pool.reserve(_init_pool_capacity);
    while (_init_pool_size--)  spare_items_pool.emplace_back(new_item());
  }

  buffer_pool(const buffer_pool&) = delete;
  buffer_pool& operator =(const buffer_pool&) = delete;

  buffer_pool(buffer_pool&&) noexcept = default;
  buffer_pool& operator =(buffer_pool&&) noexcept = default;

private: /*functions*/
  uv::buffer new_item()
  {
    uv::buffer ret{ buf_size };
    ret.sink_cb() = [this](uv::buffer &_buf)
    {
      if (pool_destroying)  return;  // buffers, that are being destroyed on `spare_items_pool` emptying
                                     // during its destroying when executing the `buffer_pool` class'
                                     // destructor, are not to be added back into `spare_items_pool`

      _buf.len() = buf_size;  // restore buffer size
      spare_items_pool.push_back(std::move(_buf));
    };

    ++num_total_items;
    DEBUG_LOG(true, "[debug] buffer pool: new item #%zu\n", num_total_items);
    return std::move(ret);
  }

public: /*interface*/
  std::size_t buffer_size() const noexcept  { return buf_size; }
  std::size_t total_items() const noexcept  { return num_total_items; }
  std::size_t spare_items() const noexcept  { return spare_items_pool.size(); }

  uv::buffer get()
  {
    if (spare_items_pool.empty())
      return new_item();
    else
    {
      uv::buffer ret = std::move(spare_items_pool.back());
      spare_items_pool.pop_back();
      return std::move(ret);
    }
  }
} buffers(
    BUFFER_SIZE,
    WRITE_QUEUE_SIZE_LOWER_LIMIT/BUFFER_SIZE,
    WRITE_QUEUE_SIZE_UPPER_LIMIT/BUFFER_SIZE + 1
);

uv::buffer alloc_cb(uv::handle, std::size_t)  { return buffers.get(); }


void write_to_files(uv::buffer, int64_t);
#endif

void write_to_stdout_cb(uv::output _wr, uv::buffer _buf)
{
  if (!_wr)
  {
    if (!wr_err_reported)  // report only the very first occurrence of the failure
    {
      PRINT_UV_ERR(_wr.uv_status(), "stdout writing (%s) at offset %" PRIi64, _wr.handle().type_name(), _wr.offset());
      wr_err_reported = true;
    }

    in.read_stop();
  }

  in.read_resume(out.write_queue_size() <= WRITE_QUEUE_SIZE_LOWER_LIMIT);
}

#if 0
void write_to_file_cb(uv::fs::write, uv::buffer);

void write_to_files(uv::buffer _buf, int64_t _offset)
{
  for (auto &file : files)
  {
    uv::fs::write wr;
    wr.on_request() = write_to_file_cb;

    wr.run(file, _buf, _offset);
    if (wr)
      file_write_queues_size += _buf.len();
    else
      PRINT_UV_ERR(wr.uv_status(), "write initiation to file (%s) at offset %" PRIi64, file.path(), wr.offset());
  }
}


void write_to_file_cb(uv::fs::write _wr, uv::buffer _buf)
{
  if (!_wr)
    PRINT_UV_ERR(_wr.uv_status(), "write to file (%s) at offset %" PRIi64, _wr.handle().path(), _wr.offset());
  else
  {
    file_write_queues_size -= _buf.len();

    auto total_write_pending_bytes = out.write_queue_size() + file_write_queues_size;
    int ret = in.read_resume(total_write_pending_bytes <= WRITE_QUEUE_SIZE_LOWER_LIMIT);
    DEBUG_LOG(ret == 0, "[debug] read resumed (total_write_pending_bytes=%zu)\n", total_write_pending_bytes);
  }
}
#endif
