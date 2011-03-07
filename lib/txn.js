// Copyright 2011 Mark Cavage <mcavage@gmail.com> All rights reserved.
var BDB = require('../build/default/bdb_bindings');
var DbTxn = BDB.DbTxn;

/**
 * DbTxn CommitSync wrapper
 *
 * Optional:
 * - 'flags'   Optional Flags: Default is DB_TXN_SYNC
 *
 * @param {Object} options
 * @api public
 */
DbTxn.prototype.commitSync = function(options) {
  var flags = BDB.DB_TXN_SYNC;
  if (options) {
    if (options.flags) {
      flags = options.flags;
    }
  }

  return this._commitSync(flags);
};

exports.DbTxn = DbTxn;
