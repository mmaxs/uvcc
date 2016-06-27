
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

  uv::work< int > w;
  w.on_request() = [](decltype(w))
  {
    fprintf(stdout, "Work completion callback.\n");
    fflush(stdout);
  };

  w.run(
      uv::loop::Default(),
      [](decltype(w) _work, int _ret)
      {
        fprintf(stdout, "Work begining.\n");
        fprintf(stdout, "Loop status: %u\n", _work.loop().is_alive());
        fprintf(stdout, "Work ending.\n");
        fflush(stdout);
        return _ret;
      },
      1234
  );

  fprintf(stdout, "Work return status: %u\n", w.result().get());
  fflush(stdout);

  // launch on_request callback
  uv::loop::Default().run(UV_RUN_DEFAULT);

  getchar();
  return 0;
}
