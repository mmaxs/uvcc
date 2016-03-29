
uvcc is C++ bindings for libuv.




\page p__ref_counting Objects with reference counting semantics


To implement automatic memory management for the objects involved in libuv API function calls and user callbacks
these uvcc classes are designed with the use of a reference counting technique:
* `uv::buffer`
* `uv::handle`
* `uv::request`
* `uv::loop`

This means that the variables for one of these types are just pointers to an object instance created on the heap
and have a semantics analogous to `std::shared_ptr`.
A newly created variable produces a new object got instantiated on the heap with initial reference count value equal to one.
Copying this variable to another or passing it by value as a function argument just copies the pointer and increases the reference
count of the object instance. If the variable goes out of scope of its definition and gets destroyed then the reference count of
the object instance is decreased. The count increase/decrease operations are atomic and thread-safe.
The object instance by itself gets actually destroyed and the memory allocated for it
is automatically released when the last variable referencing the object is destroyed and the reference count becomes zero.

\sa Considerations on reference-counted objects in [libcxx](http://www.libcxx.org/refobj.html) documentation.




\page p__destroy Destroy callbacks

The following libuv types provide a field which is a pointer to the user-defined arbitrary data
that can be associated with the libuv object:
* [`uv_handle_t.data`](http://docs.libuv.org/en/v1.x/handle.html#c.uv_handle_t.data),
* [`uv_req_t.data`](http://docs.libuv.org/en/v1.x/request.html#c.uv_req_t.data),
* [`uv_loop_t.data`](http://docs.libuv.org/en/v1.x/loop.html#c.uv_loop_t.data).

Each of the uvcc classes representing these libuv types provides a destroy callback which is called when the reference count
for the object instance becomes zero:
* `uv::handle::on_destroy_t`
* `uv::request::on_destroy_t`
* `uv::loop::on_destroy_t`

The callback is intended to give the option to manipulate the user-defined data associated with the object
through the `.data` pointer before the last variable referencing this object is been destroyed.

For the `uv::handle` calss the destroy callback functionality is based on the underlying libuv API
[`uv_close_cb`](http://docs.libuv.org/en/v1.x/handle.html#c.uv_close_cb) feature.
The handle close callback passed to [`uv_close()`](http://docs.libuv.org/en/v1.x/handle.html#c.uv_close)
goes through the event loop and is called asynchronously after the `uv_close()` call. But it wouldn't be executed if the handle
to be closed is not an "active" handle (libuv API function [`uv_is_active()`](http://docs.libuv.org/en/v1.x/handle.html#c.uv_is_active)
returns zero on such handles). Instead, uvcc's destroy callback is called in any case whenever the last variable
referencing this handle is destroyed. If the handle is not "active" the destroy callback runs synchronously as part of the
variable's destructor.

For the other two classes (`uv::request` and `uv::loop`) the callbacks are a uvcc specific feature and they are executed
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




\addtogroup g__handle
\details
\note uvcc handle objects have no interface functions corresponding to the following libuv API functions that control
whether a handle is [referenced by the event loop](http://docs.libuv.org/en/v1.x/handle.html#reference-counting)
which it has been attached to while being created and where it is running on:
[`uv_ref()`](http://docs.libuv.org/en/v1.x/handle.html#c.uv_ref),
[`uv_unref()`](http://docs.libuv.org/en/v1.x/handle.html#c.uv_unref),
[`uv_has_ref()`](http://docs.libuv.org/en/v1.x/handle.html#c.uv_has_ref).
These libuv functions can be directly applied to uvcc handle objects (with explicit casting the handle variable to `uv_handle_t*`)
if necessary.




\page p__example Examples

uvcc sources are accompanied with several illustrative example programs the source code of which can be found in the
/example subdirectory.

* \subpage p__example_lxjs2012 "lxjs2012-demo" - an example used by Bert Belder at
                               [LXJS 2012](http://www.youtube.com/watch?v=nGn60vDSxQ4) to demonstrate the libuv basics. \n
                               It shows how uvcc simplifies the code.

* \subpage p__example_cpio "cpio" - a simple program that copies its `stdin` to `stdout`. \n
                           It shows some essential points that one comes across with when begin to develop
                           programs using libuv and how uvcc address them.

* \subpage p__example_tee  "tee" - a simple program that copies its `stdin` to `stdout` and also to each file specified as a program argument. \n
                           It demonstrates simple versions of the buffer and request pools that can avoid intense memory allocation requests
                           and some side effects of the C/C++ memory allocator appearing thereat.


\page p__example_lxjs2012 lxjs2012-demo

The example used by Bert Belder at [LXJS 2012](http://www.youtube.com/watch?v=nGn60vDSxQ4) to introduce the libuv basics.

The original code is slightly modified to work with recent libuv version > 0.10.

\verbatim /example/lxjs2012-demo.c \endverbatim
\includelineno lxjs2012-demo.c

["translation noise"](https://books.google.ru/books?id=Uemuaza3fTEC&pg=PT26&dq=%22translation+noise%22&hl=en&sa=X&ved=0ahUKEwigoJ3Dq8bLAhVoc3IKHQGQCFYQ6AEIGTAA)


[Introduction to libuv – Thorsten Lorenz](http://www.youtube.com/watch?v=cLL28s6yb1I)
[using libuv and http parser to build a webserver HD (with captions)](http://www.youtube.com/watch?v=aLm40q7qm3w)

\page p__example_cpio cpio

A simple program that copies its stdin to stdout written in pure C using libuv.

There are some points that should be mentioned:
- [1] The structure describing a write request should be allocated on the heap.
      \sa The rationale described on _stackoverflow.com_: ["libuv + C++ segfaults"](http://stackoverflow.com/questions/29319392/libuv-c-segfaults).
- [2] The allocated buffers should be somehow tracked in the program to be freed up to prevent
      the memory leak or to get the result of a request. The libuv suggested techniques are:
      using manual packaging/embedding the request and the buffer description structures into some sort of operational context
      enclosing structure, or using [`uv_req_t.data`](http://docs.libuv.org/en/v1.x/request.html#c.uv_req_t.data) pointer member
      to the user-defined arbitrary data.
      \sa Discussions at _github.com/joyent/libuv/issues_: ["Preserve uv_write_t->bufs in uv_write() #1059"](https://github.com/joyent/libuv/issues/1059),
      ["Please expose bufs in uv_fs_t's result for uv_fs_read operations. #1557"](https://github.com/joyent/libuv/issues/1557).
- [3] Be sure that things stay valid until some conditions in the future.
      \sa Discussion at _github.com/joyent/libuv/issues_: ["Lifetime of buffers on uv_write #344"](https://github.com/joyent/libuv/issues/344).

.

\verbatim /example/cpio-uv.c \endverbatim
\includelineno cpio-uv.c

The following is the above program being rewritten using uvcc. All the considered points are taken into account by design of uvcc.
\verbatim /example/cpio-uvcc.cpp \endverbatim
\includelineno cpio-uvcc.cpp

Here is a performance comparison between the two variants:

\verbatim
Mike@U250 [CYGWIN] /cygdrive/d/wroot/libuv/uvcc/uvcc.git
$ cat /dev/zero | ./build/example/cpio-uv.exe | dd of=/dev/null iflag=fullblock bs=1M count=10240
10240+0 records in
10240+0 records out
10737418240 bytes (11 GB, 10 GiB) copied, 23.3906 s, 459 MB/s

Mike@U250 [CYGWIN] /cygdrive/d/wroot/libuv/uvcc/uvcc.git
$ cat /dev/zero | ./build/example/cpio-uvcc.exe | dd of=/dev/null iflag=fullblock bs=1M count=10240
10240+0 records in
10240+0 records out
10737418240 bytes (11 GB, 10 GiB) copied, 25.1898 s, 426 MB/s
\endverbatim

Obviously there is an impact of reference counting operations that slightly reduce the performance.




\page p__example_tee tee

A simple program that copies its `stdin` to `stdout` and also to each file specified as a program argument.

It demonstrates two points: the very same uvcc buffer can be easily dispatched to different asynchronous operations and its lifetime
will continue until the last operation has completed and the example for a simple version of the buffer and request pool implementation.

Pools help avoid intense memory allocation requests and the effect of continuous linear increasing of the memory consumed by the program
until the C/C++ memory allocator decides to actually free up the allocated space.
The provided simple implementation is an auto-growing pool that is not thread-safe, so only one dedicated thread might acquire an item
from the pool in multi-thread environment. The condition of item's `nrefs() == 1` indicates that no more references
are left anywhere at the runtime other than in the pool container itself.

\sa _stackoverflow.com_: ["libuv allocated memory buffers re-use techniques"](http://stackoverflow.com/questions/28511541/libuv-allocated-memory-buffers-re-use-techniques)

\verbatim /example/tee.cpp \endverbatim
\includelineno tee.cpp




\page p__compiling Compiling

uvcc consists only of header files. You need just to copy files and subfolders from /src/include directory into one of
the directories being searched for header files in your project and include uvcc.hpp header into your source files.

To compile bundled examples use the following make goal:

> make example/SOURCE_FILE_BASE_NAME

where SOURCE_FILE_BASE_NAME is a base name of one of the source files located in /example subdirectory.

On Windows you can use [Mingw-w64](http://mingw-w64.org) compiler, make utility, and the command shell that are packaged
with [Cygwin](https://cygwin.com) or [Msys2](http://msys2.github.io) projects. Then add the goal prefix:

> make windows/example/SOURCE_FILE_BASE_NAME

You will get Windows native binaries built against the libuv Windows release saved
in /libuv-x64-v1.8.0.build8 subdirectory. If you don't have libuv installed in your Windows system root folder
don't forget to copy libuv.dll from the saved libuv release subdirectory and put it along with the resulting executable files.

All build results will be placed in /build/example subdirectory from the uvcc root directory.




