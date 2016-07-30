
#if 1

typedef void (*foo_t)();

void test(foo_t)
{}

template< typename >
struct A
{
  template< typename = void > static void foo();

  void bar()
  {
    test(foo);
  }

  template< typename >
  void baz()
  {
    test(foo);
  }
};

template< typename _T_ >
template< typename >
void A< _T_ >::foo()
{}

#else

template< typename >
struct Outer
{
  template< typename >
  struct Inner;
};

template<>
template< typename _T_ >
class Outer< _T_ >::Inner< _T_ >
{};

template struct Outer< int >::Inner< int >;

#endif

