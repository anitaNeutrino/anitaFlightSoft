/* twtest.c - unit tests for telemwrap
 *
 * Marty Olevitch, May '08
 */

#include "../telemwrap.h"
#include <stdio.h>
#include <check.h>

#define DBSIZE 3000	// databuf size in words
#define WBSIZE 3050	// wrapbuf size in words

// No science data, default source (LOS)
START_TEST (test_no_science_data)
{
    unsigned short databuf[DBSIZE];
    unsigned short wrapbuf[WBSIZE];

    int ret;
    unsigned long bufcnt;
    unsigned short checksum;
    unsigned short nbytes;
    int totbytes;

    totbytes = telemwrap(databuf, wrapbuf, 0);
    fail_unless((totbytes == 20),
    	"totbytes: want %d got %d", 20, totbytes);

    // Initial header words
    fail_unless((0xF00D == wrapbuf[0]), NULL);
    fail_unless((0xD0CC == wrapbuf[1]),
    	"wrapbuf[%d]: want %04x got %04x", 1, 0xD0CC, wrapbuf[1]);
    fail_unless((0xAE00 == wrapbuf[2]), 
    	"wrapbuf[%d]: want %04x got %04x", 2, 0xAE01, wrapbuf[2]);

    // Buffer count
    memcpy(&bufcnt, wrapbuf+3, sizeof(bufcnt));
    fail_unless((bufcnt == 0), 
    	"bufcnt: want %lu got %lu", 0, bufcnt);

    // Number of science bytes
    memcpy(&nbytes, wrapbuf+5, sizeof(nbytes));
    fail_unless((nbytes == 0), 
    	"wrapbuf[%d] (nbytes): want %04x got %04x", 5, 0, wrapbuf[5]);

    // Should be no science data in this one. Next is checksum.
    memcpy(&checksum, wrapbuf+6, sizeof(checksum));
    fail_unless((checksum == 0),
    	"wrapbuf[%d] (checksum): want %04x got %04x", 6, 0, wrapbuf[6]);
    
    // Ending header words
    fail_unless((0xAEFF == wrapbuf[7]),
    	"wrapbuf[%d]: want %04x got %04x", 7, 0xAEFF, wrapbuf[7]);
    fail_unless((0xC0FE == wrapbuf[8]),
    	"wrapbuf[%d]: want %04x got %04x", 8, 0xC0FE, wrapbuf[8]);
    fail_unless((0xD0CC == wrapbuf[9]), 
    	"wrapbuf[%d]: want %04x got %04x", 9, 0xD0CC, wrapbuf[9]);

}
END_TEST

// No science data, explicit SIP source
START_TEST (test_no_science_data_sip)
{
    unsigned short databuf[DBSIZE];
    unsigned short wrapbuf[WBSIZE];

    int ret;
    unsigned long bufcnt;
    unsigned short checksum;
    unsigned short nbytes;
    int totbytes;

    telemwrap_init(TW_SIP);
    totbytes = telemwrap(databuf, wrapbuf, 0);
    fail_unless((totbytes == 20),
    	"totbytes: want %d got %d", 20, totbytes);

    // Initial header words
    fail_unless((0xF00D == wrapbuf[0]), NULL);
    fail_unless((0xD0CC == wrapbuf[1]),
    	"wrapbuf[%d]: want %04x got %04x", 1, 0xD0CC, wrapbuf[1]);
    fail_unless((0xAE01 == wrapbuf[2]), 
    	"wrapbuf[%d]: want %04x got %04x", 2, 0xAE01, wrapbuf[2]);

    // Buffer count
    memcpy(&bufcnt, wrapbuf+3, sizeof(bufcnt));
    fail_unless((bufcnt == 0), 
    	"bufcnt: want %lu got %lu", 0, bufcnt);

    // Number of science bytes
    memcpy(&nbytes, wrapbuf+5, sizeof(nbytes));
    fail_unless((nbytes == 0), 
    	"wrapbuf[%d] (nbytes): want %04x got %04x", 5, 0, wrapbuf[5]);

    // Should be no science data in this one. Next is checksum.
    memcpy(&checksum, wrapbuf+6, sizeof(checksum));
    fail_unless((checksum == 0),
    	"wrapbuf[%d] (checksum): want %04x got %04x", 6, 0, wrapbuf[6]);
    
    // Ending header words
    fail_unless((0xAEFF == wrapbuf[7]),
    	"wrapbuf[%d]: want %04x got %04x", 7, 0xAEFF, wrapbuf[7]);
    fail_unless((0xC0FE == wrapbuf[8]),
    	"wrapbuf[%d]: want %04x got %04x", 8, 0xC0FE, wrapbuf[8]);
    fail_unless((0xD0CC == wrapbuf[9]), 
    	"wrapbuf[%d]: want %04x got %04x", 9, 0xD0CC, wrapbuf[9]);

}
END_TEST

