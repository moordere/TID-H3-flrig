// ----------------------------------------------------------------------------
//
// flxmlrpc Copyright (c) 2015 by W1HKJ, Dave Freese <iam_w1hkj@w1hkj.com>
//
// XmlRpc++ Copyright (c) 2002-2008 by Chris Morley
//
// This file is part of fldigi
//
// flxmlrpc is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------

#include "config.h"

#include <iostream>

#if !defined(__WIN32__) && !defined(__APPLE__)
#  include <sys/un.h>
#endif

#include "XmlRpcSocket.h"
#include "XmlRpcUtil.h"


#if defined(_WINDOWS)
#undef _WINSOCKAPI_
# include <stdio.h>
# include <winsock2.h>
//# pragma lib(WS2_32.lib)

// Updates to MXE require conditional test before assignment to
// prevent duplicate defines.

#undef EINPROGRESS
#define EINPROGRESS  WSAEINPROGRESS

#undef EWOULDBLOCK
#define EWOULDBLOCK  WSAEWOULDBLOCK

#undef ETIMEDOUT
#define ETIMEDOUT    WSAETIMEDOUT

typedef int socklen_t;

#include "compat.h"

#else
extern "C" {
# include <unistd.h>
# include <stdio.h>
# include <string.h>
# include <sys/types.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <netdb.h>
# include <errno.h>
# include <fcntl.h>
# include <signal.h>
}
#endif  // _WINDOWS

#if !defined(__WIN32__) && !defined(__APPLE__)
const char LINUX_UDS_PATH[] = "/tmp/flrig_uds.sock";
#endif

using namespace XmlRpc;

// One-time initializations
static bool initialized = false;

static void initialize()
{
    initialized = true;

#if defined(_WINDOWS)
    {
        WORD wVersionRequested = MAKEWORD( 2, 0 );
        WSADATA wsaData;
        WSAStartup(wVersionRequested, &wsaData);
        atexit((void(*)(void)) WSACleanup);
    }
#else
    {
        // Ignore SIGPIPE
        (void) signal(SIGPIPE, SIG_IGN);
    }
#endif // _WINDOWS
}

// These errors are not considered fatal for an IO operation; the operation will be re-tried.
bool
XmlRpcSocket::nonFatalError()
{
  int err = XmlRpcSocket::getError();
  return (err == EINPROGRESS ||
#if defined(EAGAIN)
          err == EAGAIN ||
#endif
#if defined(EINTR)
          err == EINTR ||
#endif
          err == EWOULDBLOCK);
}


XmlRpcSocket::Socket
XmlRpcSocket::socket()
{
  if ( ! initialized) initialize();
#if !defined(__WIN32__) && !defined(__APPLE__)
  if (XmlRpc::UDS)
    return ::socket(AF_UNIX, SOCK_STREAM, 0);
#endif
  return ::socket(AF_INET, SOCK_STREAM, 0);
}

void
XmlRpcSocket::close(XmlRpcSocket::Socket fd)
{
  XmlRpcUtil::log(4, "XmlRpcSocket::close: fd %d.", fd);
#if defined(_WINDOWS)
  closesocket(fd);
#else
  ::close(fd);
#endif // _WINDOWS
}




bool
XmlRpcSocket::setNonBlocking(XmlRpcSocket::Socket fd)
{
#if defined(_WINDOWS)
  unsigned long flag = 1;
  return (ioctlsocket(fd, FIONBIO, &flag) == 0);
#else
  return (fcntl(fd, F_SETFL, O_NONBLOCK) == 0);
#endif // _WINDOWS
}


bool
XmlRpcSocket::setReuseAddr(XmlRpcSocket::Socket fd)
{
  // Allow this port to be re-bound immediately so server re-starts are not delayed
  int sflag = 1;
  return (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&sflag, sizeof(sflag)) == 0);
}


// Bind to a specified port
bool
XmlRpcSocket::bind(XmlRpcSocket::Socket fd, int port)
{
  struct sockaddr_in saddr;
  memset(&saddr, 0, sizeof(saddr));
  saddr.sin_family = AF_INET;
  saddr.sin_addr.s_addr = htonl(INADDR_ANY);
  saddr.sin_port = htons((u_short) port);
  return (::bind(fd, (struct sockaddr *)&saddr, sizeof(saddr)) == 0);
}

#if !defined(__WIN32__) && !defined(__APPLE__)
// Bind to a specific path
	bool
	XmlRpcSocket::bind( XmlRpcSocket::Socket fd, const char *socket_path)
	{
		struct sockaddr_un saddr;
		memset(&saddr, 0, sizeof(saddr));
		saddr.sun_family = AF_UNIX;
		strncpy(saddr.sun_path, socket_path, sizeof(saddr.sun_path) - 1);
		int ret =  ::bind(fd, (struct sockaddr *)&saddr, sizeof(saddr));
		return ( ret == 0);
	}
#endif

// Set socket in listen mode
bool
XmlRpcSocket::listen(XmlRpcSocket::Socket fd, int backlog)
{
  return (::listen(fd, backlog) == 0);
}


XmlRpcSocket::Socket
XmlRpcSocket::accept(XmlRpcSocket::Socket fd)
{
  struct sockaddr_in addr;
  socklen_t addrlen = sizeof(addr);

  return ::accept(fd, (struct sockaddr*)&addr, &addrlen);
}



// Connect a socket to a server (from a client)
bool
XmlRpcSocket::connect(XmlRpcSocket::Socket fd, std::string& host, int port)
{
  int result;
#if !defined(__WIN32__) && !defined(__APPLE__)
  if (XmlRpc::UDS) {
    struct sockaddr_un saddr;

    // Configure server address
    memset(&saddr, 0, sizeof(struct sockaddr_un));
    saddr.sun_family = AF_UNIX;
    strncpy(saddr.sun_path, LINUX_UDS_PATH, sizeof(saddr.sun_path) - 1);

    result = ::connect(fd, (struct sockaddr *)&saddr, sizeof(struct sockaddr_un));

  } else 
#endif
  {
      struct sockaddr_in saddr;
      memset(&saddr, 0, sizeof(saddr));
      saddr.sin_family = AF_INET;

      struct hostent *hp = gethostbyname(host.c_str());
      if (hp == 0) return false;

      saddr.sin_family = hp->h_addrtype;
      memcpy(&saddr.sin_addr, hp->h_addr, hp->h_length);
      saddr.sin_port = htons((u_short) port);

      result = ::connect(fd, (struct sockaddr *)&saddr, sizeof(saddr));
  }
// std::cout << "::connect result " << result << std::endl;

  return result == 0 || nonFatalError();
}



// Get the port of a bound socket
int
XmlRpcSocket::getPort(XmlRpcSocket::Socket socket)
{
  struct sockaddr_in saddr;
  socklen_t saddr_len = sizeof(saddr);
  int port;

  int result = ::getsockname(socket, (sockaddr*) &saddr, &saddr_len);

  if (result != 0) {
    port = -1;
  } else {
    port = ntohs(saddr.sin_port);
  }
  return port;
}


// Returns last errno
int
XmlRpcSocket::getError()
{
#if defined(_WINDOWS)
  return WSAGetLastError();
#else
  return errno;
#endif
}


// Returns message corresponding to last errno
std::string
XmlRpcSocket::getErrorMsg()
{
  return getErrorMsg(getError());
}

// Returns message corresponding to errno... well, it should anyway
std::string
XmlRpcSocket::getErrorMsg(int error)
{
  char err[60];
  snprintf(err,sizeof(err),"error %d", error);
  return std::string(err);
}


