#include <getopt.h>
#include <signal.h>
#include <atomic>
#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <thread>

//#include "boost/foreach.hpp"

//#define foreach BOOST_FOREACH

//#include <mesos/resources.hpp>


//#include <boost/property_tree/json_parser.hpp>
//#include <boost/signals2/signal.hpp>
#include "dds_intercom.h"
#include "DDSScheduler.h"

using namespace std;
using namespace mesos;
using namespace dds::intercom_api;

// Use unnamed namespaces in C++ instead
// of static specifier as used in C
namespace {
    //volatile sig_atomic_t interrupted = false;
    atomic<bool> interrupted(false);
    MesosSchedulerDriver *msd;

    void SigIntHandler(int signum) {
        if (interrupted) {
            return;
        }
        interrupted = true;
        msd->stop();
    }

    void registerSigInt(void sigIntHandler(int)) {
        struct sigaction sa{}; // zero-initialize
        sa.sa_handler = sigIntHandler;
        sigfillset(&sa.sa_mask);
        sigaction(SIGINT, &sa, nullptr);
    }

    const char *const defaultMaster = "192.168.134.134:5050";
    const char *const defaultExecutorUri = "http://kevinnapoli.com/mesos/mesosmyexecutor";
    const int defaultNumTasks = 1;
    const int defaultCpusPerTask = 1;
    const int defaultMemSizePerTask = 1024;
}

// Opens a file, writes a value, and closes the file
// Keep in mind that if writing multiple values, opening
// and closing the file is very inefficient. One must 
// keep the ofstream instance open, write all data,
// and then close the file
template<class V>
bool writeToFile(const string &path, const V &value, bool append = false) {
    // To write, we can use the Output File Stream Object
    ofstream file;

    // Open File either in append mode or in truncate mode
    if (append) {
        file.open(path, ios_base::app);
    } else {
        file.open(path);
    }

    // open failed
    if (file.fail()) {
        return false;
    }

    // Write value
    file << value;

    // Write failed
    if (file.fail()) {
        return false;
    }

    // Close file (optional, since ofstream destructor will close file, but still good practice)
    file.close();

    return true;
}

const string logFilePath("/home/kevin/mesos-dds.txt");

void logToFile(std::string str) {
    str += '\n';
    writeToFile(logFilePath, str, true);
}

int main(int argc, char **argv) {

    writeToFile(logFilePath, string(" "));

    ostringstream ostr;
    ostr << "Welcome to dds-submit-mesos" << endl
         << "Main PID is: " << ::getpid() << endl
         << "Main Thread ID is: " << std::this_thread::get_id() << endl;
    logToFile(ostr.str());

    string master = defaultMaster;
    string executorUri = defaultExecutorUri;
    int numTasks = defaultNumTasks;
    int numCpuPerTask = defaultCpusPerTask;
    int memSizePerTask = defaultMemSizePerTask;

    // Describe My Framework
    FrameworkInfo frameworkInfo;
    frameworkInfo.set_user("root");
    frameworkInfo.set_name("Mesos DDS Framework");
    frameworkInfo.set_principal("ddsframework");

    // Describe Executor
    ExecutorInfo execInfo;
    {
        CommandInfo cmdInfo;
        cmdInfo.set_value("./mesos-dds-executor");

        execInfo.mutable_executor_id()->set_value("mesos-dds-executor");
        execInfo.mutable_command()->MergeFrom(cmdInfo);
        execInfo.set_name("DDS Executor");
    }

    // Describe Resources per Task
    Resources resourcesPerTask = Resources::parse(
            "cpus:" + to_string(numCpuPerTask) +
            ";mem:" + to_string(memSizePerTask)
    ).get();

    // Docker Info
    ContainerInfo container;
    container.set_type(ContainerInfo::DOCKER);
    Volume * volume = container.add_volumes();
    volume->set_host_path("/home/kevin/.DDS");
    volume->set_container_path("/home/kevin/.DDS");
    volume->set_mode(Volume_Mode::Volume_Mode_RW);
    {
        ContainerInfo::DockerInfo dockerInfo;
        dockerInfo.set_image("ubuntu:14.04");
        container.mutable_docker()->CopyFrom(dockerInfo);
    }

    // Instantiate our custom scheduler containing the information
    // about the executor who is supposed to execute the task

    mutex mt;
    unique_lock <std::mutex> uniqueLock(mt);
    condition_variable mesosStarted;

    DDSScheduler ddsScheduler(mesosStarted, execInfo, resourcesPerTask, container);

    MesosSchedulerDriver msd(&ddsScheduler, frameworkInfo, master);
    ::msd = &msd;
    registerSigInt(SigIntHandler);

    // Block here and make our scheduler available

    logToFile("Starting Mesos on a seperate thread");
    Status status = msd.start();

    // Wait for mesos thread to signal to us that it started
    mesosStarted.wait(uniqueLock);

    logToFile("Mesos Started! Now running DDS");


    logToFile("Mesos DDS Plugin - Running\n");

    CRMSPluginProtocol protocol("mesos");

    try {

        // This must be the first call to let DDS commander know that we are online.
        protocol.sendInit();

        protocol.onSubmit([&protocol, &ddsScheduler](const SSubmit &submit) {
            // Implement submit related functionality here.
            // After submit has completed call stop() function.

            ostringstream ostr;

            ostr << "DDS-Intercom onSubmit..: " << endl
                 << "\tonSubmit PID is: " << getpid() << endl
                 << "\tonSubmit Thread ID is: " << std::this_thread::get_id() << endl
                 << "\tm_nInstances: " << submit.m_nInstances << endl
                 << "\tm_cfgFilePath: " << submit.m_cfgFilePath << endl
                 << "\tm_id: " << submit.m_id << endl
                 << "\tm_wrkPackagePath: " << submit.m_wrkPackagePath << endl;
            logToFile(ostr.str());

            // Inform Mesos to deploy n agents

            DDSSubmitInfo ddsSubmitInfo;
            ddsSubmitInfo.m_cfgFilePath = submit.m_cfgFilePath;
            ddsSubmitInfo.m_id = submit.m_id;
            ddsSubmitInfo.m_nInstances = submit.m_nInstances;
            ddsSubmitInfo.m_wrkPackagePath = submit.m_wrkPackagePath;

            ddsScheduler.addAgents(ddsSubmitInfo);

            // Call to stop waiting
            protocol.stop();
        });

        protocol.onMessage([](const SMessage &_message) {
            // Message from commander received.
            // Implement related functionality here.
            logToFile("DDS-Intercom onMessage: ");
        });

        protocol.onRequirement([](const SRequirement &_requirement) {
            // Implement functionality related to requirements here.
            logToFile("DDS-Intercom onRequirement: ");
        });

        // Stop here and wait for notifications from commander.
        protocol.wait();

    } catch (const exception &e) {
        // Report error to DDS commander

        logToFile(string("DDS-Intercom Exception: ") + e.what());

        protocol.sendMessage(dds::intercom_api::EMsgSeverity::error, e.what());

    }

    //msd.stop();
    msd.join();

    logToFile("Exiting Mesos DDS");

    return 0;
}