// Copyright 2011 Mark Cavage <mcavage@gmail.com> All rights reserved.
#ifndef BDB_DB_H_
#define BDB_DB_H_

#include "bdb_object.h"

class Db: public DbObject {
 public:
  Db();
  virtual ~Db();

  static void Initialize(v8::Handle<v8::Object> target);

  static v8::Handle<v8::Value> AssociateS(const v8::Arguments &);
  static v8::Handle<v8::Value> CloseS(const v8::Arguments &);
  static v8::Handle<v8::Value> CursorGet(const v8::Arguments &);
  static v8::Handle<v8::Value> CursorGetS(const v8::Arguments &);
  static v8::Handle<v8::Value> Del(const v8::Arguments &);
  static v8::Handle<v8::Value> DelS(const v8::Arguments &);
  static v8::Handle<v8::Value> Get(const v8::Arguments &);
  static v8::Handle<v8::Value> GetS(const v8::Arguments &);
  static v8::Handle<v8::Value> New(const v8::Arguments &);
  static v8::Handle<v8::Value> OpenS(const v8::Arguments &);
  static v8::Handle<v8::Value> Put(const v8::Arguments &);
  static v8::Handle<v8::Value> PutIf(const v8::Arguments &);
  static v8::Handle<v8::Value> PutS(const v8::Arguments &);
  static v8::Handle<v8::Value> SetEncrypt(const v8::Arguments &);
  static v8::Handle<v8::Value> SetFlags(const v8::Arguments &);
  static v8::Handle<v8::Value> Fd(const v8::Arguments &);

 protected:
  static void EIO_Get(eio_req *req);
  static int EIO_AfterGet(eio_req *req);
  static void EIO_CursorGet(eio_req *req);
  static int EIO_AfterCursorGet(eio_req *req);
  static void EIO_Put(eio_req *req);
  static void EIO_PutIf(eio_req *req);
  static void EIO_Del(eio_req *req);

 private:
  Db(const Db &rhs);
  Db &operator=(const Db &rhs);

  DB *_db;
  DB_ENV *_env;
  int _retries;
  bool _transactional;
};

#endif  // BDB_DB_H_
