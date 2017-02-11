/**
 * \file example_confgen.cpp
 * This example illustrates the way ApMon can obtain the configuration 
 * parameters from a servlet or a CGI script.
 */
 
#include <stdlib.h> 
#include <time.h>

#include "ApMon.h"
#include "utils.h"

using namespace apmon_utils;

int main(int argc, char **argv) {
  char *destList[1]= {(char *)"http://pcardaab.cern.ch:8888/cgi-bin/ApMonConf?appName=confgen_test"}; 
		      
  int nDatagrams = 20;
  char logmsg[100];
  double val = 0;
  int i, timestamp;

  if (argc ==2)
    nDatagrams = atoi(argv[1]);
  
   try {
    ApMon *apm = new ApMon(1, destList);
    apm -> setRecheckInterval(300);

    for (i = 0; i < nDatagrams; i++) {
      /* generate a value for the parameter  */
      val += 0.05;
      if (val > 2) 
	val = 0;
      sprintf(logmsg, "Sending %lf for cpu_load\n", val);
      logger(INFO, logmsg);

      /* use the wrapper function with simplified syntax */
     
      try {
	apm -> sendParameter((char *)"TestClusterCG_cpp", NULL, (char *)"my_parameter", 
			  XDR_REAL64, (char *)&val);

	/* send a datagram with timestamp */
	timestamp = time(NULL) - (5 * 3600); // as if we sent the datagram 5 hours ago
	apm -> sendTimedParameter((char *)"TestClusterOldCG_cpp", NULL, (char *)"my_parameter", 
			  XDR_REAL64, (char *)&val, timestamp) ;
      } catch(runtime_error &e) {
	logger(WARNING, e.what());
      }
    // test that we can stop the reloading thread  
    if (i == 15)
      apm -> setRecheckInterval(-1);
    sleep(2);
    } // for
    
    delete apm;
  } catch(runtime_error &e) {
    logger(WARNING, e.what());
  }
  
  return 0;
}
