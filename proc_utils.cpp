/**
 * \file proc_utils.cpp
 * This file contains the implementations of the methods for extracting 
 * information from the proc filesystem.
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
#include "mon_constants.h"
#include "utils.h"
#include "proc_utils.h"

#ifndef WIN32
#include <dirent.h>
#endif

using namespace apmon_utils;

void ProcUtils::getCPUUsage(ApMon& apm, double& cpuUsage, 
			       double& cpuUsr, double& cpuSys, 
			       double& cpuNice, double& cpuIdle,
			       double& cpuIOWait, double& cpuIRQ,
			       double& cpuSoftIRQ, double& cpuSteal,
			       double& cpuGuest,
			       int numCPUs){

#ifndef WIN32
  FILE *fp1;
  char line[MAX_STRING_LEN];
  char s1[20];
  double 
    usrTime = RET_ERROR, 
    sysTime = RET_ERROR, 
    niceTime = RET_ERROR , 
    idleTime = RET_ERROR , 
    iowaitTime = RET_ERROR , 
    irqTime = RET_ERROR , 
    softirqTime = RET_ERROR , 
    stealTime = RET_ERROR , 
    guestTime = RET_ERROR, 
    totalTime = 0 ;
    
  int indU, indS, indN, indI, indIOWAIT, indIRQ, indSOFTIRQ, indSTEAL, indGUEST;

  time_t crtTime = time(NULL);

#ifdef __SUNOS
	double tmp = 0;

	char extraline[MAX_STRING_LEN];

	fp1 = popen("vmstat -s", "r");

	while (fgets(line, MAX_STRING_LEN, fp1)){
		sscanf(line, "%lf %s", &tmp, extraline);

		if (!strcmp(extraline, "user"))
			usrTime = tmp;
		else
		if (!strcmp(extraline, "system"))
			sysTime = tmp;
		else
		if (!strcmp(extraline, "idle"))
			idleTime = tmp;
		else
		if (!strcmp(extraline, "wait"))
			iowaitTime = tmp;
	}
	
	pclose(fp1);
#else
  fp1 = fopen("/proc/stat", "r");
  if (fp1 == NULL)
    return;

  while (fgets(line, MAX_STRING_LEN, fp1)) {
      if (strstr(line, "cpu") == line)
	break;
  }
  
  if (line == 0) {
    fclose(fp1);
    return;
  }

  sscanf(line, "%s %lf %lf %lf %lf %lf %lf %lf %lf %lf", s1, &usrTime, &niceTime, &sysTime, &idleTime, &iowaitTime, &irqTime, &softirqTime, &stealTime, &guestTime);

  fclose(fp1);
#endif

  indU       = getVectIndex("cpu_usr"    , apm.sysMonitorParams, apm.nSysMonitorParams);
  indS       = getVectIndex("cpu_sys"    , apm.sysMonitorParams, apm.nSysMonitorParams);
  indN       = getVectIndex("cpu_nice"   , apm.sysMonitorParams, apm.nSysMonitorParams);
  indI       = getVectIndex("cpu_idle"   , apm.sysMonitorParams, apm.nSysMonitorParams);
  indIOWAIT  = getVectIndex("cpu_iowait" , apm.sysMonitorParams, apm.nSysMonitorParams);
  indIRQ     = getVectIndex("cpu_irq"    , apm.sysMonitorParams, apm.nSysMonitorParams);
  indSOFTIRQ = getVectIndex("cpu_softirq", apm.sysMonitorParams, apm.nSysMonitorParams);
  indSTEAL   = getVectIndex("cpu_steal"  , apm.sysMonitorParams, apm.nSysMonitorParams);
  indGUEST   = getVectIndex("cpu_guest"  , apm.sysMonitorParams, apm.nSysMonitorParams);

  if (idleTime < apm.lastSysVals[indI]) {
    apm.lastSysVals[indU] = usrTime;
    apm.lastSysVals[indS] = sysTime;
    apm.lastSysVals[indI] = idleTime;
    apm.lastSysVals[indIOWAIT] = iowaitTime;
#ifndef __SUNOS
    apm.lastSysVals[indN] = niceTime;
    apm.lastSysVals[indIRQ] = irqTime;
    apm.lastSysVals[indSOFTIRQ] = softirqTime;
    apm.lastSysVals[indSTEAL] = stealTime;
    apm.lastSysVals[indGUEST] = guestTime;
#endif

    return;
  }

  if (numCPUs == 0)
    return;

  //printf("### crtTime %ld lastSysInfo %ld\n", crtTime, apm.lastSysInfoSend);  
  if (crtTime <= apm.lastSysInfoSend)
    return;
  
  totalTime = (usrTime - apm.lastSysVals[indU]) + 
    (sysTime -apm.lastSysVals[indS]) +
    (idleTime - apm.lastSysVals[indI]) +
    (iowaitTime - apm.lastSysVals[indIOWAIT])
#ifndef __SUNOS
    + (niceTime - apm.lastSysVals[indN])
    + (irqTime - apm.lastSysVals[indIRQ])
    + (softirqTime - apm.lastSysVals[indSOFTIRQ])
    + (stealTime - apm.lastSysVals[indSTEAL])
    + (guestTime - apm.lastSysVals[indGUEST])
#endif
    ;

  cpuUsr     = 100 * (usrTime     - apm.lastSysVals[indU]      ) / totalTime;
  cpuSys     = 100 * (sysTime     - apm.lastSysVals[indS]      ) / totalTime;
  cpuIdle    = 100 * (idleTime    - apm.lastSysVals[indI]      ) / totalTime;
  cpuIOWait  = 100 * (iowaitTime  - apm.lastSysVals[indIOWAIT] ) / totalTime;

#ifndef __SUNOS
  cpuNice    = 100 * (niceTime    - apm.lastSysVals[indN]      ) / totalTime;
  cpuIRQ     = 100 * (irqTime     - apm.lastSysVals[indIRQ]    ) / totalTime;
  cpuSoftIRQ = 100 * (softirqTime - apm.lastSysVals[indSOFTIRQ]) / totalTime;
  cpuSteal   = 100 * (stealTime   - apm.lastSysVals[indSTEAL]  ) / totalTime;
  cpuGuest   = 100 * (guestTime   - apm.lastSysVals[indGUEST]  ) / totalTime;
#else
  cpuNice = cpuIRQ = cpuSoftIRQ = cpuSteal = cpuGuest = RET_ERROR;
#endif

  cpuUsage   = 100 * (totalTime - (idleTime - apm.lastSysVals[indI])) / totalTime;

  apm.lastSysVals[indU]       = usrTime;
  apm.lastSysVals[indI]       = idleTime;
  apm.lastSysVals[indS]       = sysTime;
  apm.lastSysVals[indIOWAIT]  = iowaitTime;
#ifndef __SUNOS
  apm.lastSysVals[indN]       = niceTime;
  apm.lastSysVals[indIRQ]     = irqTime;
  apm.lastSysVals[indSOFTIRQ] = softirqTime;
  apm.lastSysVals[indSTEAL]   = stealTime;
  apm.lastSysVals[indGUEST]   = guestTime;
#endif

#endif
}

void ProcUtils::getSwapPages(ApMon& apm, double& pagesIn, 
			       double& pagesOut, double& swapIn, 
			     double& swapOut) {
#ifndef WIN32
  FILE *fp1;
  char line[MAX_STRING_LEN];
  char s1[20];
  bool foundPages, foundSwap;
  double p_in, p_out, s_in, s_out;
  int ind1, ind2;

  time_t crtTime = time(NULL);

  foundPages = foundSwap = false;

#ifdef __SUNOS
	double tmp = 0;

	char w1[MAX_STRING_LEN];
	char w2[MAX_STRING_LEN];
	char w3[MAX_STRING_LEN];

	swapIn = swapOut = pagesIn = pagesOut = 0;

	fp1 = popen("vmstat -s", "r");
	
	if (fp1==NULL){
	    return;
	}

	while (fgets(line, MAX_STRING_LEN, fp1)){
		sscanf(line, "%lf %s %s %s", &tmp, w1, w2, w3);

		int index = -1;

		if (strcmp(w1, "pages")==0){
			if (strcmp(w2, "swapped")==0){
				if (strcmp(w3, "in")==0){
					index = getVectIndex("swap_in", apm.sysMonitorParams, apm.nSysMonitorParams);
					foundSwap = true;
					if (tmp >= apm.lastSysVals[index])
						swapIn = (tmp - apm.lastSysVals[index]) / (crtTime - apm.lastSysInfoSend);
				}
				else
				if (strcmp(w3, "out")==0){
					foundSwap = true;
					index = getVectIndex("swap_out", apm.sysMonitorParams, apm.nSysMonitorParams);
					if (tmp >= apm.lastSysVals[index])
						swapOut = (tmp - apm.lastSysVals[index]) / (crtTime - apm.lastSysInfoSend);
				}
			}
			else
			if (strcmp(w2, "paged")==0){
				if (strcmp(w3, "in")==0){
					foundPages = true;
					index = getVectIndex("pages_in", apm.sysMonitorParams, apm.nSysMonitorParams);
					if (tmp >= apm.lastSysVals[index])
						pagesIn = (tmp - apm.lastSysVals[index]) / (crtTime - apm.lastSysInfoSend);
				}
				else
				if (strcmp(w3, "out")==0){
					foundPages = true;
					index = getVectIndex("pages_out", apm.sysMonitorParams, apm.nSysMonitorParams);
					if (tmp >= apm.lastSysVals[index])
						pagesOut = (tmp - apm.lastSysVals[index]) / (crtTime - apm.lastSysInfoSend);

				}
			}
		}

		if (index>=0)
			apm.lastSysVals[index] = tmp;
	}
	
	pclose(fp1);
#else
  fp1 = fopen("/proc/stat", "r");
  if (fp1 == NULL)
    return;

  if (crtTime <= apm.lastSysInfoSend)
    return;

  while (fgets(line, MAX_STRING_LEN, fp1)) {
    if (strstr(line, "page") == line) {
      foundPages = true;
      sscanf(line, "%s %lf %lf ", s1, &p_in, &p_out);

      ind1 = getVectIndex("pages_in", apm.sysMonitorParams, apm.nSysMonitorParams);
      ind2 = getVectIndex("pages_out", apm.sysMonitorParams, apm.nSysMonitorParams);
      if (p_in < apm.lastSysVals[ind1] || p_out < apm.lastSysVals[ind2]) {
	apm.lastSysVals[ind1] = p_in;
	apm.lastSysVals[ind2] = p_out;
	return;
      }
      pagesIn = (p_in - apm.lastSysVals[ind1]) / (crtTime - apm.lastSysInfoSend);
      pagesOut = (p_out - apm.lastSysVals[ind2]) / (crtTime - apm.lastSysInfoSend);
      apm.lastSysVals[ind1] = p_in;
      apm.lastSysVals[ind2] = p_out;

    }

    if (strstr(line, "swap") == line) {
      foundSwap = true;
      sscanf(line, "%s %lf %lf ", s1, &s_in, &s_out);

      ind1 = getVectIndex("swap_in", apm.sysMonitorParams, apm.nSysMonitorParams);
      ind2 = getVectIndex("swap_out", apm.sysMonitorParams, apm.nSysMonitorParams);
      if (s_in < apm.lastSysVals[ind1] || s_out < apm.lastSysVals[ind2]) {
	apm.lastSysVals[ind1] = s_in;
	apm.lastSysVals[ind2] = s_out;
	return;
      }
      swapIn = (s_in - apm.lastSysVals[ind1]) / (crtTime - apm.lastSysInfoSend);
      swapOut = (s_out - apm.lastSysVals[ind2]) / (crtTime - apm.lastSysInfoSend);
      apm.lastSysVals[ind1] = s_in;
      apm.lastSysVals[ind2] = s_out;

    }
  }
  
  fclose(fp1);

#endif

  if (!foundPages || !foundSwap) {
    return;
  }
#endif
}

void ProcUtils::getLoad(double &load1, double &load5, 
	   double &load15, double &processes) {
#ifndef WIN32
  double v1, v5, v15, activeProcs, totalProcs;
  FILE *fp1;

#ifdef __SUNOS
	char line[MAX_STRING_LEN];

	fp1 = popen("uptime", "r");

	fgets(line, MAX_STRING_LEN, fp1);

	char* ptr = strtok(line, " ,");

	while (ptr){
		load1 = load5;
		load5 = load15;
		load15 = atof(ptr);

		ptr = strtok(NULL, " ,");
	}
	
	// uptime on SunOS doesn't contain the number of processes on the system
	processes = RET_ERROR;
	
	pclose(fp1);
#else
  fp1 = fopen("/proc/loadavg", "r");
  if (fp1 == NULL)
    return;

  int retScan = fscanf(fp1, "%lf %lf %lf", &v1, &v5, &v15);
  if (retScan != 3) {
    fclose(fp1);
    return;
  }

  load1 = v1;
  load5 = v5;
  load15 = v15;

  retScan = fscanf(fp1, "%lf/%lf", &activeProcs, &totalProcs);
  if (retScan != 2) {
    fclose(fp1);
    return;
  }    

  processes = totalProcs;

  fclose(fp1);
#endif

#endif
}

void ProcUtils::getProcesses(double& processes, double states[]) {
#if defined(WIN32) || defined(__SUNOS)
  processes = RET_ERROR;

  for (int i = 0; i < NLETTERS; i++)
    states[i] = RET_ERROR;
#else
  char *argv[4];
  char psstat_f[40];
  pid_t mypid = getpid();
  pid_t cpid;
  int status;
  char ch, buf[100];
  FILE *pf;

  snprintf(psstat_f, 39, "/tmp/apmon_psstat%ld", (long)mypid);

 switch (cpid = fork()) {
  case -1:
    return;
  case 0:
    argv[0] = (char *)"/bin/sh"; argv[1] = (char *)"-c";
    snprintf(buf, 99, "ps -A -o state > %s", psstat_f);
    argv[2] = buf;
    argv[3] = 0;
    execv("/bin/sh", argv);
    exit(RET_ERROR);
  default:
    if (waitpid(cpid, &status, 0) == -1) {
      snprintf(buf, 99, "[ getProcesses() ] The number of processes could not be determined");
      return;
    }
  }

  pf = fopen(psstat_f, "rt");
  if (pf == NULL) {
    unlink(psstat_f);
    snprintf(buf, 99, "[ getProcesses() ] The number of processes could not be determined");
    return;
  } 

  processes = 0;
  // the states table keeps an entry for each alphabet letter, for efficient 
  // indexing
  for (int i = 0; i < NLETTERS; i++)
    states[i] = 0.0;
  while (fgets(buf, 10, pf) != 0) {
    ch = buf[0];
    states[ch - 65]++;
    processes++;
  }

  fclose(pf);   
  unlink(psstat_f);
#endif
}

void ProcUtils::getSysMem(double &totalMem, double &totalSwap) {
#ifndef WIN32

char s1[20], line[MAX_STRING_LEN];
bool memFound = false, swapFound = false;
double valMem, valSwap;
FILE *fp1;

#ifdef __SUNOS
	  char tempbuf[MAX_STRING_LEN];
	  char* pbuf = tempbuf;

	  fp1 = popen("prtconf", "r");
	  while (fgets(line, MAX_STRING_LEN, fp1)){
		  if (!memFound && strstr(line, "Memory size")==line){	// Memory size: 768 Megabytes
			  memFound = true;

			  strtok_r(line, " \t\n", &pbuf);
			  strtok_r(NULL, " \t\n", &pbuf);
			  valMem = atof(strtok_r(NULL, " \t\n", &pbuf));

			  switch (strtok_r(NULL, " \t\n", &pbuf)[0]){
				  case 'T':
				  case 't': valMem *= 1024;
				  case 'G':
				  case 'g': valMem *= 1024;
				  case 'M':
				  case 'm': valMem *= 1024;
				  case 'K':
				  case 'k': valMem *= 1024;
			  }
		  }
	  }

	  pclose(fp1);

	  fp1 = popen("swap -l", "r");

	  if (fgets(line, MAX_STRING_LEN, fp1)){ //swapfile             dev    swaplo   blocks     free
		  swapFound = true;
	  }

	  while (fgets(line, MAX_STRING_LEN, fp1)){
		  ///dev/zvol/dsk/rpool/swap 182,1         8  1048568  1046496

		  strtok_r(line, " \t\n", &pbuf);
		  strtok_r(NULL, " \t\n", &pbuf);
		  strtok_r(NULL, " \t\n", &pbuf);

		  double swapBlocks = atof(strtok_r(NULL, " \t\n", &pbuf));
		  
		  valSwap += swapBlocks * 512;	// 512-byte blocks
	  }
	  
	  pclose(fp1);
#else
  fp1 = fopen("/proc/meminfo", "r");
  if (fp1 == NULL)
    return;

  while (fgets(line, MAX_STRING_LEN, fp1)) {
    if (strstr(line, "MemTotal:") == line) {
      sscanf(line, "%s %lf", s1, &valMem);
      memFound = true;
      continue;
    }

    if (strstr(line, "SwapTotal:") == line) {
      sscanf(line, "%s %lf", s1, &valSwap);
      swapFound = true;
      continue;
    }
    
  }

  fclose(fp1); 
#endif

  if (!memFound || !swapFound)
    return;

  totalMem = valMem;
  totalSwap = valSwap;

#endif
}

void ProcUtils::getMemUsed(double &usedMem, double& freeMem, 
				  double &usedSwap, double& freeSwap) {
#ifndef WIN32

  double mFree = 0, mTotal = 0, sFree = 0, sTotal = 0;
  char s1[20], line[MAX_STRING_LEN];
  bool mFreeFound = false, mTotalFound = false;
  bool sFreeFound = false, sTotalFound = false;
  FILE *fp1;

#ifdef __SUNOS
  fp1 = popen("vmstat", "r");

  fgets(line, MAX_STRING_LEN, fp1); // kthr      memory            page            disk          faults      cpu
  fgets(line, MAX_STRING_LEN, fp1); // r b w   swap  free  re  mf pi po fr de sr cd f0 s0 --   in   sy   cs us sy id

  if (fgets(line, MAX_STRING_LEN, fp1)){ // 0 0 0 494204 64652  13  79  0  0  1  0 53  3 -0  1  0  313  673  275  2 10 88
	  mFreeFound = true;
  }
  else{
	usedMem = freeMem = usedSwap = freeSwap = -1;
	pclose(fp1);
	return;
  }

  char tempbuf[MAX_STRING_LEN];
  char* pbuf = tempbuf;

  strtok_r(line, " \t\n", &pbuf);
  strtok_r(NULL, " \t\n", &pbuf);
  strtok_r(NULL, " \t\n", &pbuf);
  strtok_r(NULL, " \t\n", &pbuf);
  mFree  = atof(strtok_r(NULL, " \t\n", &pbuf)) * 1024;

  pclose(fp1);

  fp1 = popen("prtconf", "r");
  while (fgets(line, MAX_STRING_LEN, fp1)){
	  if (!mTotalFound && strstr(line, "Memory size")==line){	// Memory size: 768 Megabytes
		  mTotalFound = true;

		  strtok_r(line, " \t\n", &pbuf);
		  strtok_r(NULL, " \t\n", &pbuf);
		  mTotal = atof(strtok_r(NULL, " \t\n", &pbuf));

		  switch (strtok_r(NULL, " \t\n", &pbuf)[0]){
			  case 'T':
			  case 't': mTotal *= 1024;
			  case 'G':
			  case 'g': mTotal *= 1024;
			  case 'M':
			  case 'm': mTotal *= 1024;
			  case 'K':
			  case 'k': mTotal *= 1024;
		  }
	  }
  }

  pclose(fp1);

  fp1 = popen("swap -l", "r");

  if (fgets(line, MAX_STRING_LEN, fp1)){ //swapfile             dev    swaplo   blocks     free
	  sTotalFound = sFreeFound = true;
  }

  while (fgets(line, MAX_STRING_LEN, fp1)){
	  ///dev/zvol/dsk/rpool/swap 182,1         8  1048568  1046496

	  strtok_r(line, " \t\n", &pbuf);
	  strtok_r(NULL, " \t\n", &pbuf);
	  strtok_r(NULL, " \t\n", &pbuf);

	  double swapBlocks = atof(strtok_r(NULL, " \t\n", &pbuf));
	  double freeBlocks = atof(strtok_r(NULL, " \t\n", &pbuf));
	  
	  sTotal += swapBlocks * 512;
	  sFree += freeBlocks * 512;
  }
  
  pclose(fp1);
#else

  fp1 = fopen("/proc/meminfo", "r");
  if (fp1 == NULL)
    return;

  while (fgets(line, MAX_STRING_LEN, fp1)) {
    if (strstr(line, "MemTotal:") == line) {
      sscanf(line, "%s %lf", s1, &mTotal);
      mTotalFound = true;
      continue;
    }

    if (strstr(line, "MemFree:") == line) {
      sscanf(line, "%s %lf", s1, &mFree);
      mFreeFound = true;
      continue;
    }

    if (strstr(line, "SwapTotal:") == line) {
      sscanf(line, "%s %lf", s1, &sTotal);
      sTotalFound = true;
      continue;
    }

    if (strstr(line, "SwapFree:") == line) {
      sscanf(line, "%s %lf", s1, &sFree);
      sFreeFound = true;
      continue;
    }
    
  }

  fclose(fp1); 
#endif

  if (!mFreeFound || !mTotalFound || !sFreeFound || !sTotalFound)
    return;

  if (mFree > mTotal)
    mTotal = mFree;

  if (sFree > sTotal)
    sTotal = sFree;

  usedMem = (mTotal - mFree) / 1024;
  freeMem = mFree / 1024;
  usedSwap = (sTotal - sFree) / 1024;
  freeSwap = sFree / 1024;

#endif
}

void ProcUtils::getNetworkInterfaces(int &nInterfaces, char names[][20]) {

nInterfaces = 0;

#ifndef WIN32


#ifdef __SUNOS
	// on SunOS the TCP stats are global, not per interface, so we account everything under a virtual "eth0" device
	//nInterfaces = 1;
	//strcpy(names[0], "eth0");
	
	int sockfd, i;
	struct lifnum ln;
	struct lifconf lc;

	sockfd=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);

	ln.lifn_family=AF_INET;
	ln.lifn_flags=ln.lifn_count=0; 

	if (ioctl(sockfd,SIOCGLIFNUM,&ln) == -1){
	    perror("Failed to get number of interfaces");
	    close(sockfd);
	    return;
	}
	
	lc.lifc_family=AF_INET;
	lc.lifc_flags=0; 
	lc.lifc_len=ln.lifn_count * sizeof(struct lifreq);
	
	lc.lifc_req=(struct lifreq *) malloc(lc.lifc_len);
	
	if (ioctl(sockfd,SIOCGLIFCONF,&lc) == -1){
	    perror("Failed to enumerate interfaces");
	    close(sockfd);
	    free(lc.lifc_req);
	    return;
	}
	
	for (i=0; i<ln.lifn_count && i<100; i++){
	    strncpy(names[nInterfaces], lc.lifc_req[i].lifr_name, 19);
	    nInterfaces ++;
	}
	
	close(sockfd);
	free(lc.lifc_req);
	
#else
  char line[MAX_STRING_LEN], *tmp;
//  char buf[MAX_STRING_LEN];
//  char *pbuf = buf;
  FILE *fp1;

  fp1 = fopen("/proc/net/dev", "r");
  if (fp1 == NULL)
    return;

  while (fgets(line, MAX_STRING_LEN, fp1) && nInterfaces<100) {
    if (strchr(line, ':') == NULL)
      continue;

    tmp = strtok/*_r*/(line, " :");//, &pbuf);

    if (strcmp(tmp, "lo") == 0)
      continue;
    
    strncpy(names[nInterfaces], tmp, 19);
    nInterfaces++;
  }
    
  fclose(fp1);
