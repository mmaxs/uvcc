
#ifndef UVCC_UTILITY__HPP
#define UVCC_UTILITY__HPP

#include <type_traits>  // is_void is_convertible enable_if_t decay common_type aligned_storage
#include <atomic>       // atomic memory_order_*
#include <utility>      // forward() move()
#include <memory>       // addressof()
#include <stdexcept>    // runtime_error
#include <typeinfo>     //


namespace uv
{
/*! \defgroup g__utility Utility structures and definitions
    \brief The utility definitions being used throughout the library. */
//! \{


/*! \brief The analogue of the `std::default_delete`.
    \details It also provides the static member function `void Delete(void*)`
    holding the proper delete operator for the type `_T_`. The client code then
    can store a pointer to this function being alike a pointer to the virtual
    delete operator for implementing the run-time data polymorphism.
    */
template< typename _T_ > struct default_delete
{
  using value_type = typename std::decay< _T_ >::type;

  constexpr default_delete() noexcept = default;

  // we can delete an object through the polymorphic pointer to its base class
  template<
      typename _U_,
      typename = std::enable_if_t< std::is_convertible< _U_*, _T_* >::value >
  > default_delete(const default_delete< _U_ >&) noexcept   {}

  void operator ()(value_type *_) const   { Delete(_); }

  // not for use for polymorphic deleting without the operator () above
  static void Delete(void *_)
  {
    static_assert(!std::is_void< value_type >::value, "void type");
    static_assert(sizeof(value_type) > 0, "incomplete type");
    delete static_cast< value_type* >(_);
    //fprintf(stderr, "%s\n", __PRETTY_FUNCTION__); fflush(stderr);
  }
};


/*! \brief The analogue of the `uv::default_delete` but for the type destructor only.
    \details Provides the static member function `void Destroy(void*)` holding
    the proper destructor call for the type `_T_`. The client code then can store
    a pointer to this function  being alike a pointer to the virtual destructor
    for implementing the run-time data polymorphism.
    */
template< typename _T_ > struct default_destroy
{
  using value_type = typename std::decay< _T_ >::type;

  constexpr default_destroy() noexcept = default;

  // we can destroy an object through the polymorphic pointer to its base class
  template<
      typename _U_,
      typename = std::enable_if_t< std::is_convertible< _U_*, _T_* >::value >
  > default_destroy(const default_destroy< _U_ >&) noexcept   {}

  void operator ()(value_type *_) const   { Destroy(_); }

  // not for use for polymorphic destroying without the operator () above
  static void Destroy(void *_)
  {
    static_assert(!std::is_void< value_type >::value, "void type");
    static_assert(sizeof(value_type) > 0, "incomplete type");
    static_cast< value_type* >(_)->~value_type();
    //fprintf(stderr, "%s\n", __PRETTY_FUNCTION__); fflush(stderr);
  }
};



/*! \defgroup g__variadic Dealing with type lists and parameter packs */
//! \{
#define BUGGY_DOXYGEN
#undef BUGGY_DOXYGEN

/*! \brief The type of an absent entity.
    \details Used to substitute nonexistent and `void` types in various template expressions e.g. like `sizeof()`, `alignof()`. */
struct null_t  { null_t() = delete; };


/*! \brief Checks if a type `_T_` belongs to a type list `_Ts_`.
    \details Provides the constexpr `value` that is equal to the index of the given type `_T_`
    in the type list `_Ts_` starting from **1** up to `sizeof...(_Ts_)` or **0** otherwise. */
template< typename _T_, typename... _Ts_ > struct is_one_of;
//! \cond
template< typename _T_ > struct is_one_of< _T_ >
{
  constexpr static const std::size_t value = 0;
};
template< typename _T_, typename... _Ts_ > struct is_one_of< _T_, _T_, _Ts_... >
{
  constexpr static const std::size_t value = 1;
};
template< typename _T_, typename _U_, typename... _Ts_ > struct is_one_of< _T_, _U_, _Ts_... >
{
private:
  constexpr static const std::size_t value_ = is_one_of< _T_, _Ts_... >::value;
public:
  constexpr static const std::size_t value = value_ ? value_+1 : 0;
};
//! \endcond

/*! \brief Checks if a type `_T_` is convertible to one of the types from the type list `_Ts_`.
    \details Provides the constexpr `value` that is equal to the index of the type from the type
    list `_Ts_` which the given type `_T_` can be converted to by using implicit conversion.
    The index starts from **1** up to `sizeof...(_Ts_)` or is **0** otherwise. */
template< typename _T_, typename... _Ts_ > struct is_convertible_to_one_of;
//! \cond
template< typename _T_ > struct is_convertible_to_one_of< _T_ >
{
  constexpr static const std::size_t value = 0;
};
template< typename _T_, typename _U_, typename... _Ts_ > struct is_convertible_to_one_of< _T_, _U_, _Ts_... >
{
private:
  constexpr static const std::size_t value_ = is_convertible_to_one_of< _T_, _Ts_... >::value;
public:
  constexpr static const std::size_t value = std::is_convertible< _T_, _U_ >::value ? 1 : (value_ ? value_+1 : 0);
};
//! \endcond

/*! \brief Provides a typedef member `type` equal to the type from the type list `_Ts_` at index `_index_`.
    The index should start from **1** up to `sizeof...(_Ts_)`. */
template< std::size_t _index_, typename... _Ts_ > struct type_at;
//! \cond
template< typename... _Ts_ > struct type_at< 0, _Ts_... >  {};  // index in type lists starts from 1 rather than from 0
template< std::size_t _index_ > struct type_at< _index_ >  {};  // index is behind the length of the type list
template< typename _T_, typename... _Ts_ > struct type_at< 1, _T_, _Ts_...>  { using type = _T_; };
template< std::size_t _index_, typename _T_, typename... _Ts_ > struct type_at< _index_, _T_, _Ts_...>
{ using type = typename type_at< _index_-1, _Ts_... >::type; };
//! \endcond


//! \cond
template< typename _T_ > constexpr const _T_& greatest(const _T_& _v)  { return _v; }
//! \endcond
/*! \brief Intended to be used instead of `constexpr T max(std::initializer_list<T>)`...
    \details ...if the latter is not defined being `constexpr` in the current STL version and therefore
    cannot be employed at compile-time. Does not require the arguments being of the same type and using
    the `std::initializer_list` curly braces when there are more than two arguments. */
template< typename _T_, typename... _Ts_ > constexpr const _T_& greatest(const _T_& _v, const _Ts_&... _vs)
{ return _v < greatest(_vs...) ? greatest(_vs...) : _v; }

//! \cond
template< typename _T_ > constexpr const _T_& lowest(const _T_& _v)  { return _v; }
//! \endcond
/*! \brief The counterpart of `greatest()` */
template< typename _T_, typename... _Ts_ > constexpr const _T_& lowest(const _T_& _v, const _Ts_&... _vs)
{ return lowest(_vs...) < _v ? lowest(_vs...) : _v; }


//! \cond
template< typename _T_ > constexpr auto sum(const _T_& _v) -> typename std::decay< _T_ >::type  { return _v; }
//! \endcond
/*! \brief Primarily intended for summation of values from parameter packs if fold expressions are not supported. */
template< typename _T_, typename... _Ts_ > constexpr auto sum(const _T_& _v, const _Ts_&... _vs) -> typename std::common_type< _T_, _Ts_... >::type
{ return _v + sum(_vs...); }

// \}



/*! \brief A reference counter with atomic increment/decrement.
    \details The default constructor creates a new `ref_count` object with the count value = **1**.

    Atomic operations on the `ref_count` object provide the following memory ordering semantics:
     Member function | Memory ordering
    :----------------|:---------------:
     `value()`       | acquire
     `inc()`         | relaxed
     `dec()`         | release

    Thus the client code can use `value()` function to check the current number of the variables
    referencing a counted object and be sure to be _synchronized-with_ the last `dec()` operation
    (i.e. to see all the results of non-atomic memory changes _happened-before_ the last `dec()`
    operation which should normally occurs when a variable of the counted object is destroyed on
    going out of its scope).

    `inc()` throws `std::runtime_error` if the current value to be incremented is **0** as this
    circumstance is considered as a variable of the counted object is being constructed/copied
    from a reference just becoming a dangling one.
*/
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
    auto c = count.load(std::memory_order_relaxed);
    do
      if (c == 0)   throw std::runtime_error(__PRETTY_FUNCTION__);  /*
      // perhaps constructing/copying from a reference just becoming a dangling one */
    while (!count.compare_exchange_weak(c, c+1, std::memory_order_relaxed, std::memory_order_relaxed));
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

/*! \brief A scoped reference counting guard.
    \details Similar to `std::lock_guard` but it is for reference counting.
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



/*! \brief A wrapper providing the feature of being a _standard layout type_ for the given type `_T_`.
    \note All the member functions creating a new value in the storage from their arguments
    use the _curly brace initialization_. */
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
  type_storage()  { new(static_cast< void* >(&storage)) value_type{}; }

