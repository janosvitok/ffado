#!/usr/bin/python
#

#
# Copyright (C) 2008 Pieter Palmers
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, version 3 of the License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

#
# Test for common FFADO problems
#

import sys

import os
import commands
import re
import logging

## logging setup
logging.basicConfig()
log = logging.getLogger('diag')

## helper routines

# kernel
def get_kernel_version():
    (exitstatus, outtext) = commands.getstatusoutput('uname -r')
    log.debug("uname -r outputs: %s" % outtext)
    return outtext

def get_kernel_rt_patched():
    print "FIXME: implement test for RT kernel"
    return False

# modules
def check_for_module_loaded(modulename):
    log.info("Checking if module '%s' is loaded... " % modulename)
    f = open('/proc/modules')
    lines = f.readlines()
    f.close()
    for l in lines:
        mod = l.split()[0]
        if mod == modulename or mod == modulename.replace('-', '_'):
            log.info(" found")
            return True
    log.info(" not found")
    return False

def check_for_module_present(modulename):
    log.info("Checking if module '%s' is present... " % modulename)
    kver = get_kernel_version()
    (exitstatus, outtext) = commands.getstatusoutput("find \"/lib/modules/%s/\" -name '%s.ko' | grep '%s'" % \
                                                     (kver, modulename, modulename) )
    log.debug("find outputs: %s" % outtext)
    if outtext == "":
        log.info(" not found")
        return False
    else:
        log.info(" found")
        return True

def check_1394oldstack_loaded():
    retval = True
    if not check_for_module_loaded('ieee1394'):
        retval = False
    if not check_for_module_loaded('ohci1394'):
        retval = False
    if not check_for_module_loaded('raw1394'):
        retval = False
    return retval

def check_1394oldstack_present():
    retval = True
    if not check_for_module_present('ieee1394'):
        retval = False
    if not check_for_module_present('ohci1394'):
        retval = False
    if not check_for_module_present('raw1394'):
        retval = False
    return retval

def check_1394newstack_loaded():
    retval = True
    if not check_for_module_loaded('firewire-core'):
        retval = False
    if not check_for_module_loaded('firewire-ohci'):
        retval = False
    return retval

def check_1394newstack_present():
    retval = True
    if not check_for_module_present('firewire-core'):
        retval = False
    if not check_for_module_present('firewire-ohci'):
        retval = False
    return retval

def check_1394oldstack_devnode_present():
    return os.path.exists('/dev/raw1394')

def check_1394oldstack_devnode_permissions():
    f = open('/dev/raw1394','w')
    if f:
        f.close()
        return True
    else:
        return False

def run_command(cmd):
    (exitstatus, outtext) = commands.getstatusoutput(cmd)
    log.debug("%s outputs: %s" % (cmd, outtext))
    return outtext

# package versions
def get_package_version(name):
    cmd = "pkg-config --modversion %s" % name
    return run_command(cmd)

def get_package_flags(name):
    cmd = "pkg-config --cflags --libs %s" % name
    return run_command(cmd)

def get_command_path(name):
    cmd = "which %s" % name
    return run_command(cmd)

def get_version_first_line(cmd):
    ver = run_command(cmd).split("\n")
    if len(ver) == 0:
        ver = ["None"]
    return ver[0]


def list_host_controllers():
    cmd = "lspci | grep 1394"
    controllers = run_command(cmd).split("\n")
    log.debug("lspci | grep 1394: %s" % controllers)
    for c in controllers:
        tmp = c.split()
        if len(tmp) > 0:
            tmp
            cmd = "lspci -vv -nn -s %s" % tmp[0]
            print run_command(cmd)