#endif

#endif
}

void ProcUtils::getNetInfo(ApMon& apm, double **vNetIn, 
				  double **vNetOut, double **vNetErrs) {
#ifndef WIN32
  double *netIn, *netOut, *netErrs, bytesReceived = 0, bytesSent = 0;
  int errs = 0;
  char line[MAX_STRING_LEN], msg[MAX_STRING_LEN];
//  char buf[MAX_STRING_LEN];
//  char *pbuf = buf;
  char *tmp, *tok;
  double bootTime = 0;
  FILE *fp1;
  time_t crtTime = time(NULL);
  int ind, i;

  if (apm.lastSysInfoSend == 0) {
    try {
      bootTime = getBootTime();
    } catch (procutils_error& err) {
      logger(WARNING, "[ getNetInfo() ] Error obtaining boot time. The first system monitoring datagram will contain incorrect data.");
      bootTime = 0;
    }
  }

  if (crtTime <= apm.lastSysInfoSend)
    return;

  netIn = (double *)malloc(apm.nInterfaces * sizeof(double));
  netOut = (double *)malloc(apm.nInterfaces * sizeof(double));
  netErrs = (double *)malloc(apm.nInterfaces * sizeof(double));

#ifdef __SUNOS
    fp1 = popen("netstat -P tcp -s", "r");

    // find out the first interface that is not "lo"
    // we cannot bind traffic per interface, so we put everything on the first real interface
    ind = -1;

    for (i=0; i<apm.nInterfaces; i++){
	netIn[i] = RET_ERROR;
	netOut[i] = RET_ERROR;
	netErrs[i] = RET_ERROR;

	if ( strncmp(apm.interfaceNames[i], "lo", 2) == 0 )
	    continue;
	
	if (ind<0)
	    ind = i;
    }
    
    if (ind<0){
	pclose(fp1);
	return;
    }

	while (fgets(line, MAX_STRING_LEN, fp1)){
		char* ptr = strtok(line, " =\t");

		while (ptr){
			if (strstr(ptr, "tcpIn")==ptr && strstr(ptr, "Bytes")!=NULL){
				ptr = strtok(NULL, " =\t");

				bytesReceived += atof(ptr);
			}
			else
				if (strcmp(ptr, "tcpOutDataBytes")==0 || strcmp(ptr, "tcpRetransBytes")==0){
					ptr = strtok(NULL, " =\t");

					bytesSent += atof(ptr);
				}

			ptr = strtok(NULL, " =\t");
		}
	}

	pclose(fp1);
	
	errs = -1;
#else
  fp1 = fopen("/proc/net/dev", "r");
  if (fp1 == NULL)
    return;

  while (fgets(line, MAX_STRING_LEN, fp1)) {
    if (strchr(line, ':') == NULL)
      continue;
    tmp = strtok/*_r*/(line, " :");//, &pbuf);
    
    /* the loopback interface is not considered */
    if (strcmp(tmp, "lo") == 0)
      continue;

    /* find the index of the interface in the vector */
    ind = -1;
    for (i = 0; i < apm.nInterfaces; i++)
      if (strcmp(apm.interfaceNames[i], tmp) == 0) {
	ind = i;
	break;
      }
    
    if (ind < 0) {
      fclose(fp1);
      free(netIn); free(netOut); free(netErrs);
      snprintf(msg, MAX_STRING_LEN-1, "[ getNetInfo() ] Could not find interface %s in /proc/net/dev", tmp);  
      return;
    }

    /* parse the rest of the line */
    tok = strtok/*_r*/(NULL, " ");//, &pbuf);
    bytesReceived = atof(tok); /* bytes received */
    tok = strtok/*_r*/(NULL, " ");//, &pbuf); /* packets received */
    tok = strtok/*_r*/(NULL, " ");//, &pbuf); /* input errors */
    errs = atoi(tok);
    /* some parameters that we are not monitoring */
    for (i = 1; i <= 5; i++)
      tok = strtok/*_r*/(NULL, " ");//, &pbuf);

    tok = strtok/*_r*/(NULL, " ");//, &pbuf); /* bytes transmitted */
    bytesSent = atof(tok);
    tok = strtok/*_r*/(NULL, " ");//, &pbuf); /* packets transmitted */
    tok = strtok/*_r*/(NULL, " ");//, &pbuf); /* output errors */
    errs += atoi(tok);
#endif

    //printf("### bytesReceived %lf lastRecv %lf\n", bytesReceived, 
    // apm.lastBytesReceived[ind]); 
    if (bytesReceived < apm.lastBytesReceived[ind] || bytesSent < 
	apm.lastBytesSent[ind] || errs < apm.lastNetErrs[ind]) {
      apm.lastBytesReceived[ind] = bytesReceived;
      apm.lastBytesSent[ind] = bytesSent;
      apm.lastNetErrs[ind] = errs;
      fclose(fp1);
      free(netIn); free(netOut); free(netErrs);
      return;
    }

    if (apm.lastSysInfoSend == 0) {
      netIn[ind] = bytesReceived/(crtTime - bootTime);
      netOut[ind] = bytesSent/(crtTime - bootTime);
      netErrs[ind] = errs;
    }
    else {
      netIn[ind] = (bytesReceived - apm.lastBytesReceived[ind]) / (crtTime -
						     apm.lastSysInfoSend);
      netIn[ind] /= 1024; /* netIn is measured in KBps */
      netOut[ind] = (bytesSent - apm.lastBytesSent[ind]) / (crtTime - 
						     apm.lastSysInfoSend);
      netOut[ind] /= 1024; /* netOut is measured in KBps */
      /* for network errors give the total number */
      netErrs[ind] = errs; // - apm.lastNetErrs[ind];
    }

    apm.lastBytesReceived[ind] = bytesReceived;
    apm.lastBytesSent[ind] = bytesSent;
    apm.lastNetErrs[ind] = errs;

#ifndef __SUNOS
  }

  fclose(fp1);
#endif
    
  *vNetIn = netIn;
  *vNetOut = netOut;
  *vNetErrs = netErrs;
#endif
 }

