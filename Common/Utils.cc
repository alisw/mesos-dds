//
// Created by kevin on 6/23/16.
//

// Required for getHome
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <stdexcept>

// Boost Includes
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/filesystem.hpp>

// Our includes
#include "Utils.h"

using namespace std;
using namespace DDSMesos::Common;

string Utils::getHome() {
    const char* ptHome = getenv("HOME");
    if (ptHome == nullptr || *ptHome == '\0') {
        // Home environment variable is unset
        const struct passwd *pw = getpwuid(getuid());
        if (pw == nullptr || (ptHome = pw->pw_dir) == nullptr) {
            throw runtime_error("Cannot retrieve Home directory");
        }
    }
    return ptHome;
}

void Utils::setupLogging(const string& logFileName) {
    namespace logging = boost::log;
    namespace keywords = boost::log::keywords;
    string logFilePath;
    try {
        boost::filesystem::path homePath(getHome());
        boost::filesystem::path logFile(logFileName);
        logFilePath = (homePath / logFile).string();
    } catch (const exception *ex) {
        logFilePath = "/tmp/" + logFileName;
    }
    logging::add_file_log(keywords::file_name = logFilePath,
                          keywords::auto_flush = true,
                          keywords::format = "[%TimeStamp%]: %Message%");

}


