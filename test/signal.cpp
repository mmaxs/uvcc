
#include "uvcc.hpp"
#include <cstdio>
#include <cinttypes>
#include <functional>


#pragma GCC diagnostic ignored "-Wunused-variable"


int main(int _argc, char *_argv[])
{
  int count = 0;

  uv::signal sigint(uv::loop::Default());
  sigint.start(SIGINT,
      [](uv::signal _s, int &_count){
        ++_count;
        fprintf(stdout, "SIGINT (%i): count=%i\n", SIGINT, _count);
        fflush(stdout);
      },
      std::ref(count)
  );

  uv::signal sigbreak(uv::loop::Default());
  sigbreak.start(SIGBREAK,
      [](uv::signal _sigbreak, uv::signal _sigint){
        fprintf(stdout, "SIGBREAK (%i)\n", SIGBREAK);
        fflush(stdout);

        _sigbreak.stop();
        _sigint.stop();
      },
      sigint
  );


  uv::loop::Default().run(UV_RUN_DEFAULT);

  return 0;
}
