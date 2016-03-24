#include <iostream>
#include <algorithm>
#include <thread>
#include <sstream>
#include "DDSScheduler.h"

using namespace std;
using namespace mesos;

void logToFile(std::string);

DDSScheduler::DDSScheduler(condition_variable &mesosStarted,
                           const ExecutorInfo &executorInfo,
                           const Resources &resourcesPerTask,
                           const ContainerInfo& containerInfo)
        : mesosStarted(mesosStarted),
          executorInfo(executorInfo),
          resourcesPerTask(resourcesPerTask),
          containerInfo ( containerInfo )
{
    logToFile("DDS Framework Constructor");
}

DDSScheduler::~DDSScheduler() {
    logToFile("DDS Framework Destructor");
}

void DDSScheduler::registered(
        SchedulerDriver *driver,
        const FrameworkID &frameworkId,
        const MasterInfo &masterInfo
) {
    ostringstream ostr;
    ostr << "DDS Framework: Registered with Master ID: "
         << masterInfo.id()
         << " and Framework "
         << frameworkId.SerializeAsString()
         << endl;
    logToFile(ostr.str());
    mesosStarted.notify_one();
}

void DDSScheduler::reregistered(
        SchedulerDriver *driver,
        const MasterInfo &masterInfo
) {
    logToFile("DDS Framework reregistered");
}

void DDSScheduler::disconnected(SchedulerDriver *driver) {
    logToFile("DDS Framework Disconnected");
}

void DDSScheduler::resourceOffers(
        SchedulerDriver *driver,
        const vector <Offer> &offers
) {
    lock_guard<std::mutex> lock(ddsMutex);
    logToFile("DDS Framework Resource Offers");



    for (const Offer& offer : offers) {

        // If no tasks to schedule, decline all offers
        if (waitingTasks.size() == 0) {
            driver->declineOffer(offer.id());
            logToFile("DDS Framework Resource Offers - Declined offers, nothing to schedule");
            continue;
        }

        // There's something in the waitingList..
        vector<TaskInfo> taskInfoList;
        Resources offeredResources = offer.resources();
        while (offeredResources.contains(resourcesPerTask) && waitingTasks.size() > 0) {
            waitingTasks.front().mutable_slave_id()->MergeFrom(offer.slave_id());
            taskInfoList.push_back(waitingTasks.front());
            runningTasks.push_back(waitingTasks.front());
            offeredResources -= resourcesPerTask;
            waitingTasks.pop_front();
        }

        // Process taskInfoList
        if (taskInfoList.size() == 0) {
            driver->declineOffer(offer.id());
            logToFile("DDS Framework: Declined offer, no suitable resources for the task/s");
        } else {
            logToFile("DDS Framework: Accepting some or all of resources from offer");
            driver->launchTasks(offer.id(), taskInfoList);
        }
    }
}

void DDSScheduler::offerRescinded(
        SchedulerDriver *driver,
        const OfferID &offerId
) {
    logToFile("DDS Framework Offer Rescinded");
}

void DDSScheduler::statusUpdate(
        SchedulerDriver *driver,
        const TaskStatus &status
) {
    lock_guard<std::mutex> lock(ddsMutex);
    logToFile("DDS Framework Status Update");

    switch (status.state()) {
        case TASK_STAGING:
            break;
        case TASK_STARTING:
            break;
        case TASK_RUNNING:
            break;
        case TASK_FINISHED:
        {
            vector<TaskInfo>::iterator item = find_if(runningTasks.begin(), runningTasks.end(),
                                                      [&status](const TaskInfo& taskInfo) -> bool {
                                                          return taskInfo.task_id() == status.task_id();
                                                      }
            );
            if (item == runningTasks.end()) {
                logToFile("STATUPDATE ERROR: TASK NOT FOUND!!!!!");
            } else {
                // move this item
                finishedTasks.push_back(status);
                runningTasks.erase(item);
            }
        }
            break;
        case TASK_FAILED:
        case TASK_KILLED:
        case TASK_LOST:
        case TASK_ERROR:
        {
            // Restore to the waiting queue
            vector<TaskInfo>::iterator item = find_if(runningTasks.begin(), runningTasks.end(),
                                                      [&status](const TaskInfo& taskInfo) -> bool {
                                                          return taskInfo.task_id() == status.task_id();
                                                      }
            );
            logToFile("Reason: " + status.message());
            if (item == runningTasks.end()) {
                logToFile("DDS Framework: STATUPDATE ERROR: TASK NOT FOUND!!!!! Cannot ReQueue");
            } else {
                // move this item
                waitingTasks.push_back(move(*item));
                runningTasks.erase(item);
                logToFile("DDS Framework: Task ERROR (ReQueued) ID: ");
                logToFile(status.task_id().value());
            }
        }
            break;
        default:
            logToFile("DDS Framewrk - Unknown Task Status Value");
            break;
    }


}

void DDSScheduler::frameworkMessage(
        SchedulerDriver *driver,
        const ExecutorID &executorId,
        const SlaveID &slaveId,
        const string &data
) {
    logToFile("DDS Framework Framework Message");
}

void DDSScheduler::slaveLost(
        SchedulerDriver *driver,
        const SlaveID &slaveId
) {
    // Do we need to reschedule tasks?


}

void DDSScheduler::executorLost(SchedulerDriver *driver,
                                const ExecutorID &executorId,
                                const SlaveID &slaveId,
                                int status
) {
    logToFile("DDS Framework Executor Lost");
}

void DDSScheduler::error(SchedulerDriver *driver, const string &message) {
    logToFile("DDS Framework Error");
    logToFile(message);
}

// Synchronised
void DDSScheduler::addAgents(const DDSSubmitInfo& submit) {
    lock_guard<std::mutex> lock(ddsMutex);
    logToFile("Adding Agents from DDS-Submit");

    for (size_t i = 1; i <= submit.m_nInstances; ++i) {
        TaskInfo taskInfo;
        taskInfo.set_name(string("DDS Framework Task #") + to_string(i));
        taskInfo.mutable_task_id()->set_value(to_string(i));
        //taskInfo.mutable_slave_id()->MergeFrom(offer.slave_id());
        taskInfo.mutable_resources()->MergeFrom(resourcesPerTask);
        taskInfo.mutable_container()->MergeFrom(containerInfo);

        // Either execute command or executor
        CommandInfo commandInfo;
        ostringstream ostr;
        // Create temporary directory and enter it
        ostr << "cd / && mkdir DDSEnvironment && cd DDSEnvironment && " ;
        // Copy Worker Package to this directory and execute script
        ostr << "cp " << submit.m_wrkPackagePath << " . && ./$(basename " << submit.m_wrkPackagePath << ")";
        // Set command
        commandInfo.set_value(ostr.str());

        taskInfo.mutable_command()->MergeFrom(commandInfo);
        //taskQueue.push();
        waitingTasks.push_back(taskInfo);
    }

}