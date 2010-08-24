#ifndef _F_UTIL_H_
#define  _F_UTIL_H_

LPSTR  gencnonce(void);
LPSTR  gencallid(void);
LPSTR  get_token(LPCSTR str, LPCSTR start, LPCSTR end);
LPSTR fetion_cipher_digest_calculate_response(
		LPCSTR sid,
		LPCSTR domain,
		LPCSTR password,
		LPCSTR nonce,
		LPCSTR cnonce);
bool IsCMccNo(LPCSTR name);
bool IsUnicomNo(LPSTR name);
LPSTR auth_header(CNetwork *sip,
		struct sip_auth *auth, LPCSTR method, LPCSTR target);
LPSTR parse_attribute(LPCSTR attrname, LPCSTR source);
 void fill_auth(CNetwork *sip, LPCSTR hdr, struct sip_auth *auth);
LPSTR parse_from(LPCSTR hdr);
LPSTR fetion_ssi_calcuate_digest(LPCSTR password);

#endif

