/**
 * \file ApMon.cpp
 * This file contains the implementations of the methods from the ApMon class.
 */

/*
 * ApMon - Application Monitoring Tool
 * Version: 2.2.8
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
#include "utils.h"
#include "proc_utils.h"
#include "monitor_utils.h"

using namespace apmon_utils;
using namespace apmon_mon_utils;

#define RECHECK_CONF 0
#define SYS_INFO_SEND 1
#define JOB_INFO_SEND 2

char boolStrings[][10] = {"false", "true"};

//========= Implementations of the functions ===================

ApMon::ApMon(char *initsource) {

  if (initsource == NULL)
    return;

  if (strstr(initsource, "http://") == initsource) {
    char *destList[1];
    destList[0] = initsource;
    constructFromList(1, destList);
  } else {
    nInitSources = 1;
    initType = FILE_INIT;
    initSources = (char **)malloc(nInitSources * sizeof(char *));
    if (initSources == NULL)
      return;
    
    initSources[0] = strdup(initsource);
    initMonitoring();

    initialize(initsource, true);
  }
}

void ApMon::initialize(char *filename, bool firstTime) {

  char *destAddresses[MAX_N_DESTINATIONS];
  int destPorts[MAX_N_DESTINATIONS];
  char *destPasswds[MAX_N_DESTINATIONS];
  int nDest = 0, i; 
  ConfURLs confURLs;
  
  confURLs.nConfURLs = 0;

  try {
    loadFile(filename, &nDest, destAddresses, destPorts, destPasswds);

    arrayInit(nDest, destAddresses, destPorts, destPasswds, firstTime);
  } catch (runtime_error& err) {
    if (firstTime)
      return;
    else {
      logger(WARNING, err.what());
      logger(WARNING, "Error reloading the configuration. Keeping the previous one.");
      return;
    }
  }

  for (i = 0; i < nDest; i++) {
    free(destAddresses[i]);
    free(destPasswds[i]);
  }

  pthread_mutex_lock(&mutex);
  this -> confURLs = confURLs;
  pthread_mutex_unlock(&mutex);
}

void ApMon::loadFile(char *filename, int *nDestinations, char **destAddresses, 
		     int *destPorts, char **destPasswds) {
  FILE *f;
  char msg[100];
 
  /* initializations for the destination addresses */
  f = fopen(filename, "rt");
  if (f == NULL) {
    return;
  }

  snprintf(msg, 99, "Loading file %s ...", filename);
  logger(INFO, msg);

  lastModifFile = time(NULL);
  
  parseConf(f, nDestinations, destAddresses, destPorts, destPasswds);
  fclose(f);
}

ApMon::ApMon(int nDestinations, char **destinationsList) {
  constructFromList(nDestinations, destinationsList);
}

void ApMon::constructFromList(int nDestinations, char **destinationsList) {
  int i;

  if (destinationsList == NULL) 
    return;
 
#ifdef __APPLE__
  initType = OLIST_INIT;
#else
  initType = LIST_INIT;
#endif

  initMonitoring();

  /* save the initialization list */
  nInitSources = nDestinations;
  initSources = (char **)malloc(nInitSources * sizeof(char*));
  if (initSources == NULL)
    return;

  for (i = 0; i < nInitSources; i++)
    initSources[i] = strdup(destinationsList[i]);

  initialize(nDestinations, destinationsList, true);
}

void ApMon::initialize(int nDestinations, char **destinationsList,
		       bool firstTime) { 
  char *destAddresses[MAX_N_DESTINATIONS];
  int destPorts[MAX_N_DESTINATIONS];
  char *destPasswds[MAX_N_DESTINATIONS];
  char errmsg[200];
  int i;
  int cnt = 0;
  ConfURLs confURLs;

  logger(INFO, "Initializing destination addresses & ports:");

  if (nDestinations > MAX_N_DESTINATIONS)
    return;
  
  confURLs.nConfURLs = 0;

  for (i = 0; i < nDestinations; i++) {
    try { 
      if (strstr(destinationsList[i], "http") == destinationsList[i])
	getDestFromWeb(destinationsList[i], &cnt, 
		       destAddresses, destPorts, destPasswds, confURLs);
      else 
	addToDestinations(destinationsList[i], &cnt, 
			destAddresses, destPorts, destPasswds);

    } catch (runtime_error &e) {
      snprintf(errmsg, 199, "[ initialize() ] Error while loading the configuration: %s", e.what());
      logger(WARNING, errmsg);
      if (!firstTime) {
	for (i = 0; i < cnt; i++) {
	  free(destAddresses[i]);
	  free(destPasswds[i]);
	}
	logger(WARNING, "Configuration not reloaded successfully. Keeping the previous one.");
	return; 
      }     
    }  // catch
  }  // for

  try {
    arrayInit(cnt, destAddresses, destPorts, destPasswds, firstTime);
  } catch (runtime_error& err) {
    if (firstTime)
      return;
    else {
      logger(WARNING, "Error reloading the configuration. Keeping the previous one.");
      return;
    }
  }
  
  for (i = 0; i < cnt; i++) {
    free(destAddresses[i]);
    free(destPasswds[i]);
  }

  pthread_mutex_lock(&mutex);
  this -> confURLs = confURLs;
  pthread_mutex_unlock(&mutex);
}

void ApMon::addToDestinations(char *line, int *nDestinations, 
	       char *destAddresses[], int destPorts[], char *destPasswds[]) {
  char *addr, *port, *passwd;
  char *sep1 = (char *)" \t";
  char *sep2 = (char *)":";
 
  char *tmp = strdup(line);
  char *firstToken;
//  char buf[MAX_STRING_LEN];
//  char *pbuf = buf;

  /* the address & port are separated from the password with spaces */
  firstToken = strtok/*_r*/(tmp, sep1);//, &pbuf);
  passwd = strtok/*_r*/(NULL, sep1);//, &pbuf);

  /* the address and the port are separated with ":" */
  addr = strtok/*_r*/(firstToken, sep2);//, &pbuf);
  port = strtok/*_r*/(NULL, sep2);//, &pbuf);
  destAddresses[*nDestinations] = strdup(addr);
  if (port == NULL)
    destPorts[*nDestinations] = DEFAULT_PORT;
  else
    destPorts[*nDestinations] = atoi(port);
  if (passwd == NULL)
    destPasswds[*nDestinations] = strdup("");
  else
    destPasswds[*nDestinations] = strdup(passwd);
  (*nDestinations)++;

  free(tmp);
}
  
