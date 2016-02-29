This is introduction
* \\subpage p__uvcc
* \\subpage p__ref_counting
* \\subpage p__destroy

\page p__uvcc libuv and uvcc
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
-# libuv's handle close callback passed to `uv_close()` is used primarily for freeing up the memory allocated for the handle.
   In uvcc the memory allocated for a handle is released automatically when the last variable referencing this handle is been destroyed.
   So uvcc's handle destroy callback is intended only for providing the option to manipulate the
   user-defined data associated with the handle through the `uv_handle_t.data` pointer before the handle will become vanished.
-# libuv's handle close callback passed to `uv_close()` goes through the event loop and is called asynchronously after the `uv_close()` call.
   This callback wouldn't be executed if the handle to be closed is not an "active" handle
   (libuv API function `uv_is_active()` returns zero on such handles). Instead, uvcc's destroy callback is called unconditionally in any case
   when the last variable referencing this handle is destroyed. If the handle is not "active" the destroy callback runs synchronously
   as part of the variable's destructor.


\addtogroup g__buffer
\details
[`uv_buf_t`]: http://docs.libuv.org/en/latest/misc.html#c.uv_buf_t "libuv"
[`iovec`]: http://man7.org/linux/man-pages/man2/readv.2.html "readv(2)"
[`WSABUF`]: https://msdn.microsoft.com/en-us/library/ms741542(v=vs.85).aspx "MSDN"

libuv uses the [`uv_buf_t`] structure to describe the I/O buffer. It is made up in accordance with
system depended type [`iovec`] on Unix-like OSes and with [`WSABUF`] on Windows. An array of
`uv_buf_t` structures is used to allow writing multiple buffers in a single `uv::write` request.

The class `uv::buffer` encapsulates `uv_buf_t` structure and provides `uv_buf_t[]` array functionality.
See descriptions for `uv::buffer`'s constructors.

For the following example here is the diagram illustrating the internals of the `uv::buffer` object while executing `foo(BUF)`:
```
uv::buffer BUF{100, 200, 0, 300};

void foo(uv::buffer _b)  { /*...*/ }

int main() {
  foo(BUF);
}
```
\verbatim

         BUF                       uv::buffer::instance
 ╔═══════════════╗       ╔═════════════════════════════════════╗
 ║ uv_buf_t* ptr ╫────┐  ║ ref_count rc = 2                    ║
 ╚═══════════════╝    │  ╟─────────────────────────────────────╢
                      │  ║ size_t    buf_count = 4             ║
                      │  ╟─────────────────┬───────────────────╢
         _b        ┌─►└─►║ uv_buf_t buf[0] │ char*  .base      ╫───┐
 ╔═══════════════╗ │     ║                 │ size_t .len = 100 ║   │
 ║ uv_buf_t* ptr ╫─┘     ╚═════════════════╧═══════════════════╝   │
 ╚═══════════════╝       │ uv_buf_t buf[1] │ char*  .base      ┼───│───┐
                         │                 │ size_t .len = 200 │   │   │
                         ├─────────────────┼───────────────────┤   │   │
                         │ uv_buf_t buf[2] │ char*  .base      ┼───│───│───┐
                         │                 │ size_t .len = 0   │   │   │   │
                         ├─────────────────┼───────────────────┤   │   │   │
                         │ uv_buf_t buf[3] │ char*  .base      ┼───│───│───│───┐
                         │                 │ size_t .len = 300 │   │   │   │   │
                         ├─────────────────┴───────────────────┤   │   │   │   │
                         │ ... std::max_align_t padding ...    │   │   │   │   │
                         ├─────────────────────────────────────┤   │   │   │   │
                         │                                     │◄──┘   │   │   │
                         │ ... 100 bytes ...                   │       │   │   │
                         │                                     │       │   │   │
                         ├─────────────────────────────────────┤       │   │   │
                         │                                     │◄──────┘   │   │
                         │                                     │           │   │
                         │ ... 200 bytes ...                   │           │   │
                         │                                     │           │   │
                         │                                     │           │   │
                         ├─────────────────────────────────────┤           │   │
                         │                                     │◄──────────┘◄──┘
                         │                                     │
                         │                                     │
                         │ ... 300 bytes ...                   │
                         │                                     │
                         │                                     │
                         │                                     │
                         └─────────────────────────────────────┘
\endverbatim                            
                            
[ ]: # "▲ ► ▼ ◄"
[ ]: # "─ │ ┌ ┐ └ ┘ ├ ┤ ┬ ┴ ┼"
[ ]: # "═ ║ ╒ ╓ ╔ ╕ ╖ ╗ ╘ ╙ ╚ ╛ ╜ ╝ ╞ ╟ ╠ ╡ ╢ ╣ ╤ ╥ ╦ ╧ ╨ ╩ ╪ ╫ ╬"

