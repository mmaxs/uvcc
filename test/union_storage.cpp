
#include <cstdio>

#include "uvcc.hpp"
#include <cstdio>
#include <functional>
#include <type_traits>


#pragma GCC diagnostic ignored "-Wunused-variable"




void foo()
{
  fprintf(stdout, "%s\n", __PRETTY_FUNCTION__); fflush(stdout);
}

using foo_t = decltype(foo);
static_assert(std::is_same< foo_t, void() >::value, "");



int main(int _argc, char *_argv[])
{

  uv::union_storage< std::function< foo_t >, int > s(2);
  s.reset(1);

  getchar();
  return 0;
}
