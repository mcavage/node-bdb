This library provides node bindings for the Berkely Database (BDB).

## What's BDB

Here's a 100% plagerized answer from the BDB docs:

BDB is a general-purpose embedded database engine that is capable of providing
a wealth of data management services. It is designed from the ground up for
high-throughput applications requiring in-process, bullet-proof management of
mission-critical data. BDB can gracefully scale from managing a few bytes to
terabytes of data. For the most part, BDB is limited only by your system's
available physical resources.

You use BDB through a series of programming APIs which give you the ability to
read and write your data, manage your database(s), and perform other more
advanced activities such as managing transactions.

Because BDB is an embedded database engine, it is extremely fast. You compile
and link it into your application in the same way as you would any third-party
library. This means that BDB runs in the same process space as does your
application, allowing you to avoid the high cost of interprocess communications
incurred by stand-alone database servers.

To further improve performance, BDB offers an in-memory cache designed to
provide rapid access to your most frequently used data. Once configured, cache
usage is transparent. It requires very little attention on the part of the
application developer.

Beyond raw speed, BDB is also extremely configurable. It provides several
different ways of organizing your data in its databases. Known as access
methods, each such data organization mechanism provides different
characteristics that are appropriate for different data management profiles.
(Note that this manual focuses almost entirely on the BTree access method as
this is the access method used by the vast majority of BDB applications).

To further improve its configurability, BDB offers many different subsystems,
each of which can be used to extend BDB's capabilities. For example, many
applications require write-protection of their data so as to ensure that data
is never left in an inconsistent state for any reason (such as software bugs
or hardware failures). For those applications, a transaction subsystem can be
enabled and used to transactional-protect database writes.

## How do I install it?

    npm install bdb

(You can also install it by doing `node-waf configure build` and then
linking or copying the folder into your project's `node_modules`
directory.)

Note that this will also build a static version of BDB, and *will
place BDB utilities in your $NODE_PATH/bin*.  So, if you've already got a BDB
on your system in that path (like, oh say /usr/bin), this is going to end badly
for you. If you've got a version of BDB on your system, there are
--shared-bdb-[include/libpath] options you can pass in at configure time.  That
will skip the static library compilation, and *not* lay down any of the BDB
utilities.

## Usage/Node-specific information

So, if that crazy awesome intro got you all fired up on BDB, by now you've gone
and already looked at the [BDB documentation](http://download.oracle.com/docs/cd/E17076_02/html/toc.htm).
Assuming that you have some knowledge of the [C API](http://download.oracle.com/docs/cd/E17076_02/html/api_reference/C/frame_main.html),
the node bindings are a pretty thin JS wrapper over them.  However, there are a
few important notes to be aware of with the node environment;  since node is
written to abstract away concurrency from you, and BDB is written to assume you
have complete control over your concurrency model, there are some complications:

- Since environment/database open/close can only be done by one thread of
control, these bindings don't have an asynchronous open/close.  Best practice is
to open everything at startup time (e.g., before you do a listen()).
- Transactions can't be active in more than one thread at a time.  So, in node
best practice is to call DbEnv.txnBegin(), and then call all your DB operation's
*synchronous* calls in the transaction scope, and commit the transaction with
commitSync().  Note that your app is not serial since in this fashion all those
synchronous calls are running on an EIO thread, not the main node event thread.
(In fact, this will probably make your app faster since you won't be
context-switching into the EIO subsystem so much).

Other information:

- Everything is setup in the library to use the BDB transactional store against
the BTREE access method unless you take some other action.  You will get better
performance out of the CDS product, but you're prone to bad failure modes. So,
in general, BDB is most usable in high-concurrency applications in TDS mode, so
that's what's defaulted.
- The TDS settings are configured for optimal durability (e.g., everything is
set up for WRITE SYNC).  BDB's transaction model uses write ahead logs, and in
with synchronous writes you get optimum durability, but you're going to care
about IOPs.  You will get better performance if you relax that constraint.  But
you lose durability.  Since I'm the author, I get to pick the defaults.  I care
very much about data durability, and the applications I work tend to be higher
read than write.  So, there you go.  YMMV.
- There's not 100% parity with the BDB API (yet).  Specifically missing are:
   - Secondary indices.  This is top of my list to add.
   - BDB encryption support.  I'll get to this too.
   - The many BDB APIs supporting configuration/stats. (notice an order here?)
   - Replication: I don't really have plans to add this. It's complicated, and
   hard to make work.  In my experience, you're better off doing this in your
   application;  contrast something like slurpd in the plethora of LDAP systems
   using BDB to BDB replication with Paxos.  One is not like the other.

## API

To load the library:

    var bdb = require('bdb');

Note that most any API supported here that in BDB has 'flags' or `NULL`
parameters is wrapped up with an options `Object`.  You'll of course be taking
my defaults if you don't specify a given field.  In every case the convention
is that you can directly get at the underlying bdb api by prefacing the method
name with an `_`.  For example:
    db.getSync({key: key}); ==>
    db._getSync(undefined, key, 0);

### DbEnv

Loading:

    var env = new bdb.DbEnv();
    env.openSync({home: '/path/to/your/env'});
    env.closeSync();

What's supported:

- `openSync(options)`
- `closeSync(options)`
- `setLockDetect(policy)`
- `setLockTimeout(timeout)`
- `setMaxLocks(max)`
- `setMaxLockers(max)`
- `setMaxLockObjects(max)`
- `setShmKey(key)`
- `setTxnMax(max)`
- `setTxnTimeout(timeout)`
- `txnBegin(options, callback)`
- `txnBeginSync(options)`
- `txnCheckpoint(options, callback)`

### Db

Loading:

    var db = new bdb.Db();
    db.openSync({env: env, file: 'db_file_name'});
    db.closeSync();

What's supported:

- `openSync(options)`
- `closeSync(options)`
- `put(options, callback)`
- `get(options, callback)`
- `del(options, callback)`
- `putSync(options)`
- `getSync(options)`
- `delSync(options)`
- `cursor(options)`

### Cursor

Loading:

    var cursor = new bdb.DbCursor();
    db.cursor({cursor: cursor});

What's supported:

- `put(options, callback)`
- `get(options, callback)`
- `del(options, callback)`

NOTE:  this isn't well-tested with transactions, and I suspect there might be
dragons here under load.

### Txn

Loading:

    var txn = new bdb.DbTxn();
    env.txnBegin({txn: txn}, function(res) {});

What's supported:

- `id()`
- `abortSync()`
- `commitSync(options)`

## License

All the bindings I'm putting out as MIT, but you *really* need to be aware of
the BDB license.  BDB is put out under a dual-license model: Sleepycat and
commercial.  IANAL, but effectively the sleepycat license has a copy-left clause
in it that makes any linking to BDB require your application to be open sourced
if you redistribute it.  You can opt to license BDB under a commercial license
if those terms don't suit you.

## TODO/Roadmap

- Making sure cursors work under load
- Secondary indicies
- Testing with shared-memory segments and in-memory DBs
- Parity with config/stat APIs
- Higher-level K/V store optimized for node (why do you think I wrote these in
the first place?)

## Bugs

See <https://github.com/mcavage/node-bdb/issues>.
