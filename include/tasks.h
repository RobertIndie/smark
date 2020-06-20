#pragma once
#include <libcotask/task.h>

#include <functional>
#include <map>
#include <memory>
#include <queue>
#include <vector>

#include "debug.h"
#include "util.h"

namespace smark::tasks {
  class TaskManager;
  class Task;

  typedef std::function<void(std::shared_ptr<Task>)> TaskProc;

  // TODO: add exception handler
  class Task : public smark::util::enable_shared_from_this<Task> {
  public:
    typedef std::function<void(std::shared_ptr<Task>)> ProcType;
    enum State { New, Runable, Dead };
    State state = State::New;
    explicit Task(TaskProc proc);
    Task() = default;
    void SetProc(TaskProc proc);
    void Start();
    void Yield();
    void Resume();
    void Wait(std::shared_ptr<Task> task);
    void WaitAll(const std::vector<std::shared_ptr<Task>>* task_list);
    void Stop();
    virtual ~Task() = default;

  protected:
    template <typename TaskType>
    void SetProcContext_(std::shared_ptr<TaskType> task_ptr,
                         std::function<void(std::shared_ptr<TaskType>)> proc) {
      task_ptr_ = cotask::task<>::create([&, task_ptr, proc]() {
        DEFER(std::dynamic_pointer_cast<Task>(task_ptr)->state = State::Dead;
              UnregisterTaskFromMap2Task();)
        RegisterTaskToMap2Task(std::dynamic_pointer_cast<Task>(task_ptr));
        proc(task_ptr);
      });
    }

  private:
    cotask::task<>::ptr_t task_ptr_;
    void* result_;
    void RegisterTaskToMap2Task(std::shared_ptr<Task> task_ptr);
    void UnregisterTaskFromMap2Task();
  };

  template <typename T> class ValueTask : public smark::util::enable_shared_from_this<ValueTask<T>>,
                                          public Task {
  public:
    typedef std::function<void(std::shared_ptr<ValueTask<T>>)> ProcType;
    explicit ValueTask(ProcType proc) : Task() { SetProc(proc); }
    void SetProc(ProcType proc) {
      SetProcContext_<ValueTask<T>>(enable_shared_from_this<ValueTask<T>>::shared_from_this(),
                                    proc);
    }
    inline void Complete(std::shared_ptr<T> result) {
      result_ = std::static_pointer_cast<void>(result);
      Task::Stop();
    }
    inline std::shared_ptr<T> GetResult() { return std::dynamic_pointer_cast<T>(result_); }
    inline State GetState() { return state; }

  private:
    std::shared_ptr<void*> result_;
  };

  class TaskManager {
  public:
    std::shared_ptr<Task> NewTask(TaskProc proc);
    void AddTask(std::shared_ptr<Task> task);
    template <typename TaskType> std::shared_ptr<TaskType> AddTask(std::shared_ptr<TaskType> task) {
      AddTask(std::dynamic_pointer_cast<Task>(task));
      return task;
    }
    void Wait(std::shared_ptr<Task> waiter, std::shared_ptr<Task> waitting);
    void StopTask(std::shared_ptr<Task> task);
    int RunOnce();
    void Stop();
    bool is_stopped = false;

  private:
    std::vector<std::shared_ptr<Task>> task_list_;
    std::map<std::shared_ptr<Task>, std::shared_ptr<Task>> waitting_tasks_;
    std::queue<std::shared_ptr<Task>> starting_tasks_;
  };
  std::shared_ptr<Task> GetCurrentTask();
// std::shared_ptr<Task> async(TaskProc proc);
#define _async(...) GET_MACRO_V2(__VA_ARGS__, vt_async, task_async)(__VA_ARGS__)
#define task_async(proc) smark::tasks::task_mgr.NewTask(proc);
#define vt_async(T, proc)                                     \
  smark::tasks::task_mgr.AddTask<smark::tasks::ValueTask<T>>( \
      smark::util::make_shared<smark::tasks::ValueTask<T>>(proc))
  // template <typename T> std::shared_ptr<T> async(T::ProcType[T = T] proc) {
  //   auto task = util::make_shared<T>(proc);
  //   task_mgr.AddTask(std::dynamic_pointer_cast<Task>(T));
  //   return task;
  // }
  std::shared_ptr<Task> await(std::shared_ptr<Task> task);
  extern thread_local TaskManager task_mgr;
#ifdef DEBUG
  extern thread_local std::map<cotask::task<>*, std::shared_ptr<Task>> map2task;
#endif
}  // namespace smark::tasks