void ApMon::getDestFromWeb(char *url, int *nDestinations, 
  char *destAddresses[], int destPorts[], char *destPasswds[],
			   ConfURLs& confURLs) {
  char temp_filename[300];
  FILE *tmp_file;
  char *line, *ret, *tmp = NULL;
  bool modifLineFound;
  long mypid = getpid();
  char str1[20], str2[20];
  int totalSize, headerSize, contentSize;

#ifndef WIN32
  snprintf(temp_filename, 299, "/tmp/apmon_webconf%ld", mypid);
#else
  char *tmpp = getenv("TEMP");
  if(tmpp == NULL)
	  tmpp = getenv("TMP");
  if(tmpp == NULL)
	  tmpp = "c:";
  snprintf(temp_filename, 299, "%s\\apmon_webconf%ld", tmpp, mypid);
#endif 
  /* get the configuration file from web and put it in a temporary file */
  totalSize = httpRequest(url, "GET", temp_filename);

  /* read the configuration from the temporary file */
  tmp_file = fopen(temp_filename, "rt");
  if (tmp_file == NULL)
    return;

  line = (char*)malloc((MAX_STRING_LEN + 1) * sizeof(char));

  //check the HTTP header to see if we got the page correctly
  char *retGet = fgets(line, MAX_STRING_LEN, tmp_file);
  if (retGet == NULL)
    return;

  sscanf(line, "%s %s", str1, str2);
  if (atoi(str2) != 200) {
    free(line);
    fclose(tmp_file);
    return;
  }

  confURLs.vURLs[confURLs.nConfURLs] = strdup(url);

  // check the  header for the "Last-Modified" and "Content-Length" lines
  modifLineFound = false; 
  contentSize = 0;
  do {
    if (tmp != NULL)
      free(tmp);
    ret = fgets(line, MAX_STRING_LEN, tmp_file);
    if (ret == NULL) {
      free(line); fclose(tmp_file);
      return;
    }
    if (strstr(line, "Last-Modified") == line) {
      modifLineFound = true;
      confURLs.lastModifURLs[confURLs.nConfURLs] = strdup(line);
    }

    if (strstr(line, "Content-Length") == line) {
      sscanf(line, "%s %d", str1, &contentSize);
    }

    tmp = trimString(line);
  } while (strlen(tmp) != 0);
  free(tmp); free(line);

  if (!modifLineFound)
    confURLs.lastModifURLs[confURLs.nConfURLs] = strdup("");
  confURLs.nConfURLs++;

  headerSize = ftell(tmp_file);
  if (totalSize - headerSize < contentSize) {
    fclose(tmp_file);
    return;
  }

  try {
    parseConf(tmp_file, nDestinations, destAddresses, destPorts, 
	      destPasswds);
  } catch (...) {
    fclose(tmp_file);
    unlink(temp_filename);
    return;
  } 

  fclose(tmp_file);
  unlink(temp_filename);
}


ApMon::ApMon(int nDestinations, char **destAddresses, int *destPorts, 
	     char **destPasswds) {
  initMonitoring();

  arrayInit(nDestinations, destAddresses, destPorts, destPasswds);
}

void ApMon::arrayInit(int nDestinations, char **destAddresses, 
		      int *destPorts, char **destPasswds) {
	arrayInit(nDestinations, destAddresses, destPorts, destPasswds, true);
}


