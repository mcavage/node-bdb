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
	    var txn = new BDB.DbTxn();
	    stat = env.txnBegin(txn);
	    assert.equal(0, stat.code, stat.message);
	    var cursor = new BDB.DbCursor();
	    stat = db.cursor(cursor, txn);
	    assert.equal(0, stat.code, stat.message);
	    cursor.get(function(stat, key, value) {
		console.log(require('util').inspect(stat));
		console.log(require('util').inspect(key.toString(encoding='utf8')));
		console.log(require('util').inspect(value.toString(encoding='utf8')));
	    });
	}
    });
}

