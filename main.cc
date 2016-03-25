#include <getopt.h>
#include <signal.h>
#include <atomic>
#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <thread>

/**/

//#include <boost/property_tree/json_parser.hpp>
//#include <boost/signals2/signal.hpp>
// Must keep this order when including these header files
#include "dds_intercom.h"
#include "DDSScheduler.h"

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/file.hpp>

using namespace std;
using namespace mesos;
using namespace dds::intercom_api;

namespace logging = boost::log;
namespace keywords = boost::log::keywords;

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
        BOOST_LOG_TRIVIAL(trace) << "Mesos DDS - Received Interrupt Signal" << endl;
        msd->stop();
    }

    void registerSigInt(void sigIntHandler(int)) {
        struct sigaction sa{}; // zero-initialize
        sa.sa_handler = sigIntHandler;
        sigfillset(&sa.sa_mask);
        sigaction(SIGINT, &sa, nullptr);
    }

    const char* const defaultMaster = "192.168.134.137:5050";
    const char* const defaultLogFilePath = "/home/kevin/mesos-dds.txt";
    const char* const defaultDockerAgentImage = "ubuntu:14.04";
    const char* const defaultTempDirInContainer = "DDSEnvironment";
    const int defaultCpusPerTask = 1;
    const int defaultMemSizePerTask = 1024;
}

int main(int argc, char **argv) {

    // Set or use defaults
    string master = defaultMaster;
    string logFilePath = defaultLogFilePath;
    string dockerAgentImage = defaultDockerAgentImage;
    string tempDirInContainer = defaultTempDirInContainer;
    int numCpuPerTask = defaultCpusPerTask;
    int memSizePerTask = defaultMemSizePerTask;

    // Setup Logging
    if (logFilePath.length() > 0) {
        logging::add_file_log(keywords::file_name = logFilePath,
                              keywords::auto_flush = true,
                              keywords::format = "[%TimeStamp%]: %Message%");
    }

    // Proceed
    BOOST_LOG_TRIVIAL(trace)
        << "Welcome to dds-submit-mesos" << endl
        << "Main PID is: " << ::getpid() << endl
        << "Main Thread ID is: " << std::this_thread::get_id() << endl
        << "Argument Count: " << argc << endl;

    for (int i = 0; i < argc; ++i) {
        BOOST_LOG_TRIVIAL(trace) << i << ") " << argv[i] << endl;
    }

    // Describe My Framework
    FrameworkInfo frameworkInfo;
    frameworkInfo.set_user("root");
    frameworkInfo.set_name("Mesos DDS Framework");
    frameworkInfo.set_principal("ddsframework");

    // Describe Resources per Task
    Resources resourcesPerTask = Resources::parse(
            "cpus:" + to_string(numCpuPerTask) +
            ";mem:" + to_string(memSizePerTask)
    ).get();

    // Define condition variable
    mutex mt;
    unique_lock <std::mutex> uniqueLock(mt);
    condition_variable mesosStarted;

    DDSScheduler ddsScheduler(mesosStarted, resourcesPerTask);
    ddsScheduler.setFutureTaskContainerImage(dockerAgentImage);
    ddsScheduler.setFutureWorkDirName(tempDirInContainer);

    MesosSchedulerDriver msd(&ddsScheduler, frameworkInfo, master);
    ::msd = &msd;
    registerSigInt(SigIntHandler);

    BOOST_LOG_TRIVIAL(trace) << "Starting Mesos on a seperate thread" << endl;
    Status status = msd.start();

    // Wait for mesos thread to signal to us that it started
    BOOST_LOG_TRIVIAL(trace) << "Waiting for Mesos Signal.." << endl;
    mesosStarted.wait(uniqueLock);

    BOOST_LOG_TRIVIAL(trace) << "Mesos DDS Plugin - Running" << endl;

    CRMSPluginProtocol protocol("mesos");

    try {
        // This must be the first call to let DDS commander know that we are online.
        protocol.sendInit();

        protocol.onSubmit([&protocol, &ddsScheduler](const SSubmit &submit) {
            // Implement submit related functionality here.
            // After submit has completed call stop() function.

            BOOST_LOG_TRIVIAL(trace)
                 << "DDS-Intercom onSubmit..: " << endl
                 << "\tonSubmit PID is: " << getpid() << endl
                 << "\tonSubmit Thread ID is: " << std::this_thread::get_id() << endl
                 << "\tm_nInstances: " << submit.m_nInstances << endl
                 << "\tm_cfgFilePath: " << submit.m_cfgFilePath << endl
                 << "\tm_id: " << submit.m_id << endl
                 << "\tm_wrkPackagePath: " << submit.m_wrkPackagePath << endl;

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
            BOOST_LOG_TRIVIAL(trace) << "DDS-Intercom onMessage:" << endl;
        });

        protocol.onRequirement([](const SRequirement &_requirement) {
            // Implement functionality related to requirements here.
            BOOST_LOG_TRIVIAL(trace) << "DDS-Intercom onRequirement:" << endl;
        });

        // Stop here and wait for notifications from commander.
        protocol.wait();

    } catch (const exception &e) {
        BOOST_LOG_TRIVIAL(error) << "DDS-Intercom Exception: " << e.what() << endl;
        // Report error to DDS commander
        protocol.sendMessage(dds::intercom_api::EMsgSeverity::error, e.what());
    }

    //msd.stop();
    msd.join();

    BOOST_LOG_TRIVIAL(trace) << "Exiting Mesos DDS" << endl;

    return 0;
}