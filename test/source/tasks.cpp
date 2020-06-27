#include "platform.h"
DISABLE_SOME_WARNINGS
#include <doctest/doctest.h>

#include "tasks.h"
#include "util.h"

using namespace smark::tasks;
using namespace smark::util;

TEST_CASE("Task_Simple") {
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

TEST_CASE("Task_Map2Task") {
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

TEST_CASE("Task_Async") {
  int task = 0;
  INIT_TASK;

  std::thread([&task]() {
    auto t1 = _async([&](std::shared_ptr<Task> this_task) {
      (void)this_task;
      auto child_task = task_async([&](std::shared_ptr<Task> this_task) {
        SUB_TASK(task);
        this_task->Stop();
      });
      SUB_TASK(task);
      await(child_task);
      SUB_TASK(task);
    });
    t1->Start();

    while (task_mgr.RunOnce())
      ;
  }).join();

  END_TASK;
  CHECK(task == __task_count);
}

TEST_CASE("Task_StopFromOutside") {
  int task = 0;
  INIT_TASK;

  std::thread([&task]() {
    std::function func([]() {});

    auto t1 = _async([&](std::shared_ptr<Task> this_task) {
      (void)this_task;
      auto child_task = task_async([&](std::shared_ptr<Task> this_task) {
        SUB_TASK(task);
        func = [this_task]() { this_task->Stop(); };
      });
      SUB_TASK(task);
      await(child_task);
      SUB_TASK(task);
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

TEST_CASE("Task_StopBeforeWait") {
  int task = 0;
  INIT_TASK;

  std::thread([&task]() {
    auto proc = [&](std::shared_ptr<Task> this_task) {
      (void)this_task;
      auto child_task = _async([](auto) {});
      child_task->Stop();
      SUB_TASK(task);
      await(child_task);
      SUB_TASK(task);
    };
    auto t1 = _async(proc);
    t1->Start();

    task_mgr.RunOnce();  // restart t1

    CHECK(task_mgr.RunOnce() == 0);  // ensure no running task remain.
  }).join();

  END_TASK;
  CHECK(task == __task_count);
}

TEST_CASE("Task_ValueTask") {
  int task = 0;
  INIT_TASK;

  std::thread([&task]() {
    auto co_task = _async(int, [&](std::shared_ptr<ValueTask<int>> this_task) {
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

TEST_CASE("Task_ValueTaskAsync") {
  int task = 0;
  INIT_TASK;

  std::thread([&task]() {
    auto proc = [&](std::shared_ptr<Task> this_task) {
      (void)this_task;
      auto child_task = _async(int, [&](std::shared_ptr<ValueTask<int>> t) {
        SUB_TASK(task);
        t->Complete(10);
      });
      SUB_TASK(task);
      CHECK(await(child_task)->GetResult() == 10);
      SUB_TASK(task);
    };  // TODO: why? lambda-expression in template-argument only available with ‘-std=c++2a’ or
        // ‘-std=gnu++2a’
    auto t1 = _async(proc);
    t1->Start();

    while (task_mgr.RunOnce())
      ;
  }).join();

  END_TASK;
  CHECK(task == __task_count);
}