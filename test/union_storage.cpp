
#undef NDEBUG

#include <cstdio>

#include "uvcc.hpp"
#include <cstdio>
#include <cassert>
#include <functional>
#include <type_traits>


#define __pf { fprintf(stderr, "%s\n", __PRETTY_FUNCTION__); fflush(stderr); fflush(stdout); }

#define __t  { fprintf(stderr, "@%u: ", __LINE__); fflush(stderr); }



#pragma GCC diagnostic ignored "-Wunused-variable"



void foo()  __pf

using foo_t = decltype(foo);
static_assert(std::is_same< foo_t, void() >::value, "");


struct bar
{
  ~bar()  __pf;
  bar()  __pf;
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
  fprintf(stdout, "<int> tag:%s\n", s.tag()->name()); fflush(stdout);
  assert(s.tag() == &typeid(int));
  
  s.reset(foo);
  fprintf(stdout, "<void (*)()> tag:%s\n", s.tag()->name()); fflush(stdout);
  s.get< foo_t* >()();
  assert(s.tag() == &typeid(foo_t*));

  s.reset(bar());
  fprintf(stdout, "<bar> tag:%s\n", s.tag()->name()); fflush(stdout);
  assert(s.tag() == &typeid(bar));

  s.reset(1);
  fprintf(stdout, "<int> tag:%s\n", s.tag()->name()); fflush(stdout);
  assert(s.tag() == &typeid(int));

  getchar();
  return 0;
}

