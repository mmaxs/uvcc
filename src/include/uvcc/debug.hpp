
#ifndef UVCC_DEBUG__HPP
#define UVCC_DEBUG__HPP
#ifdef DEBUG

#include <uv.h>
#include <cstdio>


//! \cond
namespace uv
{
namespace
{
namespace debug
{


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

void print_handle(void *_uv_handle)
{
  auto h = static_cast< ::uv_handle_t* >(_uv_handle);
  fprintf(stderr,
      "%s handle [0x%08llX]: has_ref=%i is_active=%i is_closing=%i\n",
      handle_type_name(h), (uintptr_t)h, ::uv_has_ref(h), ::uv_is_active(h), ::uv_is_closing(h)
  );
  fflush(stderr);
}

void print_loop_handles(void *_uv_loop_handle)
{
  auto walk_cb = [](::uv_handle_t *_h, void*){ print_handle(_h); };
  ::uv_walk(static_cast< ::uv_loop_t* >(_uv_loop_handle), static_cast< ::uv_walk_cb >(walk_cb), nullptr);
}


}
}
}
//! \endcond

#endif
#endif
