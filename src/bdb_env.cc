// Copyright 2011 Mark Cavage <mcavage@gmail.com> All rights reserved.
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "bdb_common.h"
#include "bdb_env.h"

using v8::FunctionTemplate;

class EIOCheckpointBaton: public EIOBaton {
 public:
  explicit EIOCheckpointBaton(DbEnv *env):
      EIOBaton(env), kbytes(0), minutes(0) {}
  virtual ~EIOCheckpointBaton() {}
  unsigned int kbytes;
  unsigned int minutes;
 private:
  EIOCheckpointBaton(const EIOCheckpointBaton &);
  EIOCheckpointBaton &operator=(const EIOCheckpointBaton &);
};


DbEnv::DbEnv(): DbObject(), _transactional(false), _env(0) {}

DbEnv::~DbEnv() {
  if (_env != NULL) {
    _env->close(_env, 0);
    _env = NULL;
  }
}

DB_ENV *&DbEnv::getDB_ENV() {
  return _env;
}

bool DbEnv::isTransactional() {
  return _transactional;
}

// Start EIO Exposed Methonds

int DbEnv::EIO_Checkpoint(eio_req *req) {
  EIOCheckpointBaton *baton = static_cast<EIOCheckpointBaton *>(req->data);

  if (baton->object == NULL ||
      dynamic_cast<DbEnv *>(baton->object)->_env == NULL) {
    return 0;
  }

  DB_ENV *&env = dynamic_cast<DbEnv *>(baton->object)->_env;

  baton->status =
      env->txn_checkpoint(env, baton->kbytes, baton->minutes, baton->flags);

  return 0;
}

// Start V8 Exposed Methods

v8::Handle<v8::Value> DbEnv::New(const v8::Arguments& args) {
  v8::HandleScope scope;

  DbEnv* env = new DbEnv();
  if (env == NULL)
    RET_EXC("Unable to create a new DbEnv (out of memory)");

  int rc = db_env_create(&(env->_env), 0);
  if (rc != 0)
    RET_EXC(db_strerror(rc));

  env->Wrap(args.This());
  return args.This();
}

v8::Handle<v8::Value> DbEnv::CloseS(const v8::Arguments &args) {
  v8::HandleScope scope;

  DbEnv* env = node::ObjectWrap::Unwrap<DbEnv>(args.This());

  REQ_INT_ARG(0, flags);

  int rc = env->_env->close(env->_env, flags);

  DB_RES(rc, db_strerror(rc), msg);
  return msg;
}

v8::Handle<v8::Value> DbEnv::OpenS(const v8::Arguments &args) {
  v8::HandleScope scope;

  DbEnv* env = node::ObjectWrap::Unwrap<DbEnv>(args.This());

  REQ_STR_ARG(0, db_home);
  REQ_INT_ARG(1, flags);
  REQ_INT_ARG(2, mode);

  env->_transactional = (flags & DB_INIT_TXN);
  int rc = env->_env->open(env->_env, *db_home, flags, mode);

  DB_RES(rc, db_strerror(rc), msg);
  return msg;
}

v8::Handle<v8::Value> DbEnv::SetEncrypt(const v8::Arguments& args) {
  v8::HandleScope scope;

  DbEnv* env = node::ObjectWrap::Unwrap<DbEnv>(args.This());
  REQ_STR_ARG(0, passwd);

  int rc = env->_env->set_encrypt(env->_env, *passwd, DB_ENCRYPT_AES);

  DB_RES(rc, db_strerror(rc), msg);
  return msg;
}

v8::Handle<v8::Value> DbEnv::SetErrorFile(const v8::Arguments &args) {
  v8::HandleScope scope;

  DbEnv* env = node::ObjectWrap::Unwrap<DbEnv>(args.This());
  REQ_STR_ARG(0, fname);

  FILE *fp = fopen(*fname, "a");
  int rc = 0;
  if (fp != NULL) {
    env->_env->set_errfile(env->_env, fp);
  } else {
    rc = errno;
  }

  DB_RES(rc, db_strerror(rc), msg);
  return msg;
}

v8::Handle<v8::Value> DbEnv::SetErrorPrefix(const v8::Arguments &args) {
  v8::HandleScope scope;

  DbEnv* env = node::ObjectWrap::Unwrap<DbEnv>(args.This());
  REQ_STR_ARG(0, prefix);

  // Yes, this is a memory leak, but BDB is just really stupid about this...
  env->_env->set_errpfx(env->_env, strdup(*prefix));

  DB_RES(0, db_strerror(0), msg);
  return msg;
}

v8::Handle<v8::Value> DbEnv::SetFlags(const v8::Arguments& args) {
  v8::HandleScope scope;

  DbEnv* env = node::ObjectWrap::Unwrap<DbEnv>(args.This());
  REQ_INT_ARG(0, flags);
  REQ_INT_ARG(1, onoff);

  int rc = env->_env->set_flags(env->_env, flags, onoff);

  DB_RES(rc, db_strerror(rc), msg);
  return msg;
}


v8::Handle<v8::Value> DbEnv::SetLockDetect(const v8::Arguments &args) {
  v8::HandleScope scope;

  DbEnv* env = node::ObjectWrap::Unwrap<DbEnv>(args.This());

  REQ_INT_ARG(0, policy);

  int rc = env->_env->set_lk_detect(env->_env, policy);

  DB_RES(rc, db_strerror(rc), msg);
  return msg;
}

