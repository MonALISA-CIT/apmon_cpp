/**
 * \file ApMon.h
 * Declarations for the ApMon class.
 * The ApMon class can be used for sending monitoring data to one or 
 * more destination hosts that run MonALISA. 
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

#ifndef _POSIX_PTHREAD_SEMANTICS 
#define _POSIX_PTHREAD_SEMANTICS
#endif

// this is needed for Solaris, works on Linux, but breaks the compilation on OS X
#ifndef __APPLE__

#ifndef _POSIX_SOURCE
#define _POSIX_SOURCE
#endif

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 2
#endif

#endif
// --------------------------------------------------------------------------------

#ifndef _POSIX_VERSION
#define _POSIX_VERSION 200101L
#endif

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE
#endif

#ifndef __EXTENSIONS__
#define __EXTENSIONS__
#endif

#ifndef __STDC__
#define __STDC__
#endif

#ifdef _WIN32
  // FIXME: (MCl) the following warning tells that the usage of throw
  // declaration may cause trouble with VC > 7.1

  // Disable warning C4290: C++ exception specification ignored except to indicate a function is not __declspec(nothrow)
#pragma warning ( disable : 4290 )
#endif

#ifndef ApMon_h
#define ApMon_h

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdexcept>
#include <ctype.h>
#include <time.h>
#include "xdr.h"

#ifdef WIN32
#include <Winsock2.h>
#include <string.h>
#include <process.h>

#else
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/utsname.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/if.h>
#include <netdb.h>
#include <unistd.h>
#include <pthread.h>
#include <pwd.h>
#include <grp.h>

#if defined (__SVR4) && defined (__sun)
#define __SUNOS
#endif

#ifdef __SUNOS
#include <sys/times.h>
#include <limits.h>
#include <sys/sockio.h>
#endif

#if defined(__APPLE__) || (defined (__SUNOS))
#include <sys/param.h>
#else
#include <linux/param.h>
#endif

#endif  // ~WIN32

using namespace std;

#define XDR_STRING  0  /**< Used to code the string data type */
//#define XDR_INT16   1  /**< Used to code the 2 bytes integer data type */ // NOT SUPPORTED YET!
#define XDR_INT32   2  /**< Used to code the 4 bytes integer data type */
//#define XDR_INT64   3  /**< Used to code the 8 bytes integer data type */  // THE SAME!
#define XDR_REAL32  4  /**< Used to code the 4 bytes real data type */
#define XDR_REAL64  5  /**< Used to code the 8 bytes real data type */

#define MAX_DGRAM_SIZE   8192  /**< Maximum UDP datagram size. */
#define MAX_STRING_LEN 512   /**< Maximum string length (for hostnames). */
#define MAX_STRING_LEN1 (MAX_STRING_LEN + 1)
#define RET_SUCCESS  0  /**< Function return value (succes). */
#define RET_ERROR   -1  /**< Function return value (error). */
#define PROCUTILS_ERROR -2
#define RET_NOT_SENT -3 /**< A datagram was not sent because the number of
			   datagrams that can be sent per second is limited. */

#define MAX_N_DESTINATIONS 30  /**< Maximum number of destinations hosts to 
				  which we send the parameters. */

#define DEFAULT_PORT 8884 /**< The default port on which MonALISa listens. */
#define MAX_HEADER_LENGTH 40  /**< Maximum header length. */

/** Indicates that the object was initialized from a file. */
#define FILE_INIT  1
/** Indicates that the object was initialized from a list. */
#ifdef __APPLE__
#define OLIST_INIT  2
#else
#define LIST_INIT  2
#endif
/** Indicates that the object was initialized directly. */
#define DIRECT_INIT  3
/** Time interval (in sec) at which job monitoring datagrams are sent. */
#define JOB_MONITOR_INTERVAL 20
/** Time interval (in sec) at which system monitoring datagams are sent. */
#define SYS_MONITOR_INTERVAL 20
/** Time interval (in sec) at which the configuration files are checked
    for changes. */
#define RECHECK_INTERVAL 600
/** The number of time intervals at which ApMon sends general system monitoring
 * information (considering the time intervals at which ApMon sends system
 * monitoring information).
 */
#define GEN_MONITOR_INTERVALS 10
/** The maximum number of jobs that can be monitored. */
#define MAX_MONITORED_JOBS 35
/** The maximum number of system parameters. */
#define MAX_SYS_PARAMS 35
/** The maximum number of general system parameters. */
#define MAX_GEN_PARAMS 35
/** The maximum number of job parameters. */
#define MAX_JOB_PARAMS 35

