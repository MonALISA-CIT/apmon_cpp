/**
 * \file example_3.cpp
 * This example illustrates the way to send several parameters in the
 * same UDP datagram, with the functions sendParameters() and 
 * sendTimedParameters().
 * The number of parameter sets can be specified in the command line (if 
 * it is not, the default is 20). A parameter set contains: the OS name and two 
 * random values for the parameters "my_cpu_load" and "my_cpu_idle".
 * The number of parameter sets that will be sent can be specified in the command 
 * line (if it is not, the default is 20).For each parameter set two 
 * datagrams are sent: one with a timestamp and one without a timestamp. For the
 * latter, the local time at the MonALISA host will be considered.
 * The file "destinations_3.conf" contains the addresses of the hosts to which
 * we send the parameters, and also the corresponding ports.
 */ 
#include <stdlib.h> 
#include <time.h>

#include "ApMon.h"

int main(int argc, char **argv) {
  char *filename = (char *)"destinations_3.conf";  
  char *my_os_name = (char *)"linux";
  int nDatagrams;
  int i;
  double val;
  int val2;
  long timestamp;

  int nParameters = 3;
  char **paramNames, **paramValues;
  int *valueTypes;

  if (argc == 2) 
    nDatagrams = atoi(argv[1]);
  else
    nDatagrams = 20;

  srand(time(NULL));

  /* allocate memory for the arrays */
  paramNames = (char **)malloc(nParameters * sizeof(char *));
  paramValues = (char **)malloc(nParameters * sizeof(char *));
  valueTypes = (int *)malloc(nParameters * sizeof(int));

  /* initialize the parameter names and types */
  paramNames[0] = (char *)"my_cpu_load";
  valueTypes[0] = XDR_REAL64;
  paramNames[1] = (char *)"os_name";
  valueTypes[1] = XDR_STRING;
  paramNames[2] = (char *)"my_cpu_idle";
  valueTypes[2] = XDR_INT32;

  /* initialize the pointers to the values */
  /* (the values will change, but the addresses remain the same) */ 
  paramValues[0] = (char *)&val;
  paramValues[1] = my_os_name; 
  paramValues[2] = (char *)&val2;

  /* start sending the datagrams */
  try {
    ApMon::setLogLevel((char *)"FATAL");
    ApMon *apm = new ApMon(filename);
    /* set the maximum number of messages that can be sent per second */
    apm -> setMaxMsgRate(100);
    
    for (i = 1; i <= nDatagrams; i++) {

      /* add a value for the CPU load (random between 0 and 2) */
      val = 2 * (double)rand() / RAND_MAX;
      printf("Sending %f for my_cpu_load\n", val);

      /* add a value for the CPU idle percentage (random between 0 and 50) */
      val2 = (int)(50 * (double)rand() / RAND_MAX);
      printf("Sending %d for cpu_idle\n", val2);

      /* send the datagram */
      try {
	apm -> sendParameters((char *)"TestCluster3_cpp", (char *)"MyNode3", nParameters, 
			   paramNames, valueTypes, paramValues);
	/* the timestamped datagram looks as if it was sent 2 hours ago: */
	timestamp = time(NULL) - 2 * 3600;
	apm -> sendTimedParameters((char *)"TestClusterOld3_cpp", (char *)"MyNode3", nParameters, 
			   paramNames, valueTypes, paramValues, timestamp);
      } catch(runtime_error &e) {
	fprintf(stderr, "Send operation failed: %s\n", e.what());
	//exit(-1);
      }
      
    } // for
    
    delete apm;
  } catch(runtime_error &e) {
    fprintf(stderr, "%s\n", e.what());
  }

  return 0;
}
