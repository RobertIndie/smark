#include "platform.h"
DISABLE_SOME_WARNINGS
#include <doctest/doctest.h>

#include "tasks.h"
#include "util.h"

using namespace smark::tasks;
using namespace smark::util;

TEST_CASE("TaskSimple") {
  int task = 0;
  INIT_TASK;

  std::thread([&task]() {
    auto co_task = make_shared<Task>([&](std::shared_ptr<Task> this_task) {
      SUB_TASK(task);
      this_task->Yield();
      SUB_TASK(task);
    });
    CHECK(co_task->state == Task::State::New);
    co_task->Start();
    CHECK(co_task->state == Task::State::Runable);
    co_task->Resume();
    CHECK(co_task->state == Task::State::Dead);
  }).join();

  END_TASK;
  CHECK(task == __task_count);
}

TEST_CASE("TaskMap2Task") {
  int task = 0;
  INIT_TASK;

  std::thread([&task]() {
    SUB_TASK(task);
    cotask::task<>* t = nullptr;
    auto co_task = make_shared<Task>([&t](std::shared_ptr<Task> this_task) {
      t = cotask::this_task::get<cotask::task<>>();
      CHECK(GetCurrentTask().get() == this_task.get());
      this_task->Yield();
      CHECK(GetCurrentTask().get() == this_task.get());
    });
    co_task->Start();
    auto r1 = map2task[t];
    CHECK(r1.get() == co_task.get());
    co_task->Resume();
    CHECK(map2task.find(t) == map2task.end());
  }).join();

  END_TASK;
  CHECK(task == __task_count);
}

TEST_CASE("TaskAsync") {
  int task = 0;
  INIT_TASK;

  std::thread([&task]() {
    auto t1 = task_mgr.NewTask([&](std::shared_ptr<Task> this_task) {
      (void)this_task;
      auto child_task = async([&](std::shared_ptr<Task> this_task) {
        SUB_TASK(task);
        this_task->Stop();
      });
      SUB_TASK(task);
      await(child_task);
    });
    t1->Start();

    while (task_mgr.RunOnce())
      ;
  }).join();

  END_TASK;
  CHECK(task == __task_count);
}

TEST_CASE("TaskStopFromOutside") {
  int task = 0;
  INIT_TASK;

  std::thread([&task]() {
    std::function func([]() {});

    auto t1 = task_mgr.NewTask([&](std::shared_ptr<Task> this_task) {
      (void)this_task;
      auto child_task = async([&](std::shared_ptr<Task> this_task) {
        SUB_TASK(task);
        func = [this_task]() { this_task->Stop(); };
      });
      SUB_TASK(task);
      await(child_task);
    });
    t1->Start();

    task_mgr.RunOnce();  // run child_task
    func();              // set result of child_task
    task_mgr.RunOnce();  // let child_task trigger parent_task

    CHECK(task_mgr.RunOnce() == 0);  // ensure no running task remain.
  }).join();

  END_TASK;
  CHECK(task == __task_count);
}

TEST_CASE("TaskValueTask") {
  int task = 0;
  INIT_TASK;

  std::thread([&task]() {
    auto co_task = make_shared<ValueTask<int>>([&](std::shared_ptr<ValueTask<int>> this_task) {
      SUB_TASK(task);
      this_task->Yield();
      SUB_TASK(task);
    });
    CHECK(co_task->GetState() == Task::State::New);
    co_task->Start();
    CHECK(co_task->GetState() == Task::State::Runable);
    co_task->Resume();
    CHECK(co_task->GetState() == Task::State::Dead);
  }).join();

  END_TASK;
  CHECK(task == __task_count);
}
