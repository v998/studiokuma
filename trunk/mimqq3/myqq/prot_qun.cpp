/*
 *  prot_qun.c
 *
 *  QQ Protocol. Part Qun
 *
 *  Copyright (C) 2008  Huang Guan
 *
 *  2008-7-12 Created.
 *
 *  Description: This file mainly includes the functions about 
 *  
 *
 */

#include <StdAfx.h>

extern "C" {
#include "qqclient.h"
#include "memory.h"
#include "debug.h"
#include "qqpacket.h"
#include "packetmgr.h"
#include "protocol.h"
#include "qun.h"
#include "utf8.h"

void prot_qun_get_info( struct qqclient* qq, uint number, uint pos )
{
	qqpacket* p = packetmgr_new_send( qq, QQ_CMD_QUN_CMD );
	if( !p ) return;
	bytebuffer *buf = p->buf;
	put_byte( buf, 0x72 );	//command?
	put_int( buf, number );	//
	put_int( buf, pos );	//
	post_packet( qq, p, SESSION_KEY );
}

void prot_qun_send_msg( struct qqclient* qq, uint number, char* msg_content )
{
	qqpacket* p = packetmgr_new_send( qq, QQ_CMD_QUN_CMD );
	if( !p ) return;
	ushort len = strlen( msg_content );
	bytebuffer *buf = p->buf;
	put_byte( buf, 0x2A );
	put_int( buf, number );
	bytebuffer* content_buf;
	NEW( content_buf, sizeof(bytebuffer),bytebuffer );
	if( !content_buf ) {
		packetmgr_del_packet( &qq->packetmgr, p );
		return;
	}
	content_buf->size = PACKET_SIZE;
	
	put_word( content_buf, 0x0001 );	//text type
	put_byte( content_buf, 0x01 );		//slice_count
	put_byte( content_buf, 0x00 );		//slice_no
	put_word( content_buf, 0 );		//id??
	put_int( content_buf, 0 );		//zeros

	put_int( content_buf, 0x4D534700 ); //"MSG"
	put_int( content_buf, 0x00000000 );
	put_int( content_buf, p->time_create );
	put_int( content_buf, rand() );
	put_int( content_buf, 0x00000000 );
	put_int( content_buf, 0x09008600 );
	// char font_name[] = "宋体";	//must be in UTF8
	char font_name[] = "\xe5\xae\x8b\xe4\xbd\x93";	//must be in UTF8
	put_word( content_buf, strlen(font_name) );
	put_data( content_buf, (uchar*)font_name, strlen( font_name) );
	put_word( content_buf, 0x0000 );
	put_byte( content_buf, 0x01 );
	put_word( content_buf, len+3 );
	put_byte( content_buf, 1 );			//unknown, keep 1
	put_word( content_buf, len );
	put_data( content_buf, (uchar*)msg_content, len );
	
	put_word( buf, content_buf->pos );
	put_data( buf, content_buf->data, content_buf->pos );
	DEL( content_buf );
	post_packet( qq, p, SESSION_KEY );
}

void prot_qun_get_memberinfo( struct qqclient* qq, uint number, uint* numbers, int count )
{
	qqpacket* p = packetmgr_new_send( qq, QQ_CMD_QUN_CMD );
	if( !p ) return;
	bytebuffer *buf = p->buf;
	put_byte( buf, 0x0C );	//command?
	put_int( buf, number );	//
	int i;
	if( count > 30 ) count = 30;	//TXQQ一次获取30个。
	for( i=0; i<count; i++ ){
		put_int( buf, numbers[i] );	//
	}
	post_packet( qq, p, SESSION_KEY );
}

void prot_qun_get_online( struct qqclient* qq, uint number )
{
	qqpacket* p = packetmgr_new_send( qq, QQ_CMD_QUN_CMD );
	if( !p ) return;
	bytebuffer *buf = p->buf;
	put_byte( buf, 0x0B );	//command?
	put_int( buf, number );	//
	post_packet( qq, p, SESSION_KEY );
}

void prot_qun_get_membername( struct qqclient* qq, uint number, uint pos )
{
	qqpacket* p = packetmgr_new_send( qq, QQ_CMD_QUN_CMD );
	if( !p ) return;
	bytebuffer *buf = p->buf;
	put_byte( buf, 0x0F );	//command?
	put_int( buf, number );	//
	put_int( buf, 0x0 );	//?? 
	put_int( buf, pos );	//position
	post_packet( qq, p, SESSION_KEY );
}

static void parse_quninfo( struct qqclient* qq, qqpacket* p, qqqun* q )
{
	uint last_number;
	uchar more, status;
	bytebuffer *buf = p->buf;
	q->ext_number = get_int( buf );
	get_word( buf );	//00 00
	get_byte( buf );	//00
	status = get_byte( buf );	//03 or 02
	if( status == 3 ){
		q->type = get_byte( buf );
		get_int( buf );	//unknown
	//	get_int( buf );	//(???)unknown in 1205
		q->owner = get_int( buf );
		q->auth_type = get_byte( buf );
		buf->pos += 6;
		q->category = get_int( buf );
		q->max_member = get_word( buf );
		buf->pos += 5;
		q->version = get_int( buf );
		
		//name
		uchar len = get_byte( buf );
		len = MIN( NICKNAME_LEN-1, len );
		get_data( buf,  (uchar*)q->name, len );
		q->name[len] = 0;
	//	DBG("qun: %s", q->name );
		get_byte( buf );
		get_byte( buf );	//separator
		//ann
		len = get_byte( buf );
		
		get_data( buf,  (uchar*)q->ann, len );
		q->ann[len] = 0;
		//intro
		len = get_byte( buf );
		get_data( buf,  (uchar*)q->intro, len );
		q->intro[len] = 0;
		//token data
		get_token( buf, &q->token_cmd );
		get_word( buf ); //00 05  qq2011 beta2
	}
	last_number = get_int( buf );	//member last came in
	more = get_byte( buf );	//more member data
	while( buf->pos < buf->len ){
		uint n = get_int( buf );
		uchar org = get_byte( buf );
		uchar role = get_byte( buf );
		qunmember* m = qun_member_get( qq, q, n, 1 );
		if( m==NULL ){
			DBG("m==NULL");
			break;
		}
		m->org = org;
		m->role = role;
	}
	if( more ){
		prot_qun_get_info( qq, q->number, last_number );
	}else{
		qun_update_memberinfo( qq, q );
		qun_set_members_off( qq, q );
		qun_update_online( qq, q );
	}
}

static void parse_memberinfo( struct qqclient* qq, qqpacket* p, qqqun* q )
{
	bytebuffer *buf = p->buf;
	while( buf->pos < buf->len ){
		uint number = get_int( buf );
		qunmember* m = qun_member_get( qq, q, number, 0 );
		if( !m ){
			DBG("m==NULL  number: %d", number);
			break;
		}
		m->face = get_word( buf );
		m->age = get_byte( buf );
		m->sex = get_byte( buf );
		uchar name_len = get_byte( buf );
		name_len = MIN( NICKNAME_LEN-1, name_len );
		get_data( buf,  (uchar*)m->nickname, name_len );
		m->nickname[name_len] = 0;
		//TX技术改革不彻底，还保留使用GB码 2009-1-25 11:02
		//gb_to_utf8( m->nickname, m->nickname, NICKNAME_LEN-1 ); after qq2011 beta2 tx used utf8
		get_word( buf );	//00 00
		m->qqshow = get_byte( buf );
		m->flag = get_byte( buf );
	}
}

static void parse_online( struct qqclient* qq, qqpacket* p, qqqun* q )
{
	bytebuffer *buf = p->buf;
	get_byte( buf );
	//set all off
	qun_set_members_off( qq, q );
	while( buf->pos < buf->len ){
		uint number = get_int( buf );
		qunmember* m = qun_member_get( qq, q, number, 1 );
		if( m )
			m->status = QQ_ONLINE;
	}
	qun_put_single_event( qq, q );
}

static void parse_membername( struct qqclient* qq, qqpacket* p, qqqun* q )
{
	bytebuffer *buf = p->buf;
	uint pos;
	q->realnames_version = get_int( buf );
	pos = get_int( buf );	// next position, 0 when finish
	while( buf->pos < buf->len ){
		uint number = get_int( buf );
		qunmember* m = qun_member_get( qq, q, number, 0 );
		if( !m ){
			DBG("m==NULL");
			break;
		}
		uchar name_len = get_byte( buf );
		if( name_len > 0 ) {
			name_len = MIN( NICKNAME_LEN-1, name_len );
			get_data( buf,  (uchar*)m->nickname, name_len );
			m->nickname[name_len] = 0;
			gb_to_utf8( m->nickname, m->nickname, NICKNAME_LEN-1 );
			// DBG("membername: %u=%s",number,m->nickname);
		}
	}

	if( pos ) {
		prot_qun_get_membername( qq, q->number, pos );
	} else {
		char event[64];
		sprintf( event, "clustermembernames^$%u", q->number );
		qqclient_put_event( qq, event );
	}
}

static void parse_search( struct qqclient* qq, qqpacket* p )
{
	bytebuffer *buf = p->buf;
	uchar search_type;
	uchar len;
	char event[1024] = "qun_search_reply^$";
	int offset = strlen(event);

	search_type = get_byte( buf );

	if ( search_type != 0 ) {
		offset += sprintf( event + offset, "%d^$", (int) search_type ); // search_type
		offset += sprintf( event + offset, "%u^$%u^$", get_int( buf ), get_int( buf ) ); // extid, intid
		offset += sprintf( event + offset, "%d^$", (int) get_byte( buf ) ); // type
		buf->pos += 4;
		offset += sprintf( event + offset, "%u^$", get_int( buf ) ); // creator
		buf->pos += 6;
		offset += sprintf( event + offset, "%u^$", get_int( buf ) ); // category

		len = get_byte( buf );
		offset += get_data( buf, (uchar*)(event + offset), len ); // name
		strcpy( event + offset, "^$" );
		offset += 2;

		buf->pos += 2;
		offset += sprintf( event + offset, "%d^$", (int) get_byte( buf ) ); // auth_type

		len = get_byte( buf );
		offset += get_data( buf, (uchar*)(event + offset), len ); // intro
		event[offset] = 0;
	} else
		strcpy( event + offset, "0" );

	qqclient_put_event( qq, event );
}

static void parse_join( struct qqclient* qq, qqpacket* p, uint number, uchar result ) {
	bytebuffer *buf = p->buf;
	char event[1024];

	if ( result == 0 ) {
		uint qun_id = get_int( buf );
		uchar join_reply = get_byte( buf );
		sprintf( event, "qunmyjoin^$%u^$%d^$%u^$%d", number, result, qun_id, join_reply );
	} else {
		int len = buf->len - buf->pos;
		buf->data[buf->pos+len] = 0;
		sprintf( event, "qunmyjoin2^$%u^$%d^$", number, result );
		gb_to_utf8( (char*)(buf->data+buf->pos), event + strlen( event ), 256 );
	}

	qqclient_put_event( qq, event );
}

void prot_qun_cmd_reply( struct qqclient* qq, qqpacket* p )
{
	bytebuffer *buf = p->buf;
	uchar cmd = get_byte( buf );
	uchar result = get_byte( buf );
	if( result != 0 && cmd != 0x07 ){
		DBG("result = %d", result );
		return ;
	}

	if ( cmd == 0x06 ) {
		parse_search( qq, p );
		return;
	}

	uint number = get_int( buf );
	qqqun* q = qun_get( qq, number, 0 );
	if( !q && cmd != 0x07 && cmd != 0x08 ){
		DBG("q==null");
		return;
	}
	switch( cmd ){
		case 0x2A:	//send msg;
			break;
		case 0x72:
			parse_quninfo( qq, p, q );
			break;
		case 0x0C:
			parse_memberinfo( qq, p, q );
			break;
		case 0x0B:
			parse_online( qq, p, q );
			break;
		case 0x0F:
			parse_membername( qq, p, q );
			break;
		case 0x19: /*0x07:*/ // Result for my joining
			parse_join( qq, p, number, result );
			break;
		case 0x08: // auth: just an ack for auth result, no use for parsing (just a qun id)
			break;
		default:
			DBG("unknown cmd = %x", cmd );
	}

}

// MIMQQ3
void prot_qun_search( struct qqclient* qq, uint number )
{
	qqpacket* p = packetmgr_new_send( qq, QQ_CMD_QUN_CMD );
	if( !p ) return;
	bytebuffer *buf = p->buf;
	put_byte( buf, 0x06 /*QQ_ROOM_CMD_SEARCH*/ );
	put_byte( buf, number ? 0x01 /*QQ_ROOM_SEARCH_TYPE_BY_ID*/ : 0x02 /*QQ_ROOM_SEARCH_TYPE_DEMO*/ );
	put_int( buf, number );
	post_packet( qq, p, SESSION_KEY );
}

ushort prot_qun_send_msg_format( struct qqclient* qq, uint to, char* msg, char* fontname, int fontsize, uchar bold, uchar italic, uchar underline, uchar red, uchar green, uchar blue )
{
	const int max_length = 700;	//TX made it.
	int pos=0, end_pos, slice_count, i;
	int len = strlen( msg );
	slice_count = len / max_length + 1;	//in fact this not reliable.
	ushort msg_id = (ushort)rand();
	for( i=0; i<slice_count; i++ ){
		end_pos = get_splitable_pos( msg, pos+MIN( len-pos, max_length ) );	
		DBG("%u send (%d,%d) %d/%d", qq->number, pos, end_pos, i, slice_count );
		//msg[pos] might be 0
		prot_qun_send_msg_format_direct( qq, to, &msg[pos], end_pos-pos, msg_id, slice_count, i, fontname, fontsize, bold, italic, underline, red, green, blue );
		pos = end_pos;
	}
	return msg_id;
}

void prot_qun_send_msg_format_direct( struct qqclient* qq, uint number, char* msg_content, int len, ushort msg_id, uchar slice_count, uchar which_piece, char* fontname, int fontsize, uchar bold, uchar italic, uchar underline, uchar red, uchar green, uchar blue )
{
	qqpacket* p = packetmgr_new_send( qq, QQ_CMD_QUN_CMD );
	if( !p ) return;
	ushort len2 = strlen( msg_content );
	bytebuffer *buf = p->buf;
	put_byte( buf, 0x2A );
	put_int( buf, number );
	bytebuffer* content_buf;
	NEW( content_buf, sizeof(bytebuffer),bytebuffer );
	if( !content_buf ) {
		packetmgr_del_packet( &qq->packetmgr, p );
		return;
	}
	content_buf->size = PACKET_SIZE;
	
	put_word( content_buf, 0x0002 );	//text type
	put_byte( content_buf, slice_count );		//slice_count
	put_byte( content_buf, which_piece );		//slice_no
	put_word( content_buf, msg_id );		//id??
	put_int( content_buf, 0 );		//zeros

	put_int( content_buf, 0x4D534700 ); //"MSG"
	put_int( content_buf, 0x00000000 );
	put_int( content_buf, p->time_create );
	put_int( content_buf, rand() );
	/*
	put_int( content_buf, 0x00000000 );
	put_int( content_buf, 0x09008600 );
	*/
	// Color should actually memcpy of COLORREF
	put_byte( content_buf, 0 );
	put_byte( content_buf, blue );
	put_byte( content_buf, green );
	put_byte( content_buf, red );
	put_byte( content_buf, fontsize );
	
	uchar biu=0;
	if (bold!=0) biu|=1;
	if (italic!=0) biu|=2;
	if (underline!=0) biu|=4;
	put_byte( content_buf, biu );

	put_word( content_buf, 0x8600);
	
	put_word( content_buf, strlen(fontname) );
	put_data( content_buf, (uchar*)fontname, strlen( fontname) );
	put_word( content_buf, 0x0000 );

	/*
	// char font_name[] = "宋体";	//must be in UTF8
	char font_name[] = "\xe5\xae\x8b\xe4\xbd\x93";	//must be in UTF8
	put_word( content_buf, strlen(font_name) );
	put_data( content_buf, (uchar*)font_name, strlen( font_name) );
	put_word( content_buf, 0x0000 );
	put_byte( content_buf, 0x01 );
	put_word( content_buf, len+3 );
	put_byte( content_buf, 1 );			//unknown, keep 1
	put_word( content_buf, len );
	put_data( content_buf, (uchar*)msg_content, len );
	*/
	
	char* msg=(char*)malloc(len2+1);
	char* pMsg=msg;
	char* psz14;
	char* psz15;

	memcpy(msg,msg_content,len);
	msg[len]=0;

	while (*pMsg) {
		psz14=strchr(pMsg,0x14);
		psz15=strchr(pMsg,0x15);
		if (psz15!=NULL && psz14>psz15) 
			psz14=NULL;
		else if (psz14!=NULL && psz15>psz14)
			psz15=NULL;

		if (psz14!=pMsg && psz15!=pMsg) {
			// Marker not at start of text, write text first
			if (psz14) *psz14=0; else if (psz15) *psz15=0;

			len=strlen(pMsg);
			put_byte( content_buf, 0x01 ); // Content Type
			put_word( content_buf, len+3 ); // Size of this section
			put_byte( content_buf, 0x01 ); // Content indicator
			put_word( content_buf, len ); // Size of content
			put_data( content_buf, (uchar*)pMsg, len );
			pMsg+=strlen(pMsg);
		}

		if (psz14) {
			// Write smiley
			put_byte( content_buf, 0x02 ); // Content Type
			put_word( content_buf, 0x09 ); // Size of this section
			put_byte( content_buf, 0x01 ); // Content indicator: 2009
			put_word( content_buf, 0x01 ); // Size of content: 1 byte
			put_byte( content_buf, prot_im_get2006smiley( ( (uchar*)psz14)[1] ) ); // TODO
			put_byte( content_buf, 0xff ); // Content indicator: Legacy
			put_word( content_buf, 0x02 ); // Size of content: 2 bytes
			put_byte( content_buf, 0x14 ); // Smiley header
			put_byte( content_buf, psz14[1] ); // Smiley data
			pMsg=psz14+2;
		} else if (psz15) {
			// Write image
			char szLen[10];
			uchar cbShortcut;
			uchar cbFilename;
			memcpy(szLen,psz15+2,3);
			if (*szLen==' ') *szLen='0';
			szLen[3]=0;
			len=atoi(szLen);
			cbShortcut=psz15[6]-'A';
			cbFilename=len-49-cbShortcut-1;

			put_byte( content_buf, 0x03 ); // Content Type
			put_word( content_buf, 61+len+cbFilename-cbShortcut ); // Size of this section

			put_byte( content_buf, 0x02 ); // Content indicator: file name
			put_word( content_buf, cbFilename ); // Size of content
			put_data( content_buf, (uchar*)(psz15+49), cbFilename );

			for (int c=0; c<3; c++) {
				put_byte( content_buf, 0x04+c ); // Content indicator: 04=SessionID, 05=IP, 06=Port
				put_word( content_buf, 0x04 ); // Size of content
				memcpy(szLen,psz15+9+8*c,8);
				szLen[8]=0;
				put_int( content_buf, strtoul(szLen,NULL,16) );
			}

			put_byte( content_buf, 0x07 ); // Content indicator: Format (A=GIF C=JPG)
			put_word( content_buf, 0x01 ); // Size of content
			put_byte( content_buf, psz15[8] ); 

			put_byte( content_buf, 0x08 ); // Content indicator: File Agent Key
			put_word( content_buf, 0x10 ); // Size of content
			put_data( content_buf, (uchar*)(psz15+33), 16 );

			put_byte( content_buf, 0x09 ); // Content indicator: Unknown
			put_word( content_buf, 0x01 ); // Size of content
			put_byte( content_buf, 0x01 );

			put_byte( content_buf, 0x14 ); // Content indicator: Unknown
			put_word( content_buf, 0x04 ); // Size of content
			put_int( content_buf, 0x00 );

			put_byte( content_buf, 0xff ); // Content indicator: Legacy
			put_word( content_buf, len-cbShortcut ); // Size of content
			*psz15=0x15;
			memcpy(content_buf->data+content_buf->pos,psz15,len-cbShortcut-1);
			content_buf->data[content_buf->pos+6]='A';
			content_buf->pos+=len-cbShortcut-1;
			content_buf->data[content_buf->pos++]=psz15[len-1];
			pMsg=psz15+len;
		}
	}

	free(msg);

	put_word( buf, content_buf->pos );
	put_data( buf, content_buf->data, content_buf->pos );
	DEL( content_buf );
	
	post_packet( qq, p, SESSION_KEY );
}

void prot_qun_group_auth( struct qqclient* qq, uint qun_id, uint number, uchar result, const char* reason, const uchar* token, ushort tokenlen )
{
	qqpacket* p = packetmgr_new_send( qq, QQ_CMD_QUN_CMD );
	if( !p ) return;
	bytebuffer *buf = p->buf;

	put_byte( buf, 0x08 );	// QQ_ROOM_CMD_AUTH
	put_int( buf, qun_id );

	put_byte( buf, result );

	if ( result == QQ_QUN_AUTH_REQUEST ) {
		// Note: token and tokenlen shared, this is actually code
		put_word( buf, tokenlen );
		put_data( buf, (uchar*) token, tokenlen );
		number = 0;
	}

	put_int( buf, number );
	put_byte( buf, strlen( reason ) );
	put_data( buf, (uchar*) reason, strlen( reason ) );

	if ( result != QQ_QUN_AUTH_REQUEST ) {
		// Note: token and tokenlen shared, this is actually token
		put_word( buf, tokenlen );
		put_data( buf, (uchar*) token, tokenlen );
	}

	post_packet( qq, p, SESSION_KEY );
}

void prot_qun_join( struct qqclient* qq, uint qun_id )
{
	qqpacket* p = packetmgr_new_send( qq, QQ_CMD_QUN_CMD );
	if( !p ) return;
	bytebuffer *buf = p->buf;

	put_byte( buf, 0x19 );	// QQ_ROOM_CMD_JOIN
	put_int( buf, qun_id );

	post_packet( qq, p, SESSION_KEY );
}

void prot_qun_exit( struct qqclient* qq, uint qun_id )
{
	qqpacket* p = packetmgr_new_send( qq, QQ_CMD_QUN_CMD );
	if( !p ) return;
	bytebuffer *buf = p->buf;

	put_byte( buf, 0x09 );	// QQ_ROOM_CMD_QUIT
	put_int( buf, qun_id );

	post_packet( qq, p, SESSION_KEY );
}

} // extern "C"
