
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
        fprintf(stdout, "timer: count=%i repeat=%" PRIi64 "\n", _count, ::uv_timer_get_repeat(static_cast< ::uv_timer_t* >(_t)));
        fflush(stdout);
        if (--_count <= 0)  ::uv_timer_stop(static_cast< ::uv_timer_t* >(_t));
      },
      count
  );

  uv::loop::Default().run(UV_RUN_DEFAULT);

  fprintf(stdout, "count=%i\n", count);
  fflush(stdout);

  return 0;
}
