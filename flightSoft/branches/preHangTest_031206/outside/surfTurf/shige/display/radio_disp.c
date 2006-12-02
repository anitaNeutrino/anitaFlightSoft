/*
    this is a test program using gtk-1.2
                                           22-Apr-03  SM    
    converted to a scope display,  started 26-May
    add header information display window.  7-Aug

    modified to display Salt Factory data. started on 29-Jun-04

    this program has been successfully compiled and exacuted on
    Sun in Krauss-anex, beauty, and shige.phys.hawaii.edu.

    modified to deal with SURF board test @UMN.  10-Dec-04 SM
*/

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <math.h>
#include <dirent.h>
#include <unistd.h>
#include <gtk/gtk.h>

#include "radio_disp.h"  /* this is for global variables and definitions */

/* these handling data files. */
FILE *open_files(int reopen) {

  int i, n, n_fn ;
  char d_buf[50], f_buf[50] ;
  char *last_f ;
  long tw_sec, tw_msec ;
  struct dirent *entry=NULL ;
  DIR *directory ;

  bzero(d_buf, 50) ;
  bzero(f_buf, 50) ;

  for (n=79 ; n && fname[n]!='/' ; n--) if (fname[n] == '\0') i=n;

  /* put directory name expricitly, in case of the current one. */
  if (!n++) {
    for(n=79 ; n>2 ; n--) fname[n] = fname[n-2] ;
    fname[0] = '.' ;
    fname[1] = '/' ;
  }

  /* separate directory and file name parts. */
  strncpy(f_buf, fname+n, (n_fn=i-n)) ;
  strncpy(d_buf, fname, n) ;
  if (!n_fn) f_buf[n_fn++] = '.' ;
/*        fprintf(stderr, " dir ; %s, file ; %s, n/i= %d, %d \n", */
/*  	      d_buf, f_buf, n, i) ; */

  if (!(directory = opendir(d_buf))) {
    fprintf(stderr, " failed to open data directory, %s\n", d_buf) ;
    return (wv_file = cm_file = NULL) ;
/*     wv_file = cm_file = NULL ; */
/*     return wv_file ; */
  }

  for (i=0 ; i++<10 ; ) usleep(1000) ; /* added on 26-Oct-04 */

  if (last_ev) {    /* sp. case ; open the last event of the directory. */
    while((entry=readdir(directory))) last_f = entry->d_name ;
    if (last_f[0] == '.') {
      closedir(directory) ;     /* no event data yet, so return NULL. */
      wv_file = NULL ;
      return wv_file ;
    }
    strcpy(fname, d_buf) ;
    strcat(fname, last_f) ;
    last_ev = FALSE ;
  }
  else {
    while((entry=readdir(directory)) && 
	  strncmp(f_buf, entry->d_name, n_fn)) ;
    if ((entry = readdir(directory))) {
      while (entry->d_name[0] != 's') 
	if (!(entry = readdir(directory))) {
	  closedir(directory) ;
	  wv_file = NULL ;  /* signifies failed file opening. */
	  return NULL ;
	}
      strcpy(fname, d_buf) ;
      strcat(fname, entry->d_name) ;
    }
    else {
      closedir(directory) ;
      wv_file = NULL ;  /* signifies failed file opening. */
      return NULL ;
    }
  }
  closedir(directory) ;

  /* get time information from wave form file name. */
  if (sscanf(fname+n, "surf_data.%ld", &curr.sec) != 1) {
    fprintf(stderr, " Failed to purse wv file, %s.\n", fname+n) ;
    return (wv_file = NULL) ;
  }

  /* open both wv and cm files. */
  if ((wv_file=fopen(fname, "r")) == NULL) 
    fprintf(stderr, " Failed to open wv file, %s\n", fname) ;
  else
    fprintf(stderr, " opened wv file, %s\n", fname) ;

  return wv_file ;
}

/* especially for SunOS where binary data bytes are swapped. */
void swap_byte(short *data) {
  short dl, dh ;

  dl = *data & 0x00ff ;
  dh = *data & 0xff00 ;
  *data = dh>>8 | dl <<8 ;
}

