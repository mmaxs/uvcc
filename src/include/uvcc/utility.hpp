
#ifndef UVCC_UTILITY__HPP
#define UVCC_UTILITY__HPP

//#include "uvcc/debug.hpp"

#include <cstddef>      // nullptr_t
#include <type_traits>  // is_void is_convertible enable_if_t decay common_type aligned_storage
#include <atomic>       // atomic memory_order_* atomic_flag ATOMIC_FLAG_INIT
#include <utility>      // forward() move()
#include <memory>       // addressof()
#include <stdexcept>    // runtime_error
#include <typeinfo>     // type_info


namespace uv
{
/*! \defgroup doxy_group__utility  Utility structures and definitions
    \brief The utility definitions to be used throughout the library. */
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
    fprintf(stderr, "0x%08tX\n", (ptrdiff_t)_);
    //uvcc_debug_function_return();
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
    //uvcc_debug_function_return();
  }
};



/*! \defgroup doxy_group__variadic  Dealing with type lists and parameter packs */
//! \addtogroup doxy_group__variadic
//! \{

/*! \brief The type of an absent entity. Cannot be instantiated.
    \details Used to substitute nonexistent and `void` types in various template metaprogramming expressions.
    It may also service as a guard that ensure a template is not instantiated in unexpected manner. */
struct null_t  { null_t() = delete; };
/*! \brief The type of an empty entity. Like `null_t`, but is allowed to be instantiated that might be feasible for code simplicity. */
struct empty_t { constexpr empty_t() = default; };


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
template< typename _T_ >
constexpr
inline
auto greatest(_T_&& _v) -> decltype(_v)  // return _T_&& or _T_&
{
  return std::forward< _T_ >(_v);
}
//! \endcond
/*! \brief Intended to be used instead of `constexpr T max(std::initializer_list<T>)`...
    \details ...if the latter is not defined of being `constexpr` in the current STL version and therefore
    cannot be employed at compile-time. Does not require the arguments to be of the same type and using
    the `std::initializer_list` curly braces when there are more than two arguments.

    While promoting to a common resulting type a temporary local value can be implicitly created by the compiler,
    therefore the function is not able to safely return a reference type result, or else the
    `'warning: returning reference to temporary'` is generated in some use cases. */
template< typename _T_, typename... _Ts_ >
constexpr
inline
auto greatest(_T_&& _v, _Ts_&&... _vs) -> std::common_type_t< decltype(_v), decltype(_vs)... >  // thouhg it is anyway a decayed type
{
  return _v < greatest(std::forward< _Ts_ >(_vs)...) ? greatest(std::forward< _Ts_ >(_vs)...) : _v;
}

//! \cond
template< typename _T_ >
constexpr
inline
auto lowest(_T_&& _v) -> decltype(_v)  // return _T_&& or _T_&
{
  return std::forward< _T_ >(_v);
}
//! \endcond
/*! \brief The counterpart of `greatest()` */
template< typename _T_, typename... _Ts_ >
constexpr
inline
auto lowest(_T_&& _v, _Ts_&&... _vs) -> std::common_type_t< decltype(_v), decltype(_vs)... >  // though it is anyway a decayed type
{
  return lowest(std::forward< _Ts_ >(_vs)...) < _v ? lowest(std::forward< _Ts_ >(_vs)...) : _v;
}


//! \cond
template< typename _T_ >
constexpr
auto sum(_T_&& _v) -> decltype(_v)  // return _T_&& or _T_&
{
  return std::forward< _T_ >(_v);
}
//! \endcond
/*! \brief Primarily intended for summation of values from parameter packs if fold expressions are not supported. */
template< typename _T_, typename... _Ts_ >
constexpr
auto sum(_T_&& _v, _Ts_&&... _vs) -> std::common_type_t< _T_, _Ts_... >
{
  return _v + sum(std::forward< _Ts_ >(_vs)...);
}

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
      if (c == 0)  // perhaps constructing/copying from a reference just becoming a dangling one
        throw std::runtime_error(__PRETTY_FUNCTION__);
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

private: /*data*/
  target_type &t;

