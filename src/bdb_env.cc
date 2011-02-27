#include <string>

#include "bdb_env.h"
#include "bdb_txn.h"

using namespace node;
using namespace v8;

struct eio_baton_t {
  eio_baton_t() : env(0), status(0), flags(0) {}
  DbEnv *env;
  int status;
  int flags;
  v8::Persistent<v8::Function> cb;
};

struct eio_checkpoint_baton_t: eio_baton_t {
  eio_checkpoint_baton_t() : eio_baton_t(), kbytes(0), minutes(0) {}
  unsigned int kbytes;
  unsigned int minutes;
};


DbEnv::DbEnv() : _env(0) {}

DbEnv::~DbEnv() {
  if(_env != NULL)
    _env->close(_env, 0);
}

DB_ENV *&DbEnv::getDB_ENV() {
  return _env;
}

// Start EIO Exposed Methonds

int DbEnv::EIO_Checkpoint(eio_req *req) {
  eio_checkpoint_baton_t *baton =
    static_cast<eio_checkpoint_baton_t *>(req->data);

  DB_ENV *&env = baton->env->_env;

  baton->status =
    env->txn_checkpoint(env, baton->kbytes, baton->minutes, baton->flags);

  return 0;
}

int DbEnv::EIO_AfterCheckpoint(eio_req *req) {
  HandleScope scope;
  eio_checkpoint_baton_t *baton =
    static_cast<eio_checkpoint_baton_t *>(req->data);
  ev_unref(EV_DEFAULT_UC);

  DB_RES(baton->status, db_strerror(baton->status), msg);
  Local<Value> argv[1] = { msg };

  TryCatch try_catch;

  baton->cb->Call(Context::GetCurrent()->Global(), 1, argv);

  if (try_catch.HasCaught()) {
    FatalException(try_catch);
  }

  baton->env->Unref();
  baton->cb.Dispose();
  delete baton;
  return 0;
}

// Start V8 Exposed Methods

Handle<Value> DbEnv::New(const Arguments& args) {
  HandleScope scope;

  DbEnv* env = new DbEnv();
  if(env == NULL)
    RET_EXC("Unable to create a new DbEnv (out of memory)");

  int rc = db_env_create(&(env->_env), 0);
  if(rc != 0)
    RET_EXC(db_strerror(rc));

  env->Wrap(args.This());
  return args.This();
}

v8::Handle<v8::Value> DbEnv::Open(const v8::Arguments &args) {
  HandleScope scope;

  DbEnv* env = ObjectWrap::Unwrap<DbEnv>(args.This());
  int flags = DEF_OPEN_FLAGS;
  int mode = 0;

  REQ_STR_ARG(0, db_home);
  if(args.Length() > 1) {
    REQ_INT_ARG(1, tmp);
    flags = tmp->Value();
  }
  if(args.Length() > 2) {
    REQ_INT_ARG(2, tmp);
    mode = tmp->Value();
  }

  int rc = env->_env->open(env->_env, *db_home, flags, mode);

  DB_RES(rc, db_strerror(rc), msg);
  Handle<Value> result = { msg };
  return result;
}

v8::Handle<v8::Value> DbEnv::SetLockDetect(const v8::Arguments &args) {
  HandleScope scope;

  DbEnv* env = ObjectWrap::Unwrap<DbEnv>(args.This());

  REQ_INT_ARG(0, tmp);
  int policy = tmp->Value();

  int rc = env->_env->set_lk_detect(env->_env, policy);

  DB_RES(rc, db_strerror(rc), msg);
  Handle<Value> result = { msg };
  return result;
}

v8::Handle<v8::Value> DbEnv::SetLockTimeout(const v8::Arguments &args) {
  HandleScope scope;

  DbEnv* env = ObjectWrap::Unwrap<DbEnv>(args.This());

  REQ_INT_ARG(0, tmp);
  int timeout = tmp->Value();

  int rc = env->_env->set_timeout(env->_env, timeout, DB_SET_LOCK_TIMEOUT);

  DB_RES(rc, db_strerror(rc), msg);
  Handle<Value> result = { msg };
  return result;
}

v8::Handle<v8::Value> DbEnv::SetMaxLocks(const v8::Arguments &args) {
  HandleScope scope;

  DbEnv* env = ObjectWrap::Unwrap<DbEnv>(args.This());

  REQ_INT_ARG(0, tmp);
  int max = tmp->Value();

  int rc = env->_env->set_lk_max_locks(env->_env, max);

  DB_RES(rc, db_strerror(rc), msg);
  Handle<Value> result = { msg };
  return result;
}

v8::Handle<v8::Value> DbEnv::SetMaxLockers(const v8::Arguments &args) {
  HandleScope scope;

  DbEnv* env = ObjectWrap::Unwrap<DbEnv>(args.This());

  REQ_INT_ARG(0, tmp);
  int max = tmp->Value();

  int rc = env->_env->set_lk_max_lockers(env->_env, max);

  DB_RES(rc, db_strerror(rc), msg);
  Handle<Value> result = { msg };
  return result;
}

