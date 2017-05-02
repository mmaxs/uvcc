
#include "uvcc.hpp"
#include <cstdio>
#include <cinttypes>
#include <functional>


#pragma GCC diagnostic ignored "-Wunused-variable"


int main(int _argc, char *_argv[])
{
  uv::process p(uv::loop::Default());

  fprintf(stdout, "pid = %i\n", p.pid());
  fflush(stdout);


  //p.create_stdio_pipe(1, uv::loop::Default(), UV_WRITABLE_PIPE);
  p.inherit_stdio(0, 0);
  p.inherit_stdio(1, 1);
  p.inherit_stdio(2, 2);

  for (std::size_t i = 0; i < p.stdio().size(); ++i)
  {
    fprintf(stdout, "stdio #%zu: id=0x%08tX\n", i, p.stdio()[i].id());
    fflush(stdout);
  }

  p.spawn(_argv[1], _argv+1);

  return uv::loop::Default().run(UV_RUN_DEFAULT);
}