int read_data(void) {

  int i, j, n ; 
  short wv_data[N_CHN*N_CHP][N_SCA] ;
  GtkAdjustment *adjust ;

  int n_sample=N_SCA ;
  int rtn_v = 0 ;

  /* file has to be opened before reading. */
  if (!wv_file) 
    if (!(open_files(1))) return EOF ;

  /* change the setting of spin buttons accordingly. */
  if (scl_min) {
    adjust = gtk_range_get_adjustment(GTK_RANGE (scl_min)) ;
    adjust->upper = n_sample - MIN_D ; 
    adjust->lower = adjust->value = plt_xmin = 0;
    gtk_range_set_adjustment(GTK_RANGE (scl_min), adjust) ;
    gtk_signal_emit_by_name (GTK_OBJECT (adjust), "changed") ;
  }

  if (scl_max) {
    adjust = gtk_range_get_adjustment(GTK_RANGE (scl_max)) ;
    adjust->lower = MIN_D ; 
    adjust->upper = adjust->value = n_sample ;
    gtk_range_set_adjustment(GTK_RANGE (scl_max), adjust) ;
    gtk_signal_emit_by_name (GTK_OBJECT (adjust), "changed") ;
    plt_xmax = n_sample - 1 ;
  }

  if ((n=fread(wv_data, sizeof(short), N_SCA*N_CHN*N_CHP, wv_file))
      != N_SCA*N_CHN*N_CHP) {
/* in case of reading an incomplete file, read again after sleep. */
    fprintf(stderr, " wv_file is not correct size. %d.  retry. \n", n) ;
    for (i=0 ; i<200 ; i++) usleep (1000) ;
    rewind(wv_file) ;
    if ((n=fread(wv_data, sizeof(short), N_SCA*N_CHN*N_CHP, wv_file))
        != N_SCA*N_CHN*N_CHP) {
      fprintf(stderr, " wv_file is not correct size. %d\n", n) ;
      return EOF ;
    }
  }

/*   fprintf(stderr, " wv_file is correct size. %d.  finish. \n", n) ; */
  fclose (wv_file) ; wv_file = NULL ; /* data is read, so close the file. */

#ifdef SUN
  for(i=0 ; i<N_SCA*N_CHN*N_CHP ; i++) swap_byte(*wv_data+i) ;
#endif

  //  for(j=rms_n=0, v_max=v_min=0. ; j<n_channel ; j++) {
  for(j=rms_n=0, v_max=v_min=0. ; j<n_channel-8 ; j++) { // Loop over only over 3 boards. PM 04-01-05
    
    rms_m[j]=rms_m2[j]=0. ;

    for(i=0 ; i<n_sample ; i++) {
      if(wv_data[j][i]!=0) v_data[j][i] = (wv_data[j][i] - 2750)/1.6 ; // Rough calibration PM 04-01-05
      else wv_data[j][i]=0;
      v_data[j][i] /= 1000. ;          /* data are in mV unit */
      if (v_data[j][i] > v_max) v_max = v_data[j][i] ;
      if (v_data[j][i] < v_min) v_min = v_data[j][i] ;
      if (i<n_sample*0.4 || i>n_sample*0.6) {
	rms_m[j] += v_data[j][i] ;
	rms_m2[j] += v_data[j][i]*v_data[j][i] ;
	if (j==0) rms_n += 1. ;
      }
    }

    rms_m[j] /= rms_n ; rms_m2[j] /= rms_n ;
    rms_m[j] = rms_m2[j] - rms_m[j]*rms_m[j] ;
    if (rms_m[j] > 0.) rms_m[j] = sqrt(rms_m[j]) ;
    else rms_m[j] = 0. ;

    /* check whether this event contains "peak" or not. */
/*     for (i=n_sample/5 ; i<4*n_sample/5 ; i++) */
/*       if (fabs(v_data[j][i]) > hp_rms*rms_m[j]) */
/* 	printf("  detected peak #%d at bd #%d, channel #%d, points %d\n", */
/* 	       ++rtn_v, j/12, j-12*(j/12), i) ; */
  }

  return rtn_v ;
}

/* --- callbask functions for various widget. --- */

/* ends gtk task and kill all the windows. */
void destroy( GtkWidget *widget,
	      gpointer   data )
{
  gtk_main_quit();
}

/* this is a function that destroys a widget sent as a data. */
void del_win( GtkWidget *widget,
	      gpointer   data )
{
  gtk_widget_destroy(data) ;
  data = NULL ;
}

/* quit call back.  confirm and quit or kill diag window. */
gint quit( GtkWidget *widget,
                       GdkEvent  *event,
                       gpointer   data )
{
  char y_n ;

  GtkWidget *w_diag ;
  GtkWidget *button ;
  GtkWidget *label ;

  /* these are for dialog window. */
  w_diag = gtk_dialog_new() ;

  label = gtk_label_new(" Are you sure to quit.") ;
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(w_diag)->vbox), label, TRUE, FALSE, 5);
  gtk_widget_show(label) ;

  button = gtk_button_new_with_label("Yes");
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                            GTK_SIGNAL_FUNC (destroy), NULL);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(w_diag)->action_area), 
		     button, TRUE, FALSE, 0);
  gtk_widget_show(button) ;

  button = gtk_button_new_with_label("No");
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (del_win), w_diag);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(w_diag)->action_area), 
		     button, TRUE, FALSE, 0);
  gtk_widget_show(button) ;

  gtk_widget_show(w_diag) ;

  return(TRUE);
}

void toggle_callback(GtkWidget *widget, 
		     gpointer data) 
{
  if (GTK_TOGGLE_BUTTON(widget)->active) auto_f=TRUE ;
  else auto_f=FALSE ;

  area_press(d_area, (GdkEventButton *)"rewrite") ;
}

void bd_onoff(GtkWidget *widget, 
	      gpointer data) 
{
  int i ;

  for (i=0 ; widget != bd_b[i] && i<4 ; i++) ;
  if (i == 4) printf("can't find matching channel\n") ;
/*   else printf("channel #%d is toggled.\n", i) ; */
/*   if (GTK_TOGGLE_BUTTON(widget)->active) auto_f=TRUE ; */
/*   else auto_f=FALSE ; */

  if (pixmap) 
    area_press(d_area, (GdkEventButton *)"rewrite") ;
}

