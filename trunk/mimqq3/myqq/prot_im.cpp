/*
 *  prot_im.c
 *
 *  QQ Protocol. Part Internet Message
 *
 *  Copyright (C) 2008  Huang Guan
 *
 *  2008-7-12 Created.
 *
 *  Description: This file mainly includes the functions about 
 *  sending/receiving buddy or qun messages
 *
 */
#include <StdAfx.h>
#define ADV_MSG

extern "C" {
#include "qqclient.h"
#include "memory.h"
#include "debug.h"
#include "qqpacket.h"
#include "packetmgr.h"
#include "qqcrypt.h"
#include "buddy.h"
#include "protocol.h"
#include "utf8.h"
#include "util.h"

// we divide the message into pieces if it's too long
void prot_im_send_msg( struct qqclient* qq, uint to, char* msg )
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
		prot_im_send_msg_ex( qq, to, &msg[pos], end_pos-pos, msg_id, slice_count, i );
		pos = end_pos;
	}
}

void prot_im_send_msg_ex( struct qqclient* qq, uint to, char* msg, int len,
	ushort msg_id, uchar slice_count, uchar which_piece )
{
//	DBG("str: %s  len: %d", msg, len );
	qqpacket* p;
	if( !len ) return;
	p = packetmgr_new_send( qq, QQ_CMD_SEND_IM );
	if( !p ) return;
	bytebuffer *buf = p->buf;
	put_int( buf, qq->number );
	put_int( buf, to );
	//00 00 00 08 00 01 00 04 00 00 00 00  09SP1 changes
	put_int( buf, 0x00000008 );
	put_int( buf, 0x00010004 );
	put_int( buf, 0x00000000 );
	put_word( buf, qq->version );
	put_int( buf, qq->number );
	put_int( buf, to );
	put_data( buf, qq->data.im_key, 16 );
	put_word( buf, QQ_NORMAL_IM_TEXT );	//message type
	put_word( buf, p->seqno );
	put_int( buf, p->time_create );
	put_word( buf, qq->self->face );	//my face
	put_int( buf, 1 );	//has font attribute
	put_byte( buf, slice_count );	//slice_count
	put_byte( buf, which_piece );	//slice_no
	put_word( buf, msg_id );	//msg_id??
	put_byte( buf, QQ_IM_TEXT );	//auto_reply
	put_int( buf, 0x4D534700 ); //"MSG"
	put_int( buf, 0x00000000 );
	put_int( buf, p->time_create );
	put_int( buf, (msg_id<<16)|msg_id );	//maybe a random interger
	put_int( buf, 0x00000000 );
	put_int( buf, 0x09008600 );
	char font_name[] = "\xe5\xae\x8b\xe4\xbd\x93"; //"宋体";	//must be UTF8
	put_word( buf, strlen(font_name) );
	put_data( buf, (uchar*)font_name, strlen( font_name) );
	put_word( buf, 0x0000 );
	put_byte( buf, 0x01 );
	put_word( buf, len+3 );
	put_byte( buf, 1 );
	put_word( buf, len );
//	put_word( buf, p->seqno );
	put_data( buf, (uchar*)msg, len );
	post_packet( qq, p, SESSION_KEY );
}

void prot_im_ack_recv( struct qqclient* qq, qqpacket* pre )
{
	qqpacket* p = packetmgr_new_send( qq, pre->command );
	if( !p ) return;
	bytebuffer *buf = p->buf;
	p->seqno = pre->seqno;
	put_data( buf, pre->buf->data, 16 );
	p->need_ack = 0;
	post_packet( qq, p, SESSION_KEY );
}

static void parse_message_09( qqpacket* p, qqmessage* msg, char* tmp, int outlen )
{
	bytebuffer *buf = p->buf;
	int i = 0;
	ushort len;
	buf->pos += 8; //'M' 'S' 'G' 00 00 00 00 00
	msg->msg_time = get_int( buf );	//send time
	buf->pos += 12;
	len = get_word( buf );	//pass font name
	buf->pos += len;
	get_word( buf );	//00 00 2009-2-7 Huang Guan, updated
	while( buf->pos < buf->len ){
		uchar ch;
		uchar type = get_byte(buf);
		len = get_word(buf);
		get_byte(buf);	//is 01 if text or face, 02 if image
		ushort len_str;
		switch( type ){
		case 01:	//pure text
			len_str = get_word( buf );
			len_str = MIN(len_str, outlen-i);
			get_data( buf, (uchar*)&tmp[i], len_str );
			i += len_str;
			break;
		case 02:	//face
			len_str = get_word( buf );
			buf->pos += len_str;	//
			get_byte( buf );	//FF
			len_str = get_word( buf );
			if( len_str == 2 ){
				get_byte( buf );
				if( outlen-i > 15 )
					i += sprintf( &tmp[i], "[face:%d]", get_byte( buf ) );
			}else{ //unknown situation
				buf->pos += len_str;
			}
			break;
		case 03:	//image
			len_str = get_word( buf );
			buf->pos += len_str;	//
			do{
				ch = get_byte( buf );	//ff
				if (ch==0xff) break;
				len = get_word(buf);
				buf->pos += len;
			}while(/* ch!=0xff && */buf->pos<buf->len );

			if (ch==0xff) {
				token tok_pic;
				// buf->pos-=2;
				get_token( buf, &tok_pic );
				if( outlen-i > 10 ) {
					// i += sprintf( &tmp[i], "[image]" );
					memcpy(tmp+i,tok_pic.data,tok_pic.len);
					i+=tok_pic.len;
					tmp[i]=0;
				}
			}
		}
		len = 0;	//use it, or the compiler would bark.
	}
	tmp[i] = 0;
}

