/**
 * \file monitor_utils.cpp
 * This file contains the implementations of some functions used for 
 * obtaining monitoring information.
 */

/*
 * ApMon - Application Monitoring Tool
 *
 * Copyright (C) 2006 California Institute of Technology
 *
 * Permission is hereby granted, free of charge, to use, copy and modify 
 * this software and its documentation (the "Software") for any
 * purpose, provided that existing copyright notices are retained in 
 * all copies and that this notice is included verbatim in any distributions
 * or substantial portions of the Software. 
 * This software is a part of the MonALISA framework (http://monalisa.cacr.caltech.edu).
 * Users of the Software are asked to feed back problems, benefits,
 * and/or suggestions about the software to the MonALISA Development Team
 * (developers@monalisa.cern.ch). Support for this software - fixing of bugs,
 * incorporation of new features - is done on a best effort basis. All bug
 * fixes and enhancements will be made available under the same terms and
 * conditions as the original software,

 * IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
 * OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
 * EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 * THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT. THIS SOFTWARE IS
 * PROVIDED ON AN "AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO
 * OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR
 * MODIFICATIONS.
 */

#include "ApMon.h"
#include "monitor_utils.h"
#include "proc_utils.h"
#include "utils.h"
#include "mon_constants.h"

using namespace apmon_utils;
using namespace apmon_mon_utils;

void ApMon::sendJobInfo() {
#ifndef WIN32
  int i;
  long crtTime;

 /* the apMon_free() function calls sendJobInfo() from another thread and 
     we need mutual exclusion */
  pthread_mutex_lock(&mutexBack);

  if (nMonJobs == 0) {
    logger(WARNING, "There are no jobs to be monitored, not sending job monitoring information.");
    pthread_mutex_unlock(&mutexBack);
    return;
  }

  crtTime = time(NULL);
  logger(INFO, "Sending job monitoring information...");
  lastJobInfoSend = (time_t)crtTime; 

  /* send monitoring information for all the jobs specified by the user */
  for (i = 0; i < nMonJobs; i++) 
    sendOneJobInfo(monJobs[i]);

  pthread_mutex_unlock(&mutexBack);
#endif
}

void ApMon::updateJobInfo(MonitoredJob job) {
  bool needJobInfo, needDiskInfo;
  bool jobExists = true;
  char err_msg[200];

  PsInfo jobInfo;
  JobDirInfo dirInfo;

  /**** runtime, CPU & memory usage information ****/ 
  needJobInfo = actJobMonitorParams[JOB_RUN_TIME] || 
    actJobMonitorParams[JOB_CPU_TIME] || actJobMonitorParams[JOB_CPU_USAGE] ||
    actJobMonitorParams[JOB_MEM_USAGE] || actJobMonitorParams[JOB_VIRTUALMEM] 
    || actJobMonitorParams[JOB_RSS] || actJobMonitorParams[JOB_OPEN_FILES];
  if (needJobInfo) {
    try {
      readJobInfo(job.pid, jobInfo);
      currentJobVals[JOB_RUN_TIME] = jobInfo.etime;
      currentJobVals[JOB_CPU_TIME] = jobInfo.cputime; 
      currentJobVals[JOB_CPU_USAGE] = jobInfo.pcpu;
      currentJobVals[JOB_MEM_USAGE] = jobInfo.pmem; 
      currentJobVals[JOB_VIRTUALMEM] = jobInfo.vsz;
      currentJobVals[JOB_RSS] = jobInfo.rsz;

      if (jobInfo.open_fd < 0)
	jobRetResults[JOB_OPEN_FILES] = RET_ERROR;
      currentJobVals[JOB_OPEN_FILES] = jobInfo.open_fd;

    } catch (runtime_error &err) {
      logger(WARNING, err.what());
      jobRetResults[JOB_RUN_TIME] = jobRetResults[JOB_CPU_TIME] = 
	jobRetResults[JOB_CPU_USAGE] = jobRetResults[JOB_MEM_USAGE] =
	jobRetResults[JOB_VIRTUALMEM] = jobRetResults[JOB_RSS] =
	jobRetResults[JOB_OPEN_FILES] = RET_ERROR;
      strncpy(err_msg, err.what(), 199);
      if (strstr(err_msg, "does not exist") != NULL)
	jobExists = false;
    } 
  }

  /* if the monitored job has terminated, remove it */
  if (!jobExists) {
    try {
      removeJobToMonitor(job.pid);
    } catch (runtime_error &err) {
      logger(WARNING, err.what());
    }
    return;
  }

  /* disk usage information */
  needDiskInfo = actJobMonitorParams[JOB_DISK_TOTAL] || 
    actJobMonitorParams[JOB_DISK_USED] || actJobMonitorParams[JOB_DISK_FREE] ||
    actJobMonitorParams[JOB_DISK_USAGE] || actJobMonitorParams[JOB_WORKDIR_SIZE];
  if (needDiskInfo) {
    try {
      readJobDiskUsage(job, dirInfo);
      currentJobVals[JOB_WORKDIR_SIZE] = dirInfo.workdir_size;
      currentJobVals[JOB_DISK_TOTAL] = dirInfo.disk_total; 
      currentJobVals[JOB_DISK_USED] = dirInfo.disk_used;
      currentJobVals[JOB_DISK_USAGE] = dirInfo.disk_usage; 
      currentJobVals[JOB_DISK_FREE] = dirInfo.disk_free;
    } catch (runtime_error& err) {
      logger(WARNING, err.what());
      jobRetResults[JOB_WORKDIR_SIZE] = jobRetResults[JOB_DISK_TOTAL] = 
	jobRetResults[JOB_DISK_USED] = jobRetResults[JOB_DISK_USAGE] =
	jobRetResults[JOB_DISK_FREE] = RET_ERROR;
    }
  }
}
 
void ApMon::sendOneJobInfo(MonitoredJob job) {
  int i;
  int nParams = 0;

  char **paramNames, **paramValues;
  int *valueTypes;

  valueTypes = (int *)malloc(nJobMonitorParams * sizeof(int));
  paramNames = (char **)malloc(nJobMonitorParams * sizeof(char *));
  paramValues = (char **)malloc(nJobMonitorParams * sizeof(char *));

  for (i = 0; i < nJobMonitorParams; i++) {
    jobRetResults[i] = RET_SUCCESS;
    currentJobVals[i] = 0;
  }
    
  updateJobInfo(job);

  for (i = 0; i < nJobMonitorParams; i++) {
    if (actJobMonitorParams[i] && jobRetResults[i] != RET_ERROR) {
     
      paramNames[nParams] = jobMonitorParams[i];
      paramValues[nParams] = (char *)&currentJobVals[i];
      valueTypes[nParams] = XDR_REAL64;
      nParams++;
    } 
    /* don't disable the parameter (maybe for another job it can be
	 obtained) */
      /*
	else
	if (autoDisableMonitoring)
	actJobMonitorParams[ind] = 0;
      */
  }

  if (nParams == 0) {
    free(paramNames); free(valueTypes);
    free(paramValues);
    return;
  }

  try {
    if (nParams > 0)
      sendParameters(job.clusterName, job.nodeName, nParams, 
		     paramNames, valueTypes, paramValues);
  } catch (runtime_error& err) {
    logger(WARNING, err.what());
  }

  free(paramNames);
  free(valueTypes);
  free(paramValues);
}


