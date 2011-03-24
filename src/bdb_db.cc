// Copyright 2011 Mark Cavage <mcavage@gmail.com> All rights reserved.
#include <sched.h>
#include <stdlib.h>
#include <string.h>

#include <node_buffer.h>

#include "bdb_common.h"
#include "bdb_db.h"
#include "bdb_env.h"

#define TXN_BEGIN(DBOBJ)                         \
  DB_ENV *&_env = DBOBJ->_env;                   \
  DB_TXN *_txn = NULL;                           \
  int _attempts = 0;                             \
  int _rc = 0;                                   \
again:                                           \
  if (DBOBJ->_transactional) {                   \
    _rc = _env->txn_begin(_env, NULL, &_txn, 0); \
    if (_rc != 0)                                \
      goto out;                                  \
  }                                              \


#define TXN_END(DBOBJ, STATUS)                                          \
  if (DBOBJ->_transactional) {                                          \
    if (STATUS == 0) {                                                  \
      STATUS = _txn->commit(_txn, 0);                                   \
    } else if (STATUS == DB_LOCK_DEADLOCK &&                            \
               ++_attempts <= DBOBJ->_retries) {                        \
      _txn->abort(_txn);                                                \
      sched_yield();                                                    \
      goto again;                                                       \
    } else {                                                            \
      _txn->abort(_txn);                                                \
    }                                                                   \
  }                                                                     \
 out:


