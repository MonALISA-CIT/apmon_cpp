/**
 * \file example_4.cpp
 * This example shows how multiple ApMon objects can be used in the same 
 * program. However, this is not the suggested way to use ApMon; if possible, 
 * it is better to work with a single ApMon object because in this way the
 * network socket and other resources are re-used.
 * The number of times we instantiate an ApMon object can be specified in the 
 * command line (if it is not, the default is 20).
 * The file "destinations_1.conf" contains the addresses of the hosts to which
 * we send the parameters, and also the corresponding ports.
 */ 
#include <stdlib.h> 
#include <time.h>

#include "ApMon.h"

/* these are for using the ApMon logger */
#include "utils.h"
using namespace apmon_utils;

int main(int argc, char **argv) {
  char *initurl = (char *)"http://panther.rogrid.pub.ro/corina/destinations_x1.conf";
  //char *filename = "destinations_1.conf";  
  int nObjects = 20;
  char logmsg[100];
  double val = 0;
  int i;

  srand(time(NULL));

  if (argc ==2)
    nObjects = atoi(argv[1]);

  for (i = 0; i < nObjects; i++) {
    try {
      ApMon *apm = new ApMon(initurl);

      val = 10 * (double)rand() / RAND_MAX;
      apm -> sendParameter((char *)"TestCluster4_cpp", NULL, (char *)"my_cpu_load", 
			     val);
      sleep(1);	

      // cleanup the memory and terminate the background thread:
      delete apm;

    } catch(runtime_error &e) {
      logger(WARNING, e.what());
    }
  }

  return 0;
}
