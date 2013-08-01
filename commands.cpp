/*
 *
 * botcommand.cpp
 *
 */
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <time.h>

#include "ircbot.h"

 using namespace std;

void IRCBot::handle_command(string command, string params[], string msgLoc, string remote_nick, string remote_host, User * remote_user)
{
	if (command == "QUIT") 
			{
				if (remote_user != NULL && remote_user->access_level >= 99)
				{
					if (params[0] != "")
					{
						command = "";
						for (int i = 0; i < MAXMSGPARAMS; i++)
						{
							if (params[i] != "")
								command += ((i > 0) ? " " : "") + params[i];
							else
								break;
						}
						sendRawMsg("QUIT :" + command);
					}
					else sendRawMsg("QUIT :peace");
					return;
				} else sendNoticeMsg(remote_nick, "You are not authorized to use this command!");
				return;
			}


			if (command == "JOIN") 
			{
				if (remote_user != NULL && remote_user->access_level >= 80)
				{
					command = "JOIN " + params[0];
					sendRawMsg(command);
					sendNoticeMsg(remote_nick, "Joined " + params[0]);
				} else sendNoticeMsg(remote_nick, "You are not authorized to use this command!");
				return;
			}

			if (command == "PART")
			{
				if (remote_user != NULL && remote_user->access_level >= 80)
				{
					if (msgLoc[0] == '#')
					{
						command = "PART ";
						if (params[0] == "")
							command += msgLoc;
						else
						{
							if (params[0][0] == '#')
							{
								command += params[0];
								sendNoticeMsg(remote_nick, "Parted " + params[0]);
							}
							else 
							{
								sendNoticeMsg(remote_nick, "Invalid syntax! Use: PART <channel>");
								return;
							}
						}
						command += " :deuces";
						sendRawMsg(command);
					}
					else
					{
						if (params[0] != "")
						{
							if (params[0][0] == '#')
							{
								command = "PART " + params[0] + " :deuces";
								sendRawMsg(command);
								sendNoticeMsg(remote_nick, "Parted " + params[0]);
							} else sendNoticeMsg(remote_nick, "Invalid syntax! Use: PART <channel>");
						}
						else sendNoticeMsg(remote_nick, "Invalid syntax! Use: PART <channel>");
					}
				} else sendNoticeMsg(remote_nick, "You are not authorized to use this command!");
				return;
			}

			if (command == "NICK")
			{
				if (remote_user != NULL && remote_user->access_level >= 100)
				{
					if (params[0] != "")
					{
						command = "NICK " + params[0];
						sendRawMsg(command);
					}
					else sendNoticeMsg(remote_nick, "Invalid syntax! Use: NICK <new_nick>");
				} else sendNoticeMsg(remote_nick, "You are not authorized to use this command!");
				return;
			}

			if (command == "VERSION")
			{
				sendNoticeMsg(remote_nick, "Current Version: " + versionString);
				return;
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
								sendNoticeMsg(remote_nick, "You are now authenticated!");
							} else sendNoticeMsg(remote_nick, "Incorrect username or password");
						} else sendNoticeMsg(remote_nick, "Invalid syntax! Use: AUTH <username> <password>");
					} else sendNoticeMsg(remote_nick, "You are already authenticated!");
				} else sendNoticeMsg(remote_nick, "This command must be used in a private message. Noob.");
				return;
			}

			if (command == "DEAUTH")
			{
				if (remote_user != NULL)
				{
					remote_user->nick = "";
					remote_user->host = "";
					sendNoticeMsg(remote_nick, "You are no longer authenticated.");
				} else sendNoticeMsg(remote_nick, "You are not currently authenticated!");
				return;
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
						else sendNoticeMsg(remote_nick, "Invalid syntax! Use: INVITEME <channel>");
					} else sendNoticeMsg(remote_nick, "Invalid syntax! Use: INVITEME <channel>");
				} else sendNoticeMsg(remote_nick, "You are not authorized to use this command!");
				return;
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
					else sendNoticeMsg(remote_nick, "This command must be executed in a channel. Noob.");
				} else sendNoticeMsg(remote_nick, "You are not authorized to use this command!");
				return;
			}

			if (command == "CHANNELS")
			{
				if (remote_user != NULL && remote_user->access_level >= 80)
				{
					stringstream s;
					s <<"Currently in " << current_channels.size() << " channels";
					sendNoticeMsg(remote_nick, s.str());
					command = "";
					for (int i = current_channels.size()-1; i >= 0; i--)
					{
						command += current_channels.at(i).name;
						command += (i == 0) ? "" : " ";
					}
					sendNoticeMsg(remote_nick, command);
				} else sendNoticeMsg(remote_nick, "You are not authorized to use this command!");
				return;
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
						} else sendNoticeMsg(remote_nick, "Invalid syntax! Use: KICK <nick>");
					}
					else
					{
						if (params[0][0] == '#' && params[1] != "")
						{
							command = ":source KICK " + params[0] + " " + params[1] + " :kicked";
							sendRawMsg(command);
							sendNoticeMsg(remote_nick, "Kicked!");
						} else sendNoticeMsg(remote_nick, "Invalid syntax! Use: KICK <channel> <nick>");
					}
				} else sendNoticeMsg(remote_nick, "You are not authorized to use this command!");
				return;
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
						} else sendNoticeMsg(remote_nick, "Invalid syntax! Use: BOOT <nick>");
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
							sendNoticeMsg(remote_nick, "Kicked!");
						} else sendNoticeMsg(remote_nick, "Invalid syntax! Use: BOOT <channel> <nick>");
					}
				} else sendNoticeMsg(remote_nick, "You are not authorized to use this command!");
				return;
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
						} else sendNoticeMsg(remote_nick, "Invalid syntax! Use: OP <nick>");
					} else sendNoticeMsg(remote_nick, "This command must be used in a channel!");
				} else sendNoticeMsg(remote_nick, "You are not authorized to use this command!");
				return;
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
						} else sendNoticeMsg(remote_nick, "Invalid syntax! Use: DEOP <nick>");
					} else sendNoticeMsg(remote_nick, "This command must be used in a channel!");
				} else sendNoticeMsg(remote_nick, "You are not authorized to use this command!");
				return;
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
								sendNoticeMsg(remote_nick, command);
								stringstream s;
								s << "You have an access level of " << remote_user->access_level;
								sendNoticeMsg(remote_nick, s.str());
							} else sendNoticeMsg(remote_nick, "You are not a registered user or are not logged in.");
						}
						else
						{
							User* u = getUserByName(params[1]);
							if (u != NULL)
							{
								stringstream s;
								s << u->name << ((u->nick != "") ? " (authd as " + u->nick + ") " : " (not authd) ") << "has an access level of " << u->access_level;
								sendNoticeMsg(remote_nick, s.str());
							} else sendNoticeMsg(remote_nick, "The user you specified does not exist!");
						}
						return;
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
														sendNoticeMsg(remote_nick, "The user " + u->name + " now has an access level of " + params[3]);
													} else sendNoticeMsg(remote_nick, "You cannot give a user higher access than your current level!");
												} else sendNoticeMsg(remote_nick, "You must set a valid access level! (1-99)");
											} else sendNoticeMsg(remote_nick, "The user you specified does not exist!");
										} else sendNoticeMsg(remote_nick, "Invalid syntax! Use: USER SET ACCESS <nick> <level>");
									} else sendNoticeMsg(remote_nick, "You are not authorized to use this command!");
									return;
								}
								//USER SET PASSWORD
								if (strToUpper(params[1]) == "PASSWORD")
								{
									if (params[2] != "")
										{
											remote_user->passwd = params[2];
											saveUsers();
											sendNoticeMsg(remote_nick, "Your password has been updated.");
										} else sendNoticeMsg(remote_nick, "Invalid syntax! Use: USER SET PASSWORD <newpassword>");
									return;
								}
								//USER SET 
								sendNoticeMsg(remote_nick, "Invalid syntax! Use: USER SET <ACCESS|PASSWORD>");
							} else sendNoticeMsg(remote_nick, "Invalid syntax! Use: USER SET <ACCESS|PASSWORD>");
						} else sendNoticeMsg(remote_nick, "You are not authorized to use this command!");
						return;
					}
					//USER ADD
					if (strToUpper(params[0]) == "ADD")
					{
						if (remote_user != NULL && remote_user->access_level >= 80)
						{
							if (params[1] != "" && params[2] != "")
							{
								//TODO: check for valid username (won't break things)
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
										sendNoticeMsg(remote_nick, "User have been created. Please have user auth with: AUTH " + params[1] + " " + s.str());
									} else sendNoticeMsg(remote_nick, "You must set a valid access level! (1-99)");
								} else sendNoticeMsg(remote_nick, "The username specified already exists!");
							} else sendNoticeMsg(remote_nick, "Invalid syntax! Use: USER ADD <username> <access>");
						} else sendNoticeMsg(remote_nick, "You are not authorized to use this command!");
						return;
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
									sendNoticeMsg(remote_nick, "The user has been removed.");
								} else sendNoticeMsg(remote_nick, "The user you specified doesn't exist!");
							} else sendNoticeMsg(remote_nick, "Invalid syntax! Use: USER REMOVE <username>");
						} else sendNoticeMsg(remote_nick, "You are not authorized to use this command!");
						return;
					}
					//USER LIST
					if (strToUpper(params[0]) == "LIST")
					{
						if (remote_user != NULL && remote_user->access_level >= 80)
						{
							stringstream s;
							s << current_users.size() << " users";
							sendNoticeMsg(remote_nick, s.str());
							command = "";
							for (int i = current_users.size()-1; i >= 0; i--)
							{
								command += current_users.at(i).name;
								command += (i == 0) ? "" : " ";
							}
							sendNoticeMsg(remote_nick, command);
						} else sendNoticeMsg(remote_nick, "You are not authorized to use this command!");
						return;
					}
					//nothing hit
					sendNoticeMsg(remote_nick, "Invalid syntax! Use: USER <INFO|SET|ADD|REMOVE|LIST>");
				} else sendNoticeMsg(remote_nick, "Invalid syntax! Use: USER <INFO|SET|ADD|REMOVE|LIST>");
				//nothing hit
				return;
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
				return;
			}

			if (command == "BEER")
			{
				command = getRandomLine("beer");
				if (command != "")
					sendPrivMsg(msgLoc, command);
				else
					sendNoticeMsg(remote_nick, "There are no beer quotes!");
				return;
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
						else sendNoticeMsg(remote_nick, "Invalid syntax! Use: SAY <nick> <message>");
					}
					else
					{
				 		if (params[0] != "" && params[1] != "")
				 		{
				 			if (params[0][0] == '#')
				 			{
				 				command = params[1] + " " + params[2] + " " + params[3] + " " + params[4];
								sendPrivMsg(params[0], command);
				 			} else sendNoticeMsg(remote_nick, "Invalid syntax! Use: SAY <channel> <message>");
				 		} else sendNoticeMsg(remote_nick, "Invalid syntax! Use: SAY <channel> <message>");
					}
				} else sendNoticeMsg(remote_nick, "You are not authorized to use this command!");
				return;
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
							sendNoticeMsg(remote_nick, "Reason added!");
						} else sendNoticeMsg(remote_nick, "Couldn't open T-Hate file!");
					} else sendNoticeMsg(remote_nick, "Invalid syntax! Use: NEWREASON <reason>");
				} else sendNoticeMsg(remote_nick, "You are not authorized to use this command!");
			}

			if (command == "GETREASON" && remote_user->access_level >= 50)
			{
				if (remote_user != NULL)
				{
					command = getRandomLine("t-hate");
					if (command != "")
						sendPrivMsg(msgLoc, command);
					else sendNoticeMsg(remote_nick, "Uh oh... Error");
				} else sendNoticeMsg(remote_nick, "You are not authorized to use this command!");
			}
}