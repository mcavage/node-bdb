// Copyright 2011 Mark Cavage <mcavage@gmail.com> All rights reserved.
var BDB = require('../build/default/bdb_bindings');
var DbTxn = BDB.DbTxn;

/**
 * DbTxn Commit wrapper
 *
 * Optional:
 * - 'flags'   Optional Flags: Default is DB_TXN_SYNC
 *
 * @param {Object} options
 * @api public
 */
DbTxn.prototype.commit = function(options, callback) {
  var flags = BDB.DB_TXN_SYNC;
  if (options) {
    if (options.flags) {
      flags = options.flags;
    }
  }
  if (!callback) {
    if (typeof(options) === 'function') {
      callback = options;
    }
  }

  return this._commit(flags, callback);
};

exports.DbTxn = DbTxn;
