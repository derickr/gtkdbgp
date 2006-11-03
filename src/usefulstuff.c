/*
   +----------------------------------------------------------------------+
   | Xdebug                                                               |
   +----------------------------------------------------------------------+
   | Copyright (c) 2002, 2003, 2004, 2005, 2006 Derick Rethans            |
   +----------------------------------------------------------------------+
   | This source file is subject to version 1.0 of the Xdebug license,    |
   | that is bundled with this package in the file LICENSE, and is        |
   | available at through the world-wide-web at                           |
   | http://xdebug.derickrethans.nl/license.php                           |
   | If you did not receive a copy of the Xdebug license and are unable   |
   | to obtain it through the world-wide-web, please send a note to       |
   | xdebug@derickrethans.nl so we can mail you a copy immediately.       |
   +----------------------------------------------------------------------+
   | Authors:  Derick Rethans <derick@xdebug.org>                         |
   +----------------------------------------------------------------------+
 */

#include <stdlib.h>
#include <string.h>
#ifndef WIN32
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/file.h>
#else
#define PATH_MAX MAX_PATH
#include <winsock2.h>
#include <io.h>
#include "win32/time.h"
#include <process.h>
#endif
#include "xdebug_mm.h"
#include "xdebug_str.h"
#include "usefulstuff.h"

#define READ_BUFFER_SIZE 128

char* fd_read_line_delim(int socket, fd_buf *context, int type, unsigned char delim, int *length)
{
	int size = 0, newl = 0, nbufsize = 0;
	char *tmp;
	char *tmp_buf = NULL;
	char *ptr;
	char buffer[READ_BUFFER_SIZE + 1];

	if (!context->buffer) {
		context->buffer = calloc(1,1);
		context->buffer_size = 0;
	}

	while (context->buffer_size < 1 || context->buffer[context->buffer_size - 1] != delim) {
		ptr = context->buffer + context->buffer_size;
		if (type == FD_RL_FILE) {
			newl = read(socket, buffer, READ_BUFFER_SIZE);
		} else {
			newl = recv(socket, buffer, READ_BUFFER_SIZE, 0);
		}
		if (newl > 0) {
			context->buffer = realloc(context->buffer, context->buffer_size + newl + 1);
			memcpy(context->buffer + context->buffer_size, buffer, newl);
			context->buffer_size += newl;
			context->buffer[context->buffer_size] = '\0';
		} else {
			return NULL;
		}
	}

	ptr = memchr(context->buffer, delim, context->buffer_size);
	size = ptr - context->buffer;
	/* Copy that line into tmp */
	tmp = malloc(size + 1);
	tmp[size] = '\0';
	memcpy(tmp, context->buffer, size);
	/* Rewrite existing buffer */
	if ((nbufsize = context->buffer_size - size - 1)  > 0) {
		tmp_buf = malloc(nbufsize + 1);
		memcpy(tmp_buf, ptr + 1, nbufsize);
		tmp_buf[nbufsize] = 0;
	}
	free(context->buffer);
	context->buffer = tmp_buf;
	context->buffer_size = context->buffer_size - (size + 1);

	/* Return normal line */
	if (length) {
		*length = size;
	}
	return tmp;
}

char *xdebug_join(char *delim, xdebug_arg *args, int begin, int end)
{
	int         i;
	xdebug_str *ret;

	xdebug_str_ptr_init(ret);
	if (begin < 0) {
		begin = 0;
	}
	if (end > args->c - 1) {
		end = args->c - 1;
	}
	for (i = begin; i < end; i++) {
		xdebug_str_add(ret, args->args[i], 0);
		xdebug_str_add(ret, delim, 0);
	}
	xdebug_str_add(ret, args->args[end], 0);
	return ret->d;
}

void xdebug_explode(char *delim, char *str, xdebug_arg *args, int limit) 
{
	char *p1, *p2, *endp;

	endp = str + strlen(str);

	p1 = str;
	p2 = xdebug_memnstr(str, delim, strlen(delim), endp);

	if (p2 == NULL) {
		args->c++;
		args->args = (char**) xdrealloc(args->args, sizeof(char*) * args->c);
		args->args[args->c - 1] = (char*) xdmalloc(strlen(str) + 1);
		memcpy(args->args[args->c - 1], p1, strlen(str));
		args->args[args->c - 1][strlen(str)] = '\0';
	} else {
		do {
			args->c++;
			args->args = (char**) xdrealloc(args->args, sizeof(char*) * args->c);
			args->args[args->c - 1] = (char*) xdmalloc(p2 - p1 + 1);
			memcpy(args->args[args->c - 1], p1, p2 - p1);
			args->args[args->c - 1][p2 - p1] = '\0';
			p1 = p2 + strlen(delim);
		} while ((p2 = xdebug_memnstr(p1, delim, strlen(delim), endp)) != NULL && (limit == -1 || --limit > 1));

		if (p1 <= endp) {
			args->c++;
			args->args = (char**) xdrealloc(args->args, sizeof(char*) * args->c);
			args->args[args->c - 1] = (char*) xdmalloc(endp - p1 + 1);
			memcpy(args->args[args->c - 1], p1, endp - p1);
			args->args[args->c - 1][endp - p1] = '\0';
		}
	}
}

