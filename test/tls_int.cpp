
#include <cstdio>

#include "uvcc.hpp"
#include <cstdio>
#include <cassert>
#include <functional>
#include <type_traits>


#define __pf { fprintf(stderr, "%s\n", __PRETTY_FUNCTION__); fflush(stderr); fflush(stdout); }

#define __t  { fprintf(stderr, "@%u: ", __LINE__); fflush(stderr); fflush(stdout); }



#pragma GCC diagnostic ignored "-Wunused-variable"



void foo()  __pf

using foo_t = decltype(foo);
static_assert(std::is_same< foo_t, void() >::value, "");


struct bar
{
  ~bar()  __pf
  bar()  __pf
};



int main(int _argc, char *_argv[])
{

  uv::tls_int i(1), j(2);
  if (i != 1)  __pf;
  if (i >= j)  __pf;

  int k = i;
  const int n = j;

  getchar();
  return 0;
}