static void process_buddy_im_text_adv( struct qqclient* qq, qqpacket* p, qqmessage* msg, char* tmp )
{
	bytebuffer *buf = p->buf;

	buf->pos += 3;
    uchar hasFontAttribute = get_byte( buf );
	//分片
	msg->slice_count = get_byte( buf );
	msg->slice_no = get_byte( buf );
	msg->msg_id = get_word( buf );
	msg->auto_reply = get_byte( buf );

	uchar red=0, green=0, blue=0, fontSize=0, format=0;
	char szFont[32]={0};
	ushort cb;

	//if (!memcmp(buf->data+buf->pos,"MSG",4)) {
	switch( msg->im_type ){
	case QQ_RECV_IM_BUDDY_09:
	case QQ_RECV_IM_BUDDY_09SP1:
	{
		unsigned char type;
		std::string msg2;
		
		buf->pos+=8; //'M' 'S' 'G' 00 00 00 00 00
		// /*sendTime=*/ntohl(*(int*)(buf+pos));
		buf->pos+=4; 

		buf->pos+=5; // 15 46 88 03 00 >27 05 01 09 00 86 22

		hasFontAttribute=true;
		blue=get_byte( buf );
		green=get_byte( buf );
		red=get_byte( buf );
		fontSize=get_byte( buf );
		format=get_byte( buf );
		/* format=next 4 lines
		bold=buf[pos] & 0x1;
		italic=buf[pos] & 0x2;
		underline=buf[pos] & 0x4;
		pos++;
		*/

		buf->pos+=2; // Unknown

		cb = get_word( buf );
		get_data( buf, (uchar*)szFont, cb );
		szFont[cb]=0;
		// gb_to_utf8(szFont,szFont,32);

		buf->pos+=2;	//00 00 2009-2-7 Huang Guan, updated

		while (buf->pos<buf->len) {
			type = get_byte( buf );
			cb = get_word( buf );

			while (buf->pos<buf->len) {
				if (type!=0x01 && buf->data[buf->pos]==0xff || (type==0x01 && buf->data[buf->pos]==0x01))
					break;
				else {
					buf->pos++;
					buf->pos+=get_word( buf );
				}
			}
			buf->pos++;
			cb=get_word(buf);

			switch (type) {
				case 0x01: //pure text
				case 0x03: //image
					get_data( buf, (uchar*)tmp, cb);
					if (type!=0x03) {
						char* ppszTemp=tmp;
						while (ppszTemp=strchr(ppszTemp,'\r')) *ppszTemp++='\n';
					}
					tmp[cb]=0;
					msg2.append(tmp);
					break;
				case 0x02: //face
					if (cb==2) {
						// First byte is the usual 0x14 as old QQ
						sprintf(tmp,"[face:%d]",buf->data[buf->pos+1]);
						msg2.append(tmp);
						buf->pos+=cb;
					} else {
						util_log(0,"ReceivedQunIM::parseData(09) face assert!");
						DebugBreak();
					}
					break;
			}
			//buf->pos+=cb;
		}

		if (type==0x03) msg2.append(" "); // This line is required for ReceiveIMPacket:convertToShow()

		strcpy(msg->msg_content,msg2.c_str());
		break;
	}
	case QQ_RECV_IM_BUDDY_0802:
		buf->pos += 8;
	case QQ_RECV_IM_BUDDY_0801:
	{
		get_string( buf, tmp, MSG_CONTENT_LEN );
		gb_to_utf8( tmp, tmp, MSG_CONTENT_LEN-1 );
		trans_faces( tmp, msg->msg_content, MSG_CONTENT_LEN );

		if(hasFontAttribute) {
			if(buf->pos<buf->len) {
				cb=buf->len-buf->pos;
				get_data( buf, (uchar*)tmp, cb );
				fontSize = tmp[0] & 0x1F;
				format = 0;
				if((tmp[0] & 0x20) != 0) format |= 1;
				if((tmp[0] & 0x40) != 0) format |= 2;
				if((tmp[0] & 0x80) != 0) format |= 4;
				red = tmp[1] & 0xFF;
				green = tmp[2] & 0xFF;
				blue = tmp[3] & 0xFF;
				/*
				short tmp2;
				memcpy(&tmp2, fa+5, 2);
				encoding = ntohs(tmp2);
				*/
				cb = cb-7-1;
				memcpy(szFont, tmp+7, cb);
				szFont[cb]=0x00;
				gb_to_utf8(szFont,szFont,32);
			} else
				hasFontAttribute = false;
		}
	}
	}

	sprintf(tmp,"buddyimtextadv^$%u^$%d^$%d^$%d^$%d^$%d^$%02x^$%02x^$%02x^$%d^$%d^$%s^$%s",msg->from,msg->msg_id,msg->slice_count,msg->slice_no,(unsigned char)msg->auto_reply,hasFontAttribute,red,green,blue,fontSize,format,szFont,msg->msg_content);
	qqclient_put_event( qq, tmp );
}

