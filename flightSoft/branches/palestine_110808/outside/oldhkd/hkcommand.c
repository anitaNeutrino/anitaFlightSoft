/*
  this is a program to deal with a command to change daq comfiguraion.
  will be called by "cmdd.py" written with phython.

                                started  10-Jul-03  SM
		       1st V. completed  13-Jul
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <sys/types.h>
#include <dirent.h>

#define ERROR 1
#define SUCCESS 0

FILE *log_f ;  /* log file pointer.  used by many function. */

/* list of available parameters in confgiuration file.  11-Jul-03  SM */
/* I think the names must be unique in first 6 chars */ 
/* and at least 6 chars long.  Spaces after make the config file look nice */
/* (I think...)  I am sticking to exactly 6 just to save thinking. */

struct param {
  char *p_name ;
  char format ;
  float f_value ;
  int i_value ;
  int flag ;
} cfg_p[] = { {"dacv00      ", 'd'}, {"dacv01      ", 'd'},
	      {"dacv02      ", 'd'}, {"dacv03      ", 'd'},
	      {"dacv04      ", 'd'}, {"dacv05      ", 'd'},
	      {"dacv06      ", 'd'}, {"dacv07      ", 'd'},
	      {"servo0      ", 'd'}, {"servo1      ", 'd'}, 
	      {"servo2      ", 'd'}, {"servo3      ", 'd'}, 
	      {"lint00      ", 'd'}, {"lint01      ", 'd'}, 
	      {"lint02      ", 'd'}, {"lint03      ", 'd'}, 
	      {"lint04      ", 'd'}, {"lint05      ", 'd'}, 
	      {"lint06      ", 'd'}, {"lint07      ", 'd'}, 
	      {"lint08      ", 'd'}, {"lint09      ", 'd'}, 
	      {"loop00      ", 'd'}, {"loop01      ", 'd'}, 
	      {"loop02      ", 'd'}, {"loop03      ", 'd'}, 
	      {"loop04      ", 'd'}, {"loop05      ", 'd'}, 
	      {"loop06      ", 'd'}, {"loop07      ", 'd'}, 
	      {"loop08      ", 'd'}, {"loop09      ", 'd'}, 
	      {"loop10      ", 'd'}, {"loop11      ", 'd'}, 
	      {"loop12      ", 'd'}, {"loop13      ", 'd'}, 
	      {"loop14      ", 'd'}, {"loop15      ", 'd'}, 
	      {"loop16      ", 'd'}, {"loop17      ", 'd'}, 
	      {"loop18      ", 'd'}, {"loop19      ", 'd'}, 
	      {"loop20      ", 'd'}, {"loop21      ", 'd'}, 
	      {"loop22      ", 'd'}, {"loop23      ", 'd'}, 
	      {"loop24      ", 'd'}, {"loop25      ", 'd'}, 
	      {"loop26      ", 'd'}, {"loop27      ", 'd'}, 
	      {"loop28      ", 'd'}, {"loop29      ", 'd'}, 
	      {"loop30      ", 'd'}, {"loop31      ", 'd'}, 
	      {"sample      ", 'd'}, {"smooth      ", 'd'}, 
	      {"hk1int      ", 'd'}, {"hk2int      ", 'd'}, 
	      NULL } ;

int *suffix ;

/* find the number of configuration template files in "dir" and
   put suffix of these template in variable suffix.
   these files are required to have a name "tmpl_??.cfg".  */
int count_tmpl(char *dir) {

  int i, n, n_t ;
  struct dirent **entry ;

  n = scandir(dir, &entry, 0, alphasort);
  if (n < 0)
    perror("scandir");
  else {
    suffix = malloc(n * sizeof(int)) ; /* 2 more than needed, should be OK. */
    for (i=n_t=0 ; i<n ; i++) {
      if (sscanf(entry[i]->d_name,"tmpl_%d.cfg",suffix+n_t) == 1) n_t++ ;
      free(entry[i]);
    }
    free(entry);
  }

  return n_t ;
}

int copy_tmpl(int temp_n, char *temp_dir, char *config) {

  FILE *source, *dest ;
  char buf[80] ;
  int n, m;

  /* compose file name and open both files for copying. */
  sprintf(buf, "%s/tmpl_%02d.cfg", temp_dir, temp_n) ;
  if (!(source=fopen(buf, "r"))) {
    fprintf(log_f, " copy_tmpl: could not open template file ,%s\n", buf) ;
    return ERROR ;
  }

  if (!(dest=fopen(config, "w"))) {
    fprintf(log_f, " copy_tmpl: could not open config file ,%s\n", config) ;
    return ERROR ;
  }

  /* copy charactors 80 at a time, till EOF is reached. */
  while ((n=fread(buf, 1, 80, source)) > 0) {
    if ((m=fwrite(buf, 1, n, dest)) != n) {
      fprintf(log_f, " copy_tmpl: wrote only %d characters.\n", m) ;
      return ERROR ;
    }
    if (n != 80) break ;
  }

  if (feof(source)) ;  /* make sure EOF is reached. */
  else {
    printf(" copy_tmpl: read only %d charactors, but not EOF.\n", n) ;
    return ERROR ;
  }
    
  fclose (source) ;
  fclose (dest) ;
}

