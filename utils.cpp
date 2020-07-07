/**
 * \file utils.cpp
 * This file contains the implementations of some helper methods for ApMon.
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
#include "utils.h"

bool apmon_utils::urlModified(char *url, char *lastModified) {
  char temp_filename[300]; 
  FILE *tmp_file;
  bool lineFound;
  char line[MAX_STRING_LEN1];

#ifndef WIN32
  long mypid = getpid();
#else
  long mypid = _getpid();
#endif

  char str1[100], str2[100];
#ifndef WIN32
  snprintf(temp_filename, 299, "/tmp/apmon_http%ld", mypid);
#else
  char *tmp = getenv("TEMP");
  if(tmp == NULL)
	  tmp = getenv("TMP");
  if(tmp == NULL)
	  tmp = "c:";
  snprintf(temp_filename, 299, "%s\\apmon_http%ld", tmp, mypid);
#endif
  /* get the HTTP header and put it in a temporary file */
  httpRequest(url, "HEAD", temp_filename);

   /* read the header from the temporary file */
  tmp_file = fopen(temp_filename, "rt");
  if (tmp_file == NULL)
    return false;

  //line = (char*)malloc(MAX_STRING_LEN * sizeof(char));

  //check if we got the page correctly
  char *retGet = fgets(line, MAX_STRING_LEN, tmp_file);
  if (retGet == NULL)
    return false;

  sscanf(line, "%s %s", str1, str2);
  if (atoi(str2) != 200) {
    fclose(tmp_file);
    unlink(temp_filename);
    return false;
  }

  // look for the "Last-Modified" line
  lineFound = false;
  while (fgets(line, MAX_STRING_LEN, tmp_file) != NULL) {
    if (strstr(line, "Last-Modified") == line) {
      lineFound = true;
      break;
    }
  }
 
  fclose(tmp_file); 
  unlink(temp_filename);
  if (lineFound) {
    if (strcmp(line, lastModified) != 0) {
      return true;
    }
    else
      return false;
  } else
    // if the line was not found we must assume the page was modified
    return true;

}  

int apmon_utils::httpRequest(char *url, const char *reqType, char *temp_filename) {
  // the server from which we get the configuration file
  char hostname[MAX_STRING_LEN]; 
  // the name of the remote file
  char filename[MAX_STRING_LEN];
  // the port on which the server listens (by default 80)
  int port;
  char msg[MAX_STRING_LEN*3];
  
  int sd, rc;
  struct sockaddr_in localAddr, servAddr;
  struct hostent *h;
  struct timeval optval;
  size_t retWrite;

  char *request; // the HTTP request

  char buffer[MAX_STRING_LEN]; // for reading from the socket
  int totalSize; // the size of the remote file
  FILE *tmp_file; 

  parse_URL(url, hostname, &port, filename);

  snprintf(msg, MAX_STRING_LEN*3-1, "Sending HTTP %s request to: \n Hostname: %s , Port: %d , Filename: %s", reqType, hostname, port, filename);
  logger(INFO, msg);
  
  request = (char *)malloc(MAX_STRING_LEN * sizeof(char));
  strncpy(request, reqType, MAX_STRING_LEN-1);
  strncat(request, " ", MAX_STRING_LEN-strlen(request)-1);

  strncat(request, filename, MAX_STRING_LEN-strlen(request)-1);
  strncat( request, " HTTP/1.0\r\nHOST: ", MAX_STRING_LEN-strlen(request)-1);
  strncat( request, hostname, MAX_STRING_LEN-strlen(request)-1);
  strncat( request, "\r\n\r\n", MAX_STRING_LEN-strlen(request)-1);

  h = gethostbyname(hostname);
  if(h==NULL) {
    free(request);
    snprintf(msg, MAX_STRING_LEN*3-1, "[ httpRequest() ] Unknown host: %s ", hostname);
    return -1;
  }

  servAddr.sin_family = h->h_addrtype;
  memcpy((char *) &servAddr.sin_addr.s_addr, h->h_addr_list[0], h->h_length);
  servAddr.sin_port = htons(port);//(LOCAL_SERVER_PORT);

  sd = socket(AF_INET, SOCK_STREAM, 0);
  if(sd<0) {
    free(request);
    return -1;
  }

  /* set connection timeout */
  
  optval.tv_sec = 10;
  optval.tv_usec = 0;
  //setsockopt(sd, SOL_SOCKET, SO_SNDTIMEO, (char *) &optval, 
  //		sizeof(optval));
  setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, (char *) &optval, 
			sizeof(optval));
  
  localAddr.sin_family = AF_INET;
  localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  localAddr.sin_port = htons(0);
  
  /*
  rc = bind(sd, (struct sockaddr *) &localAddr, sizeof(localAddr));
  if(rc<0) {
    free(request);
    close(sd);
    snprintf(msg, MAX_STRING_LEN-1, "%s: cannot bind port TCP %u", url,port);
    throw runtime_error(msg);
  }
  */
				
  // connect to the server
  rc = connect(sd, (struct sockaddr *) &servAddr, sizeof(servAddr));
  if(rc<0) {
    free(request);
#ifndef WIN32
    close(sd);
#else
	closesocket(sd);
#endif
    return -1;
  }

  // send the request
  rc = send(sd, request, strlen(request), 0);
  if(rc<0) {  
#ifndef WIN32
	close(sd);
#else
	closesocket(sd);
#endif
    free(request);
    return -1;
  }
 
  free(request);

  /* read the response and put it in a temporary file */
  tmp_file = fopen(temp_filename, "wb");
  if (tmp_file == NULL) {
#ifndef WIN32
    close(sd);
#else
	closesocket(sd);
#endif
    return -1;
  }

  rc = 0, totalSize = 0;
  do {
    memset(buffer,0x0,MAX_STRING_LEN);    //  init line 
    rc = recv(sd, buffer, MAX_STRING_LEN, 0);
    if( rc > 0) { 
      retWrite = fwrite(buffer, rc, 1, tmp_file);
      if (retWrite < 1)
        break;
      totalSize += rc;
    }
  }while(rc>0);

  snprintf(msg, MAX_STRING_LEN*3-1, "Received response from  %s, response size is %d bytes",  hostname, totalSize);
  logger(INFO, msg);

