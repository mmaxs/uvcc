
#include "uvcc.hpp"
#include <cstdio>
#include <vector>
#include <fcntl.h>  // O_*


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
#define DEBUG_LOG(condition, ...)  (void)((condition))
#endif


#define PRINT_UV_ERR(code, prefix, ...)  do {\
  fflush(stdout);\
  fprintf(stderr, (prefix), ##__VA_ARGS__);\
  fprintf(stderr, ": %s (%i): %s\n", ::uv_err_name(code), (int)(code), ::uv_strerror(code));\
  fflush(stderr);\
} while (0)


uv::io in = uv::io::guess_handle(uv::loop::Default(), fileno(stdin)),
       out = uv::io::guess_handle(uv::loop::Default(), fileno(stdout));

constexpr std::size_t WRITE_QUEUE_SIZE_UPPER_LIMIT =  4*8192,
                      WRITE_QUEUE_SIZE_LOWER_LIMIT =  2*8192;
std::size_t all_write_queues_size = 0;

std::vector< uv::file > files;


uv::buffer alloc_cb(uv::handle, std::size_t);
template< class _WriteReq_ > void write_cb(_WriteReq_, uv::buffer);


int main(int _argc, char *_argv[])
{
  if (!in)
  {
    PRINT_UV_ERR(in.uv_status(), "stdin open (%s)", in.type_name());
    return in.uv_status();
  }
  if (!out)
  {
    PRINT_UV_ERR(out.uv_status(), "stdout open (%s)", out.type_name());
    return out.uv_status();
  }

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
          _io.read_stop();
          if (_nread != UV_EOF)  PRINT_UV_ERR(_nread, "stdin read (%s)", in.type_name());
        }
        else if (_nread > 0)
        {
          // set the actual data size
          _buf.len() = _nread;

          // write to stdout
          uv::output io_wr;
          io_wr.on_request() = write_cb< uv::output >;
          io_wr.run(out, _buf, _offset, _info);  // note: UDP connected sockets are not supported by libuv

          if (io_wr)
            all_write_queues_size += _buf.len();
          else
            PRINT_UV_ERR(io_wr.uv_status(), "stdout write request initiation (%s)", out.type_name());

          // write to files
          for (auto &file : files)
          {
            uv::fs::write file_wr;
            file_wr.on_request() = write_cb< uv::fs::write >;
            file_wr.run(file, _buf, _offset);

            if (file_wr)
              all_write_queues_size += _buf.len();
            else
              PRINT_UV_ERR(file_wr.uv_status(), "file write request initiation (%s)", file.path());
          }

          int ret = in.read_pause(all_write_queues_size >= WRITE_QUEUE_SIZE_UPPER_LIMIT);
          DEBUG_LOG(ret == 0, "[read paused]: all_write_queues_size=%zu\n", all_write_queues_size);
        }
      }
  );

  return uv::loop::Default().run(UV_RUN_DEFAULT);
}



uv::buffer alloc_cb(uv::handle, std::size_t)
{
  constexpr std::size_t default_size = 8192;
  static std::vector< uv::buffer > buf_pool;

  for (std::size_t i = 0; i < buf_pool.size(); ++i)  if (buf_pool[i].nrefs() == 1)  // a spare item
  {
      buf_pool[i].len() = default_size;  // restore the buffer capacity size

      DEBUG_LOG(true, "[buffer pool]: item #%zu of %zu\n", i+1, buf_pool.size());
      return buf_pool[i];
  }

  buf_pool.emplace_back(uv::buffer{ default_size });

  DEBUG_LOG(true, "[buffer pool]: new item #%zu\n", buf_pool.size());
  return buf_pool.back();
}


template< class _WriteReq_ >
void write_cb(_WriteReq_ _wr, uv::buffer _buf)
{
  if (!_wr)  PRINT_UV_ERR(_wr.uv_status(),
      "%s write (%s)",
      _wr.handle().type() == UV_FILE ? "file" : "stdout",
      _wr.handle().type() == UV_FILE ? static_cast< uv::file&& >(_wr.handle()).path() : _wr.handle().type_name()
  );

  all_write_queues_size -= _buf.len();

  int ret = in.read_resume(all_write_queues_size <= WRITE_QUEUE_SIZE_LOWER_LIMIT);
  DEBUG_LOG(ret == 0, "[read resumed]: all_write_queues_size=%zu\n", all_write_queues_size);
}