// No science data, explicit LOS source
START_TEST (test_no_science_data_los)
{
    unsigned short databuf[DBSIZE];
    unsigned short wrapbuf[WBSIZE];

    int ret;
    unsigned long bufcnt;
    unsigned short checksum;
    unsigned short nbytes;
    int totbytes;

    telemwrap_init(TW_LOS);
    totbytes = telemwrap(databuf, wrapbuf, 0);
    fail_unless((totbytes == 20),
    	"totbytes: want %d got %d", 20, totbytes);

    // Initial header words
    fail_unless((0xF00D == wrapbuf[0]), NULL);
    fail_unless((0xD0CC == wrapbuf[1]),
    	"wrapbuf[%d]: want %04x got %04x", 1, 0xD0CC, wrapbuf[1]);
    fail_unless((0xAE00 == wrapbuf[2]), 
    	"wrapbuf[%d]: want %04x got %04x", 2, 0xAE01, wrapbuf[2]);

    // Buffer count
    memcpy(&bufcnt, wrapbuf+3, sizeof(bufcnt));
    fail_unless((bufcnt == 0), 
    	"bufcnt: want %lu got %lu", 0, bufcnt);

    // Number of science bytes
    memcpy(&nbytes, wrapbuf+5, sizeof(nbytes));
    fail_unless((nbytes == 0), 
    	"wrapbuf[%d] (nbytes): want %04x got %04x", 5, 0, wrapbuf[5]);

    // Should be no science data in this one. Next is checksum.
    memcpy(&checksum, wrapbuf+6, sizeof(checksum));
    fail_unless((checksum == 0),
    	"wrapbuf[%d] (checksum): want %04x got %04x", 6, 0, wrapbuf[6]);
    
    // Ending header words
    fail_unless((0xAEFF == wrapbuf[7]),
    	"wrapbuf[%d]: want %04x got %04x", 7, 0xAEFF, wrapbuf[7]);
    fail_unless((0xC0FE == wrapbuf[8]),
    	"wrapbuf[%d]: want %04x got %04x", 8, 0xC0FE, wrapbuf[8]);
    fail_unless((0xD0CC == wrapbuf[9]), 
    	"wrapbuf[%d]: want %04x got %04x", 9, 0xD0CC, wrapbuf[9]);

}
END_TEST

// Science data sequence
START_TEST (test_science_sequence)
{
    unsigned short databuf[DBSIZE];
    unsigned short wrapbuf[WBSIZE];

    int ret;
    unsigned long bufcnt;
    unsigned short checksum;
    unsigned short nbytes;
    int science_bytes;
    int totbytes;

    unsigned short i;
    for (i=0; i<DBSIZE; i++) {
	databuf[i] = i;
    }

    telemwrap_init(TW_LOS);
    science_bytes = DBSIZE * sizeof(short);
    totbytes = telemwrap(databuf, wrapbuf, science_bytes);
    fail_unless((totbytes == science_bytes + 20),
    	"totbytes: want %d got %d", science_bytes + 20, totbytes);

    // Initial header words
    fail_unless((0xF00D == wrapbuf[0]), NULL);
    fail_unless((0xD0CC == wrapbuf[1]),
    	"wrapbuf[%d]: want %04x got %04x", 1, 0xD0CC, wrapbuf[1]);
    fail_unless((0xAE00 == wrapbuf[2]), 
    	"wrapbuf[%d]: want %04x got %04x", 2, 0xAE01, wrapbuf[2]);

    // Buffer count
    memcpy(&bufcnt, wrapbuf+3, sizeof(bufcnt));
    fail_unless((bufcnt == 0), 
    	"bufcnt: want %lu got %lu", 0, bufcnt);

    // Number of science bytes
    memcpy(&nbytes, wrapbuf+5, sizeof(nbytes));
    fail_unless((nbytes == science_bytes),
    	"nbytes: want %u got %u", science_bytes, nbytes);

    // Science data
    {
	unsigned short val;
	for (i=0; i<DBSIZE; i++) {
	    val = wrapbuf[6+i];
	    fail_unless((val == databuf[i]),
	    	"Bad science data at word offset %d: want %04x got %04x",
		i, databuf[i], wrapbuf[6+i]);
	}
    }

    // Checksum
    {
	unsigned short checksum_calc;
	checksum_calc = crc_short(wrapbuf+6, nbytes / sizeof(short));
	memcpy(&checksum, wrapbuf+6+i, sizeof(checksum));
	/**
	printf("=========== checksum %04x checksum_calc %04x\n",
	    checksum, checksum_calc);
	**/
	fail_unless((checksum == checksum_calc),
	    "checksum: want %04x got %04x", checksum_calc, checksum);
    }
    
    // Ending header words
    fail_unless((0xAEFF == wrapbuf[7+i]),
    	"wrapbuf[%d]: want %04x got %04x", 7+i, 0xAEFF, wrapbuf[7+i]);
    fail_unless((0xC0FE == wrapbuf[8+i]),
    	"wrapbuf[%d]: want %04x got %04x", 8+i, 0xC0FE, wrapbuf[8+i]);
    fail_unless((0xD0CC == wrapbuf[9+i]), 
    	"wrapbuf[%d]: want %04x got %04x", 9+i, 0xD0CC, wrapbuf[9+i]);

}
END_TEST

