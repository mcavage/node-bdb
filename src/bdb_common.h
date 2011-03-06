// Copyright 2011 Mark Cavage <mcavage@gmail.com> All rights reserved.
#ifndef BDB_COMMON_H_
#define BDB_COMMON_H_

#include <db.h>
#include <node.h>
#include <node_buffer.h>
#include <v8.h>

extern v8::Persistent<v8::String> status_code_sym;
extern v8::Persistent<v8::String> err_message_sym;

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

#define OPT_TXN_ARG(I, TXN)                             \
  REQ_ARGS();                                           \
  v8::Local<v8::Object> _txnObj;                        \
  if (args.Length() <= (I))                             \
    RET_EXC("Not enough arguments");                    \
  if (args[I]->IsNull() || args[I]->IsUndefined()) {    \
    TXN = NULL;                                         \
  } else {                                              \
    if (!args[I]->IsObject())                           \
      RET_EXC("argument " #I " must be a object");      \
    _txnObj = v8::Local<v8::Object>::Cast(args[I]);     \
    TXN = node::ObjectWrap::Unwrap<DbTxn>(_txnObj);     \
  }

#define REQ_BUF_ARG(I, VAR)                                 \
  REQ_ARGS();                                               \
  v8::Local<v8::Value> __ ## VAR = args[I];                 \
  if (!node::Buffer::HasInstance(__ ## VAR))                \
    RET_EXC("argument " #I " must be a buffer");            \
  v8::Local<v8::Object> _ ## VAR = __ ## VAR->ToObject();   \
  char *VAR = node::Buffer::Data(_ ## VAR);                 \
  size_t VAR ## _len = node::Buffer::Length(_ ## VAR);

#endif  // BDB_COMMON_H__
