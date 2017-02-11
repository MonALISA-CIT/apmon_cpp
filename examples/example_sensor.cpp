/**
 * \file example_sensor.cpp
 * This example shows how ApMon can be used for collecting system monitoring
 * information. The program acts like a simple sensor, which only sends 
 * system monitoring datagrams in the background thread. The time interval at
 * which the datagrams are sent can be set from the destinations_s.conf file.
 */ 
#include <stdlib.h> 
#include <time.h>

#include "ApMon.h"

#include "utils.h"
using namespace apmon_utils;

int main(int argc, char **argv) {
  char *filename = (char *)"destinations_s.conf";  

  try {
    ApMon *apm = new ApMon(filename);

    apm -> setRecheckInterval(300);
    // this way we can change the logging level
    apm -> setLogLevel((char *)"FINE");
    while (true) {
      sleep(1);
      
    }
  } catch(runtime_error &e) {
    logger(WARNING, e.what());
  }
  
  return 0;
}
