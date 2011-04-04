// Copyright 2011 Mark Cavage <mcavage@gmail.com> All rights reserved.
#include "bdb_common.h"
#include "bdb_object.h"

#define ERR_MSG(RC)                                                     \
  (RC == -2 ? "ConsistencyError" : db_strerror(RC))

int DbObject::EIO_After_ReturnStatus(eio_req *req) {
  v8::HandleScope scope;
  EIOBaton *baton = static_cast<EIOBaton *>(req->data);
  ev_unref(EV_DEFAULT_UC);

  DB_RES(baton->status, ERR_MSG(baton->status), msg);
  v8::Local<v8::Value> argv[1] = { msg };

  v8::TryCatch try_catch;

  baton->cb->Call(v8::Context::GetCurrent()->Global(), 1, argv);

  if (try_catch.HasCaught()) {
    node::FatalException(try_catch);
  }
  baton->object->Unref();
  delete baton;
  return 0;
}

DbObject::DbObject() {}
DbObject::~DbObject() {}

// Start EIOBaton

EIOBaton::EIOBaton(DbObject *obj): object(obj), flags(0), status(0) {}

EIOBaton::~EIOBaton() {
  cb.Dispose();
}
