/**
 * @file: socket.h
 * @author: Jan Dusek <xdusek17@stud.fit.vutbr.cz>
 *
 * Projekt c.2 do predmetu IPK
 * Wrapper class for bsd socket for TCP
 */

#ifndef _SOCKET_H_
#define _SOCKET_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <iostream>
#include <streambuf>
#include <string>
#include <vector>

/// Endpoint where socket could connect
class EndPoint
{
public:
	EndPoint() : info(NULL) { }
	explicit EndPoint(addrinfo* info) : info(info) { };
	addrinfo* getData() const { return info; }
	const char* getName() const { return info->ai_canonname; }
private:
	addrinfo* info;
};

class Resolver
{
public:
	/// iterator to endpoint
	class Iterator
	{
	public:
		/// default ctor creates iterator to end
		Iterator() { }
		/// creates iterator to specific endpoint
		Iterator(EndPoint& ep) : endPoint(ep) {}

		EndPoint& operator *() { return endPoint; };
		EndPoint* operator ->() { return &endPoint; };

		// prefix
		Iterator& operator ++() { if (endPoint.getData() != NULL) endPoint = EndPoint(endPoint.getData()->ai_next); return *this; };
		// postfix
		Iterator operator ++(int) {
			Iterator tmp = *this;
			if (endPoint.getData() != NULL)
				endPoint = EndPoint(endPoint.getData()->ai_next);
			return tmp;
		};

		bool operator ==(const Iterator& it) const { return this->endPoint.getData() == it.endPoint.getData(); };
		bool operator !=(const Iterator& it) const { return !(*this == it); };
	private:
		EndPoint endPoint;
	};

	Resolver() { }
	~Resolver();
	/**
	 * Resolves hostname and service name. Uses getaddrinfo function
	 * @param hostname name of host in internet
	 * @param servicename port number or name which is resolved from /etc/services file
	 * @return iterator to first endpoint returned by getaddrinfo
	 */
	Iterator resolve(const std::string& hostname, const std::string& servicename);
private:
	/// prevent using copy ctor and assign operator
	Resolver(const Resolver&);
	Resolver& operator= (const Resolver&);

	std::vector<addrinfo*> infos;
};

/// specialization of abstract base class streambuf for SocketStream
class SocketBuf : public std::streambuf
{
public:
	/// creates buffer with specified sizes of input and output buffers
    explicit SocketBuf(std::streamsize inBufSize = 256, std::streamsize outBufSize = 256);
	virtual ~SocketBuf();

	/// gets and sets internal socket file descriptor of bsd sockets api
	void setSocket(int fd) { sockFd = fd; }
	int getSocket() const { return sockFd; }
protected:
	virtual int overflow(int c = traits_type::eof());
	virtual int underflow();
	virtual int sync();
private:
	/// prevent using copy ctor and assign operator
	SocketBuf(const SocketBuf&);
	SocketBuf& operator= (const SocketBuf&);

	typedef std::char_traits<char> traits_type;

	char* gBuf;		/// internal get buffer
	char* pBuf;		/// internal put buffer

	/// internal buffers size
	std::streamsize gSize, pSize;

	int sockFd;
};

/// Wrapper around bsd sockets API so it could be used as classic STL stream
/// its input and output. For receiving and sending data use classic STL stream approach
class SocketStream : public std::istream, public std::ostream
{
	/// socket states
	enum State { Open, Connected, Listening };
public:
	/// default ctor
	SocketStream();
	/// destructor calls close()
	~SocketStream();

	/**
	 * Connects socket to endpoint
	 * @param endPoint to connect to
	 * @retval error error number from socket or connect system calls (errno)
	 */
	int connect(const EndPoint& endPoint, int& error);
	void connect(const EndPoint& endPoint);
	/**
	 * Socket starts listening on specified port. Waiting for connections.
	 * Returns immediately. Socket has to be in free state.
	 * @param port port num where socket starts to listen
	 * @retval error error number from socket, bind, or listem system calls (errno)
	 */
	int listen(int port, int& error);
	void listen(int port);
	/**
	 * Accepts waiting connection. Socket must be in listening state.
	 * If there're no waiting connections it blocks the caller.
	 * @retval error error number from accept system call (errno)
	 * @return new connected socket representing established connection
	 */
	SocketStream* accept(int& error);
	SocketStream* accept();
	/**
	 * Closes socket. socket needs to be closed to connect again
	 */
	void close();

	int getSockFd() const { return sockFd; }
private:
	/// prevent using copy ctor and assign operator
	SocketStream(const SocketStream&);
	SocketStream& operator= (const SocketStream&);

	State state;
	int sockFd;
	SocketBuf* sockbuf;
};

#endif // _SOCKET_H_
