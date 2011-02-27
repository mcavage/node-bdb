#include <stdlib.h>
#include <string.h>

#include <node_buffer.h>

#include "bdb_cursor.h"
#include "bdb_db.h"
#include "bdb_env.h"
#include "bdb_txn.h"

using namespace node;
using namespace v8;

struct eio_baton_t {
  eio_baton_t() : db(0), txn(0), status(0), flags(0) {}

  Db *db;
  DbTxn *txn;
  int status;
  int flags;
  v8::Persistent<v8::Function> cb;
};

struct eio_open_baton_t: eio_baton_t {
  eio_open_baton_t() :
    eio_baton_t(), env(0), file(0), type(DB_UNKNOWN), mode(0) {}

  DbEnv *env;
  char *file;
  DBTYPE type;
  int mode;
};

struct eio_data_baton_t: eio_baton_t {
  eio_data_baton_t() : eio_baton_t()  {
    memset(&key, 0, sizeof(DBT));
    memset(&value, 0, sizeof(DBT));
  }

  DBT key;
  DBT value;
};

struct eio_cursor_baton_t: eio_baton_t {
  eio_cursor_baton_t() : eio_baton_t(), cursor(0) {}

  DbCursor *cursor;
};


Db::Db(): _db(0) {}

Db::~Db() {
  if(_db != NULL) {
    _db->close(_db, 0);
  }
}

// Start EIO Methods

int Db::EIO_Open(eio_req *req) {
  eio_open_baton_t *baton = static_cast<eio_open_baton_t *>(req->data);

  DB *&db = baton->db->_db;

  if(db == NULL) {
    baton->status = db_create(&db, baton->env->getDB_ENV(), 0);
    if(baton->status != 0) return 0;
  }

  if(db != NULL && baton->status == 0) {
    baton->status =
      db->open(db,
	       NULL, // txn TODO
	       baton->file,
	       NULL,
	       baton->type,
	       baton->flags,
	       baton->mode);
  }

  return 0;
}

int Db::EIO_AfterOpen(eio_req *req) {
  HandleScope scope;
  eio_open_baton_t *baton = static_cast<eio_open_baton_t *>(req->data);
  ev_unref(EV_DEFAULT_UC);

  DB_RES(baton->status, db_strerror(baton->status), msg);
  Local<Value> argv[1] = { msg };

  TryCatch try_catch;

  baton->cb->Call(Context::GetCurrent()->Global(), 1, argv);

  if (try_catch.HasCaught()) {
    FatalException(try_catch);
  }

  baton->db->Unref();
  baton->cb.Dispose();
  if(baton->file != NULL) free(baton->file);
  delete baton;
  return 0;
}

int Db::EIO_Get(eio_req *req) {
  eio_data_baton_t *baton = static_cast<eio_data_baton_t *>(req->data);

  DB *&db = baton->db->_db;
  DB_TXN *txn = baton->txn ? baton->txn->getDB_TXN() : NULL;
  baton->value.flags = DB_DBT_MALLOC;
  if(db != NULL) {
    baton->status = db->get(db,
			    txn,
			    &(baton->key),
			    &(baton->value),
			    baton->flags);
  }

  return 0;
}

int Db::EIO_AfterGet(eio_req *req) {
  HandleScope scope;
  eio_data_baton_t *baton = static_cast<eio_data_baton_t *>(req->data);
  ev_unref(EV_DEFAULT_UC);

  DB_RES(baton->status, db_strerror(baton->status), msg);
  Buffer *buf = Buffer::New(baton->value.size);
  memcpy(Buffer::Data(buf), baton->value.data, baton->value.size);

  Handle<Value> argv[2];
  argv[0] = msg;
  argv[1] = buf->handle_;

  TryCatch try_catch;

  baton->cb->Call(Context::GetCurrent()->Global(), 2, argv);

  if (try_catch.HasCaught()) {
    FatalException(try_catch);
  }

  baton->db->Unref();
  baton->cb.Dispose();
  if(baton->value.data != NULL) free(baton->value.data);
  delete baton;
  return 0;
}

