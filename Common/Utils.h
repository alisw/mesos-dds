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
            std::string decode64(const std::string &val);
            std::string encode64(const std::string &val);
            std::string readFromFile(const std::string& fileName);
            void writeToFile(const std::string& fileName, const std::string& fileData);
        };
    }
}

#endif //DDS_SUBMIT_MESOS_UTILS_H
