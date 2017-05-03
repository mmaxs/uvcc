
#define UVCC_DEBUG 1

#include "uvcc.hpp"
#include <cstdio>


int main(int _argc, char *_argv[])
{
  uv::process p(uv::loop::Default());

  p.inherit_stdio(0, 0);
  p.create_stdio_pipe(1, uv::loop::Default(), UV_WRITABLE_PIPE);

  for (std::size_t i = 0; i < p.stdio().size(); ++i)  uvcc_debug_log_if(true,
      "stdio #%zu: id=0x%08tX", i, p.stdio()[i].id()
  );

  p.spawn(_argv[1], _argv+1);
  uvcc_debug_log_if(true, "pid = %i", p.pid());

  p.stdio()[1].read_start(
      [](uv::handle, std::size_t _size){ return uv::buffer{ _size }; },
      [](uv::io _io, ssize_t _nread, uv::buffer _buffer, int64_t, void*)
      {
        if (_nread < 0)
        {
          if (_nread != UV_EOF)  uvcc_debug_log_if(true,
              "read error: %s (%zi): %s", ::uv_err_name(_nread), _nread, ::uv_strerror(_nread)
          );
          _io.read_stop();
        }
        else if (_nread > 0)
        {
          fwrite(_buffer.base(), 1, _nread, stdout);
          fflush(stdout);
        }
      },
      8192
  );

  return uv::loop::Default().run(UV_RUN_DEFAULT);
}
