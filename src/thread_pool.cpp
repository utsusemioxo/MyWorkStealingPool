
#include <algorithm>
#include <random>
#include "thread_pool.h"
#include "itasksys.h"


TaskSystemParallelThreadPool::TaskSystemParallelThreadPool(int num_threads)
    : ITaskSystem(num_threads), m_num_threads(num_threads),
      m_local_cond(num_threads), m_local_mtx(num_threads),
      m_local_tasks(num_threads), m_num_total_tasks(0) {
  m_workers.reserve(m_num_threads);
  for (int i = 0; i < m_num_threads; i++) {
    m_workers.emplace_back(WorkerThread(this, i));
  }
}

TaskSystemParallelThreadPool::~TaskSystemParallelThreadPool() {
  m_shutdown = true;
  for (auto &cond : m_local_cond) {
    cond.notify_all();
  }
  for (auto &thread : m_workers) {
    if (thread.joinable())
      thread.join();
  }
}

const char *TaskSystemParallelThreadPool::Name() {
  return "Parallel + Thread Pool + Sleep";
}

void TaskSystemParallelThreadPool::Run(IRunnable *runnable,
                                       int num_total_tasks) {
  m_task_finish_cnt = 0;
  m_num_total_tasks = num_total_tasks;

  for (int i = 0; i < num_total_tasks; i++) {
    Task task = [runnable, i, this]() {
      runnable->RunTask(i, m_num_total_tasks);
    };
    AddTask(task, i);
  }

  {
    std::unique_lock<std::mutex> ul(m_finish_mtx);
    m_finish_cond.wait(ul, [this]() {
      return m_task_finish_cnt == m_num_total_tasks;
    });
  }
}

void TaskSystemParallelThreadPool::AddTask(Task &task, TaskID task_id) {
  std::unique_lock<std::mutex> ul_roundrobin(
      m_local_mtx[task_id % m_num_threads]);
  m_local_tasks[task_id % m_num_threads].push_back(std::move(task));
  ul_roundrobin.unlock();
  m_local_cond[task_id % m_num_threads].notify_one();
}

TaskSystemParallelThreadPool::WorkerThread::WorkerThread(
    TaskSystemParallelThreadPool *tasksys, int index)
    : m_tasksys(tasksys), m_worker_index(index){};

void TaskSystemParallelThreadPool::WorkerThread::operator()() {
  while (!m_tasksys->m_shutdown) {
    std::unique_lock<std::mutex> ul_local(
        m_tasksys->m_local_mtx[m_worker_index], std::defer_lock);
    if (TryRunTask(ul_local)) {
      continue;
    } else {
      m_tasksys->m_task_finish_cnt +=
          m_local_task_finish_cnt; // todo:move into TryRunTask
      m_local_task_finish_cnt = 0;

      if (m_tasksys->m_task_finish_cnt == m_tasksys->m_num_total_tasks || m_tasksys->m_shutdown) {
        m_tasksys->m_finish_cond.notify_all();
      }

      if (TryStealWork(2) == false){
        if (m_add_done == false || m_task_finish_cnt == m_tasksys->m_num_total_tasks) {
          WaitForTask();
        }
      }
    }
  }
}

bool TaskSystemParallelThreadPool::WorkerThread::TryRunTask(
    std::unique_lock<std::mutex> &ul_local) {
  ul_local.lock();
  if (!m_tasksys->m_local_tasks[m_worker_index].empty()) {
    Task task = std::move(m_tasksys->m_local_tasks[m_worker_index].front());
    m_tasksys->m_local_tasks[m_worker_index].pop_front();
    ul_local.unlock();
    task();
    m_local_task_finish_cnt++;
    return true;
  } else {
    ul_local.unlock();
    return false;
  }
}

void TaskSystemParallelThreadPool::WorkerThread::WaitForTask() {
  std::unique_lock<std::mutex> ul_local(m_tasksys->m_local_mtx[m_worker_index]);
  m_tasksys->m_local_cond[m_worker_index].wait(ul_local, [this]() {
    return !m_tasksys->m_local_tasks[m_worker_index].empty() ||
           m_tasksys->m_shutdown;
  });
}

bool TaskSystemParallelThreadPool::WorkerThread::TryStealWork(int steal_num) {
  static thread_local std::vector<int> indices;
  if (indices.empty()) {
    indices.resize(m_tasksys->m_num_threads);
    std::iota(indices.begin(), indices.end(), 0);
  }
  std::shuffle(indices.begin(), indices.end(),
               std::mt19937{std::random_device{}()});

  for (auto i : indices) {
    if (i == m_worker_index)
      continue;
    std::unique_lock<std::mutex> ul_steal(m_tasksys->m_local_mtx[i],
                                          std::try_to_lock);
    if (ul_steal.owns_lock() &&
        m_tasksys->m_local_tasks[i].size() >= steal_num * 2) {
      for (int steal_cnt = 0; steal_cnt < steal_num; steal_cnt++) {
        stolen_tasks.emplace(std::move(m_tasksys->m_local_tasks[i].back()));
        m_tasksys->m_local_tasks[i].pop_back();
      }
      ul_steal.unlock();

      std::unique_lock<std::mutex> ul_local(
          m_tasksys->m_local_mtx[m_worker_index]);
      while (!stolen_tasks.empty()) {
        m_tasksys->m_local_tasks[m_worker_index].emplace_back(
            std::move(stolen_tasks.top()));
        stolen_tasks.pop();
      }
      ul_local.unlock();
      return true; // steal success, no need to WaitForTask()
    }
  }

  return false; // need to WaitForTask() or TryStealWork() in next loop
}
