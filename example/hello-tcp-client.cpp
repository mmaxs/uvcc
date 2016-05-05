
#include "uvcc.hpp"
#include <cstdio>


#define PRINT_UV_ERR(prefix, code)  do {\
    fflush(stdout);\
    fprintf(stderr, "%s: %s (%i): %s\n", prefix, uv_err_name(code), (int)(code), uv_strerror(code));\
    fflush(stderr);\
} while (0)



int main(int _argc, char *_argv[])
{
  uv::buffer greeting;
  greeting.base() = const_cast< char* >(
      "client: Hello from uvcc!\n"
  );
  greeting.len() = strlen(greeting.base());

  uv::connect conn;
  conn.on_request() = [&greeting](uv::connect _conn)
  {
    if (!_conn)  { PRINT_UV_ERR("connect", _conn.uv_status()); return; };

    auto client = static_cast< uv::tcp&& >(_conn.handle());

    client.read_start(
        [](uv::handle, std::size_t _suggested_size)  { return uv::buffer{_suggested_size}; },
        [](uv::io _io, ssize_t _nread, uv::buffer _buf)
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
    if (!client)  { PRINT_UV_ERR("read_start", client.uv_status()); return; };

    uv::write wr;
    wr.on_request() = [](uv::write _wr, uv::buffer)  { if (!_wr)  PRINT_UV_ERR("write", _wr.uv_status()); };
    wr.run(client, greeting);

    uv::shutdown shut_wr;
    shut_wr.run(client);
  };

  sockaddr_storage server_addr;
  const char *ip = _argc > 1 ? _argv[1] : "127.0.0.1";
  const char *port = _argc > 2 ? _argv[2] : "54321";
  int status = uv::init(server_addr, ip, port);
  if (status != 0)  { PRINT_UV_ERR("init", status); return status; };

  uv::tcp client(uv::loop::Default(), reinterpret_cast< sockaddr& >(server_addr).sa_family);
  if (!client)  { PRINT_UV_ERR("client", client.uv_status()); return client.uv_status(); };

  conn.run(client, server_addr);

  return uv::loop::Default().run(UV_RUN_DEFAULT);
}

