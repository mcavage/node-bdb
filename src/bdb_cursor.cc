// Copyright 2001 Mark Cavage <mark@bluesnoop.com> Sleepycat License
#include <stdlib.h>
#include <string.h>

#include <node_buffer.h>

#include "bdb_common.h"
#include "bdb_cursor.h"
#include "bdb_db.h"

using v8::FunctionTemplate;

class EIOCursorBaton: public EIOBaton {
 public:
  EIOCursorBaton(DbCursor *cursor): EIOBaton(cursor) {
    memset(&key, 0, sizeof (DBT));
    memset(&val, 0, sizeof (DBT));
  }

  virtual ~EIOCursorBaton() {}

  DBT key;
  DBT val;

 private:
  EIOCursorBaton(const EIOCursorBaton &);
  EIOCursorBaton &operator=(const EIOCursorBaton &);
};

DbCursor::DbCursor(): DbObject(), _cursor(0) {}

DbCursor::~DbCursor() {
  if(_cursor != NULL) {
    _cursor->close(_cursor);
    _cursor = NULL;
  }
}

DBC *&DbCursor::getDBC() {
  return _cursor;
}

// Start EIO Methods

int DbCursor::EIO_Get(eio_req *req) {
  EIOCursorBaton *baton = static_cast<EIOCursorBaton *>(req->data);

  if (baton->object == NULL ||
      dynamic_cast<DbCursor *>(baton->object)->_cursor == NULL) {
    return 0;
  }

  DBC *&cursor = dynamic_cast<DbCursor *>(baton->object)->_cursor;
  baton->key.flags = DB_DBT_MALLOC;
  baton->val.flags = DB_DBT_MALLOC;
  baton->status =
      cursor->get(cursor, &(baton->key), &(baton->val), baton->flags);

  return 0;
}

int DbCursor::EIO_AfterGet(eio_req *req) {
  v8::HandleScope scope;
  EIOCursorBaton *baton = static_cast<EIOCursorBaton *>(req->data);
  ev_unref(EV_DEFAULT_UC);

  DB_RES(baton->status, db_strerror(baton->status), msg);
  node::Buffer *key = node::Buffer::New(baton->key.size);
  memcpy(node::Buffer::Data(key), baton->key.data, baton->key.size);
  node::Buffer *val = node::Buffer::New(baton->val.size);
  memcpy(node::Buffer::Data(val), baton->val.data, baton->val.size);

  v8::Handle<v8::Value> argv[3];
  argv[0] = msg;
  argv[1] = key->handle_;
  argv[2] = val->handle_;

  v8::TryCatch try_catch;

  baton->cb->Call(v8::Context::GetCurrent()->Global(), 3, argv);

  if (try_catch.HasCaught())
    node::FatalException(try_catch);

  baton->object->Unref();
  baton->cb.Dispose();

  if (baton->key.data != NULL)
    free(baton->key.data);
  if (baton->val.data != NULL)
    free(baton->val.data);

  delete baton;
  return 0;
}

int DbCursor::EIO_Put(eio_req *req) {
  EIOCursorBaton *baton = static_cast<EIOCursorBaton *>(req->data);

  if (baton->object == NULL ||
      dynamic_cast<DbCursor *>(baton->object)->_cursor == NULL) {
    return 0;
  }

  DBC *&cursor = dynamic_cast<DbCursor *>(baton->object)->_cursor;

  baton->status =
      cursor->put(cursor, &(baton->key), &(baton->val), baton->flags);

  return 0;
}

int DbCursor::EIO_Del(eio_req *req) {
  EIOCursorBaton *baton = static_cast<EIOCursorBaton *>(req->data);

  if (baton->object == NULL ||
      dynamic_cast<DbCursor *>(baton->object)->_cursor == NULL) {
    return 0;
  }

  DBC *&cursor = dynamic_cast<DbCursor *>(baton->object)->_cursor;
  baton->status = cursor->del(cursor, baton->flags);

  return 0;
}

// Start V8 Exposed Methods

v8::Handle<v8::Value> DbCursor::Get(const v8::Arguments& args) {
  v8::HandleScope scope;

  DbCursor *cursor = node::ObjectWrap::Unwrap<DbCursor>(args.This());
  int flags = DEF_DATA_FLAGS;

  REQ_FN_ARG(args.Length() - 1, cb);

  if(args.Length() > 1) {
	REQ_INT_ARG(0, tmp);
	flags = tmp->Value();
  }

  EIOCursorBaton *baton = new EIOCursorBaton(cursor);
  baton->cb = v8::Persistent<v8::Function>::New(cb);
  baton->flags = flags;

  cursor->Ref();
  eio_custom(EIO_Get, EIO_PRI_DEFAULT, EIO_AfterGet, baton);
  ev_ref(EV_DEFAULT_UC);

  return v8::Undefined();
}

v8::Handle<v8::Value> DbCursor::Put(const v8::Arguments& args) {
  v8::HandleScope scope;

  DbCursor *cursor = ObjectWrap::Unwrap<DbCursor>(args.This());
  int flags = DB_KEYFIRST;

  REQ_FN_ARG(args.Length() - 1, cb);

  REQ_BUF_ARG(0, key);
  REQ_BUF_ARG(1, value);
  if(args.Length() > 3) {
	REQ_INT_ARG(2, tmp);
	flags = tmp->Value();
  }

  EIOCursorBaton *baton = new EIOCursorBaton(cursor);
  baton->cb = v8::Persistent<v8::Function>::New(cb);
  baton->flags = flags;
  baton->key.data = key;
  baton->key.size = key_len;
  baton->val.data = value;
  baton->val.size = value_len;

  cursor->Ref();
  eio_custom(EIO_Put, EIO_PRI_DEFAULT, EIO_After_ReturnStatus, baton);
  ev_ref(EV_DEFAULT_UC);

  return v8::Undefined();
}

v8::Handle<v8::Value> DbCursor::Del(const v8::Arguments& args) {
  v8::HandleScope scope;

  DbCursor *cursor = ObjectWrap::Unwrap<DbCursor>(args.This());
  int flags = 0;

  REQ_FN_ARG(args.Length() - 1, cb);

  if(args.Length() > 1) {
	REQ_INT_ARG(0, tmp);
	flags = tmp->Value();
  }

  EIOCursorBaton *baton = new EIOCursorBaton(cursor);
  baton->cb = v8::Persistent<v8::Function>::New(cb);
  baton->flags = flags;

  cursor->Ref();
  eio_custom(EIO_Del, EIO_PRI_DEFAULT, EIO_After_ReturnStatus, baton);
  ev_ref(EV_DEFAULT_UC);

  return v8::Undefined();
}

v8::Handle<v8::Value> DbCursor::New(const v8::Arguments& args) {
  v8::HandleScope scope;

  DbCursor* cursor = new DbCursor();
  cursor->Wrap(args.This());
  return args.This();
}

void DbCursor::Initialize(v8::Handle<v8::Object> target) {
  v8::HandleScope scope;

  v8::Local<v8::FunctionTemplate> t = v8::FunctionTemplate::New(New);
  t->InstanceTemplate()->SetInternalFieldCount(1);

  NODE_SET_PROTOTYPE_METHOD(t, "get", Get);
  NODE_SET_PROTOTYPE_METHOD(t, "put", Put);
  NODE_SET_PROTOTYPE_METHOD(t, "del", Del);

  target->Set(v8::String::NewSymbol("DbCursor"), t->GetFunction());
}