void ApMon::updateSysInfo() {
  int needCPUInfo, needSwapPagesInfo, needLoadInfo, needMemInfo,
    needNetInfo, needUptime, needProcessesInfo, needNetstatInfo; 
 
  /**** CPU usage information ****/ 
  needCPUInfo = actSysMonitorParams[SYS_CPU_USAGE] || 
    actSysMonitorParams[SYS_CPU_USR] || actSysMonitorParams[SYS_CPU_SYS] ||
    actSysMonitorParams[SYS_CPU_NICE] || actSysMonitorParams[SYS_CPU_IDLE] ||
    actSysMonitorParams[SYS_CPU_IOWAIT] || actSysMonitorParams[SYS_CPU_IRQ] ||
    actSysMonitorParams[SYS_CPU_SOFTIRQ] || actSysMonitorParams[SYS_CPU_STEAL] ||
    actSysMonitorParams[SYS_CPU_GUEST];

  if (needCPUInfo) {
    try {
      ProcUtils::getCPUUsage(*this, currentSysVals[SYS_CPU_USAGE], 
			     currentSysVals[SYS_CPU_USR], 
			     currentSysVals[SYS_CPU_SYS],
			     currentSysVals[SYS_CPU_NICE], 
			     currentSysVals[SYS_CPU_IDLE],
			     currentSysVals[SYS_CPU_IOWAIT],
			     currentSysVals[SYS_CPU_IRQ],
			     currentSysVals[SYS_CPU_SOFTIRQ],
			     currentSysVals[SYS_CPU_STEAL],
			     currentSysVals[SYS_CPU_GUEST],
			     numCPUs);
    } catch (procutils_error &perr) {
      /* "permanent" error (the parameters could not be obtained) */
      logger(WARNING, perr.what());
      sysRetResults[SYS_CPU_USAGE] = sysRetResults[SYS_CPU_SYS] = 
	  sysRetResults[SYS_CPU_USR] = sysRetResults[SYS_CPU_NICE] =
	  sysRetResults[SYS_CPU_IDLE] = sysRetResults[SYS_CPU_USAGE] =
	  sysRetResults[SYS_CPU_IOWAIT] = sysRetResults[SYS_CPU_IRQ] =
	  sysRetResults[SYS_CPU_SOFTIRQ] = sysRetResults[SYS_CPU_STEAL] =
	  sysRetResults[SYS_CPU_GUEST] = 
	  PROCUTILS_ERROR;
    } catch (runtime_error &err) {
      /* temporary error (next time we might be able to get the paramerers) */
      logger(WARNING, err.what());
      sysRetResults[SYS_CPU_USAGE] = sysRetResults[SYS_CPU_SYS] = 
	  sysRetResults[SYS_CPU_USR] = sysRetResults[SYS_CPU_NICE] =
	  sysRetResults[SYS_CPU_IDLE] = sysRetResults[SYS_CPU_USAGE] =
	  sysRetResults[SYS_CPU_IOWAIT] = sysRetResults[SYS_CPU_IRQ] =
	  sysRetResults[SYS_CPU_SOFTIRQ] = sysRetResults[SYS_CPU_STEAL] =
	  sysRetResults[SYS_CPU_GUEST] =
	  RET_ERROR;
    }
  }

  needSwapPagesInfo = actSysMonitorParams[SYS_PAGES_IN] || 
    actSysMonitorParams[SYS_PAGES_OUT] || actSysMonitorParams[SYS_SWAP_IN] ||
    actSysMonitorParams[SYS_SWAP_OUT];

  if (needSwapPagesInfo) {
    try {
      ProcUtils::getSwapPages(*this, currentSysVals[SYS_PAGES_IN], 
			      currentSysVals[SYS_PAGES_OUT], 
			      currentSysVals[SYS_SWAP_IN],
			      currentSysVals[SYS_SWAP_OUT]);
    } catch (procutils_error &perr) {
      /* "permanent" error (the parameters could not be obtained) */
      logger(WARNING, perr.what());
      sysRetResults[SYS_PAGES_IN] = sysRetResults[SYS_PAGES_OUT] = 
      sysRetResults[SYS_SWAP_OUT] = sysRetResults[SYS_SWAP_IN] = PROCUTILS_ERROR;
    } catch (runtime_error &err) {
      /* temporary error (next time we might be able to get the paramerers) */
      logger(WARNING, err.what());
      sysRetResults[SYS_PAGES_IN] = sysRetResults[SYS_PAGES_OUT] = 
	sysRetResults[SYS_SWAP_IN] = sysRetResults[SYS_SWAP_OUT] = RET_ERROR;
    }
  }

  needLoadInfo = actSysMonitorParams[SYS_LOAD1] || 
    actSysMonitorParams[SYS_LOAD5] || actSysMonitorParams[SYS_LOAD15];
    
  if (needLoadInfo) {
    double dummyVal;
    try {
      /* the number of processes is now obtained with the getProcesses()
	 function, not with getLoad() */
      ProcUtils::getLoad(currentSysVals[SYS_LOAD1], currentSysVals[SYS_LOAD5], 
		    currentSysVals[SYS_LOAD15],dummyVal);
    } catch (procutils_error& perr) {
      /* "permanent" error (the parameters could not be obtained) */
      logger(WARNING, perr.what());
      sysRetResults[SYS_LOAD1] = sysRetResults[SYS_LOAD5] = 
	sysRetResults[SYS_LOAD15] = PROCUTILS_ERROR;
    }
  }

  /**** get statistics about the current processes ****/
  needProcessesInfo = actSysMonitorParams[SYS_PROCESSES];
  if (needProcessesInfo) {
    try {
      ProcUtils::getProcesses(currentSysVals[SYS_PROCESSES], 
			      currentProcessStates);
    } catch (runtime_error& err) {
      logger(WARNING, err.what());
      sysRetResults[SYS_PROCESSES] = RET_ERROR;
    }
  }

  /**** get the amount of memory currently in use ****/
  needMemInfo = actSysMonitorParams[SYS_MEM_USED] || 
    actSysMonitorParams[SYS_MEM_FREE] || actSysMonitorParams[SYS_SWAP_USED] ||
    actSysMonitorParams[SYS_SWAP_FREE] || actSysMonitorParams[SYS_MEM_USAGE] ||
    actSysMonitorParams[SYS_SWAP_USAGE];

  if (needMemInfo) {
    try {
      ProcUtils::getMemUsed(currentSysVals[SYS_MEM_USED], 
			    currentSysVals[SYS_MEM_FREE], 
			    currentSysVals[SYS_SWAP_USED],
			    currentSysVals[SYS_SWAP_FREE]);
      currentSysVals[SYS_MEM_USAGE] = 100 * currentSysVals[SYS_MEM_USED] /
	(currentSysVals[SYS_MEM_USED] +  currentSysVals[SYS_MEM_FREE]); 
      currentSysVals[SYS_SWAP_USAGE] = 100 * currentSysVals[SYS_SWAP_USED] /
	(currentSysVals[SYS_SWAP_USED] +  currentSysVals[SYS_SWAP_FREE]); 
    } catch (procutils_error &perr) {
      logger(WARNING, perr.what());
      sysRetResults[SYS_MEM_USED] = sysRetResults[SYS_MEM_FREE] = 
	sysRetResults[SYS_SWAP_USED] = sysRetResults[SYS_SWAP_FREE] = 
	sysRetResults[SYS_MEM_USAGE] = sysRetResults[SYS_SWAP_USAGE] = 
	PROCUTILS_ERROR;
    }
  }

  
  /**** network monitoring information ****/
  needNetInfo = actSysMonitorParams[SYS_NET_IN] || 
    actSysMonitorParams[SYS_NET_OUT] || actSysMonitorParams[SYS_NET_ERRS];
  if (needNetInfo && this -> nInterfaces > 0) {
    try {
      ProcUtils::getNetInfo(*this, &currentNetIn, &currentNetOut, 
			    &currentNetErrs);
    } catch (procutils_error &perr) {
      logger(WARNING, perr.what());
      sysRetResults[SYS_NET_IN] = sysRetResults[SYS_NET_OUT] = 
	sysRetResults[SYS_NET_ERRS] = PROCUTILS_ERROR;     
    } catch (runtime_error &err) {
      logger(WARNING, err.what());
      sysRetResults[SYS_NET_IN] = sysRetResults[SYS_NET_OUT] = 
	sysRetResults[SYS_NET_ERRS] = RET_ERROR; 
    }
  }

  needNetstatInfo = actSysMonitorParams[SYS_NET_SOCKETS] || 
    actSysMonitorParams[SYS_NET_TCP_DETAILS];
  if (needNetstatInfo) {
    try {
      ProcUtils::getNetstatInfo(*this, this -> currentNSockets, 
				this -> currentSocketsTCP); 
    } catch (runtime_error &err) {
      logger(WARNING, err.what());
      sysRetResults[SYS_NET_SOCKETS] = sysRetResults[SYS_NET_TCP_DETAILS] = 
	RET_ERROR; 
    }
  }

  needUptime = actSysMonitorParams[SYS_UPTIME];
  if (needUptime) {
    try {
      currentSysVals[SYS_UPTIME] = ProcUtils::getUpTime();
    } catch (procutils_error &perr) {
      logger(WARNING, perr.what());
      sysRetResults[SYS_UPTIME] = PROCUTILS_ERROR;
    } 
  }

}