/** The maxim number of mesages per second that will be sent to MonALISA */
#define MAX_MSG_RATE 20

#define NLETTERS 26

#define TWO_BILLION 2000000000

#define APMON_VERSION "2.2.8"

/**
 * Data structure which holds the configuration URLs.
 */
typedef struct ConfURLs {
  /** The number of webpages with configuration data. */
  int nConfURLs; 
  /** The addresses of the webpages with configuration data. */
  char *vURLs[MAX_N_DESTINATIONS];
  /** The "Last-Modified" line from the HTTP header for all the 
   * configuration webpages. */ 
  char *lastModifURLs[MAX_N_DESTINATIONS];
} ConfURLs;

/**
 * Data structure which holds information about a job monitored by ApMon.
 */
typedef struct MonitoredJob {
  long pid;
  /* the job's working dierctory */
  char workdir[MAX_STRING_LEN];
  /* the cluster name that will be included in the monitoring datagrams */
  char clusterName[50]; 
  /* the node name that will be included in the monitoring datagrams */
  char nodeName[50];
} MonitoredJob;

#ifdef WIN32
#define pthread_mutex_lock(mutex_ref) (WaitForSingleObject(*mutex_ref, INFINITE))
#define pthread_mutex_unlock(mutex_ref) (ReleaseMutex(*mutex_ref))
#define pthread_mutex_destroy(mutex_ref) (CloseHandle(*mutex_ref))
#define pthread_cond_signal(event_ref) (SetEvent(*event_ref))
#define pthread_cond_destroy(event_ref) (CloseHandle(*event_ref))
#define ETIMEDOUT WAIT_TIMEOUT
#endif

/**
 * Data structure used for sending monitoring data to a MonaLisa module. 
 * The data is packed in UDP datagrams, in XDR format.    
 * A datagram has the following structure:
 * - header which contains the ApMon version and the password for the MonALISA
 * host and has the following syntax: v:<ApMon_version>p:<password> 
 * - cluster name (string) 
 * - node name (string)
 * - number of parameters (int)
 * - for each parameter: name (string), value type (int), value
 * <BR>
 * There are two ways to send parameters:
 * 1) a single parameter in a packet (with the function sendParameter() which
 * has several overloaded variants
 * 2) multiple parameters in a packet (with the function sendParameters())
 *
 * Since v1.6 ApMon has the xApMon extension, which can be configured to send 
 * periodically, in a background thread, monitoring information regarding 
 * the system and/or some specified jobs.
 */
class ApMon {
 protected:
  char *clusterName; /**< The name of the monitored cluster. */
  char *nodeName; /**< The name of the monitored node. */ 

  /** The cluster name used when sending system monitoring datagrams. */
  char *sysMonCluster; 
  /** The node name used when sending system monitoring datagrams. */
  char *sysMonNode;

  int nDestinations; /**< The number of destinations to send the results to. */
  char **destAddresses; /**< The IP addresses where the results will be sent.*/
  int *destPorts; /**< The ports where the destination hosts listen. */
  char **destPasswds; /**< Passwords for the MonALISA hosts. */ 

  char *buf; /**< The buffer which holds the message data (encoded in XDR). */
  int dgramSize; /**< The size of the data inside the datagram (header not included) */
#ifndef WIN32
  int sockfd; /**< Socket descriptor */
#else
  SOCKET sockfd;
#endif
 
  /** If this flag is true, the configuration file / URLs are periodically
   * rechecked for changes. */
  bool confCheck;

  /** The number of initialization sources. */
  int nInitSources;
  /** The name(s) of the initialization source(s) (file or list). */
  char  **initSources;
  /* The initialization type (from file / list / directly). */
  int initType;

 /** The configuration file and the URLs are checked for changes at 
  * this numer of seconds (this value is requested by the user and will
  * be used if no errors appear when reloading the configuration). */
  long recheckInterval;

  /** If the configuration URLs cannot be reloaded, the interval until 
   * the next attempt will be increased. This is the actual value
   * (in seconds) of the interval that is used by ApMon.
   */
  long crtRecheckInterval;

#ifndef WIN32
  /** Background thread which periodically rechecks the configuration
      and sends monitoring information. */
  pthread_t bkThread;
  
  /** Used to protect the general ApMon data structures. */
  pthread_mutex_t mutex;

