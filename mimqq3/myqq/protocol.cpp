/*  *** This file is modified data types for MSVC
 *  protocol.c
 *
 *  QQ Protocol. Deal with sending and receving packets.
 *
 *  Copyright (C) 2008  Huang Guan
 *
 *  2008-7-12 Created.
 *
 *  Description: This file mainly includes the functions about 
 *
 */

#include <StdAfx.h>

extern "C" {
	/*
#include <string.h>
#include <stdlib.h>
#include <time.h>
#ifdef __WIN32__
#include <winsock.h>
#include <wininet.h>
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif
	*/
#include "qqdef.h"
#include "qqclient.h"
#include "qqcrypt.h"
#include "debug.h"
#include "memory.h"
#include "protocol.h"

//encypt data
int post_packet( struct qqclient* qq, qqpacket* p, int key_type )
{
	bytebuffer* buf = p->buf;
	unsigned char* encrypted;
	int head_len = qq->network==TCP || qq->network==PROXY_HTTP ? 24 : 22;
	static uchar unknown_packet_flags[] = {0x02,0x00,0x00,0x00,0x01,0x01,0x01,0x00,0x00,0x64,0x70};
	if( qq->log_packet ){
		if( p->command == QQ_CMD_QUN_CMD )
			DBG("[%d] send packet cmd: %x subcmd: %x  seq: %x", qq->number, p->command, (int)p->buf->data[0], p->seqno );
		else
			DBG("[%d] send packet cmd: %x  seq: %x", qq->number, p->command, p->seqno );
		hex_dump( buf->data, buf->pos );
	}
	switch( key_type )//use this key to enrypt data
	{
		case NO_KEY:
		{
			if( buf->pos+head_len+1 <= buf->size ){
				memmove( buf->data+head_len, buf->data, buf->pos );
				buf->pos += head_len;
			}
			break;
		}
		case RANDOM_KEY:
		{
			int out_len = PACKET_SIZE;
			NEW( encrypted, PACKET_SIZE, unsigned char );
			if( !encrypted ) {
				DBG("Error: encrypted not allocated.");
				return -1;
			}
			qqencrypt( buf->data, buf->pos, p->key, encrypted, &out_len );
			//for tcp
			if( p->command == QQ_CMD_LOGIN_SEND_INFO || p->command == QQ_CMD_LOGIN_GET_INFO ||
				p->command == QQ_CMD_LOGIN_GET_LIST || p->command == QQ_CMD_LOGIN_A4 ){
				if( out_len+head_len+1+(2+qq->data.login_info_magic.len) <= buf->size ){
					*(ushort*)(buf->data+head_len) = htons( qq->data.login_info_magic.len );
					memcpy( buf->data+head_len+2, qq->data.login_info_magic.data, qq->data.login_info_magic.len );
					memcpy( buf->data+qq->data.login_info_magic.len+head_len+2, encrypted, out_len );
					buf->pos = out_len+qq->data.login_info_magic.len+head_len+2;
				}else{
					DBG("encrypted data is too large to store in the packet.");
				}
			}else{
				if( out_len+16+head_len+1 <= buf->size ){
					memcpy( buf->data+head_len, p->key, 16 );
					memcpy( buf->data+16+head_len, encrypted, out_len );
					buf->pos = out_len+16+head_len;
				}else{
					DBG("encrypted data is too large to store in the packet.");
				}
			}
			DEL( encrypted );
			break;
		}
		case SESSION_KEY:
		{
			int out_len = PACKET_SIZE;
			NEW( encrypted, PACKET_SIZE, unsigned char );
			if( !encrypted ) {
				DBG("Error: encrypted not allocated.");
				return -2;
			}
			qqencrypt( buf->data, buf->pos, qq->data.session_key, encrypted, &out_len );
			//for tcp
			if( out_len+head_len+1 <= buf->size ){
				memcpy( buf->data+head_len, encrypted, out_len );
				buf->pos = out_len+head_len;
			}else{
				DBG("encrypted data is too large to store in the packet.");
			}
			DEL( encrypted );
			break;
		}
	}
	//end the packet
	put_byte( buf, p->tail );
	buf->len = buf->pos;
	buf->pos = 0;
	//basic information
	if( qq->network == TCP || qq->network==PROXY_HTTP )
		put_word( buf, buf->len );
	put_byte( buf, p->head );
	put_word( buf, p->version );
	put_word( buf, p->command );
	put_word( buf, p->seqno );
	put_int( buf, qq->number );
	put_data( buf, unknown_packet_flags, 11 );
	p->key_type = key_type;
	return packetmgr_put( qq, p );
}

static int decrypt_with_key( qqclient* qq, qqpacket* p, bytebuffer* buf, uchar* key )
{
	int out_len = PACKET_SIZE;
	int head_len = qq->network==TCP || qq->network==PROXY_HTTP ? 16 : 14;
	int ret;
	uchar* decrypted;
	NEW( decrypted, PACKET_SIZE, unsigned char );
	if( !decrypted ) {
		DBG("Error: decrypted not allocated.");
		return -1;
	}
	ret = qqdecrypt( buf->data+head_len, buf->len-head_len-1, key, decrypted, &out_len );
	if( ret ){
		//for tcp
		if( !p->buf )
			return 0;
		if( out_len < PACKET_SIZE )
			memcpy( p->buf->data, decrypted, out_len );
		else
			DBG("Wrong out_len : %d", out_len );
		p->buf->pos = 0;
		p->buf->len = out_len;
	}
	DEL( decrypted );
	return ret;
}

static int decrypt_packet( qqclient* qq, qqpacket* p, bytebuffer* buf )
{
	//size2+head1+version2+command2+sequence2 = 9
	int head_len = qq->network==TCP || qq->network==PROXY_HTTP ? 16 : 14;
	if( p->match ){
		switch( p->match->key_type ){
		case NO_KEY:
			memcpy( p->buf->data, buf->data+head_len, buf->len-head_len-1 );
			p->buf->len = buf->len-head_len-1;
			p->buf->pos = 0;
			return 1;
		case RANDOM_KEY:
		{
			if( p->command == QQ_CMD_LOGIN_VERIFY ){
				if( decrypt_with_key( qq, p, buf, qq->data.verify_key2 ) )
					return 1;
			}
			if( p->command == QQ_CMD_LOGIN_GET_INFO ){
				if( decrypt_with_key( qq, p, buf, qq->data.login_info_key3 ) )
					return 1;
			}
			if( p->command == QQ_CMD_LOGIN_SEND_INFO || p->command == QQ_CMD_LOGIN_GET_LIST ){
				if( decrypt_with_key( qq, p, buf, qq->data.login_info_key2 ) )
					return 1;
			}
			if( decrypt_with_key( qq, p, buf, p->match->key ) )
				return 1;
			break;
		}
		case SESSION_KEY:
		{
			if( decrypt_with_key( qq, p, buf, qq->data.session_key ) )
				return 1;
			break;
		}
		}
	}else{
		if( decrypt_with_key( qq, p, buf, qq->data.session_key ) )
			return 1;
	}
	if( decrypt_with_key( qq, p, buf, qq->md5_pass2 ) )
		return 1;
	DBG("cannot decrypt packet. cmd:%x", p->command );
	return 0;

}

int process_packet( qqclient* qq, qqpacket* p, bytebuffer* buf )
{
	if( !decrypt_packet( qq, p, buf ) )
		return -1;
	if( qq->log_packet ){
		DBG("[%d] recv packet ver:%x cmd: %x seqno: %x", qq->number, p->version, p->command, p->seqno );
		hex_dump( p->buf->data, p->buf->len );
	}

	// memcpy(qq->mimnetwork->m_libevabuffer,p->buf->data,p->buf->len);
	qq->mimnetwork->m_packet=p;

	switch( p->command ){
	case QQ_CMD_TOUCH:
		prot_login_touch_reply( qq, p );
		break;
	case QQ_CMD_LOGIN_REQUEST:
		prot_login_request_reply( qq, p );
		break;
	case QQ_CMD_LOGIN_VERIFY:
		prot_login_verify_reply( qq, p );
		break;
	case QQ_CMD_LOGIN_GET_INFO:
		prot_login_get_info_reply( qq, p );
		break;
	case QQ_CMD_LOGIN_A4:
		prot_login_a4_reply( qq, p );
		break;
	case QQ_CMD_LOGIN_GET_LIST:
		prot_login_get_list_reply( qq, p );
		break;
	case QQ_CMD_LOGIN_SEND_INFO:
		prot_login_send_info_reply( qq, p );
		break;
	case QQ_CMD_E9:
		prot_login_e9_reply( qq, p );
		break; 
	case QQ_CMD_EA:
		prot_login_ea_reply( qq, p );
		break; 
	case QQ_CMD_EC:
		prot_login_ec_reply( qq, p );
		break; 
	case QQ_CMD_ED:
		prot_login_ed_reply( qq, p );
		break; 
	case QQ_CMD_KEEP_ALIVE:
		prot_user_keep_alive_reply( qq, p );
		break;
	case QQ_CMD_RECV_IM_09:
	case QQ_CMD_RECV_IM:
		// MirandaQQ handles message by itself due to userhead/emoticon stuff
		// however ack is better handled by myqq3 in case of proto change
		 prot_im_recv_msg( qq, p );
		//prot_im_ack_recv( qq, p );
		break;
	case QQ_CMD_CHANGE_STATUS:
		prot_user_change_status_reply( qq, p );
		break;
#ifndef NO_BUDDY_INFO
	case QQ_CMD_GET_BUDDY_LIST:
		prot_buddy_update_list_reply( qq, p );
		break;
	case QQ_CMD_GET_BUDDY_ONLINE:
		prot_buddy_update_online_reply( qq, p );
		break;
	case QQ_CMD_BUDDY_STATUS:
		prot_buddy_status( qq, p );
		break;
#endif
#ifndef NO_QUN_INFO
	case QQ_CMD_QUN_CMD:
		prot_qun_cmd_reply( qq, p );
		break;
#endif
	case QQ_CMD_GET_KEY:
		prot_user_get_key_reply( qq, p );
		break;
	case QQ_CMD_GET_NOTICE:
		prot_user_get_notice_reply( qq, p );
		break;
	case QQ_CMD_CHECK_IP:
		prot_user_check_ip_reply( qq, p );
		break;
#ifndef NO_BUDDY_DETAIL_INFO
	case QQ_CMD_GET_BUDDY_SIGN:
		prot_buddy_update_signiture_reply( qq, p );
		break;
	case QQ_CMD_ACCOUNT:
		prot_buddy_update_account_reply( qq, p );
		break;
	case QQ_CMD_BUDDY_ALIAS:
		prot_buddy_update_alias_reply( qq, p );
		break;
	case QQ_CMD_GET_BUDDY_EXTRA_INFO:
		prot_buddy_get_extra_info_reply( qq, p );
		break;
	case QQ_CMD_BUDDY_INFO:
		prot_buddy_get_info_reply( qq, p );
		break;
#endif
#ifndef NO_GROUP_INFO
	case QQ_CMD_GROUP_LABEL:
		prot_group_download_labels_reply( qq, p );
		break;
#endif
	case QQ_CMD_SEND_IM:
		break;
	case QQ_CMD_BROADCAST:
		prot_misc_broadcast( qq, p );
		break;
	case QQ_CMD_GET_LEVEL:
		prot_user_get_level_reply( qq, p );
		break;
	case QQ_CMD_ADDBUDDY_REQUEST:
		prot_buddy_request_addbuddy_reply( qq, p );
		break;
	case QQ_CMD_ADDBUDDY_VERIFY:
		prot_buddy_verify_addbuddy_reply( qq, p );
		break;
	case QQ_CMD_REQUEST_TOKEN:
		prot_user_request_token_reply( qq, p );
		break;
	case QQ_CMD_DEL_BUDDY:
		prot_buddy_del_buddy_reply( qq, p );
		break;
	case QQ_CMD_SEARCH_UID:
		prot_buddy_search_uid_reply( qq, p );
		break;
	case 0xa6: // Weather
		prot_weather_reply( qq, p );
		break;
	default:
		DBG("unknown cmd: %x", p->command );
		hex_dump( p->buf->data, p->buf->len );
		break;
	}

	/*
	qq->mimnetwork->m_packet=p;
	qq->mimnetwork->processPacket(qq->mimnetwork->m_libevabuffer,p->buf->len);
	*/

	return 0;
}
} // extern "C"
