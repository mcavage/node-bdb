// Copyright 2011 Mark Cavage <mcavage@gmail.com> All rights reserved.
#include <dlfcn.h>
#include <sched.h>
#include <stdlib.h>
#include <string.h>

#include <utility>
#include <vector>

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
  explicit EIODbBaton(Db *db): EIOBaton(db), env(0), records() {
    memset(&key, 0, sizeof(DBT));
    memset(&val, 0, sizeof(DBT));
  }

  virtual ~EIODbBaton() { records.clear(); }

  DbEnv *env;
  DBT key;
  DBT val;

  // Cursors only
  int limit;
  int initFlag;
  std::vector< std::pair<DBT *, DBT *> > records;

  // PutIf
  DBT oldVal;
 private:
  EIODbBaton(const EIODbBaton &);
  EIODbBaton &operator=(const EIODbBaton &);
};


#define ADD_CURSOR_RECORD(KEY, VAL, OBJ, ARR, POS)                      \
  OBJ = v8::Object::New();                                              \
  OBJ->Set(key_sym, node::Buffer::New(static_cast<char *>(KEY->data),   \
                                      KEY->size)->handle_);             \
  OBJ->Set(val_sym, node::Buffer::New(static_cast<char *>(VAL->data),   \
                                      VAL->size)->handle_);             \
  ARR->Set(v8::Number::New(POS), OBJ)

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
  node::Buffer *buf = node::Buffer::New(static_cast<char *>(baton->val.data),
                                        baton->val.size);

  v8::Handle<v8::Value> argv[2] = {};
  argv[0] = msg;
  argv[1] = buf->handle_;

  v8::TryCatch try_catch;

  baton->cb->Call(v8::Context::GetCurrent()->Global(), 2, argv);

  if (try_catch.HasCaught())
    node::FatalException(try_catch);

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
  rc = cursor->get(cursor, key, val, baton->initFlag);
  if (rc != 0) {
    baton->status = rc;
    goto error;
  }
  baton->records.push_back(std::make_pair(key, val));

  ALLOC_DBT(key);
  ALLOC_DBT(val);
  while ((i++ < baton->limit) &&
         (rc = cursor->get(cursor, key, val, baton->flags)) == 0) {
    baton->records.push_back(std::make_pair(key, val));
    ALLOC_DBT(key);
    ALLOC_DBT(val);
  }

 error:
  if (cursor != NULL) {
    baton->status = cursor->close(cursor);
    cursor = NULL;
  }
  TXN_END(dbObj, baton->status);
  // These get alloc'd once more than we need...
  if (key != NULL) {
    free(key);
    key = NULL;
  }
  if (val != NULL) {
    free(val);
    val = NULL;
  }
  if (baton->status == 0 && rc == DB_NOTFOUND) {
    baton->status = DB_NOTFOUND;
  }
  return 0;
}

