#include "popclient.h"
#include <iostream>
#include <cstring>
#include <sstream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

using namespace std;

#define BUFFERSIZE 1024

//reasons to connect
#define CONNREASON_CHECKINBOX 0

POPClient::POPClient()
{

}

POPClient::~POPClient()
{
	if (sock != 0) close(sock);
}

//internal functions

bool POPClient::establishConnection()
{
	int sock_status;
	keepAlive = true;
	struct addrinfo addr_hints;
	struct addrinfo *host_info;
	memset(&addr_hints, 0, sizeof addr_hints);
	addr_hints.ai_family = AF_UNSPEC;
	addr_hints.ai_socktype = SOCK_STREAM;
	
	sock_status = getaddrinfo(host, port, &addr_hints, &host_info);
	if (sock_status) {
		cout << "Error getting address information for the supplied host." << endl;
		cout << gai_strerror(sock_status) << endl;
		return false;
	}

	sock = socket(host_info->ai_family, host_info->ai_socktype, host_info->ai_protocol);
	if (sock == -1) {
		cout << "Error creating socket." << endl;
		return false;
	}

	sock_status = connect(sock, host_info->ai_addr, host_info->ai_addrlen);
	if (sock_status == -1) {
		cout << "Error creating connection." << endl;
		cout << "Error code: " << sock_status << endl;
		return false;
	}

	freeaddrinfo(host_info);

	//connection loop
	connectionLoop(CONNREASON_CHECKINBOX);

	cout << "Closing connection..." << endl;

	close(sock);
	return true;

}

void POPClient::connectionLoop(int connReason)
{
	int sock_status;
	switch (connReason) {

		//CONNECT TO CHECK INBOX FOR NUMBER OF MESSAGES
		case 0:
			for (int i = 1; keepAlive; i++) {
				char buf[BUFFERSIZE];
				sock_status = recv(sock, buf, BUFFERSIZE-1, 0);
				if (sock_status > 0) {
					//cout << buf << endl;
				} else cout << "sock_status: " << sock_status << endl;

				string str_msgIn(buf);
				bool recvOK = false;
				if (str_msgIn.substr(0, 3) == "+OK") recvOK = true;

				switch (i) {

					case 1:
						if (recvOK) {
							stringstream s;
							s << "USER " << username << "\r\n";
							const char *msgOut = s.str().c_str();
							sock_status = send(sock, msgOut, strlen(msgOut), 0);
						} else i = 0;
						break;

					case 2:
						if (recvOK) {
							stringstream s;
							s << "PASS " << passwd << "\r\n";
							const char *msgOut = s.str().c_str();
							sock_status = send(sock, msgOut, strlen(msgOut), 0);
						} else {
							cout << "Bad response to USER. Closing connection." << endl;
							const char *msgOut = "QUIT\r\n";
							send(sock, msgOut, strlen(msgOut), 0);
							keepAlive = false;
						}
						break;

					case 3:
						if (recvOK) {
							cout << "Authenticated!" << endl;
							const char *msgOut = "STAT\r\n";
							send(sock, msgOut, strlen(msgOut), 0);
						} else {
							cout << "Bad response to PASS. Incorrect credentials?" << endl;
							const char *msgOut = "QUIT\r\n";
							send(sock, msgOut, strlen(msgOut), 0);
							keepAlive = false;
						}
						break;

					case 4:
						if (recvOK) {
							cout << str_msgIn.substr(4, str_msgIn.substr(4).find_first_of(' ')) << " messages in your inbox." << endl;
						}
						const char *msgOut = "QUIT\r\n";
						send(sock, msgOut, strlen(msgOut), 0);
						keepAlive = false;
						break;

				}
			}
			break;
	}
}


//public functions

void POPClient::setHost(const char* _host, const char* _port)
{
	host = _host;
	port = _port;
}

void POPClient::setCred(string _user, string _pass)
{
	username = _user;
	passwd = _pass;
}

bool POPClient::checkForMail(std::string checkFor)
{
	establishConnection();
}