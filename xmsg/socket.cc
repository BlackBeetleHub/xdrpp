
#include <cstring>
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <xdrc/socket.h>

namespace xdr {

using std::string;

void
set_nonblock(int fd)
{
  int n;
  if ((n = fcntl (fd, F_GETFL)) == -1
      || fcntl (fd, F_SETFL, n | O_NONBLOCK) == -1)
    throw std::system_error(errno, std::system_category(), "O_NONBLOCK");
}

void
set_close_on_exec(int fd)
{
  int n;
  if ((n = fcntl (fd, F_GETFD)) == -1
      || fcntl (fd, F_SETFD, n | FD_CLOEXEC) == -1)
    throw std::system_error(errno, std::system_category(), "F_SETFD");
}

void
really_close(int fd)
{
  while (close(fd) == -1)
    if (errno != EINTR) {
      std::cerr << "really_close: " << std::strerror(errno) << std::endl;
      return;
    }
}

int
parse_uaddr_port(const string &uaddr)
{
  int low = uaddr.rfind('.');
  if (low == string::npos || low == 0)
    return -1;
  int high = uaddr.rfind('.', low - 1);
  if (high == string::npos)
    return -1;
}

}
