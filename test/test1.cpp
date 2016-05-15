
#include <cstdio>

#include "uvcc.hpp"
#include <cstdio>
#include <cstring>
#include <functional>
#include <type_traits>
#include <atomic>


#pragma GCC diagnostic ignored "-Wunused-variable"


#define PRINT_UV_ERR(prefix, code)  do {\
    fflush(stdout);\
    fprintf(stderr, "%s: %s (%i): %s\n", prefix, uv_err_name(code), (int)(code), uv_strerror(code));\
    fflush(stderr);\
} while (0)


using namespace uv;


void destroy_cb1(void*)
{
  fprintf(stderr, "destroy_cb1\n");  fflush(stderr);
}

struct destroy_cb2
{
  void operator ()(void*)
  {
    fprintf(stderr, "destroy_cb2\n");  fflush(stderr);
  }
};


void ccb1(uv::connect _conn)
{
  if (!_conn)
    PRINT_UV_ERR("ccb1", _conn.uv_status());
  else
    ::send(static_cast< uv::tcp&& >(_conn.handle()).socket(), "Hello!\n", 7, 0);
}

void ccb2(uv::connect _conn)
{
  if (!_conn)
    PRINT_UV_ERR("ccb2", _conn.uv_status());
  else
  {
    uv::write wr;
    wr.on_request() = [](uv::write _wr, uv::buffer) -> void  {
        if (!_wr)  PRINT_UV_ERR("write", _wr.uv_status());
        _wr.handle().loop().walk(
            [](handle _h, void*) -> void
            {
              fprintf(stdout, "walk: %p:%s(%u)\n", _h.fileno(), _h.type_name(), _h.type());  fflush(stdout);
            },
            nullptr
        );
    };
    static const char* msg = "Hello, uvcc!\n";
    buffer buf;
    buf.base() = const_cast< char* >(msg);
    buf.len() = std::strlen(msg) + 1;
    wr.run(_conn.handle(), buf);
  };
}


int main(int _argc, char *_argv[])
{
/*
  std::atomic< int > ai(0);
  fprintf(stdout, "atomic<int> is_lock_free=%u\n", ai.is_lock_free());
  ai.fetch_add(1);

  struct s
  {
    std::atomic< int > i;
    int j;
  };
  fprintf(stdout, "is_standard_layout<s>=%u\n", std::is_standard_layout< s >::value);
  fflush(stdout);
*/

  {
    buffer b{1, 2};

  //tcp c(::uv_default_loop());
  tcp c(loop::Default());
  c.on_destroy() = destroy_cb1;
  handle &h1 = c;

  handle h2 = h1;

  c.read_start(
      [](handle, std::size_t){ return buffer(); },
      [](io, ssize_t, buffer, void*){}
  );
  c.read_stop();

  //}

  //{
    uv::connect c_req;
    c_req.on_destroy() = destroy_cb2();

    ::sockaddr_in in_loopback;
    in_loopback.sin_family = AF_INET;
    in_loopback.sin_port   = hton16(80);
    in_loopback.sin_addr.s_addr  = hton32(0x7f000001);  // 127.0.0.1
    //in_loopback.sin_zero   = {0};
    
    c_req.on_request() = ccb2;

    c_req.run(c, in_loopback);
    if (!c_req)  PRINT_UV_ERR("c_req", c_req.uv_status());
  
    //int t = ::uv_run(::uv_default_loop(), UV_RUN_DEFAULT);
    loop::Default().run(UV_RUN_DEFAULT);

  }

  /*
  std::function< std::remove_pointer_t< ::uv_close_cb > > f;

  f = ccb1;
  f(nullptr);
  f = ccb2();
  f(nullptr);
  */
/* 
  fprintf(stderr,
      "sizeof(on_request_union_t)=%llu alignof(on_request_union_t)=%llu\n",
      sizeof(on_request_union_t), alignof(on_request_union_t)
  );
  fflush(stderr);
*/

  getchar();
  return 0;
}
