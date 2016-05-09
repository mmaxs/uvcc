
#include "uvcc.hpp"
#include <cstdio>
#include <fcntl.h>  // O_*


#define PRINT_UV_ERR(prefix, code)  do {\
    fflush(stdout);\
    fprintf(stderr, "%s: %s (%i): %s\n", prefix, uv_err_name(code), (int)(code), uv_strerror(code));\
    fflush(stderr);\
} while (0)



uv::pipe out(uv::loop::Default(), fileno(stdout));


int main(int _argc, char *_argv[])
{

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
    uv::file f(_argv[i], O_RDONLY, mode);
    if (!f)
      PRINT_UV_ERR(f.path(), f.uv_status());
    else
    {
      f.read_start(
          [](uv::handle, std::size_t _suggested_size) -> uv::buffer  // alloc_cb
          {
            fprintf(stderr, "_suggested_size=%zu\n", _suggested_size); fflush(stderr);
            return uv::buffer{_suggested_size};
          },
          [](uv::io _io, ssize_t _nread, uv::buffer _buf, void *_offset) -> void  // read_cb
          {
            fprintf(stderr, "_nread=%zi _offset=%zi\n", _nread, *(ssize_t*)_offset); fflush(stderr);

            if (_nread < 0)
            {
              _io.read_stop();
              if (_nread != UV_EOF)  PRINT_UV_ERR("read", _nread);
            }
            else if (_nread > 0)
            {
              uv::write wr;
              wr.on_request() = [&_io](uv::write _wr, uv::buffer) -> void  // write_cb
              {
                if (!_wr)
                {
                  PRINT_UV_ERR("write", _wr.uv_status());
                  _io.read_stop();
                };
              };

              _buf.len() = _nread;
              wr.run(out, _buf);
            };
          },
          30, 0
      );
    }
  }

  return uv::loop::Default().run(UV_RUN_DEFAULT);
}
