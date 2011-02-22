#ifndef __NODE_BDB_ENV_H__
#define __NODE_BDB_ENV_H__

#include "common.h"

class DbEnv: node::ObjectWrap {
public:
  DbEnv();
  ~DbEnv();
  
  DB_ENV *getDB_ENV();

  static void Initialize(v8::Handle<v8::Object> target);
  
  static v8::Handle<v8::Value> New(const v8::Arguments&);
  static v8::Handle<v8::Value> Open(const v8::Arguments &);
  static v8::Handle<v8::Value> OpenSync(const v8::Arguments &);
  static v8::Handle<v8::Value> SetShmKeySync(const v8::Arguments &);

  static const int DEF_OPEN_FLAGS = DB_CREATE | DB_INIT_MPOOL | 
	DB_INIT_CDB | DB_THREAD;

private:
  DbEnv(const DbEnv &);
  DbEnv &operator=(const DbEnv &);
  
  static int EIO_Open(eio_req *req);
  static int EIO_AfterOpen(eio_req *req);
  
  DB_ENV *_env;  
};

#endif
