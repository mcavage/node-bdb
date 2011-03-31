// Copyright 2011 Mark Cavage <mcavage@gmail.com> All rights reserved.
#include <dlfcn.h>
#include <sched.h>
#include <stdlib.h>
#include <string.h>

#include <map>

#include <node_buffer.h>

#include "bdb_common.h"
#include "bdb_db.h"
#include "bdb_env.h"


using v8::FunctionTemplate;
using v8::Persistent;
using v8::String;

v8::Persistent<v8::String> fd_sym;

class EIODbBaton: public EIOBaton {
 public:
  explicit EIODbBaton(Db *db): EIOBaton(db), env(0), data() {
    memset(&key, 0, sizeof(DBT));
    memset(&val, 0, sizeof(DBT));
  }

  virtual ~EIODbBaton() {
    std::map<DBT *, DBT *>::iterator i = data.begin();
    while (i != data.end()) {
      if (i->first != NULL) {
        if (i->first->data != NULL) free(i->first->data);
        free(i->first);
      }
      if (i->second) {
        if (i->second->data) free(i->second->data);
        free(i->second);
      }
      i++;
    }
  }

  DbEnv *env;
  DBT key;
  DBT val;

  // These are for cursor get only
  std::map<DBT *, DBT *> data;
  int limit;

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

  if (baton->val.data != NULL) {
    free(baton->val.data);
    baton->val.data = NULL;
  }
  baton->object->Unref();
  delete baton;

  return 0;
}

int Db::EIO_CursorGet(eio_req *req) {
  EIODbBaton *baton = static_cast<EIODbBaton *>(req->data);
  if (baton->object == NULL || dynamic_cast<Db *>(baton->object)->_db == NULL)
    return 0;

  Db *dbObj = dynamic_cast<Db *>(baton->object);
  DB *&db = dbObj->_db;
  int rc = 0;
  DBC *cursor = NULL;
  DBT *key = NULL;
  DBT *val = NULL;
  int i = 1;

  TXN_BEGIN(dbObj);

  rc = db->cursor(db, _txn, &cursor, 0);
  if (rc != 0) {
    baton->status = rc;
    goto error;
  }

  ALLOC_DBT(key);
  ALLOC_DBT(val);
  key->size = baton->key.size;
  key->data = calloc(1, key->size);
  memcpy(key->data, baton->key.data, key->size);
  rc = cursor->get(cursor, (&baton->key), val, DB_SET_RANGE);
  if (rc != 0) {
    baton->status = rc;
    goto error;
  }
  baton->data[key] = val;

  ALLOC_DBT(key);
  ALLOC_DBT(val);
  while ((rc = cursor->get(cursor, key, val, DB_NEXT)) == 0 &&
         i < baton->limit) {
    baton->data[key] = val;
    ALLOC_DBT(key);
    ALLOC_DBT(val);
    i++;
  }

  if (key) {
    if (key->data) free(key->data);
    free(key);
  }
  if (val) {
    if (val->data) free(val->data);
    free(val);
  }

error:
  TXN_END(dbObj, baton->status);
  return 0;
}

