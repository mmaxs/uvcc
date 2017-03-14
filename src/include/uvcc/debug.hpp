
#ifndef UVCC_DEBUG__HPP
#define UVCC_DEBUG__HPP
//! \cond

#include <uv.h>
#include <cstddef>  // ptrdiff_t
#include <cstdio>


#ifdef UVCC_DEBUG

#define MAKE_LITERAL_STRING_VERBATIM(...)  #__VA_ARGS__
#define MAKE_LITERAL_STRING_AFTER_EXPANDING(...)  MAKE_LITERAL_STRING_VERBATIM(__VA_ARGS__)

#define CONCATENATE_VERBATIM(left, right)  left ## right
#define CONCATENATE_AFTER_EXPANDING(left, right)  CONCATENATE_VERBATIM(left, right)


#define uvcc_debug_log_if(log_condition, printf_args...)  do {\
    if ((log_condition))\
    {\
      std::fflush(stdout);\
      std::fprintf(stderr, "[debug]");\
      int n = std::fprintf(stderr, " " printf_args);\
      if (n > 1)\
        std::fprintf(stderr, "\n");\
      else\
        std::fprintf(stderr, "log: function=%s file=%s line=%i\n", __PRETTY_FUNCTION__, __FILE__, __LINE__);\
      std::fflush(stderr);\
    }\
} while (0)

#define uvcc_debug_function_enter(printf_args...)  do {\
    std::fflush(stdout);\
    std::fprintf(stderr, "[debug] enter function %s", __PRETTY_FUNCTION__);\
    if (sizeof(MAKE_LITERAL_STRING_AFTER_EXPANDING(printf_args)) > 0)  std::fprintf(stderr, ": " printf_args);\
    std::fprintf(stderr, "\n");\
    std::fflush(stderr);\
} while (0)

#define uvcc_debug_function_return(printf_args...)  do {\
    std::fflush(stdout);\
    std::fprintf(stderr, "[debug] return from function %s", __PRETTY_FUNCTION__);\
    if (sizeof(MAKE_LITERAL_STRING_AFTER_EXPANDING(printf_args)) > 0)  std::fprintf(stderr, ": " printf_args);\
    std::fprintf(stderr, "\n");\
    std::fflush(stderr);\
} while (0)

#define uvcc_debug_condition(condition, context_printf_args...)  do {\
    bool c = (condition);\
    std::fflush(stdout);\
    std::fprintf(stderr, "[debug] condition (%s):", MAKE_LITERAL_STRING_VERBATIM(condition));\
    int n = std::fprintf(stderr, " " context_printf_args);\
    std::fprintf(stderr, "%s-> %s\n", (n-1)?" ":"", c?"true":"false");\
    std::fflush(stderr);\
} while (0)

#define uvcc_debug_do_if(log_condition, operators...)  do {\
    if ((log_condition))\
    {\
      operators;\
    }\
} while(0)

#else

#define uvcc_debug_log_if(log_condition, ...)  (void)(log_condition)
#define uvcc_debug_check_entry(...)
#define uvcc_debug_check_exit(...)
#define uvcc_debug_check_condition(condition, ...)  (void)(condition)
#define uvcc_debug_do_if(log_condition, ...)  (void)(log_condition)

#endif


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
namespace uv
{
namespace debug
{


template< class _UVCC_CLASS_ >
typename _UVCC_CLASS_::instance* instance(_UVCC_CLASS_ &_var) noexcept
{
  return _UVCC_CLASS_::instance::from(static_cast< typename _UVCC_CLASS_::uv_t* >(_var));
}

static
const char* handle_type_name(::uv_handle_t *_uv_handle) noexcept
{
  const char *ret;
  switch (_uv_handle->type)
  {
#define XX(X, x) case UV_##X: ret = #x; break;
    UV_HANDLE_TYPE_MAP(XX)
#undef XX
    case UV_FILE: ret = "file"; break;
    default: ret = "<unknown>"; break;
  }
  return ret;
}

static
void print_handle(::uv_handle_t* _uv_handle) noexcept
{
  std::fprintf(stderr,
      "[debug] %s handle [0x%08tX]: has_ref=%i is_active=%i is_closing=%i\n",
      handle_type_name(_uv_handle), (ptrdiff_t)_uv_handle,
      ::uv_has_ref(_uv_handle), ::uv_is_active(_uv_handle), ::uv_is_closing(_uv_handle)
  );
  std::fflush(stderr);
}

static
void print_loop_handles(::uv_loop_t *_uv_loop)
{
  auto walk_cb = [](::uv_handle_t *_h, void*){ print_handle(_h); };
  std::fprintf(stderr, "[debug] handles associated with loop [0x%08tX]: {\n", (ptrdiff_t)_uv_loop);
  ::uv_walk(static_cast< ::uv_loop_t* >(_uv_loop), static_cast< ::uv_walk_cb >(walk_cb), nullptr);
  std::fprintf(stderr, "[debug] }\n");
}


}
}
#pragma GCC diagnostic pop

//! \endcond
#endif
