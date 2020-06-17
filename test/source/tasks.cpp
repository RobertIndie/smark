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
  END_TASK;
  CHECK(task == __task_count);
}

TEST_CASE("TaskMap2Task") {
  cotask::task<>* t = nullptr;
  auto co_task = make_shared<Task>([&t](std::shared_ptr<Task> this_task) {
    t = cotask::this_task::get<cotask::task<>>();
    this_task->Yield();
  });
  co_task->Start();
  auto r1 = map2task[t];
  CHECK(r1.get() == co_task.get());
  co_task->Resume();
  CHECK(map2task.find(t) == map2task.end());
}
