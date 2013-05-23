/**
 * @file exceptions.h
 * @author: Jan Dusek <xdusek17@stud.fit.vutbr.cz>
 *
 * Projekt c.2 do predmetu IPK
 * exception classes
 */

#ifndef _EXCEPTIONS_H_
#define _EXCEPTIONS_H_

#include <stdexcept>
#include <string>

#include <string.h>

/**
 * exception for system calls
 * uses strerror function to get proper message describing error
 */
class system_error : public std::exception
{
public:
	/// parameterless ctor
	system_error() throw() { }
	/// destructor
	virtual ~system_error() throw() { }
	/**
	 * Constructs system_error object from optional string and errnum
	 * if str is NULL then what() = strerror(errnum) else what() = str + ": " + strerror(errnum)
	 * @param str optional parameter
	 * @param errnum describing error. see errno
	 */
	system_error(const char* str, int errnum) {
		if (str != NULL)
			msg = std::string(str) + std::string(": ") + strerror(errnum);
		else
			msg = strerror(errnum);
	}

	virtual const char * what() const throw() {
		return msg.c_str();
	}
private:
	std::string msg;
};

#endif // _EXCEPTIONS_H_
