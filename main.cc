#include <getopt.h>
#include <signal.h>
#include <atomic>
#include <cstdlib>
#include <iostream>

//#include "boost/foreach.hpp"

//#define foreach BOOST_FOREACH

//#include <mesos/resources.hpp>


//#include <boost/property_tree/json_parser.hpp>
//#include <boost/signals2/signal.hpp>
#include "dds_intercom.h"
#include "DDSScheduler.h"

using namespace std;
using namespace mesos;
using namespace dds::intercom_api;

// Use unnamed namespaces in C++ instead
// of static specifier as used in C
namespace
{
    //volatile sig_atomic_t interrupted = false;
    
}

int main (int argc, char ** argv) {

  cout << "Mesos DDS Plugin - Running!" << endl;

  CRMSPluginProtocol protocol("mesos");

  try {

    // This must be the first call to let DDS commander know that we are online.
    protocol.sendInit();

    protocol.onSubmit([&protocol](const SSubmit& _submit) {
       // Implement submit related functionality here.
       // After submit has completed call stop() function.

      cout << "DDS-Intercom onSubmit: " << endl;

      //protocol.stop();
    });

    protocol.onMessage([](const SMessage& _message){
      // Message from commander received.
      // Implement related functionality here.
      cout << "DDS-Intercom onMessage: " << endl;
    });

    protocol.onRequirement([](const SRequirement& _requirement) {
      // Implement functionality related to requirements here.
      cout << "DDS-Intercom onRequirement: " << endl; 
    });

    
    // Stop here and wait for notifications from commander.
    protocol.wait();

  } catch (const exception& e) {
    // Report error to DDS commander
    protocol.sendMessage(dds::intercom_api::EMsgSeverity::error, e.what());
  }

    return 0;
}