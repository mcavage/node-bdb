#include <stdlib.h>
#include <string.h>

#include <node_buffer.h>

#include "bdb_db.h"
#include "bdb_env.h"

using namespace node;
using namespace v8;

struct eio_baton_t {
  eio_baton_t() : db(0), status(0) {}
  Db *db;
  int status;
  v8::Persistent<v8::Function> cb;
};

struct eio_open_baton_t: eio_baton_t {
  eio_open_baton_t() : 
	eio_baton_t(), env(0), file(0), type(DB_UNKNOWN), flags(0), mode(0) {}
  DB_ENV *env;
  char *file;
  DBTYPE type;
  int flags;
  int mode;
};

struct eio_data_baton_t: eio_baton_t {
  eio_data_baton_t() : 
	eio_baton_t(), flags(0) {

	memset(&key, 0, sizeof(DBT));
	memset(&value, 0, sizeof(DBT));
  }

  DBT key;
  DBT value;
  int flags;
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
	baton->status = db_create(&db, baton->env, 0);
	if(baton->status != 0) return 0;
  }
  
  if(db != NULL && baton->status == 0) {
	baton->status = 
	  db->open(db,
			   NULL, // txn TODO
			   baton->file,
			   NULL, // db - this is generally useless, so hide it
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
  baton->value.flags = DB_DBT_MALLOC;
  if(db != NULL) {
	baton->status = 
	  db->get(db,
			  NULL, // txn
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

  if(db != NULL) {
	baton->status = 
	  db->put(db,
			  NULL, // txn
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

  if(db != NULL) {
	baton->status = 
	  db->del(db,
			  NULL, // txn
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
  baton->env = env->getDB_ENV();
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

Handle<Value> Db::Get(const Arguments& args) {
  HandleScope scope;

  Db* db = ObjectWrap::Unwrap<Db>(args.This());
  int flags = DEF_DATA_FLAGS;

  REQ_FN_ARG(args.Length() - 1, cb);

  REQ_BUF_ARG(0, key);
  if(args.Length() > 3) {
	REQ_INT_ARG(2, tmp);
	flags = tmp->Value();
  }

  eio_data_baton_t *baton = new eio_data_baton_t();
  baton->db = db;
  baton->cb = Persistent<Function>::New(cb);
  baton->flags = flags;
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

  REQ_FN_ARG(args.Length() - 1, cb);

  REQ_BUF_ARG(0, key);
  REQ_BUF_ARG(1, value);
  if(args.Length() > 3) {
	REQ_INT_ARG(2, tmp);
	flags = tmp->Value();
  }

  eio_data_baton_t *baton = new eio_data_baton_t();
  baton->db = db;
  baton->cb = Persistent<Function>::New(cb);
  baton->flags = flags;
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

  REQ_FN_ARG(args.Length() - 1, cb);

  REQ_BUF_ARG(0, key);
  if(args.Length() > 3) {
	REQ_INT_ARG(2, tmp);
	flags = tmp->Value();
  }

  eio_data_baton_t *baton = new eio_data_baton_t();
  baton->db = db;
  baton->cb = Persistent<Function>::New(cb);
  baton->flags = flags;
  baton->key.data = key;
  baton->key.size = key_len;
  
  db->Ref();  
  eio_custom(EIO_Del, EIO_PRI_DEFAULT, EIO_AfterDel, baton);
  ev_ref(EV_DEFAULT_UC);

  return Undefined();
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
  NODE_SET_PROTOTYPE_METHOD(t, "get", Get);
  NODE_SET_PROTOTYPE_METHOD(t, "put", Put);
  NODE_SET_PROTOTYPE_METHOD(t, "del", Del);  

  target->Set(String::NewSymbol("Db"), t->GetFunction());
}
