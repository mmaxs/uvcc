
#include "uvcc.hpp"
#include <cstdio>
#include <vector>
#include <fcntl.h>  // O_*


#define PRINT_UV_ERR(prefix, code)  do {\
    fflush(stdout);\
    fprintf(stderr, "%s: %s (%i): %s\n", prefix, uv_err_name(code), (int)(code), uv_strerror(code));\
    fflush(stderr);\
} while (0)

#ifndef NDEBUG
#define DEBUG_LOG(format, ...)  do {\
    fflush(stdout);\
    fprintf(stderr, format, ##__VA_ARGS__);\
    fflush(stderr);\
} while (0)
#else
#define DEBUG_LOG(...)
#endif


uv::io in = uv::io::guess_handle(uv::loop::Default(), fileno(stdin)),
       out = uv::io::guess_handle(uv::loop::Default(), fileno(stdout));

constexpr const std::size_t WRITE_QUEUE_SIZE_UPPER_LIMIT =  4*8192,
                            WRITE_QUEUE_SIZE_LOWER_LIMIT =  2*8192;
std::size_t all_write_queues_size = 0;

std::vector< uv::file > files;


int main(int _argc, char *_argv[])
{

  if (!in)
  {
    fprintf(stderr, "%s ", in.type_name());
    PRINT_UV_ERR("stdin open", in.uv_status());
    return in.uv_status();
  }
  if (!out)
  {
    fprintf(stderr, "%s ", out.type_name());
    PRINT_UV_ERR("stdout open", out.uv_status());
    return out.uv_status();
  }

  #ifdef _WIN32
  constexpr const int mode = _S_IREAD|_S_IWRITE;
  #else
  constexpr const int mode = S_IRWXU|S_IRWXG|S_IRWXO;
  #endif
  for (int i = 1; i < _argc; ++i)
  {
    uv::file f(uv::loop::Default(), _argv[i], O_CREAT|O_TRUNC|O_WRONLY, mode);
    if (f)
      files.emplace_back(std::move(f));
    else
      PRINT_UV_ERR(f.path(), f.uv_status());
  }

  in.read_start(
      [](uv::handle, std::size_t) -> uv::buffer
      {
        constexpr const std::size_t default_size = 8192;
        static std::vector< uv::buffer > buf_pool;

        for (std::size_t i = 0; i < buf_pool.size(); ++i)  if (buf_pool[i].nrefs() == 1)  {
            DEBUG_LOG("[buffer pool]: item #%zu of %zu\n", i+1, buf_pool.size());
            buf_pool[i].len() = default_size;
            return buf_pool[i];
        }

        buf_pool.emplace_back(uv::buffer{default_size});

        DEBUG_LOG("[buffer pool]: new item #%zu\n", buf_pool.size());
        return buf_pool.back();
      },
      [](uv::io _io, ssize_t _nread, uv::buffer _buf, int64_t _offset, void *_info) -> void
      {
        if (_nread < 0)
        {
          _io.read_stop();
          if (_nread != UV_EOF)  PRINT_UV_ERR("read", _nread);
        }
        else if (_nread > 0)
        {
          _buf.len() = _nread;

          uv::output io_wr;
          io_wr.on_request() = [](uv::output _io_wr, uv::buffer _buf) -> void
          {
            if (!_io_wr)
            {
              PRINT_UV_ERR("output", _io_wr.uv_status());
              in.read_stop();
            }

            all_write_queues_size -= _buf.len();
            int ret = in.read_resume(all_write_queues_size <= WRITE_QUEUE_SIZE_LOWER_LIMIT);
            if (ret == 0)  DEBUG_LOG("[read resumed]: all_write_queues_size=%zu\n", all_write_queues_size);
          };

          io_wr.run(out, _buf, _offset, _info);  // UDP connected sockets are not supported by libuv
          if (io_wr)
            all_write_queues_size += _buf.len();
          else
            PRINT_UV_ERR("output::run", io_wr.uv_status());

          for (auto &file : files)
          {
            uv::fs::write file_wr;
            file_wr.on_request() = [](uv::fs::write _file_wr, uv::buffer _buf) -> void
            {
              if (!_file_wr)  PRINT_UV_ERR("fs::write", _file_wr.uv_status());

              all_write_queues_size -= _buf.len();
              int ret = in.read_resume(all_write_queues_size <= WRITE_QUEUE_SIZE_LOWER_LIMIT);
              if (ret == 0)  DEBUG_LOG("[read resumed]: all_write_queues_size=%zu\n", all_write_queues_size);
            };

            file_wr.run(file, _buf, _offset);
            if (file_wr)
              all_write_queues_size += _buf.len();
            else
              PRINT_UV_ERR("fs::write::run", file_wr.uv_status());
          }

          int ret = in.read_pause(all_write_queues_size >= WRITE_QUEUE_SIZE_UPPER_LIMIT);
          if (ret == 0)  DEBUG_LOG("[read paused]: all_write_queues_size=%zu\n", all_write_queues_size);
        }
      }
  );

  return uv::loop::Default().run(UV_RUN_DEFAULT);
}
