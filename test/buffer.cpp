

#include <cstdio>

#include "uvcc.hpp"


#define __pf { fprintf(stderr, "%s\n", __PRETTY_FUNCTION__); fflush(stderr); fflush(stdout); }

#define __t  { fprintf(stderr, "@%u: ", __LINE__); fflush(stderr); }



int main()
{

  uv::buffer b({10, 20, 30, 0, 40, 50});
  printf("b.count()=%zu\n", b.count());  fflush(stdout);

  printf("%lu\t%p\n", b.len(), b.base());
  for(std::size_t i = 1; i < b.count(); ++i) printf("%lu\t%p (%p)\n", b[i].len, b[i].base, (b[i-1].base + b[i-1].len));
  fflush(stdout);

  getchar();
  return 0;
}