void ApMon::sendSysInfo() {
#ifndef WIN32
  int nParams = 0, maxNParams;
  int i;
  long crtTime;

  int *valueTypes;
  char **paramNames, **paramValues;

  crtTime = time(NULL);
  logger(INFO, "Sending system monitoring information...");

  /* make some initializations only the first time this
     function is called */
  if (this -> sysInfo_first) {
    for (i = 0; i < this -> nInterfaces; i++) {
     this -> lastBytesSent[i] = this -> lastBytesReceived[i] = 0.0;
     this -> lastNetErrs[i] = 0;
     
    }
    this -> sysInfo_first = FALSE;
  }

  /* the maximum number of parameters that can be included in a datagram */
  /* (the last three terms are for: parameters corresponding to each possible
     state of the processes, parameters corresponding to the types of open 
     sockets, parameters corresponding to each possible state of the TCP
     sockets.) */
  maxNParams = nSysMonitorParams + (2 * nInterfaces - 1) + 15 + 4 + 
    N_TCP_STATES;

  valueTypes = (int *)malloc(maxNParams * sizeof(int));
  paramNames = (char **)malloc(maxNParams * sizeof(char *));
  paramValues = (char **)malloc(maxNParams * sizeof(char *));

  for (i = 0; i < nSysMonitorParams; i++) {
    if (actSysMonitorParams[i] > 0) /* if the parameter is enabled */
      sysRetResults[i] = RET_SUCCESS;
    else /* mark it with RET_ERROR so that it will be not included in the
	    datagram */
      sysRetResults[i] = RET_ERROR;
  }

  updateSysInfo();

  for (i = 0; i < nSysMonitorParams; i++) {
    if (i == SYS_NET_IN || i == SYS_NET_OUT || i == SYS_NET_ERRS ||
	i == SYS_NET_SOCKETS || i == SYS_NET_TCP_DETAILS || i == SYS_PROCESSES)
      continue;

    if (sysRetResults[i] == PROCUTILS_ERROR) {
      /* could not read the requested information from /proc, disable this
	 parameter */
        if (autoDisableMonitoring)
	    actSysMonitorParams[i] = 0;
    } 
    else 
    if (sysRetResults[i] != RET_ERROR && currentSysVals[i] != RET_ERROR) {
        /* the parameter is enabled and there were no errors obtaining it */
        paramNames[nParams] = strdup(sysMonitorParams[i]);
        paramValues[nParams] = (char *)&currentSysVals[i];
        valueTypes[nParams] = XDR_REAL64;
        nParams++;
    } 
  }

  if (actSysMonitorParams[SYS_NET_IN] == 1) {
    if (sysRetResults[SYS_NET_IN] == PROCUTILS_ERROR) {
      if (autoDisableMonitoring)
	actSysMonitorParams[SYS_NET_IN] = 0;
    } else  if (sysRetResults[SYS_NET_IN] != RET_ERROR) {
      for (i = 0; i < nInterfaces; i++) {
        if (currentNetIn[i] != RET_ERROR){ 
	    paramNames[nParams] =  (char *)malloc(20 * sizeof(char));
	    strncpy(paramNames[nParams], interfaceNames[i], 16);
	    strcat(paramNames[nParams], "_in");
	    paramValues[nParams] = (char *)&currentNetIn[i];
    	    valueTypes[nParams] = XDR_REAL64;
	    nParams++;
	}
      }
    }
  }

  if (actSysMonitorParams[SYS_NET_OUT] == 1) {
    if (sysRetResults[SYS_NET_IN] == PROCUTILS_ERROR) {
      if (autoDisableMonitoring)
	actSysMonitorParams[SYS_NET_OUT] = 0;
    } else  if (sysRetResults[SYS_NET_OUT] != RET_ERROR) {
      for (i = 0; i < nInterfaces; i++) { 
	if (currentNetOut[i] != RET_ERROR){
    	    paramNames[nParams] =  (char *)malloc(20 * sizeof(char));
	    strncpy(paramNames[nParams], interfaceNames[i], 15);
	    strcat(paramNames[nParams], "_out");
	    paramValues[nParams] = (char *)&currentNetOut[i];
    	    valueTypes[nParams] = XDR_REAL64;
	    nParams++;
	}
      }
    }
  }

  if (actSysMonitorParams[SYS_NET_ERRS] == 1) {
    if (sysRetResults[SYS_NET_ERRS] == PROCUTILS_ERROR) {
      if (autoDisableMonitoring)
	actSysMonitorParams[SYS_NET_ERRS] = 0;
    } else  if (sysRetResults[SYS_NET_ERRS] != RET_ERROR) {
      for (i = 0; i < nInterfaces; i++) {
        if (currentNetErrs[i] != RET_ERROR ){ 
	    paramNames[nParams] =  (char *)malloc(20 * sizeof(char));
	    strncpy(paramNames[nParams], interfaceNames[i], 14);
	    strcat(paramNames[nParams], "_errs");
	    paramValues[nParams] = (char *)&currentNetErrs[i];
	    valueTypes[nParams] = XDR_REAL64;
	    nParams++;
	}
      }
    }
  }


  if (actSysMonitorParams[SYS_PROCESSES] == 1) {
    if (sysRetResults[SYS_PROCESSES] != RET_ERROR) {
      char act_states[] = {'D', 'R', 'S', 'T', 'Z'};
      for (i = 0; i < 5; i++) {
        if (currentProcessStates[act_states[i] - 65] != RET_ERROR){
	  		paramNames[nParams] =  (char *)malloc(20 * sizeof(char));
	  		snprintf(paramNames[nParams], 19, "processes_%c", act_states[i]);
	  		paramValues[nParams] = (char *)&currentProcessStates[act_states[i] - 65];
	  		valueTypes[nParams] = XDR_REAL64;
	  		nParams++;
		}
      }
    }
  }

  if (actSysMonitorParams[SYS_NET_SOCKETS] == 1) {
    if (sysRetResults[SYS_NET_SOCKETS] != RET_ERROR) {
      const char * const socket_types[] = {"tcp", "udp", "icm", "unix"};
      for (i = 0; i < 4; i++) { 
        if (currentNSockets[i] != RET_ERROR) {
	    	paramNames[nParams] =  (char *)malloc(30 * sizeof(char));
	    	snprintf(paramNames[nParams], 29, "sockets_%s", socket_types[i]);
	    	paramValues[nParams] = (char *)&currentNSockets[i];
    	    valueTypes[nParams] = XDR_REAL64;
	    	nParams++;
		}
      }
    }
  }

  if (actSysMonitorParams[SYS_NET_TCP_DETAILS] == 1) {
    if (sysRetResults[SYS_NET_TCP_DETAILS] != RET_ERROR) {
      for (i = 0; i < N_TCP_STATES; i++) {
        if (currentSocketsTCP[i] != RET_ERROR){
	    	paramNames[nParams] =  (char *)malloc(30 * sizeof(char));
	    	snprintf(paramNames[nParams], 29, "sockets_tcp_%s", socketStatesMapTCP[i]);
	    	paramValues[nParams] = (char *)&currentSocketsTCP[i];
	    	valueTypes[nParams] = XDR_REAL64;
	    	nParams++;
		}
      }
    }
  }

  try {
    if (nParams > 0)
      sendParameters(sysMonCluster, sysMonNode, nParams, 
		     paramNames, valueTypes, paramValues);
  } catch (runtime_error& err) {
    logger(WARNING, err.what());
  }

  this -> lastSysInfoSend = crtTime;

  if (sysRetResults[SYS_NET_IN] == RET_SUCCESS) {
    free(currentNetIn);
    free(currentNetOut);
    free(currentNetErrs);
  }

  for (i = 0; i < nParams; i++)
    free(paramNames[i]);
  free(paramNames);
  free(valueTypes);
  free(paramValues);
#endif
}

