
#include <iostream>
#include <xdrpp/server.h>

namespace xdr {

bool xdr_trace_server = std::getenv("XDR_TRACE_SERVER");

msg_ptr
rpc_accepted_error_msg(uint32_t xid, accept_stat stat)
{
  assert(stat != SUCCESS && stat != PROG_MISMATCH);
  msg_ptr buf(message_t::alloc(24));
  xdr_put p(buf);
  p(xid);
  p(REPLY);
  p(MSG_ACCEPTED);
  p(AUTH_NONE);
  p(uint32_t(0));
  p(stat);
  assert(p.p_ == p.e_);
  return buf;
}

msg_ptr
rpc_prog_mismatch_msg(uint32_t xid, uint32_t low, uint32_t high)
{
  msg_ptr buf(message_t::alloc(32));
  xdr_put p(buf);
  p(xid);
  p(REPLY);
  p(MSG_ACCEPTED);
  p(AUTH_NONE);
  p(uint32_t(0));
  p(PROG_MISMATCH);
  p(low);
  p(high);
  assert(p.p_ == p.e_);
  return buf;
}

msg_ptr
rpc_auth_error_msg(uint32_t xid, auth_stat stat)
{
  msg_ptr buf(message_t::alloc(20));
  xdr_put p(buf);
  p(xid);
  p(REPLY);
  p(MSG_DENIED);
  p(AUTH_ERROR);
  p(stat);
  assert(p.p_ == p.e_);
  return buf;
}

msg_ptr
rpc_rpc_mismatch_msg(uint32_t xid)
{
  msg_ptr buf(message_t::alloc(24));
  xdr_put p(buf);
  p(xid);
  p(REPLY);
  p(MSG_DENIED);
  p(RPC_MISMATCH);
  p(uint32_t(2));
  p(uint32_t(2));
  assert(p.p_ == p.e_);
  return buf;
}


void
rpc_server_base::register_service_base(service_base *s)
{
  servers_[s->prog_][s->vers_].reset(s);
}

void
rpc_server_base::dispatch(void *session, msg_ptr m, service_base::cb_t reply)
{
  xdr_get g(m);
  rpc_msg hdr;

  try { archive(g, hdr); }
  catch (const xdr_runtime_error &e) {
    std::cerr << "rpc_server_base::dispatch: ignoring malformed header: "
	      << e.what() << std::endl;
    return;
  }
  if (hdr.body.mtype() != CALL) {
    std::cerr << "rpc_server_base::dispatch: ignoring non-CALL" << std::endl;
    return;
  }

  if (hdr.body.cbody().rpcvers != 2)
    return reply(rpc_rpc_mismatch_msg(hdr.xid));

  auto prog = servers_.find(hdr.body.cbody().prog);
  if (prog == servers_.end())
    return reply(rpc_accepted_error_msg(hdr.xid, PROG_UNAVAIL));

  auto vers = prog->second.find(hdr.body.cbody().vers);
  if (vers == prog->second.end()) {
    uint32_t low = prog->second.cbegin()->first;
    uint32_t high = prog->second.crbegin()->first;
    return reply(rpc_prog_mismatch_msg(hdr.xid, low, high));
  }

  try {
    vers->second->process(session, hdr, g, reply);
    return;
  }
  catch (const xdr_runtime_error &e) {
    std::cerr << "rpc_server_base::dispatch: " << e.what() << std::endl;
  }
  reply(rpc_accepted_error_msg(hdr.xid, GARBAGE_ARGS));
}


rpc_tcp_listener_common::rpc_tcp_listener_common(unique_fd &&fd, bool reg)
  : listen_fd_(fd ? std::move(fd) : tcp_listen()),
    use_rpcbind_(reg)
{
  set_close_on_exec(listen_fd_.get());
  ps_.fd_cb(listen_fd_.get(), pollset::Read,
	    std::bind(&rpc_tcp_listener_common::accept_cb, this));
}

rpc_tcp_listener_common::~rpc_tcp_listener_common()
{
  ps_.fd_cb(listen_fd_.get(), pollset::Read);
}

void
rpc_tcp_listener_common::accept_cb()
{
  int fd = accept(listen_fd_.get(), nullptr, 0);
  if (fd == -1) {
    std::cerr << "rpc_tcp_listener_common: accept: " << std::strerror(errno)
	      << std::endl;
    return;
  }
  set_close_on_exec(fd);
  msg_sock *ms = new msg_sock(ps_, fd);
  ms->setrcb(std::bind(&rpc_tcp_listener_common::receive_cb, this, ms,
		       session_alloc(fd, ms), std::placeholders::_1));
}

void
rpc_tcp_listener_common::receive_cb(msg_sock *ms, void *session, msg_ptr mp)
{
  if (!mp) {
    session_free(session);
    delete ms;
    return;
  }
  try {
    dispatch(nullptr, std::move(mp), msg_sock_put_t(ms));
  }
  catch (const xdr_runtime_error &e) {
    std::cerr << e.what() << std::endl;
    session_free(session);
    delete ms;
  }
}

void
rpc_tcp_listener_common::run()
{
  while (ps_.pending())
    ps_.poll();
}


}
