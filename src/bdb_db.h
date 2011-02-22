#ifndef __NODE_BDB_DB_H__
#define __NODE_BDB_DB_H__

#include "common.h"

class Db: node::ObjectWrap {
public:
  Db();
  ~Db();

  static void Initialize(v8::Handle<v8::Object> target);
  
  static v8::Handle<v8::Value> New(const v8::Arguments&);
  static v8::Handle<v8::Value> Open(const v8::Arguments &);
  static v8::Handle<v8::Value> OpenSync(const v8::Arguments &);
  static v8::Handle<v8::Value> Get(const v8::Arguments &);
  static v8::Handle<v8::Value> Put(const v8::Arguments &);
  static v8::Handle<v8::Value> Del(const v8::Arguments &);

  static int EIO_Open(eio_req *req);
  static int EIO_AfterOpen(eio_req *req);
  static int EIO_Get(eio_req *req);
  static int EIO_AfterGet(eio_req *req);
  static int EIO_Put(eio_req *req);
  static int EIO_AfterPut(eio_req *req);
  static int EIO_Del(eio_req *req);
  static int EIO_AfterDel(eio_req *req);

  static int EIO_Callback(eio_req *req);

  static const DBTYPE DEF_TYPE = DB_BTREE;
  static const int DEF_OPEN_FLAGS = DB_CREATE | DB_THREAD;
  static const int DEF_DATA_FLAGS = 0;

private:
  Db(const Db &rhs);
  Db &operator=(const Db &rhs);

  DB *_db;
};

#endif
