// Copyright 2001 Mark Cavage <mark@bluesnoop.com> Sleepycat License
#include <string>

#include "bdb_common.h"
#include "bdb_txn.h"

using v8::FunctionTemplate;
using v8::Persistent;
using v8::String;

v8::Persistent<v8::String> txn_id_sym;

DbTxn::DbTxn(): _txn(0) {}

DbTxn::~DbTxn() {}

DB_TXN *&DbTxn::getDB_TXN() {
  return _txn;
}

// Start EIO Methods

int DbTxn::EIO_Commit(eio_req *req) {
  EIOBaton *baton = static_cast<EIOBaton *>(req->data);

  if (baton->object == NULL ||
      dynamic_cast<DbTxn *>(baton->object)->_txn == NULL) {
    return 0;
  }

  DB_TXN *&txn = dynamic_cast<DbTxn *>(baton->object)->_txn;
  baton->status = txn->commit(txn, baton->flags);

  return 0;
}

int DbTxn::EIO_Abort(eio_req *req) {
  EIOBaton *baton = static_cast<EIOBaton *>(req->data);

  if (baton->object == NULL ||
      dynamic_cast<DbTxn *>(baton->object)->_txn == NULL) {
    return 0;
  }

  DB_TXN *&txn = dynamic_cast<DbTxn *>(baton->object)->_txn;
  baton->status = txn->abort(txn);

  return 0;
}

// Start V8 Exposed Methods

v8::Handle<v8::Value> DbTxn::Id(const v8::Arguments &args) {
  v8::HandleScope scope;

  DbTxn* txn = node::ObjectWrap::Unwrap<DbTxn>(args.This());
  unsigned int id = txn->_txn->id(txn->_txn);

  DB_RES(0, db_strerror(0), msg);
  msg->Set(txn_id_sym, v8::Integer::New(id));
  v8::Local<v8::Value> result = { msg };
  return result;
}

v8::Handle<v8::Value> DbTxn::Abort(const v8::Arguments &args) {
  v8::HandleScope scope;

  DbTxn* txn = ObjectWrap::Unwrap<DbTxn>(args.This());

  REQ_FN_ARG(0, cb);

  EIOBaton *baton = new EIOBaton(txn);
  baton->cb = v8::Persistent<v8::Function>::New(cb);

  txn->Ref();
  eio_custom(EIO_Abort, EIO_PRI_DEFAULT, EIO_After_ReturnStatus, baton);
  ev_ref(EV_DEFAULT_UC);

  return v8::Undefined();
}

v8::Handle<v8::Value> DbTxn::Commit(const v8::Arguments &args) {
  v8::HandleScope scope;

  DbTxn* txn = node::ObjectWrap::Unwrap<DbTxn>(args.This());
  int flags = 0;

  REQ_FN_ARG(args.Length() - 1, cb);
  if(args.Length() > 1) {
	REQ_INT_ARG(0, tmp);
	flags = tmp->Value();
  }

  EIOBaton *baton = new EIOBaton(txn);
  baton->cb = v8::Persistent<v8::Function>::New(cb);
  baton->flags = flags;

  txn->Ref();
  eio_custom(EIO_Commit, EIO_PRI_DEFAULT, EIO_After_ReturnStatus, baton);
  ev_ref(EV_DEFAULT_UC);

  return v8::Undefined();
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