  /** Used to protect the variables needed by the background thread. */
  pthread_mutex_t mutexBack;
  
  /** Used for the condition variable confChangedCond. */
  pthread_mutex_t mutexCond;

  /** Used to notify changes in the monitoring configuration. */
  pthread_cond_t confChangedCond;
#else
 public:
  HANDLE bkThread;
  HANDLE mutex;
  HANDLE mutexBack;
  HANDLE mutexCond;
  HANDLE confChangedCond;
 protected:
#endif
  /** These flags indicate changes in the monitoring configuration. */
  bool recheckChanged, jobMonChanged,sysMonChanged;

  /** If this flag is true, the background thread is created (but
      not necessarily started). */
  bool haveBkThread;

  /** If this flag is true, the background thread is started. */
  bool bkThreadStarted;

  /** If this flag is true, there was a request to stop the background thread. */
  bool stopBkThread;

  /** If this flag is set to true, when the value of a parameter cannot be
   * read from proc/, ApMon will not attempt to include that value in the
   * next datagrams.
   */
  bool autoDisableMonitoring;

  /** If this flag is true, packets with system information taken from 
   * /proc are periodically sent to MonALISA.
   */
  bool sysMonitoring;

  /** If this flag is true, packets with job information taken from
   * /proc are periodically sent to MonALISA.
   */
  bool jobMonitoring;

 /** If this flag is true, packets with general system information taken from
   * /proc are periodically sent to MonALISA.
   */
  bool genMonitoring;

 /** Job/System monitoring information obtained from /proc is sent at these
   * time intervals. 
   */
  long jobMonitorInterval, sysMonitorInterval; 
  
  /** General system monitoring information is sent at a time interval equal
   * to genMonitorIntervals * sysMonitorInterval.
   */
  int genMonitorIntervals;

  /** Number of parameters that can be enabled/disabled by the user in
   * the system/job/general monitoring datagrams.
   */
  int nSysMonitorParams, nJobMonitorParams, nGenMonitorParams;
  
  /* The names of the parameters that can be enabled/disabled by the user in
   * the system/job/general monitoring datagrams.
   */
  char *sysMonitorParams[MAX_SYS_PARAMS];
  char *genMonitorParams[MAX_GEN_PARAMS];
  char *jobMonitorParams[MAX_JOB_PARAMS];

  /* Arrays of flags that specifiy the active monitoring parameters (the
   * ones that are sent in the datagams). 
   */
  int actSysMonitorParams[MAX_SYS_PARAMS];
  int actGenMonitorParams[MAX_GEN_PARAMS];
  int actJobMonitorParams[MAX_JOB_PARAMS];

  ConfURLs confURLs;

 /** The number of jobs that will be monitored */
  int nMonJobs;

  /** Array which holds information about the jobs to be monitored. */
  MonitoredJob *monJobs;

  /** The last time when the configuration file was modified. */
  long lastModifFile;

  /* The moment when the last datagram with job monitoring information
   * was sent. 
   */
  time_t lastJobInfoSend;

  /** The name of the user who owns this process. */
  char username[MAX_STRING_LEN];
  /** The group to which the user belongs. */
  char groupname[MAX_STRING_LEN];
  /** The name of the host on which ApMon currently runs. */
  char myHostname[MAX_STRING_LEN];
  /** The main IP address of the host on which ApMon currently runs. */
  char myIP[MAX_STRING_LEN];
 /** The number of IP addresses of the host. */
  int numIPs;
  /** A list with all the IP addresses of the host. */
  char allMyIPs[100][20];
  /** The number of CPUs on the machine that runs ApMon. */
  int numCPUs;

  bool sysInfo_first;
  /** The moment when the last system monitoring datagram was sent. */
  time_t lastSysInfoSend;
 /* The last recorded values for system parameters. */
  double lastSysVals[MAX_SYS_PARAMS];
  /* The current values for the system parameters */
  double currentSysVals[MAX_SYS_PARAMS];
  /* The success/error codes returned by the functions that calculate
     the system parameters */
  int sysRetResults[MAX_SYS_PARAMS];

  /* The current values for the job parameters */
  double currentJobVals[MAX_JOB_PARAMS];
  /* The success/error codes retuprorned by the functions that calculate
     the job parameters */
  int jobRetResults[MAX_JOB_PARAMS];

