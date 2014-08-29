
%{
#include "xdrc_internal.h"
#define YYSTYPE YYSTYPE

  //string xdr_unbounded = "xdr::XDR_MAX_LEN";
string xdr_unbounded = "";
static int proc_compare(const void *, const void *);
static int vers_compare(const void *, const void *);
static string getnewid(string, bool repeats_bad);
%}

%token <str> T_ID
%token <str> T_QID
%token <str> T_NUM

%token T_CONST
%token T_STRUCT
%token T_UNION
%token T_ENUM
%token T_TYPEDEF
%token T_PROGRAM
%token T_NAMESPACE

%token T_BOOL
%token T_UNSIGNED
%token T_INT
%token T_HYPER
%token T_FLOAT
%token T_DOUBLE
%token T_QUADRUPLE
%token T_VOID

%token T_VERSION
%token T_SWITCH
%token T_CASE
%token T_DEFAULT

%token <str> T_OPAQUE
%token <str> T_STRING

%type <str> id qid newid type_or_void type base_type value nsid
%type <decl> declaration
%type <cnst> enum_cnstag
%type <num> number

%%
file: /* empty */ { checkliterals (); }
	| file { checkliterals (); } definition { checkliterals (); }
	;

definition: def_const
	| def_enum
	| def_struct
	| def_type
	| def_union
        | def_program
	| def_namespace
	;

def_type: T_TYPEDEF declaration
	{
	  rpc_sym *s = &symlist.push_back ();
	  s->settype (rpc_sym::TYPEDEF);
	  *s->stypedef = $2;
	  s->stypedef->id = getnewid (s->stypedef->id, true);
	}
	| T_TYPEDEF T_STRUCT declaration
	{
	  rpc_sym *s = &symlist.push_back ();
	  s->settype (rpc_sym::TYPEDEF);
	  *s->stypedef = $3;
	  s->stypedef->type = string("struct ") + $3.type;
	  s->stypedef->id = getnewid (s->stypedef->id, true);
	}
	;

def_const: T_CONST newid '=' value ';'
	{
	  rpc_sym *s = &symlist.push_back ();
	  s->settype (rpc_sym::CONST);
	  s->sconst->id = $2;
	  s->sconst->val = $4;
	}
	;

def_enum: T_ENUM newid '{'
	{
	  rpc_sym *s = &symlist.push_back ();
	  s->settype (rpc_sym::ENUM);
	  s->senum->id = $2;
	}
	enum_taglist comma_warn '}' ';'
	;

comma_warn: /* empty */
	| ',' { yywarn ("comma not allowed at end of enum"); }
	;

def_struct: T_STRUCT newid '{'
	{
	  rpc_sym *s = &symlist.push_back ();
	  s->settype (rpc_sym::STRUCT);
	  s->sstruct->id = $2;
	}
	struct_decllist '}' ';'
	;

def_union: T_UNION newid T_SWITCH '(' type T_ID ')' '{'
	{
	  rpc_sym *s = &symlist.push_back ();
	  s->settype (rpc_sym::UNION);
	  s->sunion->id = $2;
	  s->sunion->tagtype = $5;
	  s->sunion->tagid = $6;
	  s->sunion->fields.push_back();
	}
	union_fieldlist '}' ';'
	;

def_program: T_PROGRAM newid '{'
	{
	  rpc_program *s = get_prog (true);
	  s->id = $2;
	}
	version_list '}' '=' number ';'
	{
	  rpc_program *s = get_prog (false);
	  s->val = $8;
	  qsort (s->vers.data(), s->vers.size(), 
	         sizeof(rpc_vers), vers_compare);
	}
	;

def_namespace: T_NAMESPACE nsid '{'
        {
	  rpc_sym *s = &symlist.push_back ();
	  s->settype (rpc_sym::NAMESPACE);
	  s->snamespace->id = $2;

        } 
	program_list '}' ';'
	;

program_list: def_program | program_list def_program
        ;

version_list: version_decl | version_list version_decl
	;

version_decl: T_VERSION newid '{'
	{
          rpc_program *p = get_prog (false);
	  rpc_vers *rv = &p->vers.push_back ();
	  rv->id = $2;
	}
	proc_list '}' '=' number ';'
	{
          rpc_program *p = get_prog (false);
	  rpc_vers *rv = &p->vers.back ();
	  rv->val = $8;
	  qsort (rv->procs.data(), rv->procs.size(),
		 sizeof(rpc_proc), proc_compare);
	}
	;

proc_list: proc_decl | proc_list proc_decl
	;

proc_decl: type_or_void newid '(' type_or_void ')' '=' number ';'
	{
          rpc_program *p = get_prog (false);
	  rpc_vers *rv = &p->vers.back ();
	  rpc_proc *rp = &rv->procs.push_back ();
	  rp->id = $2;
	  rp->val = $7;
	  rp->arg = $4;
	  rp->res = $1;
	}
	;

union_caselist: union_case | union_caselist union_case
	;

union_case: T_CASE value ':'
	{
	  rpc_union &u = *symlist.back().sunion;
	  rpc_ufield &uf = u.fields.back();
	  uf.cases.push_back($2);
	}
	| T_DEFAULT ':'
	{
	  rpc_union &u = *symlist.back().sunion;
	  rpc_ufield &uf = u.fields.back();
	  if (u.hasdefault) {
	    yyerror("duplicate default statement");
	    YYERROR;
	  }
	  uf.cases.push_back("");
	  u.hasdefault = uf.hasdefault = true;
	}
	;