v8::Handle<v8::Value> DbEnv::SetLockTimeout(const v8::Arguments &args) {
  v8::HandleScope scope;

  DbEnv* env = node::ObjectWrap::Unwrap<DbEnv>(args.This());

  REQ_INT_ARG(0, timeout);

  int rc = env->_env->set_timeout(env->_env, timeout, DB_SET_LOCK_TIMEOUT);

  DB_RES(rc, db_strerror(rc), msg);
  return msg;
}

v8::Handle<v8::Value> DbEnv::SetMaxLocks(const v8::Arguments &args) {
  v8::HandleScope scope;

  DbEnv* env = node::ObjectWrap::Unwrap<DbEnv>(args.This());

  REQ_INT_ARG(0, max);

  int rc = env->_env->set_lk_max_locks(env->_env, max);

  DB_RES(rc, db_strerror(rc), msg);
  return msg;
}

v8::Handle<v8::Value> DbEnv::SetMaxLockers(const v8::Arguments &args) {
  v8::HandleScope scope;

  DbEnv* env = node::ObjectWrap::Unwrap<DbEnv>(args.This());

  REQ_INT_ARG(0, max);

  int rc = env->_env->set_lk_max_lockers(env->_env, max);

  DB_RES(rc, db_strerror(rc), msg);
  return msg;
}

v8::Handle<v8::Value> DbEnv::SetMaxLockObjects(const v8::Arguments &args) {
  v8::HandleScope scope;

  DbEnv* env = node::ObjectWrap::Unwrap<DbEnv>(args.This());

  REQ_INT_ARG(0, max);

  int rc = env->_env->set_lk_max_objects(env->_env, max);

  DB_RES(rc, db_strerror(rc), msg);
  return msg;
}

v8::Handle<v8::Value> DbEnv::SetShmKey(const v8::Arguments &args) {
  v8::HandleScope scope;

  DbEnv* env = node::ObjectWrap::Unwrap<DbEnv>(args.This());
  REQ_INT_ARG(0, shmKey);

  int rc = env->_env->set_shm_key(env->_env, shmKey);
  DB_RES(rc, db_strerror(rc), msg);
  return msg;
}

v8::Handle<v8::Value> DbEnv::SetTxnMax(const v8::Arguments &args) {
  v8::HandleScope scope;

  DbEnv* env = node::ObjectWrap::Unwrap<DbEnv>(args.This());

  REQ_INT_ARG(0, max);

  int rc = env->_env->set_tx_max(env->_env, max);

  DB_RES(rc, db_strerror(rc), msg);
  return msg;
}

v8::Handle<v8::Value> DbEnv::SetTxnTimeout(const v8::Arguments &args) {
  v8::HandleScope scope;

  DbEnv* env = node::ObjectWrap::Unwrap<DbEnv>(args.This());

  REQ_INT_ARG(0, timeout);

  int rc = env->_env->set_timeout(env->_env, timeout, DB_SET_TXN_TIMEOUT);

  DB_RES(rc, db_strerror(rc), msg);
  return msg;
}


v8::Handle<v8::Value> DbEnv::TxnCheckpoint(const v8::Arguments& args) {
  v8::HandleScope scope;

  DbEnv* env = node::ObjectWrap::Unwrap<DbEnv>(args.This());

  REQ_INT_ARG(0, kbytes);
  REQ_INT_ARG(1, minutes);
  REQ_INT_ARG(2, flags);
  REQ_FN_ARG(3, cb);

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

  NODE_SET_PROTOTYPE_METHOD(t, "_closeSync", CloseS);
  NODE_SET_PROTOTYPE_METHOD(t, "_openSync", OpenS);
  NODE_SET_PROTOTYPE_METHOD(t, "setEncrypt", SetEncrypt);
  NODE_SET_PROTOTYPE_METHOD(t, "setErrorFile", SetErrorFile);
  NODE_SET_PROTOTYPE_METHOD(t, "setErrorPrefix", SetErrorPrefix);
  NODE_SET_PROTOTYPE_METHOD(t, "setFlags", SetFlags);
  NODE_SET_PROTOTYPE_METHOD(t, "setLockDetect", SetLockDetect);
  NODE_SET_PROTOTYPE_METHOD(t, "setLockTimeout", SetLockTimeout);
  NODE_SET_PROTOTYPE_METHOD(t, "setMaxLocks", SetMaxLocks);
  NODE_SET_PROTOTYPE_METHOD(t, "setMaxLockers", SetMaxLockers);
  NODE_SET_PROTOTYPE_METHOD(t, "setMaxLockObjects", SetMaxLockObjects);
  NODE_SET_PROTOTYPE_METHOD(t, "setShmKey", SetShmKey);
  NODE_SET_PROTOTYPE_METHOD(t, "setTxnMax", SetTxnMax);
  NODE_SET_PROTOTYPE_METHOD(t, "setTxnTimeout", SetTxnTimeout);
  NODE_SET_PROTOTYPE_METHOD(t, "_txnCheckpoint", TxnCheckpoint);

  target->Set(v8::String::NewSymbol("DbEnv"), t->GetFunction());
}