  /* The current values for the general parameters */
  double currentGenVals[MAX_GEN_PARAMS];
  /* The success/error codes returned by the functions that calculate
     the general parameters */
  int genRetResults[MAX_GEN_PARAMS];

  /* Table which stores the number of processes in each state 
     (R -runnable, S - sleeping etc.) Each entry in the table 
     corresponds to a capital letter. */
  double currentProcessStates[NLETTERS];

  /* CPU information: */
  char cpuVendor[100];
  char cpuFamily[100];
  char cpuModel[100];
  char cpuModelName[200];

  /** The names of the network interfaces. */
  char interfaceNames[100][20];
  /** The number of network interfaces. */
  int nInterfaces;
  /** The total number of bytes sent through each interface, when the
     previous system monitoring datagram was sent. */
  double lastBytesSent[20];
  double lastBytesReceived[20];
 /** The total number of network errors for each interface, when the
     previous system monitoring datagram was sent. */
  double lastNetErrs[20];
  /** The current values for the net_in, net_out, net_errs parameters */
  double *currentNetIn, *currentNetOut, *currentNetErrs;

  /** The number of open TCP, UDP, ICM and Unix sockets. */
  double currentNSockets[4];
  /** The number of TCP sockets in each possible state (ESTABLISHED, 
      LISTEN, ...) */
  double currentSocketsTCP[20];
  /** Table that associates the names of the TCP sockets states with the
      symbolic constants. */
  char *socketStatesMapTCP[20];  

  /* don't allow a user to send more than MAX_MSG messages per second, in average */
  int maxMsgRate;
  long prvTime;
  double prvSent;
  double prvDrop;
  long crtTime;
  long crtSent;
  long crtDrop;
  double hWeight;

  /** Random number that identifies this instance of ApMon. */
  int instance_id;
  /** Sequence number for the packets that are sent to MonALISA.
      MonALISA v 1.4.10 or newer is able to verify if there were 
      lost packets. */
  int seq_nr;

 private:
  /** Copy constructor */
  ApMon(const ApMon&);	// Not implemented
  
  /** The equals operator */
  ApMon& operator=(const ApMon&);	// Not implemented

 public:
  /**
   * Initializes an ApMon object from a configuration file or URL.
   * @param filename The name of the file/URL which contains the addresses and
   * the ports of the destination hosts, and also the passwords (see README 
   * for details about the structure of this file).
   */
  ApMon(char *initsource);
  

  /**
   * Initializes an ApMon data structure from a vector of strings. The strings
   * can be of the form hostname[:port] [passwd] or can be URLs from where the 
   * hostnames are to be read.
   */
  ApMon(int nDestinations, char **destinationsList);

  /**
   * Initializes an ApMon data structure, using arrays instead of a file.
   * @param nDestinations The number of destination hosts where the results will
   * be sent.
   * @param destAddresses Array that contains the hostnames or IP addresses 
   * of the destination hosts.
   * @param destPorts The ports where the MonaLisa modules listen on the 
   * destination hosts.
   * @param destPasswds The passwords for the MonALISA hosts.
   *
   */
  ApMon(int nDestinations, char **destAddresses, int *destPorts, char **destPasswds);

  /**
   * ApMon destructor.
   */
  ~ApMon();

  /**
   * Sends a parameter and its value to the MonALISA module. 
   * @param clusterName The name of the cluster that is monitored. If it is
   * NULL, we keep the same cluster and node name as in the previous datagram.
   * @param nodeName The name of the node from the cluster from which the 
   * value was taken.
   * @param paramName The name of the parameter.
   * @param valueType The value type of the parameter. Can be one of the 
   * constants XDR_INT32 (integer), XDR_REAL32 (float), XDR_REAL64 (double),
   * XDR_STRING (null-terminated string).
   * @param paramValue Pointer to the value of the parameter.
   * @return RET_SUCCESS (0) on success, RET_NOT_SENT (-3) if the message was 
   * not sent because the maximum number of messages per second was exceeded. 
   * On error an exception is thrown.
   */
  int sendParameter(char *clusterName, char *nodeName,
	       char *paramName, int valueType, char *paramValue);

