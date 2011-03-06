// Copyright 2011 Mark Cavage <mcavage@gmail.com> All rights reserved.
#include <stdlib.h>
#include <string.h>

#include <node_buffer.h>

#include "bdb_common.h"
#include "bdb_cursor.h"
#include "bdb_db.h"
#include "bdb_env.h"
#include "bdb_txn.h"

using v8::FunctionTemplate;

class EIODbBaton: public EIOBaton {
 public:
  explicit EIODbBaton(Db *db): EIOBaton(db), env(0), txn(0) {
    memset(&key, 0, sizeof(DBT));
    memset(&val, 0, sizeof(DBT));
  }

  virtual ~EIODbBaton() {}

  DbEnv *env;
  DbTxn *txn;
  DBT key;
  DBT val;

 private:
  EIODbBaton(const EIODbBaton &);
  EIODbBaton &operator=(const EIODbBaton &);
};


Db::Db(): DbObject(), _db(0) {}

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

  DB *&db = dynamic_cast<Db *>(baton->object)->_db;
  DB_TXN *txn = baton->txn ? baton->txn->getDB_TXN() : NULL;
  baton->val.flags = DB_DBT_MALLOC;

  baton->status = db->get(db, txn, &(baton->key), &(baton->val), baton->flags);

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

  DB *&db = dynamic_cast<Db *>(baton->object)->_db;
  DB_TXN *txn = baton->txn ? baton->txn->getDB_TXN() : NULL;

  baton->status = db->put(db, txn, &(baton->key), &(baton->val), baton->flags);

  return 0;
}

int Db::EIO_Del(eio_req *req) {
  EIODbBaton *baton = static_cast<EIODbBaton *>(req->data);

  if (baton->object == NULL || dynamic_cast<Db *>(baton->object)->_db == NULL)
    return 0;

  DB *&db = dynamic_cast<Db *>(baton->object)->_db;
  DB_TXN *txn = baton->txn ? baton->txn->getDB_TXN() : NULL;

  baton->status = db->del(db, txn, &(baton->key), baton->flags);

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

  int rc = 0;
  if (db->_db == NULL)
    rc = db_create(&(db->_db), env->getDB_ENV(), 0);

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
  v8::Local<v8::Value> result = { msg };
  return result;
}

v8::Handle<v8::Value> Db::Get(const v8::Arguments& args) {
  v8::HandleScope scope;

  Db* db = node::ObjectWrap::Unwrap<Db>(args.This());
  DbTxn *txn = NULL;

  OPT_TXN_ARG(0, txn);
  REQ_BUF_ARG(1, key);
  REQ_INT_ARG(2, flags);
  REQ_FN_ARG(args.Length() - 1, cb);

  EIODbBaton *baton = new EIODbBaton(db);
  baton->cb = v8::Persistent<v8::Function>::New(cb);
  baton->flags = flags;
  baton->txn = txn;
  baton->key.data = key;
  baton->key.size = key_len;

  db->Ref();
  eio_custom(EIO_Get, EIO_PRI_DEFAULT, EIO_AfterGet, baton);
  ev_ref(EV_DEFAULT_UC);

  return v8::Undefined();
}

v8::Handle<v8::Value> Db::Put(const v8::Arguments& args) {
  v8::HandleScope scope;

  Db* db = node::ObjectWrap::Unwrap<Db>(args.This());
  DbTxn *txn = NULL;

  OPT_TXN_ARG(0, txn);
  REQ_BUF_ARG(1, key);
  REQ_BUF_ARG(2, value);
  REQ_INT_ARG(3, flags);
  REQ_FN_ARG(args.Length() - 1, cb);

  EIODbBaton *baton = new EIODbBaton(db);
  baton->cb = v8::Persistent<v8::Function>::New(cb);
  baton->flags = flags;
  baton->txn = txn;
  baton->key.data = key;
  baton->key.size = key_len;
  baton->val.data = value;
  baton->val.size = value_len;

  db->Ref();
  eio_custom(EIO_Put, EIO_PRI_DEFAULT, EIO_After_ReturnStatus, baton);
  ev_ref(EV_DEFAULT_UC);

  return v8::Undefined();
}

v8::Handle<v8::Value> Db::Del(const v8::Arguments& args) {
  v8::HandleScope scope;

  Db* db = node::ObjectWrap::Unwrap<Db>(args.This());
  DbTxn *txn = NULL;

  OPT_TXN_ARG(0, txn);
  REQ_BUF_ARG(1, key);
  REQ_INT_ARG(2, flags);
  REQ_FN_ARG(args.Length() - 1, cb);

  EIODbBaton *baton = new EIODbBaton(db);
  baton->cb = v8::Persistent<v8::Function>::New(cb);
  baton->flags = flags;
  baton->txn = txn;
  baton->key.data = key;
  baton->key.size = key_len;

  db->Ref();
  eio_custom(EIO_Del, EIO_PRI_DEFAULT, EIO_After_ReturnStatus, baton);
  ev_ref(EV_DEFAULT_UC);

  return v8::Undefined();
}

v8::Handle<v8::Value> Db::Cursor(const v8::Arguments& args) {
  v8::HandleScope scope;

  Db* db = node::ObjectWrap::Unwrap<Db>(args.This());

  DbTxn *txn = NULL;
  OPT_TXN_ARG(0, txn);
  REQ_OBJ_ARG(1, cursorObj);
  DbCursor *dbCursor = node::ObjectWrap::Unwrap<DbCursor>(cursorObj);
  REQ_INT_ARG(2, flags);

  DBC *&cursor = dbCursor->getDBC();
  int rc = db->_db->cursor(db->_db,
                           txn ? txn->getDB_TXN() : NULL,
                           &cursor,
                           flags);

  DB_RES(rc, db_strerror(rc), msg);
  v8::Local<v8::Value> result = { msg };
  return result;
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

  NODE_SET_PROTOTYPE_METHOD(t, "_openSync", OpenS);
  NODE_SET_PROTOTYPE_METHOD(t, "_get", Get);
  NODE_SET_PROTOTYPE_METHOD(t, "_put", Put);
  NODE_SET_PROTOTYPE_METHOD(t, "_del", Del);
  NODE_SET_PROTOTYPE_METHOD(t, "_cursor", Cursor);

  target->Set(v8::String::NewSymbol("Db"), t->GetFunction());
}
