/**
 * \file proc_utils.h
 * This file contains declarations for various functions that extract 
 * information from the proc/ filesystem.
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

#ifndef apmon_procutils_h
#define apmon_procutils_h

#include <time.h>
#include <stdexcept>

#ifndef WIN32
#include <sys/wait.h>
#endif

using namespace std;
 
/** Used when throwing exceptions from the proc_utils functions. When a
 * proc_utils_error is being thrown, a certain parameter could not be
 * obtained from proc/ and ApMon will not make further attempts to read
 * that parameter (if the autoDisable flag is on). 
 */
class procutils_error : public runtime_error {
 public:
  procutils_error(const char *msg) : runtime_error(msg) {}
};


class ProcUtils {

 public:  
  /** Calculates the parameters cpu_usr, cpu_sys, cpu_nice, cpu_idle,
      cpu_usage and stores them in the output parameters cpuUsr, cpuSys,...
  */
  static void getCPUUsage(ApMon& apm, double& cpuUsage, 
			       double& cpuUsr, double& cpuSys, 
			       double& cpuNice, double& cpuIdle,
			       double& cpuIOWait, double& cpuIRQ,
			       double& cpuSoftIRQ, double& cpuSteal,
			       double& cpuGuest,
			       int numCPUs);

   /** Calculates the parameters pages_in, pages_out, swap_in, swap_out,
      cpu_usage and stores them in the output parameters pagesIn, pagesOut,...
   */
  static void getSwapPages(ApMon& apm, double& pagesIn, 
			       double& pagesOut, double& swapIn, 
			     double& swapOut);

  /**
   * Obtains the CPU load in the last 1, 5 and 15 mins and the number of 
   * processes currently running and stores them in the variables given 
   * as parameters.
   */
  static void getLoad(double& load1, double& load5, double& load15, 
	       double& processes);

  /**
   * Obtains statistics about the total number of processes and
   * the number of processes in each state.
   * Possible states: 
   *   D uninterruptible sleep (usually IO)
   *   R runnable (on run queue)
   *   S sleeping
   *   T traced or stopped
   *   W paging (2.4 kernels and older only)
   *   X dead
   *   Z a defunct ("zombie") process
   */
  static void getProcesses(double& processes, double states[]);

  /**
   * Obtains the total amount of memory and the total amount of swap (in KB)
   * and stores them in the variables given as parameters.
   */
  static void getSysMem(double& totalMem, double& totalSwap);

  /**
   * Obtains the amount of memory and of swap currently in use and stores
   * them in the variables given as parameters.
   */
  static void getMemUsed(double& usedMem, double&freeMem, double &usedSwap,
		  double& freeSwap);

  /**
   * Obtains the names of the network interfaces (excepting the loopback one).
   * @param nInterfaces Output parameter which will store the number of 
   * network interfaces.
   * @param names Output parameter which will store the names of the
   * interfaces.
   */
  static void getNetworkInterfaces(int &nInterfaces, char names[][20]);

  /**
   * Computes the input/output traffic for all the network interfaces,
   * in kilobytes per second averaged over the interval between the moment
   * when the last system monitoring datagram was sent and the present moment.
   * @param apm The ApMon object used for monitoring.
   * @param vNetIn Points to an array with the average values of bytes 
   * received per second by each interface.
   * @param vNetOut  Points to an array with the average values of bytes 
   * transmitted per second by each interface. 
   * @param vNetErrs  Points to an array with the number of errors for each 
   * interface. 
   */  
  static void getNetInfo(ApMon& apm, double **vNetIn, double **vNetOut, 
		  double **vNetErrs);

  /**
   * Obtains information about the currently opened sockets.
   * @param nsockets Output parameter that holds a table with the number of 
   * TCP, UDP, ICM, Unix sockets.
   * @param tcp_states Output parameter that holds a table with the number
   * of TCP sockets in each possible state (ESTABLISHED, LISTEN, ...).
   */
  static void getNetstatInfo(ApMon& apm, double nsockets[], double 
			     tcp_states[]);

  /**
   * Returns the number of CPUs in the system.
   */
  static int getNumCPUs();

  /**
   * Obtains CPU information (vendor, model, frequency, bogomips) and fills the
   * corresponding fields in the ApMon object.
   */
  static void getCPUInfo(ApMon& apm);

  /**
   * Returns the system boot time, in seconds since the Epoch.
   */
  static long getBootTime();

  /**
   * Returns the system uptime in days.
   */
  static double getUpTime();


  /**
   * Obtains the number of opened file descriptors for the process with
   * the given pid.
   */
  static int countOpenFiles(long pid);
};

#endif
