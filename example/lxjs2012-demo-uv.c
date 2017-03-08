
/* node.js equivalent stuff:
 * require('net').connect(80, 'www.nyan.cat').pipe(process.stdout);
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <uv.h>


void after_getaddrinfo(uv_getaddrinfo_t*, int, struct addrinfo*);
void after_connect(uv_connect_t*, int);
void after_write(uv_write_t*, int);
void on_alloc(uv_handle_t*, size_t, uv_buf_t*);
void on_read(uv_stream_t*, ssize_t, const uv_buf_t*);
void on_close(uv_handle_t*);


int main(int _argc, char *_argv[])
{
  uv_getaddrinfo_t* gai_req = (uv_getaddrinfo_t*)malloc(sizeof(uv_getaddrinfo_t));

  uv_getaddrinfo(
      uv_default_loop(),
      gai_req,
      after_getaddrinfo,
      "www.nyan.cat",
      "80",
      NULL
  );

  uv_run(uv_default_loop(), UV_RUN_DEFAULT);

  return 0;
}



void after_getaddrinfo(uv_getaddrinfo_t *_gai_req, int _status, struct addrinfo *_ai)
{
  uv_tcp_t *tcp_handle;
  uv_connect_t *connect_req;

  if (_status < 0)  abort();  /* handle the error */

  tcp_handle = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
  uv_tcp_init(uv_default_loop(), tcp_handle);

  connect_req = (uv_connect_t*)malloc(sizeof(uv_connect_t));
  uv_tcp_connect(
      connect_req,
      tcp_handle,
      _ai->ai_addr,
      after_connect
  );

  free(_gai_req);
  uv_freeaddrinfo(_ai);
}


void after_connect(uv_connect_t *_connect_req, int _status)
{
  uv_write_t *write_req;
  uv_buf_t buf;

  if (_status < 0)  abort();  /* handle the error */

  write_req = (uv_write_t*)malloc(sizeof(uv_write_t));

  buf.base = (char*)"GET / HTTP/1.0\r\n"
                    "Host: www.nyan.cat\r\n"
                    "\r\n";
  buf.len = strlen(buf.base);

  uv_write(
      write_req,
      _connect_req->handle,
      &buf,
      1,
      after_write
  );

  uv_read_start(
      _connect_req->handle,
      on_alloc,
      on_read
  );

  free(_connect_req);
}


void after_write(uv_write_t *_write_req, int _status)
{
  if (_status < 0)  abort();  /* handle the error */
  free(_write_req);
}


void on_alloc(uv_handle_t *_handle, size_t _suggested_size, uv_buf_t* _buf)
{
  _buf->base = (char*)malloc(_suggested_size);
  _buf->len = _suggested_size;
}


void on_read(uv_stream_t *_tcp_handle, ssize_t _nread, const uv_buf_t *_buf)
{
  if (_nread < 0)
  { /* error or EOF */
    if (_nread == UV_EOF)
    { /* no more data; close the connection */
      uv_close(
          (uv_handle_t*) _tcp_handle,
          on_close
      );
    }
    else
    { /* that's an error */
      abort();
    }
  }

  if (_nread > 0)
  { /* print it! FTW!!! */
    fwrite(_buf->base, 1, _nread, stdout);
  }

  free(_buf->base);
}


void on_close(uv_handle_t *_handle)
{
  free(_handle);
}