void ApMon::arrayInit(int nDestinations, char **destAddresses, int *destPorts,
		      char **destPasswds, bool firstTime) {
  int i, j;
  int ret;
  char *ipAddr, logmsg[100];
  bool found, havePublicIP;
  int tmpNDestinations;
  char **tmpAddresses, **tmpPasswds;
  int *tmpPorts;

  if (destAddresses == NULL || destPorts == NULL || nDestinations == 0)
    return;

  /* initializations that we have to do only once */
  if (firstTime) {
    //this -> appPID = getpid();

    this -> nMonJobs = 0;
    this -> monJobs = (MonitoredJob *)malloc(MAX_MONITORED_JOBS * 
					    sizeof(MonitoredJob));
   
    try {
      this -> numCPUs = ProcUtils::getNumCPUs();
    } catch (procutils_error &err) {
      logger(WARNING, err.what());
      this -> numCPUs = 0;
    }

    /* get the names of the network interfaces */
    this -> nInterfaces = 0;
    try {
      ProcUtils::getNetworkInterfaces(this -> nInterfaces, 
				     this -> interfaceNames);
    } catch (procutils_error &err) {
      logger(WARNING, err.what());
      this -> nInterfaces = 0;
    } 
     
    /* get the hostname of the machine */
    ret = gethostname(this -> myHostname, MAX_STRING_LEN -1);
    if (ret < 0) {
      logger(WARNING, "Could not obtain the local hostname");
      strcpy(myHostname, "unknown");
    } else
      myHostname[MAX_STRING_LEN - 1] = 0;

    /* get the IPs of the machine */
    this -> numIPs = 0; havePublicIP = false;
    strcpy(this -> myIP, "unknown");
   
    /* default values for cluster name and node name */
    this -> clusterName = strdup("ApMon_UserSend");
    this -> nodeName = strdup(myHostname);

#ifndef WIN32
    int sockd = socket(PF_INET, SOCK_STREAM, 0);
    if(sockd < 0){
      logger(WARNING, "Could not obtain local IP addresses");
    } else {
      for (i = 0; i < this -> nInterfaces; i++) {
    struct ifreq ifr;
	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, this -> interfaceNames[i], sizeof(ifr.ifr_name) - 1); 
	char ip[4], tmp_s[50];

	if(ioctl(sockd, SIOCGIFADDR, &ifr)<0){
	  snprintf(logmsg, 99, "Cannot get the address of %s", this->interfaceNames[i]);
	  logger(WARNING, logmsg);
	  continue;	//????????
	}

#if defined(__APPLE__) || defined (__SUNOS)
	memcpy(ip, ifr.ifr_addr.sa_data+2, 4);
#else
	memcpy(ip, ifr.ifr_hwaddr.sa_data+2, 4);
#endif

	strncpy(tmp_s, inet_ntoa(*(struct in_addr *)ip), 49);
	snprintf(logmsg, 99, "Found local IP address: %s", tmp_s);
	logger(FINE, logmsg);
	if (strcmp(tmp_s, "127.0.0.1") != 0 && !havePublicIP) {
	  strncpy(this -> myIP, tmp_s, MAX_STRING_LEN-1);
	  if (!isPrivateAddress(tmp_s))
	    havePublicIP = true;
	}
	strncpy(this -> allMyIPs[this -> numIPs], tmp_s, MAX_STRING_LEN-1);
	this -> numIPs++;
      }
    }
#else
	struct hostent *hptr;
    if ((hptr = gethostbyname(myHostname))!= NULL) {
      i = 0;
	  struct in_addr addr;
	  while ((hptr -> h_addr_list)[i] != NULL) {
	    memcpy(&(addr.s_addr), (hptr -> h_addr_list)[i], 4);
        ipAddr = inet_ntoa(addr);
	    if (strcmp(ipAddr, "127.0.0.1") != 0) {
	      strncpy(this -> myIP, ipAddr, MAX_STRING_LEN-1);
	      if (!isPrivateAddress(ipAddr))
		    break;
		}
	    i++;
	  }
	}
#endif

    this -> sysMonCluster = strdup("ApMon_SysMon");
    this -> sysMonNode = strdup(this -> myIP);

    this -> prvTime = 0;
    this -> prvSent = 0;
    this -> prvDrop = 0;
    this -> crtTime = 0;
    this -> crtSent = 0;
    this -> crtDrop = 0;
    
    //  exp(-5.0/60.0);
    this -> hWeight = 0.92004441462932322925;

    srand(time(NULL));

    /* initialize buffer for XDR encoding */
    this -> buf = (char *)malloc(MAX_DGRAM_SIZE);
    if (this -> buf == NULL)
	return;

    this -> dgramSize = 0;

    /*create the socket & set options*/
    initSocket();

    /* initialize the sender ID and the sequence number */
    instance_id = rand();
    seq_nr = 0;
  }
  
  /* put the destination addresses, ports & passwords in some temporary
     buffers (because we don't want to lock mutex while making DNS
     requests)
  */
  tmpNDestinations = 0;
  tmpPorts = (int *)malloc(nDestinations * sizeof(int));
  tmpAddresses = (char **)malloc(nDestinations * sizeof(char *));
  tmpPasswds = (char **)malloc(nDestinations * sizeof(char *));
  if (tmpPorts == NULL || tmpAddresses == NULL || 
      tmpPasswds == NULL)
    return;

  for (i = 0; i < nDestinations; i++) {
    try {
      ipAddr = findIP(destAddresses[i]);
    } catch (runtime_error &err) {
      logger(FATAL, err.what());
      continue;
    }
    
    /* make sure this address is not already in the list */
    found = false;
    for (j = 0; j < tmpNDestinations; j++) {
      if (!strcmp(ipAddr, tmpAddresses[j])) {
	found = true;
	break;
      }
    }

    /* add the address to the list */
    if (!found) {
      tmpAddresses[tmpNDestinations] = ipAddr;
      tmpPorts[tmpNDestinations] = destPorts[i];
      tmpPasswds[tmpNDestinations] = strdup(destPasswds[i]);

      snprintf(logmsg, 99, "Adding destination host: %s  - port %d",  tmpAddresses[tmpNDestinations], tmpPorts[tmpNDestinations]);
      logger(INFO, logmsg);

      tmpNDestinations++;
    }
  }

  if (tmpNDestinations == 0) {
    freeMat(tmpAddresses, tmpNDestinations);
    freeMat(tmpPasswds, tmpNDestinations);
    return;
  }

  pthread_mutex_lock(&mutex);
  if (!firstTime)
      freeConf();
  this -> nDestinations = tmpNDestinations;
  this -> destAddresses = tmpAddresses;
  this -> destPorts = tmpPorts;
  this -> destPasswds = tmpPasswds;
  pthread_mutex_unlock(&mutex);

  /* start job/system monitoring according to the settings previously read 
     from the configuration file */
  setJobMonitoring(jobMonitoring, jobMonitorInterval);
  setSysMonitoring(sysMonitoring, sysMonitorInterval);
  setGenMonitoring(genMonitoring, genMonitorIntervals);
  setConfRecheck(confCheck, recheckInterval);
}


ApMon::~ApMon() {
  int i;

  if (bkThreadStarted) {
    if (getJobMonitoring()) {
      /* send a datagram with job monitoring information which covers
	 the last time interval */
      sendJobInfo();
    }
  }

  pthread_mutex_lock(&mutexBack);
  setBackgroundThread(false);
  pthread_mutex_unlock(&mutexBack);

  pthread_mutex_destroy(&mutex);
  pthread_mutex_destroy(&mutexBack);
  pthread_mutex_destroy(&mutexCond);
  pthread_cond_destroy(&confChangedCond);

  free(clusterName);
  free(nodeName);
  free(sysMonCluster); free(sysMonNode);

  freeConf();

  free(monJobs);
  for (i = 0; i < nInitSources; i++) {
    free(initSources[i]);
  }
  free(initSources);
  
  free(buf);
#ifndef WIN32
  close(sockfd);
#else
  closesocket(sockfd);
  WSACleanup();
#endif
}

void ApMon::freeConf() {
  int i;
  freeMat(destAddresses, nDestinations);
  freeMat(destPasswds, nDestinations);
  free(destPorts);

  for (i = 0; i < confURLs.nConfURLs; i++) {
      free(confURLs.vURLs[i]);
      free(confURLs.lastModifURLs[i]);
  }
}

int ApMon::sendParameters(char *clusterName, char *nodeName,
	       int nParams, char **paramNames, int *valueTypes, 
			 char **paramValues) {
 return sendTimedParameters(clusterName, nodeName, nParams, 
			    paramNames, valueTypes, paramValues, -1);
}

