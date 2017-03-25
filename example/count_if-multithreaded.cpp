
#include "uvcc.hpp"
#include <cstdio>
#include <algorithm>  // count_if
#include <numeric>    // accumulate
#include <iterator>   // next advance
#include <thread>     // hardware_concurrency
#include <future>     // shared_future
#include <utility>    // move
#include <vector>     // vector
#include <random>     // random_device uniform_int_distribution


template< typename _Work_ >
void task_report(_Work_ _task)
{
  fprintf(stdout, "work [0x%08tX] completed, target values found: %zu\n", _task.id(), _task.result().get());
  fflush(stdout);
}


template< class _InputIterator_, class _UnaryPredicate_ >
std::size_t count_if_multithreaded(_InputIterator_ _begin, _InputIterator_ _end, _UnaryPredicate_ _predicate)
{
  const std::size_t length = std::distance(_begin, _end);
  const unsigned int hardware_concurrency = std::thread::hardware_concurrency() ? std::thread::hardware_concurrency() : 1;
  const unsigned int minimum_section_length = uv::lowest(27u, length);
  const unsigned int alloted_section_length = uv::greatest(minimum_section_length, length/hardware_concurrency);

  std::vector< std::shared_future< std::size_t > > results;
  results.reserve(length/alloted_section_length + 1);

  for (auto section_end = _begin; _begin < _end; _begin = section_end)
  {
    std::advance(section_end, alloted_section_length);
    if (section_end > _end)  section_end = _end;

    uv::work< std::size_t > task;
    task.on_request() = task_report< decltype(task) >;

    fprintf(stdout, "work [0x%08tX] starting\n", task.id());  fflush(stdout);
    task.run(uv::loop::Default(), std::count_if< _InputIterator_, _UnaryPredicate_ >, _begin, section_end, _predicate);

    results.emplace_back(task.result());
  }

  return std::accumulate(
      results.begin(), results.end(),
      0u,
      [](auto &_sum, auto &_value)  {
          return _sum + _value.get();  // automatically waiting for the section result
      }
  );
}



int main(int _argc, char *_argv[])
{

  constexpr int target_value = 1;
  constexpr std::size_t vector_size = 10e8;
  constexpr std::size_t nvalues = vector_size/10-1;

  fprintf(stdout, "generating a random test vector of vector_size = %zu, step 1\n", vector_size);  fflush(stdout);
  std::vector< int > test(vector_size, ~target_value);
  fprintf(stdout, "generating a random test vector of vector_size = %zu, step 2\n", vector_size);  fflush(stdout);
  std::random_device seed;
  std::uniform_int_distribution<> random(0, vector_size-1);
  for (std::size_t i, n = nvalues; n > 0;)
  {
    i = random(seed);
    if (test[i] == target_value)  continue;
    test[i] = target_value;
    --n;
  }

  std::size_t n = count_if_multithreaded(
      test.begin(), test.end(),
      [](const auto &_v){ return _v == target_value; }
  );

  fprintf(stdout, "target values (nvalues = %zu) total: %zu\n", nvalues, n);  fflush(stdout);

  return uv::loop::Default().run(UV_RUN_DEFAULT);
}
