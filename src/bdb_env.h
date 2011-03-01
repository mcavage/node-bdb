// Copyright 2001 Mark Cavage <mark@bluesnoop.com> Sleepycat License
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

  static v8::Handle<v8::Value> New(const v8::Arguments &);

  static v8::Handle<v8::Value> Open(const v8::Arguments &);
  static v8::Handle<v8::Value> SetLockDetect(const v8::Arguments &);
  static v8::Handle<v8::Value> SetLockTimeout(const v8::Arguments &);
  static v8::Handle<v8::Value> SetMaxLocks(const v8::Arguments &);
  static v8::Handle<v8::Value> SetMaxLockers(const v8::Arguments &);
  static v8::Handle<v8::Value> SetMaxLockObjects(const v8::Arguments &);
  static v8::Handle<v8::Value> SetShmKey(const v8::Arguments &);
  static v8::Handle<v8::Value> SetTxnMax(const v8::Arguments &);
  static v8::Handle<v8::Value> SetTxnTimeout(const v8::Arguments &);
  static v8::Handle<v8::Value> TxnBegin(const v8::Arguments &);
  static v8::Handle<v8::Value> TxnCheckpoint(const v8::Arguments &);

  static const int DEF_OPEN_FLAGS =
    DB_CREATE     |
    DB_INIT_TXN   |
    DB_INIT_LOCK  |
    DB_INIT_LOG   |
    DB_INIT_MPOOL |
    DB_REGISTER   |
    DB_RECOVER    |
    DB_THREAD;


 private:
  DbEnv(const DbEnv &);
  DbEnv &operator=(const DbEnv &);

  static int EIO_Checkpoint(eio_req *);

  DB_ENV *_env;
};

#endif  // BDB_ENV_H_