int ProcUtils::getNumCPUs() {
#ifdef WIN32
	return 0;
#else
  int numCPUs = 0;
  char line[MAX_STRING_LEN];

  FILE *fp;

#ifdef __SUNOS
  fp = popen("psrinfo -p", "r");

  fgets(line, MAX_STRING_LEN, fp);

  pclose(fp);

  numCPUs = atoi(line);
#else
  fp = fopen("/proc/stat", "r");

  if (fp == NULL)
    return -1;

  while(fgets(line, MAX_STRING_LEN, fp)) {
    if (strstr(line, "cpu") == line && isdigit(line[3]))
      numCPUs++;
  }

  fclose(fp);

#endif

  return numCPUs;
#endif
}

void ProcUtils::getCPUInfo(ApMon& apm) {
#ifndef WIN32
#ifndef __SUNOS
  double freq = 0;
  char line[MAX_STRING_LEN], s1[100], s2[100], s3[100];
//  char buf[MAX_STRING_LEN];
//  char *pbuf = buf;
  char *tmp, *tmp_trim;
  bool freqFound = false, bogomipsFound = false;

  FILE *fp = fopen("/proc/cpuinfo", "r");
  if (fp == NULL)
    return;

  while (fgets(line, MAX_STRING_LEN, fp)) {
    if (strstr(line, "cpu MHz") == line) {
      sscanf(line, "%s %s %s %lf", s1, s2, s3, &freq);
      apm.currentGenVals[GEN_CPU_MHZ] = freq;
      freqFound = true;
      continue;
    }

    if (strstr(line, "bogomips") == line) {
      sscanf(line, "%s %s %lf", s1, s2, &(apm.currentGenVals[GEN_BOGOMIPS]));
      bogomipsFound = true;
      continue;
    }

    if (strstr(line, "vendor_id") == line) {
      tmp = strtok/*_r*/(line, ":");//, &pbuf);
      /* take everything that's after the ":" */
      tmp = strtok/*_r*/(NULL, ":");//, &pbuf);
      tmp_trim = trimString(tmp);
      strncpy(apm.cpuVendor, tmp_trim, 99);
      free(tmp_trim);
      continue;
    } 

    if (strstr(line, "cpu family") == line) {
      tmp = strtok/*_r*/(line, ":");//, &pbuf);
      tmp = strtok/*_r*/(NULL, ":");//, &pbuf);
      tmp_trim = trimString(tmp);
      strncpy(apm.cpuFamily, tmp_trim, 99);
      free(tmp_trim);
      continue;
    }

    if (strstr(line, "model") == line && strstr(line, "model name") != line) {
      tmp = strtok/*_r*/(line, ":");//, &pbuf);
      tmp = strtok/*_r*/(NULL, ":");//, &pbuf);
      tmp_trim = trimString(tmp);
      strncpy(apm.cpuModel, tmp_trim, 99);
      free(tmp_trim);
      continue;
    }  

    if (strstr(line, "model name") == line) {
      tmp = strtok/*_r*/(line, ":");//, &pbuf);
      /* take everything that's after the ":" */
      tmp = strtok/*_r*/(NULL, ":");//, &pbuf);
      tmp_trim = trimString(tmp);
      strncpy(apm.cpuModelName, tmp_trim, 199);
      free(tmp_trim);
      continue;
    } 
  }

  fclose(fp);
  if (!freqFound || !bogomipsFound)
    return;
#endif
#endif
}