int Db::EIO_Put(eio_req *req) {
  eio_data_baton_t *baton = static_cast<eio_data_baton_t *>(req->data);

  DB *&db = baton->db->_db;
  DB_TXN *txn = baton->txn ? baton->txn->getDB_TXN() : NULL;

  if(db != NULL) {
    baton->status =
      db->put(db,
	      txn,
	      &(baton->key),
	      &(baton->value),
	      baton->flags);
  }

  return 0;
}

int Db::EIO_AfterPut(eio_req *req) {
  HandleScope scope;
  eio_data_baton_t *baton = static_cast<eio_data_baton_t *>(req->data);
  ev_unref(EV_DEFAULT_UC);

  DB_RES(baton->status, db_strerror(baton->status), msg);
  Local<Value> argv[1] = { msg };

  TryCatch try_catch;

  baton->cb->Call(Context::GetCurrent()->Global(), 1, argv);

  if (try_catch.HasCaught()) {
    FatalException(try_catch);
  }

  baton->db->Unref();
  baton->cb.Dispose();
  delete baton;
  return 0;
}

int Db::EIO_Del(eio_req *req) {
  eio_data_baton_t *baton = static_cast<eio_data_baton_t *>(req->data);

  DB *&db = baton->db->_db;
  DB_TXN *txn = baton->txn ? baton->txn->getDB_TXN() : NULL;

  if(db != NULL) {
    baton->status = db->del(db,
			    txn,
			    &(baton->key),
			    baton->flags);
  }

  return 0;
}

int Db::EIO_AfterDel(eio_req *req) {
  HandleScope scope;
  eio_data_baton_t *baton = static_cast<eio_data_baton_t *>(req->data);
  ev_unref(EV_DEFAULT_UC);

  DB_RES(baton->status, db_strerror(baton->status), msg);
  Local<Value> argv[1] = { msg };

  TryCatch try_catch;

  baton->cb->Call(Context::GetCurrent()->Global(), 1, argv);

  if (try_catch.HasCaught()) {
    FatalException(try_catch);
  }

  baton->db->Unref();
  baton->cb.Dispose();
  delete baton;
  return 0;
}

int Db::EIO_Cursor(eio_req *req) {
  eio_cursor_baton_t *baton = static_cast<eio_cursor_baton_t *>(req->data);

  DB *&db = baton->db->_db;
  DB_TXN *txn = baton->txn ? baton->txn->getDB_TXN() : NULL;
  DBC *&cursor = baton->cursor->getDBC();

  if(db != NULL) {
    baton->status =
      db->cursor(db,
		 txn,
		 &cursor,
		 baton->flags);
  }

  return 0;
}

int Db::EIO_AfterCursor(eio_req *req) {
  HandleScope scope;
  eio_cursor_baton_t *baton = static_cast<eio_cursor_baton_t *>(req->data);
  ev_unref(EV_DEFAULT_UC);

  DB_RES(baton->status, db_strerror(baton->status), msg);

  Local<Value> argv[1] = { msg };

  TryCatch try_catch;

  baton->cb->Call(Context::GetCurrent()->Global(), 1, argv);

  if (try_catch.HasCaught()) {
    FatalException(try_catch);
  }

  baton->db->Unref();
  baton->cb.Dispose();
  delete baton;
  return 0;
}

// Start V8 Exposed Methods

Handle<Value> Db::Open(const Arguments& args) {
  HandleScope scope;

  Db* db = ObjectWrap::Unwrap<Db>(args.This());

  int flags = DEF_OPEN_FLAGS;
  DBTYPE type = DEF_TYPE;
  int mode = 0;

  REQ_FN_ARG(args.Length() - 1, cb);

  REQ_OBJ_ARG(0, envObj);
  DbEnv *env = node::ObjectWrap::Unwrap<DbEnv>(envObj);

  REQ_STR_ARG(1, file);
  if(args.Length() > 3) {
    REQ_INT_ARG(2, tmp);
    type = static_cast<DBTYPE>(tmp->Value());
  }
  if(args.Length() > 4) {
    REQ_INT_ARG(3, tmp);
    flags = tmp->Value();
  }
  if(args.Length() > 5) {
    REQ_INT_ARG(4, tmp);
    mode = tmp->Value();
  }

  eio_open_baton_t *baton = new eio_open_baton_t();
  baton->db = db;
  baton->cb = Persistent<Function>::New(cb);
  baton->env = env;;
  baton->type = type;
  baton->flags = flags;
  baton->mode = mode;

  if(file.length() > 0) {
    baton->file = (char *)calloc(1, file.length() + 1);
    if(baton->file == NULL) {
      LOG_OOM();
      // WTF to do in node when OOM?
    } else {
      strncpy(baton->file, *file, file.length());
    }
  }

  db->Ref();
  eio_custom(EIO_Open, EIO_PRI_DEFAULT, EIO_AfterOpen, baton);
  ev_ref(EV_DEFAULT_UC);

  return Undefined();
}

