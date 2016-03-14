#include <iostream>
#include <algorithm>
#include <thread>
#include "DDSScheduler.h"

using namespace std;
using namespace mesos;


DDSScheduler::DDSScheduler (const ExecutorInfo& executorInfo, 
                          const Resources& resourcesPerTask, 
                          size_t numTasks) 
    :   executorInfo (executorInfo), 
        resourcesPerTask (resourcesPerTask), 
        numTasks (numTasks)
{
    
}

DDSScheduler::~DDSScheduler() {
    cout << "DDS Framework: Scheduler Destructor" << endl;
}

void DDSScheduler::registered (
  SchedulerDriver* driver,
    const FrameworkID& frameworkId,
    const MasterInfo& masterInfo
)
{
    cout    << "My Framework: Registered with Master ID: " 
            << masterInfo.id()
            << " and Framework "
            << frameworkId.SerializeAsString()
            << endl;
    
}

void DDSScheduler::reregistered (
  SchedulerDriver* driver,
  const MasterInfo& masterInfo
)
{
    cout    << "My Framework: Re-Registered" << endl;
}

void DDSScheduler::disconnected (SchedulerDriver* driver) {
    cout    << "My Framework: Disconnected" << endl;
}

void DDSScheduler::resourceOffers (
  SchedulerDriver* driver, 
  const vector<Offer>& offers
)
{
    cout << "My Framework: Resource Offers" << endl;
    
    // Call stop here as calling it from statusUpdate is problematic
    
}

void DDSScheduler::offerRescinded ( 
  SchedulerDriver* driver,
        const OfferID& offerId
)
{
    cout    << "My Framework: Offer Rescinded" << endl;
}

void DDSScheduler::statusUpdate (
  SchedulerDriver* driver,
  const TaskStatus& status
) 
{
    
    
}

void DDSScheduler::frameworkMessage (
  SchedulerDriver* driver,
  const ExecutorID& executorId,
  const SlaveID& slaveId,
  const string& data
)
{
    
}

void DDSScheduler::slaveLost (
  SchedulerDriver* driver,
  const SlaveID& slaveId
)
{
    // Do we need to reschedule tasks?
    
    
}

void DDSScheduler::executorLost (SchedulerDriver* driver,
  const ExecutorID& executorId,
  const SlaveID& slaveId,
  int status
)
{
    cout    << "My Framework: Executor Lost" << endl;
}

void DDSScheduler::error (SchedulerDriver* driver, const string& message) {
    cout    << "My Framework: Error" << endl;
}