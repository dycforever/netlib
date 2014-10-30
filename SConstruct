#!/usr/bin/python

import os
import getpass
homedir = os.path.expanduser("~")
user=getpass.getuser()

num_cpu=4
SetOption('num_jobs', num_cpu)
cflags="-g -std=c++0x"
cflags="-g "
libsname=[
     'netlib',
     'z',
     'pthread'
]

include_dir = [ homedir + '/github/netlib/',
                homedir + '/tools/zlib/include', 
                homedir + '/tools/log4c/include', 
                homedir + '/github/program/common'
                ]

lib_path=[homedir + '/github/netlib/build',
          homedir + '/github/program/common',
          homedir + '/tools/log4c/lib',
          homedir + '/tools/zlib/lib']



VariantDir('build', '.')

SConscript(['./SConscript'], variant_dir='build', exports=['cflags', 'include_dir', 'lib_path'], duplicate=0)
SConscript(['example/SConscript'], variant_dir='build/example', exports=['cflags', 'include_dir', 'lib_path'])

# SConscript(['utils/SConscript'], variant_dir='build/utils', exports='include_dir')
# SConscript(['test/SConscript'], variant_dir='build/test', exports='include_dir')






















# installation
# env.Install("/home/dyc/intall_test/", main)
# env.Alias("install", "/home/dyc/intall_test/")

# runTests = """
# echo "start run test !!!"
# """
# 
# test = env.Command( target = "test", source = "", action = runTests)
# Depends( test, main )
# env.AlwaysBuild(test)

#ut = Builder(action = 'echo "running unit test" ')

# print "#############################################################"
# print "installed: ", FindInstalledFiles()
# env.Package( NAME           = 'httpc',
#         VERSION        = '0.0.1',
#         PACKAGEVERSION = 0,
#         PACKAGETYPE    = 'rpm',
#         LICENSE        = 'gpl',
#         SUMMARY        = 'balalalalal',
#         DESCRIPTION    = 'this should be really really long',
#         X_RPM_GROUP    = 'Application/fu', 
#         source = FindInstalledFiles()
#         )
# 

