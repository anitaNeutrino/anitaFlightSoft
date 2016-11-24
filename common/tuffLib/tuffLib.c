#include "tuffLib.h"
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <syslog.h>
#include <fcntl.h>
#include <sys/select.h>

#define _GNU_SOURCE
static char buf[1024];
#define BAUDRATE B115200

struct tuff_dev 
{
  int fd; 
  struct termios oldtio; 
}; 



/*
// eats all available bytes. Yummy! 
static int bytegobbler(int fd) 
{

  int nbytes; 
  int nread = 0; 
  char stomach[16]; 
  ioctl(fd, FIONREAD, &nbytes); 

  while (nbytes > 0) 
  {
    nread += read(fd, stomach, nbytes - nread> 16 ? 16 : nbytes - nread); 
  }

  return nread; 
}
*/



int tuff_reset(tuff_dev_t * d,
		  unsigned int irfcm) {
  sprintf(buf, "\r\n{\"reset\":%d}\r\n", irfcm);
  return write(d->fd, buf, strlen(buf));
}

int tuff_onCommand(tuff_dev_t * d,
		      unsigned int irfcm,
		      unsigned int channel,
		      unsigned int notchMask) {
  sprintf(buf, "\r\n{\"on\":[%d,%d,%d]}\r\n",irfcm, channel, notchMask);
  return write(d->fd, buf, strlen(buf));
}

int tuff_offCommand(tuff_dev_t * d,
		       unsigned int irfcm,
		       unsigned int channel,
		       unsigned int notchMask) {
  sprintf(buf, "\r\n{\"off\":[%d,%d,%d]}\r\n",irfcm,channel,notchMask);
  return write(d->fd, buf, strlen(buf));
}

int tuff_setChannelNotches(tuff_dev_t * d,
			      unsigned int irfcm,
			      unsigned int channel,
			      unsigned int notchMask) {
  unsigned int notch = notchMask & 0x7;
  unsigned int notNotch = (~notchMask) & 0x7;
  
  sprintf(buf,"\r\n{\"on\":[%d,%d,%d],\"off\":[%d,%d,%d]}\r\n", irfcm, channel,notch,
	  irfcm,channel,notNotch);
  printf("sending %s\r\n", buf);
  return write(d->fd, buf, strlen(buf));
}

int tuff_updateCaps(tuff_dev_t * d,
		       unsigned int irfcm,
		       unsigned int channel) {
  unsigned int tuffStack;
  unsigned int cmd;

  if (channel > 23) return -1;
  
  if (channel < 12) tuffStack = 0;
  else {
    tuffStack = 1;
    channel -= 12;
  }
  if (channel < 6) {
    cmd = (1<<channel) << 8;
  } else {
    cmd = ((1<<(channel-6)) << 8) | 0x4000;
  }
  cmd |= 0x60;
  sprintf(buf, "\r\n{\"raw\":[%d,%d,%d]}\r\n",
	    irfcm,
	    tuffStack,
	    cmd);
  return write(d->fd, buf, strlen(buf));  
}

int tuff_setCap(tuff_dev_t * d,
		   unsigned int irfcm,
		   unsigned int channel,
		   unsigned int notch,
		   unsigned int capValue) {
  // build up the raw command, there's no nice function for this
  unsigned int tuffStack;
  unsigned int cmd;

  if (channel > 23 || notch > 2 || capValue > 31) return -1;
  
  if (channel < 12) tuffStack = 0;
  else {
    tuffStack = 1;
    channel -= 12;
  }
  if (channel < 6) {
    cmd = (1<<channel) << 8;
  } else {
    cmd = ((1<<(channel-6)) << 8) | 0x4000;
  }
  // notch cmd (lower 8 bits) is
  // 0 n1 n0 v4 v3 v2 v1 v0
  // where n[1:0] is the notch
  // and v[4:0] is the value
  cmd |= capValue;
  cmd |= (notch << 5);
  sprintf(buf, "\r\n{\"raw\":[%d,%d,%d]}\r\n",
	    irfcm,
	    tuffStack,
	    cmd);
  printf("going to send %s\r\n", buf);
  return write(d->fd, buf, strlen(buf));
}

int tuff_setQuietMode(tuff_dev_t * d,
			  bool quiet) {
  sprintf(buf,"\r\n{\"quiet\":%d}\r\n", quiet);
  return write(d->fd, buf, strlen(buf));
}