/**
 * Returns the system boot time in milliseconds since the Epoch.
 */
long ProcUtils::getBootTime() {
#if defined(WIN32) || defined(__SUNOS)
	return 0;
#else
  char line[MAX_STRING_LEN], s[MAX_STRING_LEN];
  long btime = 0;
  FILE *fp = fopen("/proc/stat", "rt");
  if (fp == NULL)
    return 0;

  while (fgets(line, MAX_STRING_LEN, fp)) {
    if (strstr(line, "btime") == line) {
      sscanf(line, "%s %ld", s, &btime);
      break;
    }
  }  
  fclose(fp);

  return btime;
#endif
}


double ProcUtils::getUpTime() {
#ifdef WIN32
	return 0;
#else
  double uptime = 0;

#ifdef __SUNOS
  clock_t ticks;
  struct tms tmp;
  ticks=times(&tmp);
  uptime = ticks / CLK_TCK;	// in seconds
#else
  FILE *fp = fopen("/proc/uptime", "rt");
  if (fp == NULL) {
    return -1;
  }

  int retScan = fscanf(fp, "%lf", &uptime);
  
  if (retScan != 1) {
    return -1;
  }

  fclose(fp);
#endif
  if (uptime <= 0) {
    return -1;
  }

  return uptime / (24 * 3600);
#endif
}

