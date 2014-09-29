
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <thread>
#include <xdrc/srpc.h>
#include "xdrtest.hh"

using namespace std;
using namespace xdr;

using namespace testns;

class xdrtest2_server {
public:
  using rpc_interface_type = testns::xdrtest2;

  void null2();
  std::unique_ptr<ContainsEnum> nonnull2(std::unique_ptr<u_4_12> arg);
  void ut(std::unique_ptr<uniontest> arg);
};

void
xdrtest2_server::null2()
{
  cerr << "I got a null request" << endl;
}

std::unique_ptr<testns::ContainsEnum>
xdrtest2_server::nonnull2(std::unique_ptr<u_4_12> arg)
{
  using namespace testns;
  std::unique_ptr<ContainsEnum> res(new ContainsEnum);
  
  cerr << "I got a nonnull request" << endl
       << xdr_to_string(*arg, "arg");
  res->c(::REDDER).num() = ContainsEnum::TWO;
  
  return res;
}

void
xdrtest2_server::ut(std::unique_ptr<uniontest> arg)
{
  
  // Fill in function body here
  
}

void
getmsg(int fd)
{
#if 0
  msg_ptr p;
  try { p = read_message(fd); }
  catch (const std::exception &e) {
    cerr << "caught " << e.what() << endl;
    close(fd);
    return;
  }

  cerr << "got " << p->size() << "-byte message" << endl;

  xdr_get g(p);
  rpc_msg hdr;
  archive(g, hdr);
  
  cerr << xdr_to_string(hdr);

  close(fd);
#endif

  xdrtest2_server s;
  server_fd sfd(fd);
  sfd.register_server(s);
  sfd.run();
  
  close(fd);
}

void
sendreq(int fd)
{
  testns::xdrtest2::client<synchronous_client> sc(fd);

  u_4_12 arg(12);
  arg.f12().i = 77;
  arg.f12().d = 3.141592654;
  testns::ContainsEnum ce = sc.nonnull2(arg);

  cout << xdr_to_string(ce);
}

int
main()
{
  int fds[2];
  if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == -1) {
    perror("socketpair");
    exit(1);
  }

  thread t1(sendreq, fds[0]);
  getmsg(fds[1]);
  
  t1.join();

  return 0;
}
