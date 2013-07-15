#include "ircbot.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <cstddef>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <vector>
#include <unistd.h>
#include "ircuser.h"

using namespace std;

#define DEBUGMODE 1

#define MAXBUFFERSIZE 1024
#define MAXMSGPARAMS 5

IRCBot::IRCBot()
{
	publicTrigger = '!';
}

IRCBot::~IRCBot()
{
	if (sock > 1) close(sock);
}



//UTILITY FUNCTIONS

string IRCBot::strToUpper(string strIn)
{
	stringstream s;
	for (int i = 0; i < strIn.length(); i++)
	{
		char c = strIn[i];
		if (c > 96 && c < 123)
		{
			c -= 32;
		}
		s << c;
	}
	return s.str();
}





//USER CONTROL

void IRCBot::loadUsers()
{
	//TODO: load users from file
	ifstream f_users_in("ircusers");
	if (f_users_in.is_open())
	{
		while (f_users_in.good())
		{
			string lineIn;
			getline(f_users_in, lineIn);
			if (lineIn[0] == '#') continue; //ignore comments
			
			string uname;
			string pword;
			bool isAdmin;
			bool isMaster;

			short comma = lineIn.find_first_of(',');
			if (comma == string::npos) continue; //break if the line is invalid
			uname = lineIn.substr(0, comma);
			lineIn = lineIn.substr(comma+1);

			comma = lineIn.find_first_of(',');
			if (comma == string::npos) continue; //break if the line is invalid
			pword = lineIn.substr(0, comma);
			lineIn = lineIn.substr(comma+1);

			comma = lineIn.find_first_of(',');
			if (comma == string::npos) continue; //break if the line is invalid
			isAdmin = (lineIn.substr(0, comma) == "1");
			lineIn = lineIn.substr(comma+1);

			isMaster = (lineIn == "1");

			User * newUser = addUser(uname, pword);
			if (newUser != NULL) {
				if (isAdmin) newUser->flags |= USER_ISADMIN;
				if (isMaster) newUser->flags |= (USER_ISADMIN | USER_ISMASTER);
			}
		}
		f_users_in.close();
	}
	else
	{
		ofstream f_conf_out("ircusers");
		if (f_conf_out.is_open())
		{
			f_conf_out << "# FILE USAGE: username,password,isAdmin,isMaster" << endl;
			f_conf_out.close();
		}
		else cout << "Unable to write to ircusers" << endl;
	}
}

User* IRCBot::getUserByName(string n)
{
	for (int i = 0; i < current_users.size(); i++)
	{
		if (current_users.at(i).name == n) return &current_users.at(i);
	}
	return NULL;
}

User* IRCBot::getUserByHost(string h)
{
	for (int i = 0; i < current_users.size(); i++)
	{
		if (current_users.at(i).host == h) return &current_users.at(i);
	}
	return NULL;
}

User* IRCBot::authUser(string u, string p, string n, string h)
{
	for (int i = current_users.size()-1; i >= 0; i--)
	{
		if (current_users.at(i).name == u && current_users.at(i).passwd == p)
		{
			current_users.at(i).nick = n;
			current_users.at(i).host = h;
			current_users.at(i).flags = 0;
			return &current_users.at(i);
		}
	}
	return NULL;
}

User* IRCBot::addUser(string uname, string pword)
{
	if (getUserByName(uname) != NULL) return NULL;
	User newUser;
	newUser.name = uname;
	//TODO: generate passwd
	newUser.passwd = pword;
	current_users.push_back(newUser);

	//TODO: add user to file!!!!

	User * createdUser = getUserByName(uname);
	return createdUser;
}



//CHANNEL CONTROL

void IRCBot::loadChannels()
{

}


bool IRCBot::isOper(Channel chan, User host)
{
	//TODO: check for user's oper status in channel
}






//IRC RELATED FUNCTIONS

void IRCBot::sendRawMsg(string msgOut)
{
	if (sock > 1)
	{
		stringstream s;
		s << msgOut << "\r\n";
		const char *buf = s.str().c_str();
		int sent = send(sock, buf, strlen(buf), 0);
		int remaining = strlen(buf) - sent;
		while (remaining > 0)
		{
			sent = send(sock, buf, strlen(buf) - sent, sent);
			remaining -= sent;
		}
		if (DEBUGMODE) cout << s.str() << endl;
	}
}

