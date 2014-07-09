// conf_simple.c

#include "conf_simple.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <stdlib.h>

static int Nconf = 0;
static char *Dir[CONF_SIMPLE_MAX];
#define ERROR_STRING_LEN 2048
static char *Error_string[ERROR_STRING_LEN];

static void
set_error_string(int tok, char *fmt, ...)
{
    va_list ap;
    if (fmt == NULL) {
        Error_string[tok][0] = '\0';
    } else {
        va_start(ap, fmt);
        vsnprintf(Error_string[tok], ERROR_STRING_LEN, fmt, ap);
        va_end(ap);
    }
}

char* conf_get_dir(int tok)
{
    if (tok < Nconf) {
        return Dir[tok];
    }
    return NULL;
}

int conf_read(int tok, const char* key, char *value)
{
    FILE *fp;
    char filename[512];

    if (tok < 0 || tok >= Nconf) {
        return -4;
    }

    if (NULL == Dir[tok]) {
	set_error_string(tok, "conf_read: null directory name");
	return -1;
    }

    sprintf(filename, "%s/%s", Dir[tok], key);
    fp = fopen(filename, "rt");
    if (NULL == fp) {
	set_error_string(tok, "conf_read: bad fopen on '%s' (%s)",
	    filename, strerror(errno));
	return -2;
    }

    {
	char *s;
	int loc;

	s = fgets(value, CONF_SIMPLE_VALUE_SIZE, fp);
	if (NULL == s) {
	    set_error_string(tok, "conf_read: bad fgets (%s)",
		strerror(errno));
	    return -3;
	}
	loc = strlen(value)-1;
	if ('\n' == value[loc]) {
	    value[loc] = '\0';	// delete \n
	}
    }

    fclose(fp);
    return 0;
}

int conf_write(int tok, const char* key, char *value)
{
    FILE *fp;
    char filename[512];

    if (tok < 0 || tok >= Nconf) {
        return -4;
    }

    if (NULL == Dir) {
	set_error_string(tok, "conf_write: null directory name");
	return -1;
    }

    sprintf(filename, "%s/%s", Dir[tok], key);
    fp = fopen(filename, "wt");
    if (NULL == fp) {
	set_error_string(tok, "conf_write: bad fopen on '%s' (%s)",
	    filename, strerror(errno));
	return -2;
    }

    fprintf(fp, "%s\n", value);
    fclose(fp);
    return 0;
}

int conf_set_dir(const char *dirname)
{
    if (Nconf >= CONF_SIMPLE_MAX) {
        return -1;
    }

    Dir[Nconf] = strdup(dirname);
    if (NULL == Dir[Nconf]) {
        return -2;
    }

    Error_string[Nconf] = (char *)malloc(ERROR_STRING_LEN);
    if (NULL == Error_string[Nconf]) {
        free(Dir[Nconf]);
        return -3;
    }

    Nconf++;
    return Nconf - 1;
}

char *
conf_strerror(int tok)
{
    if (tok < 0 || tok >= Nconf) {
        return NULL;
    }

    return Error_string[tok];
}
