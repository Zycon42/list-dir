# Makefile for listDir(Project 2 IPK)
# author: Jan Dusek <xdusek17@stud.fit.vutbr.cz>
#

CXX=g++
CXXFLAGS=-std=c++98 -pedantic -Wall -O2
LDFLAGS=
CLIENT_NAME=client
SERVER_NAME=server
CLIENT_OBJFILES=client.o socket.o
SERVER_OBJFILES=server.o socket.o

.PHONY: all clean #dep

all: $(CLIENT_NAME) $(SERVER_NAME)

#-include dep.list

$(CLIENT_NAME): $(CLIENT_OBJFILES)
	$(CXX) $(LDFLAGS) -o $(CLIENT_NAME) $(CLIENT_OBJFILES)

$(SERVER_NAME): $(SERVER_OBJFILES)
	$(CXX) $(LDFLAGS) -o $(SERVER_NAME) $(SERVER_OBJFILES)

#dep:
#	$(CXX) -MM *.cpp > dep.list

clean:
	rm -f *.o $(CLIENT_NAME) $(SERVER_NAME)