void set_value( GtkWidget *widget,
                  gpointer   data )
{
/*    printf("  value set is %.2f\n",  */
/*  	 gtk_spin_button_get_value_as_float(data)) ; */
  v_in = gtk_spin_button_get_value_as_float(data) ;

  if (GTK_TOGGLE_BUTTON(auto_scl)->active)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(auto_scl), FALSE) ;
  else area_press(d_area, (GdkEventButton *)"rewrite") ;
}

void set_range( GtkAdjustment *adj,
                  gpointer   data )
{
  int temp ;
  GtkAdjustment *adj2 ;

/*    printf("  value set is %.2f\n",  */
/*  	 gtk_spin_button_get_value_as_float(data)) ; */
  if (data == scl_min) {
    plt_xmin = adj->value ;
    adj2 = gtk_range_get_adjustment(GTK_RANGE(scl_max)) ;
    if (plt_xmax <= plt_xmin+MIN_D) {
      adj2->value = plt_xmax = plt_xmin+MIN_D ;
      gtk_signal_emit_by_name (GTK_OBJECT (adj2), "changed") ;
    }
  }
  else {
    plt_xmax = adj->value ;
    adj2 = gtk_range_get_adjustment(GTK_RANGE(scl_min)) ;
    if (plt_xmin > plt_xmax-MIN_D) {
      adj2->value = plt_xmin = plt_xmax-MIN_D ;
      gtk_signal_emit_by_name (GTK_OBJECT (adj2), "changed") ;
    }
  }

  area_press(d_area, (GdkEventButton *)"rewrite") ;
}

/* OK clicked, so set the file and open it. */
void file_sel( GtkWidget *widget,
	       GtkWidget  *data)
{
  strcpy(fname, 
	 gtk_file_selection_get_filename(GTK_FILE_SELECTION( data ))) ;
  printf(" file_sel: try to open %s\n",fname) ;
  open_files(0) ;

  del_win(widget, data) ;
  area_press(d_area, (GdkEventButton *)"file_sel") ;
}

void sel_file( GtkWidget *widget,
                  gpointer   data )
{
  GtkWidget *filew ;

  filew = gtk_file_selection_new(" select wv data ") ;

  gtk_signal_connect(GTK_OBJECT(filew) , "destroy", 
		     GTK_SIGNAL_FUNC (del_win), filew) ;
  gtk_signal_connect(GTK_OBJECT (GTK_FILE_SELECTION 
				 (filew)->ok_button), "clicked",
			    GTK_SIGNAL_FUNC (file_sel),  filew) ;
  gtk_signal_connect(GTK_OBJECT (GTK_FILE_SELECTION 
				 (filew)->cancel_button), "clicked",
			    GTK_SIGNAL_FUNC (del_win),  filew) ;
  gtk_widget_show(filew) ;
}

/* deal with header information window. */
void header_write()
{

/* header information  window stuff.  these have to be static. */
  static GtkWidget *hd_win = NULL ;
  static GtkWidget *hd_box = NULL ;
  static GtkWidget *ev_number = NULL ;
  static GtkWidget *time_UT = NULL ;
  static GtkWidget *rms_disp = NULL ;

  GtkWidget *separator ;
  int first, i, j, sec ;
  char buff[700] ;
 
  bzero(buff, 700) ;

  /* define a new window to show header information. */
  if (!hd_win) {
    hd_win = gtk_window_new(GTK_WINDOW_TOPLEVEL) ;
    gtk_window_set_title (GTK_WINDOW (hd_win), "HD file information") ;
    gtk_window_set_default_size (GTK_WINDOW (hd_win), 250, 100) ;
    first = 1 ;
  }
  else first = 0 ;

  if (!hd_box) hd_box = gtk_vbox_new(FALSE, 0) ;

  /* event number and such. */
  sprintf(buff, "ev # : %5d", curr.evnumber++) ;
  if (ev_number) gtk_label_set_text(GTK_LABEL (ev_number), buff) ;
  else {
    gtk_widget_show(ev_number = gtk_label_new(buff)) ;
    gtk_box_pack_start(GTK_BOX(hd_box), ev_number, FALSE, FALSE, 5);
   /* separator */
    gtk_widget_show(separator = gtk_hseparator_new()) ;
    gtk_box_pack_start(GTK_BOX(hd_box), separator, FALSE, TRUE, 5);
  }

  /* computer time down to m sec in UT. */
  sprintf(buff, " Unix time : %5d \n %s", 
	  curr.sec,  asctime(localtime((time_t *)&curr.sec))) ;
  if (time_UT) gtk_label_set_text(GTK_LABEL (time_UT), buff) ;
  else {
    gtk_widget_show(time_UT = gtk_label_new(buff)) ;
    gtk_box_pack_start(GTK_BOX(hd_box), time_UT, FALSE, FALSE, 5);
   /* separator */
    gtk_widget_show(separator = gtk_hseparator_new()) ;
    gtk_box_pack_start(GTK_BOX(hd_box), separator, FALSE, TRUE, 5); 
  }

  /* RMS of each channel. */
  sprintf(buff, "  RMS ;  bd #0,  bd #1,  bd #2,  bd #3  in V \n") ;
  for (i=0 ; i<8 ; i++) {
    sprintf(buff, "%s   ch%2d |", buff, i) ;
    for (j=0 ; j<4 ; j++)
      sprintf(buff, "%s  %.3f ", buff, rms_m[8*j+i]) ;
    sprintf(buff, "%s \n", buff) ;
  }
  if (rms_disp) gtk_label_set_text(GTK_LABEL (rms_disp), buff) ;
  else {
    gtk_widget_show(separator = gtk_hseparator_new()) ;
    gtk_box_pack_start(GTK_BOX(hd_box), separator, FALSE, TRUE, 5);
    gtk_widget_show(rms_disp = gtk_label_new(buff)) ;
    gtk_label_set_justify(GTK_LABEL(rms_disp), GTK_JUSTIFY_LEFT) ;
    gtk_box_pack_start(GTK_BOX(hd_box), rms_disp, FALSE, FALSE, 3);
  }

  /* define call=back function, etc.  has to be done only once. */
  if (first) {
  /* When the window is given the "delete_event" signal. */
    gtk_signal_connect (GTK_OBJECT (hd_win), "delete_event",
			GTK_SIGNAL_FUNC (quit), hd_win);

    /* Put the box into the main window. */
    gtk_container_add (GTK_CONTAINER (hd_win), hd_box);
    gtk_widget_show(hd_box) ;
    gtk_widget_show(hd_win) ;
  }
}

