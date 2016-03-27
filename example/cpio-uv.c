
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
  /* allocate the memory for a new I/O buffer */
  *_buf = uv_buf_init((char*)malloc(_suggested_size), _suggested_size);
}


void read_cb(uv_stream_t *_stream, ssize_t _nread, const uv_buf_t *_buf)
{
  if (_nread == UV_EOF)
    uv_read_stop(_stream);
  else if (_nread < 0)
    PRINT_UV_ERR("read", _nread);
  else if (_nread > 0)
  {
    /* initialize a new buffer descriptor specifying the actual data length */
    uv_buf_t buf = uv_buf_init(_buf->base, _nread);

    /* create a write request descriptor; see note [1] */
    uv_write_t *wr = (uv_write_t*)malloc(sizeof(uv_write_t));

    /* save a reference to the output buffer somehow along with the write request; see note [2] */
    wr->data = _buf->base;

    /* fire up the write request */
    uv_write(wr, (uv_stream_t*)&out, &buf, 1, write_cb);

    /* the I/O buffer being used up should be deleted somewhere; see note [3] */
  }
}


void write_cb(uv_write_t *_wr, int _o)
{
  if (_o < 0)
  {
    PRINT_UV_ERR("write", _o);
    uv_read_stop((uv_stream_t*)&in);
  };

  /* when the write request has completed it's safe to free up the memory allocated for the I/O buffer;
     see notes [2][3] */
  free(_wr->data);

  /* delete the write request descriptor */
  free(_wr);
}

