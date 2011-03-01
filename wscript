## -*- Mode: python; py-indent-offset: 2; indent-tabs-mode: nil; coding: utf-8; -*-

import os
import Options, Utils
from os import unlink, symlink, chdir, popen, system
from os.path import exists, join

srcdir = '.'
blddir = 'build'
VERSION = '0.1'
REVISION = popen("git log | head -n1 | awk '{printf \"%s\", $2}'").readline()

def set_options(opt):
  opt.tool_options("compiler_cxx")
  opt.tool_options("compiler_cc")
  opt.add_option('--debug', action='store', default=False, help='Enable debugging output')

def configure(conf):
  conf.check_tool('compiler_cxx')
  conf.check_tool("compiler_cc")
  conf.check_tool('node_addon')

  conf.check_cxx(lib='db-5.1', header_name="db.h", mandatory=True)
  conf.env.append_value('CXXFLAGS', ["-g", "-D_FILE_OFFSET_BITS=64", "-D_LARGEFILE_SOURCE", "-Wall"])

def build(bld):
  obj = bld.new_task_gen('cxx', 'shlib', 'node_addon')
  obj.target = 'bdb_bindings'
  obj.lib = "db-5.1"
  obj.source = './src/bdb_object.cc ./src/bdb_bindings.cc '
  obj.source += './src/bdb_env.cc ./src/bdb_db.cc ./src/bdb_cursor.cc '
  obj.source += './src/bdb_txn.cc '
  obj.name = "node-bdb"
  obj.defines = ['NODE_BDB_REVISION="' + REVISION + '"']

  if (Options.options.debug != False) and (Options.options.debug == 'true'):
    obj.defines.append('ENABLE_DEBUG=1')

def tests(ctx):
  system('node test/test_open.js')
  system('node test/test_put.js')
  system('node test/test_get.js')
  system('node test/test_del.js')
  system('node test/test_cursor.js')
  system('node test/test_concurrent.js')

def shutdown():
  t = 'bdb_bindings.node';
  if exists('build/default/' + t) and not exists(t):
    symlink('build/default/' + t, t)

