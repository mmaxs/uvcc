

#include <cstdio>

#define __pf(...) { fprintf(stderr, "%s\n", __PRETTY_FUNCTION__); __VA_ARGS__; }

#define __t  { fprintf(stderr, "@%u: ", __LINE__); }


using buf_t = struct {
    char *base;
    size_t len;
};

long buf_count;
buf_t *bufs;


template< typename... _Ls_ >
void init(const _Ls_&... _len)
{
  buf_count = sizeof...(_Ls_);
  bufs = new buf_t[buf_count];
  bufs[0].base = new char[(... + _len)];
  printf("%u\n", (... + _len));
  auto buf = bufs;
  (..., ((buf++)->len = _len));
  buf = bufs;
  (..., ((++buf)->base = &buf[-1].base[buf[-1].len], (void)_len));
}


int main()
{

  init(1, 0, 1, 1, 1);

  for(long i = 0; i < buf_count; ++i) printf("%zu\t", bufs[i].len);
  printf("\n");
  for(long i = 0; i < buf_count; ++i) printf("%p\t", bufs[i].base);
  printf("\n");
    
    
  return 0;
}

