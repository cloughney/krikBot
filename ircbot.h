#ifndef IRCBOT_H
#define IRCBOT_H
#endif

#include <string>
#include <vector>

class IRCBot
{
private:
	const char *host;
	const char *port;
	const char *nick;
	const char *altNick;
	char publicTrigger;
	std::string masterHost;
	std::vector<std::string> opHosts;
	std::string pwd_master;
	std::string pwd_op;

	int sock;
	int sock_status;

	//util functions
	std::string strToUpper(std::string);

	//users
	bool isOper(std::string);

	//irc
	void sendRawMsg(std::string);
	void sendPrivMsg(std::string, std::string);
	void messageLoop();
public:
	IRCBot();
	~IRCBot();
	bool start(const char *, const char *, const char *, std::string, std::string);
	void stop();
};