int tuff_pingiRFCM(tuff_dev_t * d,
		      unsigned int timeout,
		      unsigned int numPing,
		      unsigned int *irfcmList) {
  char *p;
  unsigned int pingsSeen;
  int res;
  unsigned int nb;
  struct timeval tv;
  fd_set set;
  int rv;
  unsigned char c;
  unsigned int check_irfcm;
  unsigned int i; 

  switch(numPing) {
  case 0: return 0;
  case 1: sprintf(buf, "\r\n{\"ping\":[%d]}\r\n", irfcmList[0]); break;
  case 2: sprintf(buf, "\r\n{\"ping\":[%d,%d]}\r\n",
		  irfcmList[0],
		  irfcmList[1]); break;
  case 3: sprintf(buf, "\r\n{\"ping\":[%d,%d,%d]}\r\n",
		  irfcmList[0],
		  irfcmList[1],
		  irfcmList[2]); break;
  case 4: sprintf(buf, "\r\n{\"ping\":[%d,%d,%d,%d]}\r\n",
		  irfcmList[0],
		  irfcmList[1],
		  irfcmList[2],
		  irfcmList[3]); break;
  default: return -1;
  }
  res = write(d->fd, buf, strlen(buf));
  if (res < 0) return res;
  // sent off the ping, now wait for response
  pingsSeen = 0;
  nb = 0;
  while (pingsSeen != numPing) {
    FD_ZERO(&set);
    FD_SET(d->fd, &set);
    tv.tv_sec = timeout;
    tv.tv_usec = 0;
    rv = select(d->fd+1,&set,NULL,NULL,&tv);
    if (rv == 0) return pingsSeen;
    if (rv == -1) return -1;
    read(d->fd, &c, 1);
    if (c == '\n') {
      buf[nb] = 0;
      p = strstr(buf, "pong");
      if (p != NULL) {
	p += 2+strlen("pong"); // move past '":'
	check_irfcm = *p - '0';
	for (i=0;i<numPing;i++) {
	  if (irfcmList[i] == check_irfcm) {
	    pingsSeen++;
	    irfcmList[i] = 0xFFFFFFFF;
	  }
	}
      }
      nb = 0;
    } else {
      buf[nb] = c;
      nb++;
    }
  }
  return pingsSeen;
}

int tuff_setNotchRange(tuff_dev_t * d,
			   unsigned int notch,
			   unsigned int phiStart,
			   unsigned int phiEnd) {
  const char *notchStr[3] = { "r0", "r1", "r2" };
  if (notch > 2) return -1;

  sprintf(buf, "\r\n{\"%s\":[%d,%d]}\r\n", notchStr[notch], phiStart,phiEnd);
  return write(d->fd, buf, strlen(buf));
}

int tuff_setPhiSectors(tuff_dev_t * d,
			  unsigned int irfcm,
			  unsigned int *phiList,
			  unsigned int nb) {
  char *p;
  unsigned int i;
  int cnt;
  p = buf;
  cnt = sprintf(buf, "\r\n{\"set\":{\"irfcm\":%d,\"phi\":[", irfcm);
  p += cnt;
  for (i=0;i<nb;i++) {
    if (i != 0) {
      cnt = sprintf(p, ",");
      p += cnt;
    }
    cnt = sprintf(p, "%d", phiList[i]);
    p += cnt;
  }
  sprintf(p, "]}}\r\n");
  return write(d->fd, buf, strlen(buf));
}

int tuff_waitForAck(tuff_dev_t * d,
                     unsigned int irfcm,
                     int timeout)
{
  unsigned char nb;
  unsigned char c;
  char *p;
  unsigned int check_irfcm;
  int rv; 
  fd_set set; 
  struct timeval tv; 
  nb = 0;
  while (1) {
    FD_ZERO(&set); 
    FD_SET(d->fd, &set); 
    if (timeout > 0) 
    {
      tv.tv_sec = timeout; 
      tv.tv_usec = 0; 
    }
    rv = select(d->fd+1, &set, 0,0, timeout > 0 ? &tv: 0); 
    if (rv ==0) return 1; 
    if (rv == -1) return 2; 
    read(d->fd, &c, 1);
    if (c == '\n') {
      buf[nb] = 0;
      p = strstr(buf, "ack");
      if (p != NULL) {
	p += 2+strlen("ack"); // move past '":'
	check_irfcm = *p - '0';
  // 42 is the only 2-character iRFCM.
  // We're cheating and only checking the first character here.
  if (irfcm == 42 && check_irfcm == 4) break;
	if (check_irfcm == irfcm) break;
      }
      nb = 0;
    } else {
      buf[nb] = c;
      nb++;
    }
  }  
  return 0; 
}