static void process_buddy_im_text( struct qqclient* qq, qqpacket* p, qqmessage* msg )
{
	bytebuffer *buf = p->buf;
	char tmp[MSG_CONTENT_LEN];
	get_word( buf );	//session id
	msg->msg_time = get_int( buf );
	get_word( buf );	//face

#ifdef ADV_MSG
	process_buddy_im_text_adv( qq, p, msg, tmp );
#else
	buf->pos += 4; 	//0000001
	//分片
	msg->slice_count = get_byte( buf );
	msg->slice_no = get_byte( buf );
	msg->msg_id = get_word( buf );
	msg->auto_reply = get_byte( buf );

	switch( msg->im_type ){
	case QQ_RECV_IM_BUDDY_09:
	case QQ_RECV_IM_BUDDY_09SP1:
		parse_message_09( p, msg, tmp, MSG_CONTENT_LEN );
		strcpy( msg->msg_content, tmp );
		break;
	case QQ_RECV_IM_BUDDY_0801:
		get_string( buf, tmp, MSG_CONTENT_LEN );
		gb_to_utf8( tmp, tmp, MSG_CONTENT_LEN-1 );
		trans_faces( tmp, msg->msg_content, MSG_CONTENT_LEN );
		break;
	case QQ_RECV_IM_BUDDY_0802:
		buf->pos += 8;
		get_string( buf, tmp, MSG_CONTENT_LEN );
		gb_to_utf8( tmp, tmp, MSG_CONTENT_LEN-1 );
		trans_faces( tmp, msg->msg_content, MSG_CONTENT_LEN );
		break;
	}
//	DBG("buddy msg from %u:", msg->from );
	if( qq->auto_reply[0]!='\0' ){ //
		prot_im_send_msg( qq, msg->from, qq->auto_reply );
	}
	buddy_msg_callback( qq, msg->from, msg->msg_time, msg->msg_content );
#endif
}

static void process_buddy_im( struct qqclient* qq, qqpacket* p, qqmessage* msg )
{
	bytebuffer *buf = p->buf;
	get_word( buf );	//version
	msg->from = get_int( buf );
	if( get_int( buf ) != qq->number ){
		DBG("nothing but this is impossible!!");
		return;
	}
	//to check if this buddy is in our list, or we add it.
	buddy_get( qq, msg->from, 1 );
	//IM key
	uchar key[16];
	get_data( buf, key, 16 );
	ushort content_type = get_word( buf );
	switch( content_type ){
	case QQ_NORMAL_IM_TEXT:
	//	DBG("QQ_NORMAL_IM_TEXT");
		process_buddy_im_text( qq, p, msg );
		break;
	case QQ_NORMAL_IM_FILE_REQUEST_TCP:
		DBG("QQ_NORMAL_IM_FILE_REQUEST_TCP");
		break;
	case QQ_NORMAL_IM_FILE_APPROVE_TCP:
		DBG("QQ_NORMAL_IM_FILE_APPROVE_TCP");
		break;
	case QQ_NORMAL_IM_FILE_REJECT_TCP:
		DBG("QQ_NORMAL_IM_FILE_REJECT_TCP");
		break;
	case QQ_NORMAL_IM_FILE_REQUEST_UDP:
		DBG("QQ_NORMAL_IM_FILE_REQUEST_UDP");
		break;
	case QQ_NORMAL_IM_FILE_APPROVE_UDP:
		DBG("QQ_NORMAL_IM_FILE_APPROVE_UDP");
		break;
	case QQ_NORMAL_IM_FILE_REJECT_UDP:
		DBG("QQ_NORMAL_IM_FILE_REJECT_UDP");
		break;
	case QQ_NORMAL_IM_FILE_NOTIFY:
		DBG("QQ_NORMAL_IM_FILE_NOTIFY");
		break;
	case QQ_NORMAL_IM_FILE_PASV:
		DBG("QQ_NORMAL_IM_FILE_PASV");
		break;
	case QQ_NORMAL_IM_FILE_CANCEL:
		DBG("QQ_NORMAL_IM_FILE_CANCEL");
		break;
	case QQ_NORMAL_IM_FILE_EX_REQUEST_UDP:
//		DBG("QQ_NORMAL_IM_FILE_EX_REQUEST_UDP");
		break;
	case QQ_NORMAL_IM_FILE_EX_REQUEST_ACCEPT:
		DBG("QQ_NORMAL_IM_FILE_EX_REQUEST_ACCEPT");
		break;
	case QQ_NORMAL_IM_FILE_EX_REQUEST_CANCEL:
		DBG("QQ_NORMAL_IM_FILE_EX_REQUEST_CANCEL");
		break;
	case QQ_NORMAL_IM_FILE_EX_NOTIFY_IP:
		DBG("QQ_NORMAL_IM_FILE_EX_NOTIFY_IP");
		break;
	default:
		DBG("UNKNOWN type: %x", content_type );
		break;
	}
}

static void process_sys_im( struct qqclient* qq, qqpacket* p, qqmessage* msg )
{
	bytebuffer *buf = p->buf;
	msg->msg_time = time(NULL);
	uchar content_type;
	content_type = get_byte( buf );
	uchar len = get_byte( buf );
	get_data( buf, (uchar*)msg->msg_content, len );
	msg->msg_content[len] = 0;
	buddy_msg_callback( qq, msg->from, msg->msg_time, msg->msg_content );
	// if( strstr( msg->msg_content, "另一地点登录" ) != NULL ){
	if( strstr( msg->msg_content, "\xe5\x8f\xa6\xe4\xb8\x80\xe5\x9c\xb0\xe7\x82\xb9\xe7\x99\xbb\xe5\xbd\x95" ) != NULL ){
		qqclient_set_process( qq, P_BUSY );
	}else{
		qqclient_set_process( qq, P_ERROR );
	}
	DBG("sysim(type:%x): %s", content_type, msg->msg_content );
}

