

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
  uv::tcp server(uv::loop::Default());

  uv::buffer greeting;
  greeting.base() = "server: Hello from uvcc!\n";
  greeting.len() = strlen(greeting.base());

  sockaddr_in listen_addr;
  uv::reset(listen_addr);
  listen_addr.sin_port = uv::hton16(80);
  listen_addr.sin_addr.s_addr = uv::hton32(0x7F000001);
  server.bind(listen_addr, 0);

  server.listen(5, [&greeting](uv::stream _server)
      {
        if (!_server)  { PRINT_UV_ERR("connection", _server.uv_status()); return; };
        
        uv::tcp client = _server.accept< uv::tcp >();
        if (!client)  { PRINT_UV_ERR("accept", client.uv_status()); return; };

        uv::write wr;
        wr.on_request() = [](uv::write _wr)  { if (!_wr)  PRINT_UV_ERR("write", _wr.uv_status()); };
        wr.run(client, greeting);

        uv::shutdown shut_wr;
        shut_wr.on_request() = [](uv::shutdown _shut_wr)  { if (!_shut_wr)  PRINT_UV_ERR("shutdown", _shut_wr.uv_status()); };
        shut_wr.run(client);
      }
  );

  return uv::loop::Default().run(UV_RUN_DEFAULT);
}
