#include "ircbot.h"
#include <iostream>
#include <fstream>

using namespace std;

void readSettings(string &irc_server, string &irc_port, string &irc_nick, string &pwd_master, string &pwd_op)
{
	//create ircbot.conf or read current settings
	ifstream f_conf_in("ircbot.conf");
	if (f_conf_in.is_open())
	{
		while (f_conf_in.good())
		{
			string lineIn;
			getline(f_conf_in, lineIn);
			short posEq = lineIn.find_first_of('=');
			string key = lineIn.substr(0, posEq);
			string val = (posEq == string::npos) ? "" : lineIn.substr(posEq+1);
			if (key == "server")
			{
				irc_server = val.c_str();
				continue;
			}
			if (key == "port")
			{
				irc_port = val.c_str();
				continue;
			}
			if (key == "nick")
			{
				irc_nick = val.c_str();
				continue;
			}
			if (key == "pwd_master")
			{
				pwd_master = val;
				continue;
			}
			if (key == "pwd_operator")
			{
				pwd_op = val;
				continue;
			}
		}
		f_conf_in.close();
	}
	else
	{
		ofstream f_conf_out("ircbot.conf");
		if (f_conf_out.is_open())
		{
			f_conf_out << "server=" << endl;
			f_conf_out << "port=" << endl;
			f_conf_out << "nick=" << endl;
			f_conf_out << "pwd_master=" << endl;
			f_conf_out << "pwd_operator=" << endl;
			f_conf_out.close();
		}
		else cout << "Unable to write to ircbot.conf!" << endl;
	}
}

int main() {

	//POPClient pop;
	//pop.setHost("mail.tmetrics.com", "110");
	//pop.setCred("cloughney", "*****");

	//pop.checkForMail("bleh");

	string irc_server = "";
	string irc_port = "";
	string irc_nick = "";
	string pwd_master = "";
	string pwd_op = "";

	readSettings(irc_server, irc_port, irc_nick, pwd_master, pwd_op);
	

	if (irc_server == "" || irc_port == "" || irc_nick == "")
	{
		cout << "No connection information! Closing..." << endl;
		return 1;
	}
	else
	{
		if (pwd_master == "" || pwd_op == "")
		{
			cout << "No master/operator password set! Closing..." << endl;
			return 1;
		}
		cout << "(" << irc_server << ", " << irc_port << ", " << irc_nick << ")" << endl;
	}

	IRCBot bot;
	bot.start(irc_server.c_str(), irc_port.c_str(), irc_nick.c_str(), pwd_master, pwd_op);
	


	return 0;
}