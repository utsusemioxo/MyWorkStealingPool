#include "itasksys.h"      // Assuming this contains IRunnable and ITaskSystem
#include "pingpong_task.h" // Assuming this contains PingPongTask
#include "thread_pool.h"
#include <atomic>
#include <iostream>
#include <vector>

int main() {
  ITaskSystem *t = new TaskSystemParallelThreadPool(8);
  double minT = 1e30;

  TestResults result = pingPongTest(t, false, 16, 0);
  if (!result.passed) {
    printf("ERROR: Results did not pass correctness check! (iter=%d, "
           "ref_impl=%s)\n",
           0, t->Name());
    exit(1);
  }
  minT = std::min(minT, result.time);
  printf("[%s]:\t\t[%.3f] ms\n", t->Name(), minT * 1000);

  delete t;
  return 0;
}