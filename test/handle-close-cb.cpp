

#define DEBUG 2

#include "uvcc.hpp"
#include <cstdio>

#pragma GCC diagnostic ignored "-Wunused-variable"


#define PRINT_UV_ERR(code, prefix, ...)  do {\
  fflush(stdout);\
  fprintf(stderr, (prefix), ##__VA_ARGS__);\
  fprintf(stderr, ": %s (%i): %s\n", ::uv_err_name(code), (int)(code), ::uv_strerror(code));\
  fflush(stderr);\
} while (0)

struct A
{
  A()
  {
    fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
    fflush(stderr);
  }
  ~A()
  {
    fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
    fflush(stderr);
  }
};

A a;
uv::loop &L = uv::loop::Default();

void ccb(void *_data)
{
  fprintf(stderr, "destroy callback for the ");
  uv::debug::print_handle(static_cast< ::uv_handle_t* >(_data));
}


int main(int _argc, char *_argv[])
{
  L.on_exit() = [](uv::loop _loop)
  {
    fprintf(stderr, "loop after exit walk: begin\n");
    uv::debug::print_loop_handles(static_cast< ::uv_loop_t* >(_loop));
    fprintf(stderr, "loop after exit walk: end\n");
    fflush(stderr);
  };


#if 0  
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
#endif
//{
  // initialize a tcp socket
  uv::tcp peer(uv::loop::Default(), /*server_addr.ss_family*/AF_INET);
  if (!peer)
  {
    PRINT_UV_ERR(peer.uv_status(), "tcp socket");
    return peer.uv_status();
  }
  peer.data() = static_cast< ::uv_handle_t* >(peer);
  peer.on_destroy() = ccb;
  fprintf(stderr, "create a ");
  uv::debug::print_handle(static_cast< ::uv_handle_t* >(peer));
//}
#if 0
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
#endif

  L.run(UV_RUN_DEFAULT);
#if 0
  ::uv_unref(static_cast<::uv_handle_t*>(peer));
  fprintf(stderr, "un-reference ");
  uv::debug::print_handle(static_cast<::uv_handle_t*>(peer));
  fprintf(stderr, "loop walk: begin\n");
  uv::debug::print_loop_handles(static_cast< ::uv_loop_t* >(L));
  fprintf(stderr, "loop walk: end\n");
  fflush(stderr);
#endif
#if 0
  fprintf(stderr, "- loop exit -\n");
  //::uv_print_all_handles(::uv_default_loop()/*(uv_loop_t*)L*/, stderr);  // segmentation fault
  //fflush(stderr);
  L.walk([](uv::handle _handle){
      fprintf(stderr,
          "0x%08llX:%s (uvref:%i active:%i closing:%i nrefs:%li)\n",
          (uintptr_t)_handle.fileno(), _handle.type_name(), ::uv_has_ref((::uv_handle_t*)_handle), _handle.is_active(), _handle.is_closing(), _handle.nrefs()
      );
      fflush(stderr);
  });
  fprintf(stderr, "loop alive:%i\n", L.is_alive());  fflush(stderr);

  ::uv_unref(static_cast<::uv_handle_t*>(peer));
  fprintf(stderr, "- handle unref -\n");
  //::uv_print_all_handles(::uv_default_loop()/*(uv_loop_t*)L*/, stderr);  // segmentation fault
  //fflush(stderr);
  L.walk([](uv::handle _handle){
      fprintf(stderr,
          "0x%08llX:%s (uvref:%i active:%i closing:%i nrefs:%li)\n",
          (uintptr_t)_handle.fileno(), _handle.type_name(), ::uv_has_ref((::uv_handle_t*)_handle), _handle.is_active(), _handle.is_closing(), _handle.nrefs()
      );
      fflush(stderr);
  });
  fprintf(stderr, "loop alive:%i\n", L.is_alive());  fflush(stderr);
#endif
  return 0;
}