/* count number of parameters and clear the flag for later use. */
int count_param() {

  int i ;

  for (i=0 ; (cfg_p+i)->p_name ; i++) cfg_p[i].flag = 0 ;

  return i ;
}

int change_value(char *cmd, char *file) {

  FILE *ptr ;
  int i, n ;
  int mm,nn;
  char buff[80] ;
  char comm[32] ;

  if (!(ptr=fopen(file,"r"))) {
    fprintf(log_f, " change_value: can't open file, %s\n", file) ;
    return ERROR ;
  }

  while ((n=getc(ptr)) != EOF) {
    for (i=0, buff[0]=n ; (buff[++i]=getc(ptr)) != '\n' ; ) ;
    if (buff[0] != '#') {           /* this is to get rid of comments. */
      sscanf(buff, "%s", comm) ;
      for (i=0 ; strncmp(comm, (cfg_p+i)->p_name, 6) ; i++) ;      if ((cfg_p+i)) {
	if ((cfg_p+i)->format == 'd') 
	  sscanf(buff, "%*s %d", &((cfg_p+i)->i_value)) ;
	else 
	  sscanf(buff, "%*s %f", &((cfg_p+i)->f_value)) ;
      }
      (cfg_p+i)->flag = 1 ;
    }
  }

  fclose (ptr) ;
  ptr=fopen(file, "w") ;

  printf("start writing data.\n") ;
  i=cmd[1] ;
  (cfg_p+i)->i_value = (cfg_p+i)->f_value = 
    (unsigned char)cmd[3] + 256*(char)cmd[2] ;
  printf("Command[2]: %u, Command[3]: %u, Value %d",(unsigned) cmd[2],
	 (unsigned) cmd[3],(cfg_p+i)->i_value);
  (cfg_p+i)->flag = 1 ;

  for(i=0 ; (cfg_p+i)->p_name ; i++) 
    if ((cfg_p+i)->flag) {
      sprintf(buff, "%%s  \t %%%c\n", (cfg_p+i)->format) ;
      if ((cfg_p+i)->format == 'd') 
	fprintf(ptr, buff, (cfg_p+i)->p_name, (cfg_p+i)->i_value) ;
      else fprintf(ptr, buff, (cfg_p+i)->p_name, (cfg_p+i)->f_value) ;
    }

  fclose(ptr) ;
  return SUCCESS ;}

int main(int argn, char **argv) {

/* this is the drectory where template for configuration files live. */
  char *tmpl_dir = "/home/anita/etc/hkd" ;
/* this is the configuration file for daq program to read. */
  char *main_cfg = "/home/anita/etc/hkd.conf" ;

  int n_tmpl=0 ;
  char *cmd_name ;
  int n_param ;
  struct param **cfg_p ;

  char com_par[4] ; /* 0; main command, 
		      1; parameter # (0xff = choose template)
		      2, 3; parameter value */
  int i ;

  log_f = stderr ;  /* log file, pointing to stderr for debugging. */

  /* parse command and put values into parameter array. */
  if (argn != 4) {
    fprintf(log_f, "number of arguments is not 4, %d instead.\n", argn) ;
    return ERROR ;
  }
  else for (i=0 ; i<4 ; i++) {
    if (i == 0) cmd_name = *argv ;
    else {
      if (strncmp(*++argv, "0x", 2)) sscanf(*argv, "%d", com_par+i) ;
      else sscanf(*argv, "%x", com_par+i) ;
    }
  }
  printf(" parameter is %d %d %d. \n", com_par[1], com_par[2], com_par[3]) ;

  /* select pre-made configuration file command. */
  if (com_par[1] == (char)-1) {
    if (!(n_tmpl = count_tmpl(tmpl_dir))) {
      fprintf(log_f, "there is no template files in %s\n", tmpl_dir) ;
      return ERROR ;
    }

    for (i=0 ; i<n_tmpl ; i++) if (suffix[i] == com_par[3]) break ;

    if (com_par[3] < 0 || i==n_tmpl) {
      fprintf(log_f, "template number %d does not exist.\n", com_par[3]) ;
      return ERROR ;
    }

    /* do coying of the template file to main cfg file. */

    if (copy_tmpl(suffix[i], tmpl_dir, main_cfg)) {
      fprintf(log_f, "copying template file to main.cfg failed\n") ;
      return ERROR ;
    }

    if (suffix) free(suffix) ;
  }

  /* change one parameter in current configuration file. */
  else {

    if (com_par[1] >  count_param()) {
      fprintf(log_f, "parameter out of range, %d\n", com_par[1]) ;
      return ERROR ;
    }

    if (change_value(com_par, main_cfg)) {
      fprintf(log_f, "failts to change value, %d, %d, %d\n",
	     com_par[1], com_par[2], com_par[3]) ;
      return ERROR ;
    }
  }

  return SUCCESS ;
}
