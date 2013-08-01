//USER CONTROL

#include "ircbot.h"

using namespace std;

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