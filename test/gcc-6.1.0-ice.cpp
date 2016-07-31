
#if 1

typedef void (*foo_t)();

void test(foo_t, foo_t)
{}

template< typename >
struct A
{
  template< typename = void > static void foo1();
  template< typename = void > static void foo2();

  void bar()
  {
    test(foo1, foo2<>);
  }

  template< typename >
  void baz()
  {
    test(foo1<>, foo2);
  }
};

template< typename _T_ >
template< typename >
void A< _T_ >::foo1()
{}

template< typename _T_ >
template< typename >
void A< _T_ >::foo2()
{}

#else

template< typename = void >
struct Outer
{
  template< typename >
  struct Inner;
};

template<>
template< typename _T_ >
class Outer< _T_ >::Inner< _T_ >
{};

template struct Outer< void >::Inner< void >;

#endif

int main() {}