public: /*constructors*/
  ~ref_guard()  { t.unref(); }

  explicit ref_guard(target_type &_t) : t(_t)  { t.ref(); }
  explicit ref_guard(target_type &_t, const adopt_ref_t) : t(_t)  {}  /*!< \brief The constructor to be used with `uv::adopt_ref` tag. */

  ref_guard(const ref_guard&) = delete;
  ref_guard& operator =(const ref_guard&) = delete;

  ref_guard(ref_guard&&) = delete;
  ref_guard& operator =(ref_guard&&) = delete;
};



/*! \brief A simple spinlock mutex built around `std::atomic_flag`. */
class spinlock
{
private: /*data*/
  std::atomic_flag flag;

public: /*constructors*/
  ~spinlock() = default;
  spinlock() : flag(ATOMIC_FLAG_INIT)  {}

  spinlock(const spinlock&) = delete;
  spinlock& operator =(const spinlock&) = delete;

  spinlock(spinlock&&) = delete;
  spinlock& operator =(spinlock&&) = delete;

public: /*interface*/
  void lock(std::memory_order _o = std::memory_order_acquire) noexcept
  {
    while (flag.test_and_set(_o));
  }

  void unlock(std::memory_order _o = std::memory_order_release) noexcept
  {
    flag.clear(_o);
  }
};



/*! \brief A wrapper around `std::aligned_storage< _LEN_, _ALIGN_ >::type` that simplifies
    initializing the provided storage space, getting from it, setting it to, and automatic destroying it from
    objects of any type fitting to the given size and alignment requirements.
    \note All the member functions creating a new value in the storage from their arguments
    use the _curly brace initialization_. */
template< std::size_t _LEN_, std::size_t _ALIGN_ >
class aligned_storage
{
private: /*data*/
  const std::type_info *type_tag = nullptr;
  void (*Destroy)(void*) = nullptr;
  typename std::aligned_storage< _LEN_, _ALIGN_ >::type storage;

public: /*constructors*/
  ~aligned_storage()  { destroy(); }
  aligned_storage() = default;  /*!< \brief Create an uninitialized storage. */

  aligned_storage(const aligned_storage&) = delete;
  aligned_storage& operator =(const aligned_storage&) = delete;

  aligned_storage(aligned_storage&&) = delete;
  aligned_storage& operator =(aligned_storage&&) = delete;

