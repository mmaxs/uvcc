
#include "uvcc.hpp"
#include <cstdio>
#include <algorithm>  // count_if
#include <iterator>   // next advance
#include <thread>     // hardware_concurrency
#include <future>     // future
#include <utility>    // move
#include <vector>     // vector


template< class _InputIterator_, class _UnaryPredicate_ >
std::size_t count_if(_InputIterator_ _begin, _InputIterator_ _end, _UnaryPredicate_ _predicate)
{
  const auto length = std::distance(_begin, _end);
  const auto hardware_concurrency = std::thread::hardware_concurrency() ? std::thread::hardware_concurrency() : 1;
  const auto section_length = uv::greatest(uv::lowest(length, 27), length/hardware_concurrency);

  std::vector< std::future< std::size_t > > results;
  results.reserve(length/section_length + 1);

  while (_begin < _end)
  {
    auto end = std::next(_begin, section_length);
    if (end > _end)  end = _end;

    uv::work< std::size_t > task;
    task.on_request() = [](decltype(task) _work_request)
    { fprintf(stdout, "work [%llu] completed\n", _work_request.id()); fflush(stdout); };

    task.run(uv::loop::Default(), std::count_if< _InputIterator_, _UnaryPredicate_ >, _begin, end, _predicate);

    results.emplace_back(std::move(task.result()));

    std::advance(_begin, section_length);
  }

  return std::accumulate(results.begin(), results.end(), (std::size_t)0, [](auto &_sum, auto &_value)
  {
    return _sum + _value.get();
  });
}



int main(int _argc, char *_argv[])
{
  std::vector< int > t(1000);

  int e = 0;

  std::size_t n = ::count_if(t.begin(), t.end(), [&e](const auto &_e){ return _e == e; });

  fprintf(stdout, "%zu\n", n);  fflush(stdout);

  return uv::loop::Default().run(UV_RUN_DEFAULT);
}
