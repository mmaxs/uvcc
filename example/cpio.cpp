
#include <uv.h>
#include <cstdio>


#define ERR(prefix, code)  do {\
    fflush(stdout);\
    fprintf(stderr, "%s: %s (%i): %s\n", prefix, uv_err_name(code), (int)code, uv_strerror(code));\
    fflush(stderr);\
} while (0)



uv_pipe_t in, out;


void alloc_cb(uv_handle_t*, size_t _suggested_size, uv_buf_t *_buf)
{
  *_buf = uv_buf_init(new char[_suggested_size], _suggested_size);
}

void write_cb(uv_write_t*, int);

void read_cb(uv_stream_t *_stream, ssize_t _nread, const uv_buf_t *_buf)
{
  if (_nread == UV_EOF)
    uv_read_stop(_stream);
  else if (_nread < 0)
    ERR("read", _nread);
  else if (_nread >0)
  {
    uv_buf_t buf = uv_buf_init(_buf->base, _nread);
    uv_write_t *wr = new uv_write_t;  // should be allocated on the heap, \see [libuv + C++ segfaults](http://stackoverflow.com/questions/29319392/libuv-c-segfaults)
    uv_write(wr, reinterpret_cast< uv_stream_t* >(&out), &buf, 1, write_cb);
    //delete _buf->base;  // \see [Lifetime of buffers on uv_write (#344)](https://github.com/joyent/libuv/issues/344)
  };
}

void write_cb(uv_write_t *_wr, int _o)
{
  if (_o < 0)  ERR("write", _o);
  delete _wr;
}


// ["translation noise"](https://books.google.ru/books?id=Uemuaza3fTEC&pg=PT26&dq=%22translation+noise%22&hl=en&sa=X&ved=0ahUKEwigoJ3Dq8bLAhVoc3IKHQGQCFYQ6AEIGTAA)
int main(int _argc, char *_argv[])
{
  int o = 0;

  uv_loop_t *loop = uv_default_loop();

  uv_pipe_init(loop, &in, 0);
  o = uv_pipe_open(&in, fileno(stdin));
  if (o < 0)  {
    ERR("stdin open", o);
    return o;
  }

  uv_pipe_init(loop, &out, 0);
  o = uv_pipe_open(&out, fileno(stdout));
  if (o < 0)  {
    ERR("stdout open", o);
    return o;
  }

  uv_read_start(reinterpret_cast< uv_stream_t* >(&in), alloc_cb, read_cb);

  return uv_run(loop, UV_RUN_DEFAULT);
}

