/* los.c - library routines for the Washington U. LOS telemetry board.
 *
 * Marty Olevitch
 * Jul '05
 */

#include "los.h"
#include "PlxApi.h"
#include "crc_simple.h"
#include "version.h"

static char Version_string[] = LIBLOS_VERSION;

#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>

U16 Data_buf[LOS_MAX_WORDS];

static DEVICE_LOCATION Dev_location;
static HANDLE Dev_handle;
static HANDLE Intr_handle;
static PLX_INTR Plx_intr;
static IOP_SPACE IopSpace = IopSpace0;	// for 9030 chip

static unsigned long Bufcnt = 0;	// No. of buffers transmitted.

// Wait_for_interrupt - nonzero if we should use interrupts. If 0, we poll
// address 0 to see if we can read/write the board.
static int Wait_for_interrupt = 0;

// Poll_pause - minimum ms to pause between polls.
static int Poll_pause = 0;

// Delay_factor - divide the number of bytes in a buffer by this value to
// get the number of ms to delay after writing a buffer to the board. I.e.
// 	ms = nbytes / Delay_factor
// This value can be changed by the los_set_delay_factor() function.
static unsigned int Delay_factor = 100000;

static int Write = -1;	// read mode = 0, write mode = 1

static RETURN_CODE dev_open(DEVICE_LOCATION *d, HANDLE *h, U8 bus, U8 slot);
static RETURN_CODE dev_close(HANDLE h);

#define ERROR_STRING_LEN 2048
static char Error_string[ERROR_STRING_LEN];

static void pause_ms(int ms);
static int poll_addr_0(U16 want);

#ifdef NOTDEF
    static int poll_data_avail();
#endif

static int poll_data_ready();
static int prep_interrupt(void);
static int read_addr_0(U16 *val);
static int read_addr_01(U16 *val);
static void set_error_string(char *fmt, ...);
static int wait_for_interrupt(int ms);
static int write_addr_0(unsigned short val);
static int write_minimal_buffer();

#define DATA_READY	0xfeed
#define DATA_AVAIL	0xf00d
#define EMPTY_HDR	0xb3a5
#define LOST_SYNC	0xd0d0
#define NO_END_FOUND	0xbeef
#define SYNC_HDR	0xeb90

int
los_init(unsigned char bus, unsigned char slot, int intr, int wr, int ms)
{
    RETURN_CODE rc;
    U16 val;

    // Open our device.
    rc = dev_open(&Dev_location, &Dev_handle, bus, slot);

    if (ApiSuccess != rc) {
	set_error_string(
	    "los_init: can't open device at bus 0x%02x slot 0x%02x",
	    bus, slot);
        return -1;
    }

    los_reset();

    if (intr) {
	Wait_for_interrupt = 1;
	if (prep_interrupt()) {
	    return -4;
	}
    }

    if (wr) {
	Write = 1;

	// Expect a DATA_READY in address 0. Otherwise, this may not be a
	// WU LOS telemetry board.
	if (read_addr_0(&val)) {
	    set_error_string("los_init: bad read in read_addr_0");
	    return -2;
	}

	if (val != DATA_READY) {
	    set_error_string(
		"los_init: addr 0 value. Want %04X got %04x. Not LOS board?",
		DATA_READY, val);
	    return -3;
	}

	if (intr) {
	    write_minimal_buffer();
	    write_addr_0(DATA_AVAIL);
	}
	
    } else {
	Write = 0;
	write_addr_0(DATA_READY);
    }

    if (ms != -1) {
	Poll_pause = ms;
    }

    return 0;
}

int
los_end(void)
{
    RETURN_CODE rc;
    int ret = 0;

    if (Wait_for_interrupt) {
	rc = PlxIntrDisable(Dev_handle, &Plx_intr);
	if (ApiSuccess != rc) {
	    set_error_string("los_end: can't disable interrupt.");
	    ret = -1;
	}
    }

    rc = dev_close(Dev_handle);

    if (ApiSuccess != rc) {
	set_error_string("los_end: can't close device");
        return -2;
    }

    return ret;
}

char
*los_strerror(void)
{
    return Error_string;
}

char *
los_version()
{
    return Version_string;
}