#ifndef WIN32
  close(sd);
#else
  closesocket(sd);
#endif
  fclose(tmp_file);
  return totalSize;
}

char *apmon_utils::findIP(char *address) {
  int isIP = 1;
  char *destIP, *s;
  struct in_addr addr;
  unsigned int j;
  bool ipFound;

  for (j = 0; j < strlen(address); j++) 
      if (isalpha(address[j])) {
	// if we found a letter, this is not an IP address
	isIP = 0;
	break;
      }
     
    if (!isIP) {  // the user provided a hostname, find the IP
      struct hostent *he = gethostbyname(address);
      if (he == NULL) {
	char tmp_msg[128];
	snprintf(tmp_msg, 127, "[ findIP() ] Invalid destination address %s", address);
        return 0;
      }
      j = 0;
      /* get from the list the first IP address 
	 (which is not a loopback one) */
      ipFound = false;
      while ((he -> h_addr_list)[j] != NULL) {
	memcpy(&(addr.s_addr), (he -> h_addr_list)[j], 4);
	s = inet_ntoa(addr);
	if (strcmp(s, "127.0.0.1") != 0) {
	  destIP = strdup(s);
	  ipFound = true;
	  break;
	}
	j++;
      }
      if (!ipFound) {
	destIP = strdup("127.0.0.1");
	fprintf(stderr, "The destination for datagrams is localhost\n");
      }
    
    } else // the string was an IP address
      destIP = strdup(address);
    
    return destIP;
}


void apmon_utils::parse_URL(char *url, char *hostname, int *port, char *identifier) {
    char protocol[MAX_STRING_LEN], scratch[MAX_STRING_LEN], *ptr=0, *nptr=0;
    char msg[MAX_STRING_LEN];

    strncpy(scratch, url, MAX_STRING_LEN-1);
    ptr = (char *)strchr(scratch, ':');
    if (!ptr)
      return;

    strcpy(ptr, "\0");
    strncpy(protocol, scratch, MAX_STRING_LEN-1);
    if (strcmp(protocol, "http")) {
	return;
    }

    strncpy(scratch, url, MAX_STRING_LEN-1);
    ptr = (char *)strstr(scratch, "//");
    if (!ptr) {
	return;
    }
    ptr += 2;

    strncpy(hostname, ptr, MAX_STRING_LEN-1);
    nptr = (char *)strchr(ptr, ':');
    if (!nptr) {
		*port = 80; /* use the default HTTP port number */
		nptr = (char *)strchr(hostname, '/');
    } else {	
		sscanf(nptr, ":%d", port);
		nptr = (char *)strchr(hostname, ':');
    }

    if (nptr)
      *nptr = '\0';

    nptr = (char *)strchr(ptr, '/');
    if (!nptr) {
	return;
    }
    
    strncpy(identifier, nptr, MAX_STRING_LEN-1);
}


void apmon_utils::freeMat(char **mat, int nRows) {
  int i;
  for (i = 0; i < nRows; i++)
    free(mat[i]);
  free(mat);
}

char *apmon_utils::trimString(char *s) {
  unsigned int i, j, firstpos, lastpos;
  char *ret = (char *)malloc((strlen(s) + 1) * sizeof(char));
  j = 0;

  // find the position of the first non-space character in the string
  for (i = 0; i < strlen(s); i++)
    if (!isspace(s[i]))
      break;
  firstpos = i; 

  if (firstpos == strlen(s)) {
    ret[0] = 0;
    return ret;
  }

  // find the position of the last non-space character in the string
  for (i = strlen(s) - 1; i >= 0; i--)
    if (!isspace(s[i]))
	break;
  lastpos = i; 

  for (i = firstpos; i <= lastpos; i++)
      ret[j++] = s[i];

  ret[j++] = 0;
  return ret;
}