  /**
   * Sends a parameter and its value to the MonALISA module, together with a 
   * timestamp.
   * @param clusterName The name of the cluster that is monitored. If it is
   * NULL, we keep the same cluster and node name as in the previous datagram.
   * @param nodeName The name of the node from the cluster from which the
   * value was taken.
   * @param paramName The name of the parameter.
   * @param valueType The value type of the parameter. Can be one of the
   * constants XDR_INT32 (integer), XDR_REAL32 (float), XDR_REAL64 (double),
   * XDR_STRING (null-terminated string).
   * @param paramValue Pointer to the value of the parameter.
   * @param timestamp The associated timestamp (in seconds).
   * @return RET_SUCCESS (0) on success, RET_NOT_SENT (-3) if the message was 
   * not sent because the maximum number of messages per second was exceeded. 
   * On error an exception is thrown.
   */
  int sendTimedParameter(char *clusterName, char *nodeName,
	      char *paramName, int valueType, char *paramValue, int timestamp);

  /**
   * Sends an integer parameter and its value to the MonALISA module. 
   * @param clusterName The name of the cluster that is monitored. If it is
   * NULL, we keep the same cluster and node name as in the previous datagram.
   * @param nodeName The name of the node from the cluster from which the 
   * value was taken.
   * @param paramName The name of the parameter.
   * @param paramValue The value of the parameter.
   * @return RET_SUCCESS (0) on success, RET_NOT_SENT (-3) if the message was 
   * not sent because the maximum number of messages per second was exceeded. 
   * On error an exception is thrown.
   */
  int sendParameter(char *clusterName, char *nodeName,
	       char *paramName, int paramValue);

  /**
   * Sends a parameter of type float and its value to the MonALISA module. 
   * @param clusterName The name of the cluster that is monitored. If it is
   * NULL, we keep the same cluster and node name as in the previous datagram.
   * @param nodeName The name of the node from the cluster from which the 
   * value was taken.
   * @param paramName The name of the parameter.
   * @param paramValue The value of the parameter.
   * @return RET_SUCCESS (0) on success, RET_NOT_SENT (-3) if the message was 
   * not sent because the maximum number of messages per second was exceeded. 
   * On error an exception is thrown.
   */
  int sendParameter(char *clusterName, char *nodeName,
	       char *paramName, float paramValue);

  /**
   * Sends a parameter of type double and its value to the MonALISA module. 
   * @param clusterName The name of the cluster that is monitored. If it is
   * NULL,we keep the same cluster and node name as in the previous datagram.
   * @param nodeName The name of the node from the cluster from which the 
   * value was taken.
   * @param paramName The name of the parameter.
   * @param paramValue The value of the parameter.
   * @return RET_SUCCESS (0) on success, RET_NOT_SENT (-3) if the message was 
   * not sent because the maximum number of messages per second was exceeded. 
   * On error an exception is thrown.
   */
  int sendParameter(char *clusterName, char *nodeName,
	       char *paramName, double paramValue);

  /**
   * Sends a parameter of type string and its value to the MonALISA module. 
   * @param clusterName The name of the cluster that is monitored. If it is
   * NULL, we keep the same cluster and node name as in the previous datagram.
   * @param nodeName The name of the node from the cluster from which the 
   * value was taken.
   * @param paramName The name of the parameter.
   * @param paramValue The value of the parameter.
   * @return RET_SUCCESS (0) on success, RET_NOT_SENT (-3) if the message was 
   * not sent because the maximum number of messages per second was exceeded. 
   * On error an exception is thrown.
   */
  int sendParameter(char *clusterName, char *nodeName,
	       char *paramName, char *paramValue);


  /**
   * Sends a parameter of type string and its value to the MonALISA module. 
   * @param clusterName The name of the cluster that is monitored.If it is
   * NULL, we keep the same cluster and node name as in the previous datagram.
   * @param nodeName The name of the node from the cluster from which the 
   * value was taken.
   * @param paramName The name of the parameter.
   * @param paramValue The value of the parameter.
   * @return RET_SUCCESS (0) on success, RET_NOT_SENT (-3) if the message was 
   * not sent because the maximum number of messages per second was exceeded. 
   * On error an exception is thrown.
   */
  int sendParameters(char *clusterName, char *nodeName,
	       int nParams, char **paramNames, int *valueTypes, 
			 char **paramValues);

