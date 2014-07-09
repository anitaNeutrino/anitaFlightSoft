/* conf_simple.h - simple config file
 *
 * Marty Olevitch, Feb '08
 *
 * conf_simple provides a way to configure various parameters at run time.
 *
 * The key is the name of a file and the value is the first line in the
 * file. Subsequent lines can be used for comments.
 *
 * Something similar is used in the qmail distribution by Dan Bernstein.
 *
 */

#ifndef CONF_SIMPLE_H
#define CONF_SIMPLE_H

// CONF_SIMPLE_VALUE_SIZE - number of bytes read from a conf file.
#define CONF_SIMPLE_VALUE_SIZE 80

// CONF_SIMPLE_MAX - maximum number of config directories.
#define CONF_SIMPLE_MAX 50

// conf_set_dir - specify name of directory that holds the config files. 
// Returns a token >= 0 if successful, else -1.
int conf_set_dir(const char *dirname);

// conf_get_dir - returns name of directory that holds the config files. 
//      tok - token obtained from conf_set_dir()
// Returns NULL if the token is invalid, else the directory string.
char* conf_get_dir(int tok);

// conf_read - get the value corresponding to the key. If all went well,
//      tok - token obtained from conf_set_dir()
// return 0, else a negative value.
int conf_read(int tok, const char* key, char *value);

// conf_write - set the value corresponding to the key. If all went well,
//      tok - token obtained from conf_set_dir()
// return 0, else a negative value.
int conf_write(int tok, const char* key, char *value);

// conf_strerror - returns a hopefully helpful error string. Should only be
// used after getting a non-zero return code from conf_read() or
// conf_write().
//      tok - token obtained from conf_set_dir()
// Returns NULL if token is invalid.
char *conf_strerror(int tok);

#endif // CONF_SIMPLE_H
