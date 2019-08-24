//
// Created by chinthaka on 8/24/19.
//

#include "SchedulerService.h"
#include "Scheduler.h"

using namespace Bosma;

Logger scheduler_logger;

void SchedulerService::startScheduler() {
    unsigned int max_n_threads = 12;

    PerformanceUtil util;
    Utils utils;

    std::string schedulerEnabled = utils.getJasmineGraphProperty("org.jasminegraph.scheduler.enabled");

    if (schedulerEnabled == "true") {

        scheduler_logger.log("#######SCHEDULER ENABLED#####","info");

        Bosma::Scheduler s(max_n_threads);

        s.every(std::chrono::seconds(5), util.reportPerformanceStatistics);

        std::this_thread::sleep_for(std::chrono::minutes(10));
    }


}