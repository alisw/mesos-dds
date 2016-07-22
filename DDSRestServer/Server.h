//
// Created by kevin on 6/24/16.
//

#ifndef DDS_SUBMIT_MESOS_SERVER_H
#define DDS_SUBMIT_MESOS_SERVER_H

// System Includes
#include <cstdint>
#include <map>
#include <thread>

// Boost Includes
#include <boost/filesystem.hpp>

// CppRestSDK (Casablanca)
#include <cpprest/http_listener.h>

// Other Incudes
#include "Structures.h"
#include "DDSScheduler.h"

namespace DDSMesos {
    class Server {
    public:

        // Types
        using SubType = std::map<size_t, DDSSubmitInfo>;

        Server(DDSScheduler& ddsScheduler, const std::string& host);
        ~Server();

        void run();
        void setMesosHandler(void p_handler(const DDSSubmitInfo& submitInfo));

    private:
        // Exploits
        size_t getNextId();
        DDSSubmitInfo& getNextIdAndCommitSubmission();
        bool removeSubmission(size_t id);

        DDSScheduler& ddsScheduler;
        web::http::experimental::listener::http_listener statusListener;
        web::http::experimental::listener::http_listener ddsSubmitListener;
        web::http::experimental::listener::http_listener wrkPackageListener;
        std::recursive_mutex mtx;
        SubType submissions;
        void (*handler)(const DDSSubmitInfo& submitInfo);

    };
}


#endif //DDS_SUBMIT_MESOS_SERVER_H
