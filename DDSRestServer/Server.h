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

// Other Incudes
#include "Structures.h"
#include "DDSScheduler.h"

namespace DDSMesos {
    class Server {
    public:

        // Types
        using SubType = std::map<size_t, DDSSubmitInfo>;

        Server(DDSScheduler& ddsScheduler);
        ~Server();

        bool start();
        void setMesosHandler(void p_handler(const DDSSubmitInfo& submitInfo));

    private:
        // Exploits
        size_t getNextId();
        size_t getNextIdAndCommitSubmission();
        void run();

        DDSScheduler& ddsScheduler;
        std::thread t;
        std::recursive_mutex mtx;
        SubType submissions;
        void (*handler)(const DDSSubmitInfo& submitInfo);

    };
}




#endif //DDS_SUBMIT_MESOS_SERVER_H
