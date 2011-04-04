// Copyright 2011 Mark Cavage <mcavage@gmail.com> All rights reserved.
#ifndef BDB_COMMON_H_
#define BDB_COMMON_H_

#include <db.h>
#include <node.h>
#include <node_buffer.h>
#include <v8.h>

extern v8::Persistent<v8::String> status_code_sym;
extern v8::Persistent<v8::String> err_message_sym;

extern v8::Persistent<v8::String> data_sym;
extern v8::Persistent<v8::String> key_sym;
extern v8::Persistent<v8::String> val_sym;

#define DB_RES(CODE, MSG, VAR)                                  \
  v8::Local<v8::Object> VAR = v8::Object::New();                \
  VAR->Set(status_code_sym, v8::Integer::New(CODE));            \
  VAR->Set(err_message_sym, v8::String::New(MSG));

#define RET_EXC(MSG)                                                    \
  return v8::ThrowException(v8::Exception::Error(v8::String::New(MSG)))

#define REQ_ARGS()                  \
  if (args.Length() == 0)           \
    RET_EXC("missing arguments");

#define REQ_FN_ARG(I, VAR)                                              \
  REQ_ARGS();                                                           \
  if (args.Length() <= (I) || !args[I]->IsFunction())                   \
    RET_EXC("argument " #I " must be a function");                      \
  v8::Local<v8::Function> VAR = v8::Local<v8::Function>::Cast(args[I]);

#define REQ_INT_ARG(I, VAR)                                      \
  REQ_ARGS();                                                    \
  if (args.Length() <= (I) || !args[I]->IsNumber())              \
    RET_EXC("argument " #I " must be an integer");               \
  v8::Local<v8::Integer> _ ## VAR(args[I]->ToInteger());         \
  int VAR = _ ## VAR->Value();

#define REQ_STR_ARG(I, VAR)                             \
  REQ_ARGS();                                           \
  if (args.Length() <= (I) || !args[I]->IsString())     \
    RET_EXC("argument " #I " must be a string");        \
  v8::String::Utf8Value VAR(args[I]->ToString());

#define REQ_OBJ_ARG(I, VAR)                             \
  REQ_ARGS();                                           \
  if (args.Length() <= (I) || !args[I]->IsObject())     \
    RET_EXC("argument " #I " must be a object");        \
  v8::Local<v8::Object> VAR(args[I]->ToObject());

#define REQ_BUF_ARG(I, VAR)                                 \
  REQ_ARGS();                                               \
  v8::Local<v8::Value> __ ## VAR = args[I];                 \
  if (!node::Buffer::HasInstance(__ ## VAR))                \
    RET_EXC("argument " #I " must be a buffer");            \
  v8::Local<v8::Object> _ ## VAR = __ ## VAR->ToObject();   \
  char *VAR = node::Buffer::Data(_ ## VAR);                 \
  size_t VAR ## _len = node::Buffer::Length(_ ## VAR);

#define TXN_BEGIN(DBOBJ)                         \
  DB_ENV *&_env = DBOBJ->_env;                   \
  DB_TXN *_txn = NULL;                           \
  int _attempts = 0;                             \
  int _rc = 0;                                   \
again:                                           \
  if (DBOBJ->_transactional) {                   \
    _rc = _env->txn_begin(_env, NULL, &_txn, 0); \
    if (_rc != 0)                                \
      goto out;                                  \
  }                                              \


#define TXN_END(DBOBJ, STATUS)                                          \
  if (DBOBJ->_transactional) {                                          \
    if (STATUS == 0) {                                                  \
      STATUS = _txn->commit(_txn, 0);                                   \
    } else if (STATUS == DB_LOCK_DEADLOCK &&                            \
               ++_attempts <= DBOBJ->_retries) {                        \
      _txn->abort(_txn);                                                \
      sched_yield();                                                    \
      goto again;                                                       \
    } else {                                                            \
      _txn->abort(_txn);                                                \
    }                                                                   \
  }                                                                     \
 out:

#define INIT_DBT(NAME, LEN)                              \
  DBT dbt_ ## NAME = {0};                                \
  memset(&dbt_ ## NAME, 0, sizeof(DBT));                 \
  dbt_ ## NAME.data = NAME;                              \
  dbt_ ## NAME.size = LEN

#define ALLOC_DBT(NAME)                                  \
  NAME = static_cast<DBT *>(calloc(1, sizeof(DBT)));     \
  NAME->flags = DB_DBT_MALLOC



#endif  // BDB_COMMON_H__
