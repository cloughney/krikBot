/*
*
*
*
*/

#include "ircbot.h"

using namespace std;

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