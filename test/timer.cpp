
#include "uvcc.hpp"
#include <cstdio>
#include <cinttypes>
#include <functional>


#pragma GCC diagnostic ignored "-Wunused-variable"


int main(int _argc, char *_argv[])
{
  int count = 10;

  uv::timer t(uv::loop::Default());

  t.start(0, 1000,
      [](uv::timer _t, int &_count){
        auto rv = _t.repeat_interval();
        fprintf(stdout, "timer: count=%i repeat=%" PRIi64 "\n", _count, rv);
        fflush(stdout);

        rv = rv/3;
        fprintf(stdout, "timer: count=%i repeat:=%" PRIi64 "\n\n", _count, rv);
        fflush(stdout);

        _t.repeat_interval(rv);

        if (--_count <= 0)  _t.stop();
      },
      std::ref(count)
  );

  uv::loop::Default().run(UV_RUN_DEFAULT);

  fprintf(stdout, "remaining count: %i\n", count);
  fflush(stdout);

  return 0;
}