void set_color(GtkWidget *widget, GdkGC *gc, 
	       char red, char green, char blue) 
{
  GdkColor *color;

  /* the color we want to use */
  color = (GdkColor *)malloc(sizeof(GdkColor));

  color->red = red * (65535/255);
  color->green = green * (65535/255);
  color->blue = blue * (65535/255);

  color->pixel = (gulong)(red*65536 + green*256 + blue);
  /* However, the pixel valule is only truly valid on 24-bit (TrueColor)
   * displays. Therefore, this call is required so that GDK and X can
   * give us the closest color available in the colormap
   */
  gdk_color_alloc(gtk_widget_get_colormap(widget), color);

  /* set the foreground to our color */
  gdk_gc_set_foreground(gc, color);

}

/* reset vertical voltage scale setting to a current choice. */
void reset_v() 
{
  if (auto_f) {
    plt_v_fs = v_max-v_min ;
    plt_v_0 = v_max ;
  }
  else {
    plt_v_fs = 2*v_in ;
    plt_v_0 = v_in ;
  }
}

/* write dashed line, only horizontal or vertical. */
void dash_line(GdkGC *gc, int x1, int y1, int x2, int y2)
{
  int i, v1, v2, inc=20 ;

  if (x1==x2) {
    for(i=0, v1=y1, v2=v1+inc/2 ; v2<y2 ; v1+=inc, v2+=inc)
      gdk_draw_line(pixmap, gc, x1, v1, x2, v2) ; 
    gdk_draw_line(pixmap, gc, x1, v1, x2, y2) ; 
  }
  if (y1==y2) {
    for(i=0, v1=x1, v2=v1+inc/2 ; v2<x2 ; v1+=inc, v2+=inc)
      gdk_draw_line(pixmap, gc, v1, y1, v2, y2) ; 
    gdk_draw_line(pixmap, gc, v1, y1, x2, y2) ; 
  }
}

/* write frame for scope. */
void write_frame(GdkGC *gc, int width, int height, int x_off, int y_off,
		 int n_ch, int x_scal, int y_scal) 
{
  int x1, v0 ;
  int i, n_sample ;
  int c_h, c_w ;
  float v_min_sc, v_max_sc, v_diff ;
  char buff[32] ;
  GdkFont *font ;

  /* initialize voltage values used on the screen. */
  if (auto_f) { v_min_sc = v_min ; v_max_sc = v_max ; }
  else v_min_sc = -(v_max_sc = v_in) ;

  v_diff = (v_max_sc - v_min_sc)/4. ;
  font = gdk_font_load("a14") ;

  gdk_draw_rectangle(pixmap, gc, FALSE, x_off-1, y_off, width+1, height) ;

  /* draw dashed lines where needed. */
  for (i=v_min_sc/v_diff ; v_diff && v_diff*i<v_max_sc ; i++) {
    v0 = height*(plt_v_0-v_diff*i)/plt_v_fs + y_off ;
    dash_line(gc, x_off, v0, x_off+width, v0) ;
  }
  for (x1=x_off+width/4 ; x1<x_off+4*(width/4) ; x1 += width/4)
    dash_line(gc, x1, y_off, x1, y_off+height) ;

  /* put a charctor to show where is what. */
  strcpy(buff, "0") ;
  c_w = gdk_string_width(font,buff)/2 ;
  c_h = gdk_string_height(font,buff)/2 ;
  v0 = height*plt_v_0/plt_v_fs + y_off ;
  if (y_scal)
    gdk_draw_string(pixmap, font, gc, x_off-3*c_w, v0+c_h, buff) ;

  sprintf(buff, "%d", plt_xmin) ;
  if (x_scal) gdk_draw_string(pixmap, font, gc, 
		  x_off - gdk_string_width(font,buff) + c_w, 
		  y_off + height + gdk_string_height(font,buff) + c_h, 
		  buff) ;
  sprintf(buff, "%d", plt_xmax) ;
  if (x_scal) gdk_draw_string(pixmap, font, gc, 
		  x_off + width - gdk_string_width(font,buff) + c_w, 
		  y_off + height + gdk_string_height(font,buff) + c_h, 
		  buff) ;
  sprintf(buff, "%.3f", v_max_sc) ;
  if (y_scal) gdk_draw_string(pixmap, font, gc, 
		  x_off - gdk_string_width(font,buff) - c_w, 
		  y_off + c_h*3/2, 
		  buff) ;
  sprintf(buff, "%.3f", v_min_sc) ;
  if (y_scal) gdk_draw_string(pixmap, font, gc, 
		  x_off - gdk_string_width(font,buff) - c_w, 
		  y_off + height + c_h/2, 
		  buff) ;
  sprintf(buff, "ch #%d", n_ch) ;
  gdk_draw_string(pixmap, font, gc, 
		  x_off + c_w, 
		  y_off + 2*(c_h+2), 
		  buff) ;
}

