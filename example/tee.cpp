
#include "uvcc.hpp"
#include <cstdio>
#include <vector>
#include <fcntl.h>  // O_*


#define PRINT_UV_ERR(prefix, code)  do {\
    fflush(stdout);\
    fprintf(stderr, "%s: %s (%i): %s\n", prefix, uv_err_name(code), (int)(code), uv_strerror(code));\
    fflush(stderr);\
} while (0)



uv::pipe in(uv::loop::Default(), fileno(stdin)),
         out(uv::loop::Default(), fileno(stdout));

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
    uv::file f(_argv[i], O_CREAT|O_TRUNC|O_WRONLY, mode);
    if (f)
      files.emplace_back(std::move(f));
    else
      PRINT_UV_ERR(f.path(), f.uv_status());
  }

  in.read_start(
      [](uv::handle, std::size_t) -> uv::buffer  // alloc_cb
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
      },
      [](uv::io _io, ssize_t _nread, uv::buffer _buf, void*) -> void  // read_cb
      {
        if (_nread < 0)
        {
          _io.read_stop();
          if (_nread != UV_EOF)  PRINT_UV_ERR("read", _nread);
        }
        else if (_nread > 0)
        {
          auto wr = []() -> uv::write
          {
            static std::vector< uv::write > wr_pool;

            #ifndef NDEBUG
            for (std::size_t i = 0; i < wr_pool.size(); ++i)  if (wr_pool[i].nrefs() == 1)  {
              fprintf(stderr, "[write request pool]: item #%zu of %zu\n", i+1, wr_pool.size());  fflush(stderr);  
              return wr_pool[i];
            };
            #else
            for (auto &wr : wr_pool)  if (wr.nrefs() == 1)  return wr;
            #endif

            wr_pool.emplace_back();
            wr_pool.back().on_request() = [](uv::write _wr, uv::buffer) -> void  // write_cb
            {
              if (!_wr)
              {
                PRINT_UV_ERR("write", _wr.uv_status());
                in.read_stop();
              };
            };

            #ifndef NDEBUG
            fprintf(stderr, "[write request pool]: new item #%zu\n", wr_pool.size());  fflush(stderr);
            #endif
            return wr_pool.back();
          };

          _buf.len() = _nread;
          wr().run(out, _buf);

          for (auto &f : files)
          {
            uv::fs::write fwr;
            fwr.on_request() = [](uv::fs::write _req, uv::buffer) -> void
            {
              if (!_req)  PRINT_UV_ERR("fs::write", _req.uv_status());
            };
            fwr.run(f, _buf);
          }
        };
      }
  );

  return uv::loop::Default().run(UV_RUN_DEFAULT);
}