static void process_qun_im_adv( struct qqclient* qq, qqpacket* p, qqmessage* msg )
{
	// msg->from, msg_id, msg_time, qunversion, sentTime, slice_count, slice_number
	bytebuffer *buf = p->buf;
	char tmp[MSG_CONTENT_LEN];
	get_int( buf );		//00 00 00 00  09SP1
	get_int( buf );		//ext_number
	get_byte( buf );	//normal qun or temp qun?

	if(msg->im_type == QQ_RECV_IM_TEMP_QUN_IM){ // internal ID of temporary Qun
		get_int( buf ); // qunId
	}

	msg->from = get_int( buf );
	/*if( msg->from == qq->number ) // MIMQQ needs it
		return;*/ 
	get_word( buf );	//zero
	msg->msg_id = get_word( buf );
	msg->msg_time = get_int( buf );

	uint qun_version = get_int( buf );   // version ID is to tell if something like memebers, or rules of this Qun were changed
	ushort msg_id2=0;
	uint sent_time;
	uchar has_font_attr=0;
	uchar blue, green, red, font_size, format;
	char font_name[32];

	*font_name = 0;

	buf->pos += 2; // ignore 2 bytes;  Luma QQ says it's the length for the following data.

	// 10 unknown bytes
	if( msg->im_type != QQ_RECV_IM_UNKNOWN_QUN_IM) {
		buf->pos+=2;
		msg->slice_count = get_byte( buf );
		msg->slice_no = get_byte( buf );
		msg_id2 = get_word( buf ); // This is msg id in eva, msg->msg_id is seq
		buf->pos+=4;
	}

	if (msg->im_type==QQ_RECV_IM_QUN_IM_09) {
		unsigned short sz;
		unsigned char type;
		std::string msg;
		
		DBG("msg->im_type==QQ_RECV_IM_QUN_IM_09");

		buf->pos+=8; //'M' 'S' 'G' 00 00 00 00 00
		sent_time = get_int( buf );

		buf->pos += 5; // 15 46 88 03 00 >27 05 01 09 00 86 22

		has_font_attr = 1;

		blue = get_byte( buf );
		green = get_byte( buf );
		red = get_byte( buf );
		font_size = get_byte( buf );
		format = get_byte( buf );

		buf->pos += 2; // Unknown

		sz = get_word( buf ); //pass font name
		get_data( buf, (uchar*)font_name, sz );
		font_name[sz] = 0;

		buf->pos += 2;	//00 00 2009-2-7 Huang Guan, updated

		DBG("Before loop");

		while ( buf->pos<buf->len ) {
			type = get_byte( buf );
			sz = get_word( buf );

			DBG("pos=%d type=%d sz=%d",buf->pos,type,sz);

			while ( buf->pos<buf->len) {
				if (type!=0x01 && buf->data[buf->pos]==0xff || (type==0x01 && buf->data[buf->pos]==0x01))
					break;
				else {
					buf->pos++;
					buf->pos += get_word( buf );
				}
			}

			buf->pos++;
			sz = get_word( buf );

			DBG("pos=%d sz=%d",buf->pos,sz);

			switch (type) {
				case 0x01: //pure text
				case 0x03: //image
					get_data( buf, (uchar*)tmp, sz );
					tmp[sz] = 0;

					if (type!=0x03) {
						char* ppszTemp=tmp;
						while (ppszTemp=strchr(ppszTemp,'\r')) *ppszTemp++='\n';
					}

					DBG("tmp=%s",tmp);
					msg.append( tmp );
					break;
				case 0x02: //face
					if (sz == 2) {
						// First byte is the usual 0x14 as old QQ
						sprintf( tmp,"[face:%d]", buf->data[buf->pos+1] );
						msg.append( tmp );
					} else {
						util_log(0,"ReceivedQunIM::parseData(09) face assert!");
						DebugBreak();
					}
					buf->pos += sz;
					break;
			}
			DBG("end of one loop pos=%d",buf->pos);
		}

		if (type==0x03) msg.append(" "); // This line is required for ReceiveIMPacket:convertToShow()

		strcpy( tmp, msg.c_str() );
		// this->message.assign(ReceiveIMPacket::convertToShow(pszTemp, QQ_IM_NORMAL_REPLY)); // TODO: converttoshow should be called in client now
	} else {
		int i=0;

		DBG("msg->im_type!=QQ_RECV_IM_QUN_IM_09");

		//       not scan \0 but to extract the length from the packet
		if (msg->slice_count != (msg->slice_no + 1))
		{
			i = buf->len - buf->pos - 1;
			buf->pos = buf->len;
		} else {
			i = buf->data[buf->len-1];
			i = buf->len - i - buf->pos;
			buf->pos = buf->pos + i + 1;
		}

		DBG("pos=%d",buf->pos);

		memcpy( tmp, buf->data+(buf->pos-i-1), i+1);
		tmp[i+1]= 0x00;

		DBG("tmp=%s",tmp);

		char* ppszTemp = tmp;
		while (ppszTemp=strchr(ppszTemp,'\r')) *ppszTemp++='\n';

		//message = ReceiveIMPacket::convertToShow(str, QQ_IM_NORMAL_REPLY); TODO: converttoshow to be called in client
		DBG("Before gb_to_utf8");
		gb_to_utf8( tmp, tmp, MSG_CONTENT_LEN );
		
		has_font_attr = (buf->len > buf->pos)?1:0;
		format=0;
		DBG("has_font_attr=%d",has_font_attr);
		if( has_font_attr!=0 ) {
			font_size = buf->data[buf->pos] & 0x1F;
			format |= ((buf->data[buf->pos] & 0x20) != 0)?1:0;
			format |= ((buf->data[buf->pos] & 0x40) != 0)?2:0;
			format |= ((buf->data[buf->pos] & 0x80) != 0)?4:0;
			buf->pos++;
			red = get_byte( buf );
			green = get_byte( buf );
			blue = get_byte( buf );
			buf->pos++;
			get_word( buf ); // Encoding

			font_name[get_data( buf, (uchar*)font_name, buf->len-buf->pos-1 )] = 0;
			DBG("font_name=%s",font_name);
		} 
	} // source

	DBG("end if");

	strcpy( msg->msg_content, tmp );

	sprintf(tmp,"qunimadv^$%u^$%u^$%d^$%d^$%d^$%u^$%u^$%d^$%02x^$%02x^$%02x^$%d^$%d^$%s^$%s",msg->qun_number,msg->from,msg->msg_id,msg->slice_count,msg->slice_no,qun_version,sent_time,has_font_attr,red,green,blue,font_size,format,font_name,msg->msg_content);
	qqclient_put_event( qq, tmp );
}