v8::Handle<v8::Value> DbEnv::SetMaxLockObjects(const v8::Arguments &args) {
  HandleScope scope;

  DbEnv* env = ObjectWrap::Unwrap<DbEnv>(args.This());

  REQ_INT_ARG(0, tmp);
  int max = tmp->Value();

  int rc = env->_env->set_lk_max_objects(env->_env, max);

  DB_RES(rc, db_strerror(rc), msg);
  Handle<Value> result = { msg };
  return result;
}

v8::Handle<v8::Value> DbEnv::SetShmKey(const v8::Arguments &args) {
  HandleScope scope;

  DbEnv* env = ObjectWrap::Unwrap<DbEnv>(args.This());
  REQ_INT_ARG(0, shmKey);

  if(env->_env == NULL);

  int rc = env->_env->set_shm_key(env->_env, shmKey->Value());
  DB_RES(rc, db_strerror(rc), msg);
  Handle<Value> result = { msg };
  return result;
}

v8::Handle<v8::Value> DbEnv::SetTxnMax(const v8::Arguments &args) {
  HandleScope scope;

  DbEnv* env = ObjectWrap::Unwrap<DbEnv>(args.This());

  REQ_INT_ARG(0, tmp);
  int max = tmp->Value();

  int rc = env->_env->set_tx_max(env->_env, max);

  DB_RES(rc, db_strerror(rc), msg);
  Handle<Value> result = { msg };
  return result;
}

v8::Handle<v8::Value> DbEnv::SetTxnTimeout(const v8::Arguments &args) {
  HandleScope scope;

  DbEnv* env = ObjectWrap::Unwrap<DbEnv>(args.This());

  REQ_INT_ARG(0, tmp);
  int timeout = tmp->Value();

  int rc = env->_env->set_timeout(env->_env, timeout, DB_SET_TXN_TIMEOUT);

  DB_RES(rc, db_strerror(rc), msg);
  Handle<Value> result = { msg };
  return result;
}

v8::Handle<v8::Value> DbEnv::TxnBegin(const v8::Arguments &args) {
  HandleScope scope;

  DbEnv* env = ObjectWrap::Unwrap<DbEnv>(args.This());

  int flags = 0;
  REQ_OBJ_ARG(0, txnObj);
  if(args.Length() > 1) {
	REQ_INT_ARG(1, tmp);
	flags = tmp->Value();
  }
  DbTxn *txn = node::ObjectWrap::Unwrap<DbTxn>(txnObj);
  DB_TXN *&dbTxn = txn->getDB_TXN();

  int rc = env->_env->txn_begin(env->_env,
				NULL, // parent, todo
				&dbTxn,
				flags);

  DB_RES(rc, db_strerror(rc), msg);
  Handle<Value> result = { msg };
  return result;
}

Handle<Value> DbEnv::TxnCheckpoint(const Arguments& args) {
  HandleScope scope;

  DbEnv* env = ObjectWrap::Unwrap<DbEnv>(args.This());

  int kbytes = 0;
  int minutes = 0;
  int flags = 0;

  REQ_FN_ARG(args.Length() - 1, cb);

  if(args.Length() > 1) {
    REQ_INT_ARG(0, tmp);
    kbytes = tmp->Value();
  }
  if(args.Length() > 2) {
    REQ_INT_ARG(1, tmp);
    minutes = tmp->Value();
  }
  if(args.Length() > 3) {
    REQ_INT_ARG(2, tmp);
    flags = tmp->Value();
  }

  eio_checkpoint_baton_t *baton = new eio_checkpoint_baton_t();
  baton->env = env;
  baton->cb = Persistent<Function>::New(cb);
  baton->kbytes = kbytes;
  baton->minutes = minutes;
  baton->flags = flags;

  env->Ref();
  eio_custom(EIO_Checkpoint, EIO_PRI_DEFAULT, EIO_AfterCheckpoint, baton);
  ev_ref(EV_DEFAULT_UC);

  return Undefined();
}


void DbEnv::Initialize(Handle<Object> target) {
  HandleScope scope;

  Local<FunctionTemplate> t = FunctionTemplate::New(New);
  t->InstanceTemplate()->SetInternalFieldCount(1);

  NODE_SET_PROTOTYPE_METHOD(t, "open", Open);
  NODE_SET_PROTOTYPE_METHOD(t, "setLockTimeout", SetLockTimeout);
  NODE_SET_PROTOTYPE_METHOD(t, "setMaxLocks", SetMaxLocks);
  NODE_SET_PROTOTYPE_METHOD(t, "setMaxLockers", SetMaxLockers);
  NODE_SET_PROTOTYPE_METHOD(t, "setMaxLockObjects", SetMaxLockObjects);
  NODE_SET_PROTOTYPE_METHOD(t, "setShmKey", SetShmKey);
  NODE_SET_PROTOTYPE_METHOD(t, "setTxnMax", SetTxnMax);
  NODE_SET_PROTOTYPE_METHOD(t, "setTxnTimeout", SetTxnTimeout);
  NODE_SET_PROTOTYPE_METHOD(t, "txnBegin", TxnBegin);
  NODE_SET_PROTOTYPE_METHOD(t, "txnCheckpoint", TxnCheckpoint);

  target->Set(String::NewSymbol("DbEnv"), t->GetFunction());
}
