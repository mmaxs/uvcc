
#include "uvcc.hpp"
#include <cstdio>
#include <vector>


#define ERR(prefix, code)  do {\
    fflush(stdout);\
    fprintf(stderr, "%s: %s (%i): %s\n", prefix, uv_err_name(code), (int)code, uv_strerror(code));\
    fflush(stderr);\
} while (0)



uv::pipe in(uv::loop::Default(), fileno(stdin)),
         out(uv::loop::Default(), fileno(stdout));


int main(int _argc, char *_argv[])
{

  if (!in)
  {
    ERR("stdin open", in.uv_status());
    return in.uv_status();
  };
  if (!out)
  {
    ERR("stdout open", out.uv_status());
    return out.uv_status();
  };

  in.read_start(
      [](uv::handle, std::size_t) -> uv::buffer  // alloc_cb
      {
        const std::size_t default_size = 8192;
        static std::vector< uv::buffer > buf_pool;

        for (std::size_t i = 0; i < buf_pool.size(); ++i)  if (buf_pool[i].nrefs() == 1)  {
            buf_pool[i].len() = default_size;
            fprintf(stderr, "[buffer pool]: item #%zu of %zu\n", i+1, buf_pool.size());  fflush(stderr);
            return buf_pool[i];
        }; /*else {
            fprintf(stderr, "[buffer pool]: item #%zu nrefs: %li\n", i+1, buf_pool[i].nrefs());  fflush(stderr);
        };*/

        buf_pool.emplace_back(uv::buffer{default_size});
        fprintf(stderr, "[buffer pool]: new item #%zu\n", buf_pool.size());  fflush(stderr);
        return buf_pool.back();
      },
      [](uv::stream _stream, ssize_t _nread, uv::buffer _buffer) -> void  // read_cb
      {
        if (_nread == UV_EOF)
          _stream.read_stop();
        else if (_nread < 0)
          ERR("read", _nread);
        else if (_nread > 0)
        {
          auto wr = []() -> uv::write
          {
            static std::vector< uv::write > wr_pool;

            auto default_write_cb = [](uv::write _wr, int _status) -> void  // write_cb
            {
              if (_status < 0)  ERR("write", _status);
            };

            for (auto &wr : wr_pool)  if (wr.nrefs() == 1)  {
                wr.on_request() = default_write_cb;
                return wr;
            };

            wr_pool.emplace_back();
            wr_pool.back().on_request() = default_write_cb;
            fprintf(stderr, "[write request pool]: new item #%zu\n", wr_pool.size());  fflush(stderr);
            return wr_pool.back();
          };

          _buffer.len() = _nread;
          wr().run(out, _buffer);
        };
      }
  );

  return uv::loop::Default().run(UV_RUN_DEFAULT);
}