tuff_dev_t * tuff_open(const char * dev) 
{
  char sbuf[512]; 
  struct termios tio; 
  memset(&tio, 0, sizeof(tio)); 
  int fd; 

  fd = open(dev, O_RDWR | O_NOCTTY); 

  if (fd < 0) 
  {
    snprintf(sbuf, sizeof(sbuf), "tuff_open(%s)", dev); 
    perror(sbuf); 
    return NULL; 
  }


  /** initialize struct */ 
  tuff_dev_t * ptr = malloc(sizeof(tuff_dev_t)); 
  ptr->fd = fd; 

  /* save old settings */ 
  tcgetattr(fd, &ptr->oldtio); 

  memcpy(&tio, &ptr->oldtio, sizeof(tio)); 


  cfsetospeed(&tio, BAUDRATE); 
  cfsetispeed(&tio, BAUDRATE); 
  

  /* set it up the way we like it */ 
  tio.c_cflag &= ~CSIZE; 
  tio.c_cflag |=  CS8 | CLOCAL | CREAD;  
  tio.c_cflag &= ~CRTSCTS; /* NO RTSCTS */ 
  tio.c_cflag &= ~CSTOPB;  /* Just one stop bit */  
  tio.c_cflag &= ~PARENB;  /* clear the parity bit  */
  tio.c_iflag |= IGNPAR;
  tio.c_iflag &= ~(IXON | IXOFF | IXANY); 
  tio.c_lflag &= ~(ECHO | ECHOE | ICANON | ISIG );
  tio.c_oflag &= ~OPOST ; 
  tio.c_cc[VINTR]    = 0;
  tio.c_cc[VQUIT]    = 0; 
  tio.c_cc[VERASE]   = 0;
  tio.c_cc[VKILL]    = 0;
  tio.c_cc[VEOF]     = 4;
  tio.c_cc[VTIME]    = 0; 
  tio.c_cc[VMIN]     = 1;
  tio.c_cc[VSWTC]    = 0;
  tio.c_cc[VSTART]   = 0;
  tio.c_cc[VSTOP]    = 0;
  tio.c_cc[VSUSP]    = 0;
  tio.c_cc[VEOL]     = 0;
  tio.c_cc[VREPRINT] = 0;
  tio.c_cc[VDISCARD] = 0;
  tio.c_cc[VWERASE]  = 0;
  tio.c_cc[VLNEXT]   = 0;
  tio.c_cc[VEOL2]    = 0;

  tcflush(fd, TCIFLUSH);
  tcsetattr(fd,TCSANOW,&tio);



  return ptr; 
}


int tuff_close(tuff_dev_t * d) 
{
  if(!d) return 1; 
  tcsetattr(d->fd,TCSANOW,&d->oldtio);
  close(d->fd); 
  free(d); 
  return 0; 
}


 
float tuff_getTemperature(tuff_dev_t * d, unsigned int irfcm, int timeout)
{

  int i; 
  unsigned char c; 
  float ret; 
  char *p; 
  fd_set set; 
  struct timeval tv; 
  int rv; 

//  bytegobbler(d->fd);  //EAT MOAR BYTES 
  sprintf(buf,"\r\n{\"monitor\": %d}\r\n", irfcm); 
  write(d->fd, buf, strlen(buf)); 

  i = 0; 

  while(1) 
  {
    FD_ZERO(&set); 
    FD_SET(d->fd, &set); 
    if (timeout > 0) 
    {
      tv.tv_sec = timeout; 
      tv.tv_usec = 0; 
    }

    rv = select(d->fd+1, &set, 0,0, timeout > 0  ? &tv : 0); 
    if (rv < 1) 
    {
      if (rv > 0) 
      {
        syslog(LOG_ERR, "select returned %d in tuff_getTemperature\n", rv); 
        return -275; 

      }
      return -274; 
    }

    read(d->fd, &c, 1); 

    if (c == '\n')
    {
      buf[i] = 0; 

      //really dumb hack that should result in a slow painful daeth, but check for a reset here 
      // This is the only other place we commonly read in Tuffd. The real solution is to buffer
      // all input and check all calls for a boot, but whatever
      
      p = strstr(buf, "boot irfcm"); 
      if (p) 
      {

        syslog(LOG_ERR,"Reboot detected while reading temperature. Partial buffer: %s", p); 
        return -999; 


      }



      p = strstr(buf, "temp\":"); 
      if (p)
      {
        sscanf(p, "temp\":%f}", &ret); 
        return ret; 
      }
    }
    else
    {
      buf[i++] = c; 

      if (i > sizeof(buf))
      {
        i = sizeof(buf)/2; 
        memcpy(buf , buf + sizeof(buf)/2, sizeof(buf)/2); 
      }

    }
  }
}

int tuff_getfd(tuff_dev_t * dev) 
{
  return dev->fd; 
}


int tuff_rawCommand(tuff_dev_t * d, unsigned int irfcm,  unsigned int tuffStack, unsigned int cmd)
{

  if ( ((cmd & 0xE0) == 0x40) || (cmd & 0x8000)) 
  {
    fprintf(stderr, "Prevented dangerous raw TUFF command  (%x) from being sent, cmd\n",cmd); 
    syslog(LOG_ERR, "Prevented dangerous raw TUFF command  (%x) from being sent, cmd\n",cmd); 
    return 1; 
  }

  sprintf(buf, "\r\n{\"raw\":[%d,%d,%d]}\r\n",
	    irfcm,
	    tuffStack,
	    cmd);

  return write(d->fd, buf, strlen(buf));  
}

  