START_TEST (test_buffer_counting)
{
    unsigned short databuf[DBSIZE];
    unsigned short wrapbuf[WBSIZE];

    int ret;
    unsigned long bufcnt;
    unsigned short checksum;
    unsigned short nbytes;
    int totbytes;

    totbytes = telemwrap(databuf, wrapbuf, 0);
    fail_unless((totbytes == 20),
    	"totbytes: want %d got %d", 20, totbytes);

    // Initial header words
    fail_unless((0xF00D == wrapbuf[0]), NULL);
    fail_unless((0xD0CC == wrapbuf[1]),
    	"wrapbuf[%d]: want %04x got %04x", 1, 0xD0CC, wrapbuf[1]);
    fail_unless((0xAE00 == wrapbuf[2]), 
    	"wrapbuf[%d]: want %04x got %04x", 2, 0xAE01, wrapbuf[2]);

    // Buffer count
    memcpy(&bufcnt, wrapbuf+3, sizeof(bufcnt));
    fail_unless((bufcnt == 0), 
    	"bufcnt: want %lu got %lu", 0, bufcnt);

    // Number of science bytes
    memcpy(&nbytes, wrapbuf+5, sizeof(nbytes));
    fail_unless((nbytes == 0), 
    	"wrapbuf[%d] (nbytes): want %04x got %04x", 5, 0, wrapbuf[5]);

    // Should be no science data in this one. Next is checksum.
    memcpy(&checksum, wrapbuf+6, sizeof(checksum));
    fail_unless((checksum == 0),
    	"wrapbuf[%d] (checksum): want %04x got %04x", 6, 0, wrapbuf[6]);
    
    // Ending header words
    fail_unless((0xAEFF == wrapbuf[7]),
    	"wrapbuf[%d]: want %04x got %04x", 7, 0xAEFF, wrapbuf[7]);
    fail_unless((0xC0FE == wrapbuf[8]),
    	"wrapbuf[%d]: want %04x got %04x", 8, 0xC0FE, wrapbuf[8]);
    fail_unless((0xD0CC == wrapbuf[9]), 
    	"wrapbuf[%d]: want %04x got %04x", 9, 0xD0CC, wrapbuf[9]);

    // 2nd time
    totbytes = telemwrap(databuf, wrapbuf, 0);
    fail_unless((totbytes == 20),
    	"totbytes: want %d got %d", 20, totbytes);

    // Initial header words
    fail_unless((0xF00D == wrapbuf[0]), NULL);
    fail_unless((0xD0CC == wrapbuf[1]),
    	"wrapbuf[%d]: want %04x got %04x", 1, 0xD0CC, wrapbuf[1]);
    fail_unless((0xAE00 == wrapbuf[2]), 
    	"wrapbuf[%d]: want %04x got %04x", 2, 0xAE01, wrapbuf[2]);

    // Buffer count
    memcpy(&bufcnt, wrapbuf+3, sizeof(bufcnt));
    fail_unless((bufcnt == 1), 
    	"bufcnt: want %lu got %lu", 0, bufcnt);

    // Number of science bytes
    memcpy(&nbytes, wrapbuf+5, sizeof(nbytes));
    fail_unless((nbytes == 0), 
    	"wrapbuf[%d] (nbytes): want %04x got %04x", 5, 0, wrapbuf[5]);

    // Should be no science data in this one. Next is checksum.
    memcpy(&checksum, wrapbuf+6, sizeof(checksum));
    fail_unless((checksum == 0),
    	"wrapbuf[%d] (checksum): want %04x got %04x", 6, 0, wrapbuf[6]);
    
    // Ending header words
    fail_unless((0xAEFF == wrapbuf[7]),
    	"wrapbuf[%d]: want %04x got %04x", 7, 0xAEFF, wrapbuf[7]);
    fail_unless((0xC0FE == wrapbuf[8]),
    	"wrapbuf[%d]: want %04x got %04x", 8, 0xC0FE, wrapbuf[8]);
    fail_unless((0xD0CC == wrapbuf[9]), 
    	"wrapbuf[%d]: want %04x got %04x", 9, 0xD0CC, wrapbuf[9]);

}
END_TEST

