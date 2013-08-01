//CHANNEL CONTROL

#include "ircbot.h"

using namespace std;

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