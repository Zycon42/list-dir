/**
 * @file: socket.cpp
 * @author: Jan Dusek <xdusek17@stud.fit.vutbr.cz>
 *
 * Projekt c.2 do predmetu IPK
 * Wrapper class for bsd socket for TCP
 */

#include "socket.h"
#include "exceptions.h"

#include <stdexcept>
#include <string.h>
#include <errno.h>
#include <netinet/in.h>

Resolver::~Resolver(){
	/// iterate through addrinfo vector to free them
	for (std::vector<addrinfo*>::iterator it = infos.begin(); it != infos.end(); ++it){
		freeaddrinfo(*it);
	}
}


Resolver::Iterator Resolver::resolve(const std::string& hostname, const std::string& servicename){
	addrinfo hints, *info;
	memset(&hints, 0, sizeof(addrinfo));
	hints.ai_family = AF_UNSPEC;    // Allow IPv4 or IPv6
	hints.ai_socktype = SOCK_STREAM; // stream socket
	hints.ai_flags = AI_CANONNAME;	// also set cannon name
	hints.ai_protocol = 0;          // Any protocol

	int errcode = getaddrinfo(hostname.c_str(), servicename.c_str(), &hints, &info);
	if (errcode != 0)
		throw std::runtime_error(std::string("Failed to resolve hostname: ") + gai_strerror(errcode));

	// store info to vector. create endpoint and return iterator to it
	infos.push_back(info);
	EndPoint ep(info);
	return Resolver::Iterator(ep);
}

SocketBuf::SocketBuf(std::streamsize inBufSize, std::streamsize outBufSize) {
	gSize = inBufSize;
	pSize = outBufSize;

	gBuf = new char[gSize];
	pBuf = new char[pSize];

	// set input and output area
	setg(gBuf, gBuf, gBuf);
	setp(pBuf, pBuf + pSize - 1);
}

SocketBuf::~SocketBuf() {
	sync();
	delete[] gBuf;
	delete[] pBuf;
}

int SocketBuf::underflow() {
	// if get ptr is at end
	if (this->egptr() >= gBuf + gSize)
		setg(gBuf, gBuf, gBuf);		// reset it to point at begin

	// receive data from socket to input buffer
	int i = recv(sockFd, this->egptr(), gSize - (this->gptr() - gBuf), 0);

	// set end ptr to end + i
	setg(this->eback(), this->gptr(), this->egptr() + i);

	// return curr char or eof on recv error
	return i <= 0 ? traits_type::eof() : *this->gptr();
}

int SocketBuf::overflow(int c) {
	int length = this->pptr() - pBuf;		// get output data length
	// add char to output sequence
	if (c != traits_type::eof()) {
		*this->pptr() = c;
		++length;
	}

	// send output sequence to socket
	char* buf = pBuf;
	int sent;
	// we will send pBuf till whole pBuf is sent
	while ((sent = send(sockFd, buf, length, 0)) != length) {
		if (sent == -1) {
			c = traits_type::eof();
			break;
		}

		buf += sent;
		length -= sent;
	}

	// reset pointers
	setp(pBuf, pBuf + pSize - 1);

	return c;
}

int SocketBuf::sync()
{
	// if output buffer isnt empty call overflow
	if (this->pptr() != pBuf)
		overflow();
    return 0;
}


SocketStream::SocketStream() : state(SocketStream::Open), sockbuf(new SocketBuf()) {
	this->init(sockbuf);
}

SocketStream::~SocketStream(){
	close();
	delete sockbuf;
}

int SocketStream::connect(const EndPoint& endPoint, int& error) {
	if (state != SocketStream::Open)
		throw std::runtime_error("SocketStream::connect must be called on open socket");

	addrinfo* info = endPoint.getData();

	sockFd = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
	if (sockFd == -1)
		return error = errno;

	state = SocketStream::Connected;

	if (::connect(sockFd, info->ai_addr, info->ai_addrlen) == -1)
		return error = errno;

	sockbuf->setSocket(sockFd);

	return error = 0;
}

void SocketStream::connect(const EndPoint& endPoint) {
	int errnum;
	if (connect(endPoint, errnum) != 0)
		throw system_error("SocketStream::connect", errnum);
}


int SocketStream::listen(int port, int& error) {
	if (state != SocketStream::Open)
		throw std::runtime_error("SocketStream::listen must be called on open socket");

	sockFd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sockFd == -1)
		return error = errno;

	state = SocketStream::Listening;

	sockaddr_in sin;
	sin.sin_family = PF_INET;
	sin.sin_port = htons(port);
	sin.sin_addr.s_addr  = INADDR_ANY;
	if (bind(sockFd, (sockaddr*)&sin, sizeof(sockaddr_in)) == -1)
		return error = errno;

	if (::listen(sockFd, 128) == -1)
		return error = errno;

	sockbuf->setSocket(sockFd);

	return error = 0;
}

void SocketStream::listen(int port){
	int errnum;
	if (listen(port, errnum) != 0)
		throw system_error("SocketStream::listen", errnum);
}

SocketStream* SocketStream::accept(int& error) {
	if (state != SocketStream::Listening)
		throw std::runtime_error("SocketStream::accept must be called on listening socket");

	error = 0;
	SocketStream* sock = new SocketStream();

	sock->sockFd = ::accept(sockFd, NULL, NULL);
	if (sock->sockFd == -1) {
		error = errno;
		return NULL;
	}

	sock->sockbuf->setSocket(sock->sockFd);
	sock->state = SocketStream::Connected;

	return sock;
}

SocketStream* SocketStream::accept() {
	int errnum;
	SocketStream* ret = accept(errnum);
	if (errnum != 0)
		throw system_error("SocketStream::accept", errnum);
	return ret;
}

void SocketStream::close(){
	sockbuf->pubsync();
	if (state != SocketStream::Open) {
		::close(sockFd);
        std::clog << (int)getpid() << " socket " << sockFd << " closed\n";
    }
	state = SocketStream::Open;
}