void ApMon::updateGeneralInfo() {

  strcpy(cpuVendor, ""); strcpy(cpuFamily, "");
  strcpy(cpuModel, ""); strcpy(cpuModelName, "");

  if (actGenMonitorParams[GEN_CPU_MHZ] == 1 || 
      actGenMonitorParams[GEN_BOGOMIPS] == 1 || 
      actGenMonitorParams[GEN_CPU_VENDOR_ID] == 1 ||
      actGenMonitorParams[GEN_CPU_FAMILY] == 1 || 
      actGenMonitorParams[GEN_CPU_MODEL] == 1 ||
      actGenMonitorParams[GEN_CPU_MODEL_NAME] == 1) {
    try {
      ProcUtils::getCPUInfo(*this);
    } catch (procutils_error& err) {
      logger(WARNING, err.what());
      genRetResults[GEN_CPU_MHZ] = genRetResults[GEN_BOGOMIPS] = PROCUTILS_ERROR;
    }
  }

  if (actGenMonitorParams[GEN_TOTAL_MEM] == 1 || 
      actGenMonitorParams[GEN_TOTAL_SWAP] == 1) {
    try {
      ProcUtils::getSysMem(currentGenVals[GEN_TOTAL_MEM], 
			   currentGenVals[GEN_TOTAL_SWAP]);
    } catch (procutils_error& perr) {
      logger(WARNING, perr.what());
      genRetResults[GEN_TOTAL_MEM] = genRetResults[GEN_TOTAL_SWAP] = PROCUTILS_ERROR;
    }
  }

  if (this -> numCPUs > 0)
    currentGenVals[GEN_NO_CPUS] = this -> numCPUs;
  else
    genRetResults[GEN_NO_CPUS] = PROCUTILS_ERROR;
}

