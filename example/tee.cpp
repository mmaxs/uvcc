
#include "uvcc.hpp"
#include <cstdio>
#include <vector>
#include <fcntl.h>  // O_*


#define PRINT_UV_ERR(prefix, code)  do {\
    fflush(stdout);\
    fprintf(stderr, "%s: %s (%i): %s\n", prefix, uv_err_name(code), (int)(code), uv_strerror(code));\
    fflush(stderr);\
} while (0)



uv::io in = uv::io::guess_handle(uv::loop::Default(), fileno(stdin)),
       out = uv::io::guess_handle(uv::loop::Default(), fileno(stdout));

constexpr const std::size_t WRITE_QUEUE_SIZE_UPPER_LIMIT = 500*1024*1024,
                            WRITE_QUEUE_SIZE_LOWER_LIMIT =  10*1024*1024;
std::size_t all_write_queues_size = 0;

std::vector< uv::file > files;


int main(int _argc, char *_argv[])
{

  if (!in)
  {
    PRINT_UV_ERR("stdin open", in.uv_status());
    return in.uv_status();
  };
  if (!out)
  {
    PRINT_UV_ERR("stdout open", out.uv_status());
    return out.uv_status();
  };

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
      /* [](uv::handle, std::size_t) -> uv::buffer
      {
        constexpr const std::size_t default_size = 8192;
        static std::vector< uv::buffer > buf_pool;

        #ifndef NDEBUG
        for (std::size_t i = 0; i < buf_pool.size(); ++i)  if (buf_pool[i].nrefs() == 1)  {
            fprintf(stderr, "[buffer pool]: item #%zu of %zu\n", i+1, buf_pool.size());  fflush(stderr);
            buf_pool[i].len() = default_size;
            return buf_pool[i];
        };
        #else
        for (auto &buf : buf_pool)  if (buf.nrefs() == 1)  {
            buf.len() = default_size;
            return buf;
        };
        #endif

        buf_pool.emplace_back(uv::buffer{default_size});

        #ifndef NDEBUG
        fprintf(stderr, "[buffer pool]: new item #%zu\n", buf_pool.size());  fflush(stderr);
        #endif
        return buf_pool.back();
      }, */
      [](uv::handle, std::size_t _suggested_size) -> uv::buffer  { return uv::buffer{_suggested_size}; },
      [](uv::io _io, ssize_t _nread, uv::buffer _buf, void*) -> void
      {
        if (_nread < 0)
        {
          _io.read_stop();
          if (_nread != UV_EOF)  PRINT_UV_ERR("read", _nread);
        }
        else if (_nread > 0)
        {
          _buf.len() = _nread;

          uv::output wr;
          wr.on_request() = [](uv::output _wr, uv::buffer _buf) -> void
          {
            if (!_wr)
            {
              PRINT_UV_ERR("output", _wr.uv_status());
              in.read_stop();
            };

            //all_write_queues_size -= _buf.len();
            //in.read_resume(all_write_queues_size <= WRITE_QUEUE_SIZE_LOWER_LIMIT);
          };
          wr.run(out, _buf);
          if (!wr)  PRINT_UV_ERR("output::run", wr.uv_status());

          //all_write_queues_size = out.write_queue_size();

          for (auto &file : files)
          {
            uv::fs::write file_wr;
            file_wr.on_request() = [](uv::fs::write _file_wr, uv::buffer _buf) -> void
            {
              if (!_file_wr)  PRINT_UV_ERR("fs::write", _file_wr.uv_status());

              //all_write_queues_size -= _buf.len();
              //in.read_resume(all_write_queues_size <= WRITE_QUEUE_SIZE_LOWER_LIMIT);
            };
            file_wr.run(file, _buf);

            //all_write_queues_size += file.write_queue_size();
          }

          //in.read_pause(all_write_queues_size >= WRITE_QUEUE_SIZE_UPPER_LIMIT);
        };
      }
  );

  return uv::loop::Default().run(UV_RUN_DEFAULT);
}
