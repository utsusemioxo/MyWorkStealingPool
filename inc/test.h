#include "itasksys.h"
#include <cmath>
#include "itasksys.h"
#include <iostream>
/*
 * Each task computes an output list of accumulated counters. Each counter
 * is incremented in a tight for loop. The `equal_work_` field ensures that
 * each element of the output array requires a different amount of computation.
 */
class PingPongTask : public IRunnable {
public:
  int num_elements_;
  int *input_array_;
  int *output_array_;
  bool equal_work_;
  int iters_;

  PingPongTask(int num_elements, int *input_array, int *output_array,
               bool equal_work, int iters)
      : num_elements_(num_elements), input_array_(input_array),
        output_array_(output_array), equal_work_(equal_work), iters_(iters) {}
  ~PingPongTask() {}

  static inline int ping_pong_iters(int i, int num_elements, int iters) {
    int max_iters = 2 * iters;
    return std::floor((static_cast<float>(num_elements - i) / num_elements) *
                      max_iters);
  }

  static inline int ping_pong_work(int iters, int input) {
    int accum = input;
    for (int j = 0; j < iters; j++) {
      if (j % 2 == 0)
        accum++;
    }
    return accum;
  }

  void RunTask(TaskID task_id, int num_total_tasks) {

    // std::cout << "pingpong begin " << "\n";
    // handle case where num_elements is not evenly divisible by num_total_tasks
    int elements_per_task =
        (num_elements_ + num_total_tasks - 1) / num_total_tasks;
    int start_el = elements_per_task * task_id;
    int end_el = std::min(start_el + elements_per_task, num_elements_);

    if (equal_work_) {
      for (int i = start_el; i < end_el; i++) {
        output_array_[i] = ping_pong_work(iters_, input_array_[i]);
      }
    } else {
      for (int i = start_el; i < end_el; i++) {
        int el_iters = ping_pong_iters(i, num_elements_, iters_);
        output_array_[i] = ping_pong_work(el_iters, input_array_[i]);
      }
    }
    // std::cout << "pingpong end " << "\n";
  }
};

#define DEFAULT_NUM_THREADS 8
#define DEFAULT_NUM_TIMING_ITERATIONS 3

/*
 * Structure to hold results of performance tests
 */
typedef struct {
  bool passed;
  double time;
} TestResults;



class SimpleMultiplyTask : public IRunnable {
public:
  int num_elements_;
  int *array_;

  SimpleMultiplyTask(int num_elements, int *array)
      : num_elements_(num_elements), array_(array) {}
  ~SimpleMultiplyTask() {}

  static inline int multiply_task(int iters, int input) {
    int accumulator = 1;
    for (int i = 0; i < iters; ++i) {
      accumulator *= input;
    }
    return accumulator;
  }

  void runTask(int task_id, int num_total_tasks) {
    std::cout << "runTask id " << task_id << " total_tasks " << num_total_tasks
              << "\n";
    // handle case where num_elements is not evenly divisible by num_total_tasks
    int elements_per_task =
        (num_elements_ + num_total_tasks - 1) / num_total_tasks;
    int start_el = elements_per_task * task_id;
    int end_el = std::min(start_el + elements_per_task, num_elements_);

    for (int i = start_el; i < end_el; i++)
      array_[i] = multiply_task(3, array_[i]);
  }
};
