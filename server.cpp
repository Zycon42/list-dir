/**
 * @file server.cpp
 * @author: Jan Dusek <xdusek17@stud.fit.vutbr.cz>
 *
 * Vypis obsahu adresare
 * Projekt c. 2 do predmetu IPK
 * fce main serveru
 */

// own includes
#include "socket.h"
#include "exceptions.h"

// STL includes
#include <iostream>
#include <string>
#include <sstream>
#include <vector>

// posix c includes
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <dirent.h>

using namespace std;

static const char *USAGE = "Usage: server -p PORT";

/// global flag if server is runing
static volatile bool isRunning = true;

/// status codes in server response
enum StatusCodes {
	C_OK			= 0,
	C_BAD_REQUEST	= 1,
	C_ACCES_DENIED	= 2,
	C_NO_ENT		= 3,
	C_NOT_DIR		= 4,
	C_NOT_ABSOLUTE	= 5,
	C_INTERNAL		= 6
};
/// messages of status codes in server response
const char* STATUS_CODE_MSGS[] = {
	"OK",
	"Bad request",
	"Access denied",
	"Directory doesn't exists",
	"Path is not directory",
	"Path must be absolute",
	"Internal server error"
};
/// gets status message from status code
inline const char* getStatusMsg(int statusCode) {
	return STATUS_CODE_MSGS[statusCode];
}

int errnoToStatusCode() {
	switch (errno) {
		case 0:
			return C_OK;
		case EACCES:
			return C_ACCES_DENIED;
		case ENOENT:
			return C_NO_ENT;
		case ENOTDIR:
			return C_NOT_DIR;
		default:
			return C_INTERNAL;
	}
}

/// this handler sets global flag to false so we cleanup and exit
void handleTerminationSignal(int signum) {
	isRunning = false;
}

/// converts string to int. throws std::runtime_error on failure
int stringToInt(const string& str) {
	istringstream iss(str);
	int i;
	char c;
	if (!(iss >> i) || iss.get(c))
		throw runtime_error("argument is not number");
	return i;
}

/**
 * registers signal handler to multiple signals
 * signals are specified by its numbers in sigNums array of size n
 * @param sigNums array of signal numbers
 * @param n length of sigNum array
 */
void registerSignalHandlers(int* sigNums, size_t n) {
	struct sigaction action;
	action.sa_flags = 0;
	action.sa_handler = handleTerminationSignal;
	sigemptyset(&action.sa_mask);

	for (size_t i = 0; i < n; i++)
		if (sigaction(sigNums[i], &action, NULL) == -1)
			throw system_error("sigaction", errno);
}

int getDirectoryContents(string& path, vector<string>& fileNames) {
	fileNames.clear();
	if (path.empty())			// empty path is bad request
		return C_BAD_REQUEST;

	// path must be absolute and absolute path starts with /
	if (path[0] != '/')
		return C_NOT_ABSOLUTE;

	// add ending / if not present
	if (path[path.length() - 1] != '/')
		path += '/';

	DIR* dir = opendir(path.c_str());
	if (dir == NULL)
		return errnoToStatusCode();

	struct stat st;
	dirent* dirent;
	while ((dirent = readdir(dir)) != NULL) {
		// ignore .(curent dir) and ..(parent dir)
		if (dirent->d_name == string(".") || dirent->d_name == string(".."))
			continue;

		// call lstat to get file info and if its regular file directory or symlink store it in retval
		if (lstat(string(path + dirent->d_name).c_str(), &st) == -1) {
			closedir(dir);
			return C_INTERNAL;
		}
		if (S_ISREG(st.st_mode) || S_ISDIR(st.st_mode) || S_ISLNK(st.st_mode))
			fileNames.push_back(dirent->d_name);
	}

	closedir(dir);
	return C_OK;
}

/**
 * handles client request and sends response.
 * @param socket socket connected to client used for communication
 * @return exit code
 */
int handleClient(SocketStream* socket) {
	string magic, path;
	int statusCode = C_OK;
	char c;
	if (!(*socket >> magic) || magic != "LD/1.0" || !socket->get(c) || c != ' ' || !getline(*socket, path))
		statusCode = C_BAD_REQUEST;

	vector<string> fileNames;
	if (statusCode == C_OK)
		statusCode = getDirectoryContents(path, fileNames);

	*socket << "LD/1.0 " << statusCode << " " << getStatusMsg(statusCode) << "\n";

	for (vector<string>::iterator it = fileNames.begin(); it != fileNames.end(); ++it) {
		*socket << *it << "\n";
	}

	socket->flush();

	return 0;
}

int main(int argc, char** argv) {
	if (argc != 3 || argv[1] != string("-p")) {
		cerr << USAGE << endl;
		return 1;
	}

	try {
		// register terminating signals to handler cuz we need to
		// give process time to cleanup sockets
		int sigNums[] = { SIGINT, SIGTERM, SIGHUP };
		registerSignalHandlers(sigNums, sizeof sigNums / sizeof sigNums[0]);

		// create listening socket to port in argv[2]
		SocketStream listeningSocket;
		listeningSocket.listen(stringToInt(argv[2]));

		// infinite loop where we accept connection
		// fork process and in child handle client request
		while (isRunning) {
            // check if some child exited to correctly exit it
            waitpid(-1, NULL, WNOHANG);

			int err;
			SocketStream* socket = listeningSocket.accept(err);
			if (err != 0 && err != EINTR )
				throw system_error("accept", err);

			// fork process
			pid_t pid = fork();
			if (pid == 0) {			// child
				// handle client request and exit process
				int ret = handleClient(socket);
				delete socket;
				return ret;
			} else if (pid == -1) {	// error
				throw system_error("fork", errno);
			} else {				// parent
				delete socket;
			}
		}
	} catch (exception& e) {
		cerr << e.what() << endl;
		return 1;
	}
	return 0;
}