Handle<Value> Db::OpenSync(const Arguments& args) {
  HandleScope scope;

  Db* db = ObjectWrap::Unwrap<Db>(args.This());

  int flags = DEF_OPEN_FLAGS;
  DBTYPE type = DEF_TYPE;
  int mode = 0;

  REQ_OBJ_ARG(0, envObj);
  DbEnv *env = node::ObjectWrap::Unwrap<DbEnv>(envObj);

  REQ_STR_ARG(1, file);
  if(args.Length() > 2) {
    REQ_INT_ARG(2, tmp);
    type = static_cast<DBTYPE>(tmp->Value());
  }
  if(args.Length() > 3) {
    REQ_INT_ARG(3, tmp);
    flags = tmp->Value();
  }
  if(args.Length() > 4) {
    REQ_INT_ARG(4, tmp);
    mode = tmp->Value();
  }

  int rc = 0;
  if(db->_db == NULL) {
    rc = db_create(&(db->_db), env->getDB_ENV(), 0);
  }

  if(db->_db != NULL && rc == 0) {
    rc = db->_db->open(db->_db,
		       NULL, // txn TODO
		       *file,
		       NULL, // db - this is generally useless, so hide it
		       type,
		       flags,
		       mode);
  }

  DB_RES(rc, db_strerror(rc), msg);
  Handle<Value> result = { msg };

  // TODO: REMOVE THESE
  db->_db->set_errfile(db->_db, stderr);
  db->_db->set_errpfx(db->_db, "node-bdb");

  return result;
}

Handle<Value> Db::Get(const Arguments& args) {
  HandleScope scope;

  Db* db = ObjectWrap::Unwrap<Db>(args.This());
  int flags = DEF_DATA_FLAGS;
  DbTxn *txn = NULL;

  REQ_FN_ARG(args.Length() - 1, cb);
  REQ_BUF_ARG(0, key);

  if(args.Length() > 2) {
    REQ_INT_ARG(1, tmp);
    if(tmp->Value() > 0)
      flags = tmp->Value();
  }
  if(args.Length() > 3) {
    REQ_OBJ_ARG(2, txnObj);
    txn = node::ObjectWrap::Unwrap<DbTxn>(txnObj);
  }

  eio_data_baton_t *baton = new eio_data_baton_t();
  baton->db = db;
  baton->cb = Persistent<Function>::New(cb);
  baton->flags = flags;
  baton->txn = txn;
  baton->key.data = key;
  baton->key.size = key_len;

  db->Ref();
  eio_custom(EIO_Get, EIO_PRI_DEFAULT, EIO_AfterGet, baton);
  ev_ref(EV_DEFAULT_UC);

  return Undefined();
}

Handle<Value> Db::Put(const Arguments& args) {
  HandleScope scope;

  Db* db = ObjectWrap::Unwrap<Db>(args.This());
  int flags = DEF_DATA_FLAGS;
  DbTxn *txn = NULL;

  REQ_FN_ARG(args.Length() - 1, cb);
  REQ_BUF_ARG(0, key);
  REQ_BUF_ARG(1, value);

  if(args.Length() > 3) {
    REQ_INT_ARG(2, tmp);
    if(tmp->Value() > 0)
      flags = tmp->Value();
  }

  if(args.Length() > 4) {
    REQ_OBJ_ARG(3, txnObj);
    txn = node::ObjectWrap::Unwrap<DbTxn>(txnObj);
  }

  eio_data_baton_t *baton = new eio_data_baton_t();
  baton->db = db;
  baton->cb = Persistent<Function>::New(cb);
  baton->flags = flags;
  baton->txn = txn;
  baton->key.data = key;
  baton->key.size = key_len;
  baton->value.data = value;
  baton->value.size = value_len;

  db->Ref();
  eio_custom(EIO_Put, EIO_PRI_DEFAULT, EIO_AfterPut, baton);
  ev_ref(EV_DEFAULT_UC);

  return Undefined();
}

