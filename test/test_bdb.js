var assert = require('assert');
var exec  = require('child_process').exec;
var fs = require('fs');
var util = require('util');
var Buffer = require('buffer').Buffer;
var BDB = require('bdb');

var uuid = function() {
	return 'xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx'.replace(/[xy]/g, function(c) {
		var r = Math.random()*16|0, v = c == 'x' ? r : (r&0x3|0x8);
		return v.toString(16);
	}).toUpperCase();
}

var testBasic = function(db, cb) {
	var key = uuid();
	var key = new Buffer(key);
	var val = new Buffer(uuid());
	db.put(key, val, function(res) {
		assert.equal(0, res.code, res.message);		
		db.get(key, function(res, data) {
			assert.equal(0, res.code, res.message);
			assert.ok(data, "no data from get");
			assert.equal(val, data.toString(encoding='utf8'), 'Data mismatch');
			db.del(key, function(res) {
				assert.equal(0, res.code, res.message);
				db.get(key, function(res, data) {
					assert.equal(BDB.DB_NOTFOUND, res.code, res.message);
					cb();
				});
			});
		});
	});	
}

// Initialize
var env = new BDB.DbEnv();
var db = new BDB.Db();
var env_location = "/tmp/" + uuid();
fs.mkdirSync(env_location, 0750);


var ITERATIONS = 1000;
var createCallback = function(limit, fn){
    var finishedCalls = 0;
    return function() {
        if (++finishedCalls == limit){
            fn();
        }
    };
}
var callback = createCallback(ITERATIONS, function(){
	exec("rm -fr " + env_location, function(err, stdout, stderr) {});
});


var stat = env.openSync(env_location);
assert.equal(0, stat.code, stat.message);
stat = db.openSync(env, uuid());
assert.equal(0, stat.code, stat.message);
// Make sure that multiple threads are kicking in for put/get/del
for(var i = 0; i < ITERATIONS; i++) {
	testBasic(db, callback);
}