  /**
   * Sends a set of parameters and their values to the MonALISA module, 
   * together with a timestamp.
   * @param clusterName The name of the cluster that is monitored.  If it is
   * NULL, we keep the same cluster and node name as in the previous datagram.
   * @param nodeName The name of the node from the cluster from which the
   * value was taken.
   * @param nParams The number of parameters to be sent.
   * @param paramNames Array with the parameter names.
   * @param valueTypes Array with the value types represented as integers.
   * @param paramValue Array with the parameter values.
   * @param timestamp The timestamp (in seconds) associated with the data.
   * @return RET_SUCCESS (0) on success, RET_NOT_SENT (-3) if the message was 
   * not sent because the maximum number of messages per second was exceeded. 
   * On error an exception is thrown.
   */
  int sendTimedParameters(char *clusterName, char *nodeName,
	       int nParams, char **paramNames, int *valueTypes, 
	       char **paramValues, int timestamp);

  /**
   * Returns the value of the confCheck flag. If it is true, the 
   * configuration file and/or the URLs are periodically checked for
   * modifications.
   */
  bool getConfCheck() { return confCheck; }

  /**
   * Returns the value of the time interval (in seconds) between two recheck 
   * operations for the configuration files.
   * If error(s) appear when reloading the configuration, the actual interval will
   * be increased (transparently for the user).
   */
  long getRecheckInterval() { return recheckInterval; }


  /**
   * Sets the value of the time interval (in seconds) between two recheck 
   * operations for the configuration files. The default value is 5min.
   * If the value is negative, the configuration rechecking is turned off.
   * If error(s) appear when reloading the configuration, the actual interval will
   * be increased (transparently for the user).
   */
  void setRecheckInterval(long val);

  /** Enables/disables the periodical check for changes in the configuration
   * files/URLs. 
   * @param confRecheck If it is true, the periodical checking is enabled.
   * @param interval The time interval at which the verifications are done. If 
   * it is negative, a default value will be used.
   */
  void setConfRecheck(bool confRecheck, long interval);

  /** Enables/disables the periodical check for changes in the configuration
   * files/URLs. If enabled, the verifications will be done at the default 
   * time interval.
   */  
  void setConfRecheck(bool confRecheck) {
    setConfRecheck(confRecheck, RECHECK_INTERVAL);
  }

  /** Enables/disables the periodical sending of datagrams with job monitoring
   * information.
   * @param jobMonitoring If it is true, the job monitoring is enabled
   * @param interval The time interval at which the datagrams are sent. If 
   * it is negative, a default value will be used.
   */ 
  void setJobMonitoring(bool bJobMonitoring, long interval);

  /** Enables/disables the job monitoring. If the job monitoring is enabled, 
   * the datagrams will be sent at the default time interval.
   */  
  void setJobMonitoring(bool bJobMonitoring) {
    setJobMonitoring(bJobMonitoring, JOB_MONITOR_INTERVAL);
  }

  /** Returns the interval at which job monitoring datagrams are sent. If the
   * job monitoring is disabled, returns -1.
   */
  long getJobMonitorInterval() {
    long i = -1;
    pthread_mutex_lock(&mutexBack);
    if (jobMonitoring)
      i = jobMonitorInterval;
    pthread_mutex_unlock(&mutexBack);
    return i;
  }

  /** Returns true if the job monitoring is enabled, and false otherwise. */
  bool getJobMonitoring() {
    bool b;
    pthread_mutex_lock(&mutexBack);
    b = jobMonitoring;
    pthread_mutex_unlock(&mutexBack);
    return b;
  }

  /** Enables/disables the periodical sending of datagrams with system 
   * monitoring information.
   * @param sysMonitoring If it is true, the system monitoring is enabled
   * @param interval The time interval at which the datagrams are sent. If 
   * it is negative, a default value will be used.
   */ 
  void setSysMonitoring(bool bSysMonitoring, long interval); 

  /** Enables/disables the system monitoring. If the system monitoring is 
   * enabled, the datagrams will be sent at the default time interval.
   */  
  void setSysMonitoring(bool bSysMonitoring) {
    setSysMonitoring(bSysMonitoring, SYS_MONITOR_INTERVAL);
  }

  /** Returns the interval at which system monitoring datagrams are sent. If 
   * the job monitoring is disabled, returns -1.
   */
  long getSysMonitorInterval() {
    long i = -1;
    pthread_mutex_lock(&mutexBack);
    if (sysMonitoring)
      i = sysMonitorInterval;
    pthread_mutex_unlock(&mutexBack);
    return i;
  }

  /** Returns true if the system monitoring is enabled, and false otherwise. */
  bool getSysMonitoring() {
    bool b;
    pthread_mutex_lock(&mutexBack);
    b = sysMonitoring;
    pthread_mutex_unlock(&mutexBack);
    return b;
  }

