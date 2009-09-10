#ifndef _PURPLE_SIPMSG_H
#define _PURPLE_SIPMSG_H

#include <glib.h>

struct siphdrelement;

struct sipmsg {
	int response; /* 0 means request, otherwise response code */
	LPSTR method;
	LPSTR target;
	GSList* headers;
	int bodylen;
	LPSTR body;
};

struct siphdrelement {
	LPSTR name;
	LPSTR value;
};

struct sipmsg *sipmsg_parse_msg(LPCSTR msg);
struct sipmsg *sipmsg_parse_header(LPCSTR header);
void sipmsg_add_header(struct sipmsg *msg, LPCSTR name, LPCSTR value);
void sipmsg_free(struct sipmsg *msg);
LPCSTR sipmsg_find_header(struct sipmsg *msg, LPCSTR name);
void sipmsg_remove_header(struct sipmsg *msg, LPCSTR name);
void sipmsg_print(const struct sipmsg *msg);
char *sipmsg_to_string(const struct sipmsg *msg);
#endif /* _PURPLE_FETION_H */