int
los_write(unsigned char *buf, short nbytes)
{
    long ms;
    int odd_number_of_science_bytes = 0;
    U32 offset;
    RETURN_CODE rc;
    int ret;
    U16 *sp;
    U16 *startp;
    U16 total_bytes;

    if (Write != 1) {
	set_error_string("los_write: not initialized for writing.");
	return -4;
    }

    if (nbytes > LOS_MAX_BYTES) {
	set_error_string("los_write: too many bytes (%d) [maximum = %d]",
	    nbytes, LOS_MAX_BYTES);
	return -1;
    }

    if (poll_data_ready()) {
	// Not ready to accept data yet.

	if (Wait_for_interrupt) {
	    ret = wait_for_interrupt(0);
	    if (ret == -1) {
		// timeout
		;
	    } else if (ret == -2) {
		// error
		prep_interrupt();
	    }

	    if (ret) {
		return ret;
	    }

	} else {
	    return -5;
	}
    }

    // Set up our buffer.
    sp = Data_buf;
    *sp++ = START_HDR;	// Hardware bug. Need to have this here.
    *sp++ = AUX_HDR;

    *sp = ID_HDR;
    if (nbytes % 2) {
	// odd number of science bytes
	odd_number_of_science_bytes = 1;
	*sp |= 0x02;
    }
    sp++;

    {
	unsigned short *sp2;
	sp2 = (unsigned short *)&Bufcnt;
	*sp++ = *sp2++;
	*sp++ = *sp2;
    }

    if (odd_number_of_science_bytes) {
	*sp++ = nbytes + 1;
    } else {
	*sp++ = nbytes;
    }

    {
	// Add the science data.
	U8 *cp = (U8 *)sp;
	memcpy(cp, buf, nbytes);
	cp += nbytes;
	if (odd_number_of_science_bytes) {
	    // Add a fill byte.
	    *cp++ = FILL_BYTE;
	}
	sp = (U16 *)cp;
    }

    // Add the checksum and enders.
    startp = Data_buf + CHKSUM_FLIGHT_WORD_OFFSET;
    *sp = crc_short(startp, sp - startp);
    sp++;
    *sp++ = SW_END_HDR;
    *sp++ = END_HDR;
    *sp++ = AUX_HDR;

    // Write buffer.
    offset = sizeof(U16);	// offset of 1 word
    total_bytes = (sp - Data_buf) * sizeof(U16);

    rc = PlxBusIopWrite(Dev_handle, IopSpace, offset, FALSE,
	Data_buf, total_bytes, BitSize16);

    if (rc != ApiSuccess) {
	set_error_string("los_write: bad write to board");
	return -3;
    }

    write_addr_0(DATA_AVAIL);
    Bufcnt++;

    ms = total_bytes / Delay_factor;

    if (total_bytes < 150) {
	ms *= 8;
    } else if (total_bytes < 500) {
	ms *= 4;
    }

    pause_ms(ms);

    return 0;
}