/* plot data function used in area_press and area_config. */
void plot_data(GdkGC *gc, int n_bd, int n_ch, int width, int height, 
	       int x_off, int y_off) 
{
  int x1, x2, y1, y2 ;
  int i, n_sample ;

/* number of samples to be plotted.  */
  n_sample = plt_xmax - plt_xmin ;

  for(i=plt_xmin ; i<=plt_xmax ; x1=x2, y1=y2, i++) {
    x2= (i-plt_xmin)*width/n_sample + x_off ;
    y2 = height*(plt_v_0-v_data[8*n_bd+n_ch][i])/plt_v_fs + y_off ;
    if (y2 > height+y_off) y2 = height+y_off ;
    if (y2 < y_off) y2 = y_off ;
    if (i != plt_xmin) gdk_draw_line(pixmap, gc, x1, y1, x2, y2) ;
  }
}

gint area_press(GtkWidget *widget,
		GdkEventButton *b_event)
{
  static int tog=0 ;

  GdkGC *gc;
  int red[]   = {0xff, 0x00, 0x00, 0xa0} ;
  int green[] = {0x00, 0xff, 0x00, 0x00} ;
  int blue[]  = {0x00, 0x00, 0xff, 0xa0} ;
  int x_size = (IMG_X-2*XR_MARG-XL_MARG)/2 ; /* actual wf drawing area size. */
  int y_size = (IMG_Y-8*Y_MARG)/4 ;
  int i, j ;

  char info[128] ;

  /* first, create a GC to draw on */
  gc = gdk_gc_new(widget->window);
  set_color(widget, gc, 0, 0, 0) ;

  if (b_event == NULL) {  /* NULL button is a call from area_config. */
    if (pixmap == NULL) {
      pixmap = gdk_pixmap_new(widget->window, widget->allocation.width,
			      widget->allocation.height, -1) ;
      gdk_draw_rectangle (pixmap, gc, TRUE, 0, 0, widget->allocation.width,
			  widget->allocation.height) ;
    }
  }
  else {
  /* forground color is black and overwrite everything with rectangle. */
    gdk_draw_rectangle (pixmap, gc, TRUE, 0, 0, widget->allocation.width,
			  widget->allocation.height) ;

    if (strcmp((char *)b_event, "rewrite")) {  /* skip if this is rewrite. */ 
      if ((i=read_data()) == EOF) {
	if (time_out)
	  sprintf(info, "Waiting new data for %.1f sec.", 
		  (t_wait+=to_int)/1000.) ;
	else 
	  strcpy(info,  " No more data available.  Select new file.") ;
	gtk_label_set_text(GTK_LABEL(file_info), info) ;
	return time_out ;
      }
      else t_wait=0 ;
      if (hp_event) 
	while(i < hp_num)
	  if ((i=read_data()) == EOF) {
	    strcpy(info,  " No more data available.  Select new file.") ;
	    gtk_label_set_text(GTK_LABEL(file_info), info) ;
	    return time_out ;
	  }
    }
  }

  reset_v() ;                     /* reset vertical scale values. */
  set_color(widget, gc, 0xff, 0xff, 0) ;  /* write frame with yellow. */
  for (i=0, j=FALSE ; i<4 ; i++) {
    if (i==3) j = TRUE ;
    write_frame(gc, x_size, y_size, XL_MARG, Y_MARG+i*(Y_MARG+y_size),
		2*i, j, TRUE) ;
    write_frame(gc, x_size, y_size, XL_MARG+XR_MARG+x_size, 
		Y_MARG+i*(Y_MARG+y_size), 2*i+1, j, FALSE) ;
  }

  for(i=0 ; i<4 ; i++) {
  /* set forground color of gc to (R,G,B) color. */
    set_color(widget, gc, red[i], green[i], blue[i]) ;
    if (GTK_TOGGLE_BUTTON(bd_b[i])->active)
      for(j=0 ; j<4 ; j++) {
	plot_data(gc, i, 2*j, x_size, y_size, XL_MARG, 
		  Y_MARG+j*(Y_MARG+y_size)) ;
	plot_data(gc, i, (2*j+1), x_size, y_size, XL_MARG+XR_MARG+x_size, 
		  Y_MARG+j*(Y_MARG+y_size)) ;
      }
  }

  gdk_draw_pixmap(widget->window, 
		  widget->style->fg_gc[GTK_STATE_NORMAL],
		  pixmap, 0, 0, 0, 0, 
		  widget->allocation.width, widget->allocation.height) ; ;

  sprintf(info, " file : %s", fname) ;
/*   else strcpy(info,  " No more data available.  Select new file.") ; */
  gtk_label_set_text(GTK_LABEL(file_info), info) ;

  header_write() ;

}

