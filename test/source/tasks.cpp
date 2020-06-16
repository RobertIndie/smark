#include "platform.h"
DISABLE_SOME_WARNINGS
#include <doctest/doctest.h>

#include "tasks.h"
#include "util.h"

using namespace smark::tasks;

TEST_CASE("SimpleTask") {
  int task = 0;
  INIT_TASK;
  Task co_task([&](std::shared_ptr<Task> this_task) {
    SUB_TASK(task);
    this_task->Yield();
    SUB_TASK(task);
  });
  CHECK(co_task.state == Task::State::New);
  co_task.Start();
  CHECK(co_task.state == Task::State::Runable);
  co_task.Resume();
  CHECK(co_task.state == Task::State::Dead);
  END_TASK;
  CHECK(task == __task_count);
}
