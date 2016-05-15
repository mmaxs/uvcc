
#include <cstdio>

#include "uvcc.hpp"
#include <cstdio>
#include <cassert>
#include <functional>
#include <type_traits>


#define __pf { fprintf(stderr, "%s\n", __PRETTY_FUNCTION__); fflush(stderr); fflush(stdout); }

#define __t  { fprintf(stderr, "@%u: ", __LINE__); fflush(stderr); fflush(stdout); }



#pragma GCC diagnostic ignored "-Wunused-variable"



int main(int _argc, char *_argv[])
{

  uv::loop loop = uv::loop::Default();

  loop.walk(
      [](uv::handle, void*) -> void {},
      nullptr
  );

  getchar();
  return 0;
}
