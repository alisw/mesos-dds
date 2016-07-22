#include <iostream>
#include <algorithm>
#include <thread>
#include <sstream>

// Logging
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>

// File System
#include <boost/filesystem.hpp>

#include "DDSScheduler.h"

using namespace std;
using namespace mesos;

DDSScheduler::DDSScheduler()
{
    BOOST_LOG_TRIVIAL(trace) << "DDS Framework Constructor" << endl;
}

DDSScheduler::~DDSScheduler() {
    BOOST_LOG_TRIVIAL(trace) <<  "DDS Framework Destructor" << endl;
}

void DDSScheduler::registered(
        SchedulerDriver *driver,
        const FrameworkID &frameworkId,
        const MasterInfo &masterInfo
) {
    BOOST_LOG_TRIVIAL(trace)
         << "DDS Framework: Registered with Master ID: "
         << masterInfo.id()
         << " and Framework "
         << frameworkId.SerializeAsString()
         << endl;
}

void DDSScheduler::reregistered(
        SchedulerDriver *driver,
        const MasterInfo &masterInfo
) {
    BOOST_LOG_TRIVIAL(trace) << "DDS Framework reregistered" << endl;
}

void DDSScheduler::disconnected(SchedulerDriver *driver) {
    BOOST_LOG_TRIVIAL(trace) << "DDS Framework Disconnected" << endl;
}

void DDSScheduler::resourceOffers(
        SchedulerDriver *driver,
        const vector <Offer> &offers
) {
    lock_guard<std::mutex> lock(ddsMutex);
    BOOST_LOG_TRIVIAL(trace) << "DDS Framework Resource Offers" << endl;

    for (const Offer& offer : offers) {

        // If no tasks to schedule, decline all offers
        if (waitingTasks.size() == 0) {
            driver->declineOffer(offer.id());
            BOOST_LOG_TRIVIAL(info) << "DDS Framework Resource Offers - Declined offers, nothing to schedule" << endl;
            continue;
        }

        // There's something in the waitingList..
        vector<TaskInfo> taskInfoList;
        Resources offeredResources = offer.resources();
        while (waitingTasks.size() > 0 && offeredResources.contains(waitingTasks.front().resources())) {
            waitingTasks.front().mutable_slave_id()->MergeFrom(offer.slave_id());
            taskInfoList.push_back(waitingTasks.front());
            runningTasks.push_back(waitingTasks.front());
            offeredResources -= waitingTasks.front().resources();
            waitingTasks.pop_front();
        }

        // Process taskInfoList
        if (taskInfoList.size() == 0) {
            driver->declineOffer(offer.id());
            BOOST_LOG_TRIVIAL(info) << "DDS Framework: Declined offer, no suitable resources for the task/s" << endl;
        } else {
            BOOST_LOG_TRIVIAL(info) << "DDS Framework: Accepting some or all of resources from offer" << endl;
            driver->launchTasks(offer.id(), taskInfoList);
        }
    }
}

void DDSScheduler::offerRescinded(
        SchedulerDriver *driver,
        const OfferID &offerId
) {
    BOOST_LOG_TRIVIAL(trace) << "DDS Framework Offer Rescinded" << endl;
}

void DDSScheduler::statusUpdate(
        SchedulerDriver *driver,
        const TaskStatus &status
) {
    lock_guard<std::mutex> lock(ddsMutex);
    BOOST_LOG_TRIVIAL(trace) << "DDS Framework Status Update" << endl;

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
                BOOST_LOG_TRIVIAL(error) << "STATUS UPDATE ERROR: TASK NOT FOUND!!!!!" << endl;
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
            BOOST_LOG_TRIVIAL(error) << "Reason: " + status.message() << endl;
            if (item == runningTasks.end()) {
                BOOST_LOG_TRIVIAL(error) << "DDS Framework: STATUS UPDATE ERROR: TASK NOT FOUND!!!!! Cannot ReQueue" << endl;
            } else {
                // move this item
                waitingTasks.push_back(move(*item));
                runningTasks.erase(item);
                BOOST_LOG_TRIVIAL(error) << "DDS Framework: Task ERROR (ReQueued) ID: "
                                         << status.task_id().value() << endl;
            }
        }
            break;
        default:
            BOOST_LOG_TRIVIAL(warning) << "DDS Framework - Unknown Task Status Value" << endl;
            break;
    }


}

void DDSScheduler::frameworkMessage(
        SchedulerDriver *driver,
        const ExecutorID &executorId,
        const SlaveID &slaveId,
        const string &data
) {
    BOOST_LOG_TRIVIAL(trace) << "DDS Framework Framework Message" << endl;
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
    BOOST_LOG_TRIVIAL(trace) << "DDS Framework Executor Lost" << endl;
}

void DDSScheduler::error(SchedulerDriver *driver, const string &message) {
    BOOST_LOG_TRIVIAL(error) << "DDS Framework Error: " << message << endl;
}

void DDSScheduler::setFutureTaskContainerImage(const string &imageName) {
    containerImageName = imageName;
}

void DDSScheduler::setFutureWorkDirName(const string &workDirName) {
    workDirectoryName = workDirName;
}

// Synchronised
void DDSScheduler::addAgents(const DDSSubmitInfo& submit, const Resources& resourcesPerAgent) {
    lock_guard<std::mutex> lock(ddsMutex);
    BOOST_LOG_TRIVIAL(trace) << "Adding Agents from DDS-Submit" << endl;

    // Docker Info
    ContainerInfo container;
    {
        container.set_type(ContainerInfo::DOCKER);

        ContainerInfo::DockerInfo dockerInfo;
        dockerInfo.set_image(containerImageName);
        container.mutable_docker()->CopyFrom(dockerInfo);
    }

    CommandInfo_URI cUri;
    cUri.set_cache(false);
    cUri.set_extract(false);
    cUri.set_executable(true);
    cUri.set_value(submit.m_wrkPackageUri);

    // Either execute command or executor
    CommandInfo commandInfo;
    commandInfo.add_uris()->MergeFrom(cUri);
    {
        ostringstream ostr;

        // Copy worker package in a temporary directory
        ostr
            << " cd $MESOS_SANDBOX && pwd"
            << " && mv $(a=" << submit.m_wrkPackageUri << ";a=${a##*/};echo $a) " << submit.m_wrkPackageName
            << " && mkdir " << workDirectoryName
            << " && cd " << workDirectoryName
            << " && mv ../" << submit.m_wrkPackageName << " ."
            << " && ./" << submit.m_wrkPackageName;

        // Set command
        commandInfo.set_value(ostr.str());
    }

    for (size_t i = 1; i <= submit.m_nInstances; ++i) {
        TaskInfo taskInfo;

        taskInfo.set_name("DDS Framework Task");
        taskInfo.mutable_task_id()->set_value(string("dds-") + submit.m_id + "-r-" + to_string(submit.m_restId) + "-t-" + to_string(i));
        taskInfo.mutable_resources()->MergeFrom(resourcesPerAgent);
        if (!containerImageName.empty()) {
            taskInfo.mutable_container()->MergeFrom(container);
        }
        taskInfo.mutable_command()->MergeFrom(commandInfo);

        waitingTasks.push_back(taskInfo);
    }
}