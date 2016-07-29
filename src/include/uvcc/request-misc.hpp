
#ifndef UVCC_REQUEST_MISC__HPP
#define UVCC_REQUEST_MISC__HPP

#include "uvcc/utility.hpp"
#include "uvcc/request-base.hpp"

#include <uv.h>
#include <functional>   // function bind placeholders::
#include <future>       // future packaged_task
#include <type_traits>  // enable_if is_convertible


namespace uv
{


/*! \ingroup doxy_group_request
    \brief Work scheduling request type.
    \sa libuv API documentation: [Thread pool work scheduling](http://docs.libuv.org/en/v1.x/threadpool.html#thread-pool-work-scheduling). */
template< typename _Result_ = void >
class work : public request
{
  //! \cond
  friend class request::instance< work >;
  //! \endcond

public: /*types*/
  using uv_t = ::uv_work_t;
  using on_request_t = std::function< void(work _request) >;
  /*!< \brief The function type of the callback called after the work on the threadpool has been completed.
       \details This callback is called _on the loop thread_.
       \sa libuv API documentation: [`uv_after_work_cb`](http://docs.libuv.org/en/v1.x/threadpool.html#c.uv_after_work_cb). */
  template< typename... _Args_ >
  using on_work_t = std::function< _Result_(_Args_&&... _args) >;
  /*!< \brief The function type of the task which is scheduled to be run on the thread pool.
       \details This function is called _on the one of the threads from the thread pool_.
       \sa libuv API documentation: [`uv_work_cb`](http://docs.libuv.org/en/v1.x/threadpool.html#c.uv_work_cb). */

protected: /*types*/
  //! \cond
  struct properties
  {
    std::packaged_task< _Result_() > task;
    std::future< _Result_ > res;
  };
  //! \endcond

private: /*types*/
  using instance = request::instance< work >;

private: /*constructors*/
  explicit work(uv_t *_uv_req)
  {
    if (_uv_req)  instance::from(_uv_req)->ref();
    uv_req = _uv_req;
  }

public: /*constructors*/
  ~work() = default;
  work()
  {
    uv_req = instance::create();
    static_cast< uv_t* >(uv_req)->type = UV_WORK;
  }

  work(const work&) = default;
  work& operator =(const work&) = default;

  work(work&&) noexcept = default;
  work& operator =(work&&) noexcept = default;

private: /*functions*/
  template< typename = void > static void work_cb(::uv_work_t*);
  template< typename = void > static void after_work_cb(::uv_work_t*, int);

public: /*interface*/
  const on_request_t& on_request() const noexcept  { return instance::from(uv_req)->request_cb_storage.value(); }
        on_request_t& on_request()       noexcept  { return instance::from(uv_req)->request_cb_storage.value(); }

  /*! \brief The libuv loop that started this `work` request and where completion will be reported.
      \details It is guaranteed that it will be a valid instance at least within the request callback. */
  uv::loop loop() const noexcept  { return uv::loop(static_cast< uv_t* >(uv_req)->loop); }

  /*! \brief Get the result of the work. */
  std::future< _Result_ >& result() const  { return instance::from(uv_req)->properties().res; }

  /*! \brief Run the request. Queue the `_Task_` to the thread pool.
      \details The given `_Task_` function is called with specified `_args` applied and is executed
      on the one of the threads from the thread pool. Once it is completed, `on_request` callback will be called
      on the `_loop` thread.
      \note All arguments are copied (or moved) to the `_task` function object.
      For passing arguments by reference (when parameters are used as output ones), wrap them with `std::ref()`.
      \sa libuv API documentation: [`uv_queue_work()`](http://docs.libuv.org/en/v1.x/threadpool.html#c.uv_queue_work). */
  template< class _Task_, typename... _Args_ >
  std::enable_if_t< std::is_convertible< _Task_, on_work_t< _Args_&&... > >::value, int >
  run(uv::loop &_loop, _Task_&& _task, _Args_&&... _args)
  {
    auto instance_ptr = instance::from(uv_req);

    instance_ptr->ref();

    {
      auto &properties = instance_ptr->properties();
      using task_t = decltype(properties.task);
      properties.task = task_t(std::bind(_task, std::forward< _Args_ >(_args)...));
      properties.res = properties.task.get_future();
    }

    uv_status(0);
    int ret = ::uv_queue_work(
        static_cast< uv::loop::uv_t* >(_loop), static_cast< uv_t* >(uv_req),
        work_cb, after_work_cb
    );
    if (!ret)  uv_status(ret);
    return ret;
  }

public: /*conversion operators*/
  explicit operator const uv_t*() const noexcept  { return static_cast< const uv_t* >(uv_req); }
  explicit operator       uv_t*()       noexcept  { return static_cast<       uv_t* >(uv_req); }
};

template< typename _Result_ >
template< typename >
void work< _Result_ >::work_cb(::uv_work_t *_uv_req)
{
  auto &task = instance::from(_uv_req)->properties().task;
  if (task.valid())  task();
}

template< typename _Result_ >
template< typename >
void work< _Result_ >::after_work_cb(::uv_work_t *_uv_req, int _status)
{
  auto instance_ptr = instance::from(_uv_req);
  instance_ptr->uv_error = _status;

  ref_guard< instance > unref_req(*instance_ptr, adopt_ref);

  auto &after_work_cb = instance_ptr->request_cb_storage.value();
  if (after_work_cb)  after_work_cb(work(_uv_req));
}


}


#endif
