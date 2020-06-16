#define INIT_TASK int __task_count = __COUNTER__
#define SUB_TASK(task) \
  (void)__COUNTER__;   \
  task++
#define END_TASK __task_count = __COUNTER__ - __task_count - 1

// do not use '==' to compare string
// do not use string.compare: fail on "This is a response"
#define STR_COMPARE(str, value) strcmp(str.c_str(), value) == 0