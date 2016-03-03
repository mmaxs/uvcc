
#include "uvcc/loop.hpp"


namespace uv
{


void loop::walk_cb(handle::uv_t *_uv_handle, void *_arg)
{
  auto t = static_cast< walk_pack* >(_arg);
  t->func->operator ()(handle(_uv_handle), t->arg);
}


loop loop::Default() noexcept
{
  static loop default_loop;
  return default_loop;
}


}

