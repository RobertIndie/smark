#pragma once
#include <libcotask/task.h>

#include <functional>
#include <map>
#include <memory>
#include <queue>
#include <vector>

namespace smark::tasks {
  class TaskManager;
  class Task;

  typedef std::function<void(std::shared_ptr<Task>)> TaskProc;

  class Task : std::enable_shared_from_this<Task> {
  public:
    enum State { New, Runable, Dead };
    State state = State::New;
    Task(TaskProc proc);
    void Start();
    void Yield();
    void Resume();
    void Wait(std::shared_ptr<Task> task);
    void WaitAll(const std::vector<std::shared_ptr<Task>>* task_list);
    template <typename ResultType> void Complete(ResultType* result);
    template <typename ResultType> ResultType* GetResult();

  private:
    cotask::task<>::ptr_t task_ptr_;
    void* result_;
  };
  class TaskManager {
  public:
    std::shared_ptr<Task> NewTask(TaskProc proc);
    void Wait(std::shared_ptr<Task> waiter, std::shared_ptr<Task> waitting);
    void StopTask(std::shared_ptr<Task> task);
    void RunOnce();

  private:
    std::vector<std::shared_ptr<Task>> task_list_;
    std::map<std::shared_ptr<Task>, std::shared_ptr<Task>> waitting_tasks_;
    std::queue<std::shared_ptr<Task>> starting_tasks_;
  };
  std::shared_ptr<Task> GetCurrentTask();
  std::shared_ptr<Task> async(TaskProc proc);
  std::shared_ptr<Task> await(std::shared_ptr<Task> task);
  extern thread_local TaskManager task_mgr;
}  // namespace smark::tasks