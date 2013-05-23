/**
 * @file client.cpp
 * @author: Jan Dusek <xdusek17@stud.fit.vutbr.cz>
 *
 * Vypis obsahu adresare
 * Projekt c. 2 do predmetu IPK
 * fce main klienta
 */

#include "socket.h"
#include "exceptions.h"

#include <iostream>
#include <sstream>

const char *USAGE = "Usage: client HOST:PORT PATH";

using namespace std;

void parseWebaddr(const char* str, string& host, string& port) {
	istringstream iss(str);
	getline(iss, host, ':');
	getline(iss, port, '\0');
}

void printListDirectory(const char* path, const string& hostname, const string& service) {
	if (*path == '\0')
		throw runtime_error("Path is empty");

	SocketStream socket;
	Resolver resolver;

	// resolve hostname
	Resolver::Iterator endPointIter = resolver.resolve(hostname, service);
	Resolver::Iterator end;

	// try to connect to all endpoint returned from resolver. first connected wins
	int error = 1;
	while (error != 0 && endPointIter != end) {
		socket.close();
		socket.connect(*endPointIter++, error);
	}
	if (error != 0)
		throw system_error("Failed connecting to host", error);

	socket << "LD/1.0 " << path << "\n" << flush;	// send request

	// parse status line and check magic word
	string magicWord, statusCode, statusMsg;
	socket >> magicWord >> statusCode;
	if (magicWord != "LD/1.0")
		throw runtime_error("Invalid response status line");
	getline(socket, statusMsg);

	if (statusCode != "0")
		throw runtime_error("Error: " + statusMsg);

	// rest of response is dir content list which we will print to stdout
	string line;
	while (getline(socket, line).good())
		cout << line << endl;
}

int main(int argc, char **argv) {
	if (argc != 3) {
		cerr << USAGE << endl;
		return 1;
	}

	string host, port;
	parseWebaddr(argv[1], host, port);

	try {
		printListDirectory(argv[2], host, port);
	} catch (exception& e) {
		cerr << e.what() << endl;
		return 1;
	}
	return 0;
}
