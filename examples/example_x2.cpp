/**
 * \file example_x2.cpp
 * This example shows how several jobs can be monitored with ApMon. We must
 * provide ApMon the PIDs of the jobs that we want to monitor. In order to 
 * find the PIDs of the jobs we shall parse the output of the ps command. 
 * We shall monitor the following applications: the current job, MonALISA and 
 * Apache (assuming the last two are currently running).
 * The file "destinations_x2.conf" contains the addresses of the hosts to which
 * we send the parameters, and also the corresponding ports. It also contains
 * lines in which different parameters from the job/system monitoring datagrams
 * can be enabled or disabled.
 */ 

#include <stdlib.h> 
#include <time.h>

#include "ApMon.h"

/** Finds the PID of the parent process for an application, given
 * the name of the application. 
 */
long getAppPid(const char *cmd) {
  FILE *fp;
  char buf[1024];
  bool found;
  long pid;

  fp = popen("ps afx", "r");
  if (fp == NULL) 
    return -1;

  found = false;
  while (fgets(buf, 1024, fp) != NULL) {
    if (strstr(buf, cmd) != NULL) {
      found = true;
      break;
    }
  }
  pclose(fp);
  if (found == false)
    return -1;

  sscanf(buf, "%ld", &pid);
  return pid;
}

int main(int argc, char **argv) {
  char *filename = (char *)"destinations_x2.conf";  
  char *osname = (char *)"linux";
  int nDatagrams;
  int i, val2 = 0;
  double val = 0;
  long pid_ml, pid_a;

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
  paramNames[1] = (char *)"my_os_name";
  valueTypes[1] = XDR_STRING;
  paramNames[2] = (char *)"my_cpu_idle";
  valueTypes[2] = XDR_INT32;

  /* initialize the pointers to the values */
  /* (the values will change, but the addresses remain the same) */ 
  paramValues[0] = (char *)&val;
  paramValues[1] = osname; 
  paramValues[2] = (char *)&val2;

  /* start sending the datagrams */
  try {
    ApMon *apm = new ApMon(filename);
    apm -> setJobMonitoring(true, 5);
    apm -> setSysMonitoring(true, 10);
    apm -> setGenMonitoring(true, 100);

    /* monitor this job */
    apm -> addJobToMonitor(getpid(), (char *)"", (char *)"JobCluster_apmon", NULL);

    /* monitor MonALISA */
    pid_ml = getAppPid((char *)"java -DMonaLisa_HOME");
    if (pid_ml != -1)
        apm -> addJobToMonitor(pid_ml, (char *)"", (char *)"JobCluster_monalisa", NULL);
    else
      fprintf(stderr, "Error obtaining PID for: java MonaLisa\n");
      
    /* monitor apache */
    pid_a = getAppPid("apache");
    if (pid_a != -1)
        apm -> addJobToMonitor(pid_a, (char *)"", (char *)"JobCluster_apache", NULL);
    else
      fprintf(stderr, "Error obtaining PID for: apache\n");
  
    for (i = 1; i <= nDatagrams; i++) {

      /* add a value for the CPU load (random between 0 and 2) */
      val = 2 * (double)rand() / RAND_MAX;
      printf("Sending %f for my_cpu_load\n", val);

      /* add a value for the CPU idle percentage (random between 0 and 50) */
      val2 = (int)(50 * (double)rand() / RAND_MAX);
      printf("Sending %d for my_cpu_idle\n", val2);

      /* send the datagram */
      try {
	apm -> sendParameters((char *)"TestClusterx2_cpp", (char *)"MyNode2", nParameters, 
			   paramNames, valueTypes, paramValues);
      } catch(runtime_error &e) {
	fprintf(stderr, "Send operation failed: %s\n", e.what());
	//exit(-1);
      }
      sleep(2);

      if (i == 25) {
	apm -> removeJobToMonitor(pid_a);
	apm -> removeJobToMonitor(getpid());
      }
    } // for
    
    delete apm;
  } catch(runtime_error &e) {
    fprintf(stderr, "%s\n", e.what());
  }

  return 0;
}
