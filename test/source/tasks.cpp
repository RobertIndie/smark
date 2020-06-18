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
        auto result = new int(1);
        this_task->Complete<int>(result);
      });
      SUB_TASK(task);
      auto result = await(child_task)->GetResult<int>();
      CHECK(*result == 1);
    });
    t1->Start();

    while (task_mgr.RunOnce())
      ;
  }).join();

  END_TASK;
  CHECK(task == __task_count);
}
