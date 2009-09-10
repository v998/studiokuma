#include "stdafx.h"
//extern int g_callid;

int g_callid=0;

LPSTR  gencnonce(void)
{
	char szTemp[40];
	sprintf(szTemp,"%04X%04X%04X%04X%04X%04X%04X%04X",rand() & 0xFFFF,rand() & 0xFFFF,rand() & 0xFFFF,rand() & 0xFFFF,rand() & 0xFFFF,rand() & 0xFFFF,rand() & 0xFFFF,rand() & 0xFFFF);
	return mir_strdup(szTemp);
}


LPSTR  gencallid(void)
{
	char szTemp[20];
	sprintf(szTemp,"%d",++g_callid);
	return mir_strdup(szTemp);
}


LPSTR  get_token(LPCSTR str, LPCSTR start, LPCSTR end)
{
	const char *c, *c2; 

	if ((c = strstr(str, start)) == NULL)
		return NULL;

	c += strlen(start);

	if (end != NULL)
	{
		if ((c2 = strstr(c, end)) == NULL)
			return NULL;

		return g_strndup(c, c2 - c);
	}
	else
	{
		/* This has to be changed */
		return mir_strdup(c);
	}

}

LPSTR hex2string(unsigned char* szHex, DWORD cbHex, LPSTR szOutput) {
	LPSTR pszOutput=szOutput;

	for (;cbHex;cbHex--,szHex++) {
		sprintf(pszOutput,"%02X",*szHex);
		pszOutput+=2;
	}
	*pszOutput=0;
	return szOutput;
}

unsigned char* string2hex(LPSTR szString, unsigned char* szOutput) {
	char szCvt[3];
	unsigned char* pszOutput=szOutput;

	szCvt[2]=0;

	for (;*szString;szString+=2, pszOutput++) {
		strncpy(szCvt,szString,2);
		*pszOutput=(unsigned char)strtol(szCvt,NULL,16);
	}
	*pszOutput=0;
	return szOutput;
}

