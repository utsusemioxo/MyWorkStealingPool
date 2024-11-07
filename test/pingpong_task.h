#pragma once
#include "itasksys.h"
#include <atomic>
#include <iostream>
#include <thread>
#include "CycleTimer.h"
#include <cmath>
#include <vector>

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

  void RunTask(int task_id, int num_total_tasks) {

    std::cout << "pingpong begin " << std::this_thread::get_id() << "\n";
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
    std::cout << "pingpong end " << std::this_thread::get_id() << "\n";
  }
};

typedef struct {
  bool passed;
  double time;
} TestResults;

TestResults pingPongTest(ITaskSystem *t, bool equal_work,
                         int num_elements, int base_iters) {

  // std::this_thread::sleep_for(std::chrono::seconds(1));
  int num_tasks = 64;
  // int num_tasks = 8;
  int num_bulk_task_launches = 400;
  // int num_bulk_task_launches = 16;

  int *input = new int[num_elements];
  int *output = new int[num_elements];

  // Init input
  for (int i = 0; i < num_elements; i++) {
    input[i] = i;
    output[i] = 0;
  }

  // Ping-pong input and output buffers with all the
  // back-to-back task launches
  std::vector<PingPongTask *> runnables(num_bulk_task_launches);
  for (int i = 0; i < num_bulk_task_launches; i++) {
    if (i % 2 == 0)
      runnables[i] =
          new PingPongTask(num_elements, input, output, equal_work, base_iters);
    else
      runnables[i] =
          new PingPongTask(num_elements, output, input, equal_work, base_iters);
  }

  // Run the test
  double start_time = CycleTimer::currentSeconds();
  TaskID prev_task_id;
  for (int i = 0; i < num_bulk_task_launches; i++) {
      t->Run(runnables[i], num_tasks);
  }

  double end_time = CycleTimer::currentSeconds();

  // Correctness validation
  TestResults results;
  results.passed = true;

  // Number of ping-pongs determines which buffer to look at for the results
  int *buffer = (num_bulk_task_launches % 2 == 1) ? output : input;

  for (int i = 0; i < num_elements; i++) {
    int value = i;
    for (int j = 0; j < num_bulk_task_launches; j++) {
      int iters = (!equal_work) ? PingPongTask::ping_pong_iters(i, num_elements,
                                                                base_iters)
                                : base_iters;
      value = PingPongTask::ping_pong_work(iters, value);
    }

    int expected = value;
    if (buffer[i] != expected) {
      results.passed = false;
      printf("%d: %d expected=%d\n", i, buffer[i], expected);
      break;
    }
  }
  results.time = end_time - start_time;

  delete[] input;
  delete[] output;
  for (int i = 0; i < num_bulk_task_launches; i++)
    delete runnables[i];

  return results;
}