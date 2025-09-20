#include "module.hpp"
#include "log.hpp"

RAMPAGE_START

struct LogStage {};
struct PostLogStage {};

int LogModule::onLoad() {
  logInit();
  m_host->setLogFuncs(logGeneric, logError);

  auto& pipeline = m_host->getPipeline();
  Pipeline::Group& group = pipeline.createGroup("LogGroup", 1.0f)
    .createStage<LogStage>()
    .createStage<PostLogStage>();

  group.attachToStage<LogStage>(
    [](IHost& host) {
      host.log("We logging yo\n");
      return 0;
    });

  group.attachToStage<PostLogStage>(
    [](IHost& host) {
      host.log("From stage 2\n");
      return 0;
    });

  return 0;
}

int LogModule::onUnload() {
  m_host->setLogFuncs(nullptr, nullptr);

  return 0;
}

int LogModule::onUpdate() {
  EntityWorld& world = m_host->getWorld();

  return 0;
}

RAMPAGE_END