LPSTR fetion_cipher_digest_calculate_response(
		LPCSTR sid,
		LPCSTR domain,
		LPCSTR password,
		LPCSTR nonce,
		LPCSTR cnonce)
{
#if 1
	unsigned char digest[24];
	char temp[49];
	LPSTR hash3;

/*
function fetion_hash_password($password) {
	// in fact, salt is constant value
	$salt = chr(0x77).chr(0x7A).chr(0x6D).chr(0x03);
	$src = $salt.hash('sha1', $password, true);
	return strtoupper(bin2hex($salt.sha1($src, true)));
	*/

	memcpy(temp,SALT_BIN,4);
	mir_sha1_hash((mir_sha1_byte_t*)password,strlen(password),(unsigned char*)(temp+4));
	mir_sha1_hash((mir_sha1_byte_t*)temp,24,digest);
	hash3=(LPSTR)mir_alloc(20);
	memmove(hash3,digest,20);

	/*
}
*/


	//unsigned char digest[16];
	LPSTR hash1; /* We only support MD5. */
	LPSTR hash2; /* We only support MD5. */
	//char temp[33];
	LPSTR response; /* We only support MD5. */
	mir_md5_state_t mmst;
	mir_sha1_ctx msc;
	mir_sha1_init(&msc);
	mir_sha1_append(&msc, (mir_sha1_byte_t*)sid, strlen(sid));
	mir_sha1_append(&msc, (mir_sha1_byte_t*)":", 1);
	mir_sha1_append(&msc, (mir_sha1_byte_t*)domain, strlen(domain));
	mir_sha1_append(&msc, (mir_sha1_byte_t*)":", 1);
	//mir_md5_append(&msc, (mir_sha1_byte_t*)password, strlen(password));
	mir_sha1_append(&msc, (mir_sha1_byte_t*)hash3, 20);
	mir_sha1_finish(&msc, digest);

	mir_md5_init(&mmst);
	mir_md5_append(&mmst, digest, 20);
	mir_md5_append(&mmst, (mir_md5_byte_t *)":", 1);
	mir_md5_append(&mmst, (mir_md5_byte_t *)nonce, strlen(nonce));
	mir_md5_append(&mmst, (mir_md5_byte_t *)":", 1);
	mir_md5_append(&mmst, (mir_md5_byte_t *)cnonce, strlen(cnonce));
	mir_md5_finish(&mmst, digest);
	hash1=mir_strdup(hex2string(digest,16,temp));
	
	mir_md5_init(&mmst);
	mir_md5_append(&mmst,(mir_md5_byte_t *)"REGISTER",8 );
	mir_md5_append(&mmst,(mir_md5_byte_t *)":",1 );
	mir_md5_append(&mmst,(mir_md5_byte_t *)sid, strlen(sid) );
	mir_md5_finish(&mmst, digest);
	hash2=mir_strdup(hex2string(digest,16,temp));

	mir_md5_init(&mmst);
	mir_md5_append(&mmst,(mir_md5_byte_t *)hash1,strlen(hash1) );
	mir_md5_append(&mmst,(mir_md5_byte_t *)":",1 );
	mir_md5_append(&mmst,(mir_md5_byte_t *)nonce, strlen(nonce));
	mir_md5_append(&mmst,(mir_md5_byte_t *)":",1 );
	mir_md5_append(&mmst,(mir_md5_byte_t *)hash2,strlen(hash2) );
	mir_md5_finish(&mmst, digest);
	response=mir_strdup(hex2string(digest,16,temp));

	mir_free(hash1);
	mir_free(hash2);
	mir_free(hash3);
#else

	unsigned char digest[16];
	LPSTR hash1; /* We only support MD5. */
	LPSTR hash2; /* We only support MD5. */
	char temp[33];
	LPSTR response; /* We only support MD5. */
	mir_md5_state_t mmst;
	mir_md5_init(&mmst);
	mir_md5_append(&mmst, (mir_md5_byte_t*)sid, strlen(sid));
	mir_md5_append(&mmst, (mir_md5_byte_t*)":", 1);
	mir_md5_append(&mmst, (mir_md5_byte_t*)domain, strlen(domain));
	mir_md5_append(&mmst, (mir_md5_byte_t*)":", 1);
	mir_md5_append(&mmst, (mir_md5_byte_t*)password, strlen(password));
	mir_md5_finish(&mmst, digest);

	mir_md5_init(&mmst);
	mir_md5_append(&mmst, digest, 16);
	mir_md5_append(&mmst, (mir_md5_byte_t *)":", 1);
	mir_md5_append(&mmst, (mir_md5_byte_t *)nonce, strlen(nonce));
	mir_md5_append(&mmst, (mir_md5_byte_t *)":", 1);
	mir_md5_append(&mmst, (mir_md5_byte_t *)cnonce, strlen(cnonce));
	mir_md5_finish(&mmst, digest);
	hash1=mir_strdup(hex2string(digest,16,temp));
	
	mir_md5_init(&mmst);
	mir_md5_append(&mmst,(mir_md5_byte_t *)"REGISTER",8 );
	mir_md5_append(&mmst,(mir_md5_byte_t *)":",1 );
	mir_md5_append(&mmst,(mir_md5_byte_t *)sid, strlen(sid) );
	mir_md5_finish(&mmst, digest);
	hash2=mir_strdup(hex2string(digest,16,temp));

	mir_md5_init(&mmst);
	mir_md5_append(&mmst,(mir_md5_byte_t *)hash1,strlen(hash1) );
	mir_md5_append(&mmst,(mir_md5_byte_t *)":",1 );
	mir_md5_append(&mmst,(mir_md5_byte_t *)nonce, strlen(nonce));
	mir_md5_append(&mmst,(mir_md5_byte_t *)":",1 );
	mir_md5_append(&mmst,(mir_md5_byte_t *)hash2,strlen(hash2) );
	mir_md5_finish(&mmst, digest);
	response=mir_strdup(hex2string(digest,16,temp));

	mir_free(hash1);
	mir_free(hash2);
#endif
	return response;
}

