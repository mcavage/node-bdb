#include <stdlib.h>
#include <string.h>

#include "bdb_db.h"
#include "bdb_env.h"

using namespace node;
using namespace v8;

struct eio_baton_t {
  eio_baton_t() : db(0), status(0) {}
  Db *db;
  int status;
  v8::Persistent<v8::Function> cb;
};

struct eio_open_baton_t: eio_baton_t {
  eio_open_baton_t() : 
	eio_baton_t(), env(0), file(0), type(DB_UNKNOWN), flags(0), mode(0) {}
  DB_ENV *env;
  char *file;
  DBTYPE type;
  int flags;
  int mode;
};
  

Db::Db(): _db(0) {}

Db::~Db() {
  if(_db != NULL) {
	_db->close(_db, 0);
  }
}

// Start EIO Methods

int Db::EIO_Open(eio_req *req) {
  eio_open_baton_t *baton = static_cast<eio_open_baton_t *>(req->data);
  
  DB *&db = baton->db->_db;

  if(db == NULL) {
	baton->status = db_create(&db, baton->env, 0);
	if(baton->status != 0) return 0;
  }
  
  if(db != NULL && baton->status == 0) {
	baton->status = 
	  db->open(db,
			   NULL, // txn TODO
			   baton->file,
			   NULL, // db - this is generally useless, so hide it
			   baton->type,
			   baton->flags,
			   baton->mode);	
  }

  return 0;
}

int Db::EIO_AfterOpen(eio_req *req) {
  HandleScope scope;
  eio_open_baton_t *baton = static_cast<eio_open_baton_t *>(req->data);
  ev_unref(EV_DEFAULT_UC);
  baton->db->Unref();
  
  DB_RES(baton->status, db_strerror(baton->status), msg);
  Local<Value> argv[1] = { msg };
  
  TryCatch try_catch;
  
  baton->cb->Call(Context::GetCurrent()->Global(), 1, argv);
  
  if (try_catch.HasCaught()) {
	FatalException(try_catch);
  }
  
  baton->cb.Dispose();
  
  if(baton->file != NULL) free(baton->file);
  delete baton;
  return 0;
}

// Start V8 Exposed Methods

Handle<Value> Db::Open(const Arguments& args) {
  HandleScope scope;
  
  Db* db = ObjectWrap::Unwrap<Db>(args.This());

  int flags = DEF_OPEN_FLAGS;
  DBTYPE type = DEF_TYPE;
  int mode = 0;

  REQ_FN_ARG(args.Length() - 1, cb);

  REQ_OBJ_ARG(0, envObj);
  DbEnv *env = node::ObjectWrap::Unwrap<DbEnv>(envObj);

  REQ_STR_ARG(1, file);
  if(args.Length() > 3) {
	REQ_INT_ARG(2, tmp);
	type = static_cast<DBTYPE>(tmp->Value());
  }
  if(args.Length() > 4) {
	REQ_INT_ARG(3, tmp);
	flags = tmp->Value();
  }
  if(args.Length() > 5) {
	REQ_INT_ARG(4, tmp);
	mode = tmp->Value();
  }

  eio_open_baton_t *baton = new eio_open_baton_t();
  baton->db = db;
  baton->cb = Persistent<Function>::New(cb);
  baton->env = env->getDB_ENV();
  baton->type = type;
  baton->flags = flags;
  baton->mode = mode;

  if(file.length() > 0) {
	baton->file = (char *)calloc(1, file.length() + 1);
	if(baton->file == NULL) {
	  LOG_OOM();
	  // WTF to do in node when OOM?
	} else { 
	  strncpy(baton->file, *file, file.length());
	}
  }
  
  db->Ref();  
  eio_custom(EIO_Open, EIO_PRI_DEFAULT, EIO_AfterOpen, baton);
  ev_ref(EV_DEFAULT_UC);
  
  return Undefined();
}

Handle<Value> Db::Put(const Arguments& args) {
  HandleScope scope;

  /*  
  Db* db = ObjectWrap::Unwrap<Db>(args.This());

  int flags = DEF_PUT_FLAGS;

  REQ_FN_ARG(args.Length() - 1, cb);

  REQ_STR_ARG(1, file);
  if(file.length() <= 0) {
	RET_EXC("file cannot be empty or undefined");
  }
  DBT key, data;


  db->_file = *file;
  db->_type = type;
  db->_flags = flags;
  db->_mode = mode;

  eio_baton_t *baton = new eio_baton_t();
  baton->db = db;
  baton->cb = Persistent<Function>::New(cb);
  
  db->Ref();  
  eio_custom(EIO_Open, EIO_PRI_DEFAULT, EIO_AfterOpen, baton);
  ev_ref(EV_DEFAULT_UC);
  */
  return Undefined();
}

Handle<Value> Db::New(const Arguments& args) {
  HandleScope scope;

  Db* db = new Db();
  db->Wrap(args.This());
  return args.This();
}

void Db::Initialize(Handle<Object> target) {
  HandleScope scope;
  
  Local<FunctionTemplate> t = FunctionTemplate::New(New);
  t->InstanceTemplate()->SetInternalFieldCount(1);
  
  /// Open flags
  // type
  NODE_DEFINE_CONSTANT(target, DB_BTREE);
  NODE_DEFINE_CONSTANT(target, DB_HASH);
  NODE_DEFINE_CONSTANT(target, DB_RECNO);
  NODE_DEFINE_CONSTANT(target, DB_QUEUE);
  NODE_DEFINE_CONSTANT(target, DB_UNKNOWN);
  // flags
  NODE_DEFINE_CONSTANT(target, DB_AUTO_COMMIT);
  NODE_DEFINE_CONSTANT(target, DB_CREATE);
  NODE_DEFINE_CONSTANT(target, DB_EXCL);
  NODE_DEFINE_CONSTANT(target, DB_MULTIVERSION);
  NODE_DEFINE_CONSTANT(target, DB_NOMMAP);
  NODE_DEFINE_CONSTANT(target, DB_RDONLY);
  NODE_DEFINE_CONSTANT(target, DB_READ_UNCOMMITTED);
  NODE_DEFINE_CONSTANT(target, DB_THREAD);
  NODE_DEFINE_CONSTANT(target, DB_TRUNCATE);
  /// Put flags
  NODE_DEFINE_CONSTANT(target, DB_APPEND);
  NODE_DEFINE_CONSTANT(target, DB_NODUPDATA);
  NODE_DEFINE_CONSTANT(target, DB_NOOVERWRITE);
  NODE_DEFINE_CONSTANT(target, DB_MULTIPLE);
  NODE_DEFINE_CONSTANT(target, DB_MULTIPLE_KEY);
  NODE_DEFINE_CONSTANT(target, DB_OVERWRITE_DUP);

  NODE_SET_PROTOTYPE_METHOD(t, "open", Open);
  NODE_SET_PROTOTYPE_METHOD(t, "put", Put);
  
  target->Set(String::NewSymbol("Db"), t->GetFunction());
}
