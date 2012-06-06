// Copyright 2011 Mark Cavage <mcavage@gmail.com> All rights reserved.
#ifndef BDB_ENV_H_
#define BDB_ENV_H_

#include <db.h>

#include "bdb_object.h"


class DbEnv: public DbObject {
 public:
  DbEnv();
  virtual ~DbEnv();

  DB_ENV *&getDB_ENV();

  static void Initialize(v8::Handle<v8::Object> target);

  static v8::Handle<v8::Value> CloseS(const v8::Arguments &);
  static v8::Handle<v8::Value> New(const v8::Arguments &);
  static v8::Handle<v8::Value> OpenS(const v8::Arguments &);
  static v8::Handle<v8::Value> SetEncrypt(const v8::Arguments &);
  static v8::Handle<v8::Value> SetErrorFile(const v8::Arguments &);
  static v8::Handle<v8::Value> SetErrorPrefix(const v8::Arguments &);
  static v8::Handle<v8::Value> SetFlags(const v8::Arguments &);
  static v8::Handle<v8::Value> SetLockDetect(const v8::Arguments &);
  static v8::Handle<v8::Value> SetLockTimeout(const v8::Arguments &);
  static v8::Handle<v8::Value> SetMaxLocks(const v8::Arguments &);
  static v8::Handle<v8::Value> SetMaxLockers(const v8::Arguments &);
  static v8::Handle<v8::Value> SetMaxLockObjects(const v8::Arguments &);
  static v8::Handle<v8::Value> SetShmKey(const v8::Arguments &);
  static v8::Handle<v8::Value> SetTxnMax(const v8::Arguments &);
  static v8::Handle<v8::Value> SetTxnTimeout(const v8::Arguments &);
  static v8::Handle<v8::Value> TxnCheckpoint(const v8::Arguments &);

  bool isTransactional();

 private:
  DbEnv(const DbEnv &);
  DbEnv &operator=(const DbEnv &);

  static void EIO_Checkpoint(eio_req *req);

  bool _transactional;
  DB_ENV *_env;
};

#endif  // BDB_ENV_H_
