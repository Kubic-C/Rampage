#pragma once

#include "base.hpp"
#include "TaskScheduler.hpp"

struct b2TaskSet : public enki::ITaskSet {
  b2TaskCallback* m_task = nullptr;
  void* m_taskContext = nullptr;

  void ExecuteRange(enki::TaskSetPartition range, uint32_t threadNum) override {
    m_task(range.start, range.end, threadNum, m_taskContext);
  }
};

inline void* enki_b2EnqueueTaskCallback(b2TaskCallback* task, int itemCount, int minRange, void* taskContext, void* userContext) {
  enki::TaskScheduler& scheduler = *reinterpret_cast<enki::TaskScheduler*>(userContext);

  b2TaskSet* taskSet = new b2TaskSet();
  taskSet->m_task = task;
  taskSet->m_taskContext = taskContext;
  taskSet->m_MinRange = minRange;
  taskSet->m_SetSize = itemCount;
  scheduler.AddTaskSetToPipe(taskSet);

  return taskSet;
}

void enki_b2FinishTaskCallback(void* userTask, void* userContext) {
  enki::TaskScheduler& scheduler = *reinterpret_cast<enki::TaskScheduler*>(userContext);
  enki::ITaskSet* set = reinterpret_cast<enki::ITaskSet*>(userTask);

  scheduler.WaitforTask(set);
  delete set;
}