char* xdebug_memnstr(char *haystack, char *needle, int needle_len, char *end)
{
	char *p = haystack;
	char first = *needle;

	/* let end point to the last character where needle may start */
	end -= needle_len;
	
	while (p <= end) {
		while (*p != first)
			if (++p > end)
				return NULL;
		if (memcmp(p, needle, needle_len) == 0)
			return p;
		p++;
	}
	return NULL;
}

double xdebug_get_utime(void)
{
#ifdef HAVE_GETTIMEOFDAY
	struct timeval tp;
	long sec = 0L;
	double msec = 0.0;

	if (gettimeofday((struct timeval *) &tp, NULL) == 0) {
		sec = tp.tv_sec;
		msec = (double) (tp.tv_usec / MICRO_IN_SEC);

		if (msec >= 1.0) {
			msec -= (long) msec;
		}
		return msec + sec;
	}
#endif
	return 0;
}

/* not all versions of php export this */
static int xdebug_htoi(char *s)
{
	int value;
	int c;

	c = s[0];
	if (isupper(c)) {
		c = tolower(c);
	}
	value = (c >= '0' && c <= '9' ? c - '0' : c - 'a' + 10) * 16;

	c = s[1];
	if (isupper(c)) {
		c = tolower(c);
	}
	value += c >= '0' && c <= '9' ? c - '0' : c - 'a' + 10;

	return value;
}

/* not all versions of php export this */
int xdebug_raw_url_decode(char *str, int len)
{
	char *dest = str;
	char *data = str;

	while (len--) {
		if (*data == '%' && len >= 2 && isxdigit((int) *(data + 1)) && isxdigit((int) *(data + 2))) {
			*dest = (char) xdebug_htoi(data + 1);
			data += 2;
			len -= 2;
		} else {
			*dest = *data;
		}
		data++;
		dest++;
	}
	*dest = '\0';
	return dest - str;
}

static unsigned char hexchars[] = "0123456789ABCDEF";

char *xdebug_raw_url_encode(char const *s, int len, int *new_length, int skip_slash)
{
	register int x, y;
	unsigned char *str;

	str = (unsigned char *) xdmalloc(3 * len + 1);
	for (x = 0, y = 0; len--; x++, y++) {
		str[y] = (unsigned char) s[x];
		if ((str[y] < '0' && str[y] != '-' && str[y] != '.' && (str[y] != '/' || !skip_slash)) ||
			(str[y] < 'A' && str[y] > '9') ||
			(str[y] > 'Z' && str[y] < 'a' && str[y] != '_') ||
			(str[y] > 'z'))
		{
			str[y++] = '%';
			str[y++] = hexchars[(unsigned char) s[x] >> 4];
			str[y] = hexchars[(unsigned char) s[x] & 15];
		}
	}
	str[y] = '\0';
	if (new_length) {
		*new_length = y;
	}
	return ((char *) str);
}

/* fake URI's per IETF RFC 1738 and 2396 format */
char *xdebug_path_from_url(const char *fileurl)
{
	/* deal with file: url's */
	char dfp[PATH_MAX * 2];
	const char *fp = dfp, *efp = fileurl;
	int l = 0;
#ifdef PHP_WIN32
	int i;
#endif
	char *tmp = NULL, *ret = NULL;;

	memset(dfp, 0, sizeof(dfp));
	strncpy(dfp, efp, sizeof(dfp) - 1);
	xdebug_raw_url_decode(dfp, strlen(dfp));
	tmp = strstr(fp, "file://");

	if (tmp) {
		fp = tmp + 7;
		if (fp[0] == '/' && fp[2] == ':') {
			fp++;
		}
		ret = xdstrdup(fp);
		l = strlen(ret);
#ifdef PHP_WIN32
		/* convert '/' to '\' */
		for (i = 0; i < l; i++) {
			if (ret[i] == '/') {
				ret[i] = '\\';
			}
		}
#endif
	} else {
		ret = xdstrdup(fileurl);
	}

	return ret;
}

long xdebug_crc32(const char *string, int str_len)
{
	unsigned int crc = ~0;
	int len;
	
	len = 0 ;
	for (len += str_len; str_len--; ++string) {
	    XDEBUG_CRC32(crc, *string);
	}
	return ~crc;
}
