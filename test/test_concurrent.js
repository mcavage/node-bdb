var assert = require('assert');
var Buffer = require('buffer').Buffer;
var exec  = require('child_process').exec;
var fs = require('fs');
var BDB = require('bdb');
var helper = require('./helper');

// setup
var ITERATIONS = 100;

var env_location = "/tmp/" + helper.uuid();
fs.mkdirSync(env_location, 0750);

var env = new BDB.DbEnv();
var stat = env.setTxnMax(ITERATIONS + 1);
assert.equal(0, stat.code, stat.message);

stat = env.open(env_location);
assert.equal(0, stat.code, stat.message);

var db = new BDB.Db();
stat = db.openSync(env, helper.uuid());
assert.equal(0, stat.code, stat.message);


var run = function(callback) {
    var key = new Buffer(helper.uuid());
    var val = new Buffer(helper.uuid());
    var txn = new BDB.DbTxn();
    stat = env.txnBegin(txn);
    assert.equal(0, stat.code, stat.message);
    db.put(key, val, -1, txn, function(res) {
	assert.equal(0, res.code, res.message);
	db.get(key, -1, txn, function(res, data) {
	    assert.equal(0, res.code, res.message);
	    assert.ok(data, "no data from get");
	    assert.equal(val, data.toString(encoding='utf8'), 'Data mismatch');
	    db.del(key, -1, txn, function(res) {
		assert.equal(0, res.code, res.message);
		db.get(key, -1, txn, function(res, data) {
		    assert.equal(BDB.DB_NOTFOUND, res.code, res.message);
		    txn.commit(function(res) {
			assert.equal(0, res.code, res.message);
			callback();
		    });
		});
	    });
	});
    });
};

// Make sure that multiple threads are kicking in for put/get/del
var finished = 0;
for(var i = 0; i < ITERATIONS; i++) {
    run(function() {
	if(++finished >= ITERATIONS) {
	    console.log('test_concurrent: PASSED');
            exec("rm -fr " + env_location, function(err, stdout, stderr) {});
	}
    });
}

