// -*-c++-*-

#include <cassert>
#include <iosfwd>
#include <functional>
#include <memory>
#include <set>
#include <string>
#include <vector>
#include <unordered_map>
#include "union.h"

using std::string;

template<typename T> struct vec : std::vector<T> {
  using std::vector<T>::vector;
  using std::vector<T>::push_back;
  T &push_back() { this->emplace_back(); return this->back(); }
};

extern std::set<string> ids;

extern int lineno;
extern int printlit;
#undef yyerror
int yyerror(string);
int yywarn(string);

int yylex();
int yyparse();
void checkliterals ();

struct rpc_enum;
struct rpc_struct;
struct rpc_union;

struct rpc_decl {
  string id;
  enum { SCALAR, PTR, ARRAY, VEC } qual;
  string bound;

  enum { TS_ID, TS_ENUM, TS_STRUCT, TS_UNION } ts_which;
  string type;
  std::shared_ptr<rpc_enum> ts_enum;
  std::shared_ptr<rpc_struct> ts_struct;
  std::shared_ptr<rpc_union> ts_union;
};

struct rpc_const {
  string id;
  string val;
};

struct rpc_struct {
  string id;
  vec<rpc_decl> decls;
};

struct rpc_enum {
  string id;
  vec<rpc_const> tags;
};

struct rpc_ufield {
  vec<string> cases;
  rpc_decl decl;
  bool hasdefault{false};
  int fieldno{-1}; // 1, 2, 3, ... where default comes last
};

struct rpc_union {
  string id;
  string tagtype;
  string tagid;
  vec<rpc_ufield> fields;
  bool hasdefault{false};
};

struct rpc_proc {
  string id;
  uint32_t val;
  string arg;
  string res;
};

struct rpc_vers {
  string id;
  uint32_t val;
  vec<rpc_proc> procs;
};

struct rpc_program {
  string id;
  uint32_t val;
  vec<rpc_vers> vers;
};

struct rpc_namespace {
  string id;
  vec<rpc_program> progs;
};

struct rpc_sym {
  union {
    union_entry_base _base;
    union_entry<rpc_const> sconst;
    union_entry<rpc_decl> stypedef;
    union_entry<rpc_struct> sstruct;
    union_entry<rpc_enum> senum;
    union_entry<rpc_union> sunion;
    union_entry<rpc_program> sprogram;
    union_entry<string> sliteral;
    union_entry<rpc_namespace> snamespace;
  };

  enum symtype { CONST, STRUCT, UNION, ENUM, TYPEDEF, PROGRAM, LITERAL,
		 NAMESPACE } type;

  rpc_sym () : _base() {}
  rpc_sym (rpc_sym &&s) : _base(std::move(s._base)), type (s.type) {}
  ~rpc_sym () { _base.destroy (); }
#if 0
  rpc_sym (const rpc_sym &s) : _base(s._base), type (s.type) {}
private:
  rpc_sym &operator= (const rpc_sym &n)
    { type = n.type; _base = n._base; return *this; }
public:
#endif

  symtype gettype () const { return type; }

  void settype (symtype t) {
    switch (type = t) {
    case NAMESPACE:
      snamespace.select ();
      break;
    case CONST:
      sconst.select ();
      break;
    case STRUCT:
      sstruct.select ();
      break;
    case UNION:
      sunion.select ();
      break;
    case ENUM:
      senum.select ();
      break;
    case TYPEDEF:
      stypedef.select ();
      break;
    case PROGRAM:
      sprogram.select ();
      break;
    case LITERAL:
      sliteral.select ();
      break;
    }
  }
};

struct YYSTYPE {
  uint32_t num;
  struct rpc_decl decl;
  struct rpc_const cnst;
  string str;
};
extern YYSTYPE yylval;

using symlist_t = vec<rpc_sym>;
extern symlist_t symlist;

using strlist_t = vec<string>;
extern strlist_t litq;

rpc_program *get_prog (bool creat);
string strip_directory(string in);
string strip_dot_x(string in);
void gen_hh(std::ostream &os);

extern string input_file;
extern string output_file;

struct omanip : std::function<void(std::ostream&)> {
  using ostream = std::ostream;
  using fn_t = std::function<void(ostream&)>;
  using fn_t::function;
  template<typename T> omanip(T *t, void(T::*fn)(ostream &))
    : fn_t([t,fn](ostream &os) { (t->*fn)(os); }) {}
  friend ostream &operator<<(ostream &os, omanip &m) {
    m(os);
    return os;
  }
};

struct indenter : omanip {
  int level_{0};
  void do_indent(ostream &os) { os << std::endl << std::string(level_, ' '); }
  void do_open(ostream &os) { ++(*this); do_indent(os); }
  void do_close(ostream &os) { --(*this); do_indent(os); }
  void do_outdent(ostream &os) {
    os << std::endl << std::string(level_ > 2 ? level_ - 2 : 0, ' ');
  }

  indenter() : omanip(this, &indenter::do_indent) {}
  omanip open = omanip(this, &indenter::do_open);
  omanip close = omanip(this, &indenter::do_close);
  omanip outdent = omanip(this, &indenter::do_outdent);
  void operator++() { level_ += 2; }
  void operator--() { level_ -= 2; assert (level_ >= 0); }
};

