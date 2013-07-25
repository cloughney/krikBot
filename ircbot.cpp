#include "ircbot.h"
#include <iostream>
#include <fstream>
#include <cstring>
#include <cstddef>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <vector>
#include <unistd.h>
#include "ircuser.h"
#include <stdlib.h>
#include <time.h>
#include <sstream>

using namespace std;

#define DEBUGMODE 1

#define MAXBUFFERSIZE 1024
#define MAXMSGPARAMS 5

IRCBot::IRCBot()
{
	versionString = "0.1.002 25Jul13"; // VERSION
	publicTrigger = '!';
	keepAlive = true;
}

IRCBot::~IRCBot()
{
	if (sock > 1) close(sock);
}



//UTILITY FUNCTIONS

string IRCBot::strToUpper(string strIn)
{
	string output = "";
	for (int i = 0; i < strIn.length(); i++)
	{
		char c = strIn[i];
		if (c > 96 && c < 123)
		{
			c -= 32;
		}
		output += c;
	}
	return output;
}

string IRCBot::getRandomLine(string file)
{
	ifstream f_in(file.c_str());
	if (f_in.is_open())
	{
		vector<string> lines;
		lines.clear();
		int count = 0;
		while (f_in.good())
		{
			string lineIn;
			getline(f_in, lineIn);
			if (lineIn != "")
			{
				lines.push_back(lineIn);
				count++;
			}
		}
		f_in.close();
		srand(time(NULL));
		if (DEBUGMODE) cout << "Reading Random Line" << endl << "Count: " << count << endl;
		count = rand() % count;
		if (DEBUGMODE) cout << "Rand: " << count << endl;
		return lines.at(count);
	} else return "";
}

int IRCBot::getIntFromStr(string strIn)
{
	int output = 0;
	for (int i = 0; i < strIn.length(); i++)
	{
		if (strIn[i] >= 48 && strIn[i] <= 57)
		{
			int tmp = (strIn[i] - 48);
			for (int x = 1; x < strIn.length()-i; x++)
				tmp *= 10;
			output += tmp;
		}
		else
			return 0; //non-number character
	}
	return output;
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
			int access;

			short comma = lineIn.find_first_of(',');
			if (comma == string::npos) continue; //break if the line is invalid
			uname = lineIn.substr(0, comma);
			lineIn = lineIn.substr(comma+1);

			comma = lineIn.find_first_of(',');
			if (comma == string::npos) continue; //break if the line is invalid
			pword = lineIn.substr(0, comma);
			lineIn = lineIn.substr(comma+1);

			access = getIntFromStr(lineIn.substr(0, comma));

			addUser(uname, pword, access); //TODO: mem-leak potential?
		}
		f_users_in.close();
	}
	else
	{
		ofstream f_users_out("ircusers");
		if (f_users_out.is_open())
		{
			f_users_out << "# FILE USAGE: username,password,access_level" << endl;
			f_users_out.close();
		}
		else cout << "Unable to write to ircusers" << endl;
	}

	if (DEBUGMODE)
	{
		cout << "Users Loaded! - Listing users:" << endl;
		for (int i = 0; i < current_users.size(); i++)
			cout << current_users.at(i).name << "|" << current_users.at(i).passwd << "|" << current_users.at(i).access_level << endl;
	}
}

void IRCBot::saveUsers()
{
	ofstream f_users_out("ircusers");
	if (f_users_out.is_open())
	{
		f_users_out << "# FILE USAGE: username,password,access_level" << endl;
		for (int i = 0; i < current_users.size(); i++)
			f_users_out << current_users.at(i).name << "," << current_users.at(i).passwd << "," << current_users.at(i).access_level << endl;
		f_users_out.close();
	}
	else cout << "Unable to write to ircusers" << endl;
}

User* IRCBot::getUserByName(string n)
{
	for (int i = 0; i < current_users.size(); i++)
	{
		if (strToUpper(current_users.at(i).name) == strToUpper(n)) return &current_users.at(i);
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
			return &current_users.at(i);
		}
	}
	return NULL;
}

User* IRCBot::addUser(string uname, string pword, int access_level)
{
	if (getUserByName(uname) != NULL) return NULL;
	User newUser;
	newUser.name = uname;
	//TODO: generate passwd
	newUser.passwd = pword;
	newUser.access_level = access_level;
	current_users.push_back(newUser);

	//TODO: add user to file!!!!

	User * createdUser = getUserByName(uname);
	return createdUser;
}



//CHANNEL CONTROL

void IRCBot::loadChannels()
{

}

