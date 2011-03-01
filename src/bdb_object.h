// Copyright 2001 Mark Cavage <mark@bluesnoop.com> Sleepycat License
#ifndef BDB_OBJECT_H_
#define BDB_OBJECT_H_

#include <node.h>
#include <v8.h>

class Db;
class DbCursor;
class DbEnv;
class DbTxn;

class DbObject: public node::ObjectWrap {
 public:
  DbObject();
  virtual ~DbObject();

 protected:
  static int EIO_After_ReturnStatus(eio_req *req);

 private:
  DbObject(DbObject &);
  DbObject &operator=(DbObject &);

  friend class Db;
  friend class DbCursor;
  friend class DbEnv;
  friend class DbTxn;
};


class EIOBaton {
 public:
  explicit EIOBaton(DbObject *obj);
  virtual ~EIOBaton();

  // make these public to save on typing
  DbObject *object;
  int flags;
  int status;
  v8::Persistent<v8::Function> cb;

 private:
  EIOBaton();
  EIOBaton(const EIOBaton &);
  EIOBaton &operator=(EIOBaton &);
};

#endif  // BDB_OBJECT_H_
