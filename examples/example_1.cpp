/**
 * \file example_1.cpp
 * This is a simple example for using the ApMon class. 
 * It illustrates the way to send to MonALISA an UDP datagram with a 
 * parameter and its value, using the function sendParameter().
 * The number of datagrams can be specified in the command line (if it is not,
 * the default is 20).
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
  char *filename = (char *)"destinations_1.conf";  
  int nDatagrams = 20;
  char logmsg[100];
  double val = 0;
  int i;

  srand(time(NULL));

  if (argc ==2)
    nDatagrams = atoi(argv[1]);
   
  try {
    /* dynamic allocation is necessary because the background thread will be used */
    ApMon *apm = new ApMon(filename);

    /* for the first datagram sent we specify the cluster name, which will be
       cached and used for the next datagrams; the node name is left NULL, so the 
       local IP will be sent instead) */
    try {
      apm -> sendParameter((char *)"TestCluster1_cpp", NULL, (char *)"my_cpu_load", 
			val);
    } catch(runtime_error &e) {
      /* use the ApMon logger */
      logger(WARNING, e.what());
    }

    for (i = 0; i < nDatagrams - 1; i++) {
      /* add a value for the CPU load (random between 0 and 2) */
      val = 2 * (double)rand() / RAND_MAX;  
      sprintf(logmsg, "Sending %lf for my_cpu_load", val);
      logger(INFO, logmsg);
      /* use the wrapper function with simplified syntax */
     
      try {
	/* the cluster name is NULL, so the previously cached values for 
	   cluster name and node name will be used */
	apm -> sendParameter(NULL, NULL, (char *)"my_cpu_load", val);
      } catch(runtime_error &e) {
	fprintf(stderr, "Send operation failed: %s\n", e.what());
      }
      sleep(1);
      
    } // for
    
    delete apm;
  } catch(runtime_error &e) {
    fprintf(stderr, "%s\n", e.what());
  }
  
  return 0;
}