static void process_qun_im( struct qqclient* qq, qqpacket* p, qqmessage* msg )
{
#ifdef ADV_MSG
	process_qun_im_adv( qq, p, msg );
#else
	bytebuffer *buf = p->buf;
	char tmp[MSG_CONTENT_LEN];
	get_int( buf );		//00 00 00 00  09SP1
	get_int( buf );		//ext_number
	get_byte( buf );	//normal qun or temp qun?
	msg->from = get_int( buf );
	if( msg->from == qq->number )
		return;
	get_word( buf );	//zero
	msg->msg_id = get_word( buf );
	msg->msg_time = get_int( buf );
	switch( msg->im_type ){
	case QQ_RECV_IM_QUN_IM_09:
		buf->pos += 16;
		parse_message_09( p, msg, tmp, MSG_CONTENT_LEN );
		strcpy( msg->msg_content, tmp );
		break;
	case QQ_RECV_IM_QUN_IM:
		buf->pos += 16;
		get_string( buf, tmp, MSG_CONTENT_LEN );
		gb_to_utf8( tmp, tmp, MSG_CONTENT_LEN-1 );
		trans_faces( tmp, msg->msg_content, MSG_CONTENT_LEN );
		break;
	}
#endif
	
//	DBG("process_qun_im(number:%u): ", msg->from );
//	puts( msg->msg_content );
	qun_msg_callback( qq, msg->from, msg->qun_number, msg->msg_time, msg->msg_content );
}

static void process_qun_member_im( struct qqclient* qq, qqpacket* p, qqmessage* msg )
{
	bytebuffer *buf = p->buf;
	token tok_unknown;
	buf->pos += 14;	//00 65 01 02 00 00 00 00 00 00 00 00 00 00
	get_token( buf, &tok_unknown );	//len: 0x30
	buf->pos += 8;	//00 26 00 1c 02 00 01 00
	get_int( buf );	//unknown time 48 86 b9 e5
	get_int( buf );	//00 01 51 80
	msg->from = get_int( buf ); //from 10 4a 61 e3
	get_int( buf );	//self number
	get_int( buf );	//00 00 00 1f
	msg->msg_time = get_int( buf );	//48 86 c4 f1
	msg->qun_number = get_int( buf );	//1c aa 5d 98 
	get_int( buf );		//ext_number
	buf->pos += 15;	//00 0d 00 00 00 00 00 00 00 00 00 00 00 00 00 
	process_buddy_im( qq, p, msg );
	DBG("process_qun_member_im: qun_number: %u", msg->qun_number );
}

static void process_news( struct qqclient* qq, qqpacket* p )
{
	bytebuffer *buf = p->buf;
	char event[1024]="news^$";
	char* pszEvent=event+6;

	buf->pos += 4; // ignore unknown 4 bytes

	uchar slen = get_byte( buf );
	pszEvent[ get_data( buf, (uchar*)pszEvent, slen ) ]=0; // Title
	gb_to_utf8( pszEvent, pszEvent, 128 );

	pszEvent+=slen;
	strcpy(pszEvent,"^$");
	pszEvent+=2;

	slen = get_byte( buf );
	pszEvent[ get_data( buf, (uchar*)pszEvent, slen ) ]=0; // Brief
	gb_to_utf8( pszEvent, pszEvent, 128 );

	pszEvent+=slen;
	strcpy(pszEvent,"^$");
	pszEvent+=2;

	slen = get_byte( buf );
	pszEvent[ get_data( buf, (uchar*)pszEvent, slen ) ]=0; // Url
	gb_to_utf8( pszEvent, pszEvent, 128 );

	// still 5 unknown 0x00s

	qqclient_put_event( qq, event );
}

