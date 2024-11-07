#pragma once
#include "itasksys.h"
#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <stack>
#include <thread>
#include <vector>

class TaskSystemParallelThreadPool : public ITaskSystem {
public:
  explicit TaskSystemParallelThreadPool(int num_threads);
  ~TaskSystemParallelThreadPool() override;
  const char *Name() override;
  void Run(IRunnable *runnable, int num_total_tasks) override;

private:
  using Task = std::function<void()>;
  void AddTask(Task &task, int task_id);
  std::vector<std::thread> m_workers;

  int m_num_threads;
  int m_num_total_tasks;
  std::vector<std::condition_variable> m_local_cond;
  std::vector<std::mutex> m_local_mtx;
  std::vector<std::deque<Task>> m_local_tasks;
  std::condition_variable m_finish_cond;
  std::mutex m_finish_mtx;
  std::atomic<bool> m_shutdown{false};
  std::atomic<int> m_task_finish_cnt{0};

  class WorkerThread {
  public:
    explicit WorkerThread(TaskSystemParallelThreadPool *tasksys, int index);
    ~WorkerThread() = default;

    void operator()();

  private:
    void RunLocalTask();
    bool TryRunTask(std::unique_lock<std::mutex> &ul);
    void WaitForTask();
    bool TryStealWork(int steal_num);

  private:
    TaskSystemParallelThreadPool *m_tasksys;
    int m_worker_index;
    int m_local_task_finish_cnt{0};
    std::stack<Task> stolen_tasks;
  };
};