#define INIT_DBT(NAME, LEN)                              \
  DBT dbt_ ## NAME = {0};                                \
  memset(&dbt_ ## NAME, 0, sizeof(DBT));                 \
  dbt_ ## NAME.data = NAME;                              \
  dbt_ ## NAME.size = LEN

using v8::FunctionTemplate;
using v8::Persistent;
using v8::String;

v8::Persistent<v8::String> val_sym;

class EIODbBaton: public EIOBaton {
 public:
  explicit EIODbBaton(Db *db): EIOBaton(db), env(0) {
    memset(&key, 0, sizeof(DBT));
    memset(&val, 0, sizeof(DBT));
  }

  virtual ~EIODbBaton() {}

  DbEnv *env;
  DBT key;
  DBT val;

 private:
  EIODbBaton(const EIODbBaton &);
  EIODbBaton &operator=(const EIODbBaton &);
};


Db::Db(): DbObject(), _db(0), _env(0), _retries(0), _transactional(false) {}

Db::~Db() {
  if (_db != NULL) {
    _db->close(_db, 0);
    _db = NULL;
  }
}

// Start EIO Methods

int Db::EIO_Get(eio_req *req) {
  EIODbBaton *baton = static_cast<EIODbBaton *>(req->data);
  if (baton->object == NULL || dynamic_cast<Db *>(baton->object)->_db == NULL)
    return 0;

  Db *dbObj = dynamic_cast<Db *>(baton->object);
  DB *&db = dbObj->_db;

  baton->val.flags = DB_DBT_MALLOC;

  TXN_BEGIN(dbObj);

  baton->status = db->get(db, _txn, &(baton->key), &(baton->val), baton->flags);

  TXN_END(dbObj, baton->status);

  return 0;
}

int Db::EIO_AfterGet(eio_req *req) {
  v8::HandleScope scope;
  EIODbBaton *baton = static_cast<EIODbBaton *>(req->data);
  ev_unref(EV_DEFAULT_UC);

  DB_RES(baton->status, db_strerror(baton->status), msg);
  node::Buffer *buf = node::Buffer::New(baton->val.size);
  memcpy(node::Buffer::Data(buf), baton->val.data, baton->val.size);

  v8::Handle<v8::Value> argv[2] = {};
  argv[0] = msg;
  argv[1] = buf->handle_;

  v8::TryCatch try_catch;

  baton->cb->Call(v8::Context::GetCurrent()->Global(), 2, argv);

  if (try_catch.HasCaught())
    node::FatalException(try_catch);

  baton->object->Unref();
  baton->cb.Dispose();
  if (baton->val.data != NULL) {
    free(baton->val.data);
    baton->val.data = NULL;
  }
  delete baton;

  return 0;
}

int Db::EIO_Put(eio_req *req) {
  EIODbBaton *baton = static_cast<EIODbBaton *>(req->data);
  if (baton->object == NULL || dynamic_cast<Db *>(baton->object)->_db == NULL)
    return 0;

  Db *dbObj = dynamic_cast<Db *>(baton->object);
  DB *&db = dbObj->_db;

  TXN_BEGIN(dbObj);

  baton->status = db->put(db, _txn, &(baton->key), &(baton->val), baton->flags);

  TXN_END(dbObj, baton->status);

  return 0;
}

int Db::EIO_Del(eio_req *req) {
  EIODbBaton *baton = static_cast<EIODbBaton *>(req->data);
  if (baton->object == NULL || dynamic_cast<Db *>(baton->object)->_db == NULL)
    return 0;

  Db *dbObj = dynamic_cast<Db *>(baton->object);
  DB *&db = dbObj->_db;

  TXN_BEGIN(dbObj);

  baton->status = db->del(db, _txn, &(baton->key), baton->flags);

  TXN_END(dbObj, baton->status);

  return 0;
}

// Start V8 Exposed Methods

v8::Handle<v8::Value> Db::OpenS(const v8::Arguments& args) {
  v8::HandleScope scope;

  Db* db = node::ObjectWrap::Unwrap<Db>(args.This());

  REQ_OBJ_ARG(0, envObj);
  DbEnv *env = node::ObjectWrap::Unwrap<DbEnv>(envObj);

  REQ_STR_ARG(1, file);
  REQ_INT_ARG(2, type);
  REQ_INT_ARG(3, flags);
  REQ_INT_ARG(4, mode);
  REQ_INT_ARG(5, retries);

  db->_env = env->getDB_ENV();
  db->_transactional = env->isTransactional();
  db->_retries = retries;

  int rc = 0;
  if (db->_db == NULL)
    rc = db_create(&(db->_db), db->_env, 0);

  if (db->_db != NULL && rc == 0) {
    rc = db->_db->open(db->_db,
                       NULL,  // TODO(mcavage): support DB open as TXN
                       *file,
                       NULL,  // db
                       static_cast<DBTYPE>(type),
                       flags,
                       mode);
  }

  DB_RES(rc, db_strerror(rc), msg);
  return msg;
}

v8::Handle<v8::Value> Db::CloseS(const v8::Arguments& args) {
  v8::HandleScope scope;

  Db* db = node::ObjectWrap::Unwrap<Db>(args.This());

  REQ_INT_ARG(0, flags);

  int rc = db->_db->close(db->_db, flags);
  DB_RES(rc, db_strerror(rc), msg);
  return msg;
}

v8::Handle<v8::Value> Db::Get(const v8::Arguments& args) {
  v8::HandleScope scope;

  Db* db = node::ObjectWrap::Unwrap<Db>(args.This());

  REQ_BUF_ARG(0, key);
  REQ_INT_ARG(1, flags);
  REQ_FN_ARG(2, cb);

  EIODbBaton *baton = new EIODbBaton(db);
  baton->cb = v8::Persistent<v8::Function>::New(cb);
  baton->flags = flags;
  baton->key.data = key;
  baton->key.size = key_len;

  db->Ref();
  eio_custom(EIO_Get, EIO_PRI_DEFAULT, EIO_AfterGet, baton);
  ev_ref(EV_DEFAULT_UC);

  return v8::Undefined();
}

v8::Handle<v8::Value> Db::GetS(const v8::Arguments& args) {
  v8::HandleScope scope;

  int rc = 0;
  Db* db = node::ObjectWrap::Unwrap<Db>(args.This());

  REQ_BUF_ARG(0, key);
  REQ_INT_ARG(1, flags);

  INIT_DBT(key, key_len);
  DBT dbt_val = {0};
  memset(&dbt_val, 0, sizeof(DBT));
  dbt_val.flags = DB_DBT_MALLOC;

  TXN_BEGIN(db);

  rc = db->_db->get(db->_db, _txn, &dbt_key, &dbt_val, flags);

  TXN_END(db, rc);

  DB_RES(rc, db_strerror(rc), msg);
  node::Buffer *buf = node::Buffer::New(dbt_val.size);
  memcpy(node::Buffer::Data(buf), dbt_val.data, dbt_val.size);
  if (dbt_val.data != NULL) {
    free(dbt_val.data);
  }

  msg->Set(val_sym, buf->handle_);
  return msg;
}

v8::Handle<v8::Value> Db::Put(const v8::Arguments& args) {
  v8::HandleScope scope;

  Db* db = node::ObjectWrap::Unwrap<Db>(args.This());

  REQ_BUF_ARG(0, key);
  REQ_BUF_ARG(1, value);
  REQ_INT_ARG(2, flags);
  REQ_FN_ARG(3, cb);

  EIODbBaton *baton = new EIODbBaton(db);
  baton->cb = v8::Persistent<v8::Function>::New(cb);
  baton->flags = flags;
  baton->key.data = key;
  baton->key.size = key_len;
  baton->val.data = value;
  baton->val.size = value_len;

  db->Ref();
  eio_custom(EIO_Put, EIO_PRI_DEFAULT, EIO_After_ReturnStatus, baton);
  ev_ref(EV_DEFAULT_UC);

  return v8::Undefined();
}

v8::Handle<v8::Value> Db::PutS(const v8::Arguments& args) {
  v8::HandleScope scope;

  Db* db = node::ObjectWrap::Unwrap<Db>(args.This());
  REQ_BUF_ARG(0, key);
  REQ_BUF_ARG(1, val);
  REQ_INT_ARG(2, flags);

  int rc = 0;
  INIT_DBT(key, key_len);
  INIT_DBT(val, val_len);

  TXN_BEGIN(db);

  rc = db->_db->put(db->_db, _txn, &dbt_key, &dbt_val, flags);

  TXN_END(db, rc);

  DB_RES(rc, db_strerror(rc), msg);
  return msg;
}

v8::Handle<v8::Value> Db::Del(const v8::Arguments& args) {
  v8::HandleScope scope;

  Db* db = node::ObjectWrap::Unwrap<Db>(args.This());

  REQ_BUF_ARG(0, key);
  REQ_INT_ARG(1, flags);
  REQ_FN_ARG(2, cb);

  EIODbBaton *baton = new EIODbBaton(db);
  baton->cb = v8::Persistent<v8::Function>::New(cb);
  baton->flags = flags;
  baton->key.data = key;
  baton->key.size = key_len;

  db->Ref();
  eio_custom(EIO_Del, EIO_PRI_DEFAULT, EIO_After_ReturnStatus, baton);
  ev_ref(EV_DEFAULT_UC);

  return v8::Undefined();
}

v8::Handle<v8::Value> Db::DelS(const v8::Arguments& args) {
  v8::HandleScope scope;

  int rc = 0;
  Db* db = node::ObjectWrap::Unwrap<Db>(args.This());

  REQ_BUF_ARG(0, key);
  REQ_INT_ARG(1, flags);

  INIT_DBT(key, key_len);

  TXN_BEGIN(db);

  rc = db->_db->del(db->_db, _txn, &dbt_key, flags);

  TXN_END(db, rc);

  DB_RES(rc, db_strerror(rc), msg);
  return msg;
}

v8::Handle<v8::Value> Db::New(const v8::Arguments& args) {
  v8::HandleScope scope;

  Db* db = new Db();
  db->Wrap(args.This());
  return args.This();
}

void Db::Initialize(v8::Handle<v8::Object> target) {
  v8::HandleScope scope;

  v8::Local<v8::FunctionTemplate> t = v8::FunctionTemplate::New(New);
  t->InstanceTemplate()->SetInternalFieldCount(1);

  val_sym = NODE_PSYMBOL("value");

  NODE_SET_PROTOTYPE_METHOD(t, "_closeSync", CloseS);
  NODE_SET_PROTOTYPE_METHOD(t, "_openSync", OpenS);
  NODE_SET_PROTOTYPE_METHOD(t, "_get", Get);
  NODE_SET_PROTOTYPE_METHOD(t, "_getSync", GetS);
  NODE_SET_PROTOTYPE_METHOD(t, "_put", Put);
  NODE_SET_PROTOTYPE_METHOD(t, "_putSync", PutS);
  NODE_SET_PROTOTYPE_METHOD(t, "_del", Del);
  NODE_SET_PROTOTYPE_METHOD(t, "_delSync", DelS);

  target->Set(v8::String::NewSymbol("Db"), t->GetFunction());
}
