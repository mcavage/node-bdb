#ifndef __NODE_BDB_TXN_H__
#define __NODE_BDB_TXN_H__

#include "common.h"

class DbTxn: node::ObjectWrap {
public:
  DbTxn();
  ~DbTxn();

  DB_TXN *getDB_TXN();

  static void Initialize(v8::Handle<v8::Object>);

  static v8::Handle<v8::Value> New(const v8::Arguments &);
  static v8::Handle<v8::Value> Id(const v8::Arguments &);
  static v8::Handle<v8::Value> Abort(const v8::Arguments &);
  static v8::Handle<v8::Value> Commit(const v8::Arguments &);

private:
  DbTxn(const DbTxn &);
  DbTxn &operator=(const DbTxn &);

  static int EIO_Abort(eio_req *);
  static int EIO_Commit(eio_req *);
  static int EIO_After(eio_req *);

  DB_TXN *_txn;
};

#endif
