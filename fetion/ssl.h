/////////////////////////////////////////////////////////////////////////////////////////
// Basic SSL operation class

struct SSL_Base
{
	CNetwork *proto;

	SSL_Base(CNetwork *prt) { proto = prt; }
	virtual	~SSL_Base() {};

	//virtual  int init(void) = 0;
	virtual  char* getSslResult( const char* parUrl, const char* parAuthInfo, const char* hdrs ) = 0;
};

class SSLAgent
{
private:
	SSL_Base* pAgent;

public:
	SSLAgent(CNetwork* proto);
	~SSLAgent();

	char* getSslResult( char** parUrl, const char* parAuthInfo, const char* hdrs,
		unsigned& status, char*& htmlbody);
};

/////////////////////////////////////////////////////////////////////////////////////////
//	Windows error class

struct TWinErrorCode
{
	WINAPI	TWinErrorCode();
	WINAPI	~TWinErrorCode();

	char*		WINAPI getText();

	long		mErrorCode;
	char*		mErrorText;
};

/////////////////////////////////////////////////////////////////////////////////////////
//	MIME headers processing

class MimeHeaders
{
public:

	MimeHeaders();
	MimeHeaders( unsigned );
	~MimeHeaders();

	void clear(void);
	char*	readFromBuffer( char* pSrc );
	const char* find( const char* fieldName );
	const char* operator[]( const char* fieldName ) { return find( fieldName ); }
	char* decodeMailBody(char* msgBody);

	static wchar_t* decode(const char* val);

	void  addString( const char* name, const char* szValue, unsigned flags = 0 );
	void    addLong( const char* name, long lValue );
	void   addULong( const char* name, unsigned lValue );
	void	addBool( const char* name, bool lValue );

	size_t  getLength( void );
	char* writeToBuffer( char* pDest );

private:
	typedef struct tag_MimeHeader
	{
		const char* name;
		const char* value;
		unsigned flags;
	} MimeHeader;

	unsigned	mCount;
	unsigned	mAllocCount;
	MimeHeader* mVals;

	unsigned allocSlot(void);
};