void IRCBot::sendPrivMsg(string msgLoc, string msgOut)
{
	if (DEBUGMODE)
	{
		cout << "sock == " << sock << endl;
		cout << "msgLoc: '" << msgLoc << "'" << endl;
		cout << "msgOut: '" << msgOut << "'" << endl;
	}
	if (sock > 1) 
	{
		stringstream s;
		s.clear();
		s << ":source PRIVMSG " << msgLoc << " :" << msgOut;
		sendRawMsg(s.str());
	}
}

void IRCBot::messageLoop()
{
	for (int i = 1; true; i++) {
		char buf[MAXBUFFERSIZE];
		int bytes_in = recv(sock, buf, MAXBUFFERSIZE-1, 0);
		buf[bytes_in] = '\0';
		if (bytes_in < 1) {
			if (DEBUGMODE == 1) cout << "Connection closed by remote host (or unexpectedly)." << endl;
			return;
		}

		if (i < 3) continue;
		if (i == 3) 
		{
			stringstream s;
			s << "NICK " << nick;
			sendRawMsg(s.str());
			s.str(string());
			s << "USER " << nick << " 0 * :" << "Krik Botman";
			sendRawMsg(s.str());
			if (DEBUGMODE == 1) cout << "Sent NICK/USER" << endl;
			continue;
		}

		string msgIn = buf;
		string remote_nick;
		string remote_host;
		string command;
		string paramStr;

		//REMOTE PARTY DETECTION
		if (msgIn.substr(0,1) == ":")
		{
			size_t posBang, posSpc;
			posBang = msgIn.find_first_of('!');
			posSpc = msgIn.find_first_of(' ');
			if (posBang != string::npos && posBang < posSpc)
			{
				remote_nick = msgIn.substr(1, posBang-1);
				remote_host = msgIn.substr(posBang+1, posSpc-posBang-1);
				msgIn = msgIn.substr(posSpc+1);
				posSpc = msgIn.find_first_of(' ');
				command = msgIn.substr(0, posSpc);
				paramStr = msgIn.substr(posSpc+1);

				if (DEBUGMODE == 1)
				{
					cout << "remote_nick: '" << remote_nick << "'" << endl;
					cout << "remote_host: '" << remote_host << "'" << endl;
					cout << "command: '" << command << "'" << endl;
				}
			}
			else
			{
				remote_nick = "";
				remote_host = msgIn.substr(1, posSpc-1);
				msgIn = msgIn.substr(posSpc+1);
				posSpc = msgIn.find_first_of(' ');
				command = msgIn.substr(0, posSpc);
				paramStr = msgIn.substr(posSpc+1);
				if (DEBUGMODE == 1)
				{
					cout << "remote_nick: (N/A - Server) " << endl;
					cout << "remote_host: '" << remote_host << "'" << endl;
					cout << "command: '" << command << "'" << endl;
				}
			}
		} else {
			remote_nick = "";
			remote_host = "";
			command = msgIn.substr(0, msgIn.find_first_of(' '));
			paramStr = msgIn.substr(msgIn.find_first_of(' ')+1);
			if (DEBUGMODE == 1) cout << "No host in message." << endl << "command: '" << command << "'" << endl;
		}
		//strip the endline characters from the message
		paramStr = paramStr.substr(0, strlen(paramStr.c_str())-2);

		//PING RESPONSE
		if (command == "PING")
		{
			stringstream s;
			s << "PONG " << paramStr;
			sendRawMsg(s.str());
			if (DEBUGMODE == 1) cout << "PING/PONG! (strlen: " << strlen(paramStr.c_str()) << ") (" << s.str().c_str() << ")" << endl;
			continue;
		}

		//MESSAGE RECEIVED
		if (command == "PRIVMSG")
		{
			//USER DETECTION
			User* remote_user = getUserByHost(remote_host);

			//COMMAND/PARAMETER SEPERATION
			size_t posSpc = paramStr.find_first_of(' ');
			string msgLoc = paramStr.substr(0, posSpc);
			paramStr = paramStr.substr(posSpc+1);
			posSpc = paramStr.find_first_of(' ');
			command = paramStr.substr(1, posSpc-1);

			string params[MAXMSGPARAMS];
			for (int i = 0; i < MAXMSGPARAMS; i++)
			{
				if (posSpc == string::npos)
				{
					params[i] = "";
					continue;
				}
				paramStr = paramStr.substr(posSpc+1);
				posSpc = paramStr.find_first_of(' ');
				if (i == MAXMSGPARAMS-1)
				{
					params[i] = paramStr.substr(0);
					break;
				}
				params[i] = paramStr.substr(0, posSpc);
			}

			//convert command to upper
			command = strToUpper(command);
			//check if the message is received in a channel
			if (msgLoc[0] == '#')
			{
				//check for a trigger or continue
				if (command[0] == publicTrigger)
					command = command.substr(1);
				else continue;
			} else msgLoc = remote_nick;

			if (DEBUGMODE == 1)
			{
				cout << "MESSAGE RECEIVED" << endl;
				cout << "Return Addr: '" << msgLoc << "'" << endl;
				cout << "Command: '" << command << "'" << endl;
				cout << "Param1: '" << params[0] << "'" << endl;
				cout << "Param2: '" << params[1] << "'" << endl;
				cout << "Param3: '" << params[2] << "'" << endl;
				cout << "Param4: '" << params[3] << "'" << endl;
				cout << "Param5: '" << params[4] << "'" << endl;
			}

			//HANDLE COMMANDS

			//BOT CONTROL

			if (command == "QUIT") 
			{
				if (remote_user != NULL && remote_user->flags & USER_ISMASTER)
				{
					sendRawMsg("QUIT :peace");
					return;
				} else sendPrivMsg(msgLoc, "You are not authorized to use this command!");
				continue;
			}

			if (command == "TEST")
			{
				if (params[0] == "")
				{
					sendPrivMsg(msgLoc, "Test Usage: TEST <A|B|C|D...>");
					continue;
				}
				else
				{
					if (params[0] == "A" || params[0] == "a")
					{
						loadUsers();
						continue;
					}
					if (params[0] == "B" || params[0] == "b")
					{
						sendPrivMsg(msgLoc, "Listing users...");
						stringstream s;
						s << "Users: ";
						for (int i = 0; i < current_users.size(); i++)
						{
							s << current_users.at(i).name;
							if (i != current_users.size()-1) s << ", ";
						}
						sendPrivMsg(msgLoc, s.str());
						continue;
					}
					if (params[0] == "C" || params[0] == "c")
					{
						if (remote_user != NULL)
						{
							if (!(remote_user->flags & USER_ISADMIN))
							{
								remote_user->flags |= USER_ISADMIN;
								sendPrivMsg(remote_nick, "You are now an admin.");
								continue;
							}
							if (!(remote_user->flags & USER_ISMASTER))
							{
								remote_user->flags |= USER_ISMASTER;
								sendPrivMsg(remote_nick, "You are now a master.");
								continue;
							}
							remote_user->flags = 0;
							sendPrivMsg(remote_nick, "You are now a normal user.");
						}
						continue;
					}
				}
			}

			if (command == "JOIN") 
			{
				if (remote_user != NULL && remote_user->flags & USER_ISMASTER)
				{
					stringstream s;
					s << ":source JOIN " << params[0];
					sendRawMsg(s.str());
					sendPrivMsg(msgLoc, "Joined!");
				} else sendPrivMsg(msgLoc, "You are not authorized to use this command!");
				continue;
			}

			if (command == "PART")
			{
				if (remote_user != NULL && remote_user->flags & USER_ISMASTER)
				{
					if (msgLoc[0] == '#')
					{
						stringstream s;
						s << ":source PART ";
						if (params[0] == "")
							s << msgLoc;
						else
							s << params[0];
						s << " :deuces";
						sendRawMsg(s.str());
					}
					else
					{
						if (params[0] != "")
						{
							if (params[0][0] == '#')
							{
								stringstream s;
								s << ":source PART " << params[0] << " :deuces";
								sendRawMsg(s.str());
								sendPrivMsg(msgLoc, "Parted!");
							} else sendPrivMsg(msgLoc, "That's not a channel. I can't part that.");
						}
						else sendPrivMsg(msgLoc, "Invalid syntax! Use: PART <channel>");
					}
				} else sendPrivMsg(msgLoc, "You are not authorized to use this command!");
				continue;
			}

			if (command == "NICK")
			{
				if (remote_user != NULL && remote_user->flags & USER_ISMASTER)
				{
					if (params[0] != "")
					{
						sendPrivMsg(msgLoc, "I'll see what I can do...");
						stringstream s;
						s << ":source NICK " << params[0];
						sendRawMsg(s.str());
					}
					else sendPrivMsg(msgLoc, "Invalid syntax! Use: NICK <new_nick>");
				} else sendPrivMsg(msgLoc, "You are not authorized to use this command!");
				continue;
			}



			// USER COMMANDS

			if (command == "AUTH")
			{
				if (msgLoc[0] != '#')
				{
					if (remote_user == NULL)
					{
						if (params[0] != "" && params[1] != "")
						{
							User* attUser = authUser(params[0], params[1], remote_nick, remote_host);
							if (attUser != NULL)
							{
								sendPrivMsg(msgLoc, "You are now authenticated!");
							} else sendPrivMsg(msgLoc, "Incorrect username or password");
						} else sendPrivMsg(msgLoc, "Invalid syntax");
					} else sendPrivMsg(msgLoc, "You are already authenticated!");
				} else sendPrivMsg(msgLoc, "This command must be used in a private message. Noob.");
				continue;
			}

			if (command == "ME")
			{
				if (remote_user != NULL)
				{
					stringstream s;
					s << "You are authenticated with the username '" << remote_user->name << "'.";
					sendPrivMsg(remote_nick, s.str());
					s.str("");
					if (remote_user->flags & USER_ISMASTER) {
						s << "You are a bot master.";
						sendPrivMsg(remote_nick, s.str());
					}
					else 
					{
						if (remote_user->flags & USER_ISADMIN)
						{
							s << "You are an administrator.";
							sendPrivMsg(remote_nick, s.str());
						}
						else
						{
							s << "You are a normal user.";
							sendPrivMsg(remote_nick, s.str());
						}
					}
				}
				else sendPrivMsg(remote_nick, "You are not a registered user or are not logged in.");
				continue;
			}



			//HELPFUL COMMANDS 

			if (command == "INVITEME")
			{
				if (remote_user != NULL)
				{
					if (params[0] != "")
					{
						string chan = params[0];
						if (chan[0] == '#')
						{
							stringstream s;
							s << ":source INVITE " << remote_nick << " :" << params[0];
							sendRawMsg(s.str());
						}
						else
						{
							sendPrivMsg(msgLoc, "Something is wrong.");
						}
					} else sendPrivMsg(msgLoc, "Invalid syntax! Use: INVITEME <channel>");
				} else sendPrivMsg(msgLoc, "You are not authorized to use this command!");
				continue;
			}

			if (command == "OPME")
			{
				if (remote_user != NULL)
				{
					if (msgLoc[0] == '#')
					{
						stringstream s;
						s << ":source MODE " << msgLoc << " +o :" << remote_nick;
						sendRawMsg(s.str());
					}
					else sendPrivMsg(msgLoc, "This command must be executed in a channel. Noob.");
				} else sendPrivMsg(msgLoc, "You are not authorized to use this command!");
				continue;
			}


			//CHANNEL ADMINISTRATION
			if (command == "KICK")
			{
				if (remote_user != NULL && remote_user->flags & USER_ISADMIN)
				{
					if (msgLoc[0] == '#')
					{
						if (params[0] != "")
						{
							stringstream s;
							s << ":source KICK " << msgLoc << " " << params[0] << " :kicked";
							sendRawMsg(s.str());
						} else sendPrivMsg(msgLoc, "Invalid syntax! Use: KICK <nick>");
					}
					else
					{
						if (params[0][0] == '#' && params[1] != "")
						{
							stringstream s;
							s << ":source KICK " << params[0] << " " << params[1] << " :kicked";
							sendRawMsg(s.str());
							sendPrivMsg(msgLoc, "Kicked!");
						} else sendPrivMsg(msgLoc, "Invalid syntax! Use: KICK <channel> <nick>");
					}
				} else sendPrivMsg(msgLoc, "You are not authorized to use this command!");
				continue;
			}

			if (command == "BOOT")
			{
				if (remote_user != NULL && remote_user->flags & USER_ISADMIN)
				{
					if (msgLoc[0] == '#')
					{
						if (params[0] != "")
						{
							stringstream s;
							s << "Hey " << params[0] << ", what do you put on your feet before you ride a horse?";
							sendPrivMsg(msgLoc, s.str());
							s.str("");
							s << ":source KICK " << msgLoc << " " << params[0] << " :BOOTS!";
							sleep(3);
							sendRawMsg(s.str());
						} else sendPrivMsg(msgLoc, "Invalid syntax! Use: BOOT <nick>");
					}
					else
					{
						if (params[0][0] == '#' && params[1] != "")
						{
							stringstream s;
							s << "Hey " << params[0] << ", what do you put on your feet before you ride a horse?";
							sendPrivMsg(msgLoc, s.str());
							s.str("");
							s << ":source KICK " << params[0] << " " << params[1] << " :BOOTS!";
							sleep(3);
							sendRawMsg(s.str());
							sendPrivMsg(msgLoc, "Kicked!");
						} else sendPrivMsg(msgLoc, "Invalid syntax! Use: BOOT <channel> <nick>");
					}
				} else sendPrivMsg(msgLoc, "You are not authorized to use this command!");
				continue;
			}



			//STUPID THINGS

			if (command == "TL")
			{
				if (params[0] == "")
				{
					sendPrivMsg(msgLoc, "T-Life!");
				}
				else
				{
					sendPrivMsg(msgLoc, "Yea, whatever... T-Life!");
				}
				continue;
			}

			if (command == "BEER")
			{
				sendPrivMsg(msgLoc, "You can't be a real country unless you have a beer and an airline. It helps if you have some kind of a football team, or some nuclear weapons, but at the very least you need a beer. -Frank Zappa");
				continue;
			}

			if (command == "SAY")
			{
				if (remote_user != NULL && remote_user->flags & USER_ISADMIN)
				{
					if (msgLoc[0] == '#')
					{
						if (params[0] != "")
						{
							stringstream s;
							s << params[0] << " " << params[1] << " " << params[2] << " " << params[3] << " " << params[4] << "\r\n";
							sendPrivMsg(msgLoc, s.str());
						}
						else sendPrivMsg(msgLoc, "Invalid syntax! Use: TELL <nick> <message>");
					}
					else
					{
				 		sendPrivMsg(msgLoc, "SAY in private messages is not enabled.");
					}
				} else sendPrivMsg(msgLoc, "You are not authorized to use this command!");
				continue;
			}

		}
	}
}




//EXTERNAL FUNCTIONS

bool IRCBot::start(const char *_host, const char *_port, const char *_nick)
{
	host = _host;
	port = _port;
	nick = _nick;

	struct addrinfo hints;
	struct addrinfo *host_info;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	sock_status = getaddrinfo(host, port, &hints, &host_info);
	if (sock_status) {
		cout << "Error getting host information! Cancelling connect." << endl;
		return false;
	}

	sock = socket(host_info->ai_family, host_info->ai_socktype, host_info->ai_protocol);
	if (sock == -1) {
		cout << "Error creating socket! Cancelling connect." << endl;
		return false;
	}

	sock_status = connect(sock, host_info->ai_addr, host_info->ai_addrlen);
	if (sock_status) {
		cout << "Error connecting with socket! Cancelling connect." << endl;
		return false;
	}

	freeaddrinfo(host_info);

	messageLoop();

	cout << "Closing connection..." << endl;

	close(sock);

}

void IRCBot::stop()
{
	//TODO: stop the bot? probably not relavent unless I thread
}