//
// Created by kevin on 6/24/16.
//

// System Includes
#include <memory>
#include <iostream>
#include <stdexcept>

// Boost includes
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>

// CppRestSDK (Casablanca)
#include <cpprest/json.h>

// Other Includes
#include "Server.h"
#include "Utils.h"
#include "Constants.h"

using namespace std;
using namespace DDSMesos;
using namespace DDSMesos::Common;
//using namespace mesos;

Server::Server(DDSScheduler &ddsScheduler, const string& host)
    : ddsScheduler (ddsScheduler),
      statusListener (string("http://") + host + "/status"),
      ddsSubmitListener (string("http://") + host + "/dds-submit"),
      wrkPackageListener (string("http://") + host + "/dds-work-package")
{ }

Server::~Server() {}

void Server::run() {

    // Casablanca Namespace
    using namespace web;
    using namespace web::http;
    using namespace web::http::experimental::listener;

    // Status Listener
    statusListener.support(methods::GET, [this](http_request request) -> void {
        using namespace DDSMesos::Common::Constants::Status;
        json::value status;
        status[Status][NumSubmissions] = json::value::number(submissions.size());
        request.reply(status_codes::OK, status);
    });
    statusListener.open();

    // DDS Work Package Listener
    wrkPackageListener.support(methods::GET, [this](http_request request) -> void {
        using namespace DDSMesos::Common::Constants::Status;
        //cout << "path: " << request.request_uri().path() << " resource Path: " << request.request_uri().resource().path() << endl;
        auto vars = uri::split_query(request.request_uri().query());
        auto id = vars.find("id");
        if (id == end(vars)) {
            BOOST_LOG_TRIVIAL(trace) << "No id" << endl;
            request.reply(status_codes::BadRequest, "No id");
            return;
        }
        size_t requestedId = atoi(id->second.c_str());
        string workerPackagePath;
        string workerPackageData;
        boost::filesystem::path wrkPath;
        try {
            using namespace boost::filesystem;
            {
                lock_guard<recursive_mutex> lock(mtx);
                const DDSSubmitInfo &submission = submissions.at(requestedId);
                wrkPath = submission.m_wrkPackagePath;
                wrkPath = path(to_string(requestedId)) / path(wrkPath.filename().string());
                wrkPath = complete(wrkPath);
                workerPackageData = Utils::readFromFile(wrkPath.string());
            }
            http_response response (status_codes::OK);
            response.set_body(workerPackageData);
            response.headers().add("Content-Disposition", "attachment; filename=\"" + wrkPath.filename().string() + "\"");
            cout << "Serving request for id: " << requestedId << endl;
            request.reply(response);
        } catch (const exception& ex) {
            request.reply(status_codes::NotFound, string("Not found or other error: ") + ex.what());
            BOOST_LOG_TRIVIAL(error) << ex.what() << endl;
        }
    });
    wrkPackageListener.open();

    // DDS Submit Listener
    ddsSubmitListener.support(methods::POST, [this](http_request request) -> void {
        // Get data from request
        // Request reference is invalid, move request to lambda function instead
        // Reason being that lambda is executed by other thread, and request in this scope expires
        request.extract_json().then([this, request](pplx::task<json::value> taskValue) -> void {
            size_t id = 0;
            try {
                using namespace boost::filesystem;
                using namespace DDSMesos::Common::Constants::DDSConfInfo;
                using namespace DDSMesos::Common::Constants::DDSConfInfoResponse;

                const json::value& jsonValue = taskValue.get();

                const json::object& ddsConfInf = jsonValue.as_object();
                const json::object& dockerContainer = ddsConfInf.at(Docker).as_object();
                const json::object& resources = ddsConfInf.at(Resources).as_object();

                // DDS Submit Info
                DDSSubmitInfo& ddsSubmitInfo = getNextIdAndCommitSubmission();
                id = ddsSubmitInfo.m_restId;
                ddsSubmitInfo.m_id = ddsConfInf.at(DDSSubmissionId).as_string();
                ddsSubmitInfo.m_nInstances = resources.at(NumAgents).as_number().to_uint32();

                // Describe Resources per Task
                mesos::Resources resourcesPerAgent = mesos::Resources::parse(
                        "cpus:" + resources.at(CpusPerTask).as_string() +
                        ";mem:" + resources.at(MemorySizePerTask).as_string()
                ).get();

                // Create Name for worker package
                path wrkPackageName (ddsConfInf.at(WorkerPackageName).as_string());

                // Create directory for ID
                path dir (to_string(id));
                if (exists(dir)) {
                    remove_all(dir);
                }
                if (!create_directory(dir)) {
                    throw runtime_error("Cannot create directory: " + dir.filename().string());
                }

                // Create Path for Worker Package
                path wrkPackagePath ( dir / wrkPackageName);
                wrkPackagePath = complete(wrkPackagePath);

                ddsSubmitInfo.m_wrkPackagePath = wrkPackagePath.string();
                ddsSubmitInfo.m_wrkPackageName = wrkPackageName.filename().string();
                ddsSubmitInfo.m_wrkPackageUri  = wrkPackageListener.uri().to_string() + "?id=" + to_string(id);

                // Decode data and put in File
                Utils::writeToFile(ddsSubmitInfo.m_wrkPackagePath, Utils::decode64(ddsConfInf.at(WorkerPackageData).as_string()));

                // We have all the required info, submit to Mesos!
                ddsScheduler.setFutureTaskContainerImage(dockerContainer.at(ImageName).as_string());
                ddsScheduler.setFutureWorkDirName(dockerContainer.at(TemporaryDirectoryName).as_string());
                ddsScheduler.addAgents(ddsSubmitInfo, resourcesPerAgent);

                // Reply
                json::value responseValue;
                responseValue[Id] = json::value::number(id);
                request.reply(status_codes::OK, responseValue);
            } catch (const exception& ex) {
                if (id > 0) {
                    // Revoke
                    removeSubmission(id);
                }
                request.reply(status_codes::BadRequest, ex.what());
                BOOST_LOG_TRIVIAL(error) << ex.what() << endl;
            }
        });
    });
    ddsSubmitListener.open();
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

DDSSubmitInfo& Server::getNextIdAndCommitSubmission() {
    lock_guard<recursive_mutex> lock(mtx);
    size_t id = getNextId();
    submissions[id].m_restId = id;
    return submissions[id];
}

bool Server::removeSubmission(size_t id) {
    lock_guard<recursive_mutex> lock(mtx);
    return submissions.erase(id) == 1;
}
