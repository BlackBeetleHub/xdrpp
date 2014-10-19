// -*- C++ -*-

//! \file arpc.h Asynchronous RPC interface.

#include <map>
#include <mutex>
#include <xdrpp/exception.h>
#include <xdrpp/server.h>

namespace xdr {

class reply_cb_base {
  using cb_t = service_base::cb_t;
  uint32_t xid_;
  cb_t cb_;

protected:
  template<typename CB>
  reply_cb_base(uint32_t xid, CB &&cb)
    : xid_(xid), cb_(std::forward<CB>(cb)) {}
  reply_cb_base(reply_cb_base &&rcb)
    : xid_(rcb.xid_), cb_(std::move(rcb.cb_)) { rcb.cb_ = nullptr; }

  ~reply_cb_base() { if (cb_) reject(PROC_UNAVAIL); }

  reply_cb_base &operator=(reply_cb_base &&rcb) {
    xid_ = rcb.xid_;
    cb_ = std::move(rcb.cb_);
    rcb.cb_ = nullptr;
    return *this;
  }

  void send_reply_msg(msg_ptr &&b) {
    cb_(std::move(b));
    cb_ = nullptr;
  }

  template<typename T> void send_reply(const T &t) {
    send_reply_msg(xdr_to_msg(rpc_success_hdr(xid_), t));
  }

public:
  void reject(accept_stat stat) {
    send_reply_msg(rpc_accepted_error_msg(xid_, stat));
  }
  void reject(auth_stat stat) {
    send_reply_msg(rpc_auth_error_msg(xid_, stat));
  }
};

template<typename T> class reply_cb : public reply_cb_base {
public:
  using type = T;
  using reply_cb_base::reply_cb_base;
  reply_cb(reply_cb &&) = default;
  reply_cb &operator=(reply_cb &&) = default;
  void operator()(const T &t) { send_reply(t); }
};
template<> class reply_cb<void> : public reply_cb_base {
public:
  using type = void;
  using reply_cb_base::reply_cb_base;
  reply_cb(reply_cb &&) = default;
  reply_cb &operator=(reply_cb &&) = default;
  void operator()() { send_reply(xdr_void{}); }
  void operator()(xdr_void v) { send_reply(v); }
};

template<typename T, typename Session = void>
class arpc_service : public service_base {
  using interface = typename T::arpc_interface_type;
  T &server_;

public:
  void process(void *session, rpc_msg &hdr, xdr_get &g, cb_t reply) override {
    if (!check_call(hdr))
      reply(nullptr);
    if (!interface::call_dispatch(*this, hdr.body.cbody().proc, session, hdr, g,
				  std::move(reply)))
      reply(rpc_accepted_error_msg(hdr.xid, PROC_UNAVAIL));
  }

  template<typename P, typename...A> void
  dispatch(void *session, rpc_msg &hdr, xdr_get &g, cb_t reply) {
    std::tuple<transparent_ptr<A>...> arg;
    if (!decode_arg(g, arg))
      return reply(rpc_accepted_error_msg(hdr.xid, GARBAGE_ARGS));
    
    reply_cb<typename P::res_type> cb(hdr.xid, std::move(reply));
    P::unpack_dispatch(server_, std::move(arg),
		       reply_cb<typename P::res_type>{
			 hdr.xid, std::move(reply)});
  }

  arpc_service(T &server)
    : service_base(interface::program, interface::version),
      server_(server) {}
};

class arpc_server : public rpc_server_base {
public:
  template<typename T> void register_service(T &t) {
    register_service_base(new arpc_service<T>(t));
  }
  void receive(msg_sock *ms, msg_ptr buf);
};


//! A \c unique_ptr to a call result, or NULL if the call failed (in
//! which case \c message returns an error message).
template<typename T> struct call_result : std::unique_ptr<T> {
  rpc_call_stat stat_;
  using std::unique_ptr<T>::unique_ptr;
  call_result(const rpc_call_stat &stat) : stat_(stat) {}
  const char *message() const { return *this ? nullptr : stat_.message(); }
};
template<> struct call_result<void> {
  rpc_call_stat stat_;
  call_result(const rpc_call_stat &stat) : stat_(stat) {}
  const char *message() const { return stat_ ? nullptr : stat_.message(); }
};

class arpc_sock {
public:
  template<typename P> using call_cb_t =
    std::function<void(call_result<typename P::res_type>)>;
  using client_cb_t = std::function<void(rpc_msg &, xdr_get &)>;
  using server_cb_t = std::function<void(rpc_msg &, xdr_get &, arpc_sock *)>;

  arpc_sock(pollset &ps, int fd);

  template<typename P, typename...A> inline void
  invoke(const A &...a, call_cb_t<P> cb);

private:
  struct call_state_base {
    virtual ~call_state_base() {}
    virtual void get_reply(xdr_get &g) = 0;
    virtual void get_error(const rpc_call_stat &stat) = 0;
  };

  template<typename P> class call_state : call_state_base {
    call_cb_t<P> cb_;
    bool used_ {false};
  public:
    template<typename F> call_state(F &&f) : cb_(std::forward<F>(f)) {}
    ~call_state() { get_error(rpc_call_stat::NETWORK_ERROR); }

    void get_reply(xdr_get &g) override {
      if (used_)
	return;

      call_result<typename P::res_wire_type> r{new typename P::res_wire_type};
      try { archive(g, *r); }
      catch (const xdr_runtime_error &) {
	get_error(rpc_call_stat::GARBAGE_RES);
	return;
      }
      catch (const std::bad_alloc &) {
	get_error(rpc_call_stat::BAD_ALLOC);
	return;
      }
      used_ = true;
      cb_(r);
    }

    void get_error(const rpc_call_stat &stat) override {
      if (used_)
	return;
      used_ = true;
      cb_(stat);
    }
  };

  std::unique_ptr<msg_sock> ms_;
  uint32_t xid_counter_{0};
  std::map<uint32_t, std::unique_ptr<call_state_base>> calls_;
  std::map<uint32_t, std::map<uint32_t, server_cb_t>> services_;

  void prepare_call(rpc_msg &hdr, uint32_t prog,
		    uint32_t vers, uint32_t proc);
  void receive(msg_ptr buf);

};

#if 0
template<typename P> inline void
arpc_sock::invoke(const typename P::arg_wire_type &arg, call_cb_t<P> cb)
{
  rpc_msg hdr;
  prepare_call(hdr, P::interface_type::program,
	       P::interface_type::version, P::proc);
  ms_->putmsg(xdr_to_msg(hdr, arg));
  calls_.emplace(hdr.xid, new call_state<P>(cb));
}
#endif

}