int
los_read(unsigned char *buf, short *nwords)
{
    int ret = 0;
    U16 val[2];
    U16 words_clocked_in;

    if (Write != 0) {
	set_error_string("los_read: not initialized for reading");
	return -4;
    }

    if (Wait_for_interrupt) {
	ret = wait_for_interrupt(0);
	if (ret == -2) {
	    // error
	    prep_interrupt();
	    return -2;
	}

	ret = read_addr_01(val);
	if (ret) {
	    set_error_string("los_read: bad read_addr_01\n");
	    ret = -1;
	    goto finito;
	}

    } else {
	while (1) {
	    int retval = read_addr_01(val);
	    if (retval) {
		set_error_string("los_read: bad read_addr_01\n");
		ret = -1;
		goto finito;
	    }

	    if (DATA_AVAIL == val[0]) {
		break;
	    }

	    if (LOST_SYNC == val[0]) {
		set_error_string("los_read: lost sync\n");
		ret = -1;
		goto finito;
	    }

	    if (NO_END_FOUND == val[0]) {
		set_error_string("los_read: buffer not ended correctly\n");
		ret = -1;
		goto finito;
	    }
	}
    }

    words_clocked_in = val[1];
    if (words_clocked_in > 4095) {
	set_error_string("los_read: too many words (%u)\n", words_clocked_in);
	ret = -1;
	goto finito;
    }

    {
	// Word 0 is f00d, word 1 is the word count (words_clocked_in) put in by the
	// ground LOS board, word 2 is garbage, and the actual data starts
	// at word offset 3 instead. We give the application the buffer
	// starting at word 0, followed by words_clocked_in + 2 words from LOS.

	// Read the data. We read INITIAL_READ_WDS words to start. Then, we
	// issue a DATA_READY to allow the board to write more data when it
	// comes in, and we continue to read the rest of the data.

	int INITIAL_READ_WDS = 500;

	// The extra 3 words are for address 0, addr 1, and addr 2.
	int REMAINING_READ_WDS = (words_clocked_in - INITIAL_READ_WDS) + 3;

	RETURN_CODE rc;
	U16 *sp;
	U32 bytes_read_so_far = 0;

	// Read first INITIAL_READ_WDS words.
	rc = PlxBusIopRead(Dev_handle, IopSpace,
	    0, FALSE,
	    buf, INITIAL_READ_WDS * sizeof(U16),
	    BitSize16);

	if (rc != ApiSuccess) {
	    set_error_string("los_read: bad initial read");
	    ret = -1;
	    goto finito;
	}

	sp = (unsigned short *)buf;

	// Tell the hardware we are ready for more data.
	write_addr_0(DATA_READY);

	*nwords = words_clocked_in + 3;
	bytes_read_so_far = INITIAL_READ_WDS * sizeof(U16);

	if (words_clocked_in > INITIAL_READ_WDS) {
	    // Read the rest of the data.
	    rc = PlxBusIopRead(Dev_handle, IopSpace,
		bytes_read_so_far, FALSE,
		buf + bytes_read_so_far, REMAINING_READ_WDS * sizeof(U16),
		BitSize16);

	    if (rc != ApiSuccess) {
		set_error_string("los_read: bad big read");
		ret = -7;
		goto finito;
	    }
	}
    }

    return(0);

finito:

    // Tell the hardware we are ready for more data.
    write_addr_0(DATA_READY);

    return(ret);
}

int
los_read2(struct buffer_info *bufinfo, unsigned char *buf, short *nbytes)
{
    int ret = 0;

    if (Write != 0) {
	set_error_string("los_read: not initialized for reading");
	return -4;
    }

    if (Wait_for_interrupt) {
	ret = wait_for_interrupt(0);
	if (ret == -2) {
	    // error
	    prep_interrupt();
	    return -2;
	}

    } else {
	while (1) {
	    U16 val;
	    int retval = read_addr_0(&val);
	    if (retval) {
		break;
	    }

	    if (DATA_AVAIL == val) {
		break;
	    }

	    if (LOST_SYNC == val) {
		set_error_string("los_read: lost sync\n");
		ret = -1;
		goto finito;
	    }

	    if (NO_END_FOUND == val) {
		set_error_string("los_read: buffer not ended correctly\n");
		ret = -1;
		goto finito;
	    }
	}
    }

    {
	// Due to hardware reasons, we must read the data from word
	// offset 3 instead of word offset 2. This is taken care of by the
	// variable bug_byte_offset in the PlxBusIopRead() call.

	// Read the data. We read INITIAL_READ_WDS words to start. Then, we
	// issue a DATA_READY to allow the board to write more data when it
	// comes in, and we continue to read the rest of the data. For this
	// version we assume that we always read 1010 words. The number of
	// data bytes is at word offset 5.

	int INITIAL_READ_WDS = 500;

	int bug_byte_offset = 3 * sizeof(U16);
	RETURN_CODE rc;
	U16 *sp;
	U32 bytes_read_so_far = 0;

	// Read first INITIAL_READ_WDS words.
	rc = PlxBusIopRead(Dev_handle, IopSpace, bug_byte_offset + 0, FALSE,
	    buf, INITIAL_READ_WDS * sizeof(U16), BitSize16);

	if (rc != ApiSuccess) {
	    set_error_string("los_read: bad initial read");
	    ret = -1;
	    goto finito;
	}

	bytes_read_so_far = INITIAL_READ_WDS * sizeof(U16);

	sp = (unsigned short *)buf;

	// Tell the hardware we are ready for more data.
	write_addr_0(DATA_READY);

	if (DATA_AVAIL == *sp) {
	    // Should be properly formatted data there.
	    // Need to hide bit 1 because don't care whether there was an
	    // even or odd number of data bytes. The application can deal
	    // with that if it wants.
	    if ((ID_HDR & 0xfffd) == sp[2]) {
		U16 nb;
		unsigned long bufcnt;
		long bytes_to_read = 0L;
		long bytes_total = 0L;

		{
		    // Fill in the bufcnt.
		    U16 *sp2;
		    sp2 = (unsigned short *)&bufcnt;
		    *sp2++ = sp[3];
		    *sp2 = sp[4];
		}

		bufinfo->buffer_count = bufcnt;
		bufinfo->status = sp[2];

		nb = sp[5];
		bufinfo->science_nbytes = nb;
		bufinfo->byte_offset_to_science_data = 12;

		bytes_total = (6 * sizeof(U16)) +  // header
			      nb +                 // science bytes
			      (4 * sizeof(U16));   // ender


		bytes_to_read = bytes_total - bytes_read_so_far;

		{
		    long v = LOS_MAX_WORDS * 2;
		    if (bytes_to_read > v) {
			set_error_string("los_read: bad read amt (%ld) (%ld)",
			    bytes_to_read, v);
			ret = -8;
			goto finito;
		    }
		}

#ifdef NOTDEF
	    printf("bytes_read_so_far %u   bytes_to_read %u addr %u offset %u\n",
		    bytes_read_so_far, bytes_to_read,
		    bug_byte_offset + bytes_read_so_far,
		    (INITIAL_READ_WDS*2));
#endif

		if (bytes_to_read > 0) {
		    // Read the rest of the data.
		    rc = PlxBusIopRead(Dev_handle, IopSpace,
			bug_byte_offset + bytes_read_so_far, FALSE,
			buf + (INITIAL_READ_WDS*2), bytes_to_read, BitSize16);

		    if (rc != ApiSuccess) {
			set_error_string("los_read: bad big read");
			ret = -7;
			goto finito;
		    }
		}

		bufinfo->checksum = *((unsigned short *)buf + 6 +
		    (nb/sizeof(short)));

		*nbytes = bytes_total;
	    }
	}
    }

    return(0);

finito:

    // Tell the hardware we are ready for more data.
    write_addr_0(DATA_READY);

    return(ret);
}

