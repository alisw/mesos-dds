//
// Created by kevin on 6/24/16.
//

// System Includes
#include <memory>
#include <iostream>

// Restbed Includes
#include <restbed>

// JsonBox Includes
#include "JsonBox.h"

// Other Includes
#include "Server.h"

using namespace std;
using namespace DDSMesos;

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
        string body = "{\"status\":\"{\"NumSubmissions\":\"" + to_string(numSubmissions) + "\"}\"}";
        session->close(OK, body, {
                { "Connection", "close" },
                { "Content-Type", "application/json" },
                { "Content-Length", to_string(body.size()) }
        });
    });

    shared_ptr<Resource> ddsSubmitResource = make_shared<Resource>();
    ddsSubmitResource->set_path("/dds-submit");
    ddsSubmitResource->set_method_handler("POST", [this](const shared_ptr<Session> session) -> void {
        // Get Request
        const shared_ptr<const Request> request = session->get_request();

        // Get Content Length from Header
        size_t content_length = 0;
        request->get_header("Content-Length", content_length);

        // Get Body Data
        string strBody;
        session->fetch(content_length, [&strBody]( const shared_ptr< Session > session, const restbed::Bytes & body ){
            strBody = string(reinterpret_cast<const char*>(body.data()), body.size());
        });

        // Parse Json and get Object

        // Add submission directly to Mesos

        // Send back Id of submission
        string sbody = "{\"id\":\"\"}";
        session->close(OK, sbody, {
                { "Connection", "close" },
                { "Content-Type", "application/json" },
                { "Content-Length", to_string(sbody.size()) }
        });

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
    lock_guard<mutex> lock(mtx);
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
