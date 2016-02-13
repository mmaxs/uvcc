
#ifndef UVCC_UTILITY__HPP
#define UVCC_UTILITY__HPP

#include <cstring>      // memcpy()
#include <type_traits>  // is_void is_convertible enable_if_t decay aligned_storage
#include <atomic>       // atomic memory_order_*
#include <utility>      // forward() move()
#include <stdexcept>    // runtime_error


namespace uv
{
/*! \defgroup __utility Utility structures and definitions
    \brief The utility definitions being used throughout the library. */
//! \{


/* default_delete */
template< typename _T_ > struct default_delete
{
  using value_type = typename std::decay< _T_ >::type;

  constexpr default_delete() noexcept = default;

  // we can delete an object through the polimorphic pointer to its base class
  template<
      typename _U_,
      typename = std::enable_if_t< std::is_convertible< _U_*, _T_* >::value >
  > default_delete(const default_delete< _U_ >&) noexcept   {}

  void operator ()(value_type *_) const   { Delete(_); }

  // not for use for polimorphic deleting without the operator () above
  static void Delete(void *_)
  {
    static_assert(!std::is_void< value_type >::value, "void type");
    static_assert(sizeof(value_type) > 0, "incomplete type");
    delete static_cast< value_type* >(_);
    //fprintf(stderr, "%s\n", __PRETTY_FUNCTION__); fflush(stderr);
  }
};


/* default_destroy */
template< typename _T_ > struct default_destroy
{
  using value_type = typename std::decay< _T_ >::type;

  constexpr default_destroy() noexcept = default;

  // we can destroy an object through the polimorphic pointer to its base class
  template<
      typename _U_,
      typename = std::enable_if_t< std::is_convertible< _U_*, _T_* >::value >
  > default_destroy(const default_destroy< _U_ >&) noexcept   {}

  void operator ()(value_type *_) const   { Destroy(_); }

  // not for use for polimorphic destroying without the operator () above
  static void Destroy(void *_)
  {
    static_assert(!std::is_void< value_type >::value, "void type");
    static_assert(sizeof(value_type) > 0, "incomplete type");
    static_cast< value_type* >(_)->~value_type();
    //fprintf(stderr, "%s\n", __PRETTY_FUNCTION__); fflush(stderr);
  }
};


/*! \brief Checks if a type `_T_` belongs to a type list `_Ts_`.
    \details Provides the constexpr `value` that is equal to the index of the given type `_T_`
    in the type list `_Ts_` starting from **1** up to `sizeof...(_Ts_)` or **0** otherwise. */
template< typename _T_, typename... _Ts_ > struct is_one_of;
//! \cond
template< typename _T_ > struct is_one_of< _T_ >
{
  constexpr static const long value = 0;
};
template< typename _T_, typename... _Ts_ > struct is_one_of< _T_, _T_, _Ts_... >
{
  constexpr static const long value = 1;
};
template< typename _T_, typename _U_, typename... _Ts_ > struct is_one_of< _T_, _U_, _Ts_... >
{
private:
  constexpr static const long value_ =  is_one_of< _T_, _Ts_... >::value;
public:
  constexpr static const long value = value_ ? value_+1 : 0;
};
//! \endcond


//! \cond
template< typename _T_ > constexpr const _T_& greatest(const _T_& _v)   { return _v; }
//! \endcond
/*! \brief Similar to `std::max()` but it does not require the arguments being of the same type
    and using the `std::initializer_list` braces when there are more than two arguments. */
template< typename _T_, typename... _Ts_ > constexpr const _T_& greatest(const _T_& _v, const _Ts_&... _vs)
{ return _v < greatest(_vs...) ? greatest(_vs...) : _v; }

//! \cond
template< typename _T_ > constexpr const _T_& lowest(const _T_& _v)   { return _v; }
//! \endcond
/*! \brief Similar to `std::min()` but it does not require the arguments being of the same type
    and using the `std::initializer_list` braces when there are more than two arguments. */
template< typename _T_, typename... _Ts_ > constexpr const _T_& lowest(const _T_& _v, const _Ts_&... _vs)
{ return lowest(_vs...) < _v ? lowest(_vs...) : _v; }



/*! \brief A reference counter with atomic increment/decrement. */
class ref_count
{
public: /*types*/
  using type = long;

private: /*data*/
  std::atomic< type > count;

public: /*constructors*/
  ~ref_count() = default;