static void process_qun_join( struct qqclient* qq, qqpacket* p, qqmessage* msg )
{
	bytebuffer *buf = p->buf;
	char event[1024];

	buf->pos+=4; // Changed in QQ2009
	uint externalID = get_int( buf );
	uint subject = 0;
	uint commander = 0;
	
	uchar type = get_byte( buf );
	uchar exttype = 0;

	switch( msg->im_type ){
		case QQ_RECV_IM_ADDED_TO_QUN:
		case QQ_RECV_IM_DELETED_FROM_QUN:
			subject = get_int( buf );
			exttype = get_byte( buf );
			
			// LumaQQ disregard value of exttype(rootCause) when deal with QQ_RECV_IM_ADDED_TO_QUN
			// Values for exttype (For QQ_RECV_IM_DELETED_FROM_QUN, from LumaQQ Mac)
			// static const char kQQExitClusterDismissed = 0x01;
			// static const char kQQExitClusterActive = 0x02;
			// static const char kQQExitClusterPassive = 0x03;
			if( msg->im_type == QQ_RECV_IM_ADDED_TO_QUN || exttype == 3 ) {
				commander = get_int( buf );
			}
			break;
		case QQ_RECV_IM_APPLY_ADD_TO_QUN:
			subject = get_int( buf );
			break;
		case QQ_RECV_IM_APPROVE_JOIN_QUN:
		case QQ_RECV_IM_REJECT_JOIN_QUN:
			commander = get_int( buf );
			break;
		case QQ_RECV_IM_SET_QUN_ADMIN:
			exttype = get_byte( buf ); // commander = buf[pos++]; // just use the commander variable to save the action. 0:unset, 1:set
			subject = get_int( buf ); // sender is the persion which the action performed on
			buf->pos++;// unknonw, probably the admin value of the above member
			break;
		case QQ_RECV_IM_CREATE_QUN:
		default:
			subject = get_int( buf );
			break;
	}

	LPSTR pszEvent=event;
	pszEvent+=sprintf( event, "qunjoin^$%u^$%d^$%u^$%u^$%u^$%d^$%d^$",msg->qun_number,msg->im_type,externalID,subject,commander,type,exttype );

	if( msg->im_type != QQ_RECV_IM_DELETED_FROM_QUN || exttype == 3){
		if( buf->len > buf->pos ) {
			pszEvent[ get_data( buf, (uchar*)pszEvent, get_byte( buf ) ) ] = 0; // msg
			gb_to_utf8( pszEvent,pszEvent,16 ); // Should be 10 bytes only
			pszEvent+=strlen(pszEvent);
		}
	}

	strcpy( pszEvent, "^$" );
	pszEvent += 2;

	if ( buf->len > buf->pos ) {
		pszEvent += sprintf( pszEvent, "%d^$", subject = get_word( buf ) );
		pszEvent += get_data( buf, (uchar*)pszEvent, subject );
		strcpy( pszEvent, "^$" );
		pszEvent += 2;

		if( msg->im_type == QQ_RECV_IM_REQUEST_JOIN_QUN){
			pszEvent += sprintf( pszEvent, "%d^$", subject = get_word( buf ) );
			pszEvent += get_data( buf, (uchar*)pszEvent, subject );
		} else {
			strcpy( pszEvent, "0^$" );
			pszEvent += 2;
		}
	} else {
		strcpy( pszEvent, "0^$^$0^$" );
		pszEvent += 3;
	}

	*pszEvent = 0;

	qqclient_put_event( qq, event );
}

static void process_mail( struct qqclient* qq, qqpacket* p, qqmessage* msg )
{
	bytebuffer *buf = p->buf;

	buf->pos += 4; // QQ2010

	if ( get_byte( buf )==0x03 ) {
		char event[1024] = "mail^$";
		char* pszEvent = event + 6;
		uchar len;

		pszEvent += get_data( buf, (uchar*)pszEvent, 30 ); // Mail ID
		strcpy( pszEvent, "^$" );
		pszEvent += 2;

		pszEvent += get_data( buf, (uchar*)pszEvent, get_byte( buf ) ); // Sender
		strcpy( pszEvent, "^$" );
		pszEvent += 2;

		buf->pos += 4; // 4e 80 d9 85
		buf->pos += 4; // 00 00 03 d8
		buf->pos ++; // 00

		pszEvent[get_data( buf, (uchar*)pszEvent, get_byte( buf ) )] = 0; // Title
		gb_to_utf8(pszEvent,pszEvent,strlen(pszEvent)+128);

		qqclient_put_event( qq, event );
	}
}

static void process_signature_change( struct qqclient* qq, qqpacket* p, qqmessage* msg )
{
	bytebuffer *buf = p->buf;
	char event[1024]="buddysignature^$";
	char* pszEvent=event+16;

	buf->pos += 4; // QQ2009 has 4 zero-padding bytes
	pszEvent += sprintf( pszEvent, "%u^$", get_int( buf ) ); // your buddy qq number
	
	pszEvent += sprintf( pszEvent, "%u^$", get_int( buf ) ); // the time he/she changed his/her signature
	pszEvent[ get_data( buf, (uchar*)pszEvent, get_byte( buf ) ) ]=0; // Signature
	
	qqclient_put_event( qq, event );
}

static void process_buddy_writing( struct qqclient* qq, qqpacket* p, qqmessage* msg )
{
	bytebuffer *buf = p->buf;
	char event[32];
	buf->pos += 4;
	sprintf( event, "buddywriting^$%u", get_int( buf ) );

	qqclient_put_event( qq, event );
}