union_decl: declaration
	{
	  rpc_union &u = *symlist.back().sunion;
	  rpc_ufield &uf = u.fields.back();
	  uf.decl = $1;
	}
	| T_VOID ';'
	{
	  rpc_union &u = *symlist.back().sunion;
	  rpc_decl &ud = u.fields.back().decl;
	  ud.type = "void";
	  ud.qual = rpc_decl::SCALAR;
	}
	;

union_field: union_caselist union_decl
	;

union_fieldlist: union_field
        | union_fieldlist
        {
	  rpc_union &u = *symlist.back().sunion;
	  u.fields.push_back();
	}
        union_field
        ;

struct_decllist: struct_decl | struct_decllist struct_decl
	;

struct_decl: declaration
	{ symlist.back ().sstruct->decls.push_back ($1); }
	;

enum_taglist: enum_tag {}
	| enum_taglist ',' enum_tag {}
	;

enum_tag: enum_cnstag
	{ symlist.back ().senum->tags.push_back ($1); }
	;

enum_cnstag: newid '=' value { $$.id = $1; $$.val = $3; }
	| newid { $$.id = $1; }
	;

declaration: type T_ID ';'
	 { $$.id = $2; $$.type = $1; $$.qual = rpc_decl::SCALAR; }
	| T_STRING T_ID ';'
	 { $$.id = $2; $$.type = $1; $$.qual = rpc_decl::VEC;
	   $$.bound = xdr_unbounded;
	   yywarn ("strings require variable-length array declarations (<>)");
	 }
	| type '*' T_ID ';'
	 { $$.id = $3; $$.type = $1; $$.qual = rpc_decl::PTR; }
	| type T_ID '[' value ']' ';'
	 { $$.id = $2; $$.type = $1; $$.qual = rpc_decl::ARRAY;
	   $$.bound = $4; }
	| T_OPAQUE T_ID '[' value ']' ';'
	 { $$.id = $2; $$.type = $1; $$.qual = rpc_decl::ARRAY;
	   $$.bound = $4; }
	| type T_ID '<' value '>' ';'
	 { $$.id = $2; $$.type = $1; $$.qual = rpc_decl::VEC; $$.bound = $4; }
	| T_STRING T_ID '<' value '>' ';'
	 { $$.id = $2; $$.type = $1; $$.qual = rpc_decl::VEC; $$.bound = $4; }
	| T_OPAQUE T_ID '<' value '>' ';'
	 { $$.id = $2; $$.type = $1; $$.qual = rpc_decl::VEC; $$.bound = $4; }
	| type T_ID '<' '>' ';'
	 { $$.id = $2; $$.type = $1; $$.qual = rpc_decl::VEC;
	   $$.bound = xdr_unbounded; }
	| T_STRING T_ID '<' '>' ';'
	 { $$.id = $2; $$.type = $1; $$.qual = rpc_decl::VEC;
	   $$.bound = xdr_unbounded; }
	| T_OPAQUE T_ID '<' '>' ';'
	 { $$.id = $2; $$.type = $1; $$.qual = rpc_decl::VEC;
	   $$.bound = xdr_unbounded; }
	;

type_or_void: type | T_VOID { $$ = "void"; }
	;

type: base_type | qid
	;

base_type: T_UNSIGNED { $$ = "unsigned"; }
	| T_INT { $$ = "int"; }
	| T_UNSIGNED T_INT { $$ = "unsigned"; }
	| T_HYPER { $$ = "hyper"; }
	| T_UNSIGNED T_HYPER { $$ = "unsigned hyper"; }
	| T_FLOAT { $$ = "float"; }
	| T_DOUBLE { $$ = "double"; }
	| T_QUADRUPLE { $$ = "quadruple"; }
	| T_BOOL { $$ = "bool"; }
	;

value: id | T_NUM
	;

number: T_NUM { $$ = strtoul ($1.c_str(), NULL, 0); }
	;

newid: T_ID { $$ = getnewid ($1, true); }
	;

nsid: T_ID { $$ = getnewid ($1, false); } 
        ;

id: T_ID
	;

qid: T_ID | T_QID
	;

%%
symlist_t symlist;

static int
proc_compare(const void *_a, const void *_b)
{
  rpc_proc *a = (rpc_proc *) _a;
  rpc_proc *b = (rpc_proc *) _b;
  return a->val < b->val ? -1 : a->val != b->val;
}

static int
vers_compare(const void *_a, const void *_b)
{
  rpc_vers *a = (rpc_vers *) _a;
  rpc_vers *b = (rpc_vers *) _b;
  return a->val < b->val ? -1 : a->val != b->val;
}

void
checkliterals()
{
  for (size_t i = 0; i < litq.size (); i++) {
    rpc_sym *s = &symlist.push_back ();
    s->settype (rpc_sym::LITERAL);
    *s->sliteral = litq[i];
  }
  litq.clear ();
}

static string
getnewid(string id, bool repeats_bad)
{
  if (!repeats_bad) { /* noop */ }
  else if (ids.find(id) != ids.end()) {
    yywarn(string("redefinition of symbol ") + id);
  } else {
    ids.insert(id);
  }
  // Possible place to add namespace scope::
  return id;
}