  /** Enables/disables the periodical sending of datagrams with general system 
   * information.
   * @param genMonitoring If it is true, enables the sending of the datagrams.
   * @param interval The number of time intervals at which the datagrams are 
   * sent (considering the interval for sending system monitoring information).
   * If it is negative, a default value will be used.
   */ 
  void setGenMonitoring(bool bGenMonitoring, int nIntervals);

  /**Enables/disables the sending of datagrams with general system information.
   * A default value is used for the number of time intervals at which the
   * datagrams are sent.
   */
  void setGenMonitoring(bool bGenMonitoring) {
    setGenMonitoring(bGenMonitoring, GEN_MONITOR_INTERVALS);
  }

  /** Returns true if the sending of general system information is enabled and
   * false otherwise.
   */
  bool getGenMonitoring() {
    bool b;
    pthread_mutex_lock(&mutexBack);
    b = genMonitoring;
    pthread_mutex_unlock(&mutexBack);
    return b;
  }

  /**
   * Adds a new job to the list of the jobs monitored by ApMon.
   * @param pid The job's PID.
   * @param workdir The working directory of the job. If it is NULL or if it
   * has a zero length, directory monitoring will be disabled for this job.
   * @param clusterName The cluster name associated with the monitoring data
   * for this job in MonALISA.
   * @param nodeName The node name associated with the monitoring data
   * for this job in MonALISA.
   */
  void addJobToMonitor(long pid, char *workdir, char *clusterName,
		       char *nodeName);

  /**
   * Removes a job from the list of the jobs monitored by ApMon.
   * @param pid The pid of the job to be removed. 
   */
  void removeJobToMonitor(long pid);

  /** This function is called by the user to set the cluster name and the node 
    name for the system monitoring datagrams.*/
  void setSysMonClusterNode(char *clusterName, char *nodeName);

  /** Sets the ApMon logging level. Possible values are 0 (FATAL), 1 (WARNING),
      2 (INFO), 3 (FINE), 4 (DEBUG);
  */
  static void setLogLevel(char *newLevel_s);

  /**
   * This sets the maxim number of messages that are send to MonALISA in one second.
   * Default, this number is 50.
   */ 
  void setMaxMsgRate(int maxRate);

  /**
   * Displays an error message and exits with -1 as return value.
   * @param msg The message to be displayed.
   */
  static void errExit(char *msg);


 protected:

   /**
   * Initializes an ApMon object from a configuration file.
   * @param filename The name of the file which contains the addresses and
   * the ports of the destination hosts (see README for details about
   * the structure of this file).
   * @param firstTime If it is true, all the initializations will be done (the
   * object is being constructed now). Else, only some structures will be 
   * reinitialized.
   */
  void initialize(char *filename, bool firstTime);

  /** Initializes an ApMon object from a list with URLs and destination hosts. */
  void constructFromList(int nDestinations, char **destinationsList);

  /**
   * Initializes an ApMon object from a list with URLs and destination hosts.
   * @param nDestinations The number of elements in destList.
   * @param destList The list with URLs.
   * @param firstTime If it is true, all the initializations will be done 
   * (the object is being constructed now). Else, only some structures will 
   * be reinitialized.
   */
  void initialize(int nDestinations, char **destList, bool firstTime);


  /**
   * Parses a configuration file which contains addresses, ports and 
   * passwords for the destination hosts and puts the results in the
   * vectors given as parameters.
   * @param filename The name of the configuration file.
   * @param nDestinations Output parameter, will contain the number of 
   * destination hosts.
   * @param destAddresses Will contain the destination addresses.
   * @param destPorts Will contain the ports from the destination hosts. 
   * @param destPasswds Will contain the passwords for the destination hosts.
   */
  void loadFile(char *filename, int *nDestinations, char **destAddresses,
		int *destPorts, char **destPasswds);

 
   /**
   * Internal function that initializes an ApMon data structure.
   * @param nDestinations The number of destination hosts where the results will
   * be sent.
   * @param destAddresses Array that contains the hostnames or IP addresses 
   * of the destination hosts.
   * @param destPorts The ports where the MonaLisa modules listen on the 
   * destination hosts.
   * @param destPasswds Passwords for the destination hosts.
   *
   */
  void arrayInit(int nDestinations, char **destAddresses, int *destPorts,
		 char **destPasswds);