unsigned long
los_bufcnt(unsigned char *buf)
{
    unsigned long bufcnt;
    memcpy(&bufcnt, buf+6, sizeof(long));
    return bufcnt;
}

void
los_bufinfo(unsigned char *buf, struct buffer_info *bi)
{
    bi->buffer_count = los_bufcnt(buf);
    bi->status = *((U16 *)buf + 2);
    bi->science_nbytes = *((U16 *)buf + 5);
    bi->byte_offset_to_science_data = 12;
    bi->checksum  = *((U16 *)buf + 6 + (bi->science_nbytes/sizeof(U16)));
}

void
los_reset()
{
    PlxPciBoardReset(Dev_handle);
}

unsigned int
los_set_delay_factor(unsigned int df)
{
    unsigned int old_df = Delay_factor;
    Delay_factor = df;
    return old_df;
}

static RETURN_CODE
dev_open(DEVICE_LOCATION *d, HANDLE *h, U8 bus, U8 slot)
{
    // Select the first PLX device found.
    d->BusNumber = bus;
    d->SlotNumber = slot;
    //d->BusNumber = (U8)-1;
    //d->SlotNumber = (U8)-1;
    d->DeviceId = (U16)-1;
    d->VendorId = (U16)-1;
    d->SerialNumber[0] = '\0';

    return PlxPciDeviceOpen(d, h);
}

static RETURN_CODE
dev_close(HANDLE h)
{
    return PlxPciDeviceClose(h);
}

// read_addr_0 - copies the value in address 0 of the on-board memory into
// val. Returns 0 if all went well, else -1.
static int
read_addr_0(U16 *val)
{
    RETURN_CODE rc;

    rc = PlxBusIopRead(Dev_handle, IopSpace, 0, FALSE, val, sizeof(U16),
    	BitSize16);

    if (rc != ApiSuccess) {
	return -1;
    }

    return 0;
}

// read_addr_01 - copies the values in address 0 and addr 1 of the on-board
// memory into val[0] and val[1]. Returns 0 if all went well, else -1.
static int
read_addr_01(U16 *val)
{
    RETURN_CODE rc;

    rc = PlxBusIopRead(Dev_handle, IopSpace, 0, FALSE, val, 2 * sizeof(U16),
    	BitSize16);

    if (rc != ApiSuccess) {
	return -1;
    }

    return 0;
}

