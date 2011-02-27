#ifndef __NODE_BDB_CURSOR_H__
#define __NODE_BDB_CURSOR_H__

#include "common.h"

class DbCursor: node::ObjectWrap {
public:
  DbCursor();
  ~DbCursor();

  static void Initialize(v8::Handle<v8::Object> target);

  static v8::Handle<v8::Value> New(const v8::Arguments &);
  static v8::Handle<v8::Value> Get(const v8::Arguments &);
  static v8::Handle<v8::Value> Put(const v8::Arguments &);
  static v8::Handle<v8::Value> Del(const v8::Arguments &);

  static int EIO_Get(eio_req *req);
  static int EIO_AfterGet(eio_req *req);
  static int EIO_Put(eio_req *req);
  static int EIO_AfterPut(eio_req *req);
  static int EIO_Del(eio_req *req);
  static int EIO_AfterDel(eio_req *req);

  DBC *getDBC();

  static const int DEF_DATA_FLAGS = DB_NEXT;

private:
  DbCursor(const DbCursor &rhs);
  DbCursor &operator=(const DbCursor &rhs);

  DBC *_cursor;
};

#endif
