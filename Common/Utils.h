//
// Created by kevin on 6/23/16.
//

#ifndef DDS_SUBMIT_MESOS_UTILS_H
#define DDS_SUBMIT_MESOS_UTILS_H

// System includes
#include <string>

namespace DDSMesos {
    namespace Common {
        namespace Utils {
            std::string getHome();
            void setupLogging(const std::string& logFileName);
        };
    }
}




#endif //DDS_SUBMIT_MESOS_UTILS_H