void prot_im_recv_msg( struct qqclient* qq, qqpacket* p )
{
	bytebuffer *buf = p->buf;
	uint from, from_ip;
	ushort from_port;
	ushort im_type;
	int len;
	qqmessage *msg;
	static ushort cache[50]={0};
	static int cacheIndex=0;
/* 091027 This code maybe good for a client that prints the message as soon as it gets one.
	if( qq->login_finish!=1 ){	//not finished login
		DBG("Early message ... Abandoned.");
		return;
	}
*/
	NEW( msg, sizeof( qqmessage ),qqmessage );
	if( !msg )
		return;
	//
	for (int c=0; c<50; c++) {
		if (cache[c]!=0 && cache[c]==p->seqno) {
			DBG( "Duplicated message, ignore" );
			prot_im_ack_recv( qq, p );
			return;
		}
	}

	cache[cacheIndex]=p->seqno;
	cacheIndex=(cacheIndex+1)%50;

	from = get_int( buf );
	if( get_int( buf ) != qq->number ){
		DBG("qq->number is not equal to yours");
	}
	get_int( buf );	//sequence
	from_ip = get_int( buf );
	from_port = get_word( buf );
	im_type = get_word( buf );
	msg->im_type = im_type;
#ifdef NO_IM
	if( im_type == QQ_RECV_IM_SYS_NOTIFICATION ){
#endif
	switch( im_type ){
	case QQ_RECV_IM_BUDDY_0801:
		DBG("QQ_RECV_IM_BUDDY_0801");
		msg->msg_type = MT_BUDDY;
		//fixed for qq2007, webqq. 20090621 17:57
		buf->pos += 2;
		len = get_word( buf ); 
		buf->pos += len;
		process_buddy_im( qq, p, msg );
		break;
	case QQ_RECV_IM_BUDDY_0802:
		DBG("QQ_RECV_IM_BUDDY_0802");
		msg->msg_type = MT_BUDDY;
		process_buddy_im( qq, p, msg );
		break;
	case QQ_RECV_IM_BUDDY_09:
		DBG("QQ_RECV_IM_BUDDY_09");
		msg->msg_type = MT_BUDDY;
		buf->pos += 11;
		process_buddy_im( qq, p, msg );
		break;
	case QQ_RECV_IM_BUDDY_09SP1:
		DBG("QQ_RECV_IM_BUDDY_09SP1");
		msg->msg_type = MT_BUDDY;
		buf->pos += 2;
		len = get_word( buf ); 
		buf->pos += len;
		process_buddy_im( qq, p, msg );
		break;
	case QQ_RECV_IM_SOMEBODY:
		DBG("QQ_RECV_IM_SOMEBODY");
		break;
	case QQ_RECV_IM_WRITING:
		process_buddy_writing( qq, p, msg );
		break;
	case QQ_RECV_IM_QUN_IM:
	case QQ_RECV_IM_QUN_IM_09:
		msg->msg_type = MT_QUN;
		msg->qun_number = from;
		process_qun_im( qq, p, msg );
		break;
	case QQ_RECV_IM_TO_UNKNOWN:
		DBG("QQ_RECV_IM_TO_UNKNOWN");
		break;
	case QQ_RECV_IM_NEWS:
		process_news( qq, p );
		break;
	case QQ_RECV_IM_UNKNOWN_QUN_IM:
		DBG("QQ_RECV_IM_UNKNOWN_QUN_IM");
		break;
	case QQ_RECV_IM_ADD_TO_QUN:
	case QQ_RECV_IM_DEL_FROM_QUN:
	case QQ_RECV_IM_APPLY_ADD_TO_QUN:
	case QQ_RECV_IM_APPROVE_APPLY_ADD_TO_QUN:
	case QQ_RECV_IM_REJCT_APPLY_ADD_TO_QUN:
	case QQ_RECV_IM_CREATE_QUN:
		msg->qun_number = from;
		process_qun_join( qq, p, msg );
		break;
	case QQ_RECV_IM_TEMP_QUN_IM:
		DBG("QQ_RECV_IM_TEMP_QUN_IM");
	case QQ_RECV_IM_SYS_NOTIFICATION:
		msg->msg_type = MT_SYSTEM;
		get_int( buf );	//09 SP1 fixed
		process_sys_im( qq, p, msg );
		break;
	case QQ_RECV_IM_QUN_MEMBER_IM:
		msg->msg_type = MT_QUN_MEMBER;
		process_qun_member_im( qq, p, msg );
		break;
	case 0x0012:
		process_mail( qq, p, msg );
		break;
	case 0x1f: // Temp session
		break;
	case 0x0041:
		process_signature_change( qq, p, msg );
		break;
	case 0x1e: // Signature acknowledge
		break;
	case 0x2f: // QunCard update
		DBG( "Qun Card updated for %u", from );
		prot_qun_get_membername( qq, from, 0 );
		break;
	default:
		DBG("Unknown message type : %x from %u", im_type, from );
	}
#ifdef NO_IM
	}
#endif
	//ack recv
	prot_im_ack_recv( qq, p );
	DEL( msg );
}

void prot_im_send_msg_reply( struct qqclient* qq, qqpacket* p ) {
	bytebuffer *buf = p->buf;
	char event[64];

	uchar ret = get_byte( buf );
	sprintf( event, "imsendmsg^$%u^$%d", p->seqno, ret );
	qqclient_put_event( qq, event );
}

ushort prot_im_send_msg_format( struct qqclient* qq, uint to, char* msg, char* fontname, int fontsize, uchar bold, uchar italic, uchar underline, uchar red, uchar green, uchar blue )
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
		prot_im_send_msg_ex_format( qq, to, &msg[pos], end_pos-pos, msg_id, slice_count, i, fontname, fontsize, bold, italic, underline, red, green, blue );
		pos = end_pos;
	}
	return msg_id;
}

uchar prot_im_get2006smiley(uchar smiley) {
	static uchar smileymap[256]={
		0x87, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
		0xff, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 
		0x50, 0x51, 0x52, 0x53, 0x54, 0x2d, 0x55, 0x2c, 0x2b, 0x28, 0x29, 0x56, 0x2a, 0x57, 0x39, 0x58, 
		0x34, 0x59, 0x3f, 0x40, 0x41, 0x42, 0x43, 0x35, 0x45, 0x5a, 0x5b, 0x4a, 0x5c, 0x5d, 0x4b, 0x4c, 
		0x4d, 0x5e, 0x5f, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x1a, 0x1b, 0x1c, 0x1d, 0x2e, 0x2f, 0x30, 0x31, 
		0x3c, 0x3d, 0x3e, 0x44, 0x46, 0x47, 0x48, 0x49, 0x4e, 0x3b, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 
		0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x32, 0x33, 0x36, 0x37, 0x38, 0x3a, 
		0x4f, 0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 
		0x6f, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 
		0x7f, 0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
	};

	return smileymap[smiley];
}

