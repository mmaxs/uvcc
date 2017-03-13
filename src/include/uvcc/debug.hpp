
#ifndef UVCC_DEBUG__HPP
#define UVCC_DEBUG__HPP
//! \cond

#include <uv.h>
#include <cstdio>


#ifdef UVCC_DEBUG

#define MAKE_LITERAL_STRING_VERBATIM(...)  #__VA_ARGS__
#define MAKE_LITERAL_STRING_AFTER_EXPANDING(...)  MAKE_LITERAL_STRING_VERBATIM(__VA_ARGS__)

#define CONCATENATE_VERBATIM(left, right)  left ## right
#define CONCATENATE_AFTER_EXPANDING(left, right)  CONCATENATE_VERBATIM(left, right)


#define uvcc_debug_log_if(log_condition, printf_args...)  do {\
    if ((log_condition))\
    {\
      fflush(stdout);\
      fprintf(stderr, "[debug]");\
      int n = fprintf(stderr, " " printf_args);\
      if (n-1)\
        fprintf(stderr, "\n");\
      else\
        fprintf(stderr, "log: function=%s file=%s line=%d\n", __PRETTY_FUNCTION__, __FILE__, __LINE__);\
      fflush(stderr);\
    }\
} while (0)

#define uvcc_debug_check_entry(printf_args...)  do {\
    fflush(stdout);\
    fprintf(stderr, "[debug] enter function/block:");\
    int n = fprintf(stderr, " " printf_args);\
    fprintf(stderr, "%sfunction=%s file=%s line=%d\n", (n-1)?": ":"", __PRETTY_FUNCTION__, __FILE__, __LINE__);\
    fflush(stderr);\
} while (0)

#define uvcc_debug_check_exit(printf_args...)  do {\
    fflush(stdout);\
    fprintf(stderr, "[debug] exit from function/block:");\
    int n = fprintf(stderr, " " printf_args);\
    fprintf(stderr, "%sfunction=%s file=%s line=%d\n", (n-1)?": ":"", __PRETTY_FUNCTION__, __FILE__, __LINE__);\
    fflush(stderr);\
} while (0)

#define uvcc_debug_check_condition(condition, context_printf_args...)  do {\
    fflush(stdout);\
    fprintf(stderr, "[debug] condition (%s):", MAKE_LITERAL_STRING_VERBATIM(condition));\
    int n = fprintf(stderr, " " context_printf_args);\
    fprintf(stderr, "%s%s\n", (n-1)?": ":"", (condition)?"true":"false");\
    fflush(stderr);\
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

static
const char* handle_type_name(void *_uv_handle)
{
  const char *ret;
  switch (static_cast< ::uv_handle_t* >(_uv_handle)->type)
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
void print_handle(void *_uv_handle)
{
  auto h = static_cast< ::uv_handle_t* >(_uv_handle);
  fprintf(stderr,
      "[debug] %s handle [0x%08llX]: has_ref=%i is_active=%i is_closing=%i\n",
      handle_type_name(h), (uintptr_t)h, ::uv_has_ref(h), ::uv_is_active(h), ::uv_is_closing(h)
  );
  fflush(stderr);
}

static
void print_loop_handles(void *_uv_loop_handle)
{
  auto walk_cb = [](::uv_handle_t *_h, void*){ print_handle(_h); };
  ::uv_walk(static_cast< ::uv_loop_t* >(_uv_loop_handle), static_cast< ::uv_walk_cb >(walk_cb), nullptr);
}


}
}
#pragma GCC diagnostic pop

//! \endcond
#endif
