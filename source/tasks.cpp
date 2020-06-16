#include "tasks.h"

#include <algorithm>

namespace smark::tasks {
  thread_local std::map<cotask::task<>*, std::shared_ptr<Task>> map2task;

  Task::Task(TaskProc proc) {
    auto this_ptr = shared_from_this();
    task_ptr_ = cotask::task<>::create([=]() {
      map2task[cotask::this_task::get<cotask::task<>>()] = this_ptr;

      proc(this_ptr);

      // TODO: add to finally
      this_ptr->state = State::Dead;
      map2task.erase(cotask::this_task::get<cotask::task<>>);
    });
  }

  void Task::Start() {
    state = State::Runable;
    task_ptr_->start();
  }

  void Task::Yield() { task_ptr_->yield(); }

  void Task::Resume() { task_ptr_->resume(); }

  void Task::Wait(std::shared_ptr<Task> task) { task_mgr.Wait(shared_from_this(), task); }

  void Task::WaitAll(const std::vector<std::shared_ptr<Task>>* task_list) {
    for (auto iter = task_list->begin(); iter != task_list->end(); iter++) {
      task_mgr.Wait(shared_from_this(), *iter);
    }
  }

  template <typename ResultType> void Task::Complete(ResultType* result) {
    result_ = reinterpret_cast<void*>(result);
    task_mgr.StopTask(shared_from_this());
  }

  template <typename ResultType> ResultType* Task::GetResult() {
    return reinterpret_cast<ResultType*>(result_);
  }

  std::shared_ptr<Task> TaskManager::NewTask(TaskProc proc) {
    auto task = std::make_shared<Task>(proc);
    task_list_.push_back(task);
    return task;
  }

  void TaskManager::Wait(std::shared_ptr<Task> waiter, std::shared_ptr<Task> waitting) {
    waitting_tasks_[waitting] = waiter;
  }

  void TaskManager::StopTask(std::shared_ptr<Task> task) {
    auto iter = waitting_tasks_.find(task);
    if (iter != waitting_tasks_.end()) {
      starting_tasks_.push(iter->second);
      waitting_tasks_.erase(iter);
    }
    auto t = std::find(task_list_.begin(), task_list_.end(), task);
    if (t != task_list_.end()) {
      task_list_.erase(t);
    }
  }

  void TaskManager::RunOnce() {
    auto task = starting_tasks_.front();
    starting_tasks_.pop();
    switch (task->state) {
      case Task::State::New:
        task->Start() break;

      case Task::State::Runable:
        task->Resume();
        break;

      default:
        break;
    }
  }

  std::shared_ptr<Task> GetCurrentTask() {
    return map2task[cotask::this_task::get<cotask::task<>>()];
  }

  std::shared_ptr<Task> async(TaskProc proc) {
    auto task = task_mgr.NewTask(proc);
    return task;
  }

  std::shared_ptr<Task> await(std::shared_ptr<Task> task) {
    auto current_task = GetCurrentTask();
    task_mgr.Wait(current_task, task);

    current_task->Yield();
  }

  thread_local TaskManager task_mgr;
}  // namespace smark::tasks