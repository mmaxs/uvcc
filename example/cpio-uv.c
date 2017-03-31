
#include <uv.h>
#include <stdlib.h>
#include <stdio.h>

#ifndef _WIN32
#include <signal.h>
sighandler_t sigpipe_handler = signal(SIGPIPE, SIG_IGN);  // ignore SIGPIPE
#endif


#define PRINT_UV_ERR(code, printf_args...)  do {\
  fflush(stdout);\
  fprintf(stderr, "" printf_args);\
  fprintf(stderr, ": %s (%i): %s\n", uv_err_name(code), (int)(code), uv_strerror(code));\
  fflush(stderr);\
} while (0)


/* assume that stdin and stdout can be handled with as pipes */
uv_pipe_t in, out;


/* define queue size limits and transfer control states */
const size_t BUFFER_SIZE = 8192;
const size_t WRITE_QUEUE_SIZE_UPPER_LIMIT = 14*BUFFER_SIZE,
             WRITE_QUEUE_SIZE_LOWER_LIMIT =  2*BUFFER_SIZE;
enum { RD_UNKNOWN, RD_STOP, RD_PAUSE, RD_START } rdcmd_state = RD_UNKNOWN;

int wr_err_reported = 0;


/* forward declarations for callback functions */
void alloc_cb(uv_handle_t*, size_t, uv_buf_t*);
void read_cb(uv_stream_t*, ssize_t, const uv_buf_t*);
void write_cb(uv_write_t*, int);


int main(int _argc, char *_argv[])
{
  int ret = 0;

  uv_loop_t *loop = uv_default_loop();

  uv_pipe_init(loop, &in, 0);
  ret = uv_pipe_open(&in, fileno(stdin));
  if (ret < 0)
  {
    PRINT_UV_ERR(ret, "stdin open");
    return ret;
  }

  uv_pipe_init(loop, &out, 0);
  ret = uv_pipe_open(&out, fileno(stdout));
  if (ret < 0)
  {
    PRINT_UV_ERR(ret, "stdout open");
    return ret;
  }

  rdcmd_state = RD_START;
  ret = uv_read_start((uv_stream_t*)&in, alloc_cb, read_cb);
  if (ret < 0)
  {
    PRINT_UV_ERR(ret, "read initiation");
    return ret;
  }

  return uv_run(loop, UV_RUN_DEFAULT);
}



void alloc_cb(uv_handle_t *_handle, size_t _suggested_size, uv_buf_t *_buf)
{
  /* allocate the memory for a new I/O buffer */
  *_buf = uv_buf_init((char*)malloc(BUFFER_SIZE), BUFFER_SIZE);
}


void read_cb(uv_stream_t *_stream, ssize_t _nread, const uv_buf_t *_buf)
{
  if (_nread < 0)
  {
    if (_nread != UV_EOF)  PRINT_UV_ERR(_nread, "read");
    uv_read_stop(_stream);
  }
  else if (_nread > 0)
  {
    /* initialize a new buffer descriptor specifying the actual data length */
    uv_buf_t buf = uv_buf_init(_buf->base, _nread);

    /* create a write request descriptor; see note [1] */
    uv_write_t *wr = (uv_write_t*)malloc(sizeof(uv_write_t));

    /* save a reference to the I/O buffer somehow along with the write request; see note [2] */
    wr->data = _buf->base;

    /* fire up the write request */
    int ret = uv_write(wr, (uv_stream_t*)&out, &buf, 1, write_cb);
    /* the I/O buffer being used up should be deleted somewhere after the request has completed;
       see note [3] */
    if (ret < 0)
    {
      PRINT_UV_ERR(ret, "write initiation");
      rdcmd_state = RD_STOP;
      uv_read_stop((uv_stream_t*)&in);
    }

    /* stop reading from stdin when a consumer of stdout does not keep up with our transferring rate
       and stdout write queue size has grown up significantly; see note [4] */
    if (rdcmd_state == RD_START && out.write_queue_size >= WRITE_QUEUE_SIZE_UPPER_LIMIT)
    {
      rdcmd_state = RD_PAUSE;
      uv_read_stop((uv_stream_t*)&in);
    }
  }
}


void write_cb(uv_write_t *_wr, int _status)
{
  if (_status < 0)
  {
    if (!wr_err_reported)
    {
      /* report only the very first occurrence of the failure */
      PRINT_UV_ERR(_status, "write");
      wr_err_reported = 1;
    }

    rdcmd_state = RD_STOP;
    uv_read_stop((uv_stream_t*)&in);
  }
  else
  {
    /* resume stdin to stdout transferring when stdout output queue has gone away; see note [4] */
    if (rdcmd_state == RD_PAUSE && out.write_queue_size <= WRITE_QUEUE_SIZE_LOWER_LIMIT)
    {
      rdcmd_state = RD_START;
      uv_read_start((uv_stream_t*)&in, alloc_cb, read_cb);
    }
  }

  /* when the write request has completed it's safe to free up the memory allocated for the I/O buffer;
     see notes [2][3] */
  free(_wr->data);
  /* delete the write request descriptor */
  free(_wr);
}
