#include "pipeline.hpp"

RAMPAGE_START

float now() {
  static std::chrono::steady_clock::time_point start_time = std::chrono::steady_clock::now();
  auto current_time = std::chrono::steady_clock::now();

  return static_cast<float>(
             std::chrono::duration_cast<std::chrono::microseconds>(current_time - start_time).count()) /
      (1000000);
}

void Pipeline::run(IHost& host) {
  const float nowTime = now();
  float deltaTime = now() - m_lastTime;
  m_lastTime = nowTime;

  // Clamp deltaTime to avoid big spikes
  if (deltaTime > 0.25f)
    deltaTime = 0.25f;

  for (Group& group : m_groups) {
    group.m_tickAccum += deltaTime;

    // Only allow a single tick per frame, even if falling behind
    float realRate = group.m_invRate;
    if (realRate < 0)
      realRate = m_invDefaultRate;
    if (group.m_tickAccum < realRate)
      continue;
    group.m_tickAccum -= realRate;

    for (Stage& stage : group.m_stages) {
      for (const auto& task : stage.tasks)
        task(host, realRate);
      for (const auto& task : stage.worldTasks)
        task(host.getWorld(), realRate);
    }
  }
}

RAMPAGE_END