  type_storage(const type_storage&) = delete;
  type_storage& operator =(const type_storage&) = delete;

  type_storage(type_storage&&) = delete;
  type_storage& operator =(type_storage&&) = delete;

  template< typename... _Args_ > type_storage(_Args_&&... _args)
  {
    new(static_cast< void* >(&storage)) value_type{std::forward< _Args_ >(_args)...};
  }
  type_storage(const value_type &_value)
  {
    new(static_cast< void* >(&storage)) value_type{_value};
  }
  type_storage(value_type &&_value)
  {
    new(static_cast< void* >(&storage)) value_type{std::move(_value)};
  }

public: /*interface*/
  const value_type& value() const noexcept  { return *reinterpret_cast< const value_type* >(&storage); }
        value_type& value()       noexcept  { return *reinterpret_cast<       value_type* >(&storage); }
};



/*! \brief A mimic of STL's `std::aligned_union` missed in gcc 4.9.2. */
template< typename... _Ts_ >
using aligned_union = std::aligned_storage< greatest(sizeof(_Ts_)...), greatest(alignof(_Ts_)...) >;


/*! \brief A tagged union that provide a storage space being a _standard layout type_
    suited for all its type variants specified in the type list `_Ts_`.
    \details Only values from the specified set of types `_Ts_` are created and stored in the union
    even though the values of any types that are implicitly convertible to one of the types
    from `_Ts_` list are acceptable for copy- or move-initialization `reset()` functions.
    \note All the member functions creating a new value in the union from their arguments
    use the _curly brace initialization_. */