Handle<Value> Db::Del(const Arguments& args) {
  HandleScope scope;

  Db* db = ObjectWrap::Unwrap<Db>(args.This());
  int flags = DEF_DATA_FLAGS;
  DbTxn *txn = NULL;

  REQ_FN_ARG(args.Length() - 1, cb);
  REQ_BUF_ARG(0, key);

  if(args.Length() > 2) {
    REQ_INT_ARG(1, tmp);
    if(tmp->Value() > 0)
      flags = tmp->Value();
  }
  if(args.Length() > 3) {
    REQ_OBJ_ARG(2, txnObj);
    txn = node::ObjectWrap::Unwrap<DbTxn>(txnObj);
  }

  eio_data_baton_t *baton = new eio_data_baton_t();
  baton->db = db;
  baton->cb = Persistent<Function>::New(cb);
  baton->flags = flags;
  baton->txn = txn;
  baton->key.data = key;
  baton->key.size = key_len;

  db->Ref();
  eio_custom(EIO_Del, EIO_PRI_DEFAULT, EIO_AfterDel, baton);
  ev_ref(EV_DEFAULT_UC);

  return Undefined();
}

Handle<Value> Db::Cursor(const Arguments& args) {
  HandleScope scope;

  DbTxn *txn = NULL;
  int flags = 0;

  Db* db = ObjectWrap::Unwrap<Db>(args.This());

  REQ_OBJ_ARG(0, cursorObj);
  DbCursor *dbCursor = node::ObjectWrap::Unwrap<DbCursor>(cursorObj);

  if(args.Length() > 1) {
    REQ_OBJ_ARG(1, txnObj);
    txn = node::ObjectWrap::Unwrap<DbTxn>(txnObj);
  }

  if(args.Length() > 2) {
    REQ_INT_ARG(2, tmp);
    flags = tmp->Value();
  }

  DBC *&cursor = dbCursor->getDBC();
  int rc = db->_db->cursor(db->_db,
			   txn ? txn->getDB_TXN() : NULL,
			   &cursor,
			   flags);
  DB_RES(rc, db_strerror(rc), msg);
  Handle<Value> result = { msg };
  return result;

  /*
  REQ_FN_ARG(args.Length() - 1, cb);
  REQ_OBJ_ARG(0, curObj);
  DbCursor *cur = node::ObjectWrap::Unwrap<DbCursor>(curObj);

  if(args.Length() > 3) {
    REQ_INT_ARG(2, tmp);
    flags = tmp->Value();
  }

  eio_cursor_baton_t *baton = new eio_cursor_baton_t();
  baton->db = db;
  baton->cursor = cur->getDBC();
  baton->cb = Persistent<Function>::New(cb);
  baton->flags = flags;

  db->Ref();
  eio_custom(EIO_Cursor, EIO_PRI_DEFAULT, EIO_AfterCursor, baton);
  ev_ref(EV_DEFAULT_UC);

  return Undefined();
  */
}

Handle<Value> Db::New(const Arguments& args) {
  HandleScope scope;

  Db* db = new Db();
  db->Wrap(args.This());
  return args.This();
}

void Db::Initialize(Handle<Object> target) {
  HandleScope scope;

  Local<FunctionTemplate> t = FunctionTemplate::New(New);
  t->InstanceTemplate()->SetInternalFieldCount(1);

  NODE_SET_PROTOTYPE_METHOD(t, "open", Open);
  NODE_SET_PROTOTYPE_METHOD(t, "openSync", OpenSync);
  NODE_SET_PROTOTYPE_METHOD(t, "get", Get);
  NODE_SET_PROTOTYPE_METHOD(t, "put", Put);
  NODE_SET_PROTOTYPE_METHOD(t, "del", Del);
  NODE_SET_PROTOTYPE_METHOD(t, "cursor", Cursor);

  target->Set(String::NewSymbol("Db"), t->GetFunction());
}