void IRCBot::addChannel(Channel c)
{
	if (getChanByName(c.name) == NULL)
	{
		current_channels.push_back(c);
	}
}

void IRCBot::removeChannel(Channel c)
{

}

void IRCBot::removeChannel(string c)
{

}

Channel* IRCBot::getChanByName(string c)
{
	for (int i = current_channels.size()-1; i >= 0; i--)
		if (current_channels.at(i).name == c) return &current_channels.at(i);
	return NULL;
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
		msgOut += "\r\n";
		int sent = send(sock, msgOut.c_str(), msgOut.length(), 0);
		int remaining = msgOut.length() - sent;
		while (remaining > 0)
		{
			sent += send(sock, msgOut.c_str(), remaining, sent);
			remaining -= sent;
		}
		if (DEBUGMODE) cout << msgOut << endl;
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
		msgOut = "PRIVMSG " + msgLoc + " :" + msgOut;
		sendRawMsg(msgOut);
	}
}

void IRCBot::messageLoop()
{
	loadUsers();

	string lastCmdOut = "";

	for (int i = 1; keepAlive; i++) {
		char buf[MAXBUFFERSIZE];
		int bytes_in = recv(sock, buf, MAXBUFFERSIZE-1, 0);
		buf[bytes_in] = '\0';
		if (bytes_in < 1) {
			if (DEBUGMODE) cout << "Connection closed by remote host (or unexpectedly)." << endl;
			return;
		}

		if (i < 3) continue;
		if (i == 3) 
		{
			string msg;
			msg = "NICK ";
			msg += nick;
			sendRawMsg(msg);
			msg = "USER ";
			msg += nick;
			msg += " 0 * :Krik Botman";
			sendRawMsg(msg);
			if (DEBUGMODE) cout << "Sent NICK/USER" << endl;
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

				if (DEBUGMODE)
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
				if (DEBUGMODE)
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
			if (DEBUGMODE) cout << "No host in message." << endl << "command: '" << command << "'" << endl;
		}
		//strip the endline characters from the message
		paramStr = paramStr.substr(0, strlen(paramStr.c_str())-2);



		//PING RESPONSE
		if (command == "PING")
		{
			command = "PONG " + paramStr;
			sendRawMsg(command);
			if (DEBUGMODE) cout << "PING/PONG! (" << command << ")" << endl;
			continue;
		}


		//CHANNEL JOIN MESSAGE
		if (command == "JOIN")
		{
			if (remote_nick == nick)
			{
				if (DEBUGMODE) cout << "JOINED CHANNEL!" << endl << "'" << paramStr << "'" << endl;
				//Channel c;
				//c.name = paramStr;
				//current_channels.push_back();
			}
			continue;
		}

		//CHANNEL PART MESSAGE
		if (command == "PART")
		{
			if (remote_nick == nick)
			{
				if (DEBUGMODE) cout << "PARTED CHANNEL!" << endl << "'" << paramStr << "'" << endl;
				//current_channels.push_back(paramStr);
			}
			continue;
		}

		//CHANNEL KICK MESSAGE
		if (command == "KICK")
		{
			//if (remote_nick == nick)
			//{
				if (DEBUGMODE) cout << "KICKED FROM CHANNEL!" << endl << "'" << paramStr << "'" << endl;
				//current_channels.push_back(paramStr);
			//}
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
				if (remote_user != NULL && remote_user->access_level >= 100)
				{
					sendRawMsg("QUIT :peace");
					return;
				} else sendPrivMsg(msgLoc, "You are not authorized to use this command!");
				continue;
			}


			if (command == "JOIN") 
			{
				if (remote_user != NULL && remote_user->access_level >= 80)
				{
					command = "JOIN " + params[0];
					sendRawMsg(command);
				} else sendPrivMsg(msgLoc, "You are not authorized to use this command!");
				continue;
			}

			if (command == "PART")
			{
				if (remote_user != NULL && remote_user->access_level >= 80)
				{
					if (msgLoc[0] == '#')
					{
						command = ":source PART ";
						if (params[0] == "")
							command += msgLoc;
						else
							command += params[0];
						command += " :deuces";
						sendRawMsg(command);
					}
					else
					{
						if (params[0] != "")
						{
							if (params[0][0] == '#')
							{
								command = ":source PART " + params[0] + " :deuces";
								sendRawMsg(command);
							} else sendPrivMsg(msgLoc, "Invalid syntax! Use: PART <channel>");
						}
						else sendPrivMsg(msgLoc, "Invalid syntax! Use: PART <channel>");
					}
				} else sendPrivMsg(msgLoc, "You are not authorized to use this command!");
				continue;
			}

			if (command == "NICK")
			{
				if (remote_user != NULL && remote_user->access_level >= 100)
				{
					if (params[0] != "")
					{
						command = ":source NICK " + params[0];
						sendRawMsg(command);
						lastCmdOut = "NICK";
					}
					else sendPrivMsg(msgLoc, "Invalid syntax! Use: NICK <new_nick>");
				} else sendPrivMsg(msgLoc, "You are not authorized to use this command!");
				continue;
			}

			if (command == "VERSION")
			{
				sendPrivMsg(msgLoc, "Current Version: " + versionString);
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
						} else sendPrivMsg(msgLoc, "Invalid syntax! Use: AUTH <username> <password>");
					} else sendPrivMsg(msgLoc, "You are already authenticated!");
				} else sendPrivMsg(msgLoc, "This command must be used in a private message. Noob.");
				continue;
			}

			if (command == "DEAUTH")
			{
				if (remote_user != NULL)
				{
					remote_user->nick = "";
					remote_user->host = "";
					sendPrivMsg(msgLoc, "You are no longer authenticated.");
				} else sendPrivMsg(msgLoc, "You are not currently authenticated!");
				continue;
			}



			//HELPFUL COMMANDS 

			if (command == "INVITEME")
			{
				if (remote_user != NULL && remote_user->access_level >= 10)
				{
					if (params[0] != "")
					{
						if (params[0][0] == '#')
						{
							command = ":source INVITE " + remote_nick + " :" + params[0];
							sendRawMsg(command);
						}
						else sendPrivMsg(msgLoc, "Invalid syntax! Use: INVITEME <channel>");
					} else sendPrivMsg(msgLoc, "Invalid syntax! Use: INVITEME <channel>");
				} else sendPrivMsg(msgLoc, "You are not authorized to use this command!");
				continue;
			}

			if (command == "OPME")
			{
				if (remote_user != NULL && remote_user->access_level >= 10)
				{
					if (msgLoc[0] == '#')
					{
						command = ":source MODE " + msgLoc + " +o :" + remote_nick;
						sendRawMsg(command);
					}
					else sendPrivMsg(msgLoc, "This command must be executed in a channel. Noob.");
				} else sendPrivMsg(msgLoc, "You are not authorized to use this command!");
				continue;
			}

			if (command == "CHANNELS")
			{
				if (remote_user != NULL && remote_user->access_level >= 80)
				{
					sendPrivMsg(remote_nick, "Currently in the following channels:");
					command = "";
					for (int i = current_channels.size()-1; i >= 0; i--)
					{
						command += current_channels.at(i).name;
						command += (i == 0) ? "" : " ";
					}
					sendPrivMsg(remote_nick, command);
				} else sendPrivMsg(msgLoc, "You are not authorized to use this command!");
				continue;
			}


			//CHANNEL ADMINISTRATION
			if (command == "KICK")
			{
				if (remote_user != NULL && remote_user->access_level >= 10)
				{
					if (msgLoc[0] == '#')
					{
						if (params[0] != "")
						{
							command = ":source KICK " + msgLoc + " " + params[0] + " :kicked";
							sendRawMsg(command);
						} else sendPrivMsg(msgLoc, "Invalid syntax! Use: KICK <nick>");
					}
					else
					{
						if (params[0][0] == '#' && params[1] != "")
						{
							command = ":source KICK " + params[0] + " " + params[1] + " :kicked";
							sendRawMsg(command);
							sendPrivMsg(msgLoc, "Kicked!");
						} else sendPrivMsg(msgLoc, "Invalid syntax! Use: KICK <channel> <nick>");
					}
				} else sendPrivMsg(msgLoc, "You are not authorized to use this command!");
				continue;
			}

			if (command == "BOOT")
			{
				if (remote_user != NULL && remote_user->access_level >= 80)
				{
					if (msgLoc[0] == '#')
					{
						if (params[0] != "")
						{
							command = "Hey " + params[0] + ", what do you put on your feet before you ride a horse?";
							sendPrivMsg(msgLoc, command);
							command = ":source KICK " + msgLoc + " " + params[0] + " :BOOTS!";
							sleep(3);
							sendRawMsg(command);
						} else sendPrivMsg(msgLoc, "Invalid syntax! Use: BOOT <nick>");
					}
					else
					{
						if (params[0][0] == '#' && params[1] != "")
						{
							command = "Hey " + params[0] + ", what do you put on your feet before you ride a horse?";
							sendPrivMsg(msgLoc, command);
							command = ":source KICK " + params[0] + " " + params[1] + " :BOOTS!";
							sleep(3);
							sendRawMsg(command);
							sendPrivMsg(msgLoc, "Kicked!");
						} else sendPrivMsg(msgLoc, "Invalid syntax! Use: BOOT <channel> <nick>");
					}
				} else sendPrivMsg(msgLoc, "You are not authorized to use this command!");
				continue;
			}

			if (command == "OP")
			{
				if (remote_user != NULL && remote_user->access_level >= 10)
				{
					if (msgLoc[0] == '#')
					{
						if (params[0] != "")
						{
							command = "MODE ";
							command += msgLoc + " +o";
							command += " " + params[0];
							sendRawMsg(command);
						} else sendPrivMsg(msgLoc, "Invalid syntax! Use: OP <nick>");
					} else sendPrivMsg(msgLoc, "This command must be used in a channel!");
				} else sendPrivMsg(msgLoc, "You are not authorized to use this command!");
				continue;
			}

			if (command == "DEOP")
			{
				if (remote_user != NULL && remote_user->access_level >= 10)
				{
					if (msgLoc[0] == '#')
					{
						if (params[0] != "")
						{
							command = "MODE ";
							command += msgLoc + " -o";
							command += " " + params[0];
							sendRawMsg(command);
						} else sendPrivMsg(msgLoc, "Invalid syntax! Use: DEOP <nick>");
					} else sendPrivMsg(msgLoc, "This command must be used in a channel!");
				} else sendPrivMsg(msgLoc, "You are not authorized to use this command!");
				continue;
			}


			
			//USER ADMINISTRATION
			if (command == "USER")
			{
				if (params[0] != "")
				{
					if (strToUpper(params[0]) == "INFO")
					{
						//SELF INFORMATION
						if (params[1] == "")
						{
							if (remote_user != NULL)
							{
								command = "You are authenticated with the username '" + remote_user->name + "'.";
								sendPrivMsg(remote_nick, command);
								stringstream s;
								s << "You have an access level of " << remote_user->access_level;
								sendPrivMsg(remote_nick, s.str());
							} else sendPrivMsg(remote_nick, "You are not a registered user or are not logged in.");
						}
						else
						{
							User* u = getUserByName(params[1]);
							if (u != NULL)
							{
								stringstream s;
								s << u->name << " has an access level of " << u->access_level;
								sendPrivMsg(remote_nick, s.str());
							} else sendPrivMsg(remote_nick, "The user you specified does not exist!");
						}
						continue;
					}
					if (strToUpper(params[0]) == "SET")
					{
						if (remote_user != NULL)
						{
							if (params[1] != "")
							{
								//USER SET ACCESS
								if (strToUpper(params[1]) == "ACCESS")
								{
									if (remote_user->access_level >= 80)
									{
										if (params[2] != "" && params[3] != "")
										{
											User* u = getUserByName(params[2]);
											if (u != NULL)
											{
												int new_level = getIntFromStr(params[3]);
												if (new_level > 0 && new_level < 100)
												{
													if (remote_user->access_level >= new_level)
													{
														u->access_level = new_level;
														saveUsers();
														sendPrivMsg(remote_nick, "The user " + u->name + " now has an access level of " + params[3]);
													} else sendPrivMsg(remote_nick, "You cannot give a user higher access than your current level!");
												} else sendPrivMsg(remote_nick, "You must set a valid access level! (1-99)");
											} else sendPrivMsg(remote_nick, "The user you specified does not exist!");
										} else sendPrivMsg(remote_nick, "Invalid syntax! Use: USER SET ACCESS <nick> <level>");
									} else sendPrivMsg(remote_nick, "You are not authorized to use this command!");
									continue;
								}
								//USER SET PASSWORD
								if (strToUpper(params[1]) == "PASSWORD")
								{
									if (params[2] != "")
										{
											remote_user->passwd = params[2];
											saveUsers();
											sendPrivMsg(remote_nick, "Your password has been updated.");
										} else sendPrivMsg(remote_nick, "Invalid syntax! Use: USER SET PASSWORD <newpassword>");
									continue;
								}
								//USER SET 
								sendPrivMsg(remote_nick, "Invalid syntax! Use: USER SET <ACCESS|PASSWORD>");
							} else sendPrivMsg(remote_nick, "Invalid syntax! Use: USER SET <ACCESS|PASSWORD>");
						} else sendPrivMsg(remote_nick, "You are not authorized to use this command!");
						continue;
					}
					//USER ADD
					if (strToUpper(params[0]) == "ADD")
					{
						if (remote_user != NULL && remote_user->access_level >= 80)
						{
							if (params[1] != "" && params[2] != "")
							{
								User* u = getUserByName(params[1]);
								if (u == NULL)
								{
									int access = getIntFromStr(params[2]);
									if (access > 0 && access < 100)
									{
										stringstream s;
										s << "temp" << rand();
										addUser(params[1], s.str(), access);
										saveUsers();
										sendPrivMsg(remote_nick, "User have been created. Please have user auth with: AUTH " + params[1] + " " + s.str());
									} else sendPrivMsg(remote_nick, "You must set a valid access level! (1-99)");
								} else sendPrivMsg(remote_nick, "The username specified already exists!");
							} else sendPrivMsg(remote_nick, "Invalid syntax! Use: USER ADD <username> <access>");
						} else sendPrivMsg(remote_nick, "You are not authorized to use this command!");
						continue;
					}
					//USER REMOVE
					if (strToUpper(params[0]) == "REMOVE")
					{
						if (remote_user != NULL && remote_user->access_level >= 80)
						{
							if (params[1] != "")
							{
								User* u = getUserByName(params[1]);
								if (u != NULL)
								{
									int i = 0;
									for (; i < current_users.size(); i++)
										if (current_users.at(i).name == u->name) break;
									current_users.erase(current_users.begin()+i);
									saveUsers();
									sendPrivMsg(remote_nick, "The user has been removed.");
								} else sendPrivMsg(remote_nick, "The user you specified doesn't exist!");
							} else sendPrivMsg(remote_nick, "Invalid syntax! Use: USER REMOVE <username>");
						} else sendPrivMsg(remote_nick, "You are not authorized to use this command!");
						continue;
					}
					//USER LIST
					if (strToUpper(params[0]) == "LIST")
					{
						if (remote_user != NULL && remote_user->access_level >= 25)
						{
							sendPrivMsg(remote_nick, "This command has not been implemented yet!");
						} else sendPrivMsg(remote_nick, "You are not authorized to use this command!");
						continue;
					}
					//nothing hit
					sendPrivMsg(remote_nick, "Invalid syntax! Use: USER <INFO|SET|ADD|REMOVE|LIST>");
				} else sendPrivMsg(remote_nick, "Invalid syntax! Use: USER <INFO|SET|ADD|REMOVE|LIST>");
				//nothing hit
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
				if (remote_user != NULL && remote_user->access_level >= 80)
				{
					if (msgLoc[0] == '#')
					{
						if (params[0] != "")
						{
							command = params[0] + " " + params[1] + " " + params[2] + " " + params[3] + " " + params[4];
							sendPrivMsg(msgLoc, command);
						}
						else sendPrivMsg(msgLoc, "Invalid syntax! Use: SAY <nick> <message>");
					}
					else
					{
				 		if (params[0] != "" && params[1] != "")
				 		{
				 			if (params[0][0] == '#')
				 			{
				 				command = params[1] + " " + params[2] + " " + params[3] + " " + params[4];
								sendPrivMsg(params[0], command);
				 			} else sendPrivMsg(msgLoc, "Invalid syntax! Use: SAY <channel> <message>");
				 		} else sendPrivMsg(msgLoc, "Invalid syntax! Use: SAY <channel> <message>");
					}
				} else sendPrivMsg(msgLoc, "You are not authorized to use this command!");
				continue;
			}

			if (command == "NEWREASON")
			{
				if (remote_user != NULL && remote_user->access_level >= 50)
				{
					if (params[0] != "")
					{
						ofstream f_thate_out("t-hate", ios::app | ios::ate);
						if (f_thate_out.is_open())
						{
							f_thate_out << params[0] << " " << params[1] << " " << params[2] << " " << params[3] << " " << params[4] << endl;
							f_thate_out.close();
							sendPrivMsg(msgLoc, "Reason added!");
						} else sendPrivMsg(msgLoc, "Couldn't open T-Hate file!");
					} else sendPrivMsg(msgLoc, "Invalid syntax! Use: NEWREASON <reason>");
				} else sendPrivMsg(msgLoc, "You are not authorized to use this command!");
			}

			if (command == "GETREASON" && remote_user->access_level >= 50)
			{
				if (remote_user != NULL)
				{
					command = getRandomLine("t-hate");
					if (command != "")
						sendPrivMsg(msgLoc, command);
					else sendPrivMsg(msgLoc, "Uh oh... Error");
				} else sendPrivMsg(msgLoc, "You are not authorized to use this command!");
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

	//tMessageLoop(messageLoop);
	//tMessageLoop.join();
	messageLoop();

	cout << "Closing connection..." << endl;

	close(sock);

	return true;
}

void IRCBot::stop()
{
	keepAlive = false;
}