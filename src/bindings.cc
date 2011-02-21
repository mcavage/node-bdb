#include <node.h>
#include <v8.h>

#include "bdb_db.h"
#include "bdb_env.h"
#include "common.h"

using namespace node;
using namespace v8;

v8::Persistent<v8::String> status_code_sym;
v8::Persistent<v8::String> err_message_sym;

extern "C" {

  void init(v8::Handle<v8::Object> target) {
	v8::HandleScope scope;

	status_code_sym = NODE_PSYMBOL("code");
	err_message_sym = NODE_PSYMBOL("message");
	
	DbEnv::Initialize(target);
	Db::Initialize(target);
  }

}
