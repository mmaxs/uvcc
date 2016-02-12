
#include <cstdio>

#include "uvcc/utility.hpp"
#include "uv.h"
#include <cstdio>
#include <functional>
#include <type_traits>


#pragma GCC diagnostic ignored "-Wunused-variable"


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

  fprintf(stdout,
      "%li %li %li\n",
      is_one_of<int, long, double, char, int, float>::value,
      is_one_of<long, long, double, char, int>::value,
      is_one_of<float, long, double, char, int>::value
  );
  fflush(stdout);

  union_storage<float, long, double, char, int> ustor;

  getchar();
  return 0;
}