void ApMon::sendGeneralInfo() {
#ifndef WIN32
  int nParams, maxNParams, i;
  long crtTime;
  char tmp_s[50];
  
  char **paramNames, **paramValues;
  int *valueTypes;

  crtTime = time(NULL);
  logger(INFO, "Sending general monitoring information...");
  
  maxNParams = nGenMonitorParams + numIPs;
  valueTypes = (int *)malloc(maxNParams * sizeof(int));
  paramNames = (char **)malloc(maxNParams * sizeof(char *));
  paramValues = (char **)malloc(maxNParams * sizeof(char *));
  
  nParams = 0;

  updateGeneralInfo();

  if (actGenMonitorParams[GEN_HOSTNAME]) {
    paramNames[nParams] = strdup(genMonitorParams[GEN_HOSTNAME]);
    valueTypes[nParams] = XDR_STRING;
    paramValues[nParams] = myHostname;
    nParams++;
  }

  if (actGenMonitorParams[GEN_IP]) {
    for (i = 0; i < this -> numIPs; i++) {
      strcpy(tmp_s, "ip_");
      strncat(tmp_s, interfaceNames[i], 46);
      paramNames[nParams] = strdup(tmp_s);
      valueTypes[nParams] = XDR_STRING;
      paramValues[nParams] = this -> allMyIPs[i];
      nParams++;
    }
  }

  if (actGenMonitorParams[GEN_CPU_VENDOR_ID] && strlen(cpuVendor) != 0) {
    paramNames[nParams] = strdup(genMonitorParams[GEN_CPU_VENDOR_ID]);
    valueTypes[nParams] = XDR_STRING;
    paramValues[nParams] = cpuVendor;
    nParams++;
  }

  if (actGenMonitorParams[GEN_CPU_FAMILY] && strlen(cpuFamily) != 0) {
    paramNames[nParams] = strdup(genMonitorParams[GEN_CPU_FAMILY]);
    valueTypes[nParams] = XDR_STRING;
    paramValues[nParams] = cpuFamily;
    nParams++;
  }

  if (actGenMonitorParams[GEN_CPU_MODEL] && strlen(cpuModel) != 0) {
    paramNames[nParams] = strdup(genMonitorParams[GEN_CPU_MODEL]);
    valueTypes[nParams] = XDR_STRING;
    paramValues[nParams] = cpuModel;
    nParams++;
  }
  
  if (actGenMonitorParams[GEN_CPU_MODEL_NAME] && strlen(cpuModelName) != 0) {
    paramNames[nParams] = strdup(genMonitorParams[GEN_CPU_MODEL_NAME]);
    valueTypes[nParams] = XDR_STRING;
    paramValues[nParams] = cpuModelName;
    nParams++;
  }

  for (i = 0; i < nGenMonitorParams; i++) {
    if (actGenMonitorParams[i] != 1 || i == GEN_IP || i == GEN_HOSTNAME ||
	i == GEN_CPU_VENDOR_ID || i == GEN_CPU_FAMILY || i == GEN_CPU_MODEL
	|| i == GEN_CPU_MODEL_NAME)
      continue;

    if (genRetResults[i] == PROCUTILS_ERROR) {
      /* could not read the requested information from /proc, disable this
	 parameter */
      if (autoDisableMonitoring)
	actGenMonitorParams[i] = 0;
    } else if (genRetResults[i] != RET_ERROR) {
      paramNames[nParams] = strdup(genMonitorParams[i]);
      paramValues[nParams] = (char *)&currentGenVals[i];
      valueTypes[nParams] = XDR_REAL64;
      nParams++;
    } 
  }

  try {
    if (nParams > 0)
      sendParameters(sysMonCluster, sysMonNode, nParams, 
		     paramNames, valueTypes, paramValues);
  } catch (runtime_error& err) {
    logger(WARNING, err.what());
  }

  for (i = 0; i < nParams; i++)
    free(paramNames[i]);
  free(paramNames);
  free(valueTypes);
  free(paramValues);
#endif
}

void ApMon::initMonitoring() {
  int i;

  this -> autoDisableMonitoring = true;
  this -> sysMonitoring = false;
  this -> jobMonitoring = false;
  this -> genMonitoring = false;
  this -> confCheck = false;

#ifndef WIN32
  pthread_mutex_init(&this -> mutex, NULL);
  pthread_mutex_init(&this -> mutexBack, NULL);
  pthread_mutex_init(&this -> mutexCond, NULL);
  pthread_cond_init(&this -> confChangedCond, NULL);
#else
  logger(INFO, "init mutexes...");
  this -> mutex     = CreateMutex(NULL, FALSE, NULL);
  this -> mutexBack = CreateMutex(NULL, FALSE, NULL);
  this -> mutexCond = CreateMutex(NULL, FALSE, NULL);
  this -> confChangedCond = CreateEvent(NULL, FALSE, FALSE, NULL);

  // Initialize the Windows Sockets library

  WORD wVersionRequested;
  WSADATA wsaData;
  int err;
  wVersionRequested = MAKEWORD( 2, 0 );
  err = WSAStartup( wVersionRequested, &wsaData );
  if ( err != 0 ) {
    logger(FATAL, "Could not initialize the Windows Sockets library (WS2_32.dll)");
  }

#endif

  this -> haveBkThread = false;
  this -> bkThreadStarted = false;
  this -> stopBkThread = false;

  this -> recheckChanged = false;
  this -> jobMonChanged = false;
  this -> sysMonChanged = false;

  this -> recheckInterval = RECHECK_INTERVAL;
  this -> crtRecheckInterval = RECHECK_INTERVAL;
  this -> jobMonitorInterval = JOB_MONITOR_INTERVAL;
  this -> sysMonitorInterval = SYS_MONITOR_INTERVAL;

  this -> nSysMonitorParams = initSysParams(this -> sysMonitorParams);

  this -> nGenMonitorParams = initGenParams(this -> genMonitorParams);

  this -> nJobMonitorParams = initJobParams(this -> jobMonitorParams);

  initSocketStatesMapTCP(this -> socketStatesMapTCP);

  this -> sysInfo_first = true;
  
  try {
    this -> lastSysInfoSend = ProcUtils::getBootTime();
  } catch (procutils_error& perr) {
    logger(WARNING, perr.what());
    logger(WARNING, "The first system monitoring values may be inaccurate");
    this -> lastSysInfoSend = 0;
  } 

  for (i = 0; i < nSysMonitorParams; i++)
    this -> lastSysVals[i] = 0;

  //this -> lastUsrTime = this -> lastSysTime = 0;
  //this -> lastNiceTime = this -> lastIdleTime = 0;

  for (i = 0; i < nSysMonitorParams; i++) {
    actSysMonitorParams[i] = 1;
    sysRetResults[i] = RET_SUCCESS;
  }

  for (i = 0; i < nGenMonitorParams; i++) {
    actGenMonitorParams[i] = 1;
    genRetResults[i] = RET_SUCCESS;
  }

  for (i = 0; i < nJobMonitorParams; i++) {
    actJobMonitorParams[i] = 1;
    jobRetResults[i] = RET_SUCCESS;
  }

  this -> maxMsgRate = MAX_MSG_RATE;
}

