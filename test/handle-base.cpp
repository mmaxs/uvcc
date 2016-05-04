
#include <cstdio>

#include "uvcc.hpp"
#include <cstdio>
#include <cassert>
#include <functional>
#include <type_traits>


#define __pf { fprintf(stderr, "%s\n", __PRETTY_FUNCTION__); fflush(stderr); fflush(stdout); }

#define __t  { fprintf(stderr, "@%u: ", __LINE__); fflush(stderr); fflush(stdout); }



#pragma GCC diagnostic ignored "-Wunused-variable"



struct false_type
{
  false_type() __pf;
};

struct user_type
{
  user_type() __pf;
};


template< class _T_ >
struct test
{
  template< typename _U_, typename = std::size_t > struct test_type
  { using type = false_type; };
  template< typename _U_                         > struct test_type< _U_, decltype(sizeof(typename _U_::test_type)) >
  { using type = typename _U_::test_type; };

  using type = typename test_type< _T_ >::type;
};



struct A {};
struct B { using test_type = user_type; };


int main(int _argc, char *_argv[])
{

  typename test< A >::type a;
  typename test< B >::type b;

  getchar();
  return 0;
}
