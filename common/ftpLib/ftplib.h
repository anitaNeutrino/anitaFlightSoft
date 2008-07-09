/* API of the FTP library */

/* still to do
   handle of restart for high level file transfer functions
    get: OK  
    put: need to check */


/* for off_t */
#include <sys/types.h>


/* ------------------------------------------------------------------------
  global parameters
  they can be changed at any time between API calls
*/


/* Other FTP "parameters" that you can't change:

   struct, mode and format are obsolete
   type (ascii/binary) is automatic
*/


/* -------------------------------------------------------------------------
  basic API
*/

int ftp_open(char* host, char* login, char* passwd);
int ftp_close(void);


int ftp_cd(char* directory);
int ftp_mkdir(char* directory);
int ftp_rmdir(char* directory);

int ftp_rename(char* oldname, char* newname);
int ftp_delete(char* filename);

int ftp_ls(int long_ls, char* file_or_dir, int (*cons)(char*,unsigned));

/*  high level file transfer functions */

int ftp_getfile(char* remote_name, char* local_name, off_t restart_point);

int ftp_putfile(char* local_name, char* remote_name,
		off_t restart_point, int append);


   /* 'remote_name' is the suggested name to be used by the server
      'uname' is a buffer of size 'usize' that will contain on output the
      unique name generated by the server.
      it can be set to NULL if you don't care about this name */

int ftp_putfileunique(char* local_name, char* remote_name,
		      char *uname, unsigned usize);



/* ------------------------------------------------------------------------- 
   advanced API
*/

/* send raw FTP commands 
   answer string of the server is put in 'buffer', whose size is 'size'
   
   size is limited to BUFSIZ (but it won't be that long anyway) */

int ftp_sendcommand(char *cmd, char *buffer, unsigned size);


/* low level file transfer functions

   look at how ftp_getfile(), ftp_putfile(), and ftp_putfileunique()
   are implemented in ftplib.c to see how to use them
 */

int ftp_get(char* remote_name, int (*cons)(char*,unsigned),
	    unsigned bufsize, off_t restart_point);

int ftp_put(char* remote_name, int (*prod)(char*,unsigned),
	    unsigned bufsize, off_t restart_point, int append);

int ftp_putunique(char* remote_name, char *uname, unsigned usize,
		  int (*prod)(char*,unsigned), unsigned bufsize);

/*

  missing useful features:

  - several FTP sessions at the same time

  - third party transfers (No: this is forbidden for security reasons
  by recent servers) ...
*/