// Copyright 2001 Mark Cavage <mark@bluesnoop.com> Sleepycat License
#ifndef BDB_CURSOR_H_
#define BDB_CURSOR_H_

#include <db.h>

#include "bdb_object.h"

class DbCursor: public DbObject {
public:
  DbCursor();
  virtual ~DbCursor();

  static void Initialize(v8::Handle<v8::Object> target);

  static v8::Handle<v8::Value> New(const v8::Arguments &);
  static v8::Handle<v8::Value> Get(const v8::Arguments &);
  static v8::Handle<v8::Value> Put(const v8::Arguments &);
  static v8::Handle<v8::Value> Del(const v8::Arguments &);

  DBC *&getDBC();

  static const int DEF_DATA_FLAGS = DB_NEXT;

private:
  DbCursor(const DbCursor &rhs);
  DbCursor &operator=(const DbCursor &rhs);

  static int EIO_Get(eio_req *req);
  static int EIO_AfterGet(eio_req *req);
  static int EIO_Put(eio_req *req);
  static int EIO_Del(eio_req *req);

  DBC *_cursor;
};

#endif  // BDB_CURSOR_H_
