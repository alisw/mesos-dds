//
// Created by kevin on 7/5/16.
//

#ifndef DDS_SUBMIT_MESOS_CONSTANTS_H
#define DDS_SUBMIT_MESOS_CONSTANTS_H

#include <string>

namespace DDSMesos {
    namespace Common {
        namespace Constants {

            namespace DDSConfInfo {
                extern std::string DDSSubmissionId;
                extern std::string Resources;
                extern std::string NumAgents;
                extern std::string CpusPerTask;
                extern std::string MemorySizePerTask;
                extern std::string Docker;
                extern std::string ImageName;
                extern std::string TemporaryDirectoryName;
                extern std::string WorkerPackageName;
                extern std::string WorkerPackageData;
            }

            namespace DDSConfInfoResponse {
                extern std::string Id;
            }

            namespace Status {
                extern std::string Status;
                extern std::string NumSubmissions;
            }

        };
    }
}



#endif //DDS_SUBMIT_MESOS_CONSTANTS_H
