
#include "uvcc.hpp"
#ifdef UVCC_DEBUG
#include "uvcc/debug.hpp"
#endif

#include <cstdio>


#define PRINT_UV_ERR(code, printf_args...)  do {\
  fflush(stdout);\
  fprintf(stderr, "" printf_args);\
  fprintf(stderr, ": %s (%i): %s\n", ::uv_err_name(code), (int)(code), ::uv_strerror(code));\
  fflush(stderr);\
} while (0)


int main(int _argc, char *_argv[])
{
  // obtain server's address and initialize a sockaddr structure
  sockaddr_storage server_addr;

  const char *ip   = _argc > 1 ? _argv[1] : "127.0.0.1";
  const char *port = _argc > 2 ? _argv[2] : "54321";

  int status = uv::init(server_addr, ip, port);
  if (status != 0)
  {
    PRINT_UV_ERR(status, "ip address");
    return status;
  }

//{
  // initialize a tcp socket
  uv::tcp peer(uv::loop::Default(), server_addr.ss_family);
  if (!peer)
  {
    PRINT_UV_ERR(peer.uv_status(), "tcp socket");
    return peer.uv_status();
  }

  // fill in the buffer with greeting message
  uv::buffer greeting;
  greeting.base() = const_cast< char* >("client: Hello from uvcc!\n");
  greeting.len() = strlen(greeting.base());

  // a connect request
  uv::connect conn;
  conn.on_request() = [&greeting](uv::connect _conn)
  {
    if (!_conn)
    {
      PRINT_UV_ERR(_conn.uv_status(), "connect");
      return;
    }
    // a tcp stream for the connection
    auto peer = static_cast< uv::tcp&& >(_conn.handle());

    // dispatch to send a greeting
    uv::write wr;
    wr.on_request() = [](uv::write _wr, uv::buffer){ if (!_wr)  PRINT_UV_ERR(_wr.uv_status(), "write"); };
    wr.run(peer, greeting);
    if (!wr)  PRINT_UV_ERR(wr.uv_status(), "write initiation");

    // shutdown the write side of the connection
    uv::shutdown shut_wr;
    shut_wr.run(peer);

    // start reading from the remote peer
    peer.read_start(
        // a buffer allocation callback
        [](uv::handle, std::size_t _suggested_size){ return uv::buffer{ _suggested_size }; },
        // a read callback
        [](uv::io _io, ssize_t _nread, uv::buffer _buf, int64_t, void*)
        {
          if (_nread < 0)
          {
            _io.read_stop();
            if (_nread != UV_EOF)  PRINT_UV_ERR(_nread, "read");
          }
          else if (_nread > 0)
          {
            // print received data
            fwrite(_buf.base(), 1, _nread, stdout);
            fflush(stdout);
          }
        }
    );
    if (!peer)  PRINT_UV_ERR(peer.uv_status(), "read initiation");
  };

  // attach the connect request to the loop
  conn.run(peer, server_addr);
  if (!conn)
  {
    PRINT_UV_ERR(conn.uv_status(), "connect initiation");
    return conn.uv_status();
  }
//}

  auto ret = uv::loop::Default().run(UV_RUN_DEFAULT);

  uvcc_debug_function_return();
  return ret;
}
