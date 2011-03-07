// Copyright 2011 Mark Cavage <mcavage@gmail.com> All rights reserved.
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

// Start V8 Exposed Methods

v8::Handle<v8::Value> DbTxn::Id(const v8::Arguments &args) {
  v8::HandleScope scope;

  DbTxn* txn = node::ObjectWrap::Unwrap<DbTxn>(args.This());
  unsigned int id = txn->_txn->id(txn->_txn);

  DB_RES(0, db_strerror(0), msg);
  msg->Set(txn_id_sym, v8::Integer::New(id));
  return msg;
}

v8::Handle<v8::Value> DbTxn::AbortS(const v8::Arguments &args) {
  v8::HandleScope scope;

  DbTxn* txn = node::ObjectWrap::Unwrap<DbTxn>(args.This());

  int rc = txn->_txn->abort(txn->_txn);
  DB_RES(rc, db_strerror(rc), msg);
  return msg;
}

v8::Handle<v8::Value> DbTxn::CommitS(const v8::Arguments &args) {
  v8::HandleScope scope;

  DbTxn* txn = node::ObjectWrap::Unwrap<DbTxn>(args.This());
  REQ_INT_ARG(0, flags);

  int rc = txn->_txn->commit(txn->_txn, flags);
  DB_RES(rc, db_strerror(rc), msg);
  return msg;
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

  NODE_SET_PROTOTYPE_METHOD(t, "abortSync", AbortS);
  NODE_SET_PROTOTYPE_METHOD(t, "_commitSync", CommitS);
  NODE_SET_PROTOTYPE_METHOD(t, "id", Id);

  target->Set(v8::String::NewSymbol("DbTxn"), t->GetFunction());
}
