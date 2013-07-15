#ifndef POPCLIENT_H
#define POPCLIENT_H
#endif

#include <string>

class POPClient
{
private:
	const char *host;
	const char *port;
	std::string username;
	std::string passwd;

	int sock;
	bool keepAlive;
	bool establishConnection();
	void connectionLoop(int connReason);
public:
	POPClient();
	virtual ~POPClient();

	void setHost(const char*, const char*);
	void setCred(std::string, std::string);
	bool checkForMail(std::string);

};