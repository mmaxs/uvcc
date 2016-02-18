
#include <cstdio>

#include "uvcc.hpp"
#include <cstdio>
#include <cassert>
#include <functional>
#include <type_traits>


#define __pf(...) { fprintf(stderr, "%s\n", __PRETTY_FUNCTION__); __VA_ARGS__;  fflush(stderr); fflush(stdout); }

#define __t  { fprintf(stderr, "@%u: ", __LINE__); fflush(stderr); }



#pragma GCC diagnostic ignored "-Wunused-variable"



void foo()  __pf()

using foo_t = decltype(foo);
static_assert(std::is_same< foo_t, void() >::value, "");


struct bar
{
  ~bar()  __pf();
  bar()  __pf();
};



int main(int _argc, char *_argv[])
{

  fprintf(stdout,
      "%zu %zu %zu\n",
      uv::is_one_of<int, long, double, char, int, float>::value,
      uv::is_one_of<long, long, double, char, int>::value,
      uv::is_one_of<float, long, double, char, int>::value
  );
  fflush(stdout);

  fprintf(stdout, "uv::union_storage< foo_t*, int, std::function< foo_t >, bar >\n"); fflush(stdout);
  
  uv::union_storage< foo_t*, int, std::function< foo_t >, bar > s(2);
  fprintf(stdout, "tag:%zu\n", s.tag()); fflush(stdout);
  assert(s.tag() == 2);
  
  s.reset(foo);
  fprintf(stdout, "tag:%zu\n", s.tag()); fflush(stdout);
  s.value< foo_t* >()();
  assert(s.tag() == 1);

  s.reset(bar());
  fprintf(stdout, "tag:%zu\n", s.tag()); fflush(stdout);
  assert(s.tag() == 4);

  s.reset(1);
  fprintf(stdout, "tag:%zu\n", s.tag()); fflush(stdout);
  assert(s.tag() == 2);

  getchar();
  return 0;
}
