#pragma once
#include <libcotask/task.h>

#include <functional>
#include <map>
#include <memory>
#include <queue>
#include <set>
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
        // void task will not run to this.
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

  template <typename T> class ValueTask : public Task {
  public:
    typedef std::function<void(std::shared_ptr<ValueTask<T>>)> ProcType;
    explicit ValueTask(ProcType proc) : Task() { SetProc(proc); }
    ValueTask() = default;
    void SetProc(ProcType proc) {
      SetProcContext_<ValueTask<T>>(shared_from_this<ValueTask<T>>(), proc);
    }
    inline void Complete(T result) {
      result_ = result;
      Task::Stop();
    }
    inline T GetResult() { return result_; }
    inline State GetState() { return state; }

  private:
    T result_;
  };

  class TaskManager {
  public:
    void Wait(std::shared_ptr<Task> waiter, std::shared_ptr<Task> waitting);
    void StopTask(std::shared_ptr<Task> task);
    int RunOnce();
    void Stop();
    bool IsEmpty();
    bool is_stopped = false;

  private:
    std::set<std::shared_ptr<Task>> completed_tasks_;
    std::map<std::shared_ptr<Task>, std::shared_ptr<Task>> waitting_tasks_;
    std::queue<std::shared_ptr<Task>> starting_tasks_;
  };

  std::shared_ptr<Task> GetCurrentTask();

  extern thread_local TaskManager task_mgr;

#define _async(...) GET_MACRO_V2(__VA_ARGS__, vt_async, task_async)(__VA_ARGS__)
#define task_async(proc) smark::util::make_shared<smark::tasks::Task>(proc)
#define vt_async(T, proc) \
  smark::util::make_shared<smark::tasks::Task, smark::tasks::ValueTask<T>>(proc)
#define void_task(...) GET_MACRO_V0_1(_0, ##__VA_ARGS__, vt_void_task, task_void_task)(__VA_ARGS__)
#define task_void_task() smark::util::make_shared<smark::tasks::Task>()
#define vt_void_task(T) smark::util::make_shared<smark::tasks::Task, smark::tasks::ValueTask<T>>()

  template <typename T> std::shared_ptr<T> await(std::shared_ptr<T> task) {
    auto current_task = GetCurrentTask();
    task_mgr.Wait(current_task, std::dynamic_pointer_cast<Task>(task));

    current_task->Yield();

    return std::dynamic_pointer_cast<T>(task);
  }
#ifdef DEBUG
  extern thread_local std::map<cotask::task<>*, std::shared_ptr<Task>> map2task;
#endif
}  // namespace smark::tasks