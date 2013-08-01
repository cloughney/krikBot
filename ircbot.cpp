#include "ircbot.h"

using namespace std;

IRCBot::IRCBot()
{
	versionString = "0.1.007 31Jul13"; // VERSION
	publicTrigger = '!';
	keepAlive = true;
}

IRCBot::~IRCBot()
{
	if (sock > 1) close(sock);
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

void IRCBot::sendNoticeMsg(string msgLoc, string msgOut)
{
	if (DEBUGMODE)
	{
		cout << "sock == " << sock << endl;
		cout << "msgLoc: '" << msgLoc << "'" << endl;
		cout << "msgOut: '" << msgOut << "'" << endl;
	}
	if (sock > 1) 
	{
		msgOut = "NOTICE " + msgLoc + " :" + msgOut;
		sendRawMsg(msgOut);
	}
}

void IRCBot::messageLoop()
{
	loadUsers();

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
			//maintain channel list when joining
			if (remote_nick == nick)
			{
				if (DEBUGMODE) cout << "JOINED CHANNEL!" << endl << "'" << paramStr << "'" << endl;
				Channel c;
				c.name = paramStr;
				current_channels.push_back(c);
			}
			continue;
		}

		//CHANNEL PART MESSAGE
		if (command == "PART")
		{
			//maintain channel list when parting a channel
			if (remote_nick == nick)
			{
				if (DEBUGMODE) cout << "PARTED CHANNEL!" << endl << "'" << paramStr << "'" << endl;
				int i = 0;
				for (; i < current_channels.size(); i++)
					if (current_channels.at(i).name == paramStr) break;
				current_channels.erase(current_channels.begin()+i);
			}
			continue;
		}

		//CHANNEL KICK MESSAGE
		if (command == "KICK")
		{
			size_t pos = paramStr.find_first_of(' ');
			string kicked_chan = paramStr.substr(0, pos);
			paramStr = paramStr.substr(pos+1);
			string kicked_nick = paramStr.substr(0, paramStr.find_first_of(' '));
			if (kicked_nick == nick)
			{
				if (DEBUGMODE) cout << "KICKED FROM CHANNEL!" << endl << "'" << kicked_chan << "'" << endl << "'" << kicked_nick << "'" << endl;
				int i = 0;
				for (; i < current_channels.size(); i++)
					if (current_channels.at(i).name == kicked_chan) break;
				current_channels.erase(current_channels.begin()+i);
			}
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

			//HANDLE COMMANDSs
			handle_command(command, params, msgLoc, remote_nick, remote_host, remote_user);

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