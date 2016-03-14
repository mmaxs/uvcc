

#include "uvcc.hpp"
#include <cstdio>

#pragma GCC diagnostic ignored "-Wunused-variable"


#define error(f, e)  {\
  fprintf(stderr, "f: %s (%i): %s\n", ::uv_err_name(e), e, ::uv_strerror(e));\
  fflush(stderr);\
}


void write_cb(uv_write_t *_req, int _status)  { if (_status < 0)  error(uv_write, _status); }

int test_uv()
{
  int o;

  uv_pipe_t out;

  if ((o = uv_pipe_init(uv_default_loop(), &out, 0)) < 0)  error(uv_pipe_init, o);
  if ((o = uv_pipe_open(&out, fileno(stdout))) < 0)  error(uv_pipe_open, o);

  uv_write_t wr;

  uv_buf_t buf = uv_buf_init(const_cast< char* >("Hello from libuv!\n"), 18);

  uv_write(&wr, reinterpret_cast< uv_stream_t* >(&out), &buf,  1, write_cb);

  return  uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}


int test_uvcc()
{
  uv::pipe out(uv::loop::Default(), fileno(stdout));

  uv::buffer buf;
  buf[0] = uv_buf_init(const_cast< char* >("Hello from uvcc!\n"), 17);

  uv::write wr;
  wr.on_request() = [](uv::write, int _status) -> void  { if (_status < 0)  error(write, _status); };
  wr.run(out, buf);

  return uv::loop::Default().run(UV_RUN_DEFAULT);
}


int main(int _argc, char *_argv[])
{
  fprintf(stderr, "...\n"); fflush(stderr);
  return test_uvcc();
}
