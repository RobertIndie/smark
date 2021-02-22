#include "tasks.h"

#include <algorithm>

namespace smark::tasks {
  thread_local std::map<cotask::task<>*, std::shared_ptr<Task>> map2task;

  Task::Task(TaskProc proc) { SetProc(proc); }

  void Task::SetProc(TaskProc proc) { SetProcContext_<Task>(shared_from_this(), proc); }

  void Task::RegisterTaskToMap2Task(std::shared_ptr<Task> task_ptr) {
    map2task[cotask::this_task::get<cotask::task<>>()] = task_ptr;
  }

  void Task::UnregisterTaskFromMap2Task() {
    map2task.erase(cotask::this_task::get<cotask::task<>>());
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

  void Task::Stop() {
    state = State::Dead;
    task_mgr.StopTask(shared_from_this());
  }

  void TaskManager::Wait(std::shared_ptr<Task> waiter, std::shared_ptr<Task> waitting) {
    if (waitting->state == Task::State::Dead) {  // waitting taks is completed.
      completed_tasks_.erase(waitting);
      starting_tasks_.push(waiter);  // start waiter.
      return;
    }

    // waitting task is no completed currently.
    waitting_tasks_[waitting] = waiter;
    starting_tasks_.push(waitting);
  }

  void TaskManager::StopTask(std::shared_ptr<Task> task) {
    auto iter = waitting_tasks_.find(task);
    if (iter != waitting_tasks_.end()) {
      starting_tasks_.push(iter->second);
      waitting_tasks_.erase(iter);
      return;
    }

    // if no waitter is waitting, add to completed_tasks_ to avoid being freed when task is
    // complete.
    completed_tasks_.insert(task);
  }

  int TaskManager::RunOnce() {
    if (is_stopped) return 0;
    int run_task_count = 0;
    while (starting_tasks_.size()) {
      auto task = starting_tasks_.front();  // use front before size check is undefined behaivour.
      starting_tasks_.pop();
      switch (task->state) {
        case Task::State::New:
          task->Start();
          run_task_count++;
          break;

        case Task::State::Runable:
          task->Resume();
          run_task_count++;
          break;

        default:
          break;
      }
    }
    return run_task_count;
  }

  bool TaskManager::IsEmpty() {
    if (completed_tasks_.size() || waitting_tasks_.size() || starting_tasks_.size()) return false;
    return true;
  }

  void TaskManager::Stop() { is_stopped = true; }

  std::shared_ptr<Task> GetCurrentTask() {
    return map2task[cotask::this_task::get<cotask::task<>>()];
  }

  thread_local TaskManager task_mgr;
}  // namespace smark::tasks