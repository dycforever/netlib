#!/usr/bin/python

Import("cflags")
Import("include_dir")

env = Environment(CPPPATH = include_dir, CCFLAGS = cflags, tools=['default', 'packaging'])

debug = ARGUMENTS.get('debug', 0)
if int(debug):
        env.Append(CCFLAGS = '-g -DDEBUG_LOG')


netlib_sources = [str(fn) for fn in Glob('*.cpp') if str(fn) not in ['main.cpp']]
netlib_sources += [str(fn) for fn in Glob('http_client/*.cpp') if str(fn) not in ['main.cpp']]
netlib_sources += [str(fn) for fn in Glob('netutils/*.cpp') if str(fn) not in ['main.cpp']]

env.Library(target = 'netlib',
             source = netlib_sources,
             dirInPackage = 'lib64')


