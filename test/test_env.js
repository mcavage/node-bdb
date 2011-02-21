var assert = require('assert');
var util = require('util');
var Buffer = require('buffer').Buffer;
var BDB = require('bdb');

var key = 'mark.cavage@joyent.com';
var val = 'mcavage:*:10001:10:Mark J Cavage:/home/mark:/bin/bash';

var env = new BDB.DbEnv();

var testBasic = function(db, cb) {
	var key = new Buffer(String(key));
	var val = new Buffer(String(val));
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
env.open("/tmp/foo", function(res) {
	assert.equal(0, res.code, res.message);
	var db = new BDB.Db();
	db.open(env, "test", function(err) {
		assert.equal(0, res.code, res.message);
		testBasic(db, function() {
			console.log("testBasic: PASSED");
		});
	});
});
