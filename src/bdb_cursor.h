// Copyright 2011 Mark Cavage <mcavage@gmail.com> All rights reserved.
#ifndef BDB_CURSOR_H_
#define BDB_CURSOR_H_

#include <node.h>
#include <v8.h>

#include "bdb_object.h"

class DbCursor: public DbObject {
 public:
  DbCursor();
  virtual ~DbCursor();

  static void Initialize(v8::Handle<v8::Object> target);

  static v8::Handle<v8::Value> New(const v8::Arguments &);
  static v8::Handle<v8::Value> Get(const v8::Arguments &);

  DBC *&getDBC();

  static const int DEF_DATA_FLAGS = DB_NEXT;

 protected:
  static int EIO_Get(eio_req *req);
  static int EIO_AfterGet(eio_req *req);
  static void GetCallback(EV_P_ ev_async *w, int revents);

 private:
  DbCursor(const DbCursor &);
  DbCursor &operator=(const DbCursor &);

  DBC *_cursor;
  ev_async _recordNotifier;
};

#endif  // BDB_CURSOR_H_