int ApMon::sendTimedParameters(char *clusterName, char *nodeName,
	       int nParams, char **paramNames, int *valueTypes, 
	       char **paramValues, int timestamp) {
  int i;
  int ret, ret1, ret2;
  char msg[200], buf2[MAX_HEADER_LENGTH+4], newBuf[MAX_DGRAM_SIZE], crtAddr[128];
  char *headerTmp;
  char header[MAX_HEADER_LENGTH] = "v:";
  strcat(header, APMON_VERSION);
  strcat(header, "_cpp"); // to indicate this is the C++ version
  strcat(header, "p:");

  pthread_mutex_lock(&mutex);

  if(!shouldSend()) {
     pthread_mutex_unlock(&mutex);
     return RET_NOT_SENT;
  }
		
  if (clusterName != NULL) { // don't keep the cached values for cluster name
    // and node name
    free(this -> clusterName);
    this -> clusterName = strdup(clusterName);
    
    if (nodeName != NULL) {  /* the user provided a name */
      free(this -> nodeName);
      this -> nodeName = strdup(nodeName);
    }
    else { /* set the node name to the node's IP */
      free(this -> nodeName);
      this -> nodeName = strdup(this -> myHostname);
    }  // else
  } // if
  
  if (this -> clusterName == NULL || this -> nodeName == NULL) {
    pthread_mutex_unlock(&mutex);
    return -1;
  }

  //sortParams(nParams, paramNames, valueTypes, paramValues);

  /* try to encode the parameters */
  try {
    encodeParams(nParams, paramNames, valueTypes, paramValues, timestamp);
  } catch (runtime_error& err) {
    pthread_mutex_unlock(&mutex);
    return -1;
  }

  headerTmp = (char *)malloc(MAX_HEADER_LENGTH * sizeof(char)); 
  /* for each destination */
  for (i = 0; i < nDestinations; i++) {
    XDR xdrs;
    struct sockaddr_in destAddr;

    /* initialize the destination address */
    memset(&destAddr, 0, sizeof(destAddr));
    destAddr.sin_family = AF_INET;
    destAddr.sin_port = htons(destPorts[i]);
#ifndef WIN32
    inet_pton(AF_INET, destAddresses[i], &destAddr.sin_addr);
#else
    int dummy = sizeof(destAddr);
    snprintf(crtAddr, 127, "%s:%d", destAddresses[i], destPorts[i]);
    ret = WSAStringToAddress(crtAddr, AF_INET, NULL, (struct sockaddr *) &destAddr, &dummy);
    if(ret){
      ret = WSAGetLastError();
      snprintf(msg, 199, "[ sendTimedParameters() ] Error packing address %s, code %d ", crtAddr, ret);
      return -1;
    }
#endif
    /* add the header (which is different for each destination) */
   
    strncpy(headerTmp, header, MAX_HEADER_LENGTH-1);
    strncat(headerTmp, destPasswds[i], MAX_HEADER_LENGTH-strlen(headerTmp)-1);

    /* initialize the XDR stream to encode the header */
    xdrmem_create(&xdrs, buf2, MAX_HEADER_LENGTH, XDR_ENCODE); 

    /* encode the header */
    ret = xdr_string(&xdrs, &(headerTmp), strlen(headerTmp) + 1);
    /* add the instance ID and the sequence number */
    ret1 = xdr_int(&xdrs, &(instance_id));
    ret2 = xdr_int(&xdrs, &(seq_nr));

    if (!ret || !ret1 || !ret2) {
        free(headerTmp);
        pthread_mutex_unlock(&mutex);
        return -1;
    }

    /* concatenate the header and the rest of the datagram */
    int buf2Length = xdrSize(XDR_STRING, headerTmp) + 2 * xdrSize(XDR_INT32, NULL);
    memcpy(newBuf, buf2, buf2Length);
    memcpy(newBuf + buf2Length, buf, dgramSize);

    /* send the buffer */
    ret = sendto(sockfd, newBuf, dgramSize + buf2Length, 0, 
		 (struct sockaddr *)&destAddr, sizeof(destAddr));
    if (ret == RET_ERROR) {
      free(headerTmp);
      pthread_mutex_unlock(&mutex);

      /*re-initialize the socket */
#ifndef WIN32
      close(sockfd);
#else
      closesocket(sockfd);
#endif
      initSocket();

      /* throw exception because the datagram was not sent */
      snprintf(msg, 199, "[ sendTimedParameters() ] Error sending data to destination %s ", destAddresses[i]);
      return -1;
    }
    else {
      snprintf(msg, 199, "Datagram with size %d, instance id %d, sequence number %d, sent to %s, containing parameters:", ret, instance_id, seq_nr, destAddresses[i]);
      logger(FINE, msg);
      logParameters(FINE, nParams, paramNames, valueTypes, paramValues);
    }
    xdr_destroy(&xdrs);
    
  }

  seq_nr = (seq_nr + 1) % TWO_BILLION;
  free(headerTmp);
  pthread_mutex_unlock(&mutex);
  return RET_SUCCESS;
}


int ApMon::sendParameter(char *clusterName, char *nodeName,
			char *paramName, int valueType, char *paramValue) {

  return sendParameters(clusterName, nodeName, 1, &paramName, 
			      &valueType, &paramValue);
}

int ApMon::sendTimedParameter(char *clusterName, char *nodeName,
	      char *paramName, int valueType, char *paramValue, int timestamp) {

  return sendTimedParameters(clusterName, nodeName, 1, &paramName, 
			      &valueType, &paramValue, timestamp);
}

int ApMon::sendParameter(char *clusterName, char *nodeName,
		char *paramName, int paramValue) {
  
  return sendParameter(clusterName, nodeName, paramName, XDR_INT32, 
		    (char *)&paramValue);
}

int ApMon::sendParameter(char *clusterName, char *nodeName,
		char *paramName, float paramValue) {
  
  return sendParameter(clusterName, nodeName, paramName, XDR_REAL32, 
		    (char *)&paramValue);
}

