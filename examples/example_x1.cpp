/**
 * \file example_x1.cpp
 * This is a simple example for using the ApMon class and the xApMon extension.
 * This example "monitors itself" (i.e., sends datagrams containing 
 * information about the current process).  
 * The file "destinations_x1.conf" contains the addresses of the hosts to which
 * we send the parameters, and also the corresponding ports. The file also
 * contains settings for the job and system monitoring (there is also the 
 * possibility to enable or disable each monitoring parameter separately).
 */ 
#include <stdlib.h> 
#include <time.h>

#include "ApMon.h"
#include "utils.h"

using namespace apmon_utils;

int main(int argc, char **argv) {
  char *filename = (char *)"destinations_x1.conf";  
  char *destlist[2] = {(char *)"http://panther.rogrid.pub.ro/corina/destinations_x1.conf",
		       (char *)"http://cipsm.rogrid.pub.ro/~corina/destinations_x1.conf"};
  int nDatagrams = 100;
  char clusterName[30], workdir[100], logmsg[100];
  FILE *fp;
  double val;
  int i, retScan;

  srand(time(NULL));

  if (argc ==2)
    nDatagrams = atoi(argv[1]);

  /* get the working directory for this job */
  fp = popen("pwd", "r");
  if (fp == NULL) {
    fprintf(stderr, "Error getting the job working directory");
    strcpy(workdir, "");
  } else {
    retScan = fscanf(fp, "%s", workdir);
    if (retScan != 1) {
	fprintf(stderr, "Error getting the job working directory");
	strcpy(workdir, "");
    }
    pclose(fp);
  }
  //printf("workdir: %s\n", workdir);

  strcpy(clusterName, "TestCluster_job");

  // set the log level to FINE:
  ApMon::setLogLevel((char *)"FINE");
  
  try {
    /* try two ways to initialize ApMon */
    //ApMon *apm = new ApMon(filename);
     //or:
    ApMon *apm = new ApMon(2, destlist);

    // monitor this job (if job monitoring is enabled from the conf file)
    apm -> addJobToMonitor(getpid(), workdir, clusterName, NULL);

    for (i = 0; i < nDatagrams; i++) {
      /* add a value for the CPU load (random between 0 and 2) */
      val = 2 * (double)rand() / RAND_MAX;  
      sprintf(logmsg, "Sending %lf for my_cpu_load\n", val);
      logger(INFO, logmsg);

      /* use the wrapper function with simplified syntax */
      /* (the node name is left NULL, so the local IP will be sent instead) */
      try {
	apm -> sendParameter((char *)"TestClusterx1_cpp", NULL, (char *)"my_cpu_load", 
			  val);
      } catch(runtime_error &e) {
	logger(WARNING, e.what());
      }
      sleep(1);
      
    } // for
    
    delete apm;
  } catch(runtime_error &e) {
    logger(WARNING, e.what());
  }
  
  return 0;
}
