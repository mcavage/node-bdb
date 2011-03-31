// Copyright 2011 Mark Cavage <mcavage@gmail.com> All rights reserved.
#include <stdlib.h>
#include <string.h>

#include <node_buffer.h>

#include "bdb_common.h"
#include "bdb_cursor.h"
#include "bdb_db.h"

using v8::FunctionTemplate;

class EIOCursorBaton: public EIOBaton {
 public:
  explicit EIOCursorBaton(DbCursor *cursor): EIOBaton(cursor),keyPrefix(0) {
    memset(&key, 0, sizeof(DBT));
    memset(&val, 0, sizeof(DBT));
  }

  virtual ~EIOCursorBaton() {
    cb.Dispose();

    if (keyPrefix != NULL) {
      free(keyPrefix);
      keyPrefix = NULL;
    }
    if (key.data != NULL) {
      free(key.data);
      key.data = NULL;
    }
    if (val.data != NULL) {
      free(val.data);
      val.data = NULL;
    }
  }

  char *keyPrefix;
  DBT key;
  DBT val;

 private:
  EIOCursorBaton(const EIOCursorBaton &);
  EIOCursorBaton &operator=(const EIOCursorBaton &);
};

DbCursor::DbCursor(): DbObject(), _cursor(0) {
  //  ev_async_init(&_recordNotifier, GetCallback);
  //  ev_async_start(EV_DEFAULT_UC_ &_recordNotifier);
  //   ev_unref(EV_DEFAULT_UC);
}

DbCursor::~DbCursor() {
  if (_cursor != NULL) {
    _cursor->close(_cursor);
    _cursor = NULL;
  }
}

// Our async watcher callback; on invocation emits the queue events
/*
void DbCursor::GetCallback(EV_P_ ev_async *w, int revents) {
  HandleScope scope;
  assert(w == &_recordsNotifier);
  assert(revents == EV_ASYNC);
  // todo: emit
}
*/

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

  int ret = cursor->get(cursor, &key, &data, DB_SET_RANGE);
  if (ret == 0) {
    while ((ret = cursor->get(cursor, &key, &data, DB_NEXT)) == 0) {

    }
  }

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
  delete baton;
  return 0;
}


// Start V8 Exposed Methods

v8::Handle<v8::Value> DbCursor::Get(const v8::Arguments& args) {
  v8::HandleScope scope;

  DbCursor *cursor = node::ObjectWrap::Unwrap<DbCursor>(args.This());

  REQ_STR_ARG(0, keyPrefix);
  REQ_INT_ARG(1, flags);
  REQ_FN_ARG(2, cb);

  EIOCursorBaton *baton = new EIOCursorBaton(cursor);
  baton->cb = v8::Persistent<v8::Function>::New(cb);
  baton->flags = flags;
  baton->keyPrefix = strdup(*keyPrefix);
  if (baton->keyPrefix == NULL) {
    delete baton;
    RET_EXC("Out of Memory");
  }

  cursor->Ref();
  eio_custom(EIO_Get, EIO_PRI_DEFAULT, EIO_AfterGet, baton);
  ev_ref(EV_DEFAULT_UC);

  return v8::Undefined();
}


DBC *&DbCursor::getDBC() {
  return _cursor;
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

  NODE_SET_PROTOTYPE_METHOD(t, "_get", Get);

  target->Set(v8::String::NewSymbol("DbCursor"), t->GetFunction());
}