gint area_config(GtkWidget *widget,
		 GdkEventConfigure *event )
{
  /*just call area_press function with NULL data. */
  if (pixmap == NULL) area_press(widget, NULL) ;

}

gint area_expose( GtkWidget *widget,
		  GdkEventExpose *event,
		  gpointer data) 
{

  gdk_draw_pixmap(widget->window, 
		  widget->style->fg_gc[GTK_STATE_NORMAL],
		  pixmap, 
		  event->area.x, event->area.y, 
		  event->area.x, event->area.y, 
		  event->area.width, event->area.height) ;
  return TRUE ;
}

/* --- gtk window initial setting part. ----  */
GtkWidget * setup_window(char *title) {

  GtkWidget *window=NULL ;

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL) ;
  gtk_window_set_title (GTK_WINDOW (window), title);

  /* When the window is given the "delete_event" signal. */
  gtk_signal_connect (GTK_OBJECT (window), "delete_event",
		      GTK_SIGNAL_FUNC (quit), NULL);
  
  /* Here we connect the "destroy" event to a signal handler. */
  gtk_signal_connect (GTK_OBJECT (window), "destroy",
                            GTK_SIGNAL_FUNC (destroy), NULL);

  /* Sets the border width of the window. */
  gtk_container_set_border_width (GTK_CONTAINER (window), 10);

  return window ;
}        

/* deal with vertical scale setting of scope display. */
GtkWidget * vertical_scale() {

  GtkWidget *box2=NULL ;
  GtkWidget *button ;
  GtkWidget *spin_b ;
  GtkWidget *label ;
  GtkAdjustment *adjust ;
  GdkGC *gc;

  box2 = gtk_hbox_new(FALSE, 0) ;

  label = gtk_label_new(" Vertical scale ; ") ;
  gtk_box_pack_start(GTK_BOX(box2), label, FALSE, FALSE, 5);
  gtk_widget_show(label) ;

  auto_scl = gtk_check_button_new_with_label (" Auto") ;
  gtk_signal_connect (GTK_OBJECT (auto_scl), "clicked",
                            GTK_SIGNAL_FUNC (toggle_callback), auto_scl);
/*    gtk_signal_connect (GTK_OBJECT (check_b), "toggled", */
/*                              GTK_SIGNAL_FUNC (toggle_callback), NULL); */  
  gtk_box_pack_start(GTK_BOX(box2), auto_scl, FALSE, FALSE, 0) ;
  gtk_widget_show(auto_scl) ;

  /* this is for testing spin button. */
  label = gtk_label_new(" fixed ; ") ;
  gtk_box_pack_start(GTK_BOX(box2), label, FALSE, FALSE, 5);
  gtk_widget_show(label) ;

  /* use initial value of the plotting fullscale. */
  adjust = (GtkAdjustment *) gtk_adjustment_new(v_in, 0.01, 2.0, 
						0.01, 0.1, 0.0) ;
  spin_b = gtk_spin_button_new(adjust, 0.0, 2) ;
  gtk_box_pack_start(GTK_BOX(box2), spin_b, FALSE, FALSE, 0) ;
  gtk_widget_show(spin_b) ;
  gtk_signal_connect (GTK_OBJECT (spin_b), "changed",
                            GTK_SIGNAL_FUNC (set_value), spin_b);

  label = gtk_label_new("V ") ;
  gtk_box_pack_start(GTK_BOX(box2), label, FALSE, FALSE, 5);
  gtk_widget_show(label) ;

  label = gtk_label_new(" | ") ;
  gtk_box_pack_start(GTK_BOX(box2), label, FALSE, FALSE, 5);
  gtk_widget_show(label) ;

  label = gtk_label_new("  data file ; ") ;
  gtk_box_pack_start(GTK_BOX(box2), label, FALSE, FALSE, 5);
  gtk_widget_show(label) ;

  button = gtk_button_new_with_label ("Select");
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                            GTK_SIGNAL_FUNC (sel_file), NULL);

  gtk_box_pack_start(GTK_BOX(box2), button, FALSE, FALSE, 0) ;
  gtk_widget_show(button) ;

  return box2 ;
}

