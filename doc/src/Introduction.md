This is introduction
* \\subpage p__uvcc
* \\subpage p__ref_counting
* \\subpage p__destroy

\page p__uvcc Libuv and uvcc
Uvcc is a C++ bindings for libuv.

\page p__ref_counting Objects with reference counting semantics
buffer, handle, request

[libcxx]: http://www.libcxx.org "libcxx" 


\page p__destroy Destroy callbacks

```
void (*uv_close_cb)(uv_handle_t* handle)
```
Type definition for callback passed to `uv_close()`

Uvcc's handle destroy callback differs from libuv's handle close callback in the following points:
-# Libuv's handle close callback passed to `uv_close()` is used primarily for freeing up the memory allocated for the handle.
   In uvcc the memory allocated for a handle is released automatically when the last variable referencing this handle is been destroyed.
   So uvcc's handle destroy callback is intended only for providing the option to manipulate the
   user-defined data associated with the handle through the `uv_handle_t.data` pointer before the handle will become vanished.
-# Libuv's handle close callback passed to `uv_close()` goes through the event loop and is called asynchronously after the `uv_close()` call.
   This callback wouldn't be executed if the handle to be closed is not an "active" handle
   (libuv API function `uv_is_active()` returns zero on such handles). Instead, uvcc's destroy callback is called unconditionally in any case
   when the last variable referencing this handle is destroyed. If the handle is not "active" the destroy callback runs synchronously
   as part of the variable's destructor.


\addtogroup g__buffer
\details

[nix]: http://man7.org/linux/man-pages/man2/readv.2.html "readv(2)"
[win]: https://msdn.microsoft.com/en-us/library/ms741542(v=vs.85).aspx "MSDN"


