#include <string>

#include "bdb_env.h"

using namespace node;
using namespace v8;

struct eio_baton_t {
  eio_baton_t() : env(0), status(0) {}
  DbEnv *env;
  int status;
  v8::Persistent<v8::Function> cb;    
};

struct eio_open_baton_t: eio_baton_t {
  eio_open_baton_t(): eio_baton_t(), db_home(), flags(0), mode(0) {}
  std::string db_home;
  int flags;
  int mode;
};

DbEnv::DbEnv() {}

DbEnv::~DbEnv() {
  if(_env != NULL) _env->close(_env, 0);
}

DB_ENV *DbEnv::getDB_ENV() {
  return _env;
}

// Start EIO Methods

int DbEnv::EIO_Open(eio_req *req) {
  eio_open_baton_t *baton = static_cast<eio_open_baton_t *>(req->data);	

  DB_ENV *&env = baton->env->_env;
  if(env == NULL) {
	baton->status = db_env_create(&env, 0);
  }
  if(env != NULL && baton->status == 0) {
	baton->status = 
	  env->open(env,
				baton->db_home.empty() ? NULL : baton->db_home.c_str(),
				baton->flags,
				baton->mode);
  }

  return 0;
}

int DbEnv::EIO_AfterOpen(eio_req *req) {
  HandleScope scope;
  eio_open_baton_t *baton = static_cast<eio_open_baton_t *>(req->data);
  ev_unref(EV_DEFAULT_UC);
  baton->env->Unref();
  
  DB_RES(baton->status, db_strerror(baton->status), msg);
  Local<Value> argv[1] = { msg };
  
  TryCatch try_catch;
  
  baton->cb->Call(Context::GetCurrent()->Global(), 1, argv);
  
  if (try_catch.HasCaught()) {
	FatalException(try_catch);
  }
  
  baton->cb.Dispose();
  
  delete baton;
  return 0;
}


// Start V8 Exposed Methods

Handle<Value> DbEnv::Open(const Arguments& args) {
  HandleScope scope;
  
  DbEnv* env = ObjectWrap::Unwrap<DbEnv>(args.This());
  int flags = DEF_OPEN_FLAGS;
  int mode = 0;

  REQ_STR_ARG(0, db_home);
  REQ_FN_ARG(args.Length() - 1, cb);
  if(args.Length() > 2) {
	REQ_INT_ARG(1, tmp);
	flags = tmp->Value();
  }
  if(args.Length() > 3) {
	REQ_INT_ARG(2, tmp);
	mode = tmp->Value();
  }

  eio_open_baton_t *baton = new eio_open_baton_t();
  baton->env = env;
  baton->cb = Persistent<Function>::New(cb);
  baton->db_home = *db_home;
  baton->flags = flags;
  baton->mode = mode;
  
  env->Ref();  
  eio_custom(EIO_Open, EIO_PRI_DEFAULT, EIO_AfterOpen, baton);
  ev_ref(EV_DEFAULT_UC);
  
  return Undefined();
}


Handle<Value> DbEnv::New(const Arguments& args) {
  HandleScope scope;

  DbEnv* env = new DbEnv();
  env->Wrap(args.This());
  return args.This();
}

void DbEnv::Initialize(Handle<Object> target) {
  HandleScope scope;
  
  Local<FunctionTemplate> t = FunctionTemplate::New(New);
  t->InstanceTemplate()->SetInternalFieldCount(1);
  
  NODE_DEFINE_CONSTANT(target, DB_CREATE);
  NODE_DEFINE_CONSTANT(target, DB_INIT_CDB);
  NODE_DEFINE_CONSTANT(target, DB_INIT_LOCK);
  NODE_DEFINE_CONSTANT(target, DB_INIT_LOG);
  NODE_DEFINE_CONSTANT(target, DB_INIT_MPOOL);
  NODE_DEFINE_CONSTANT(target, DB_INIT_REP);
  NODE_DEFINE_CONSTANT(target, DB_INIT_TXN);
  
  NODE_SET_PROTOTYPE_METHOD(t, "open", Open);
  
  target->Set(String::NewSymbol("DbEnv"), t->GetFunction());
}