  /*! \brief Create a storage space with a copy-initialized value from the specified one. */
  template< typename _T_ > aligned_storage(const _T_ &_value)
  {
    using type = typename std::decay< _T_ >::type;
    static_assert(sizeof(type) <= _LEN_, "insufficient storage size");
    static_assert(alignof(type) <= _ALIGN_, "not adjusted storage alignment");

    new(static_cast< void* >(&storage)) type{ _value };
    Destroy = default_destroy< type >::Destroy;
    type_tag = &typeid(type);
  }
  /*! \brief Create a storage space with a move-initialized value from the specified value. */
  template< typename _T_ > aligned_storage(_T_ &&_value)
  {
    using type = typename std::decay< _T_ >::type;
    static_assert(sizeof(type) <= _LEN_, "insufficient storage size");
    static_assert(alignof(type) <= _ALIGN_, "not adjusted storage alignment");

    new(static_cast< void* >(&storage)) type{ std::move(_value) };
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
  /*! \name Functions to reinitialize the storage space:
      \note The previously stored value is destroyed. */
  //! \{
  /*! \brief Reinitialize the storage space with a default value of the specified type. */
  template< typename _T_ > void reset()
  {
    using type = typename std::decay< _T_ >::type;
    static_assert(sizeof(type) <= _LEN_, "insufficient storage size");
    static_assert(alignof(type) <= _ALIGN_, "not adjusted storage alignment");

    destroy();

    new(static_cast< void* >(&storage)) type{};
    Destroy = default_destroy< type >::Destroy;
    type_tag = &typeid(type);
  }
  /*! \brief Ditto but the value is created from the arguments forwarded to the type constructor. */
  template< typename _T_, typename... _Args_ > void reset(_Args_&&... _args)
  {
    using type = typename std::decay< _T_ >::type;
    static_assert(sizeof(type) <= _LEN_, "insufficient storage size");
    static_assert(alignof(type) <= _ALIGN_, "not adjusted storage alignment");

    destroy();

    new(static_cast< void* >(&storage)) type{ std::forward< _Args_ >(_args)... };
    Destroy = default_destroy< type >::Destroy;
    type_tag = &typeid(type);
  }
  /*! \brief Ditto but the value is copy-created from the specified argument. */
  template< typename _T_ > void reset(const _T_ &_value)
  {
    using type = typename std::decay< _T_ >::type;
    static_assert(sizeof(type) <= _LEN_, "insufficient storage size");
    static_assert(alignof(type) <= _ALIGN_, "not adjusted storage alignment");

    if (reinterpret_cast< type* >(&storage) == std::addressof(static_cast< const type& >(_value)))  return;  // cast _T_& to type&

    destroy();

    new(static_cast< void* >(&storage)) type{ _value };
    Destroy = default_destroy< type >::Destroy;
    type_tag = &typeid(type);
  }
  /*! \brief Ditto but the value is move-created from the specified argument. */
  template< typename _T_ > void reset(_T_ &&_value)
  {
    using type = typename std::decay< _T_ >::type;
    static_assert(sizeof(type) <= _LEN_, "insufficient storage size");
    static_assert(alignof(type) <= _ALIGN_, "not adjusted storage alignment");

    if (reinterpret_cast< type* >(&storage) == std::addressof(static_cast< type& >(_value)))  return;  // cast _T_& to type&

    destroy();

    new(static_cast< void* >(&storage)) type{ std::move(_value) };
    Destroy = default_destroy< type >::Destroy;
    type_tag = &typeid(type);
  }
  //! \}

  /*! \name Functions to get the value that this storage space is holding: */
  //! \{
  template< typename _T_ >
  const typename std::decay< _T_ >::type& get() const noexcept
  { return *reinterpret_cast< const typename std::decay< _T_ >::type* >(&storage); }
  template< typename _T_ >
        typename std::decay< _T_ >::type& get()       noexcept
  { return *reinterpret_cast<       typename std::decay< _T_ >::type* >(&storage); }
  //! \}

  /*! \brief The type tag of the stored value.
      \details It's just a pointer referring to the static global constant object returned by the
      `typeid()` operator for the type of the value currently stored in this variable.
      `nullptr` is returned if the storage space is not initialized and is not holding any value. */
  const std::type_info* tag() const noexcept  { return type_tag; }

public: /*conversion operators*/
  explicit operator bool() const noexcept  { return (tag() != nullptr); }  /*!< \brief Equivalent to `(tag() != nullptr)`. */
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
    new(static_cast< void* >(&storage)) value_type{ std::forward< _Args_ >(_args)... };
  }
  type_storage(const value_type &_value)
  {
    new(static_cast< void* >(&storage)) value_type{ _value };
  }
  type_storage(value_type &&_value)
  {
    new(static_cast< void* >(&storage)) value_type{ std::move(_value) };
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

    new(static_cast< void* >(&storage)) type{ _value };
    Destroy = default_destroy< type >::Destroy;
    type_tag = &typeid(type);
  }
  /*! \brief Create a union with a move-initialized value from the specified value. */
  template< typename _T_, typename = std::enable_if_t< is_convertible_to_one_of< _T_, _Ts_... >::value > >
  union_storage(_T_ &&_value)
  {
    constexpr const std::size_t tag = is_convertible_to_one_of< _T_, _Ts_... >::value;
    using type = typename std::decay< typename type_at< tag, _Ts_... >::type >::type;

    new(static_cast< void* >(&storage)) type{ std::move(_value) };
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
  /*! \brief Reinitialize the union storage with a default value of the one of the type from `_Ts_` list
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

    new(static_cast< void* >(&storage)) type{ std::forward< _Args_ >(_args)... };
    Destroy = default_destroy< type >::Destroy;
    type_tag = &typeid(type);
  }
  /*! \brief Ditto but the value is copy-created from the specified argument. */
  template< typename _T_ >
  typename std::enable_if< is_convertible_to_one_of< _T_, _Ts_... >::value >::type reset(const _T_ &_value)
  {
    constexpr const std::size_t tag = is_convertible_to_one_of< _T_, _Ts_... >::value;
    using type = typename std::decay< typename type_at< tag, _Ts_... >::type >::type;

    if (reinterpret_cast< type* >(&storage) == std::addressof(static_cast< const type& >(_value)))  return;  // cast _T_& to type&

    destroy();

    new(static_cast< void* >(&storage)) type{ _value };
    Destroy = default_destroy< type >::Destroy;
    type_tag = &typeid(type);
  }
  /*! \brief Ditto but the value is move-created from the specified argument. */
  template< typename _T_ >
  typename std::enable_if< is_convertible_to_one_of< _T_, _Ts_... >::value >::type reset(_T_ &&_value)
  {
    constexpr const std::size_t tag = is_convertible_to_one_of< _T_, _Ts_... >::value;
    using type = typename std::decay< typename type_at< tag, _Ts_... >::type >::type;

    if (reinterpret_cast< type* >(&storage) == std::addressof(static_cast< type& >(_value)))  return;  // cast _T_& to type&

    destroy();

    new(static_cast< void* >(&storage)) type{ std::move(_value) };
    Destroy = default_destroy< type >::Destroy;
    type_tag = &typeid(type);
  }
  //! \}

  /*! \name Functions to get the value stored in the union: */
  //! \{
  template< typename _T_, typename = std::enable_if_t< is_one_of< _T_, _Ts_... >::value > >
  const typename std::decay< _T_ >::type& get() const noexcept
  { return *reinterpret_cast< const typename std::decay< _T_ >::type* >(&storage); }
  template< typename _T_, typename = std::enable_if_t< is_one_of< _T_, _Ts_... >::value > >
        typename std::decay< _T_ >::type& get()       noexcept
  { return *reinterpret_cast<       typename std::decay< _T_ >::type* >(&storage); }
  //! \}

  /*! \brief The type tag of the stored value.
      \details It's just a pointer referring to the static global constant object returned by the
      `typeid()` operator for the type of the value currently stored in this `union_storage` variable.
      `nullptr` is returned if the union is not initialized and is not storing any value. */
  const std::type_info* tag() const noexcept  { return type_tag; }

public: /*conversion operators*/
  explicit operator bool() const noexcept  { return (tag() != nullptr); }  /*!< \brief Equivalent to `(tag() != nullptr)`. */
};



/*! \brief The analogue of `std::unique_ptr` that managed object type is not defined at compile time and can be varied. */
class any_ptr
{
private: /*data*/
  const std::type_info *type_tag = nullptr;
  void (*Delete)(void*) = nullptr;
  void *ptr = nullptr;

public: /*constructors*/
  ~any_ptr()  { destroy(); }
  constexpr any_ptr() = default;

