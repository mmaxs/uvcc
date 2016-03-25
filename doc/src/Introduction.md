This is an introduction
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
\see documentation for `uv::buffer` constructors.

For the following example here is the diagram illustrating the internals of the `uv::buffer` object while executing `foo(BUF)`:
```
buffer BUF{100, 200, 0, 300};

void foo(buffer _b)  { /*...*/ }

int main() {
  foo(BUF);
}
```
\verbatim

         BUF                         buffer::instance
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

The default constructor `uv::buffer::buffer()` creates a _null-initialized_ `uv_buf_t` structure.
One can fill it to make it pointing to manually allocated memory area in the following ways:
```
buffer buf;

const size_t len = /*...*/

buf[0] = uv_buf_init(new char[len], len);

// or

buf.base() = new char[len];
buf.len() = len;
```
\sa libuv documentation: [`uv_buf_init()`](http://docs.libuv.org/en/v1.x/misc.html#c.uv_buf_init).

\warning Do not return such a manually initialized `buffer` objects from the buffer allocation callbacks (of
`uv::on_buffer_t` function type) that should be passed to `stream::read_start()` and `udp::recv_start()` functions.
Only a copy of `uv_buf_t` structure held in a `buffer` object is passed to libuv API functions and there is no way to
reconstruct the `buffer` object's address (which is needed for the uvcc reference counting mechanism to work properly)
from the `uv_buf_t.base` pointer referencing the manually allocated external memory.
\warning Also, the `buffer` objects that contain an array of `uv_buf_t` structures are not supported as a valid result
of the `uv::on_buffer_t` callback functions. Only a single `uv_buf_t` structure in a `buffer` object is currently
supported for the purposes of the `stream::read_start()` and `udp::recv_start()` functions.



\page p__example Examples

uvcc sources are accompanied with several illustrative example programs the source code of which can be found in the
/example subdirectory.

* \subpage p__example_lxjs2012 "lxjs2012-talk" - an introductory example used by Bert Belder at
                               [LXJS 2012](http://www.youtube.com/watch?v=nGn60vDSxQ4) to introduce the libuv basics

* \subpage p__example_cpio "cpio" - a simple program that copies its stdin to stdout. \n
                           It shows some essential points that one comes across with when begin to develop
                           programs using libuv and how uvcc simplifies the code.
* \subpage p__example_tee  "tee" - illustrates simple versions for buffer and request pools


\page p__example_lxjs2012 lxjs2012-talk

[Introduction to libuv – Thorsten Lorenz](http://www.youtube.com/watch?v=cLL28s6yb1I)
[using libuv and http parser to build a webserver HD (with captions)](http://www.youtube.com/watch?v=aLm40q7qm3w)

\page p__example_cpio cpio

A simple program that copies its stdin to stdout written in pure C using libuv.
\includelineno cpio-uv.c

The following is the above program being rewritten using uvcc:
\includelineno cpio-uvcc.cpp



\page p__example_tee tee
\includelineno tee.cpp