int apmon_utils::xdrSize(int type, char *value) {
  int size;
  
  switch (type) {
//  case XDR_INT16: (not supported)
  case XDR_INT32:
  case XDR_REAL32:
    return 4;
//  case XDR_INT64:  (not supported)
  case XDR_REAL64:
    return 8;
  case XDR_STRING:
    /* XDR adds 4 bytes to hold the length of the string */
    //size = (strlen(value) + 1) + 4;
    if (value == NULL) {
      logger(WARNING, "[ xdrSize() ] null string argument");
      size = 4;
    } else {
      size = strlen(value) + 4;
      /* the length of the XDR representation must be a multiple of 4,
	 so there might be some extra bytes added*/
      if (size % 4 != 0)
	size += (4 - size % 4);
      return size;
    }
  }
  
  return RET_ERROR;
}

int apmon_utils::sizeEval(int type, char *value) {
  
  switch (type) {
//  case XDR_INT16:
  case XDR_INT32:
  case XDR_REAL32:
    return 4;
//  case XDR_INT64:
  case XDR_REAL64:
    return 8;
  case XDR_STRING:
    return (strlen(value) + 1);
  }
  
  return RET_ERROR;
}

void apmon_utils::logParameters(int level, int nParams, char **paramNames, 
		     int *valueTypes, char **paramValues) {
  int i;
  char typeNames[][15] = {"XDR_STRING", "", "XDR_INT32", "", "XDR_REAL32",  "XDR_REAL64"};
  char logmsg[MAX_STRING_LEN], val[MAX_STRING_LEN];

  int remaining = MAX_STRING_LEN-1;

  for (i = 0; i < nParams; i++) {
    if (paramNames[i] == NULL || (valueTypes[i] == XDR_STRING &&
				  paramValues[i] == NULL))
      continue;
    snprintf(logmsg, MAX_STRING_LEN-1, "%s (%s) ", paramNames[i], typeNames[valueTypes[i]]);
    //printf("%s () ", paramNames[i]);
    switch(valueTypes[i]) {
    case XDR_STRING:
      snprintf(val, MAX_STRING_LEN-1, "%s", paramValues[i]);
      break;
    case XDR_INT32:
      snprintf(val, MAX_STRING_LEN-1, "%d", *(int *)paramValues[i]);
      break;
    case XDR_REAL32:
      snprintf(val, MAX_STRING_LEN-1, "%f", *(float *)paramValues[i]);
      break;
    case XDR_REAL64:
      snprintf(val, MAX_STRING_LEN-1, "%lf", *(double *)(paramValues[i]));
      break;  
    }
    
    strncat(logmsg, val,remaining);
    logger(level, logmsg);
    
    remaining -= strlen(val);
    
    if (remaining<=0)
    	break;
  }
}


bool apmon_utils::isPrivateAddress(char *addr) {
  char *s1, *s2;
  int n1, n2;
  char tmp[MAX_STRING_LEN];
//  char buf[MAX_STRING_LEN];
//  char *pbuf = buf;

  strncpy(tmp, addr, MAX_STRING_LEN-1);
  s1 = strtok/*_r*/(tmp,".");//, &pbuf); 
  n1 = atoi(s1);

  s2 = strtok/*_r*/(NULL, ".");//, &pbuf);
  n2 = atoi(s2);

  if (n1 == 10)
    return true;
  if (n1 == 172 && n2 >= 16 && n2 <= 31)
    return true;
  if (n1 == 192 && n2 == 168)
    return true;

  return false;
}

int apmon_utils::getVectIndex(const char *item, char **vect, int vectDim) {
  int i;

  for (i = 0; i < vectDim; i++)
    if (strcmp(item, vect[i]) == 0)
      return i;
  return -1;
}
  
void apmon_utils::logger(int msgLevel, const char *msg, int newLevel) {
  char time_s[30];
  int len;
  long crtTime = time(NULL);
  const char * const levels[5] = {"FATAL", "WARNING", "INFO", "FINE", "DEBUG"};
  static int loglevel = INFO;
#ifndef WIN32
  static pthread_mutex_t logger_mutex;
#else
  static HANDLE logger_mutex;
#endif
  static bool firstTime = true;

  if (firstTime) {
#ifndef WIN32
    pthread_mutex_init(&logger_mutex, NULL);
#else
	logger_mutex = CreateMutex(NULL, FALSE, NULL);
#endif
    firstTime = false;
  }

  pthread_mutex_lock(&logger_mutex);

#ifndef WIN32
  char cbuf[50];
  strncpy(time_s, ctime_r(&crtTime, cbuf), 29);
#else
  strncpy(time_s, ctime(&crtTime), 29);
#endif

  len = strlen(time_s); time_s[len - 1] = 0;

  if (newLevel >= 0 && newLevel <=4) {
    loglevel = newLevel;
    if (loglevel>=2)
      printf("[%s] Changed the logging level to %s\n", time_s, levels[newLevel]);
  } else {
    if (msgLevel >= 0 && msgLevel <= 4) {
      if (msgLevel <= loglevel)
	printf("[%s] [%s] %s\n",time_s, levels[msgLevel], msg);
    } else
      printf("[WARNING] Invalid logging level %d!\n", msgLevel);
  }
  pthread_mutex_unlock(&logger_mutex);
}