void ApMon::parseXApMonLine(char *line) {
  bool flag, found;
  int ind;
  char tmp[MAX_STRING_LEN], logmsg[200];
  char *param, *value;
//  char sbuf[MAX_STRING_LEN];
//  char *pbuf = sbuf;
  char *sep = (char *)" =";

  strncpy(tmp, line, MAX_STRING_LEN-1);
  char *tmp2 = tmp + strlen("xApMon_");

  param = strtok/*_r*/(tmp2, sep);//, &pbuf);
  value = strtok/*_r*/(NULL, sep);//, &pbuf);

  /* if it is an on/off parameter, assign its value to flag */
  if (strcmp(value, "on") == 0)
    flag = true;
  else /* if it is not an on/off paramenter the value of flag doesn't matter */
    flag = false;

  pthread_mutex_lock(&mutexBack);

  found = false;
  if (strcmp(param, "job_monitoring") == 0) {
    this -> jobMonitoring = flag; found = true;
  }
  if (strcmp(param, "sys_monitoring") == 0) {
    this -> sysMonitoring = flag; found = true;
  }
  if (strcmp(param, "job_interval") == 0) {
    this -> jobMonitorInterval = atol(value); found = true;
  }
  if (strcmp(param, "sys_interval") == 0) {
    this -> sysMonitorInterval = atol(value); found = true;
  }
  if (strcmp(param, "general_info") == 0) {
    this -> genMonitoring = flag; found = true;
  }
  if (strcmp(param, "conf_recheck") == 0) {
    this -> confCheck = flag; found = true;
  }
  if (strcmp(param, "recheck_interval") == 0) {
    this -> recheckInterval = this -> crtRecheckInterval = atol(value); 
    found = true;
  }
  if (strcmp(param, "auto_disable") == 0) {
    this -> autoDisableMonitoring = flag;
    found = true;
  }
  if (strcmp(param, "maxMsgRate") == 0) {
    this -> maxMsgRate = atoi(value);
    found = true;
  }

  if (found) {
    pthread_mutex_unlock(&mutexBack);
    return;
  }

  if (strstr(param, "sys_") == param) {
    ind = getVectIndex(param + strlen("sys_"), sysMonitorParams, 
		       nSysMonitorParams);
    if (ind < 0) {
      pthread_mutex_unlock(&mutexBack);
      snprintf(logmsg, 199, "Invalid parameter name in the configuration file: %s", param);
      logger(WARNING, logmsg);
      return;
    }
    found = true;
    this -> actSysMonitorParams[ind] = (int)flag;
  }

  if (strstr(param, "job_") == param) {
    ind = getVectIndex(param + strlen("job_"), jobMonitorParams, 
		       nJobMonitorParams);
    
    if (ind < 0) {
      pthread_mutex_unlock(&mutexBack);
      snprintf(logmsg, 199, "Invalid parameter name in the configuration file: %s", param);
      logger(WARNING, logmsg);
      return;
    }
    found = true;
    this -> actJobMonitorParams[ind] = (int)flag;
  }

  if (!found) {
    ind = getVectIndex(param, genMonitorParams, 
		       nGenMonitorParams);
    if (ind < 0) {
      pthread_mutex_unlock(&mutexBack);
      snprintf(logmsg, 199, "Invalid parameter name in the configuration file: %s", param);
      logger(WARNING, logmsg);
      return;
    } else {
      found = true;
      this -> actGenMonitorParams[ind] = (int)flag;
    }
  }

  if (!found) {
    snprintf(logmsg, 199, "Invalid parameter name in the configuration file: %s", param);
    logger(WARNING, logmsg);
  }
  pthread_mutex_unlock(&mutexBack);
}
  
long *apmon_mon_utils::getChildren(long pid, int& nChildren){
#ifdef WIN32
	return 0;
#else
  FILE *pf;
  long *pids, *ppids, *children;
  int nProcesses;
  int i, j, status;
  pid_t cpid;
  char *argv[4], msg[MAX_STRING_LEN], sval[20];
  bool processFound;
  long mypid = getpid();
  char children_f[50], np_f[50], cmd[200];

  /* generate the names of the temporary files in which we have the output
     of some commands */
  snprintf(children_f, 49, "/tmp/apmon_children%ld", mypid);
  snprintf(np_f, 49, "/tmp/apmon_np%ld", mypid);

  switch (cpid = fork()) {
  case -1:
    return 0;
  case 0:
    argv[0] = (char *)"/bin/sh"; argv[1] = (char *)"-c";
    snprintf(cmd, 199, "ps --no-headers -A -o ppid,pid > %s && wc -l %s > %s", children_f, children_f, np_f);
    argv[2] = cmd;
    /*
    argv[2] = "ps --no-headers -eo ppid,pid > /tmp/apmon_children.txt && wc -l /tmp/out_children.txt > /tmp/out_np.txt";
    */
    argv[3] = 0;
    execv("/bin/sh", argv);
    exit(RET_ERROR);
  default:
    if (waitpid(cpid, &status, 0) == -1) {
      snprintf(msg, MAX_STRING_LEN-1, "[ getChildren() ] The number of sub-processes for %ld could not be determined", pid);
      unlink(children_f); unlink(np_f);
      return 0;
    }
  }

  /* find the number of processes */
  pf = fopen(np_f, "rt");
  if (pf == NULL) {
    unlink(np_f); unlink(children_f);
    snprintf(msg, MAX_STRING_LEN-1, "[ getChildren() ] The number of sub-processes for %ld could not be determined",
	    pid);
    return 0;
  } 
  int retScan = fscanf(pf, "%d", &nProcesses);
  if (retScan < 1)
    nProcesses = 1;
    
  fclose(pf);   
  unlink(np_f);

  pids = (long *)malloc(nProcesses * sizeof(long)); 
  ppids = (long *)malloc(nProcesses * sizeof(long)); 
  /* estimated maximum size for the returned vector; it will be realloc'ed */
  children = (long *)malloc(nProcesses * sizeof(long));

  pf = fopen(children_f, "rt");
  if (pf == NULL) {
    free(pids); free(ppids); free(children);
    unlink(children_f);
    snprintf(msg, MAX_STRING_LEN-1, "[ getChildren() ] The sub-processes for %ld could not be determined", pid);
    return 0;
  } 
 
  /* scan the output of the ps command and find the children of the process,
   and also check if the process is still running */
  children[0] = pid; nChildren = 1;
  processFound = false;
  for (i = 0; i < nProcesses; i++) {
    retScan = fscanf(pf, "%ld %ld", &ppids[i], &pids[i]);
    if (retScan < 2)
	continue;
    /* look for the given process */
    if (pids[i] == children[0] || ppids[i] == children[0])
      processFound = true;
    if (ppids[i] == children[0]) {
      children[nChildren++] = pids[i];
    }
  }
  fclose(pf);
  unlink(children_f);

  if (processFound == false) {
    free(pids); free(ppids); free(children);
    nChildren = 0;
    snprintf(msg, MAX_STRING_LEN-1, "[ getChildren() ] The process %ld does not exist", pid);
    return 0;
  } 

  /* find the PIDs of all the descendant processes */
  i = 1;
  while (i < nChildren) {
    /* find the children of the i-th child */ 
    for (j = 0; j < nProcesses; j++) {
      if (ppids[j] == children[i]) {
	children[nChildren++] = pids[j];
      }
    }
    i++;
  }

  snprintf(msg, MAX_STRING_LEN-1, "Sub-processes for process %ld: ", pid);
  for (i = 0; i < nChildren; i++) {
    snprintf(sval, 19, "%ld ", children[i]);
    if (strlen(msg) + strlen(sval) < MAX_STRING_LEN - 1)
      strcat(msg, sval);
  }
  logger(DEBUG, msg);

  free(pids); free(ppids);
  children = (long *)realloc(children, (nChildren) * sizeof(long));
  return children;
#endif
}

