//
// Created by kevin on 6/24/16.
//

// System Includes
#include <memory>
#include <iostream>
#include <stdexcept>

// Restbed Includes
#include <restbed>

// JsonBox Includes
#include "JsonBox.h"

// Other Includes
#include "Server.h"
#include "Utils.h"
#include "Constants.h"

using namespace std;
using namespace DDSMesos;
using namespace DDSMesos::Common;
//using namespace mesos;

Server::Server(DDSScheduler &ddsScheduler)
    : ddsScheduler (ddsScheduler)
{ }

Server::~Server() {
    if (t.joinable()) {
        t.join();
    }
}

bool Server::start() {
    if(t.joinable()) {
        return false;
    }
    // not joinable - no thread since we did not detach
    t = thread(&Server::run, this);
    return true;
}

void Server::run() {
    // Namespace
    using namespace restbed;

    // Construct resources
    shared_ptr<Resource> statusResource = make_shared<Resource>();
    statusResource->set_path("/status");
    statusResource->set_method_handler("GET", [this](const shared_ptr<Session> session) -> void {
        size_t numSubmissions = submissions.size();
        string body = "{\"status\":{\"NumSubmissions\":\"" + to_string(numSubmissions) + "\"}}";
        session->close(OK, body, {
                { "Connection", "close" },
                { "Content-Type", "application/json" },
                { "Content-Length", to_string(body.size()) }
        });
    });

    shared_ptr<Resource> ddsSubmitResource = make_shared<Resource>();
    ddsSubmitResource->set_path("/dds-submit");
    ddsSubmitResource->set_error_handler([](const int code, const exception& ex, const shared_ptr< Session > session) {
        cout << "Internal Error: " << ex.what() << endl;
    });
    ddsSubmitResource->set_method_handler("POST", [this](const shared_ptr<Session> session) -> void {
        // Get Request
        const shared_ptr<const Request> request = session->get_request();

        // Get Content Length from Header
        size_t content_length = 0;
        request->get_header("Content-Length", content_length);

        // Get Body Data
        string strBody;
        cout << "Content-length body size: " << content_length << endl;
        session->fetch(content_length, [&strBody]( const shared_ptr< Session > session, const restbed::Bytes & body ){
            cout << "Body Size: " << body.size() << endl;
            strBody = string(reinterpret_cast<const char*>(body.data()), body.size());
        });
        cout << "Received request" << endl;

        // Parse Json and get Object
        string sError;
        try
        {
            using namespace mesos;
            using namespace JsonBox;
            using namespace DDSMesos::Common::Constants::DDSConfInfo;
            using namespace boost::filesystem;

            JsonBox::Value value;
            value.loadFromString(strBody);

            const Object& ddsConfInf = value.getObject();
            const Object& dockerContainer = ddsConfInf.at(Docker).getObject();
            const Object& resources = ddsConfInf.at(Resources).getObject();

            // Describe Resources per Task
            mesos::Resources resourcesPerAgent = Resources::parse(
                    "cpus:" + resources.at(CpusPerTask).getString() +
                    ";mem:" + resources.at(MemorySizePerTask).getString()
            ).get();

            // Construct DDS Info object
            //
            DDSSubmitInfo ddsSubmitInfo;
            ddsSubmitInfo.m_id = ddsConfInf.at(DDSSubmissionId).getString();
            ddsSubmitInfo.m_nInstances = static_cast<uint32_t>(resources.at(NumAgents).getInteger());

            // Create Name for worker package
            path wrkPackageName (ddsConfInf.at(WorkerPackageName).getString());

            // Create directory for ID
            size_t id = getNextIdAndCommitSubmission();
            path dir (string("./") + to_string(id));
            if (create_directory(dir)) {
                throw runtime_error("Cannot create directory: " + dir.filename().string());
            }

            // Create Path for Worker Package
            path wrkPackagePath ( dir / wrkPackageName);

            ddsSubmitInfo.m_wrkPackagePath = wrkPackagePath.string();

            // Decode data and put in File
            Utils::writeToFile(wrkPackagePath.filename().string(), Utils::decode64(ddsConfInf.at(WorkerPackageData).getString()));

            // We have all the required info, submit to Mesos!
            ddsScheduler.setFutureTaskContainerImage(dockerContainer.at(ImageName).getString());
            ddsScheduler.setFutureWorkDirName(dockerContainer.at(TemporaryDirectoryName).getString());
            ddsScheduler.addAgents(ddsSubmitInfo, resourcesPerAgent);

            // Send back Id of submission
            string sbody = "{\"id\":" + to_string(id) + "\"\"}";
            session->close(OK, sbody, {
                    { "Connection", "close" },
                    { "Content-Type", "application/json" },
                    { "Content-Length", to_string(sbody.size()) }
            });
        } catch (const out_of_range& ex) {
            sError = "{\"error\":\"Exception: Malformed Request\"}";
        } catch (const exception& ex) {
            sError = "{\"error\":\"Exception: ";
            sError += ex.what();
            sError += "\"}";
        }

        if (sError.length() > 0) {
            cout << sError << endl;
            session->close(BAD_REQUEST, sError, {
                    {"Connection",     "close"},
                    {"Content-Type",   "application/json"},
                    {"Content-Length", to_string(sError.size())}
            });
        }

    });

    shared_ptr<Settings> settings = make_shared<Settings>();
    //settings->set_worker_limit(4);

    Service service;
    service.publish(statusResource);
    service.publish(ddsSubmitResource);
    service.start(settings);
}

void Server::setMesosHandler(void (*p_handler)(const DDSSubmitInfo &)) {
    handler = p_handler;
}

size_t Server::getNextId() {
    lock_guard<recursive_mutex> lock(mtx);
    if (submissions.size() == 0) {
        return 1;
    }
    const size_t firstId = submissions.begin()->first;
    if (firstId > 1) {
        return firstId - 1;
    } else {
        // find a hole
        size_t prevId = 0;
        for (const SubType::value_type& submissionPair : submissions) {
            if(prevId -= 0) {
                prevId = submissionPair.first;
                continue;
            }
            //
            if (submissionPair.first - prevId > 1) {
                return prevId + 1;
            }
            prevId = submissionPair.first;
        }
        // Didn't manage to find a hole, give next
        if (prevId < std::numeric_limits<std::size_t>::max()) {
            return prevId + 1;
        }
    }

    // We're full up
    return 0;
}

size_t Server::getNextIdAndCommitSubmission() {
    lock_guard<recursive_mutex> lock(mtx);
    size_t id = getNextId();
    submissions[id] = DDSSubmitInfo();
    return id;
}


