
#include <cstdio>

#include "uvcc/utility.hpp"
#include "uvcc/endian.hpp"
#include "uvcc/netstruct.hpp"
#include "uv.h"
#include <cstdio>
#include <functional>
#include <type_traits>
#include <string>


#pragma GCC diagnostic ignored "-Wunused-variable"


#define PRINT_UV_ERR(prefix, code)  do {\
    fflush(stdout);\
    fprintf(stderr, "%s: %s (%i): %s\n", prefix, uv_err_name(code), (int)(code), uv_strerror(code));\
    fflush(stderr);\
} while (0)



using namespace uv;

#if 0
class non_standard_layout : counted_base< non_standard_layout >
{
private /*types*/:
  using on_destroy_t = std::function< void(void*) >;
  using on_destroy_func_storage_t = std::aligned_storage< sizeof(on_destroy_t), alignof(on_destroy_t) >::type;

private /*data*/:
  on_destroy_func_storage_t on_destroy_func_storage;
  alignas(::uv_any_handle) ::uv_tcp_t uv_handle;
};

class standard_layout
{
private /*types*/:
  using on_destroy_t = std::function< void(void*) >;
  using on_destroy_func_storage_t = std::aligned_storage< sizeof(on_destroy_t), alignof(on_destroy_t) >::type;

private /*data*/:
  counted_base< standard_layout > ref_count;
  on_destroy_func_storage_t on_destroy_func_storage;
  alignas(::uv_any_handle) ::uv_tcp_t uv_handle;
};
#endif


int main(int _argc, char *_argv[])
{
/*
  //static_assert(std::is_standard_layout< non_standard_layout >::value, "not a standard layout type");  // assertion failed
  static_assert(std::is_standard_layout< standard_layout >::value, "not a standard layout type");
*/
/*
  fprintf(stdout,
      "%li %li %li\n",
      is_one_of<int, long, double, char, int, float>::value,
      is_one_of<long, long, double, char, int>::value,
      is_one_of<float, long, double, char, int>::value
  );
  fflush(stdout);

  union_storage<float, long, double, char, int> ustor;
*/

  sockaddr_in sa;
  uv::init(sa);
  sa.sin_port = uv::hton16(80);
  sa.sin_addr.s_addr = uv::hton32(0x7F000001);

  PRINT_UV_ERR("uv_ip4_addr",
      ::uv_ip4_addr("192.168.0.1", 80, &sa)
  );

  std::string name(100, '\0');
  PRINT_UV_ERR("uv_ip4_name",
      ::uv_ip4_name(&sa, (char*)name.c_str(), name.length())
  );
  printf("sa: %s\n", name.c_str());
  fflush(stdout);

  getchar();
  return 0;
}