/* deal with horizontal display of scope. */
GtkWidget * horizontal_scale() {

  GtkWidget *box2=NULL ;
  GtkWidget *label ;
  GtkAdjustment *adjust_min, *adjust_max ;
  GdkGC *gc;

  box2 = gtk_hbox_new(FALSE, 0) ;

  gtk_widget_show(label = gtk_label_new(" Horizontal scale ; ")) ;
  gtk_box_pack_start(GTK_BOX(box2), label, FALSE, FALSE, 5);

  /* set up sping buttons for min and max setting. */
  label = gtk_label_new(" Min ") ;
  gtk_box_pack_start(GTK_BOX(box2), label, FALSE, TRUE, 5);
  gtk_widget_show(label) ;

  adjust_min = (GtkAdjustment *) gtk_adjustment_new(plt_xmin, plt_xmin, 
					      plt_xmax-MIN_D, 1, 20, 0) ;
  scl_min = gtk_hscale_new(adjust_min) ;
  gtk_widget_set_usize (GTK_WIDGET (scl_min), 160, 30);
  gtk_scale_set_digits(GTK_SCALE(scl_min), 0) ;
  gtk_box_pack_start(GTK_BOX(box2), scl_min, FALSE, FALSE, 0) ;
  gtk_widget_show(scl_min) ;

  gtk_signal_connect (GTK_OBJECT (adjust_min), "value_changed",
                            GTK_SIGNAL_FUNC (set_range), scl_min);

  label = gtk_label_new(" Max ") ;
  gtk_box_pack_start(GTK_BOX(box2), label, FALSE, TRUE, 5);
  gtk_widget_show(label) ;

  adjust_max = (GtkAdjustment *) gtk_adjustment_new(plt_xmax, plt_xmin+MIN_D, 
						    plt_xmax, 1, 20, 0) ;
  scl_max = gtk_hscale_new(adjust_max) ;
  gtk_widget_set_usize (GTK_WIDGET (scl_max), 160, 30);
  gtk_scale_set_digits(GTK_SCALE(scl_max), 0) ;
  gtk_box_pack_start(GTK_BOX(box2), scl_max, FALSE, FALSE, 0) ;
  gtk_widget_show(scl_max) ;

  gtk_signal_connect (GTK_OBJECT (adjust_max), "value_changed",
                            GTK_SIGNAL_FUNC (set_range), scl_max);

  return box2 ;
}

/* function to deal with channel on/off buttons. */
GtkWidget * onoff_button() 
{
  GtkWidget *box ;
  GtkWidget *label ;
  int i ;
  char *bd_lbl[] = {" bd #0/red ", " bd #1/green", 
		    " bd #2/blue", " bd #3/purple"} ;

  /* buttons to turn on/off individual channels. */
  box = gtk_hbox_new(FALSE, 0);

  label = gtk_label_new("bd on/off ; ") ;
  gtk_box_pack_start(GTK_BOX(box), label, TRUE, FALSE, 0) ;
  gtk_widget_show(label) ;

  for (i=0 ; i<4 ; i++) {
    bd_b[i] = gtk_check_button_new_with_label (bd_lbl[i]) ;
    gtk_signal_connect (GTK_OBJECT (bd_b[i]), "clicked",
			GTK_SIGNAL_FUNC (bd_onoff), bd_b[i]);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(bd_b[i]), TRUE );
    gtk_box_pack_start(GTK_BOX(box), bd_b[i], FALSE, FALSE, 5);
    gtk_widget_show(bd_b[i]) ;
  }

  return box ;
}

/*   -- wrapper for calling area_press by timeout.  -- */
gint wrp_area_p (gpointer *data) {

  area_press (d_area, (GdkEventButton *)data) ;

  return TRUE ;
}

/* purse command arguments and change necessary parameters. */
void init_param(int argn, char**argv) {

  int i ;

  while (--argn) {
    if (**(++argv) == '-') {
      switch(*(*argv+1)) {
      case 'o' : break ;            /* this is for old format. ; obsolete */
      case 'p' : use_prev = TRUE ;  /* use prev. CAMAC data to display */
	break ;
      case 'n' : hp_event = TRUE ;  /* number of "peaks" reqired to */
	if (isdigit(**(argv+1)))    /* display an event.  default=1. */
	  if (sscanf(*(argv+1), "%d", &hp_num) == 1) {
	    printf(" peak number is changed to %d.\n", hp_rms) ;
	    argv++ ; argn-- ;
	  }
	break ;
      case 'r' : hp_event = TRUE ;  /* "peak" thres. in multiple of RMS. */
	if (isdigit(**(argv+1)))
	  if (sscanf(*(argv+1), "%d", &hp_rms) == 1) {
	    printf(" peak threshold is changed to %d.\n", hp_rms) ;
	    argv++ ; argn-- ;
	  }
	break ;
      case 't' : time_out=TRUE ;   /* time out mode ; next event will be */
	if (isdigit(**(argv+1)))   /* displayed after n ms. default=1000 */
	  if (sscanf(*(argv+1), "%d", &to_int) == 1) {
	    printf(" time interval is changed to %d.\n", to_int) ;
	    argv++ ; argn-- ;
	  }
	break ;
      case 'd' :       /* choose a directory, instead of an event file. */
	sscanf(*(++argv), "%s", fname) ;
	argn-- ;
	for(i=0 ; fname[i] && i<79 ; i++) ;
	if (fname[i-1] != '/') {
	  fname[i] = '/' ;
	  fname[i+1] = '\0' ;
	}
	break ;
      case 'l' : last_ev = TRUE ;  /* start the last event in a directory. */
	break ;
      case 'h' : 
	printf("usage: radio_disp [-lop] [-t <time(ms)>] [-d <dir>] ") ;
	printf("[-r <rms threshold>] [-n <peak #>] <filename>\n") ;
	break ;
      default :
	printf(" no such switch, %s\n", argv) ;
      }
    }
    else if (argn == 1)    /* only if this is the last argument. */
      sscanf(*(argv), "%s", fname) ;
  }

}