int ApMon::sendParameter(char *clusterName, char *nodeName,
		char *paramName, double paramValue) {
  
  return sendParameter(clusterName, nodeName, paramName, XDR_REAL64, 
		    (char *)&paramValue);
}

int ApMon::sendParameter(char *clusterName, char *nodeName,
		char *paramName, char *paramValue) {
  
  return sendParameter(clusterName, nodeName, paramName, XDR_STRING, 
		    paramValue);
}

void ApMon::encodeParams(int nParams, char **paramNames, int *valueTypes, 
			char **paramValues, int timestamp) {
  XDR xdrs; /* XDR handle. */
  int i, effectiveNParams;

  /* count the number of parameters actually sent in the datagram
     (the parameters with a NULL name and the string parameters
     with a NULL value are skipped)
  */
  effectiveNParams = nParams;
  for (i = 0; i < nParams; i++) {
      if (paramNames[i] == NULL || (valueTypes[i] == XDR_STRING && 
				    paramValues[i] == NULL)) {
	effectiveNParams--;
      }
  }
  if (effectiveNParams == 0)
    return;

  /*** estimate the length of the send buffer ***/

  /* add the length of the cluster name & node name */
  dgramSize =  xdrSize(XDR_STRING, clusterName) + 
      xdrSize(XDR_STRING, nodeName) + xdrSize(XDR_INT32, NULL);
  /* add the lengths for the parameters (name + size + value) */
  for (i = 0; i < nParams; i++) {
    dgramSize += xdrSize(XDR_STRING, paramNames[i]) +  xdrSize(XDR_INT32, NULL) +
      + xdrSize(valueTypes[i], paramValues[i]);
  }

  /* check that the maximum datagram size is not exceeded */
  if (dgramSize + MAX_HEADER_LENGTH > MAX_DGRAM_SIZE) 
    return;

  /* initialize the XDR stream */
  xdrmem_create(&xdrs, buf, MAX_DGRAM_SIZE, XDR_ENCODE); 

  try {
    /* encode the cluster name, the node name and the number of parameters */
    if (!xdr_string(&xdrs, &(clusterName), strlen(clusterName) + 1))
	return;

    if (!xdr_string(&xdrs, &(nodeName), strlen(nodeName) + 1))
	return;
    
    if (!xdr_int(&xdrs, &(effectiveNParams)))
	return;

    /* encode the parameters */
    for (i = 0; i < nParams; i++) {
      if (paramNames[i] == NULL || (valueTypes[i] == XDR_STRING && 
				    paramValues[i] == NULL)) {
	logger(WARNING, "NULL parameter name or value - skipping parameter...");
	continue;
      }

      /* parameter name */
      if (!xdr_string(&xdrs, &(paramNames[i]), strlen(paramNames[i]) + 1))
	return;
    
      /* parameter value type */
      if (!xdr_int(&xdrs, &(valueTypes[i])))  
	return;

      /* parameter value */
      switch (valueTypes[i]) {
      case XDR_STRING:
	if (!xdr_string(&xdrs, &(paramValues[i]), strlen(paramValues[i]) + 1))
	    return;
	break;
	//INT16 is not supported
	/*    case XDR_INT16:  
	      if (!xdr_short(&xdrs, (short *)(paramValues[i])))
	      return RET_ERROR;
	      break;
	*/    case XDR_INT32:
		if (!xdr_int(&xdrs, (int *)(paramValues[i])))  
		    return;
		break;
      case XDR_REAL32:
	if (!xdr_float(&xdrs, (float *)(paramValues[i])))
	    return;
	break;
      case XDR_REAL64:
	if (!xdr_double(&xdrs, (double *)(paramValues[i])))
	    return;
	break;
      default:
	    return;
      }
    }
   
    /* encode the timestamp if necessary */
    if (timestamp > 0) {
      if (!xdr_int(&xdrs, &timestamp))  
	return;
      dgramSize += xdrSize(XDR_INT32, NULL);
    }
  } catch (runtime_error& err) {
    xdr_destroy(&xdrs);
    return;
  }

  xdr_destroy(&xdrs);
}

