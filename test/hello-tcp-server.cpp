

#include "uvcc.hpp"
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
  uv::buffer greeting;
  greeting.base() = const_cast< char* >(
      "server: Hello from uvcc!\n"
  );
  greeting.len() = strlen(greeting.base());

  sockaddr_storage listen_addr;
  // uv.init(server_addr, AF_INET);
  // reinterpret_cast< sockaddr_in& >(listen_addr).sin_port = uv::hton16(54321);
  // reinterpret_cast< sockaddr_in& >(listen_addr).sin_addr.s_addr = uv::hton32(0x7F000001);
  const char *ip = _argc > 1 ? _argv[1] : "127.0.0.1";
  const char *port = _argc > 2 ? _argv[2] : "54321";
  int status = uv::init(listen_addr, ip, port);
  if (status != 0)  { PRINT_UV_ERR("init", status); return status; };

  uv::tcp server(uv::loop::Default(), reinterpret_cast< sockaddr& >(listen_addr).sa_family);
  server.bind(listen_addr);
  if (!server)  { PRINT_UV_ERR("bind", server.uv_status()); return server.uv_status(); };

  server.listen(5, [&greeting](uv::stream _server)
      {
        if (!_server)  { PRINT_UV_ERR("connection", _server.uv_status()); return; };
        
        uv::tcp client = _server.accept< uv::tcp >();
        if (!client)  { PRINT_UV_ERR("accept", client.uv_status()); return; };

        client.read_start(
            [](uv::handle, std::size_t _suggested_size)  { return uv::buffer{_suggested_size}; },
            [](uv::stream _stream, ssize_t _nread, uv::buffer _buf)
            {
              if (_nread < 0)
              {
                _stream.read_stop();
                if (_nread != UV_EOF)  PRINT_UV_ERR("read", _nread);
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
      }
  );
  if (!server)  { PRINT_UV_ERR("listen", server.uv_status()); return server.uv_status(); };

  return uv::loop::Default().run(UV_RUN_DEFAULT);
}
