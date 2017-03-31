
#include "uvcc.hpp"

#include <cstdio>
#include <cinttypes>  // PRI*
#include <vector>
#include <fcntl.h>    // O_*

#ifndef _WIN32
#include <signal.h>
sighandler_t sigpipe_handler = signal(SIGPIPE, SIG_IGN);  // ignore SIGPIPE
#endif


#ifndef NDEBUG
#define DEBUG_LOG(condition, format, ...)  do {\
    if ((condition))\
    {\
      fflush(stdout);\
      fprintf(stderr, (format), ##__VA_ARGS__);\
      fflush(stderr);\
    }\
} while (0)
#else
#define DEBUG_LOG(condition, ...)  (void)(condition)
#endif


#define PRINT_UV_ERR(code, printf_args...)  do {\
  fflush(stdout);\
  fprintf(stderr, "" printf_args);\
  fprintf(stderr, ": %s (%i): %s\n", ::uv_err_name(code), (int)(code), ::uv_strerror(code));\
  fflush(stderr);\
} while (0)


uv::io in = uv::io::guess_handle(uv::loop::Default(), fileno(stdin)),
       out = uv::io::guess_handle(uv::loop::Default(), fileno(stdout));

constexpr std::size_t BUFFER_SIZE = 8192;
constexpr std::size_t WRITE_QUEUE_SIZE_UPPER_LIMIT = 128*BUFFER_SIZE,
                      WRITE_QUEUE_SIZE_LOWER_LIMIT =  16*BUFFER_SIZE;

bool wr_err_reported = false;

std::vector< uv::file > files;
std::size_t file_write_queues_size = 0;


uv::buffer alloc_cb(uv::handle, std::size_t);
void write_to_stdout_cb(uv::output, uv::buffer);


int main(int _argc, char *_argv[])
{
  if (!in)
  {
    PRINT_UV_ERR(in.uv_status(), "stdin open (%s)", in.type_name());
    return in.uv_status();
  }
  DEBUG_LOG(true, "[debug] stdin: %s handle [0x%08tX]\n", in.type_name(), (ptrdiff_t)static_cast< ::uv_handle_t* >(in));

  if (!out)
  {
    PRINT_UV_ERR(out.uv_status(), "stdout open (%s)", out.type_name());
    return out.uv_status();
  }
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

  return uv::loop::Default().run(UV_RUN_DEFAULT);
}


uv::buffer alloc_cb(uv::handle, std::size_t)
{
  constexpr std::size_t default_size = BUFFER_SIZE;
  static std::vector< uv::buffer > buf_pool;

  for (std::size_t i = 0; i < buf_pool.size(); ++i)  if (buf_pool[i].nrefs() == 1)  // a spare item
  {
      buf_pool[i].len() = default_size;  // restore the buffer capacity size

      DEBUG_LOG(true, "[debug] buffer pool (size=%zu): spare item #%zu\n", buf_pool.size(), i+1);
      return buf_pool[i];
  }

  buf_pool.emplace_back(uv::buffer{ default_size });

  DEBUG_LOG(true, "[debug] buffer pool (size=%zu): new item #%zu\n", buf_pool.size(), buf_pool.size());
  return buf_pool.back();
}


void write_to_files(uv::buffer, int64_t);

void write_to_stdout_cb(uv::output _wr, uv::buffer _buf)
{
  if (!_wr)
  {
    if (!wr_err_reported)  // report only the very first occurrence of the failure
    {
      PRINT_UV_ERR(_wr.uv_status(), "write to stdout (%s) at offset %" PRIi64, _wr.handle().type_name(), _wr.offset());
      wr_err_reported = true;
    }

    in.read_stop();
  }
  else  // dispatch the data for writing to files only when it successfully has been written to stdout
    write_to_files(_buf, _wr.offset());

  auto total_write_pending_bytes = out.write_queue_size() + file_write_queues_size;
  int ret = in.read_resume(total_write_pending_bytes <= WRITE_QUEUE_SIZE_LOWER_LIMIT);
  DEBUG_LOG(ret == 0, "[debug] read resumed (total_write_pending_bytes=%zu)\n", total_write_pending_bytes);
}


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
