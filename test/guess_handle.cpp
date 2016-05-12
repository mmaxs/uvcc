
#include "uvcc.hpp"
#include <cstdio>


#define PRINT_UV_ERR(prefix, code)  do {\
    fflush(stdout);\
    fprintf(stderr, "%s: %s (%i): %s\n", prefix, uv_err_name(code), (int)(code), uv_strerror(code));\
    fflush(stderr);\
} while (0)



uv::io in = uv::io::guess_handle(fileno(stdin));
uv::io out = uv::io::guess_handle(fileno(stdout));


const char* str_handle_type(uv::handle &_h)
{
  const char *ret;

  switch (_h.type())
  {
#define XX(X, x) case UV_##X: ret = #x; break;
    UV_HANDLE_TYPE_MAP(XX)
#undef XX
    case UV_FILE: ret = "file"; break;
    default: ret = "<unknown>"; break;
  }

  return ret;
}


int main(int _argc, char *_argv[])
{

  if (!in)
  {
    PRINT_UV_ERR("stdout open", out.uv_status());
    return out.uv_status();
  }
  else
  {
    fprintf(stdout, "stdin: %s\n", str_handle_type(in));
    fflush(stdout);
  }

  if (!out)
  {
    PRINT_UV_ERR("stdout open", out.uv_status());
    return out.uv_status();
  }
  else
  {
    fprintf(stdout, "stdout: %s\n", str_handle_type(out));
    fflush(stdout);
  }

  return uv::loop::Default().run(UV_RUN_DEFAULT);
}
