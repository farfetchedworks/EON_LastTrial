#include "mcv_platform.h"
#include "module_multithreading.h"
#include "tbb/tbb.h"

bool CModuleMultithreading::start() {
  int nDefThreads = tbb::task_scheduler_init::default_num_threads();
  tbb::task_scheduler_init init(nDefThreads);
  return true;
}

void CModuleMultithreading::stop() {
  //todo: stop the scheduler
}

void CModuleMultithreading::update(float dt) {
}
