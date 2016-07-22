//
// Created by kevin on 6/23/16.
//

#ifndef DDS_SUBMIT_MESOS_STRUCTURES_H
#define DDS_SUBMIT_MESOS_STRUCTURES_H

#include <cstdint>
#include <string>

struct DDSSubmitInfo {
    uint32_t m_nInstances;          ///< Number of instances.
    std::string m_cfgFilePath;      ///< Path to the configuration file.
    std::string m_id;               ///< ID for communication with DDS commander.
    size_t m_restId;                ///< ID for communication with Rest Server.
    std::string m_wrkPackagePath;   ///< A full path of the agent worker package, which needs to be deployed.
    std::string m_wrkPackageName;   ///< The name of the worker package (DDSWorker.sh).
    std::string m_wrkPackageUri;    ///< The Uri of the worker package .
};

#endif //DDS_SUBMIT_MESOS_STRUCTURES_H
