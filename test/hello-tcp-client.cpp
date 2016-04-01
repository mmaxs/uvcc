

#include "uvcc.hpp"
#include "uvcc/sockaddr.hpp"
#include <cstdio>

#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wwrite-strings"


#define PRINT_UV_ERR(prefix, code)  do {\
    fflush(stdout);\
    fprintf(stderr, "%s: %s (%i): %s\n", prefix, uv_err_name(code), (int)(code), uv_strerror(code));\
    fflush(stderr);\
} while (0)



int main(int _argc, char *_argv[])
{
  uv::tcp client(uv::loop::Default());

  uv::buffer greeting;
  greeting.base() = "client: Hello from uvcc!\n";  // static memory
  greeting.len() = strlen(greeting.base());

  sockaddr_in server_addr;
  uv::reset(server_addr);
  server_addr.sin_port = uv::hton16(80);
  server_addr.sin_addr.s_addr = uv::hton32(0x7F000001);

  uv::connect conn;
  conn.on_request() = [&greeting](uv::connect _conn)
  {
      if (!_conn)  { PRINT_UV_ERR("connect", _conn.uv_status()); return; };

      uv::tcp client = static_cast< uv::tcp&& >(_conn.handle());

      client.read_start(
          [](uv::handle, std::size_t _suggested_size)  { return uv::buffer{_suggested_size}; },
          [](uv::stream _stream, ssize_t _nread, uv::buffer _buf)
          {
            if (_nread < 0)
            {
              _stream.read_stop();
              PRINT_UV_ERR("read", _nread);
            }
            else if (_nread > 0)
            {
              fwrite(_buf.base(), 1, _nread, stdout);
              fflush(stdout);
            };
          }
      );
      if (!client)  { PRINT_UV_ERR("read_start", client.uv_status()); return; };

      uv::write wr;
      wr.on_request() = [](uv::write _wr)  { if (!_wr)  PRINT_UV_ERR("write", _wr.uv_status()); };
      wr.run(client, greeting);

      uv::shutdown shut_wr;
      shut_wr.on_request() = [](uv::shutdown _shut_wr)  { if (!_shut_wr)  PRINT_UV_ERR("shutdown", _shut_wr.uv_status()); };
      shut_wr.run(client);
  };
  conn.run(client, server_addr);

  return uv::loop::Default().run(UV_RUN_DEFAULT);
}