  any_ptr(const any_ptr&) = delete;
  any_ptr& operator =(const any_ptr&) = delete;

  any_ptr(any_ptr &&_that) noexcept : type_tag(_that.type_tag), Delete(_that.Delete), ptr(_that.ptr)  { _that.release(); }
  any_ptr& operator =(any_ptr &&_that)
  {
    if (ptr != _that.ptr)
    {
      destroy();
      new (this) any_ptr(std::move(_that));
    }
    return *this;
  }

  constexpr any_ptr(std::nullptr_t) noexcept  {}
  any_ptr& operator =(std::nullptr_t)  { destroy(); return *this; }

  template< typename _T_ > explicit any_ptr(_T_* &&_ptr) noexcept
  {
    ptr = _ptr;
    Delete = default_delete< _T_ >::Delete;
    type_tag = &typeid(_T_);
    _ptr = nullptr;
  }

private: /*functions*/
  void destroy()
  {
    if (Delete)  Delete(ptr);
    Delete = nullptr;
    type_tag = nullptr;
  }

public: /*intreface*/
  void* release() noexcept
  {
    void *p = ptr;
    ptr = nullptr; Delete = nullptr; type_tag = nullptr;
    return p;
  }

  void reset(std::nullptr_t _ptr = nullptr)  { destroy(); }
  template< typename _T_ > void reset(_T_* &&_ptr)
  {
    if (ptr == _ptr)  return;
    destroy();
    new (this) any_ptr(std::move(_ptr));
  }

  template< typename _T_ = void >
  const typename std::decay< _T_ >::type* get() const noexcept
  { return static_cast< const typename std::decay< _T_ >::type* >(ptr); }
  template< typename _T_ = void >
        typename std::decay< _T_ >::type* get()       noexcept
  { return static_cast<       typename std::decay< _T_ >::type* >(ptr); }

  const std::type_info* tag() const noexcept  { return type_tag; }

public: /*conversion operators*/
  explicit operator bool() const noexcept  { return (ptr != nullptr); }  /*!< \brief Equivalent to `(get() != nullptr)`. */
};


//! \}
}


#endif