#ifndef WIN32
void *bkTask(void *param) { 
#else
DWORD WINAPI bkTask(void *param) {
#endif
  struct stat st;
#ifndef WIN32
  struct timespec delay;
#else
  DWORD delay;
#endif
  bool resourceChanged, haveChange;
  int nextOp = -1, i, ret;
  int generalInfoCount;
  time_t crtTime, timeRemained;
  time_t nextRecheck = 0, nextJobInfoSend = 0, nextSysInfoSend = 0;
  ApMon *apm = (ApMon *)param;
  char logmsg[200];

  logger(INFO, "[Starting background thread...]");
  apm -> bkThreadStarted = true;

  crtTime = time(NULL);

  pthread_mutex_lock(&(apm -> mutexBack));
  if (apm -> confCheck) {
    nextRecheck = crtTime + apm -> crtRecheckInterval;
    /*
    snprintf(logmsg, 99, "###1 crt %ld interv %ld recheck %ld ", crtTime,
       apm -> crtRecheckInterval, nextRecheck);
    logger(FINE, logmsg);
    fflush(stdout);
    */
  }
  if (apm -> jobMonitoring)
    nextJobInfoSend = crtTime + apm -> jobMonitorInterval;
  if (apm -> sysMonitoring)
    nextSysInfoSend = crtTime + apm -> sysMonitorInterval;
  pthread_mutex_unlock(&(apm -> mutexBack));
  
  timeRemained = -1;
  generalInfoCount = 0;

  while (1) {
    pthread_mutex_lock(&apm -> mutexBack);
    if (apm -> stopBkThread) {
      pthread_mutex_unlock(&apm -> mutexBack);
      break;
    }
    pthread_mutex_unlock(&apm -> mutexBack);

    /*
    crtTime = time(NULL);
    snprintf(logmsg, 99, "### 2 crt %ld recheck %ld sys %ld ",crtTime,  nextRecheck, 
        nextSysInfoSend);
    logger(FINE, logmsg);
    */
    
    /* determine the next operation that must be performed */
    if (nextRecheck > 0 && (nextJobInfoSend <= 0 || 
			    nextRecheck <= nextJobInfoSend)) {
      if (nextSysInfoSend <= 0 || nextRecheck <= nextSysInfoSend) {
	nextOp = RECHECK_CONF;
	timeRemained = (nextRecheck - crtTime > 0) ? (nextRecheck - crtTime) : 0;
      } else {
	nextOp = SYS_INFO_SEND;
	timeRemained = (nextSysInfoSend - crtTime > 0) ? (nextSysInfoSend - crtTime) : 0;
      }
    } else {
      if (nextJobInfoSend > 0 && (nextSysInfoSend <= 0 || 
				  nextJobInfoSend <= nextSysInfoSend)) {
	nextOp = JOB_INFO_SEND;
	timeRemained = (nextJobInfoSend - crtTime > 0) ? (nextJobInfoSend - crtTime) : 0;
      } else if (nextSysInfoSend > 0) {
	nextOp = SYS_INFO_SEND;
	timeRemained = (nextSysInfoSend - crtTime > 0) ? (nextSysInfoSend - crtTime) : 0;
      }
    }

    if (timeRemained == -1) {
	logger(INFO, "Background thread has no operation to perform...");
	timeRemained = RECHECK_INTERVAL;
    }

#ifndef WIN32
    /* the moment when the next operation should be performed */
    delay.tv_sec = crtTime + timeRemained;
    delay.tv_nsec = 0;
#else
    delay = (/*crtTime +*/ timeRemained) * 1000;  // this is in millis
#endif

    pthread_mutex_lock(&(apm -> mutexBack));

    pthread_mutex_lock(&(apm -> mutexCond));
    /* check for changes in the settings */
    haveChange = false;
    if (apm -> jobMonChanged || apm -> sysMonChanged || apm -> recheckChanged)
      haveChange = true;
    if (apm -> jobMonChanged) {
      if (apm -> jobMonitoring) 
	nextJobInfoSend = crtTime + apm -> jobMonitorInterval;
      else
	nextJobInfoSend = -1;
      apm -> jobMonChanged = false;
    }
    if (apm -> sysMonChanged) {
      if (apm -> sysMonitoring) 
	nextSysInfoSend = crtTime + apm -> sysMonitorInterval;
      else
	nextSysInfoSend = -1;
      apm -> sysMonChanged = false;
    }
    if (apm -> recheckChanged) {
      if (apm -> confCheck) {
	nextRecheck = crtTime + apm -> crtRecheckInterval;
      }
      else
	nextRecheck = -1;
      apm -> recheckChanged = false;
    }
    pthread_mutex_unlock(&(apm -> mutexBack));

    if (haveChange) {
      pthread_mutex_unlock(&(apm -> mutexCond));
      continue;
    }
    
    /* wait until the next operation should be performed or until
       a change in the settings occurs */
#ifndef WIN32
    ret = pthread_cond_timedwait(&(apm -> confChangedCond), 
				&(apm -> mutexCond), &delay);
    pthread_mutex_unlock(&(apm -> mutexCond));
#else
    pthread_mutex_unlock(&(apm -> mutexCond));
    ret = WaitForSingleObject(apm->confChangedCond, delay);
#endif
    if (ret == ETIMEDOUT) {
      /* now perform the operation */
      if (nextOp == JOB_INFO_SEND) {
	apm -> sendJobInfo();
	crtTime = time(NULL);
	nextJobInfoSend = crtTime + apm -> getJobMonitorInterval();
      }
      
      if (nextOp == SYS_INFO_SEND) {
	apm -> sendSysInfo();
	if (apm -> getGenMonitoring()) {
	  if (generalInfoCount <= 1)
	    apm -> sendGeneralInfo();
	  generalInfoCount = (generalInfoCount + 1) % apm -> genMonitorIntervals;
	}
	crtTime = time(NULL);
	nextSysInfoSend = crtTime + apm -> getSysMonitorInterval();
      }

      if (nextOp == RECHECK_CONF) {
	resourceChanged = false;
	try {
	  if (apm -> initType == FILE_INIT) {
	    snprintf(logmsg, 199, "Checking for modifications for file %s ", 
		    apm -> initSources[0]);
	    logger(INFO, logmsg);
	    stat(apm -> initSources[0], &st);
	    if (st.st_mtime > apm -> lastModifFile) {
	      snprintf(logmsg, 199, "File %s modified ", apm -> initSources[0]);
	      logger(INFO, logmsg);
	      resourceChanged = true;
	    }
	  }

	  // check the configuration URLs
	  for (i = 0; i < apm -> confURLs.nConfURLs; i++) {
	    snprintf(logmsg, 199, "[Checking for modifications for URL %s ] ", 
		   apm -> confURLs.vURLs[i]);
	    logger(INFO, logmsg);
	    if (urlModified(apm -> confURLs.vURLs[i], apm -> confURLs.lastModifURLs[i])) {
	      snprintf(logmsg, 199, "URL %s modified ", apm -> confURLs.vURLs[i]);
	      logger(INFO, logmsg);
	      resourceChanged = true;
	      break;
	    }
	  }

	  if (resourceChanged) {
	    logger(INFO, "Reloading configuration...");
	    if (apm -> initType == FILE_INIT)
	      apm -> initialize(apm -> initSources[0], false);
	    else
	      apm -> initialize(apm -> nInitSources, apm -> initSources, false);
	  }
	  apm -> setCrtRecheckInterval(apm -> getRecheckInterval());
	} catch (runtime_error &err) {
	  logger(WARNING, err.what());
	  logger(WARNING, "Increasing the time interval for reloading the configuration...");
	  apm -> setCrtRecheckInterval(apm -> getRecheckInterval() * 5);
	}
	crtTime = time(NULL);
	nextRecheck = crtTime + apm -> getCrtRecheckInterval();
	//sleep(apm -> getCrtRecheckInterval());
      }
    }
 
  } // while

#ifndef WIN32
  return NULL; // it doesn't matter what we return here
#else
  return 0;
#endif
}

void ApMon::setConfRecheck(bool confCheck, long interval) {
  char logmsg[100];
  if (confCheck) {
    snprintf(logmsg, 99, "Enabling configuration reloading (interval %ld)", interval);
    logger(INFO, logmsg);
  }

  pthread_mutex_lock(&mutexBack);
  if (initType == DIRECT_INIT) { // no need to reload the configuration
    logger(WARNING, "[ setConfRecheck() } No configuration file/URL to reload.");
    return;
  }

  this -> confCheck = confCheck;
  this -> recheckChanged = true;
  if (confCheck) {
    if (interval > 0) {
      this -> recheckInterval = interval;
      this -> crtRecheckInterval = interval;
    } else {
      this -> recheckInterval = RECHECK_INTERVAL;
      this -> crtRecheckInterval = RECHECK_INTERVAL;
    }
    setBackgroundThread(true);
  }
  else {
    if (jobMonitoring == false && sysMonitoring == false)
      setBackgroundThread(false);
  }
  pthread_mutex_unlock(&mutexBack);
    
}

void ApMon::setRecheckInterval(long val) {
  if (val > 0) {
    setConfRecheck(true, val);
  }
  else {
    setConfRecheck(false, val);
  }
}

void ApMon::setCrtRecheckInterval(long val) {
  pthread_mutex_lock(&mutexBack);
  crtRecheckInterval = val;
  pthread_mutex_unlock(&mutexBack);
}

void ApMon::setJobMonitoring(bool bJobMonitoring, long interval) {
  char logmsg[100];
  if (bJobMonitoring) {
    snprintf(logmsg, 99, "Enabling job monitoring, time interval %ld s... ", interval);
    logger(INFO, logmsg);
  } else
    logger(INFO, "Disabling job monitoring...");

  pthread_mutex_lock(&mutexBack);
  this -> jobMonitoring = bJobMonitoring;
  this -> jobMonChanged = true;
  if (bJobMonitoring == true) {
    if (interval > 0)
      this -> jobMonitorInterval = interval;
    else
      this -> jobMonitorInterval = JOB_MONITOR_INTERVAL;
    setBackgroundThread(true);
  } else {
    // disable the background thread if it is not needed anymore
    if (this -> sysMonitoring == false && this -> confCheck == false)
      setBackgroundThread(false);
  }
  pthread_mutex_unlock(&mutexBack);
}

void ApMon::setSysMonitoring(bool bSysMonitoring, long interval) {
  char logmsg[100];
  if (bSysMonitoring) {
    snprintf(logmsg, 99, "Enabling system monitoring, time interval %ld s... ", interval);
    logger(INFO, logmsg);
  } else
    logger(INFO, "Disabling system monitoring...");

  pthread_mutex_lock(&mutexBack);
  this -> sysMonitoring = bSysMonitoring;
  this -> sysMonChanged = true;
  if (bSysMonitoring == true) {
    if (interval > 0)
      this -> sysMonitorInterval = interval;
    else 
      this -> sysMonitorInterval = SYS_MONITOR_INTERVAL;
    setBackgroundThread(true);
  }  else {
    // disable the background thread if it is not needed anymore
    if (this -> jobMonitoring == false && this -> confCheck == false)
      setBackgroundThread(false);
  }
  pthread_mutex_unlock(&mutexBack);
}
  
void ApMon::setGenMonitoring(bool bGenMonitoring, int nIntervals) {
  char logmsg[100];
  snprintf(logmsg, 99, "Setting general information monitoring to %s ", boolStrings[(int)bGenMonitoring]);
  logger(INFO, logmsg);

  pthread_mutex_lock(&mutexBack);
  this -> genMonitoring = bGenMonitoring;
  this -> sysMonChanged = true;
  if (bGenMonitoring == true) {
    if (nIntervals > 0)
      this -> genMonitorIntervals = nIntervals;
    else 
      this -> genMonitorIntervals = GEN_MONITOR_INTERVALS; 
    
    if (this -> sysMonitoring == false) {
      pthread_mutex_unlock(&mutexBack);
      setSysMonitoring(true);
      pthread_mutex_lock(&mutexBack);
    }
  } // TODO: else check if we can stop the background thread (if no
  // system parameters are enabled for monitoring)
  pthread_mutex_unlock(&mutexBack);
}

void ApMon::setBackgroundThread(bool val) {
  // mutexBack is locked
  if (val == true) {
    if (!haveBkThread) {
#ifndef WIN32
      pthread_create(&bkThread, NULL, &bkTask, this);
#else
      DWORD dummy;
      bkThread = CreateThread(NULL, 65536, &bkTask, this, 0, &dummy);
#endif
      haveBkThread = true;
    } else {
      pthread_mutex_lock(&mutexCond);
      pthread_cond_signal(&confChangedCond);
      pthread_mutex_unlock(&mutexCond);
    }
  }
  if (val == false) {
    //if (bkThreadStarted) {
    if (haveBkThread) {
      stopBkThread = true;
      pthread_mutex_unlock(&mutexBack);
#ifndef WIN32
      pthread_mutex_lock(&mutexCond);
#endif
      pthread_cond_signal(&confChangedCond);
      logger(INFO, "[Stopping the background thread...]");
#ifndef WIN32
      pthread_mutex_unlock(&mutexCond);
      pthread_join(bkThread, NULL);
#else
      WaitForSingleObject(bkThread, INFINITE);
#endif
      pthread_mutex_lock(&mutexBack);
//      logger(INFO, "bk thread stopped!");
      haveBkThread = false;
      bkThreadStarted = false;
      stopBkThread = false;
    }
  }
}

void ApMon::addJobToMonitor(long pid, char *workdir, char *clusterName, char *nodeName) {
  if (nMonJobs >= MAX_MONITORED_JOBS)
    return;

  MonitoredJob job;
  job.pid = pid;
  if (workdir == NULL) 
    strcpy(job.workdir, "");
  else
    strncpy(job.workdir, workdir, MAX_STRING_LEN-1);

 if (clusterName == NULL || strlen(clusterName) == 0) 
    strcpy(job.clusterName, "ApMon_JobMon");
  else
    strncpy(job.clusterName, clusterName, 49);
    
 if (nodeName == NULL || strlen(nodeName) == 0) 
    strncpy(job.nodeName, this -> myIP, 49);
  else
    strncpy(job.nodeName, nodeName, 49);

  monJobs[nMonJobs++] = job;
}

void ApMon::removeJobToMonitor(long pid) {
  int i, j;
  char msg[100];

  if (nMonJobs <= 0)
    return;
  
  for (i = 0; i < nMonJobs; i++) { 
    if (monJobs[i].pid == pid) {
      /* found the job, now remove it */
      for (j = i; j < nMonJobs - 1; j++)
	monJobs[j] = monJobs[j + 1];
      nMonJobs--;
      return;
    }
  }
}

void ApMon::setSysMonClusterNode(char *clusterName, char *nodeName) {
  free (sysMonCluster); free(sysMonNode);
  sysMonCluster = strdup(clusterName);
  sysMonNode = strdup(nodeName);
}

void ApMon::setLogLevel(char *newLevel_s) {
  int newLevel;
  const char * const levels[5] = {"FATAL", "WARNING", "INFO", "FINE", "DEBUG"};
  char logmsg[100];

  for (newLevel = 0; newLevel < 5; newLevel++)
    if (strcmp(newLevel_s, levels[newLevel]) == 0)
      break;

  if (newLevel >= 5) {
    snprintf(logmsg, 99, "[ setLogLevel() ] Invalid level value: %s", newLevel_s);
    logger(WARNING, logmsg); 
  }
  else
    logger(0, NULL, newLevel);
}

	
void ApMon::setMaxMsgRate(int maxRate) {
  if (maxRate > 0)
    this -> maxMsgRate  = maxRate;
}

void ApMon::initSocket() {
  int optval1 = 1;
  struct timeval optval2; 
  int ret1 = 0, ret2 = 0, ret3 = 0;
  char logmsg[100];

  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) 
    return;
    
  ret1 = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *) &optval1, sizeof(optval1));

  if (ret1!=0){
    snprintf(logmsg, 99, "[ initSocket() ] cannot set reuseaddr: %d", ret1);
    logger(WARNING, logmsg); 
  }

