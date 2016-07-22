//
// Created by kevin on 6/23/16.
//

// Required for getHome
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <stdexcept>

// Required for I/O
#include <fstream>

// Boost Includes
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/filesystem.hpp>

#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/transform_width.hpp>
// #include <boost/iterator/transform_iterator.hpp>
// #include <boost/functional.hpp>
#include <boost/algorithm/string.hpp>

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

string Utils::decode64(const string &val) {
    using namespace boost::archive::iterators;
    using It = transform_width<binary_from_base64<string::const_iterator>, 8, 6>;

    if (val.size() == 0) {
        return string();
    }

    if (val.size() % 4 != 0) {
        throw runtime_error("Base64 string size should be a multiple of four");
    }

    // Count '='
    size_t paddingBytes = count(val.begin() + static_cast<size_t>(val.size() - 2), val.end(), '=');

    // TODO: check if replacing is necessary or if it is handled by the It iterator. If it is not, we don't need to do a copy
    // Replace '=' with 'A' -- Optional?
    string input = val;
    replace_if(input.begin() + static_cast<size_t>(input.size() - 2), input.end(), [] (char c) -> bool {
        return c == '=';
    }, 'A');

    /* auto fn = [] (const char& a) -> char {
        return a == '=' ? 'A' : a;
    };
    auto begin = boost::make_transform_iterator(val.begin(), fn);
    auto end = boost::make_transform_iterator(val.end(), fn); */
    string decodedString = string(It(input.begin()), It(input.end()));
    decodedString.erase(decodedString.length() - paddingBytes);
    return decodedString;
}

string Utils::encode64(const string &val) {
    using namespace boost::archive::iterators;
    using It = base64_from_binary<transform_width<string::const_iterator, 6, 8>>;
    string tmp (It(val.begin()), It(val.end()));
    return tmp.append((3 - val.size() % 3) % 3, '=');
}

string Utils::readFromFile(const string &fileName) {
    ifstream fStream (fileName, ios::binary);

    // Get the size
    fStream.seekg(0, ios::end);
    size_t size = fStream.tellg();

    // Contiguous storage is guaranteed in C++11
    string buffer (size, ' ');

    // Restore seek
    fStream.seekg(0);

    // write
    fStream.read(&buffer[0], size);

    return buffer;
}

void Utils::writeToFile(const std::string &fileName, const std::string& fileData) {
    ofstream fStream (fileName, ios::binary);
    fStream << fileData;
}