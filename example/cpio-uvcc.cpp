
#include "uvcc.hpp"
#include <cstdio>


#define PRINT_UV_ERR(prefix, code)  do {\
    fflush(stdout);\
    fprintf(stderr, "%s: %s (%i): %s\n", prefix, uv_err_name(code), (int)(code), uv_strerror(code));\
    fflush(stderr);\
} while (0)



uv::pipe in(uv::loop::Default(), fileno(stdin)),
         out(uv::loop::Default(), fileno(stdout));

constexpr const std::size_t WRITE_QUEUE_SIZE_UPPER_LIMIT = 500*1024*1024,
                            WRITE_QUEUE_SIZE_LOWER_LIMIT =  10*1024*1024;


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

  in.read_start(
      [](uv::handle, std::size_t _suggested_size) -> uv::buffer  // alloc_cb
      {
        return uv::buffer{_suggested_size};
      },
      [](uv::io _io, ssize_t _nread, uv::buffer _buffer, int64_t, void*) -> void  // read_cb
      {
        if (_nread < 0)
        {
          _io.read_stop();
          if (_nread != UV_EOF)  PRINT_UV_ERR("read", _nread);
        }
        else if (_nread > 0)
        {
          _buffer.len() = _nread;

          uv::write wr;
          wr.on_request() = [](uv::write _wr, uv::buffer) -> void  // write_cb
          {
            if (!_wr)
            {
              PRINT_UV_ERR("write", _wr.uv_status());
              in.read_stop();
            };

            in.read_resume(out.write_queue_size() <= WRITE_QUEUE_SIZE_LOWER_LIMIT);
          };
          wr.run(out, _buffer);

          in.read_pause(out.write_queue_size() >= WRITE_QUEUE_SIZE_UPPER_LIMIT);
        }
      }
  );

  return uv::loop::Default().run(UV_RUN_DEFAULT);
}