#ifndef __SUNOS
  // on solaris these calls fail, but they probably don't matter for UDP so skip them
  /* set connection timeout */
  optval2.tv_sec = 20;
  optval2.tv_usec = 0;
  ret2 = setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *) &optval2, sizeof(optval2));
  
  if (ret2!=0){
    snprintf(logmsg, 99, "[ initSocket() ] cannot set send timeout to %ld seconds: %d", optval2.tv_sec, ret2);
    logger(WARNING, logmsg);
  }
  
  ret3 = setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *) &optval2, sizeof(optval2));

  if (ret3!=0){
    snprintf(logmsg, 99, "[ initSocket() ] cannot set receive timeout to %ld seconds: %d", optval2.tv_sec, ret3);
    logger(WARNING, logmsg);
  }
#endif
  
  if (ret1 != 0 || ret2 != 0 || ret3 != 0){
    return;
  }
}


void ApMon::parseConf(FILE *fp, int *nDestinations, char **destAddresses, 
		     int *destPorts, char **destPasswds) {
  int i, ch;
  char *line = (char *)malloc ((MAX_STRING_LEN1) * sizeof(char));
  char *tmp = NULL; 
  char *loglevel_s;
//  char sbuf[30];
//  char *pbuf = sbuf;

  /* parse the input file */
  while(fgets(line, MAX_STRING_LEN, fp) != NULL) {

    if (tmp != NULL) {
      free(tmp);
      tmp = NULL;
    }

    line[MAX_STRING_LEN - 1] = 0;
    /* check if the line was too long */
    ch = fgetc(fp); // see if we are at the end of the file
    ungetc(ch, fp);
    if (line[strlen(line) - 1] != 10 && ch != EOF) {
      /* if the line doesn't end with a \n and we are not at the end
	 of file, the line from the file was longer than MAX_STRING_LEN */
      fclose(fp);
      return;
    }

    tmp = trimString(line);
      
    /* skip the blank lines and the comment lines */
    if (strlen(tmp) == 0 || strchr(tmp, '#') == tmp)
      continue;
    
    if (strstr(tmp, "xApMon_loglevel") == tmp) {
      char *tmp2 = tmp;
      strtok/*_r*/(tmp2, "= ");//, &pbuf);
      loglevel_s = strtok/*_r*/(NULL, "= ");//, &pbuf);
      setLogLevel(loglevel_s);
      continue;
    }

    if (strstr(tmp, "xApMon_") == tmp) {
      parseXApMonLine(tmp);
      continue;
    }
    
    if (*nDestinations >= MAX_N_DESTINATIONS) {
      free(line); free(tmp); 
      for (i = 0; i < *nDestinations; i++) {
	free(destAddresses[i]);
	free(destPasswds[i]);
      }
      fclose(fp);
      return;
    }

    addToDestinations(tmp, nDestinations, destAddresses, destPorts, 
		      destPasswds);
  }

  if (tmp != NULL)
    free(tmp);
  free(line);
}