  ref_count() noexcept : count(1)  {}

  ref_count(const ref_count&) = delete;
  ref_count& operator =(const ref_count&) = delete;

  ref_count(ref_count&&) = delete;
  ref_count& operator =(ref_count&&) = delete;

public: /*interface*/
  type value() const noexcept  { return count.load(std::memory_order_acquire); }

  type inc()
  {
    auto c = value();
    do
      if (c == 0)   throw std::runtime_error(__PRETTY_FUNCTION__);  /*
      // perhaps constructing/copying from a reference just becoming a dangling one */
    while (!count.compare_exchange_weak(c, c+1, std::memory_order_acquire, std::memory_order_relaxed));
    return c+1;
  }

  type dec() noexcept
  {
    auto c = count.fetch_sub(1, std::memory_order_release);
    return c-1;
  }
};



namespace
{
/*! \brief The type of the `adopt_ref` constant. */
struct adopt_ref_t  { constexpr adopt_ref_t() = default; };
}
/*! \brief The tag to be used to prevent `ref_guard` constructor from increasing reference count of the protected object. */
constexpr const adopt_ref_t adopt_ref;

/*! \brief A scoped reference count guard.
    \details Similar to `std::lock_guard` but it is for reference count.
    The target object should provide `ref()` and `unref()` public member functions. */
template< class _T_ >
class ref_guard
{
public: /*types*/
  using target_type = _T_;

public: /*constructors*/
  ~ref_guard()  { t.unref(); }

  explicit ref_guard(target_type &_t) : t(_t)  { t.ref(); }
  explicit ref_guard(target_type &_t, const adopt_ref_t) : t(_t)  {}  /*!< \brief The constructor for being used with `uv::adopt_ref` tag. */

  ref_guard(const ref_guard&) = delete;
  ref_guard& operator =(const ref_guard&) = delete;

  ref_guard(ref_guard&&) = delete;
  ref_guard& operator =(ref_guard&&) = delete;

private: /*data*/
  target_type &t;
};




/* type_storage */
template< typename _T_ >
class type_storage
{
public: /*types*/
  using value_type = typename std::decay< _T_ >::type;
  using storage_type = typename std::aligned_storage< sizeof(value_type), alignof(value_type) >::type;

private: /*data*/
  storage_type storage;

public: /*constructors*/
  ~type_storage()   { value().~value_type(); }
  type_storage()  { new(static_cast< void* >(&storage)) value_type(); }

  type_storage(const type_storage&) = delete;
  type_storage& operator =(const type_storage&) = delete;

  type_storage(type_storage&&) = delete;
  type_storage& operator =(type_storage&&) = delete;

  template< typename... _Args_ > type_storage(_Args_&&... _args)
  {
    new(static_cast< void* >(&storage)) value_type(std::forward< _Args_ >(_args)...);
  }
  type_storage(const value_type &_value)
  {
    new(static_cast< void* >(&storage)) value_type(_value);
  }
  type_storage(value_type &&_value)
  {
    new(static_cast< void* >(&storage)) value_type(std::move(_value));
  }

public: /*interface*/
  const value_type& value() const noexcept  { return *reinterpret_cast< const value_type* >(&storage); }
        value_type& value()       noexcept  { return *reinterpret_cast<       value_type* >(&storage); }
};


/*! \brief A mimic of STL's `std::aligned_union` missed in gcc 4.9.2. */
template< typename... _Ts_ >
using aligned_union = std::aligned_storage< greatest(sizeof(_Ts_)...), greatest(alignof(_Ts_)...) >;


/* union_storage */
template< typename... _Ts_ >
class union_storage   
{
public: /*types*/
  using storage_type = typename aligned_union< _Ts_... >::type;

private: /*data*/
  void (*Destroy)(void*) = nullptr;
  storage_type storage;

public: /*constructors*/
  ~union_storage()  { if (Destroy)  Destroy(&storage); }
  union_storage() = default;