/* ------------------- MAIN function ---------------------  */
int main(int argn, char **argv)

{
  GtkWidget *window ;
  GtkWidget *button ;
  GtkWidget *box1 ;
  GtkWidget *box2 ;
  GtkWidget *label ;
  GtkWidget *separator ;

  guchar *pos ;
  int width, height, i ;
  char info[50] ;

  gtk_init (&argn, &argv) ;
  gdk_rgb_init() ;
  bzero(fname,80) ;  /* zero file name array. */
  curr.evnumber = 0 ;  /* init event number to 0. */

  /* purse command arguments. */
  init_param(argn, argv) ;

  /* open a file and read data if file name is specified. */
  if (fname[0] && open_files(0)) read_data() ;

  /* --- start setting up gtk widgets. --- */
  /* general gtk window setup includes destroy event handling. */
  window = setup_window("Surf Test Data Display") ;
  box1 = gtk_vbox_new(FALSE, 0) ;

  /* set up buttons for vertical scale selection, */
/*   printf(" set up buttons for vertical scale.\n") ; */
  gtk_widget_show(box2 = vertical_scale()) ;
  gtk_box_pack_start(GTK_BOX(box1), box2, FALSE, FALSE, 0);

  /* separator */
  gtk_widget_show(separator = gtk_hseparator_new()) ;
  gtk_box_pack_start(GTK_BOX(box1), separator, FALSE, TRUE, 5);

  /* set up buttons for horizontal scale selection. */
/*   printf(" set up buttons for horizontal scale.\n") ; */
  gtk_widget_show(box2 = horizontal_scale()) ;
  gtk_box_pack_start(GTK_BOX(box1), box2, FALSE, FALSE, 0);

  /* separator */
  gtk_widget_show(separator = gtk_hseparator_new()) ;
  gtk_box_pack_start(GTK_BOX(box1), separator, FALSE, TRUE, 5);

/* === drawaing area for displaying things. === */
/*   printf(" prepare display area. \n") ; */
  /* make a title of the area first. */
  label = gtk_label_new("Surf board test data") ;
  gtk_box_pack_start(GTK_BOX(box1), label, TRUE, FALSE, 0) ;
  gtk_widget_show(label) ;

  /* set up for board selection button */
/*   printf(" set up board selection button.\n") ; */
  gtk_widget_show(box2 = onoff_button()) ;
  gtk_box_pack_start(GTK_BOX(box1), box2, FALSE, FALSE, 0);

  /* define actual drawing area for wave form data. */
  gtk_widget_show(d_area = gtk_drawing_area_new ()) ;
  gtk_drawing_area_size (GTK_DRAWING_AREA(d_area), IMG_X, IMG_Y) ;

  gtk_signal_connect (GTK_OBJECT (d_area), "expose-event",
                            GTK_SIGNAL_FUNC (area_expose), NULL);
  gtk_signal_connect (GTK_OBJECT (d_area), "configure-event",
                            GTK_SIGNAL_FUNC (area_config), NULL);
  gtk_signal_connect (GTK_OBJECT (d_area), "button_press_event",
                            GTK_SIGNAL_FUNC (area_press), NULL);
  gtk_widget_set_events(d_area, GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK) ;

  gtk_box_pack_start(GTK_BOX(box1), d_area, TRUE, FALSE, 0) ;

  gtk_widget_show(file_info = gtk_label_new("data file and segment #")) ;
  gtk_box_pack_start(GTK_BOX(box1), file_info, TRUE, FALSE, 5);

  /* quit button from here. */
  gtk_widget_show(separator = gtk_hseparator_new()) ;
  gtk_box_pack_start(GTK_BOX(box1), separator, FALSE, TRUE, 5);

  box2 = gtk_hbox_new(FALSE, 0);

  gtk_widget_show(button = gtk_button_new_with_label ("Quit")) ;
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                            GTK_SIGNAL_FUNC (quit), NULL);

/*    gtk_box_pack_start(GTK_BOX(box2), button, TRUE, TRUE, 0); */
  gtk_box_pack_start(GTK_BOX(box2), button, TRUE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box1), box2, FALSE, FALSE, 0);

  /* The order in which we show the buttons is not really important, but I
         * recommend showing the window last, so it all pops up at once. */
  gtk_widget_show(box2);

  gtk_widget_show(box1);

  /* Put the box into the main window. */
  gtk_container_add (GTK_CONTAINER (window), box1);

  /* and the window */
  gtk_widget_show (window);
        
  /* make area_press called every second. */
  if (time_out) to_tag = gtk_timeout_add(to_int, (GtkFunction)wrp_area_p, 
					 (gpointer *)"time") ;

  /* All GTK applications must have a gtk_main(). Control ends here
     and waits for an event to occur (like a key press or mouse event). */
/*   printf(" set everything up, so wait for an event.\n") ; */
  gtk_main() ; 

  /* close up all the files opened. */
  if (wv_file) fclose(wv_file) ;

  return 0 ;
}