int Db::EIO_AfterCursorGet(eio_req *req) {
  v8::HandleScope scope;
  EIODbBaton *baton = static_cast<EIODbBaton *>(req->data);
  ev_unref(EV_DEFAULT_UC);

  DB_RES(baton->status, db_strerror(baton->status), msg);
  v8::Local<v8::Array> arr = v8::Array::New(baton->data.size());
  int count = 0;
  std::map<DBT *, DBT *>::iterator i = baton->data.begin();
  while (i != baton->data.end()) {
    node::Buffer *k = node::Buffer::New(i->first->size);
    memcpy(node::Buffer::Data(k), i->first->data, i->first->size);
    node::Buffer *v = node::Buffer::New(i->second->size);
    memcpy(node::Buffer::Data(v), i->second->data, i->second->size);
    v8::Local<v8::Object> obj = v8::Object::New();
    obj->Set(key_sym, k->handle_);
    obj->Set(val_sym, v->handle_);
    v8::Handle<v8::Number> slot = v8::Number::New(count++);
    arr->Set(slot, obj);
    i++;
  }

  v8::Handle<v8::Value> argv[2] = {};
  argv[0] = msg;
  argv[1] = arr;

  v8::TryCatch try_catch;

  baton->cb->Call(v8::Context::GetCurrent()->Global(), 2, argv);

  if (try_catch.HasCaught())
    node::FatalException(try_catch);

  baton->object->Unref();
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

  REQ_STR_ARG(0, file);
  REQ_INT_ARG(1, type);
  REQ_INT_ARG(2, flags);
  REQ_INT_ARG(3, mode);
  REQ_INT_ARG(4, retries);

  db->_retries = retries;

  int rc = -1;
  if (db->_db != NULL) {
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


v8::Handle<v8::Value> Db::AssociateS(const v8::Arguments &args) {
  v8::HandleScope scope;

  Db* db = node::ObjectWrap::Unwrap<Db>(args.This());
  REQ_OBJ_ARG(0, sdbObj);
  REQ_STR_ARG(1, lib);
  REQ_STR_ARG(2, sym);
  Db *sdb = node::ObjectWrap::Unwrap<Db>(sdbObj);
  DB *&_db = db->_db;
  DB *&_sdb = sdb->_db;

  void *handle = dlopen(*lib, RTLD_NOW | RTLD_LOCAL);
  if (handle == NULL) {
    DB_RES(-1, dlerror(), _msg);
    return _msg;
  }

  int (*callback)(DB *, const DBT *, const DBT *, DBT *) = NULL;

  callback =  (int (*)(DB*, const DBT*, const DBT*, DBT*)) dlsym(handle, *sym);
  if (callback == NULL) {
    DB_RES(-1, dlerror(), _msg);
    return _msg;
  }

  // We don't dlclose...that's lame, but the OS will do it :-)

  int rc = _db->associate(_db, NULL, _sdb, callback, 0);
  DB_RES(rc, db_strerror(rc), msg);
  return msg;
}


v8::Handle<v8::Value> Db::CursorGet(const v8::Arguments& args) {
  v8::HandleScope scope;

  Db* db = node::ObjectWrap::Unwrap<Db>(args.This());

  REQ_BUF_ARG(0, key);
  REQ_INT_ARG(1, limit);
  REQ_INT_ARG(2, flags);
  REQ_FN_ARG(3, cb);

  EIODbBaton *baton = new EIODbBaton(db);
  baton->cb = v8::Persistent<v8::Function>::New(cb);
  baton->limit = limit;
  baton->flags = flags;
  baton->key.data = key;
  baton->key.size = key_len;

  db->Ref();
  eio_custom(EIO_CursorGet, EIO_PRI_DEFAULT, EIO_AfterCursorGet, baton);
  ev_ref(EV_DEFAULT_UC);

  return v8::Undefined();
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


v8::Handle<v8::Value> Db::SetFlags(const v8::Arguments& args) {
  v8::HandleScope scope;

  Db* db = node::ObjectWrap::Unwrap<Db>(args.This());
  REQ_INT_ARG(0, flags);

  int rc = db->_db->set_flags(db->_db, flags);

  DB_RES(rc, db_strerror(rc), msg);
  return msg;
}

v8::Handle<v8::Value> Db::Fd(const v8::Arguments& args) {
  v8::HandleScope scope;

  Db* db = node::ObjectWrap::Unwrap<Db>(args.This());
  int fd = 0;

  int rc = db->_db->fd(db->_db, &fd);

  DB_RES(rc, db_strerror(rc), msg);
  if (rc == 0)
    msg->Set(fd_sym, v8::Number::New(fd));
  return msg;
}

v8::Handle<v8::Value> Db::SetEncrypt(const v8::Arguments& args) {
  v8::HandleScope scope;

  Db* db = node::ObjectWrap::Unwrap<Db>(args.This());
  REQ_STR_ARG(0, passwd);

  int rc = db->_db->set_encrypt(db->_db, *passwd, DB_ENCRYPT_AES);

  DB_RES(rc, db_strerror(rc), msg);
  return msg;
}

v8::Handle<v8::Value> Db::New(const v8::Arguments& args) {
  v8::HandleScope scope;

  REQ_OBJ_ARG(0, envObj);
  DbEnv *env = node::ObjectWrap::Unwrap<DbEnv>(envObj);

  Db* db = new Db();
  db->Wrap(args.This());

  db->_env = env->getDB_ENV();
  db->_transactional = env->isTransactional();

  int rc = 0;
  if (db->_db == NULL)
    rc = db_create(&(db->_db), db->_env, 0);

  if (rc != 0)
    RET_EXC(db_strerror(rc));

  return args.This();
}

void Db::Initialize(v8::Handle<v8::Object> target) {
  v8::HandleScope scope;

  v8::Local<v8::FunctionTemplate> t = v8::FunctionTemplate::New(New);
  t->InstanceTemplate()->SetInternalFieldCount(1);

  fd_sym = NODE_PSYMBOL("fd");

  NODE_SET_PROTOTYPE_METHOD(t, "_associateSync", AssociateS);
  NODE_SET_PROTOTYPE_METHOD(t, "_closeSync", CloseS);
  NODE_SET_PROTOTYPE_METHOD(t, "_cursorGet", CursorGet);
  NODE_SET_PROTOTYPE_METHOD(t, "_openSync", OpenS);
  NODE_SET_PROTOTYPE_METHOD(t, "_get", Get);
  NODE_SET_PROTOTYPE_METHOD(t, "_getSync", GetS);
  NODE_SET_PROTOTYPE_METHOD(t, "_put", Put);
  NODE_SET_PROTOTYPE_METHOD(t, "_putSync", PutS);
  NODE_SET_PROTOTYPE_METHOD(t, "_del", Del);
  NODE_SET_PROTOTYPE_METHOD(t, "_delSync", DelS);
  NODE_SET_PROTOTYPE_METHOD(t, "setEncrypt", SetEncrypt);
  NODE_SET_PROTOTYPE_METHOD(t, "setFlags", SetFlags);

  target->Set(v8::String::NewSymbol("Db"), t->GetFunction());
}
