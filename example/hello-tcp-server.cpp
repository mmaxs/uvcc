
#include "uvcc.hpp"
#include <cstdio>


#define PRINT_UV_ERR(prefix, code)  do {\
    fflush(stdout);\
    fprintf(stderr, "%s: %s (%i): %s\n", prefix, ::uv_err_name(code), (int)(code), ::uv_strerror(code));\
    fflush(stderr);\
} while (0)


int main(int _argc, char *_argv[])
{
  // obtaing a listening address:port and initialize a sockaddr structure
  sockaddr_storage listen_addr;

  const char *ip   = _argc > 1 ? _argv[1] : "127.0.0.1";  // or "::1" for IPv6 loopback address
  const char *port = _argc > 2 ? _argv[2] : "54321";

  int status = uv::init(listen_addr, ip, port);  // IP agnostic address initialization
  if (status != 0)
  {
    PRINT_UV_ERR("address init", status);
    return status;
  }
  /* here is the example for direct IPv4 address initialization:
        uv::init(listen_addr, AF_INET);
        reinterpret_cast< sockaddr_in& >(listen_addr).sin_port = uv::hton16(54321);
        reinterpret_cast< sockaddr_in& >(listen_addr).sin_addr.s_addr = uv::hton32(0x7F000001);  // 127.0.0.1
   */

  // initialize a tcp socket and bind it to the desired address::port
  uv::tcp server(uv::loop::Default(), listen_addr.ss_family);
  server.bind(listen_addr);
  if (!server)
  {
    PRINT_UV_ERR("socket bind", server.uv_status());
    return server.uv_status();
  }

  // fill in the buffer with greeting message
  uv::buffer greeting;
  greeting.base() = const_cast< char* >("server: Hello from uvcc!\n");
  greeting.len() = strlen(greeting.base());

  // start listening for and accepting incoming connections
  server.listen(5,
      [&greeting](uv::stream _server)
      {
        if (!_server)
        {
          PRINT_UV_ERR("incoming connection", _server.uv_status());
          return;
        }
        auto client = static_cast< uv::tcp&& >(_server.accept());
        if (!client)  PRINT_UV_ERR("accept", client.uv_status());

        // dispatch to send a greeting
        uv::write wr;
        wr.on_request() = [](uv::write _wr, uv::buffer){ if (!_wr)  PRINT_UV_ERR("write", _wr.uv_status()); };
        wr.run(client, greeting);

        // shutdown the write side of the connection
        uv::shutdown shut_wr;
        shut_wr.run(client);

        // start reading from the remote peer
        client.read_start(
            // a buffer allocation callback
            [](uv::handle, std::size_t _suggested_size){ return uv::buffer{ _suggested_size }; },
            // a read callback
            [](uv::io _io, ssize_t _nread, uv::buffer _buf, int64_t, void*)
            {
              if (_nread < 0)
              {
                _io.read_stop();
                if (_nread != UV_EOF)  PRINT_UV_ERR("read", _nread);
              }
              else if (_nread > 0)
              {
                // print received data
                fwrite(_buf.base(), 1, _nread, stdout);
                fflush(stdout);
              }
            }
        );
        if (!client)  PRINT_UV_ERR("read start", client.uv_status());
      }
  );
  if (!server)
  {
    PRINT_UV_ERR("listen", server.uv_status());
    return server.uv_status();
  }

  return uv::loop::Default().run(UV_RUN_DEFAULT);
}
