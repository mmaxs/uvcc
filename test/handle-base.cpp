
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


using on_read_t = std::function< void(uv::io _handle, ssize_t _nread, uv::buffer _buffer) >;
using on_connection_t = std::function< void(uv::io) >;
struct properties
{
  uv::spinlock rdstate_switch;
  bool rdstate_flag = false;
  uv::on_buffer_alloc_t alloc_cb;
  on_read_t read_cb;
  on_connection_t connection_cb;
};


int main(int _argc, char *_argv[])
{

  typename test< A >::type a;
  typename test< B >::type b;

  fprintf(stdout, "spinlock: SIZE=%zu ALIGN=%zu\n", sizeof(uv::spinlock), alignof(uv::spinlock)); fflush(stdout);
  fprintf(stdout, "on_read_t: SIZE=%zu ALIGN=%zu\n", sizeof(on_read_t), alignof(on_read_t)); fflush(stdout);
  fprintf(stdout, "uv::on_buffer_alloc_t: SIZE=%zu ALIGN=%zu\n", sizeof(uv::on_buffer_alloc_t), alignof(uv::on_buffer_alloc_t)); fflush(stdout);
  fprintf(stdout, "property: SIZE=%zu ALIGN=%zu\n", sizeof(properties), alignof(properties)); fflush(stdout);

  getchar();
  return 0;
}
