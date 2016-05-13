
/* node.js equivalent stuff:
 * require('net').connect(80, 'www.nyan.cat').pipe(process.stdout);
 */

#include "uvcc.hpp"
#include <cstring>  // strlen()
#include <cstdio>


#define PRINT_UV_ERR(prefix, code)  do {\
    fflush(stdout);\
    fprintf(stderr, "%s: %s (%i): %s\n", prefix, uv_err_name(code), (int)(code), uv_strerror(code));\
    fflush(stderr);\
} while (0)



int main(int _argc, char *_argv[])
{
  uv::getaddrinfo gai_req;
  gai_req.on_request() = [](uv::getaddrinfo _gai_req)
  {
    if (!_gai_req)  { PRINT_UV_ERR("getaddrinfo", _gai_req.uv_status()); return; };

    uv::connect connect_req;
    connect_req.on_request() = [](uv::connect _connect_req)
    {
      if (!_connect_req)  { PRINT_UV_ERR("connect", _connect_req.uv_status()); return; };

      uv::tcp tcp_handle = static_cast< uv::tcp&& >(_connect_req.handle());

      tcp_handle.read_start(
          [](uv::handle, std::size_t _suggested_size)  { return uv::buffer{_suggested_size}; },
          [](uv::io _io, ssize_t _nread, uv::buffer _buf, void*)
          {
            if (_nread < 0)
            {
              _io.read_stop();
              if (_nread != UV_EOF)  PRINT_UV_ERR("read", _nread);
            }
            else if (_nread > 0)
            {
              fwrite(_buf.base(), 1, _nread, stdout);
              fflush(stdout);
            };
          }
      );
      if (!tcp_handle)  { PRINT_UV_ERR("read_start", tcp_handle.uv_status()); return; };

      uv::buffer buf;
      buf.base() = const_cast< char* >(
          "GET / HTTP/1.0\r\n"
          "Host: www.nyan.cat\r\n"
          "\r\n"
      );
      buf.len() = std::strlen(buf.base());

      uv::write wr;
      wr.on_request() = [](uv::write _wr, uv::buffer)  { if (!_wr)  PRINT_UV_ERR("write", _wr.uv_status()); };
      wr.run(tcp_handle, buf);
    };

    uv::tcp tcp_handle(uv::loop::Default());

    connect_req.run(tcp_handle, *_gai_req.addrinfo()->ai_addr);
  };

  gai_req.run(uv::loop::Default(), "www.nyan.cat", "80");

  return uv::loop::Default().run(UV_RUN_DEFAULT);
}

