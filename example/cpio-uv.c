
#include <uv.h>
#include <stdlib.h>
#include <stdio.h>


#define PRINT_UV_ERR(prefix, code)  do {\
    fflush(stdout);\
    fprintf(stderr, "%s: %s (%i): %s\n", prefix, uv_err_name(code), (int)(code), uv_strerror(code));\
    fflush(stderr);\
} while (0)



uv_pipe_t in, out;


void alloc_cb(uv_handle_t*, size_t, uv_buf_t*);
void read_cb(uv_stream_t*, ssize_t, const uv_buf_t*);
void write_cb(uv_write_t*, int);



/* ["translation noise"](https://books.google.ru/books?id=Uemuaza3fTEC&pg=PT26&dq=%22translation+noise%22&hl=en&sa=X&ved=0ahUKEwigoJ3Dq8bLAhVoc3IKHQGQCFYQ6AEIGTAA) */
int main(int _argc, char *_argv[])
{
  int o = 0;

  uv_loop_t *loop = uv_default_loop();

  uv_pipe_init(loop, &in, 0);
  o = uv_pipe_open(&in, fileno(stdin));
  if (o < 0)  {
    PRINT_UV_ERR("stdin open", o);
    return o;
  }

  uv_pipe_init(loop, &out, 0);
  o = uv_pipe_open(&out, fileno(stdout));
  if (o < 0)  {
    PRINT_UV_ERR("stdout open", o);
    return o;
  }

  uv_read_start((uv_stream_t*)&in, alloc_cb, read_cb);

  return uv_run(loop, UV_RUN_DEFAULT);
}



void alloc_cb(uv_handle_t*, size_t _suggested_size, uv_buf_t *_buf)
{
  *_buf = uv_buf_init((char*)malloc(_suggested_size), _suggested_size);
}


void read_cb(uv_stream_t *_stream, ssize_t _nread, const uv_buf_t *_buf)
{
  if (_nread == UV_EOF)
    uv_read_stop(_stream);
  else if (_nread < 0)
    PRINT_UV_ERR("read", _nread);
  else if (_nread >0)
  {
    uv_buf_t buf = uv_buf_init(_buf->base, _nread);
    uv_write_t *wr = (uv_write_t*)malloc(sizeof(uv_write_t));  /* should be allocated on the heap, \see [libuv + C++ segfaults](http://stackoverflow.com/questions/29319392/libuv-c-segfaults) */
    wr->data = _buf->base;
    uv_write(wr, (uv_stream_t*)&out, &buf, 1, write_cb);
    /* free(_buf->base);
       \see [Lifetime of buffers on uv_write (#344)](https://github.com/joyent/libuv/issues/344) */
  };
}


/* [Preserve uv_write_t->bufs in uv_write() (#1059)](https://github.com/joyent/libuv/issues/1059)
   [Please expose bufs in uv_fs_t's result for uv_fs_read operations. (#1557)](https://github.com/joyent/libuv/issues/1557) */
void write_cb(uv_write_t *_wr, int _o)
{
  if (_o < 0)  PRINT_UV_ERR("write", _o);
  free(_wr->data);
  free(_wr);
}