void apmon_mon_utils::readJobInfo(long pid, PsInfo& info){
#ifndef WIN32
  long *children;
  FILE *fp;
  int i, nChildren, status, ch, ret, open_fd;
  char *cmd , *mem_cmd_s, *argv[4], *ret_s;
  char pid_s[10], msg[100];
  char cmdName[MAX_STRING_LEN1], buf[MAX_STRING_LEN1], buf2[MAX_STRING_LEN];
  char etime_s[20], cputime_s[20];
  double rsz, vsz;
  double etime, cputime;
  double pcpu, pmem;
  /* this list contains strings of the form "rsz_vsz_command" for every pid;
     it is used to avoid adding several times processes that have multiple 
     threads and appear in ps as sepparate processes, occupying exactly the 
     same amount of memory and having the same command name. For every line 
     from the output of the ps command we verify if the rsz_vsz_command 
     combination is already in the list.
  */
  char **mem_cmd_list;
  int listSize;
  long cpid, crt_pid;
  //unsigned int maxCmdLen = 5 * MAX_STRING_LEN;
  long mypid = getpid();
  char ps_f[50];

  /* get the list of the process' descendants */
  children = getChildren(pid, nChildren);

  /* generate a name for the temporary file which holds the output of the 
     ps command */
  snprintf(ps_f, 49, "/tmp/apmon_ps%ld", mypid);

  unsigned int cmdLen = (150 + 6 * nChildren) * sizeof(char);
  cmd = (char *)malloc (cmdLen);

  /* issue the "ps" command to obtain information on all the descendants */
  strcpy(cmd, "ps --no-headers --pid ");
  for (i = 0; i < nChildren - 1; i++) {
    snprintf(pid_s, 9, "%ld,", children[i]);
    if (strlen(cmd) + strlen(pid_s) + 1 >= cmdLen) {
      free(cmd);
      snprintf(msg, 99, "[ readJobInfo() ] Job %ld has too many sub-processes to be monitored", pid);
      return;
    }
    strcat(cmd, pid_s);
    //strcat(cmd, " 2>&1");
  }

  /* the last part of the command */
  snprintf(pid_s, 9, "%ld", children[nChildren - 1]);
  snprintf(cmdName, MAX_STRING_LEN1-1, " -o pid,etime,time,%%cpu,%%mem,rsz,vsz,comm > %s", ps_f);
  if (strlen(cmd) + strlen(pid_s) + strlen(cmdName) >= cmdLen) {
    free(cmd);
    snprintf(msg, 99, "[ readJobInfo() ] Job %ld has too many sub-processes to be monitored", pid);
    return;;
  }
  strncat(cmd, pid_s, cmdLen - strlen(cmd) - 1);
  strncat(cmd, cmdName, cmdLen - strlen(cmd) - 1);
  //strcat(cmd, " 2>&1");

  switch (cpid = fork()) {
  case -1:
    free(cmd);
    snprintf(msg, 99, "[ readJobInfo() ] Unable to fork(). The job information could not be determined for %ld", pid);
    return;
  case 0:
    argv[0] = (char *)"/bin/sh"; argv[1] = (char *)"-c";
    argv[2] = cmd; argv[3] = 0;
    execv("/bin/sh", argv);
    exit(RET_ERROR);
  default:
    if (waitpid(cpid, &status, 0) == -1) {
      free(cmd);
      snprintf(msg, 99, "[ readJobInfo() ] The job information for %ld could not be determined", pid);
      return;
    }
  }

  free(cmd);
  fp = fopen(ps_f, "rt");
  if (fp == NULL) {
    snprintf(msg, 99, "[ readJobInfo() ] Error opening the ps output file for process %ld", pid);
    return;
  }

  /* parse the output file */
  info.etime = info.cputime = 0;
  info.pcpu = info.pmem = 0;
  info.rsz = info.vsz = 0;
  info.open_fd = 0;
  mem_cmd_list = (char **)malloc(nChildren * sizeof(char *));
  listSize = 0;
  cmdName[0] = 0;
  while (1) {
    ret_s = fgets(buf, MAX_STRING_LEN, fp);
    if (ret_s == NULL) 
      break;
    buf[MAX_STRING_LEN - 1] = 0;

    /* if the line was too long and fgets hasn't read it entirely, */
    /* keep only the first 512 chars from the line */
    ch = fgetc(fp); // see if we are at the end of the file
    ungetc(ch, fp);
    if (buf[strlen(buf) - 1] != 10 && ch != EOF) { 
      while (1) {
	char *sret = fgets(buf2, MAX_STRING_LEN, fp);
	if (sret == NULL || buf[strlen(buf) - 1] == 10)
	  break;
      }
    }

    ret = sscanf(buf, "%ld %s %s %lf %lf %lf %lf %s", &crt_pid, etime_s, 
		 cputime_s, &pcpu, &pmem, &rsz, &vsz, cmdName);
    if (ret != 8) {
      fclose(fp);
      unlink(ps_f);
      free(children);
      for (i = 0; i < listSize; i++) {
	free(mem_cmd_list[i]);
      }
      free(mem_cmd_list);
      return;
    }

    /* etime is the maximum of the elapsed times for the subprocesses */
    etime = parsePSTime(etime_s);
    info.etime = (info.etime > etime) ? info.etime : etime;

    /* cputime is the sum of the cpu times for the subprocesses */
    cputime = parsePSTime(cputime_s);
    info.cputime += cputime;
    info.pcpu += pcpu;

    /* get the number of opened file descriptors */
    try {
      open_fd = ProcUtils::countOpenFiles(crt_pid);
    } catch (procutils_error& err) {
      logger(WARNING, err.what());
      /* don't throw an exception if we couldn't read the number of files */
      open_fd = PROCUTILS_ERROR;
    }

    /* see if this is a process or just a thread */
    mem_cmd_s = (char *)malloc(MAX_STRING_LEN * 2 * sizeof(char));
    snprintf(mem_cmd_s, MAX_STRING_LEN*2-1, "%lf_%lf_%s", rsz, vsz, cmdName);
    //printf("### mem_cmd_s: %s\n", mem_cmd_s);
    if (getVectIndex(mem_cmd_s, mem_cmd_list, listSize) == -1) {
      /* aonther pid with the same command name, rsz and vsz was not found,
	 so this is a new process and we can add the amount of memory used by 
	 it */
      info.pmem += pmem;
      info.vsz += vsz; info.rsz += rsz;

      if (info.open_fd >= 0) // if no error occured so far
	info.open_fd += open_fd;
      /* add an entry in the list so that next time we see another thread of
	 this process we don't add the amount of  memory again */
      mem_cmd_list[listSize++] = mem_cmd_s;     
    } else {
      free(mem_cmd_s);
    }

    /* if we monitor the current process, we have two extra opened files
       that we shouldn't take into account (the output file for ps and
       /proc/<pid>/fd/)
    */
    if (crt_pid == getpid())
      info.open_fd -= 2;
  } 

  fclose(fp);
  unlink(ps_f);
  free(children);
  for (i = 0; i < listSize; i++) {
    free(mem_cmd_list[i]);
  }
  free(mem_cmd_list);
#endif
}