int ProcUtils::countOpenFiles(long pid) {
#if defined(WIN32) || defined(__SUNOS)
	return 0;
#else
  char dirname[50];
  char msg[MAX_STRING_LEN];
  DIR *dir;
  struct dirent *dir_entry;
  int cnt = 0;
 
  /* in /proc/<pid>/fd/ there is an entry for each opened file descriptor */
  snprintf(dirname, 49, "/proc/%ld/fd", pid);
  dir = opendir(dirname);
  if (dir == NULL) {
    snprintf(msg, MAX_STRING_LEN-1, "[ countOpenFiles() ] Could not open %s", dirname); 
    return -1;
  }

  /* count the files from /proc/<pid>/fd/ */
  while ((dir_entry = readdir(dir)) != NULL) {
    cnt++;
  }
  
  closedir(dir);

  /* don't take into account . and .. */
  cnt -= 2;
  if (cnt < 0) {
    snprintf(msg, MAX_STRING_LEN-1, "[ countOpenFiles() ] Directory %s has less than 2 entries",  dirname);
    logger(FINE, msg);
    cnt = 0;
  }

  return cnt;
#endif
}

void ProcUtils::getNetstatInfo(ApMon& apm, double nsockets[], 
				      double tcp_states[]) {

  // the states table keeps an entry for each alphabet letter, for efficient 
  // indexing
  int i;
  for (i = 0; i < 4; i++)
    nsockets[i] = 0.0;
  for (i = 0; i < N_TCP_STATES; i++)
    tcp_states[i] = 0.0;

#ifndef WIN32
  char *argv[4];
  char netstat_f[40];
  pid_t mypid = getpid();
  pid_t cpid;
  int status, idx;
  char ch, buf[200], msg[100];
  char *pbuf = buf, *tmp, *tmp2;
  FILE *pf;

  snprintf(netstat_f, 39, "/tmp/apmon_netstat%ld", (long)mypid);

  switch (cpid = fork()) {
  case -1:
    return;
  case 0:
    argv[0] = (char *)"/bin/sh"; argv[1] = (char *)"-c";
    snprintf(buf, 199, "netstat -an > %s", netstat_f);
    argv[2] = buf;
    argv[3] = 0;
    execv("/bin/sh", argv);
    exit(RET_ERROR);
  default:
    if (waitpid(cpid, &status, 0) == -1) {
      snprintf(msg, 99, "[ getNetstatInfo() ] The netstat information could not be collected");
      return;
    }
  }

  pf = fopen(netstat_f, "rt");
  if (pf == NULL) {
    unlink(netstat_f);
    snprintf(msg, 99, "[ getNetstatInfo() ] The netstat information could not be collected");
    return;
  } 

#ifdef __SUNOS
  bool bTCP = false;
  bool bUDP = false;
  bool bUnix = false;

  while (fgets(buf, 200, pf) > 0) {

	  if (strlen(buf)==0){
		  bTCP = bUDP = bUnix = false;
	  
		  continue;
	  }


	  tmp = strtok_r(buf, " \t\n", &pbuf);

	  if (tmp==NULL){
		  bTCP = bUDP = bUnix = false;
	
		  continue;
	  }

	  if (strncmp(tmp, "TCP", 3)==0){
		  bTCP=true;
		  fgets(buf, 200, pf);	// skip
		  fgets(buf, 200, pf);	// headers

		  continue;
	  }

	  if (strncmp(tmp, "UDP", 3)==0){
		  bUDP=true;
		  fgets(buf, 200, pf);	// skip
		  fgets(buf, 200, pf);	// headers

		  continue;
	  }

	  if (strncmp(tmp, "Active", 6)==0){
		  bUnix = true;
		  fgets(buf, 200, pf);	// skip header

		  continue;
	  }
	  
	  if (!bTCP && !bUDP && !bUnix){
		  // then what is this? ignore
		  continue;
	  }

	  if (bTCP){
		  nsockets[SOCK_TCP]++;

		  char* last = tmp;

		  while ( (tmp=strtok_r(NULL, " \t\n", &pbuf)) ){
			  last = tmp;
		  }
		  
		  idx = getVectIndex(last, apm.socketStatesMapTCP, N_TCP_STATES);
		  
		  if (idx >= 0) {
			  tcp_states[idx]++;
		  }
	  }

	  if (bUDP){
		  nsockets[SOCK_UDP]++;
	  }

	  if (bUnix){
		  nsockets[SOCK_UNIX]++;
	  }
  }
  
  nsockets[SOCK_ICM] = RET_ERROR;
#else
  while (fgets(buf, 200, pf) != 0) {
    tmp = strtok_r(buf, " \t\n", &pbuf);
    if (strstr(tmp, "tcp") == tmp) {
      nsockets[SOCK_TCP]++;

      /* go to the "State" field */
      for (i = 1; i <= 5; i++)
	tmp2 = strtok_r(NULL, " \t\n", &pbuf);

      idx = getVectIndex(tmp2, apm.socketStatesMapTCP, N_TCP_STATES);
      if (idx >= 0) {
	tcp_states[idx]++;
      } else {
	snprintf(msg, 99, "[ getNestatInfo() ] Invalid socket state: %s q", tmp2);
	logger(WARNING, msg);
      }
    } else {
      if (strstr(tmp, "udp") == tmp) {
	nsockets[SOCK_UDP]++;
      } else {
	if (strstr(tmp, "unix") == tmp)
	  nsockets[SOCK_UNIX]++;
	else if (strstr(tmp, "icm") == tmp)
	  nsockets[SOCK_ICM]++;
      }
    }
  }
#endif

  fclose(pf);   
  unlink(netstat_f);
#endif
}
