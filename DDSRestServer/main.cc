//
// Created by kevin on 6/23/16.
//

// System Includes
#include <getopt.h>

// Boost Includes
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/filesystem.hpp>

// Our includes
#include "Utils.h"
#include "DDSScheduler.h"

using namespace std;
using namespace mesos;
using namespace DDSMesos::Common;

// Use unnamed namespaces in C++ to prevent exporting the symbols
namespace {
    string master = "localhost:5050";


    bool processArguments(int argc, char **argv) {
        // Options
        struct option options[] = {
                {"master", required_argument, nullptr, 'm'},
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
                case 'h':
                    help = 1;
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
            << "\t--help (Shows this Usage Information)" << endl;
            return true;
        }

        return false;
    }
}

int main(int argc, char **argv) {

    // Process arguments
    if (processArguments(argc, argv)) {
        return EXIT_SUCCESS;
    }

    // Setup Logging
    Utils::setupLogging("dds-mesos-server.log");

    // Describe my mesos framework
    FrameworkInfo frameworkInfo;
    frameworkInfo.set_user("root");
    frameworkInfo.set_name("Mesos DDS Framework");
    frameworkInfo.set_principal("ddsframework");

    // Setup Mesos
    unique_ptr<DDSScheduler> ddsScheduler (new DDSScheduler());
    unique_ptr<MesosSchedulerDriver> mesosSchedulerDriver (new MesosSchedulerDriver(ddsScheduler.get(), frameworkInfo, master));

    // Start Mesos without blocking this thread
    Status status = mesosSchedulerDriver->start();


    // Wait for mesos
    mesosSchedulerDriver->join();

    return EXIT_SUCCESS;
}