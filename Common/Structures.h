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
    std::string m_wrkPackagePath;   ///< A full path of the agent worker package, which needs to be deployed.
    std::string m_WorkerPackageBase64; /// The binary data for the package in base64
};

#endif //DDS_SUBMIT_MESOS_STRUCTURES_H
