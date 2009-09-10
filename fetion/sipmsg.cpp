#include <stdafx.h>

struct sipmsg *sipmsg_parse_msg(LPCSTR msg) {
	const char *tmp = strstr(msg, "\r\n\r\n");
	char *line;
	struct sipmsg *smsg;

	if(!tmp) return NULL;

	line = g_strndup(msg, tmp - msg);

	smsg = sipmsg_parse_header(line);
	smsg->body = mir_strdup(tmp + 4);

	mir_free(line);
	return smsg;
}

struct sipmsg *sipmsg_parse_header(LPCSTR header) {
	struct sipmsg *msg = (struct sipmsg*) mir_alloc(sizeof(struct sipmsg));
	ZeroMemory(msg,sizeof(struct sipmsg));
	LPSTR *lines = g_strsplit(header,"\r\n",0);
	LPSTR *parts;
	LPSTR dummy;
	LPSTR dummy2;
	LPSTR tmp;
	LPCSTR tmp2;
	int i=1;
	if(!lines[0]) return NULL;
	parts = g_strsplit(lines[0], " ", 3);
	if(!parts[0] || !parts[1] || !parts[2]) {
		g_strfreev(parts);
		g_strfreev(lines);
		mir_free(msg);
		return NULL;
	}
	if (*parts[0]=='I') {
		util_log("Here");
	}

	if(strstr(parts[0],"SIP-C/2.0")) { /* numeric response */
		//fix me
		msg->method = mir_strdup(parts[2]);
		msg->response = strtol(parts[1],NULL,10);
	} else { /* request */
		msg->method = mir_strdup(parts[0]);
		msg->target = mir_strdup(parts[1]);
		msg->response = 0;
	}
	g_strfreev(parts);
	for(i=1; lines[i] && strlen(lines[i])>2; i++) {
		parts = g_strsplit(lines[i], ": ", 2);
		if(!parts[0] || !parts[1]) {
			g_strfreev(parts);
			g_strfreev(lines);
			mir_free(msg);
			return NULL;
		}
		dummy = parts[1];
		dummy2 = 0;
		while(*dummy==' ' || *dummy=='\t') dummy++;
		dummy2 = mir_strdup(dummy);
		while(lines[i+1] && (lines[i+1][0]==' ' || lines[i+1][0]=='\t')) {
			i++;
			dummy = lines[i];
			while(*dummy==' ' || *dummy=='\t') dummy++;
			tmp = g_strdup_printf("%s %s",dummy2, dummy);
			mir_free(dummy2);
			dummy2 = tmp;
		}
		//purple_debug_info("parse:","head:[%s] data[%s]\n",parts[0],dummy2);
		sipmsg_add_header(msg, parts[0], dummy2);
		g_strfreev(parts);
	}
	g_strfreev(lines);
	tmp2 = sipmsg_find_header(msg, "L");//Content-Length
	if (tmp2 != NULL)
		msg->bodylen = strtol(tmp2, NULL, 10);
	if(msg->response) {
		tmp2 = sipmsg_find_header(msg, "Q");//CSeq
		if(!tmp2) {
			/* SHOULD NOT HAPPEN */
			msg->method = 0;
		} else {
			parts = g_strsplit(tmp2, " ", 2);
			msg->method = mir_strdup(parts[1]);
			g_strfreev(parts);
		}
	}
	return msg;
}

void sipmsg_print(const struct sipmsg *msg) {
	GSList *cur;
	struct siphdrelement *elem;
	util_log("SIP MSG");
	util_log("response: %d\nmethod: %s\nbodylen: %d",msg->response,msg->method,msg->bodylen);
	if(msg->target) util_log("target: %s",msg->target);
	cur = msg->headers;
	while(cur) {
		elem = (struct siphdrelement*)cur->data;
		util_log("\tname: %s value: %s",elem->name, elem->value);
		cur = g_slist_next(cur);
	}
}

char *sipmsg_to_string(const struct sipmsg *msg) {
	GSList* cur;
	LPSTR outstr = (LPSTR)mir_alloc(65535);
	struct siphdrelement *elem;

	if(msg->response)
		sprintf(outstr, "SIP-C/2.0 %d Unknown\r\n", msg->response);
	else
		sprintf(outstr, "%s %s SIP-C/2.0\r\n", msg->method, msg->target);

	cur = msg->headers;
	while(cur) {
		elem = (struct siphdrelement *)cur->data;
		sprintf(outstr+strlen(outstr), "%s: %s\r\n", elem->name, elem->value);
		cur = g_slist_next(cur);
	}

	sprintf(outstr+strlen(outstr), "\r\n%s", msg->bodylen ? msg->body : "");

	return outstr;
}
void sipmsg_add_header(struct sipmsg *msg, LPCSTR name, LPCSTR value) {
	struct siphdrelement *element = (struct siphdrelement*)mir_alloc(sizeof(struct siphdrelement));
	element->name = mir_strdup(name);
	element->value = mir_strdup(value);
	msg->headers = g_slist_append(msg->headers, element);
}

void sipmsg_free(struct sipmsg *msg) {
	struct siphdrelement *elem;

	while(msg->headers) {
		elem = (struct siphdrelement *)msg->headers->data;
		msg->headers = g_slist_remove(msg->headers,elem);
		mir_free(elem->name);
		mir_free(elem->value);
		mir_free(elem);
	}
	mir_free(msg->method);
	mir_free(msg->target);
	mir_free(msg->body);
	mir_free(msg);
}

void sipmsg_remove_header(struct sipmsg *msg, LPCSTR name) {
	struct siphdrelement *elem;
	GSList *tmp = msg->headers;
	while(tmp) {
		elem = (struct siphdrelement *)tmp->data;
		if(strcmp(elem->name, name)==0) {
			msg->headers = g_slist_remove(msg->headers, elem);
			mir_free(elem->name);
			mir_free(elem->value);
			mir_free(elem);
			return;
		}
		tmp = g_slist_next(tmp);
	}
}

LPCSTR sipmsg_find_header(struct sipmsg *msg, LPCSTR name) {
	GSList *tmp;
	struct siphdrelement *elem;
	tmp = msg->headers;
	while(tmp) {
		elem = (struct siphdrelement *)tmp->data;
		if(strcmp(elem->name, name)==0) {
			return elem->value;
		}
		tmp = g_slist_next(tmp);
	}
	return NULL;
}

