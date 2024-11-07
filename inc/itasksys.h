#pragma once
using TaskID = int;
class IRunnable {
  public:
    virtual ~IRunnable() = default;

    /** 
      @brief Executes an instance of the task as part of a bulk task launch.
      
      @param task_id: the current task identifier. This value will be 
      between 0 and num_total_tasks-1.

      @param num_total_tasks: the total number of tasks in the bulk task launch.
     */
    virtual void RunTask(TaskID task_id, int num_total_tasks) = 0;
};

class ITaskSystem {
  public:
    /** 
      @brief
        Instantiates a  task system.

      @param num_threads
        the maximum number of threads that the task system can use.
    */
    explicit ITaskSystem(int num_threads){};
    virtual ~ITaskSystem()=default;
    virtual const char* Name() = 0;

    /**
      @brief
        Executes a bulk task launch of a num_total_tasks. Task execution is 
        synchronous with the calling thread, so run() will return only when the
        execution of all tasks is complete.
    */
    virtual void Run(IRunnable* runnable, int num_total_tasks) = 0;
};