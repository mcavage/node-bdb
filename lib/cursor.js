// Copyright 2011 Mark Cavage <mcavage@gmail.com> All rights reserved.
var BDB = require('../build/default/bdb_bindings');
var DbCursor = BDB.DbCursor;

/**
 * DbCursor get wrapper
 *
 * Optional:
 * - 'flags'   Optional Flags: Default is DB_NEXT
 *
 * @param {Object} options
 * @api public
 */
DbCursor.prototype.get = function(options, callback) {
  var flags = BDB.DB_NEXT;
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
  return this._get(flags, callback);
};


/**
 * DbCursor put wrapper
 *
 * Required:
 * - 'key'     Key to write (Buffer)
 * - 'val'     Val to associate (Buffer)
 *
 * Optional:
 * - 'flags'   Optional Flags: Default is DB_KEYFIRST
 *
 * @param {Object} options
 * @api public
 */
DbCursor.prototype.put = function(options, callback) {
  var flags = BDB.DB_KEYFIRST;
  if (!options) {
    throw new Error('options required');
  }
  if (!options.key) {
    throw new Error('options.key required');
  }
  if (!options.val) {
    throw new Error('options.val required');
  }
  if (options.flags) {
    flags = options.flags;
  }

  return this._put(options.key, options.val, flags, callback);
};


/**
 * DbCursor del wrapper
 *
 * Optional:
 * - 'flags'   Optional Flags: Default is 0
 *
 * @param {Object} options
 * @api public
 */
DbCursor.prototype.del = function(options, callback) {
  var flags = 0;
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
  return this._del(flags, callback);
};

exports.DbCursor = DbCursor;
