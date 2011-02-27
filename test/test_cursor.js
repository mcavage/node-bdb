var assert = require('assert');
var Buffer = require('buffer').Buffer;
var exec  = require('child_process').exec;
var fs = require('fs');
var BDB = require('bdb');
var helper = require('./helper');

// setup
var env_location = "/tmp/" + helper.uuid();
fs.mkdirSync(env_location, 0750);

var env = new BDB.DbEnv();
stat = env.open(env_location);
assert.equal(0, stat.code, stat.message);

var db = new BDB.Db();
stat = db.openSync(env, helper.uuid());
assert.equal(0, stat.code, stat.message);

var load = function(prefix, callback) {
    var key = new Buffer(prefix + helper.uuid());
    var val = new Buffer(helper.uuid());
    db.put(key, val, function(res) {
	assert.equal(0, res.code, res.message);
	callback();
    });
}

var ITERATIONS = 100;
var finished = 0;
for(var i = 0; i < ITERATIONS; i++) {
    load("test", function() {
	if(++finished >= ITERATIONS) {
	    var cursor = new BDB.DbCursor();
	    db.cursor(cursor, function(stat) {
		assert.equal(0, stat.code, stat.message);
		console.log(require('util').inspect(cursor));
		cursor.get(BDB.DB_NEXT, function(stat, key, value) {
		    console.log(require('util').inspect(stat));
		    console.log(require('util').inspect(key));
		    console.log(require('util').inspect(value));
		});
	    });
	}
    });
}