  union_storage(const union_storage&) = delete;
  union_storage& operator =(const union_storage&) = delete;

  union_storage(union_storage&&) = delete;
  union_storage& operator =(union_storage&&) = delete;

  template< typename _T_, typename = std::enable_if_t< is_one_of< _T_, _Ts_... >::value > >
  union_storage(const _T_ &_value)
  {
    using value_type = typename std::decay< _T_ >::type;

    new(static_cast< void* >(&storage)) value_type(_value);
    Destroy = default_destroy< _T_ >::Destroy;
  }
  template< typename _T_, typename = std::enable_if_t< is_one_of< _T_, _Ts_... >::value > >
  union_storage(_T_ &&_value)
  {
    using value_type = typename std::decay< _T_ >::type;

    new(static_cast< void* >(&storage)) value_type(std::move(_value));
    Destroy = default_destroy< _T_ >::Destroy;
  }

public: /*interface*/
  template< typename _T_ >
  typename std::enable_if< is_one_of< _T_, _Ts_... >::value >::type reset()
  {
    using value_type = typename std::decay< _T_ >::type;

    if (Destroy)  { Destroy(&storage); Destroy = nullptr; }
    new(static_cast< void* >(&storage)) value_type();
    Destroy = default_destroy< _T_ >::Destroy;
  }
  template< typename _T_, typename... _Args_ >
  typename std::enable_if< is_one_of< _T_, _Ts_... >::value >::type reset(_Args_&&... _args)
  {
    using value_type = typename std::decay< _T_ >::type;

    if (Destroy)  { Destroy(&storage); Destroy = nullptr; }
    new(static_cast< void* >(&storage)) value_type(std::forward< _Args_ >(_args)...);
    Destroy = default_destroy< _T_ >::Destroy;
  }
  template< typename _T_ >
  typename std::enable_if< is_one_of< _T_, _Ts_... >::value >::type reset(const _T_ &_value)
  {
    using value_type = typename std::decay< _T_ >::type;

    if (static_cast< void* >(&storage) == static_cast< void* >(&_value))  return;

    if (Destroy)  { Destroy(&storage); Destroy = nullptr; }
    new(static_cast< void* >(&storage)) value_type(_value);
    Destroy = default_destroy< _T_ >::Destroy;
  }
  template< typename _T_ >
  typename std::enable_if< is_one_of< _T_, _Ts_... >::value >::type reset(_T_ &&_value)
  {
    using value_type = typename std::decay< _T_ >::type;

    if (static_cast< void* >(&storage) == static_cast< void* >(&_value))  return;

    if (Destroy)  { Destroy(&storage); Destroy = nullptr; }
    new(static_cast< void* >(&storage)) value_type(std::move(_value));
    Destroy = default_destroy< _T_ >::Destroy;
  }

  template< typename _T_, typename = std::enable_if_t< is_one_of< _T_, _Ts_... >::value > >
  const typename std::decay< _T_ >::type& value() const noexcept  { return *reinterpret_cast< const typename std::decay< _T_ >::type* >(&storage); }
  template< typename _T_, typename = std::enable_if_t< is_one_of< _T_, _Ts_... >::value > >
        typename std::decay< _T_ >::type& value()       noexcept  { return *reinterpret_cast<       typename std::decay< _T_ >::type* >(&storage); }
};


//! \}
}


#endif
