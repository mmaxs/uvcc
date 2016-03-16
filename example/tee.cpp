
#include <cstdio>

#include "uvcc.hpp"
#include <cstdio>
#include <vector>


uv::pipe in(uv::loop::Default(), ::fileno(stdin)),
         out(uv::loop::Default(), ::fileno(stdout));


int main(int _argc, char *_argv[])
{

  if (!in)
  {
    fprintf(stderr, "stdin open error: %s (%i): %s\n", ::uv_err_name(in.uv_status()), in.uv_status(), ::uv_strerror(in.uv_status()));
    fflush(stderr);
    return in.uv_status();
  };
  if (!out)
  {
    fprintf(stderr, "stdout open error: %s (%i): %s\n", ::uv_err_name(out.uv_status()), out.uv_status(), ::uv_strerror(out.uv_status()));
    fflush(stderr);
    return out.uv_status();
  };

  in.read_start(
      [](uv::handle, std::size_t) -> uv::buffer
      {
        const std::size_t default_size = 8192;
        static std::vector< uv::buffer > buf_pool;

        for (auto &b : buf_pool)  if (b.nrefs() == 1)  {
            b.len() = default_size;
            return b;
        };

        buf_pool.emplace_back(uv::buffer{default_size});
        //fprintf(stderr, "new buffer: %lu\n", buf_pool.back().len());
        fprintf(stderr, "buffer pool: %zu\n", buf_pool.size());
        fflush(stderr);

        return buf_pool.back();
      },
      [](uv::stream _stream, ssize_t _nread, uv::buffer _buffer) -> void
      {
        if (_nread == UV_EOF)
        {
          _stream.read_stop();
          fprintf(stderr, "read: EOF\n"); fflush(stderr);
        }
        else if (_nread < 0)
        {
          fprintf(stderr, "read error: %s (%zi): %s\n", ::uv_err_name(_nread), _nread, ::uv_strerror(_nread));
          fflush(stderr);
        }
        else if (_nread > 0)
        {
          fprintf(stderr, "read bytes: %zi\n", _nread);
          //fprintf(stderr, "read string: '");
          //fwrite(_buffer.base(0), 1, _nread, stderr);
          //fprintf(stderr, "'\n");
          //fprintf(stderr, "buffer: %lu\n", _buffer.len());
          fflush(stderr);

          auto wr = []() -> uv::write
          {
            static std::vector< uv::write > wr_pool;

            auto default_write_cb = [](uv::write _wr, int _status) -> void
            {
              if (_status < 0)
                fprintf(stderr, "write error: %s (%i): %s\n", ::uv_err_name(_status), _status, ::uv_strerror(_status));
              else
                fprintf(stderr, "write: done\n");
              fflush(stderr);
            };

            for (auto &wr : wr_pool)  if (wr.nrefs() == 1)  {
                wr.on_request() = default_write_cb;
                return wr;
            };

            wr_pool.emplace_back();
            wr_pool.back().on_request() = default_write_cb;
            fprintf(stderr, "write request pool: %zu\n", wr_pool.size()); fflush(stderr);

            return wr_pool.back();
          };

          _buffer.len() = _nread;
          wr().run(out, _buffer);
        };
      }
  );

  return uv::loop::Default().run(UV_RUN_DEFAULT);
}