bool ApMon::shouldSend() {

  long now = time(NULL);
  bool doSend;
  char msg[200];

  //printf("now %ld crtTime %ld\n", now, crtTime);

  if (now != crtTime){
    /** new time, update previous counters; */
    prvSent = hWeight * prvSent + (1.0 - hWeight) * crtSent / (now - crtTime);
    prvTime = crtTime;
    snprintf(msg, 199, "previously sent: %ld dropped: %ld", crtSent, crtDrop);
    logger(DEBUG, msg);
    /** reset current counter */
    crtTime = now;
    crtSent = 0;
    crtDrop = 0;
    //printf("\n");
  }
		
  /** compute the history */
  int valSent = (int)(prvSent * hWeight + crtSent * (1.0 - hWeight));

  doSend = true;
  /** when we should start dropping messages */
  int level = this -> maxMsgRate - this -> maxMsgRate / 10;

 
  if (valSent > (this -> maxMsgRate - level)) {
    //int max10 = this -> maxMsgRate / 10;
    int rnd  = rand() % (this -> maxMsgRate / 10);
    doSend = (rnd <  (this -> maxMsgRate - valSent));
  }
  /** counting sent and dropped messages */
  if (doSend) {
    crtSent++;
    //printf("#");
  } else {
    crtDrop++;
    //printf(".");
  }
  
  return doSend;
}