// write_addr_0 - copies val to address 0 of the on-board memory. Returns 0
// if all went well, else -1.
static int
write_addr_0(unsigned short val)
{
    RETURN_CODE rc;

    rc = PlxBusIopWrite(Dev_handle, IopSpace, 0, FALSE,
    	&val, sizeof(unsigned short), BitSize16);

    if (rc != ApiSuccess) {
	return -1;
    }

    return 0;
}

// write_minimal_buffer - put in the minimum possible buffer that will
// leave the board in a usable state and cause it to generate an interrupt.
static int
write_minimal_buffer()
{
    RETURN_CODE rc;
    U32 buf[2];
    U16 *sp = (U16 *)buf;

    *sp++ = AUX_HDR;
    *sp++ = 0xee22;
    *sp++ = END_HDR;
    *sp++ = AUX_HDR;

    rc = PlxBusIopWrite(Dev_handle, IopSpace, 2, FALSE,
    	buf, sizeof(U32), BitSize32);

    if (rc != ApiSuccess) {
	return -1;
    }

    return 0;
}

static void
set_error_string(char *fmt, ...)
{
    va_list ap;
    if (fmt == NULL) {
        Error_string[0] = '\0';
    } else {
        va_start(ap, fmt);
        vsnprintf(Error_string, ERROR_STRING_LEN, fmt, ap);
        va_end(ap);
    }
}

// prep_interrupt - "enable" and "attach" the interrupt.
static int
prep_interrupt(void)
{
    /* Enable LINT1 interrupt. */ 
    memset(&Plx_intr, 0, sizeof(PLX_INTR)) ;
    Plx_intr.IopToPciInt = 1 ;	/* LINT1 interrupt line */
    Plx_intr.PciMainInt = 1 ;	/* PCI main int. is needed to use LINT1. */
    if (PlxIntrEnable(Dev_handle, &Plx_intr) != ApiSuccess) {
	set_error_string("prep_interrupt: failed to enable interrupt!") ;
	return -1;
    }

    /* Attach LINT1 interrupt. 
     * Note from Shige's code: Attachment should be done while LINT1 is not
     * active.  Otherwise, API disables LINT1 and waiting for it will time
     * out.  21-Jan-05, SM */
    if (PlxIntrAttach(Dev_handle, Plx_intr, &Intr_handle) != ApiSuccess) {
	set_error_string("prep_interrupt:  failed to attatch interrupt.") ;
	return -2;
    }

    return 0;
}

// wait_for_interrupt - returns 0 if we got the interrupt, -1 if we timed
// out, -2 for any other error.
static int
wait_for_interrupt(int ms)
{
    RETURN_CODE rc;
    int ret;

    /* Wait for an interrupt on LINT1 line. */
    rc = PlxIntrWait(Dev_handle, Intr_handle, ms);

    switch (rc) {
	case ApiSuccess : 
	    ret = 0;
	    if (PlxIntrAttach(Dev_handle, Plx_intr, &Intr_handle) !=
		    ApiSuccess) {
		set_error_string("wait_for_interrupt: bad attatch interrupt.");
		return -2;
	    }
	    break ;
	case ApiWaitTimeout :
	    set_error_string("wait_for_interrupt: timed out.");
	    ret = -1;
	    break ;
	case ApiWaitCanceled :
	    set_error_string("wait_for_interrupt: canceled.") ;
	    ret = -2;
	    break ;
	case ApiFailed :
	    set_error_string("wait_for_interrupt: failed.");
	    ret = -2;
	    break;
	default:
	    set_error_string("wait_for_interrupt: unknown error.");
	    ret = -2;
	    break;
    }

    return ret;
}

static void
pause_ms(int ms)
{
    static struct timespec t;
    t.tv_sec = 0;
    t.tv_nsec = ms * 1000000L;
    nanosleep(&t, NULL);
}

// poll_addr_0 - Poll addr 0 for a value.
static int
poll_addr_0(U16 want)
{
    U16 val;
    int ret = read_addr_0(&val);

    if (ret) {
	set_error_string("poll_addr_0: bad read of addr 0");
	return -5;
    }

#ifdef NOTDEF
    if (want != val) {
	set_error_string(
	    "poll_addr_0: timed out polling for (%04X)", want);
	return -6;
    }
#endif

    if (want != val) {
	// try one more time

	pause_ms(Poll_pause);

	// try again
	ret = read_addr_0(&val);
	if (ret) {
	    set_error_string("poll_addr_0: bad read of addr 0");
	    return -5;
	}

	if (want != val) {
	    set_error_string(
		"poll_addr_0: timed out polling for (%04X)", want);
	    return -6;
	}
    }

    return 0;
}