  /**
   * Internal function that initializes an ApMon data structure.
   * @param nDestinations The number of destination hosts where the results will
   * be sent.
   * @param destAddresses Array that contains the hostnames or IP addresses 
   * of the destination hosts.
   * @param destPorts The ports where the MonaLisa modules listen on the 
   * destination hosts.
   * @param destPasswds Passwords for the destination hosts.
   * @param firstTime If it is true, all the initializations will be done (the
   * object is being constructed now). Else, only some structures will be 
   * reinitialized.
   */
  void arrayInit(int nDestinations, char **destAddresses, int *destPorts,
		      char **destPasswds, bool firstTime);

  /**
   * Parses the string line, which has the form hostname:port, and
   * adds the hostname and the port to the lists given as parameters.
   * @param line The line to be parsed.
   * @param nDestinations The number of destination hosts in the lists.
   * Will be modified (incremented) in the function.
   * @param destAddresses The list with IP addresses or hostnames.
   * @param destPorts The list of corresponding ports.
   * @param destPasswds Passwords for the destination hosts.
   */ 
  void addToDestinations(char *line, int *nDestinations, 
		char *destAddresses[], int destPorts[], char *destPasswds[]); 

  /**
   * Gets a configuration file from a web location and adds the destination
   * addresses and ports to the lists given as parameters.
   */
  void getDestFromWeb(char *url, int *nDestinations, char *destAddresses[], 
		 int destPorts[], char *destPasswds[],
		      ConfURLs& confURLs);

 
  /**
   * Encodes in the XDR format the data from a ApMon structure. Must be 
   * called before sending the data over the newtork.
   */ 
  void encodeParams(int nParams, char **paramNames, int *valueTypes, 
		 char **paramValues, int timestamp);

  /** Initializes the monitoring configurations and the names of the parameters
   * included in the monitoring datagrams.
   */
  void initMonitoring();

  /** Sends datagrams containing information about the jobs that are
   * currently being monitored. 
   */
  void sendJobInfo();

  /** Sends datagrams with monitoring information about the specified job
   * to all the destination hosts.
   */
  void sendOneJobInfo(MonitoredJob job);

  /** Update the monitoring information regarding the specified job. */
  void updateJobInfo(MonitoredJob job); 

 /** Sends datagrams with system monitoring information to all the destination
     hosts. */ 
  void sendSysInfo();

  /** Update the system monitoring information with new values obtained 
      from the proc/ filesystem. */
  void updateSysInfo();

  /** Sends datagrams with general system monitoring information to all the
      destination hosts. */
  void sendGeneralInfo();

  /** Update the general monitoring information. */
  void updateGeneralInfo();

 /**
  * Sets the value of the confCheck flag. If it is true, the 
  * configuration file and/or the URLs will be periodically checked for
  * modifications. By default it is false. 
  */
  void setBackgroundThread(bool val);

 /**
  * Returns the actual value of the time interval (in seconds) between two 
  * recheck operations for the configuration files.
  */
  long getCrtRecheckInterval() {
    return crtRecheckInterval;
  }

  void setCrtRecheckInterval(long val);

  
  /**
   * Frees the data structures needed to hold the configuratin settings.
   */
  void freeConf();

  /**
   * This function is executed in a background thread and has two 
   * roles: it automatically sends the system/job monitoring parameters (if
   * the user requested) and it checks the configuration file/URLs for 
   * changes.
   */
#ifndef WIN32
  friend void *bkTask(void *param);
#else
  friend DWORD WINAPI bkTask(void *param);
#endif
  
  /** Parses an xApMon line from the configuration file and sets the 
   * corresponding parameters in the ApMon object.
   */
  void parseXApMonLine(char *line);

  /** Initializes the UDP socket used to send the datagrams. */
  void initSocket();

  /** Parses the contents of a configuration file. The destination addresses
      and ports are stored in the arrays given as parameters.
  */
  void parseConf(FILE *fp, int *nDestinations, char **destAddresses, 
		     int *destPorts, char **destPasswds);

  /**
   * Decides if the current datagram should be sent (so that the maximum
   * number of datagrams per second is respected in average).
   * This decision is based on the number of messages previously sent.
   */ 
  bool shouldSend();

  friend class ProcUtils;
};

 /**
  * Performs background actions like rechecking the configuration file and 
  * the URLs and sending monitoring information. (this is done in a separate
  * thread).
  */
#ifndef WIN32
void *bkTask(void *param); // throw(runtime_error);
#else
DWORD WINAPI bkTask(void *param);
#endif
#endif