template< typename... _Ts_ >
class union_storage   
{
public: /*types*/
  using storage_type = typename aligned_union< _Ts_... >::type;

private: /*data*/
  const std::type_info *type_tag = nullptr;
  void (*Destroy)(void*) = nullptr;
  storage_type storage;

public: /*constructors*/
  ~union_storage()  { destroy(); }
  union_storage() = default;  /*!< \brief Create an uninitialized union storage. */

  union_storage(const union_storage&) = delete;
  union_storage& operator =(const union_storage&) = delete;

  union_storage(union_storage&&) = delete;
  union_storage& operator =(union_storage&&) = delete;

  /*! \brief Create a union with a copy-initialized value from the specified one. */
  template< typename _T_, typename = std::enable_if_t< is_convertible_to_one_of< _T_, _Ts_... >::value > >
  union_storage(const _T_ &_value)
  {
    constexpr const std::size_t tag = is_convertible_to_one_of< _T_, _Ts_... >::value;
    using type = typename std::decay< typename type_at< tag, _Ts_... >::type >::type;

    new(static_cast< void* >(&storage)) type{_value};
    Destroy = default_destroy< type >::Destroy;
    type_tag = &typeid(type);
  }
  /*! \brief Create a union with a move-initialized value from the specified value. */
  template< typename _T_, typename = std::enable_if_t< is_convertible_to_one_of< _T_, _Ts_... >::value > >
  union_storage(_T_ &&_value)
  {
    constexpr const std::size_t tag = is_convertible_to_one_of< _T_, _Ts_... >::value;
    using type = typename std::decay< typename type_at< tag, _Ts_... >::type >::type;

    new(static_cast< void* >(&storage)) type{std::move(_value)};
    Destroy = default_destroy< type >::Destroy;
    type_tag = &typeid(type);
  }

private: /*functions*/
  void destroy() noexcept
  {
    if (Destroy)  Destroy(&storage);
    Destroy = nullptr;
    type_tag = nullptr;
  }

public: /*interface*/
  /*! \name Functions to reinitialize the union storage:
      \note The previously stored value is destroyed. */
  //! \{
  /*! \brief Reinitialize the union storage with a default created value of the one of the type from `_Ts_` list
      that the specified type `_T_` is convertible to. */
  template< typename _T_ >
  typename std::enable_if< is_convertible_to_one_of< _T_, _Ts_... >::value >::type reset()
  {
    constexpr const std::size_t tag = is_convertible_to_one_of< _T_, _Ts_... >::value;
    using type = typename std::decay< typename type_at< tag, _Ts_... >::type >::type;

    destroy();

    new(static_cast< void* >(&storage)) type{};
    Destroy = default_destroy< type >::Destroy;
    type_tag = &typeid(type);
  }
  /*! \brief Ditto but the value is created from the arguments forwarded to the type constructor. */
  template< typename _T_, typename... _Args_ >
  typename std::enable_if< is_convertible_to_one_of< _T_, _Ts_... >::value >::type reset(_Args_&&... _args)
  {
    constexpr const std::size_t tag = is_convertible_to_one_of< _T_, _Ts_... >::value;
    using type = typename std::decay< typename type_at< tag, _Ts_... >::type >::type;

    destroy();

    new(static_cast< void* >(&storage)) type{std::forward< _Args_ >(_args)...};
    Destroy = default_destroy< type >::Destroy;
    type_tag = &typeid(type);
  }
  /*! \brief Ditto but the value is copy-created from the specified argument. */
  template< typename _T_ >
  typename std::enable_if< is_convertible_to_one_of< _T_, _Ts_... >::value >::type reset(const _T_ &_value)
  {
    constexpr const std::size_t tag = is_convertible_to_one_of< _T_, _Ts_... >::value;
    using type = typename std::decay< typename type_at< tag, _Ts_... >::type >::type;

    if (reinterpret_cast< type* >(&storage) == static_cast< type* >(std::addressof(_value)))  return;  // cast _T_* to type*

    destroy();

    new(static_cast< void* >(&storage)) type{_value};
    Destroy = default_destroy< type >::Destroy;
    type_tag = &typeid(type);
  }
  /*! \brief Ditto but the value is move-created from the specified argument. */
  template< typename _T_ >
  typename std::enable_if< is_convertible_to_one_of< _T_, _Ts_... >::value >::type reset(_T_ &&_value)
  {
    constexpr const std::size_t tag = is_convertible_to_one_of< _T_, _Ts_... >::value;
    using type = typename std::decay< typename type_at< tag, _Ts_... >::type >::type;

    if (reinterpret_cast< type* >(&storage) == static_cast< type* >(std::addressof(_value)))  return;  // cast _T_* to type*

    destroy();

    new(static_cast< void* >(&storage)) type{std::move(_value)};
    Destroy = default_destroy< type >::Destroy;
    type_tag = &typeid(type);
  }
  //! \}

  /*! \brief The type tag of the stored value.
      \details It's just a pointer referring to the static global constant object returned by the
      `typeid()` operator for the type of the value currently stored in this `union_storage` variable. */
  const std::type_info* tag() const noexcept  { return type_tag; }

  /*! \name Functions to get the value stored in the union: */
  //! \{
  template< typename _T_, typename = std::enable_if_t< is_one_of< _T_, _Ts_... >::value > >
  const typename std::decay< _T_ >::type& get() const noexcept  { return *reinterpret_cast< const typename std::decay< _T_ >::type* >(&storage); }
  template< typename _T_, typename = std::enable_if_t< is_one_of< _T_, _Ts_... >::value > >
        typename std::decay< _T_ >::type& get()       noexcept  { return *reinterpret_cast<       typename std::decay< _T_ >::type* >(&storage); }
  //! \}
};


//! \}
}


#endif
