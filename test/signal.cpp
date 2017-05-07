
#include "uvcc.hpp"
#include <cstdio>
#include <cinttypes>
#include <functional>


#pragma GCC diagnostic ignored "-Wunused-variable"



int main(int _argc, char *_argv[])
{

  int count = 0;
{
  uv::signal sigint(uv::loop::Default());
  sigint.on_signal() = [&count](uv::signal _s, bool){
    ++count;
    fprintf(stdout, "SIGINT (%i): count=%i\n", SIGINT, count);
    fflush(stdout);
  };
  sigint.start(SIGINT);

  uv::signal sigbreak(uv::loop::Default());
  sigbreak.start(SIGBREAK, [&sigint](uv::signal _sigbreak, bool){
    fprintf(stdout, "SIGBREAK (%i)\n", SIGBREAK);
    fflush(stdout);

    _sigbreak.stop();
    sigint.stop();
  });
}

  uv::loop::Default().run(UV_RUN_DEFAULT);

  return 0;
}
