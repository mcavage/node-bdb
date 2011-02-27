#include <string>
#include "bdb_txn.h"

using namespace node;
using namespace v8;

v8::Persistent<v8::String> txn_id_sym;

struct eio_baton_t {
  eio_baton_t() : txn(0), status(0), flags(0) {}
  DbTxn *txn;
  int status;
  int flags;
  v8::Persistent<v8::Function> cb;
};

DbTxn::DbTxn(): _txn(0) {}

DbTxn::~DbTxn() {}

DB_TXN *DbTxn::getDB_TXN() {
  return _txn;
}

// Start EIO Methods

int DbTxn::EIO_Commit(eio_req *req) {
  eio_baton_t *baton = static_cast<eio_baton_t *>(req->data);

  DB_TXN *&txn = baton->txn->_txn;

  if(txn != NULL) {
    baton->status = txn->commit(txn, baton->flags);
  }

  return 0;
}

int DbTxn::EIO_Abort(eio_req *req) {
  eio_baton_t *baton = static_cast<eio_baton_t *>(req->data);

  DB_TXN *&txn = baton->txn->_txn;

  if(txn != NULL) {
    baton->status = txn->abort(txn);
  }

  return 0;
}

int DbTxn::EIO_After(eio_req *req) {
  HandleScope scope;

  eio_baton_t *baton = static_cast<eio_baton_t *>(req->data);
  ev_unref(EV_DEFAULT_UC);
  baton->txn->Unref();

  DB_RES(baton->status, db_strerror(baton->status), msg);
  Local<Value> argv[1] = { msg };

  TryCatch try_catch;

  baton->cb->Call(Context::GetCurrent()->Global(), 1, argv);

  if (try_catch.HasCaught()) {
    FatalException(try_catch);
  }

  baton->cb.Dispose();

  delete baton;
  return 0;
}


// Start V8 Exposed Methods

v8::Handle<v8::Value> DbTxn::Id(const v8::Arguments &args) {
  DbTxn* txn = ObjectWrap::Unwrap<DbTxn>(args.This());

  unsigned int id = txn->_txn->id(txn->_txn);

  DB_RES(0, db_strerror(0), msg);
  msg->Set(txn_id_sym, v8::Integer::New(id));
  v8::Handle<v8::Value> result = { msg };
  return result;
}

v8::Handle<v8::Value> DbTxn::Abort(const v8::Arguments &args) {
  HandleScope scope;

  DbTxn* txn = ObjectWrap::Unwrap<DbTxn>(args.This());

  REQ_FN_ARG(0, cb);

  eio_baton_t *baton = new eio_baton_t();
  baton->txn = txn;
  baton->cb = Persistent<Function>::New(cb);

  txn->Ref();
  eio_custom(EIO_Abort, EIO_PRI_DEFAULT, EIO_After, baton);
  ev_ref(EV_DEFAULT_UC);

  return Undefined();
}

v8::Handle<v8::Value> DbTxn::Commit(const v8::Arguments &args) {
  HandleScope scope;

  DbTxn* txn = ObjectWrap::Unwrap<DbTxn>(args.This());
  int flags = 0;

  REQ_FN_ARG(args.Length() - 1, cb);
  if(args.Length() > 1) {
	REQ_INT_ARG(0, tmp);
	flags = tmp->Value();
  }

  eio_baton_t *baton = new eio_baton_t();
  baton->txn = txn;
  baton->cb = Persistent<Function>::New(cb);
  baton->flags = flags;

  txn->Ref();
  eio_custom(EIO_Commit, EIO_PRI_DEFAULT, EIO_After, baton);
  ev_ref(EV_DEFAULT_UC);

  return Undefined();
}

v8::Handle<v8::Value> DbTxn::New(const v8::Arguments& args) {
  v8::HandleScope scope;

  DbTxn* txn = new DbTxn();
  txn->Wrap(args.This());
  return args.This();
}

void DbTxn::Initialize(v8::Handle<v8::Object> target) {
  v8::HandleScope scope;

  v8::Local<v8::FunctionTemplate> t = v8::FunctionTemplate::New(New);
  t->InstanceTemplate()->SetInternalFieldCount(1);

  txn_id_sym = NODE_PSYMBOL("id");

  NODE_SET_PROTOTYPE_METHOD(t, "abort", Abort);
  NODE_SET_PROTOTYPE_METHOD(t, "commit", Commit);
  NODE_SET_PROTOTYPE_METHOD(t, "id", Id);

  target->Set(v8::String::NewSymbol("DbTxn"), t->GetFunction());
}