bool IsCMccNo(LPCSTR name)
{
	int  mobileNo;
	int head;
	LPSTR szMobile;
	szMobile=mir_strdup(name);
	szMobile[7]='\0';
	mobileNo = atoi(szMobile);

	head = mobileNo / 10000;
	util_log("IsCMccNo:[%d]\n",mobileNo);
	mir_free(szMobile);
	if ((mobileNo <= 1300000) || (mobileNo >= 1600000))
	{
		return FALSE;
	}
	if (((head < 134) || (head > 139)) && (((head != 159) && (head != 158)) && (head != 157)))
	{   
		return (head == 150);
	}   
	return TRUE;

}

bool IsUnicomNo(LPSTR name)
{
	int mobileNo;
	int head;
	LPSTR szMobile;
	szMobile=mir_strdup(name);
	szMobile[7]='\0';
	mobileNo = atoi(szMobile);
	head = mobileNo / 10000;
	mir_free(szMobile);
	if ((mobileNo <= 1300000) || (mobileNo >= 1600000))
	{
		return FALSE;
	}
	if (((head>=130 ) && (head <=133))||head==153)
	{   
		return TRUE;
	}   

	return FALSE;

}

LPSTR auth_header(CNetwork *sip,
		struct sip_auth *auth, LPCSTR method, LPCSTR target)
{
	LPSTR ret;
	// ret = g_strdup_printf("Digest response=\"%s\",cnonce=\"%s\"",auth->digest_session_key,auth->cnonce );
	ret = g_strdup_printf("Digest algorithm=\"SHA1-sess\",response=\"%s\",cnonce=\"%s\",salt=\"" SALT "\"",auth->digest_session_key,auth->cnonce );
	// ret = g_strdup_printf("Digest algorithm=\"MD5-sess\" response=\"%s\",cnonce=\"%s\"",auth->digest_session_key,auth->cnonce );
	return ret;
}

LPSTR parse_attribute(LPCSTR attrname, LPCSTR source)
{
	const char *tmp ;
	char *retval = NULL;
	int len = strlen(attrname);
	tmp = strstr(source,attrname);

	if(tmp)
		retval = mir_strdup(tmp+len);

	return retval;
}

 void fill_auth(CNetwork *sip, LPCSTR hdr, struct sip_auth *auth)
{
	LPSTR tmp;

	if(!hdr)
	{
		util_log("fill_auth: hdr==NULL\n");
		return;
	}

	auth->type = 1;
	auth->cnonce = gencnonce();
	auth->domain = mir_strdup("fetion.com.cn");
	if((tmp = parse_attribute("nonce=\"", hdr))) {
		auth->nonce = strupr(tmp); //g_ascii_strup(tmp,32);
		auth->nonce[32]=0;
	}
	util_log("nonce: %s domain: %s\n", auth->nonce ? auth->nonce : "(null)", auth->domain ? auth->domain : "(null)");
	if(auth->domain) 
		auth->digest_session_key = fetion_cipher_digest_calculate_response(
				sip->username(), auth->domain, sip->password(), auth->nonce, auth->cnonce);

}

LPSTR parse_from(LPCSTR hdr)
{
	LPSTR from;
	LPCSTR tmp, tmp2 = hdr;

	if(!hdr) return NULL;
	util_log("parsing address out of %s\n", hdr);
	tmp = strchr(hdr, '<');

	/* i hate the different SIP UA behaviours... */
	if(tmp)
	{ /* sip address in <...> */
		tmp2 = tmp + 1;
		tmp = strchr(tmp2, '>');
		if(tmp)
		{
			from = g_strndup(tmp2, tmp - tmp2);
		} else {
			util_log("found < without > in From\n");
			return NULL;
		}
	} else {
		tmp = strchr(tmp2, ';');
		if(tmp)
		{
			from = g_strndup(tmp2, tmp - tmp2);
		} else {
			from = mir_strdup(tmp2);
		}
	}
	util_log("got %s\n", from);
	return from;
}

