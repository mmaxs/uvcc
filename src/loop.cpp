
#include "uvcc/loop.hpp"
#include "uvcc/handle.hpp"


namespace uv
{


void loop::walk_cb(::uv_handle_t *_uv_handle, void *_arg)
{
  auto t = static_cast< walk_pack* >(_arg);
  t->func->operator ()(handle(_uv_handle), t->arg);
}


}