// Odd number of science data bytes
START_TEST (test_odd_number_of_bytes)
{
    unsigned short databuf[DBSIZE];
    unsigned short wrapbuf[WBSIZE];

    int ret;
    unsigned long bufcnt;
    unsigned short checksum;
    unsigned short nbytes;
    int science_bytes;
    int totbytes;

    unsigned short d[] = { 0x0102, 0x0304, 0x0506, 0x0708, 0x090a };
    int NUMWDS = sizeof(d)/sizeof(d[0]);
    int i;
    for (i=0; i<NUMWDS; i++) {
	databuf[i] = d[i];
    }

    // Make it an odd number of science bytes.
    science_bytes = (NUMWDS * sizeof(short)) - 1;

    totbytes = telemwrap(databuf, wrapbuf, science_bytes);
    fail_unless((totbytes == science_bytes + 1 + 20),
    	"totbytes: want %d got %d", science_bytes + 20, totbytes);

    // Initial header words
    fail_unless((0xF00D == wrapbuf[0]), NULL);
    fail_unless((0xD0CC == wrapbuf[1]),
    	"wrapbuf[%d]: want %04x got %04x", 1, 0xD0CC, wrapbuf[1]);
    fail_unless((0xAE00 == wrapbuf[2]), 
    	"wrapbuf[%d]: want %04x got %04x", 2, 0xAE01, wrapbuf[2]);

    // Buffer count
    memcpy(&bufcnt, wrapbuf+3, sizeof(bufcnt));
    fail_unless((bufcnt == 0), 
    	"bufcnt: want %lu got %lu", 0, bufcnt);

    // Number of science bytes
    memcpy(&nbytes, wrapbuf+5, sizeof(nbytes));
    fail_unless((nbytes == science_bytes + 1),
    	"nbytes: want %u got %u", science_bytes, nbytes);

    // Science data
    {
	unsigned short val;
	for (i=0; i<NUMWDS-1; i++) {
	    val = wrapbuf[6+i];
	    //printf("%d. %04x\t", i, val);
	    fail_unless((val == databuf[i]),
	    	"Bad science data at word offset %d: want %04x got %04x",
		i, databuf[i], wrapbuf[6+i]);
	}
	val = wrapbuf[6+i];
	fail_unless((val == 0xDA0A),
	    "Bad science data at word offset %d: want %04x got %04x",
	    i, 0xDA0A, wrapbuf[6+i]);
	//printf("%d. %04x\n", i, val);
	i++;
    }

    // Checksum
    {
	unsigned short checksum_calc;
	checksum_calc = crc_short(wrapbuf+6, nbytes / sizeof(short));
	memcpy(&checksum, wrapbuf+6+i, sizeof(checksum));
	/**
	printf("=========== checksum %04x checksum_calc %04x\n",
	    checksum, checksum_calc);
	**/
	fail_unless((checksum == checksum_calc),
	    "checksum: want %04x got %04x", checksum_calc, checksum);
    }
    
    // Ending header words
    fail_unless((0xAEFF == wrapbuf[7+i]),
    	"wrapbuf[%d]: want %04x got %04x", 7+i, 0xAEFF, wrapbuf[7+i]);
    fail_unless((0xC0FE == wrapbuf[8+i]),
    	"wrapbuf[%d]: want %04x got %04x", 8+i, 0xC0FE, wrapbuf[8+i]);
    fail_unless((0xD0CC == wrapbuf[9+i]), 
    	"wrapbuf[%d]: want %04x got %04x", 9+i, 0xD0CC, wrapbuf[9+i]);

}
END_TEST

Suite *
telemwrap_suite(void)
{
    Suite *s = suite_create("telemwrap");
    TCase *tc_core = tcase_create("Core");
    tcase_add_test(tc_core, test_no_science_data);
    tcase_add_test(tc_core, test_no_science_data_sip);
    tcase_add_test(tc_core, test_no_science_data_los);
    tcase_add_test(tc_core, test_science_sequence);
    tcase_add_test(tc_core, test_buffer_counting);
    tcase_add_test(tc_core, test_odd_number_of_bytes);
    //tcase_add_test(tc_core, test_first_header);
    suite_add_tcase(s, tc_core);
    return s;
}

int
main(int argc, char *argv[])
{
    int number_failed;
    Suite *s = telemwrap_suite();
    SRunner *sr = srunner_create(s);
    //srunner_run_all(sr, CK_VERBOSE);
    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return number_failed;
}
