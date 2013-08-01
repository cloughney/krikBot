#ifndef IRCBOT_H
#define IRCBOT_H
#endif

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <cstring>
#include <cstddef>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <vector>
#include <unistd.h>
#include <stdlib.h>

#define DEBUGMODE 1

#define MAXBUFFERSIZE 1024
#define MAXMSGPARAMS 5

struct User {
	std::string name;
	std::string passwd;
	std::string nick;
	std::string host;
	int access_level;
	unsigned int flags;
};

struct Channel {
	std::string name;
	std::string key;
	std::string topic;
	char * modes;
	bool administrate;
	User owner;
	std::vector<User> ops;
	std::vector<User> peons;
};

class IRCBot
{
private:
	std::string versionString;

	const char *host;
	const char *port;
	const char *nick;
	const char *altNick;
	char publicTrigger;

	//new authentication system
	std::vector<User> current_users;
	std::vector<Channel> current_channels;

	int sock;
	int sock_status;
	bool keepAlive;
	//std::thread tMessageLoop;

	//util functions
	std::string strToUpper(std::string);
	std::string getRandomLine(std::string);
	int getIntFromStr(std::string);

	//users
	void loadUsers();
	void saveUsers();
	User* getUserByName(std::string);
	User* getUserByHost(std::string);
	User* authUser(std::string, std::string, std::string, std::string);
	User* addUser(std::string, std::string, int);

	//channels
	void loadChannels();
	void addChannel(Channel);
	void removeChannel(Channel);
	void removeChannel(std::string);
	Channel* getChanByName(std::string);
	bool isOper(Channel, User);

	//irc
	void sendRawMsg(std::string);
	void sendPrivMsg(std::string, std::string);
	void sendNoticeMsg(std::string, std::string);
	void messageLoop();
	void handle_command(std::string, std::string[], std::string, std::string, std::string, User*);
public:
	IRCBot();
	~IRCBot();
	bool start(const char *, const char *, const char *);
	void stop();
};