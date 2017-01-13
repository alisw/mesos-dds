//
// Created by kevin on 6/23/16.
//

// System Includes
#include <getopt.h>
#include <thread>

// Boost Includes
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/filesystem.hpp>
#include <stdlib.h>

// Our includes
#include "Utils.h"
#include "DDSScheduler.h"
#include "Server.h"

using namespace std;
using namespace DDSMesos::Common;


// Use unnamed namespaces in C++ to prevent exporting the symbols
namespace {
    string master = "localhost:5050";
    string restHost = "localhost:1234";
    bool useRevocableResources = false;

    bool processArguments(int argc, char **argv) {
        // Options
        struct option options[] = {
                {"master", required_argument, nullptr, 'm'},
                {"resthost", required_argument, nullptr, 'r'},
                {"use-revocable-resources",   no_argument,       nullptr, 'R'},
                {"help",   no_argument,       nullptr, 'h'},
                {nullptr, 0,                  nullptr, 0} // Last entry must be all zeros
        };

        int c;
        int opIndex;
        bool help = false;
        while ((c = getopt_long_only(argc, argv, "", options, &opIndex)) != -1) {
            switch (c) {
                case 'm':
                    master = optarg;
                    break;
                case 'r':
                    restHost = optarg;
                    break;
                case 'R':
                    useRevocableResources = true;
                    break;
                case 'h':
                    help = true;
                    break;
                case ':':
                case '?':
                    help = true;
                    cout << "Unknown option or missing argument: " << (char) c << " : " << optarg << endl;
                default:
                    help = true;
                    cout << "Unknown Error while Parsing Arguments- " << (char) c << " : " << optarg << endl;
            }
        }

        if (help || master.length() == 0) {
            cout << "Usage:" << endl
            << "\t--master=[ip:port], current: " << master << endl
            << "\t--resthost=[ip:port], current: " << restHost << endl
            << "\t[--use-revocable-resources]" << endl
            << "\t--help (Shows this Usage Information)" << endl;
            return true;
        }

        return false;
    }
}

int main(int argc, char **argv) {
    using namespace mesos;

    // Process arguments
    if (processArguments(argc, argv)) {
        return 0;
    }

    try {
        // Setup Logging
        Utils::setupLogging("dds-mesos-server.log");

        // Describe my mesos framework
        FrameworkInfo frameworkInfo;
        frameworkInfo.set_user("root");
        frameworkInfo.set_name("Mesos DDS Framework");
        frameworkInfo.set_principal("ddsframework");
        if (useRevocableResources)
          frameworkInfo.add_capabilities()->set_type(
            FrameworkInfo::Capability::REVOCABLE_RESOURCES);

        // Setup Mesos
        unique_ptr<DDSScheduler> ddsScheduler (new DDSScheduler());
        unique_ptr<MesosSchedulerDriver> mesosSchedulerDriver (new MesosSchedulerDriver(ddsScheduler.get(), frameworkInfo, master));

        // Start Mesos without blocking this thread
        Status status = mesosSchedulerDriver->start();

        // Start REST service
        DDSMesos::Server srv (*ddsScheduler, restHost);
        srv.run();

        // Wait for mesos
        mesosSchedulerDriver->join();
    } catch (const exception& ex) {
        BOOST_LOG_TRIVIAL(error) << "Exception: " << ex.what() << endl;
    }

    return 0;
}
