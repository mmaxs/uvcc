

#include <cstdio>

#include "uvcc.hpp"


#define __pf(...) { fprintf(stderr, "%s\n", __PRETTY_FUNCTION__); __VA_ARGS__;  fflush(stderr); fflush(stdout); }

#define __t  { fprintf(stderr, "@%u: ", __LINE__); fflush(stderr); }



int main()
{

  init(1, 0, 1, 1, 1);

  for(long i = 0; i < buf_count; ++i) printf("%zu\t", bufs[i].len);
  printf("\n");
  for(long i = 0; i < buf_count; ++i) printf("%p\t", bufs[i].base);
  printf("\n");
    
    
  return 0;
}

