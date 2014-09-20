#!/usr/bin/python

import os
import getpass
user=getpass.getuser()

num_cpu=4
SetOption('num_jobs', num_cpu)
cflags="-g -std=c++0x"
libsname=[
     'netlib',
     'z',
     'pthread'
]

include_dir = [ '/home/dyc/github/netlib/',
                '/home/dyc/tools/zlib/include', 
                '/home/dyc/tools/log4c/include', 
                '/home/dyc/github/program/common'
                ]



VariantDir('build', '.')

SConscript(['./SConscript'], variant_dir='build', exports=['cflags', 'include_dir'], duplicate=0)
SConscript(['example/SConscript'], variant_dir='build/example', exports=['cflags', 'include_dir'])

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

