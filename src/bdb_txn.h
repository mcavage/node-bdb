// Copyright 2011 Mark Cavage <mcavage@gmail.com> All rights reserved.
#ifndef BDB_TXN_H_
#define BDB_TXN_H_

#include <db.h>

#include "bdb_object.h"

class DbTxn: public DbObject {
 public:
  DbTxn();
  virtual ~DbTxn();

  DB_TXN *&getDB_TXN();

  static void Initialize(v8::Handle<v8::Object>);

  static v8::Handle<v8::Value> New(const v8::Arguments &);
  static v8::Handle<v8::Value> Id(const v8::Arguments &);
  static v8::Handle<v8::Value> AbortS(const v8::Arguments &);
  static v8::Handle<v8::Value> CommitS(const v8::Arguments &);

 private:
  DbTxn(const DbTxn &);
  DbTxn &operator=(const DbTxn &);

  static int EIO_Abort(eio_req *req);
  static int EIO_Commit(eio_req *req);
  static int EIO_After(eio_req *req);

  DB_TXN *_txn;
};

#endif  // BDB_TXN_H_
