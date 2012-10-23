#ifndef HTTPSERVER_H
#define HTTPSERVER_H

class CHttpServer {
public:
	static CHttpServer* GetInstance(unsigned short port=0, const char* qunimagepath=NULL, COpenProtocolHandler* handler=NULL);
	~CHttpServer();

	char m_qunimagepath[260];

private:
	CHttpServer(COpenProtocolHandler* handler, unsigned short port, const char* qunimagepath);

	unsigned short m_port;
};

#endif // HTTPSERVER_H