// poll_data_ready - Poll addr 0 for DATA_READY.
static int
poll_data_ready()
{
    return poll_addr_0(DATA_READY);
}

#ifdef NOTDEF
    // poll_data_avail - Poll addr 0 for DATA_AVAIL.
    static int
    poll_data_avail()
    {
	return poll_addr_0(DATA_AVAIL);
    }
#endif


int fake_los_write(unsigned char *buf, short nbytes, char *outputDir)
{
    FILE *losFile;
    FILE *numFile;
    static int losNumber=0;
    static unsigned long bytesInFile=0;
    char filename[FILENAME_MAX];
    long ms;
    int odd_number_of_science_bytes = 0;
    U32 offset;
//    RETURN_CODE rc;
//    int ret;
    U16 *sp;
    U16 *startp;
    U16 total_bytes;


    if (nbytes > LOS_MAX_BYTES) {
	set_error_string("los_write: too many bytes (%d) [maximum = %d]",
	    nbytes, LOS_MAX_BYTES);
	return -1;
    }

    if(losNumber==0) {
	sprintf(filename,"%s/losNumber.txt",outputDir);
	numFile = fopen (filename, "r");
	if(numFile) {
	    fscanf(numFile,"%d",&losNumber);
	    fclose(numFile);
	}	
	losNumber++;
	bytesInFile=0;
    }

    

    // Set up our buffer.
    sp = Data_buf;
    // RJN -- Faking board words
    *sp++ = START_HDR;
    *sp++ = 0;
    *sp++ = 0;
//Back to Marty
    *sp++ = START_HDR;	// Hardware bug. Need to have this here.
    *sp++ = AUX_HDR;

    *sp = ID_HDR;
    if (nbytes % 2) {
	// odd number of science bytes
	odd_number_of_science_bytes = 1;
	*sp |= 0x02;
    }
    sp++;

    {
	unsigned short *sp2;
	sp2 = (unsigned short *)&Bufcnt;
	*sp++ = *sp2++;
	*sp++ = *sp2;
    }

    if (odd_number_of_science_bytes) {
	*sp++ = nbytes + 1;
    } else {
	*sp++ = nbytes;
    }

    {
	// Add the science data.
	U8 *cp = (U8 *)sp;
	memcpy(cp, buf, nbytes);
	cp += nbytes;
	if (odd_number_of_science_bytes) {
	    // Add a fill byte.
	    *cp++ = FILL_BYTE;
	}
	sp = (U16 *)cp;
    }

    // Add the checksum and enders.
    startp = 3+Data_buf + CHKSUM_FLIGHT_WORD_OFFSET;
    *sp = crc_short(startp, sp - startp);
    sp++;
    *sp++ = SW_END_HDR;
    *sp++ = END_HDR;
    *sp++ = AUX_HDR;

    // Write buffer.
    offset = sizeof(U16);	// offset of 1 word
    total_bytes = (sp - Data_buf) * sizeof(U16);

    sprintf(filename,"%s/los_%d",outputDir,losNumber);
    losFile = fopen (filename, "ab");
    bytesInFile=ftell(losFile);
    if(!losFile) {
	printf("Couldn't open %s\n",filename);
	return -3;
    }
    if(fwrite(Data_buf,1,total_bytes,losFile)!=total_bytes) {
	printf("Error writing file %s\n",filename);
	fclose(losFile);
	return -3;
    }
    fclose(losFile);
    bytesInFile+=total_bytes;
    if(bytesInFile>1000000) {
	losNumber++;
	sprintf(filename,"%s/losNumber.txt",outputDir);
	numFile = fopen (filename, "w");
	if(numFile) {
	    fprintf(numFile,"%d\n",losNumber);
	    fclose(numFile);
	}		
    }

    Bufcnt++;

    ms = total_bytes / Delay_factor;

    if (total_bytes < 150) {
	ms *= 8;
    } else if (total_bytes < 500) {
	ms *= 4;
    }

    pause_ms(ms);

    return 0;

}
