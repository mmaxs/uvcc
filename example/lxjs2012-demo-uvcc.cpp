
/* node.js equivalent stuff:
 * require('net').connect(80, 'www.nyan.cat').pipe(process.stdout);
 */

#include "uvcc.hpp"
#include <cstring>  // strlen()
#include <cstdio>


#define PRINT_UV_ERR(code, printf_args...)  do {\
  fflush(stdout);\
  fprintf(stderr, "" printf_args);\
  fprintf(stderr, ": %s (%i): %s\n", ::uv_err_name(code), (int)(code), ::uv_strerror(code));\
  fflush(stderr);\
} while (0)


void connect_cb(uv::connect);


int main(int _argc, char *_argv[])
{
  // a getaddrinfo DNS request
  uv::getaddrinfo gai_req;
  // set a getaddrinfo request callback function that will be called after the DNS resolving request complete
  gai_req.on_request() = [](uv::getaddrinfo _gai_req)
  {
    if (!_gai_req)
    {
      PRINT_UV_ERR(_gai_req.uv_status(), "getaddrinfo");
      return;
    }

    // a tcp handle
    uv::tcp tcp_handle(uv::loop::Default());

    // a connect request
    uv::connect connect_req;
    // set a connect request callback function that will be called when the connection is established
    connect_req.on_request() = connect_cb;
    // run the connect request on the tcp handle to connect to the resolved peer
    connect_req.run(tcp_handle, *_gai_req.addrinfo()->ai_addr);
  };

  // run the getaddrinfo request
  gai_req.run(uv::loop::Default(), "www.nyan.cat", "80");

  return uv::loop::Default().run(UV_RUN_DEFAULT);
}



void connect_cb(uv::connect _connect_req)
{
  if (!_connect_req)
  {
    PRINT_UV_ERR(_connect_req.uv_status(), "connect");
    return;
  }

  // get the tcp handle this connect request has performed on
  uv::tcp tcp_handle = static_cast< uv::tcp&& >(_connect_req.handle());

  // fill in a buffer with HTTP request data
  uv::buffer buf;
  buf.base() = const_cast< char* >(
      "GET / HTTP/1.0\r\n"
      "Host: www.nyan.cat\r\n"
      "\r\n"
  );
  buf.len() = std::strlen(buf.base());

  // write the HTTP request to the tcp stream
  uv::write wr;
  wr.on_request() = [](uv::write _wr, uv::buffer){ if (!_wr)  PRINT_UV_ERR(_wr.uv_status(), "write"); };
  wr.run(tcp_handle, buf);

  // start reading from the tcp stream
  tcp_handle.read_start(
      // the callback function that will be called when a new input buffer get needed to be allocated for data read
      [](uv::handle, std::size_t _suggested_size){ return uv::buffer{ _suggested_size }; },
      // the callback function that will be called when data has been read from the stream
      [](uv::io _io, ssize_t _nread, uv::buffer _buf, int64_t, void*)
      {
        if (_nread < 0)
        {
          _io.read_stop();
          if (_nread != UV_EOF)  PRINT_UV_ERR(_nread, "read");
        }
        else if (_nread > 0)
        {
          // print the received data
          fwrite(_buf.base(), 1, _nread, stdout);
          fflush(stdout);
        }
      }
  );
}
