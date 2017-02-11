/**
 * \file example_2.cpp
 * This is a simple example for using the ApMon structure. 
 * It illustrates the way to send to MonALISA an UDP datagram with a 
 * parameter and its value, using the sendParameter() and sendTimedParameter()
 *  functions.
 * The number of parameters that will be sent can be specified in the command 
 * line (if it is not, the default is 20). For each parameter value two 
 * datagrams are sent: one with a timestamp and one without a timestamp. For the
 * latter, the local time at the MonALISA host will be considered.
 */ 
#include <stdlib.h> 
#include <time.h>

#include "../ApMon.h"
#include "../utils.h"

using namespace apmon_utils;

int main(int argc, char **argv) {
  char *destList[2]= {(char *)"http://panther.rogrid.pub.ro/corina/destinations_2.conf", 
		      (char *)"monalisa2.cern.ch password"};
  int nDatagrams = 20;
  char logmsg[100];
  double val = 0;
  int i, timestamp;

  if (argc ==2)
    nDatagrams = atoi(argv[1]);
  
   try {
    ApMon *apm = new ApMon(2, destList);
    ApMon::setLogLevel((char *)"FINE");
    // alternative way to initialize ApMon:
    //ApMon *apm = new ApMon("http://monalisa2.cern.ch/~catac/apmon/destinations.conf");
    apm -> setRecheckInterval(240);

    for (i = 0; i < nDatagrams; i++) {
      /* generate a value for the parameter  */
      val += 0.05;
      if (val > 2) 
	val = 0;
      sprintf(logmsg, "Sending %lf for cpu_load\n", val);
      logger(INFO, logmsg);

      /* use the wrapper function with simplified syntax */
     
      try {
	apm -> sendParameter((char *)"TestCluster2_cpp", NULL, (char *)"my_parameter", 
			  XDR_REAL64, (char *)&val);
	apm -> sendParameter((char *)"TestCluster2_cpp", NULL, (char *)"my_counter", XDR_INT32, (char *)&i);
	/* send a datagram with timestamp */
	timestamp = time(NULL) - (5 * 3600); // as if we sent the datagram 5 hours ago
	apm -> sendTimedParameter((char *)"TestClusterOld2_cpp", NULL, (char *)"my_parameter", 
			  XDR_REAL64, (char *)&val, timestamp) ;
      } catch(runtime_error &e) {
	logger(WARNING, e.what());
      }
      // test that we can stop the reloading thread  
      if (i == 15){
//	printf("###### setting recheck interval to -1\n");
	apm -> setRecheckInterval(-1);
      }
#ifdef WIN32
      Sleep(1000);
#else
      sleep(1);
#endif
    } // for
    
    delete apm;
  } catch(runtime_error &e) {
    logger(WARNING, e.what());
  }
  
  return 0;
}
