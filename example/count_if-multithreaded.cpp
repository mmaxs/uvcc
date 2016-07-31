
#include "uvcc.hpp"
#include <cstdio>
#include <algorithm>  // count_if
#include <numeric>    // accumulate
#include <iterator>   // next advance
#include <thread>     // hardware_concurrency
#include <future>     // future
#include <utility>    // move
#include <vector>     // vector


template< class _InputIterator_, class _UnaryPredicate_ >
std::size_t count_if(_InputIterator_ _begin, _InputIterator_ _end, _UnaryPredicate_ _predicate)
{
  const std::size_t length = std::distance(_begin, _end);
  const unsigned int hardware_concurrency = std::thread::hardware_concurrency() ? std::thread::hardware_concurrency() : 1;
  constexpr const unsigned int minimum_section_length = 27;
  const unsigned int section_length = uv::lowest(length, uv::greatest(minimum_section_length, length/hardware_concurrency));

  std::vector< std::future< std::size_t > > results;
  results.reserve(length/section_length + 1);

  while (_begin < _end)
  {
    auto section_end = std::next(_begin, section_length);
    if (section_end > _end)  section_end = _end;

    uv::work< std::size_t > task;
    task.on_request() = [](decltype(task) _task)
    {
      fprintf(stdout, "work [0x%08llX] completed\n", _task.id());
      fflush(stdout);
    };

    task.run(uv::loop::Default(), std::count_if< _InputIterator_, _UnaryPredicate_ >, _begin, section_end, _predicate);

    results.emplace_back(std::move(task.result()));

    std::advance(_begin, section_length);
  };

  return std::accumulate(
      results.begin(), results.end(),
      (std::size_t)0,
      [](auto &_sum, auto &_value){ return _sum + _value.get(); }
  );
}



int main(int _argc, char *_argv[])
{
  constexpr const int target_value = 111;
  std::vector< int > t(1000, target_value);

  std::size_t n = ::count_if(
      t.begin(), t.end(),
      [target_value](const auto &_v){ return _v == target_value; }
  );

  fprintf(stdout, "%zu\n", n);  fflush(stdout);

  return uv::loop::Default().run(UV_RUN_DEFAULT);
}
