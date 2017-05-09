
#include "uvcc.hpp"
#include <cstdio>
#include <cinttypes>
#include <functional>


#pragma GCC diagnostic ignored "-Wunused-variable"

int test = 1;

int main(int _argc, char *_argv[])
{
  uv::async a(uv::loop::Default());

  int count = 10;

  uv::timer t(uv::loop::Default());

  t.start(0, 1000,
      [&a](uv::timer _t, int &_count)
      {
        fprintf(stdout, "timer: count=%i repeat=%" PRIi64 "\n", _count, _t.repeat_value());
        fflush(stdout);

        a.send(
            [](uv::async _a, int _count, int _repeat_value)
            {
              fprintf(stdout, "async: count=%i\n", _count);
              fflush(stdout);
              if (_count <= 0 or _repeat_value == 0)
              {
                _a.attached(false);
                fprintf(stdout, "async: attached=%i\n", _a.attached());
                fflush(stdout);
              }
            },
            _count,
            _t.repeat_value()
        );

        _t.repeat_value(_t.repeat_value()/3);
        if (--_count <= 0)  _t.stop();
      },
      std::ref(count)
  );

  uv::loop::Default().run(UV_RUN_DEFAULT);

  fprintf(stdout, "remaining count: %i\n", count);
  fflush(stdout);

  return 0;
}
