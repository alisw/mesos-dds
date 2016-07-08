// System includes
#include <cstdlib>
#include <unistd.h>
#include <thread>
#include <fstream>

#include <string>
#include <sstream>

#include <memory>

// Boost includes
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/filesystem.hpp>

// Restbed includes
#include <restbed>

// JsonBox Includes
#include "JsonBox.h"

//#include <boost/property_tree/json_parser.hpp>
//#include <boost/signals2/signal.hpp>
// Must keep this order when including these header files
#include "dds_intercom.h"
#include "Utils.h"
#include "Structures.h"
#include "Constants.h"

using namespace std;
using namespace dds::intercom_api;
using namespace DDSMesos::Common;

// Use unnamed namespaces in C++ instead
// of static specifier as used in C
namespace {
    const char* const defaultMaster = "localhost:5050";
    const char* const defaultRestHost = "localhost:80";
    const char* const defaultDockerAgentImage = "ubuntu:14.04";
    const char* const defaultTempDirInContainer = "DDSEnvironment";
    const int defaultCpusPerTask = 1;
    const int defaultMemSizePerTask = 1024;
}

int main(int argc, char **argv) {

    // Setup Logging
    Utils::setupLogging("mesos-dds.log");

    // Proceed
    BOOST_LOG_TRIVIAL(trace)
        << "Welcome to dds-submit-mesos" << endl
        << "Argument Count: " << argc << endl;
    for (int i = 0; i < argc; ++i) {
        BOOST_LOG_TRIVIAL(trace) << i << ") " << argv[i] << endl;
    }

    CRMSPluginProtocol protocol("mesos");

    try {
        
        protocol.onSubmit([&protocol](const SSubmit &submit) {
            // Implement submit related functionality here.
            // After submit has completed call stop() function.

            BOOST_LOG_TRIVIAL(trace)
                 << "DDS-Intercom onSubmit..: " << endl
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

            //
            boost::filesystem::path ddsWorkerPackagePath(ddsSubmitInfo.m_wrkPackagePath);

            // Parse config file
            const size_t numLines = 7;
            string conf[numLines];
            if (ddsSubmitInfo.m_cfgFilePath.length() > 0) {
                ifstream ifs(ddsSubmitInfo.m_cfgFilePath);
                for (size_t i = 0; i < numLines && getline(ifs, conf[i]); ++i) {}
            }

            string master = conf[0].length() ? conf[0] : defaultMaster;
            uint32_t numAgents = static_cast<uint32_t >(stoi(conf[1].length() ? conf[1] : string("1")));
            string dockerAgentImage = conf[2].length() ? conf[2] : defaultDockerAgentImage;
            string tempDirInContainer = conf[3].length() ? conf[3] : defaultTempDirInContainer;
            string cpusPerTask (conf[4].length() ? conf[4] : to_string(defaultCpusPerTask));
            string memSizePerTask (conf[5].length() ? conf[5] : to_string(defaultMemSizePerTask));
            string restHost = conf[6].length() ? conf[6] : defaultRestHost;

            BOOST_LOG_TRIVIAL(trace)
                << "Using these values:" << endl
                << "\tmaster: " << master << endl
                << "\tnumAgents: " << numAgents << endl
                << "\tdockerAgentImage: " << dockerAgentImage << endl
                << "\tempDirInContainer: " << tempDirInContainer << endl
                << "\tcpusPerTask: " << cpusPerTask << endl
                << "\tmemSizePerTask: " << memSizePerTask << endl
                << "\trestHost: " << restHost << endl;

            // Send Information through using REST endpoint
            {
                using namespace restbed;

                shared_ptr<Request> request = make_shared<Request>( Uri( string("http://") + restHost + "/dds-submit" ) );
                request->set_header( "Accept", "application/json" );
                request->set_header( "Content-Type", "application/json" );
                request->set_header( "Host", restHost );
                request->set_method("POST");
                string strBody;

                // Json
                {
                    using namespace JsonBox;
                    using namespace DDSMesos::Common::Constants::DDSConfInfo;

                    Object resources;
                    resources[NumAgents] = Value(to_string(numAgents));
                    resources[CpusPerTask] = Value(cpusPerTask);
                    resources[MemorySizePerTask] = Value(memSizePerTask);

                    Object dockerContainer;
                    dockerContainer[ImageName] = Value(dockerAgentImage);
                    dockerContainer[TemporaryDirectoryName] = Value(tempDirInContainer);

                    Object ddsConfInf;
                    ddsConfInf[DDSSubmissionId] = submit.m_id;
                    ddsConfInf[Resources] = resources;
                    ddsConfInf[Docker] = dockerContainer;
                    ddsConfInf[WorkerPackageName] = ddsWorkerPackagePath.filename().string();
                    ddsConfInf[WorkerPackageData] = (Utils::readFromFile("/home/kevin/inputData.txt"));

                    ostringstream body;
                    Value(ddsConfInf).writeToStream(body, false);
                    strBody = body.str();
                    Utils::writeToFile("/home/kevin/MYFILE.txt", strBody);
                    request->set_header("Content-Length", to_string(strBody.length()));
                    request->set_body(strBody);
                }

                shared_ptr<Response> response = Http::sync(request);

                // Get response and process
                if (response->get_status_code() == OK) {
                    size_t content_length = 0;
                    response->get_header("Content-Length", content_length);

                    // Fetch Data
                    Http::fetch(content_length, response);

                    // Get Body Data
                    string strBody = string(reinterpret_cast<const char *>(response->get_body().data()),
                                            response->get_body().size());
                    JsonBox::Value val;
                    val.loadFromString(strBody);
                    const JsonBox::Object &replyJson = val.getObject();
                    BOOST_LOG_TRIVIAL(trace) << "OK. REST Server Submison Id: " << replyJson.at("Id") << endl;
                } else {
                    size_t content_length = 0;
                    response->get_header("Content-Length", content_length);

                    // Fetch Data
                    Http::fetch(content_length, response);

                    // Get Body Data
                    string strBody = string(reinterpret_cast<const char *>(response->get_body().data()),
                                            response->get_body().size());
                    throw runtime_error("Request Failed - Did not submit: " + strBody);
                }
            }
            // Call to stop waiting
            protocol.stop();
        });

        protocol.onMessage([](const SMessage &_message) {
            // Message from commander received.
            // Implement related functionality here.
            BOOST_LOG_TRIVIAL(trace) << "DDS-Intercom onMessage:" << endl;
        });

        // Let DDS commander know that we are online and wait for notifications from commander
        protocol.start();

    } catch (const exception &e) {
        BOOST_LOG_TRIVIAL(error) << "Mesos DDS-Intercom Exception: " << e.what() << endl;
        // Report error to DDS commander
        protocol.sendMessage(dds::intercom_api::EMsgSeverity::error, e.what());
    }

    BOOST_LOG_TRIVIAL(trace) << "Ready - Exiting Mesos DDS" << endl;

    return EXIT_SUCCESS;
}