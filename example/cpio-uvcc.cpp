
#include "uvcc.hpp"
#include <cstdio>


#define PRINT_UV_ERR(code, prefix, ...)  do {\
  fflush(stdout);\
  fprintf(stderr, (prefix), ##__VA_ARGS__);\
  fprintf(stderr, ": %s (%i): %s\n", ::uv_err_name(code), (int)(code), ::uv_strerror(code));\
  fflush(stderr);\
} while (0)


uv::pipe in(uv::loop::Default(), fileno(stdin)),
         out(uv::loop::Default(), fileno(stdout));

constexpr std::size_t WRITE_QUEUE_SIZE_UPPER_LIMIT = 500*1024*1024,
                      WRITE_QUEUE_SIZE_LOWER_LIMIT =  10*1024*1024;


void read_cb(uv::io, ssize_t, uv::buffer, int64_t, void*);
void write_cb(uv::write, uv::buffer);


int main(int _argc, char *_argv[])
{
  if (!in)
  {
    PRINT_UV_ERR(in.uv_status(), "stdin open");
    return in.uv_status();
  }
  if (!out)
  {
    PRINT_UV_ERR(out.uv_status(), "stdout open");
    return out.uv_status();
  }

  in.read_start(
      [](uv::handle, std::size_t _suggested_size){ return uv::buffer{ _suggested_size }; },
      read_cb
  );

  return uv::loop::Default().run(UV_RUN_DEFAULT);
}



void read_cb(uv::io _io, ssize_t _nread, uv::buffer _buffer, int64_t, void*)
{
  if (_nread < 0)
  {
    _io.read_stop();
    if (_nread != UV_EOF)  PRINT_UV_ERR(_nread, "read");
  }
  else if (_nread > 0)
  {
    _buffer.len() = _nread;

    uv::write wr;
    wr.on_request() = write_cb;
    wr.run(out, _buffer);

    in.read_pause(out.write_queue_size() >= WRITE_QUEUE_SIZE_UPPER_LIMIT);
  }
}


void write_cb(uv::write _wr, uv::buffer)
{
  if (!_wr)
  {
    PRINT_UV_ERR(_wr.uv_status(), "write");
    in.read_stop();
  }

  in.read_resume(out.write_queue_size() <= WRITE_QUEUE_SIZE_LOWER_LIMIT);
}
