#include <stdlib.h>
#include <string.h>

#include <node_buffer.h>

#include "bdb_cursor.h"
#include "bdb_db.h"


using namespace node;
using namespace v8;

struct eio_data_baton_t {
  eio_data_baton_t() :
    cursor(0), status(0), flags(0) {

    memset(&key, 0, sizeof(DBT));
    memset(&value, 0, sizeof(DBT));
  }

  DbCursor *cursor;
  int status;
  v8::Persistent<v8::Function> cb;
  DBT key;
  DBT value;
  int flags;
};


DbCursor::DbCursor(): _cursor(0) {}

DbCursor::~DbCursor() {
  if(_cursor != NULL) {
    _cursor->close(_cursor);
  }
}

DBC *DbCursor::getDBC() {
  return _cursor;
}

// Start EIO Methods

int DbCursor::EIO_Get(eio_req *req) {
  eio_data_baton_t *baton = static_cast<eio_data_baton_t *>(req->data);

  DBC *&cursor = baton->cursor->_cursor;
  baton->value.flags = DB_DBT_MALLOC;
  if(cursor != NULL) {
	baton->status =
	  cursor->get(cursor,
		      &(baton->key),
		      &(baton->value),
		      baton->flags);
  }

  return 0;
}

int DbCursor::EIO_AfterGet(eio_req *req) {
  HandleScope scope;
  eio_data_baton_t *baton = static_cast<eio_data_baton_t *>(req->data);
  ev_unref(EV_DEFAULT_UC);

  DB_RES(baton->status, db_strerror(baton->status), msg);
  Buffer *key = Buffer::New(baton->key.size);
  memcpy(Buffer::Data(key), baton->key.data, baton->key.size);
  Buffer *val = Buffer::New(baton->value.size);
  memcpy(Buffer::Data(val), baton->value.data, baton->value.size);

  Handle<Value> argv[3];
  argv[0] = msg;
  argv[1] = key->handle_;
  argv[2] = val->handle_;

  TryCatch try_catch;

  baton->cb->Call(Context::GetCurrent()->Global(), 2, argv);

  if (try_catch.HasCaught()) {
	FatalException(try_catch);
  }

  baton->cursor->Unref();
  baton->cb.Dispose();
  if(baton->key.data != NULL) free(baton->key.data);
  if(baton->value.data != NULL) free(baton->value.data);
  delete baton;
  return 0;
}

int DbCursor::EIO_Put(eio_req *req) {
  eio_data_baton_t *baton = static_cast<eio_data_baton_t *>(req->data);

  DBC *&cursor = baton->cursor->_cursor;

  if(cursor != NULL) {
	baton->status =
	  cursor->put(cursor,
		      &(baton->key),
		      &(baton->value),
		      baton->flags);
  }

  return 0;
}

int DbCursor::EIO_AfterPut(eio_req *req) {
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

  baton->cursor->Unref();
  baton->cb.Dispose();
  delete baton;
  return 0;
}

int DbCursor::EIO_Del(eio_req *req) {
  eio_data_baton_t *baton = static_cast<eio_data_baton_t *>(req->data);

  DBC *&cursor = baton->cursor->_cursor;

  if(cursor != NULL) {
	baton->status =
	  cursor->del(cursor,
		      baton->flags);
  }

  return 0;
}

int DbCursor::EIO_AfterDel(eio_req *req) {
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

  baton->cursor->Unref();
  baton->cb.Dispose();
  delete baton;
  return 0;
}

// Start V8 Exposed Methods

Handle<Value> DbCursor::Get(const Arguments& args) {
  HandleScope scope;

  DbCursor *cursor = ObjectWrap::Unwrap<DbCursor>(args.This());
  int flags = DEF_DATA_FLAGS;

  REQ_FN_ARG(args.Length() - 1, cb);

  if(args.Length() > 1) {
	REQ_INT_ARG(0, tmp);
	flags = tmp->Value();
  }

  eio_data_baton_t *baton = new eio_data_baton_t();
  baton->cursor = cursor;
  baton->cb = Persistent<Function>::New(cb);
  baton->flags = flags;

  cursor->Ref();
  eio_custom(EIO_Get, EIO_PRI_DEFAULT, EIO_AfterGet, baton);
  ev_ref(EV_DEFAULT_UC);

  return Undefined();
}

Handle<Value> DbCursor::Put(const Arguments& args) {
  HandleScope scope;

  DbCursor *cursor = ObjectWrap::Unwrap<DbCursor>(args.This());
  int flags = DEF_DATA_FLAGS;

  REQ_FN_ARG(args.Length() - 1, cb);

  REQ_BUF_ARG(0, key);
  REQ_BUF_ARG(1, value);
  if(args.Length() > 3) {
	REQ_INT_ARG(2, tmp);
	flags = tmp->Value();
  }

  eio_data_baton_t *baton = new eio_data_baton_t();
  baton->cursor = cursor;
  baton->cb = Persistent<Function>::New(cb);
  baton->flags = flags;
  baton->key.data = key;
  baton->key.size = key_len;
  baton->value.data = value;
  baton->value.size = value_len;

  cursor->Ref();
  eio_custom(EIO_Put, EIO_PRI_DEFAULT, EIO_AfterPut, baton);
  ev_ref(EV_DEFAULT_UC);

  return Undefined();
}

Handle<Value> DbCursor::Del(const Arguments& args) {
  HandleScope scope;

  DbCursor *cursor = ObjectWrap::Unwrap<DbCursor>(args.This());
  int flags = DEF_DATA_FLAGS;

  REQ_FN_ARG(args.Length() - 1, cb);

  if(args.Length() > 1) {
	REQ_INT_ARG(0, tmp);
	flags = tmp->Value();
  }

  eio_data_baton_t *baton = new eio_data_baton_t();
  baton->cursor = cursor;
  baton->cb = Persistent<Function>::New(cb);
  baton->flags = flags;

  cursor->Ref();
  eio_custom(EIO_Del, EIO_PRI_DEFAULT, EIO_AfterDel, baton);
  ev_ref(EV_DEFAULT_UC);

  return Undefined();
}

Handle<Value> DbCursor::New(const Arguments& args) {
  HandleScope scope;

  DbCursor* cursor = new DbCursor();
  cursor->Wrap(args.This());
  return args.This();
}

void DbCursor::Initialize(Handle<Object> target) {
  HandleScope scope;

  Local<FunctionTemplate> t = FunctionTemplate::New(New);
  t->InstanceTemplate()->SetInternalFieldCount(1);

  NODE_SET_PROTOTYPE_METHOD(t, "get", Get);
  NODE_SET_PROTOTYPE_METHOD(t, "put", Put);
  NODE_SET_PROTOTYPE_METHOD(t, "del", Del);

  target->Set(String::NewSymbol("DbCursor"), t->GetFunction());
}