long apmon_mon_utils::parsePSTime(char *s) {
  long days, hours, mins, secs;

  if (strchr(s, '-') != NULL) {
    sscanf(s, "%ld-%ld:%ld:%ld", &days, &hours, &mins, &secs);
    return 24 * 3600 * days + 3600 * hours + 60 * mins + secs;
  } else {
    if (strchr(s, ':') != NULL && strchr(s, ':') !=  strrchr(s, ':')) {
       sscanf(s, "%ld:%ld:%ld", &hours, &mins, &secs);
       return 3600 * hours + 60 * mins + secs;
    } else {
      if (strchr(s, ':') != NULL) {
	sscanf(s, "%ld:%ld", &mins, &secs);
	return 60 * mins + secs;
      } else {
	return RET_ERROR;
      }
    }
  }
}

void apmon_mon_utils::readJobDiskUsage(MonitoredJob job, JobDirInfo& info){
#ifndef WIN32
  int status;
  pid_t cpid;
  char *cmd, s_tmp[20], *argv[4], msg[200];
  FILE *fp;
  long mypid = getpid();
  char du_f[50], df_f[50]; 

  /* generate names for the temporary files which will hold the output of the
     du and df commands */
  snprintf(du_f, 49, "/tmp/apmon_du%ld", mypid);
  snprintf(df_f, 49, "/tmp/apmon_df%ld", mypid);
  
  if (strlen(job.workdir) == 0) {
    snprintf(msg, 199, "[ readJobDiskUsage() ] The working directory for the job %ld was not specified, not monitoring disk usage", job.pid);
    return;
  }
  
  unsigned int cmdLen = 300 + 2 * strlen(job.workdir);
  
  cmd = (char *)malloc(cmdLen * sizeof(char));
  strcpy(cmd, "PRT=`du -Lsk ");
  strncat(cmd, job.workdir, cmdLen - 20);
  //strcat(cmd, " | tail -1 | cut -f 1 > ");
  strncat(cmd, " ` ; if [[ $? -eq 0 ]] ; then OUT=`echo $PRT | cut -f 1` ; echo $OUT ; exit 0 ; else exit -1 ; fi > ", cmdLen - strlen(cmd) - 1); 
  strncat(cmd, du_f, cmdLen - strlen(cmd) - 1);
  
  switch (cpid = fork()) {
  case -1:
    snprintf(msg, 199, "[ readJobDiskUsage() ] Unable to fork(). The disk usage information could not be determined for %ld", job.pid);
    return;
  case 0:
    argv[0] = (char *)"/bin/sh"; argv[1] = (char *)"-c";
    argv[2] = cmd; argv[3] = 0;
    execv("/bin/sh", argv);
    exit(RET_ERROR);
  default:
    if (waitpid(cpid, &status, 0) == -1) {
      free(cmd);
      snprintf(msg, 199, "[ readJobDiskUsage() ] The disk usage (du) information for %ld could not be determined", job.pid);
      unlink(du_f); unlink(df_f);
      return;
    }
  }

  strcpy(cmd, "PRT=`df -m ");
  strncat(cmd, job.workdir, cmdLen - strlen(cmd) - 1);
  //strncat(cmd, " | tail -1 > ", cmdLen - strlen(cmd) - 1);
  strncat(cmd, " `; if [[ $? -eq 0 ]] ; then OUT=`echo $PRT | cut -d ' ' -f 8-` ; echo $OUT ; exit 0 ; else exit -1 ; fi > ", cmdLen - strlen(cmd) - 1);

  strncat(cmd, df_f, cmdLen - strlen(cmd) - 1);
  //printf("### cmd: %s\n", cmd);

  switch (cpid = fork()) {
  case -1:
    snprintf(msg, 199, "[ readJobDiskUsage() ] Unable to fork(). The disk usage information could not be determined for %ld", job.pid);
    return;
  case 0:
    argv[0] = (char *)"/bin/sh"; argv[1] = (char *)"-c";
    argv[2] = cmd; argv[3] = 0;
    execv("/bin/sh", argv);
    exit(RET_ERROR);
  default:
    if (waitpid(cpid, &status, 0) == -1) {
      free(cmd);
      snprintf(msg, 199, "[ readJobDiskUsage() ] The disk usage (df) information for %ld could not be determined", job.pid);
      unlink(du_f); unlink(df_f);
      return;
    }
  }

  free(cmd);
  fp = fopen(du_f, "rt");
  if (fp == NULL) {
    snprintf(msg, 199, "[ readJobDiskUsage() ] Error opening du output file for process %ld", job.pid);
    return;
  }

  int retScan = fscanf(fp, "%lf", &(info.workdir_size));
  if (retScan != 1) {
    fclose(fp); unlink(du_f);
    snprintf(msg, 199, "[ readJobDiskUsage() ] Error reading du output file for process %ld", job.pid);
    return;
  }
  /* keep the directory size in MB */
  info.workdir_size /= 1024.0;
  fclose(fp);
  unlink(du_f);
 
  fp = fopen(df_f, "rt");
  if (fp == NULL) {
    snprintf(msg, 199, "[ readJobDiskUsage() ] Error opening df output file for process %ld", job.pid);
    return;
  }
  retScan = fscanf(fp, "%s %lf %lf %lf %lf", s_tmp, &(info.disk_total), 
	 &(info.disk_used), &(info.disk_free), &(info.disk_usage));
  if (retScan != 5) {	 
    fclose(fp); unlink(du_f);
    snprintf(msg, 199, "[ readJobDiskUsage() ] Error reading df output file for process %ld", job.pid);
    return;
  }
	 
  fclose(fp);
  unlink(df_f);
  
#endif
}