int Db::EIO_AfterCursorGet(eio_req *req) {
  v8::HandleScope scope;
  EIODbBaton *baton = static_cast<EIODbBaton *>(req->data);
  ev_unref(EV_DEFAULT_UC);

  v8::Local<v8::Array> arr = v8::Array::New(baton->records.size());
  int count = 0;
  std::vector<std::pair<DBT *, DBT *> >::iterator i = baton->records.begin();
  while (i != baton->records.end()) {
    v8::Local<v8::Object> obj;
    ADD_CURSOR_RECORD(i->first, i->second, obj, arr, count++);
    free(i->first);
    free(i->second);
    i++;
  }

  DB_RES(baton->status, db_strerror(baton->status), msg);
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

int Db::EIO_PutIf(eio_req *req) {
  EIODbBaton *baton = static_cast<EIODbBaton *>(req->data);
  if (baton->object == NULL || dynamic_cast<Db *>(baton->object)->_db == NULL)
    return 0;

  Db *dbObj = dynamic_cast<Db *>(baton->object);
  DB *&db = dbObj->_db;
  DBT oldVal = {0};
  memset(&oldVal, 0, sizeof(DBT));
  oldVal.flags = DB_DBT_MALLOC;

  TXN_BEGIN(dbObj);

  baton->status = db->get(db, _txn, &(baton->key), &oldVal, 0);
  if (baton->status == 0) {
    if (oldVal.size != baton->oldVal.size) {
      baton->status = -2;
      goto error;
    }
    if (memcmp(oldVal.data, baton->oldVal.data, oldVal.size) != 0) {
      baton->status = -2;
      goto error;
    }
    baton->status = db->put(db, _txn, &(baton->key), &(baton->val),
                            baton->flags);
  }

 error:
  TXN_END(dbObj, baton->status);

  if (oldVal.data != NULL) {
    free(oldVal.data);
    oldVal.data = NULL;
  }

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
  db->_db = NULL;
  DB_RES(rc, db_strerror(rc), msg);
  return msg;
}


v8::Handle<v8::Value> Db::AssociateS(const v8::Arguments &args) {
  v8::HandleScope scope;

  Db* db = node::ObjectWrap::Unwrap<Db>(args.This());
  REQ_OBJ_ARG(0, sdbObj);
  REQ_STR_ARG(1, lib);
  REQ_STR_ARG(2, sym);
  REQ_INT_ARG(3, flags);
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

  int rc = _db->associate(_db, NULL, _sdb, callback, flags);
  DB_RES(rc, db_strerror(rc), msg);
  return msg;
}


v8::Handle<v8::Value> Db::CursorGet(const v8::Arguments& args) {
  v8::HandleScope scope;

  Db* db = node::ObjectWrap::Unwrap<Db>(args.This());

  REQ_BUF_ARG(0, key);
  REQ_INT_ARG(1, limit);
  REQ_INT_ARG(2, initFlag);
  REQ_INT_ARG(3, flags);
  REQ_FN_ARG(4, cb);

  EIODbBaton *baton = new EIODbBaton(db);
  baton->cb = v8::Persistent<v8::Function>::New(cb);
  baton->limit = limit;
  baton->initFlag = initFlag;
  baton->flags = flags;
  baton->key.data = key;
  baton->key.size = key_len;

  db->Ref();
  eio_custom(EIO_CursorGet, EIO_PRI_DEFAULT, EIO_AfterCursorGet, baton);
  ev_ref(EV_DEFAULT_UC);

  return v8::Undefined();
}

v8::Handle<v8::Value> Db::CursorGetS(const v8::Arguments& args) {
  v8::HandleScope scope;

  Db* dbObj = node::ObjectWrap::Unwrap<Db>(args.This());

  REQ_BUF_ARG(0, _key);
  REQ_INT_ARG(1, limit);
  REQ_INT_ARG(2, initFlag);
  REQ_INT_ARG(3, flags);

  DB *&db = dbObj->_db;
  int rc = 0;
  DBC *cursor = NULL;
  DBT key = {0};
  DBT val = {0};
  int i = 1;
  int count = 0;
  v8::Local<v8::Array> arr = v8::Array::New();
  v8::Local<v8::Object> v8Obj;
  bool last = false;

  TXN_BEGIN(dbObj);

  rc = db->cursor(db, _txn, &cursor, 0);
  if (rc != 0) goto error;

  memset(&key, 0, sizeof(DBT));
  memset(&val, 0, sizeof(DBT));
  key.size = _key_len;
  // Make a copy of the one passed in
  key.data = calloc(1, key.size);
  memcpy(key.data, _key, key.size);
  rc = cursor->get(cursor, &key, &val, initFlag);
  if (rc != 0) goto error;

  ADD_CURSOR_RECORD((&key), (&val), v8Obj, arr, count++);
  memset(&key, 0, sizeof(DBT));
  memset(&val, 0, sizeof(DBT));

  while ((i++ < limit) && (rc = cursor->get(cursor, &key, &val, flags)) == 0) {
    ADD_CURSOR_RECORD((&key), (&val), v8Obj, arr, count++);
    memset(&key, 0, sizeof(DBT));
    memset(&val, 0, sizeof(DBT));
  }

  if (rc == DB_NOTFOUND) {
    last = true;
  }

 error:
  if (cursor != NULL) {
    rc = cursor->close(cursor);
    cursor = NULL;
  }
  TXN_END(dbObj, rc);
  if (rc == 0 && last) {
    rc = DB_NOTFOUND;
  }
  DB_RES(rc, db_strerror(rc), msg);
  msg->Set(data_sym, arr);

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
  node::Buffer *buf = node::Buffer::New(static_cast<char *>(dbt_val.data),
                                        dbt_val.size);
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


v8::Handle<v8::Value> Db::PutIf(const v8::Arguments& args) {
  v8::HandleScope scope;

  Db* db = node::ObjectWrap::Unwrap<Db>(args.This());

  REQ_BUF_ARG(0, key);
  REQ_BUF_ARG(1, value);
  REQ_BUF_ARG(2, oldValue);
  REQ_INT_ARG(3, flags);
  REQ_FN_ARG(4, cb);

  EIODbBaton *baton = new EIODbBaton(db);
  baton->cb = v8::Persistent<v8::Function>::New(cb);
  baton->flags = flags;
  baton->key.data = key;
  baton->key.size = key_len;
  baton->val.data = value;
  baton->val.size = value_len;
  memset(&(baton->oldVal), 0, sizeof(DBT));
  baton->oldVal.data = oldValue;
  baton->oldVal.size = oldValue_len;

  db->Ref();
  eio_custom(EIO_PutIf, EIO_PRI_DEFAULT, EIO_After_ReturnStatus, baton);
  ev_ref(EV_DEFAULT_UC);

  return v8::Undefined();
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
  NODE_SET_PROTOTYPE_METHOD(t, "_cursorGetSync", CursorGetS);
  NODE_SET_PROTOTYPE_METHOD(t, "_openSync", OpenS);
  NODE_SET_PROTOTYPE_METHOD(t, "_get", Get);
  NODE_SET_PROTOTYPE_METHOD(t, "_getSync", GetS);
  NODE_SET_PROTOTYPE_METHOD(t, "_put", Put);
  NODE_SET_PROTOTYPE_METHOD(t, "_putIf", PutIf);
  NODE_SET_PROTOTYPE_METHOD(t, "_putSync", PutS);
  NODE_SET_PROTOTYPE_METHOD(t, "_del", Del);
  NODE_SET_PROTOTYPE_METHOD(t, "_delSync", DelS);
  NODE_SET_PROTOTYPE_METHOD(t, "setEncrypt", SetEncrypt);
  NODE_SET_PROTOTYPE_METHOD(t, "setFlags", SetFlags);

  target->Set(v8::String::NewSymbol("Db"), t->GetFunction());
}