void prot_im_send_msg_ex_format( struct qqclient* qq, uint to, char* msg2, int len, ushort msg_id, uchar slice_count, uchar which_piece, char* fontname, int fontsize, uchar bold, uchar italic, uchar underline, uchar red, uchar green, uchar blue  )
{
//	DBG("str: %s  len: %d", msg, len );
	qqpacket* p;
	if( !len ) return;
	p = packetmgr_new_send( qq, QQ_CMD_SEND_IM );
	if( !p ) return;
	bytebuffer *buf = p->buf;
	put_int( buf, qq->number );
	put_int( buf, to );
	//00 00 00 08 00 01 00 04 00 00 00 00  09SP1 changes
	put_int( buf, 0x00000008 );
	put_int( buf, 0x00010004 );
	put_int( buf, 0x00000000 );
	put_word( buf, qq->version );
	put_int( buf, qq->number );
	put_int( buf, to );
	put_data( buf, qq->data.im_key, 16 );
	put_word( buf, QQ_NORMAL_IM_TEXT );	//message type
	put_word( buf, p->seqno );
	put_int( buf, p->time_create );
	put_word( buf, qq->self->face );	//my face
	put_int( buf, 1 );	//has font attribute
	put_byte( buf, slice_count );	//slice_count
	put_byte( buf, which_piece );	//slice_no
	put_word( buf, msg_id );	//msg_id??
	put_byte( buf, QQ_IM_TEXT );	//auto_reply
	put_int( buf, 0x4D534700 ); //"MSG"
	put_int( buf, 0x00000000 );
	put_int( buf, p->time_create );
	put_int( buf, (msg_id<<16)|msg_id );	//maybe a random interger
	/*
	put_int( buf, 0x00000000 );
	put_int( buf, 0x09008600 );
	*/
	// Color should actually memcpy of COLORREF
	put_byte( buf, 0 );
	put_byte( buf, blue );
	put_byte( buf, green );
	put_byte( buf, red );
	put_byte( buf, fontsize );
	
	uchar biu=0;
	if (bold!=0) biu|=1;
	if (italic!=0) biu|=2;
	if (underline!=0) biu|=4;
	put_byte( buf, biu );

	put_word( buf, 0x8600);
	
	put_word( buf, strlen(fontname) );
	put_data( buf, (uchar*)fontname, strlen( fontname) );
	put_word( buf, 0x0000 );

	/*
	put_byte( buf, 0x01 );
	put_word( buf, len+3 );
	put_byte( buf, 1 );
	put_word( buf, len );
//	put_word( buf, p->seqno );
	put_data( buf, (uchar*)msg, len );
	*/

	char* msg=(char*)malloc(len+1);
	char* pMsg=msg;
	char* psz14;
	char* psz15;

	memcpy(msg,msg2,len);
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
			put_byte( buf, 0x01 ); // Content Type
			put_word( buf, len+3 ); // Size of this section
			put_byte( buf, 0x01 ); // Content indicator
			put_word( buf, len ); // Size of content
			put_data( buf, (uchar*)pMsg, len );
			pMsg+=strlen(pMsg);
		}

		if (psz14) {
			// Write smiley
			put_byte( buf, 0x02 ); // Content Type
			put_word( buf, 0x09 ); // Size of this section
			put_byte( buf, 0x01 ); // Content indicator: 2009
			put_word( buf, 0x01 ); // Size of content: 1 byte
			put_byte( buf, prot_im_get2006smiley( ( (uchar*)psz14)[1] ) ); // TODO
			put_byte( buf, 0xff ); // Content indicator: Legacy
			put_word( buf, 0x02 ); // Size of content: 2 bytes
			put_byte( buf, 0x14 ); // Smiley header
			put_byte( buf, psz14[1] ); // Smiley data
			pMsg=psz14+2;
		} else if (psz15) {
			// Write image
			char szLen[10];
			uchar cbShortcut;
			uchar cbFilename;
			memcpy(szLen,psz15+2,3);
			if (*szLen==' ') *szLen=0;
			szLen[3]=0;
			len=atoi(szLen);
			cbShortcut=psz15[6]-'A';
			cbFilename=len-49-cbShortcut;
			put_byte( buf, 0x03 ); // Content Type
			put_word( buf, 53+len+cbFilename ); // Size of this section

			put_byte( buf, 0x02 ); // Content indicator: file name
			put_data( buf, (uchar*)(psz15+49), cbFilename );

			for (int c=0; c<3; c++) {
				put_byte( buf, 0x04+c ); // Content indicator: 04=SessionID, 05=IP, 06=Port
				put_word( buf, 0x04 ); // Size of content
				memcpy(szLen,psz15+9+8*c,8);
				szLen[8]=0;

				put_int( buf, strtoul(szLen,NULL,16) );
			}

			put_byte( buf, 0x07 ); // Content indicator: Format (A=GIF C=JPG)
			put_word( buf, 0x01 ); // Size of content
			put_byte( buf, psz15[8] ); 

			put_byte( buf, 0x08 ); // Content indicator: File Agent Key
			put_word( buf, 0x10 ); // Size of content
			put_data( buf, (uchar*)(psz15+33), 16 );

			put_byte( buf, 0x01 ); // Content indicator: Unknown
			put_word( buf, 0x01 ); // Size of content
			put_byte( buf, 0x01 );

			put_byte( buf, 0xff ); // Content indicator: Legacy
			put_word( buf, len ); // Size of content
			put_byte( buf, 0x15 );
			put_data( buf, (uchar*)psz15, len );
			pMsg=psz15+len;
		}
	}

	free(msg);

	post_packet( qq, p, SESSION_KEY );
}

} // extern "C"
