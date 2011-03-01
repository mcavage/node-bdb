// Copyright 2001 Mark Cavage <mark@bluesnoop.com> Sleepycat License
#include <string>

#include "bdb_common.h"
#include "bdb_env.h"
#include "bdb_txn.h"

using v8::FunctionTemplate;

class EIOCheckpointBaton: public EIOBaton {
 public:
  EIOCheckpointBaton(DbEnv *env): EIOBaton(env), kbytes(0), minutes(0) {}
  virtual ~EIOCheckpointBaton() {}
  unsigned int kbytes;
  unsigned int minutes;
 private:
  EIOCheckpointBaton(const EIOCheckpointBaton &);
  EIOCheckpointBaton &operator=(const EIOCheckpointBaton &);
};


DbEnv::DbEnv(): DbObject(), _env(0) {}

DbEnv::~DbEnv() {
  if(_env != NULL) {
    _env->close(_env, 0);
    _env = NULL;
  }
}

DB_ENV *&DbEnv::getDB_ENV() {
  return _env;
}

// Start EIO Exposed Methonds

int DbEnv::EIO_Checkpoint(eio_req *req) {
  EIOCheckpointBaton *baton = static_cast<EIOCheckpointBaton *>(req->data);

  if (baton->object == NULL || dynamic_cast<DbEnv *>(baton->object)->_env == NULL)
    return 0;

  DB_ENV *&env = dynamic_cast<DbEnv *>(baton->object)->_env;

  baton->status =
      env->txn_checkpoint(env, baton->kbytes, baton->minutes, baton->flags);

  return 0;
}

// Start V8 Exposed Methods

v8::Handle<v8::Value> DbEnv::New(const v8::Arguments& args) {
  v8::HandleScope scope;

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
  v8::HandleScope scope;

  DbEnv* env = node::ObjectWrap::Unwrap<DbEnv>(args.This());
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
  return msg;
}

v8::Handle<v8::Value> DbEnv::SetLockDetect(const v8::Arguments &args) {
  v8::HandleScope scope;

  DbEnv* env = node::ObjectWrap::Unwrap<DbEnv>(args.This());

  REQ_INT_ARG(0, tmp);
  int policy = tmp->Value();

  int rc = env->_env->set_lk_detect(env->_env, policy);

  DB_RES(rc, db_strerror(rc), msg);
  return msg;
}

v8::Handle<v8::Value> DbEnv::SetLockTimeout(const v8::Arguments &args) {
  v8::HandleScope scope;

  DbEnv* env = node::ObjectWrap::Unwrap<DbEnv>(args.This());

  REQ_INT_ARG(0, tmp);
  int timeout = tmp->Value();

  int rc = env->_env->set_timeout(env->_env, timeout, DB_SET_LOCK_TIMEOUT);

  DB_RES(rc, db_strerror(rc), msg);
  return msg;
}

v8::Handle<v8::Value> DbEnv::SetMaxLocks(const v8::Arguments &args) {
  v8::HandleScope scope;

  DbEnv* env = node::ObjectWrap::Unwrap<DbEnv>(args.This());

  REQ_INT_ARG(0, tmp);
  int max = tmp->Value();

  int rc = env->_env->set_lk_max_locks(env->_env, max);

  DB_RES(rc, db_strerror(rc), msg);
  return msg;
}

v8::Handle<v8::Value> DbEnv::SetMaxLockers(const v8::Arguments &args) {
  v8::HandleScope scope;

  DbEnv* env = node::ObjectWrap::Unwrap<DbEnv>(args.This());

  REQ_INT_ARG(0, tmp);
  int max = tmp->Value();

  int rc = env->_env->set_lk_max_lockers(env->_env, max);

  DB_RES(rc, db_strerror(rc), msg);
  return msg;
}

v8::Handle<v8::Value> DbEnv::SetMaxLockObjects(const v8::Arguments &args) {
  v8::HandleScope scope;

  DbEnv* env = node::ObjectWrap::Unwrap<DbEnv>(args.This());

  REQ_INT_ARG(0, tmp);
  int max = tmp->Value();

  int rc = env->_env->set_lk_max_objects(env->_env, max);

  DB_RES(rc, db_strerror(rc), msg);
  return msg;
}

v8::Handle<v8::Value> DbEnv::SetShmKey(const v8::Arguments &args) {
  v8::HandleScope scope;

  DbEnv* env = node::ObjectWrap::Unwrap<DbEnv>(args.This());
  REQ_INT_ARG(0, shmKey);

  if(env->_env == NULL);

  int rc = env->_env->set_shm_key(env->_env, shmKey->Value());
  DB_RES(rc, db_strerror(rc), msg);
  return msg;
}

v8::Handle<v8::Value> DbEnv::SetTxnMax(const v8::Arguments &args) {
  v8::HandleScope scope;

  DbEnv* env = node::ObjectWrap::Unwrap<DbEnv>(args.This());

  REQ_INT_ARG(0, tmp);
  int max = tmp->Value();

  int rc = env->_env->set_tx_max(env->_env, max);

  DB_RES(rc, db_strerror(rc), msg);
  return msg;
}

v8::Handle<v8::Value> DbEnv::SetTxnTimeout(const v8::Arguments &args) {
  v8::HandleScope scope;

  DbEnv* env = node::ObjectWrap::Unwrap<DbEnv>(args.This());

  REQ_INT_ARG(0, tmp);
  int timeout = tmp->Value();

  int rc = env->_env->set_timeout(env->_env, timeout, DB_SET_TXN_TIMEOUT);

  DB_RES(rc, db_strerror(rc), msg);
  return msg;
}

v8::Handle<v8::Value> DbEnv::TxnBegin(const v8::Arguments &args) {
  v8::HandleScope scope;

  DbEnv* env = node::ObjectWrap::Unwrap<DbEnv>(args.This());

  int flags = 0;
  REQ_OBJ_ARG(0, txnObj);
  if(args.Length() > 1) {
	REQ_INT_ARG(1, tmp);
	flags = tmp->Value();
  }
  DbTxn *txn = node::ObjectWrap::Unwrap<DbTxn>(txnObj);
  DB_TXN *&dbTxn = txn->getDB_TXN();

  int rc = env->_env->txn_begin(env->_env,
				NULL,  // TODO(mcavage), nested txns
				&dbTxn,
				flags);

  DB_RES(rc, db_strerror(rc), msg);
  return msg;
}

v8::Handle<v8::Value> DbEnv::TxnCheckpoint(const v8::Arguments& args) {
  v8::HandleScope scope;

  DbEnv* env = node::ObjectWrap::Unwrap<DbEnv>(args.This());

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

  EIOCheckpointBaton *baton = new EIOCheckpointBaton(env);
  baton->cb = v8::Persistent<v8::Function>::New(cb);
  baton->flags = flags;
  baton->kbytes = kbytes;
  baton->minutes = minutes;

  env->Ref();
  eio_custom(EIO_Checkpoint, EIO_PRI_DEFAULT, EIO_After_ReturnStatus, baton);
  ev_ref(EV_DEFAULT_UC);

  return v8::Undefined();
}


void DbEnv::Initialize(v8::Handle<v8::Object> target) {
  v8::HandleScope scope;

  v8::Local<v8::FunctionTemplate> t = v8::FunctionTemplate::New(New);
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

  target->Set(v8::String::NewSymbol("DbEnv"), t->GetFunction());
}
