
#include <cstdio>

#include "uvcc.hpp"
#include <cstdio>
#include <cassert>
#include <functional>
#include <type_traits>


#define __pf { fprintf(stderr, "%s\n", __PRETTY_FUNCTION__); fflush(stderr); fflush(stdout); }

#define __t  { fprintf(stderr, "@%u: ", __LINE__); fflush(stderr); fflush(stdout); }



#pragma GCC diagnostic ignored "-Wunused-variable"


void w(uv::handle)  {}

struct S
{
  void operator ()(uv::handle) {}
};

S s;

std::function< void(uv::handle) > f;


int main(int _argc, char *_argv[])
{

  uv::loop loop = uv::loop::Default();

  loop.walk(
      [](uv::handle, void*) -> void {},
      nullptr
  );

  loop.walk(w);
  loop.walk(f);

  getchar();
  return 0;
}
