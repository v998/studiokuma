/*  ** This file is modified for MirandaQQ3
 *  qqclient.c
 *
 *  QQ Client. 
 *
 *  Copyright (C) 2008  Huang Guan
 *
 *  2008-7-12 Created.
 *  2008-10-26 TCP UDP Server Infos are loaded from configuration file.
 *
 *  Description: This file mainly includes the functions about 
 *  loading configuration, connecting server, guard thread, basic interface
 *
 */
#include <StdAfx.h>

extern "C" {
#include "memory.h"
#include "debug.h"
#include "config.h"
#include "qqsocket.h"
#include "packetmgr.h"
#include "qun.h"
#include "group.h"
#include "buddy.h"
#include "util.h"
#include "qqconn.h"

#if 0
static void read_config( qqclient* qq )
{
	assert( g_conf );
	if( !tcp_server_count && !udp_server_count ){
		char* tcps, *udps;
		tcps = config_readstr( g_conf, "QQTcpServerList" );
		udps = config_readstr( g_conf, "QQUdpServerList" );
		if( tcps ){
			read_server_addr( tcp_servers, tcps, &tcp_server_count );
		}
		if( udps ){
			read_server_addr( udp_servers, udps, &udp_server_count );
		}
	}
	qq->log_packet = config_readint( g_conf, "QQPacketLog" );
	if( config_readstr( g_conf, "QQNetwork" ) && stricmp( config_readstr( g_conf, "QQNetwork" ), "TCP" ) == 0 )
		qq->network = TCP;
	else
		qq->network = UDP;
	if( config_readstr( g_conf, "QQVerifyDir" ) )
		strncpy( qq->verify_dir, config_readstr( g_conf, "QQVerifyDir" ), PATH_LEN );
	if( qq->verify_dir == NULL )
		strcpy( qq->verify_dir, "./web/verify" );
	mkdir_recursive( qq->verify_dir );
}
#endif

int qqclient_create( qqclient* qq, uint num, char* pass )
{
	uchar md5_pass[16];

	//加密密码
	mir_md5_state_t mst;
	mir_md5_init(&mst);
	mir_md5_append( &mst, (mir_md5_byte_t*)pass, strlen(pass) );
	mir_md5_finish( &mst, (mir_md5_byte_t*)md5_pass );

	return qqclient_md5_create( qq, num, md5_pass );
}


static void delete_func(const void *p)
{
	DEL( p );
}

int qqclient_md5_create( qqclient* qq, uint num, uchar* md5_pass )
{
	mir_md5_state_t mst;
	//make sure all zero
	memset( qq, 0, sizeof( qqclient ) );
	qq->number = num;
	memcpy( qq->md5_pass1, md5_pass, 16 );
	//
	mir_md5_init( &mst );
	mir_md5_append( &mst, (mir_md5_byte_t*)qq->md5_pass1, 16 );
	mir_md5_finish( &mst, (mir_md5_byte_t*)qq->md5_pass2 );
	qq->mode = QQ_ONLINE;
	qq->process = P_INIT;
	// read_config( qq );
	qq->version = QQ_VERSION;
	//
// #if 0 // Handle by MIM DB
	list_create( &qq->buddy_list, MAX_BUDDY );
	list_create( &qq->qun_list, MAX_QUN );
	list_create( &qq->group_list, MAX_GROUP );
// #endif
	loop_create( &qq->event_loop, MAX_EVENT, delete_func );
	loop_create( &qq->msg_loop, MAX_EVENT, delete_func );
	pthread_mutex_init( &qq->mutex_event, NULL );

	//create self info
	qq->self = buddy_get( qq, qq->number, 1 );
	if( !qq->self ){
		DBG("[%d] Fatal error: qq->self == NULL", qq->number);
		return -1;
	}

	return 0;
}

#define INTERVAL 5
void* qqclient_keepalive( void* data )
{
	qqclient* qq = (qqclient*) data;
	static int counter = 0;
	static time_t firsttime=0;
	static uint kacount=0;
	static bool buddyUpdated=false;
	static bool levelUpdated=false;
	time_t thistime=time(NULL);
	// DBG("keepalive");
	if (firsttime==0) {
		firsttime=thistime;
	}
	counter=thistime-firsttime;

	if( qq->process != P_INIT ){
		// counter ++;
		/*if( counter % INTERVAL == 0 )*/{
			if( qq->process == P_LOGGING || qq->process == P_LOGIN ){
				packetmgr_check_packet( qq, 5 );
				if( qq->process == P_LOGIN ){
					//1次心跳/分钟
					// if( counter / ( 1 *30*INTERVAL) > 0 ){
					if( counter / 30 > 0 ){
						prot_user_keep_alive( qq );
						firsttime=0;
						kacount++;
					}
					//10分钟刷新在线列表 QQ2009是5分钟刷新一次。
					// if( counter / ( 10 *60*INTERVAL) > 0 ){
					if (!qq->mimnetwork->IsConservative()) {
						if (kacount>0 && kacount%10==0) {
							if (!buddyUpdated) {
								buddyUpdated=true;
								prot_buddy_update_online( qq, 0 );
								qun_update_online_all( qq );
							}
						} else if (buddyUpdated)
							buddyUpdated=false;
						//30分钟刷新状态和刷新等级
						// if( counter / ( 30 *60*INTERVAL) > 0 ){
						if (kacount>0 && kacount%30==0) {
							if (!levelUpdated) {
								levelUpdated=true;
								prot_user_change_status( qq );
								prot_user_get_level( qq );
							}
						} else if (levelUpdated)
							levelUpdated=false;
					}

				//	//等待登录完毕
					if( qq->login_finish==0 ){
						if( loop_is_empty(&qq->packetmgr.ready_loop) && 
							loop_is_empty(&qq->packetmgr.sent_loop) ){
							qq->login_finish = 1;	//we can recv message now.
						}
					}
					qq->online_clock ++;
				}
			}
		}
		// USLEEP( 1000/INTERVAL );
	}
	// DBG("end.");
	return NULL;
}

int qqclient_login( qqclient* qq )
{
	// int ret;
	DBG("login");
	if( qq->process != P_INIT ){
		DBG("please logout first");
		return -1;
	}
	if( qq->login_finish == 2 ){
		qqclient_logout( qq );
	}
	qqclient_set_process( qq, P_LOGGING );
	srand( qq->number + time(NULL) );
	//start packetmgr
	packetmgr_start( qq );
	packetmgr_new_seqno( qq );
	/*
	qqconn_get_server( qq );
	ret = qqconn_connect( qq );
	if( ret < 0 ){
		qqclient_set_process( qq, P_ERROR );
		return ret;
	}
	*/

	//ok, already connected to the server
	/*
	if( qq->network == PROXY_HTTP ){
		qqconn_establish( qq );
	}else{
	*/
		//send touch packet
		// MIMQQ3: Next part is handled inside QQNetwork::connectionEstablished()
	//	if( last_server_ip == 0 ){
			prot_login_touch( qq );
	//	}else{
	//		prot_login_touch_with_info( qq, last_server_info, 15 );
	//	}
	// }
	return 0;
}

void qqclient_detach( qqclient* qq )
{
	if( qq->process == P_INIT )
		return;
	if( qq->process == P_LOGIN ){
		int i;
		for( i = 0; i<4; i++ )
			prot_login_logout( qq );
	}else{
		DBG("process = %d", qq->process );
	}
	qq->login_finish = 0;
	qqclient_set_process( qq, P_INIT );
	//qqsocket_close( qq->http_sock );
	packetmgr_end( qq );
}

//该函数需要等待延时，为了避免等待，可以预先执行detach函数
void qqclient_logout( qqclient* qq )
{
	if( qq->login_finish != 2 ){ 	//未执行过detach
		if( qq->process == P_INIT )
			return;
		qqclient_detach( qq );
	}
	qq->login_finish = 0;
	DBG("joining keepalive");
#ifdef __WIN32__
	pthread_join( qq->thread_keepalive, NULL );
#else
	if( qq->thread_keepalive )
		pthread_join( qq->thread_keepalive, NULL );
#endif
	// packetmgr_end( qq );
}

void qqclient_cleanup( qqclient* qq )
{
	if( qq->login_finish == 2 )
		qqclient_logout( qq );
	if( qq->process != P_INIT )
		qqclient_logout( qq );
	pthread_mutex_lock( &qq->mutex_event );
	
	qun_member_cleanup( qq );
	list_cleanup( &qq->buddy_list );
	list_cleanup( &qq->qun_list );
	list_cleanup( &qq->group_list );
	
	loop_cleanup( &qq->event_loop );
	loop_cleanup( &qq->msg_loop );
	pthread_mutex_destroy( &qq->mutex_event );
}

int qqclient_verify( qqclient* qq, uint code )
{
	if( qq->login_finish == 1 ){
		prot_user_request_token( qq, qq->data.operating_number, qq->data.operation, 1, code );
	}else{
		qqclient_set_process( qq, P_LOGIN );
		prot_login_request( qq, &qq->data.verify_token, code, 0 );
	}
	DBG("verify code: %x", code );
	return 0;
}

int qqclient_add( qqclient* qq, uint number, char* request_str )
{
	qqbuddy* b = buddy_get( qq, number, 0 );
	if( b && b->verify_flag == VF_OK )	{
		prot_buddy_verify_addbuddy( qq, 03, number );	//允许一个请求x
	}else{
		strncpy( qq->data.addbuddy_str, request_str, 50 );
		prot_buddy_request_addbuddy( qq, number );
	}
	return 0;
}

int qqclient_del( qqclient* qq, uint number )
{
	qq->data.operating_number = number;
	prot_user_request_token( qq, number, OP_DELBUDDY, 6, 0 );
	return 0;
}

int qqclient_wait( qqclient* qq, int sec )
{
	int i;
	//we don't want to cleanup the data while another thread is waiting here.
	if( pthread_mutex_trylock( &qq->mutex_event ) != 0 )
		return -1;	//busy?
	for( i=0; (sec==0 || i<sec) && qq->process!=P_INIT; i++ ){
		if( loop_is_empty(&qq->packetmgr.ready_loop) && loop_is_empty(&qq->packetmgr.sent_loop) )
		{
			pthread_mutex_unlock( &qq->mutex_event );
			return 0;
		}
		SLEEP(1);
	}
	pthread_mutex_unlock( &qq->mutex_event );
	return -1;
}

void qqclient_change_status( qqclient* qq, uchar mode )
{
	qq->mode = mode;
	if( qq->process == P_LOGIN ){
		prot_user_change_status( qq );
	}
}

// wait: <0   wait until event arrives
// wait: 0  don't need to wait, return directly if no event
// wait: n(n>0) wait n secondes.
// return: 1:ok  0:no event  -1:error
int qqclient_get_event( qqclient* qq, char* event, int size, int wait )
{
	char* buf;
	//we don't want to cleanup the data while another thread is waiting here.
	if( pthread_mutex_trylock( &qq->mutex_event ) != 0 )
		return -1;	//busy?
	for( ; ; ){
		if(  qq->process == P_INIT ){
			pthread_mutex_unlock( &qq->mutex_event );
			return -1;
		}
		buf = (char*)loop_pop_from_head( &qq->event_loop );
		if( buf ){
			int len = strlen( buf );
			if( len < size ){
				strcpy( event, buf );
			}else{
				strncpy( event, buf, size-1 ); //strncpy的n个字符是否包括'\n'？？？
				DBG("buffer too small.");
			}
			delete_func( buf );
			pthread_mutex_unlock( &qq->mutex_event );
			return 1;
		}
		if( qq->online_clock > 10 ){
			buf = (char*)loop_pop_from_head( &qq->msg_loop );
			if( buf ){
				int len = strlen( buf );
				if( len < size ){
					strcpy( event, buf );
				}else{
					strncpy( event, buf, size-1 ); //strncpy的n个字符是否包括'\n'？？？
					DBG("buffer too small.");
				}
				delete_func( buf );
				pthread_mutex_unlock( &qq->mutex_event );
				return 1;
			}
		}
		if( wait<0 || wait> 0 ){
			if( wait>0) wait--;
			USLEEP( 200 );
		}else{
			break;
		}
	}
	pthread_mutex_unlock( &qq->mutex_event );
	return 0;
}

int qqclient_put_event( qqclient* qq, char* event )
{
	char* buf;
	int len = strlen( event );
	NEW( buf, len+1 ,char);
	if( !buf ) return -1;
	strcpy( buf, event );
	// loop_push_to_tail( &qq->event_loop, (void*)buf );
	qq->mimnetwork->_eventCallback(event);
	return 0;
}

int qqclient_put_message( qqclient* qq, char* msg )
{
	char* buf;
	int len = strlen( msg );
	NEW( buf, len+1 ,char);
	if( !buf ) return -1;
	strcpy( buf, msg );
	loop_push_to_tail( &qq->msg_loop, (void*)buf );
	return 0;
}



void qqclient_set_process( qqclient *qq, int process )
{
	char event[16];
	qq->process = process;
	sprintf( event, "process^$%d", process );
	qqclient_put_event( qq, event );
}
} // extern "C"
