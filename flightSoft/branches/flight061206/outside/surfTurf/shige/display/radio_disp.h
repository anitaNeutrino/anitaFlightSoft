/*
    this is a test program using gtk-1.2
                                      22-Apr-03  SM    
    converted to a scope display,  started 26-May
    add header information display window.  7-Aug

    modified to display Salt Factory data  29-Jun-04
    header file sepalated from source code.

*/

/* scope display area size in pixcel units. */
#define IMG_Y 500
#define IMG_X 620
#define XL_MARG 50    /* these are margine for display area */
#define XR_MARG 20
#define Y_MARG 10
#define MIN_D 40      /* minimum difference between plt_xmin and plt_xmax. */

/* particle track display area size in pixcel. */
/* make it smaller on SUN to fit to the screen. */
#ifdef SUN
#define TRCK_X 500
#define TRCK_Y 270
#else
#define TRCK_X 550
#define TRCK_Y 300
#endif

/* track display related parameters. */
#define TDC_U_1 0.2   /* TDC conversion factor.  ns/tic */
#define TDC_U_2 0.4
#define C_M 0.195     /* speed of light in MACRO tank, m/ns unit */
#define MC_L 11.2     /* active length of MACRO counter in m */
#define C_A 0.30      /* spped of light in air, m/ns unit */
#define TH_C 0.81     /* Cerenkov angle, arccos(1/1.45) ; is this valid? */
                      /* according to Rika-nenpyo, n=1.50 @ 9.4um=31THz */

/* numbers for data array. */
#define N_SCA 256
#define N_CHN 8
#define N_CHP 3

/* for data displaying area windiw. */
guchar rgb_buf[IMG_X * IMG_Y * 3] ;
GdkPixmap *pixmap=NULL ;

GtkWidget *auto_scl ;
/* widgets for x limit h scale button. */
GtkWidget *scl_min, *scl_max ; 
/* widget for board selection button. */
GtkWidget *bd_b[4] ;
/* label to dsplay file name. */
GtkWidget *file_info ;

/* gloval variables. */
int auto_f=FALSE ;
float v_max, v_min, v_in = 1.0 ;
float plt_v_0, plt_v_fs ;  /* these are values used in plotting. */
int   plt_xmin = 0, plt_xmax = N_SCA-1 ;
float v_data[32][N_SCA] ;
int   n_channel=32, f_size=0 ;
double rms_m[32], rms_m2[32], rms_n ;

int   time_out = FALSE ;      /* use gtk_timeout, if TRUE. */
int   t_wait, to_int=1000 ;   /* parameters used for timeout. */
guint to_tag ;                /* tag to be used to stop timeout. */

int  use_prev = FALSE ;   /* use previous CAMAC data to display. */
int  hp_event = FALSE ;   /* display only events with high peak. */
int  hp_rms = 7 ;         /* sigma deviation needed to be high peak. */
int  hp_num = 1 ;         /* number of peaks detected in one event. */
int  last_ev = FALSE ;  

#define HPEVENT 1 

FILE *wv_file, *cm_file ;
char fname[80] ;

typedef struct {
  int evnumber ;
  long int sec ;
  int     msec ;
} hd_data ;

hd_data curr ;

GtkWidget *d_area ;   /* display area made global to allow others access. */

gint area_press(GtkWidget *, GdkEventButton *) ;
