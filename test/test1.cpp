
#include <cstdio>

#include "uvcc.hpp"
#include <cstdio>
#include <functional>
#include <type_traits>
#include <atomic>


#pragma GCC diagnostic ignored "-Wunused-variable"


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


void ccb1(uv::connect _c, int _status)
{
  fprintf(stdout, "ccb1: %s(%i): %s\n", ::uv_err_name(_status), _status, ::uv_strerror(_status));  fflush(stdout);
  if (_status == 0)  ::send(_c.handle().socket(), "Hello!\n", 7, 0);
}
void ccb2(uv::connect _c, int _status)
{
  fprintf(stdout, "ccb2: %s(%i): %s\n", ::uv_err_name(_status), _status, ::uv_strerror(_status));  fflush(stdout);
  if (_status == 0)
  {
    uv::write wr;
    wr.on_request() = [](uv::write _req, int _status){
          fprintf(stdout, "write: %s(%i): %s\n", ::uv_err_name(_status), _status, ::uv_strerror(_status));  fflush(stdout);
    };
    static const char* msg = "Hello, uvcc!\n";
    buffer b;
    b.base() = const_cast< char* >(msg);
    b.len() = std::strlen(msg) + 1;
    wr.run(_c.handle(), b);
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

  tcp c(::uv_default_loop());
  c.on_destroy() = destroy_cb1;
  handle &h1 = c;

  handle h2 = h1;
    

  //}

  //{
    uv::connect c_req;
    c_req.on_destroy() = destroy_cb2();

    ::sockaddr_in in_loopback;
    in_loopback.sin_family = AF_INET;
    in_loopback.sin_port   = hton16(80);  //0x5000;
    in_loopback.sin_addr.s_addr  = 0x0100007f;
    //in_loopback.sin_zero   = {0};
    
    c_req.on_request() = ccb2;

    int o = c_req.run(c, in_loopback);
    fprintf(stdout, "c_req: %s(%i): %s\n", ::uv_err_name(o), o, ::uv_strerror(o));
    fflush(stdout);
  
    int t = ::uv_run(::uv_default_loop(), UV_RUN_DEFAULT);

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
