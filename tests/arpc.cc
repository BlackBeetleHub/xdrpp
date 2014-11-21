
#include <xdrpp/arpc.h>
#include "xdrtest.hh"

using namespace std;
using namespace xdr;

using namespace testns;

class xdrtest2_server {
public:
  using rpc_interface_type = xdrtest2;

  void null2(xdr::reply_cb<void> cb) { cb(); }
  void nonnull2(const u_4_12 &arg, xdr::reply_cb<ContainsEnum> cb) {
    ContainsEnum c(::REDDER);
    c.num() = ContainsEnum::TWO;
    cb(c);
  }
  void ut(const uniontest &arg, xdr::reply_cb<void> cb) {
  }
  void three(const bool &arg1, const int &arg2,
	     const bigstr &arg3, xdr::reply_cb<bigstr> cb) {
    cb("Here is your reply string");
  }
};

void
check_rpc_success_header()
{
  msg_ptr m1 (xdr_to_msg(rpc_msg(7, REPLY)));
  msg_ptr m2 (xdr_to_msg(rpc_success_hdr(7)));

  assert(m1->size() == m2->size());
  assert(!memcmp(m1->data(), m2->data(), m1->size()));
}

int
main(int argc, char **argv)
{
  check_rpc_success_header();

  if (argc > 1 && !strcmp(argv[1], "-s")) {
    xdrtest2_server s;
    arpc_tcp_listener<> rl;
    rl.register_service(s);
    rl.run();
  }
  else if (argc > 1 && !strcmp(argv[1], "-c")) {
#if 0
    auto fd = tcp_connect_rpc(argc > 2 ? argv[2] : nullptr,
			      xdrtest2::program, xdrtest2::version);
    srpc_client<xdrtest2> c{fd.get()};

    c.null2();

    u_4_12 u(12);
    u.f12().i = 1977;
    auto r = c.nonnull2(u);
    cout << xdr_to_string(*r, "nonnull2 reply");

    auto s = c.three(true, 8, "this is the third argument");
    cout << *s << endl;
#endif
  }
  else
    cerr << "need -s or -c option" << endl;
  return 0;
}
