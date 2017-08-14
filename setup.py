# Copyright (c) 2005 Nokia Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import sys
if sys.version_info<(2,4):
    print "Python 2.4 or later required."
    sys.exit(2)
import os.path
from os.path import normpath
from subprocess import *
import thread
from threading import Thread
import re
import traceback
from zipfile import ZipFile
import time

topdir=os.getcwd()
sys.path.append(os.path.join(topdir,'tools'))
import template_engine
    
###

buildconfig_defaults={'PYS60_VERSION_MAJOR': 1,
                      'PYS60_VERSION_MINOR': 3,
                      'PYS60_VERSION_MICRO': 1,
                      # The default tag for a build is based on the
                      # time the configuration script was run. To make
                      # a build that claims to be a "release" build
                      # you need to specify a PYS60_VERSION_TAG
                      # explicitly.
                      'PYS60_VERSION_TAG': time.strftime("development_build_%Y%m%d_%H%M"),
                      'PYS60_VERSION_SERIAL': 0,
                      'EMU_BUILD': 'udeb',
                      'DEVICE_PLATFORM': 'armi',
                      'DEVICE_BUILD': 'urel',
                      'SRC_DIR': topdir,
                      'EXTRA_SYSTEMINCLUDE_DIRS': [],
                      'WITH_MESSAGING_MODULE': 1,
                      'WITH_LOCATION_MODULE': 1}

buildconfig_sdks={
    '12': {'S60_VERSION': 12,
           'EMU_PLATFORM': 'wins',
           'SDK_NAME': 'S60 1st Ed. FP1 SDK',
           'SDK_MARKETING_VERSION_SHORT': '1stEd',
           'S60_REQUIRED_PLATFORM_UID': '0x101F6F88'},
    '20': {'S60_VERSION': 20,
           'EMU_PLATFORM': 'wins',
           'SDK_NAME': 'S60 2nd Ed. SDK',
           'SDK_MARKETING_VERSION_SHORT': '2ndEd',
           'S60_REQUIRED_PLATFORM_UID': '0x101F7960'},
    '26': {'S60_VERSION': 26,
           'EMU_PLATFORM': 'wins',
           'SDK_NAME': 'S60 2nd Ed. FP2 SDK',
           'SDK_MARKETING_VERSION_SHORT': '2ndEdFP2',
           'S60_REQUIRED_PLATFORM_UID': '0x10200BAB'},
    '28cw': {'S60_VERSION': 28,
             'EMU_PLATFORM': 'winscw',
             'SDK_NAME': 'S60 2nd Ed. FP3 SDK with CodeWarrior',
             'SDK_MARKETING_VERSION_SHORT': '2ndEdFP3',
             'S60_REQUIRED_PLATFORM_UID':'0x102032BD'}}

allow_missing=0

BUILDCONFIG_FILE=os.path.join(topdir,'build.cfg')

def buildconfig_exists():
    return os.path.exists(BUILDCONFIG_FILE)    

def buildconfig_load():
    global BUILDCONFIG
    if buildconfig_exists():
        BUILDCONFIG=eval(open(BUILDCONFIG_FILE,'rt').read())
    else:
        raise BuildFailedException("Source not configured.")
    
def buildconfig_save():
    open(BUILDCONFIG_FILE,'wt').write(repr(BUILDCONFIG))

def buildconfig_clean():
    if os.path.exists(BUILDCONFIG_FILE):
        delete_file(BUILDCONFIG_FILE)

###

class CommandFailedException(Exception): pass
class BuildFailedException(Exception): pass
class ConfigureError(Exception): pass

def run_shell_command(cmd,stdin='', mixed_stderr=0, verbose=0):
    stdout_buf=[]
    if mixed_stderr:
        stderr_buf=stdout_buf
    else:
        stderr_buf=[]
#    if verbose:
#        print '- %s'%cmd
    p=Popen(cmd,
            stdin=PIPE,
            stdout=PIPE,
            stderr=PIPE,
            shell=True)
    p.stdin.write(stdin)
    p.stdin.close()
    def handle_stderr():
        while 1:
            line=p.stderr.readline()
            if len(line)==0:
                break
            if verbose:
                print " ** "+line,
            stderr_buf.append(line)        
    stderr_thread=Thread(target=handle_stderr)
    stderr_thread.start()
    while 1:
        line=p.stdout.readline()
        if len(line)==0:
            break
        if verbose:
            print " -- "+line,
        stdout_buf.append(line)
    retcode=p.wait()
    stderr_thread.join()
    if retcode != 0:
        raise CommandFailedException, 'Command "%s" failed with code %s'%(cmd,retcode)
    if mixed_stderr:
        return {'stdout': ''.join(stdout_buf)}
    else:
        return {'stdout': ''.join(stdout_buf),
                'stderr': ''.join(stderr_buf)}

def scanlog(log):
    analysis=run_shell_command('perl \\epoc32\\tools\\scanlog.pl',log)
    #print analysis
    m=re.search(r'^Total\s+[0-9:]+\s+([0-9]+)\s+([0-9]+)',analysis['stdout'],re.M)
    if m:
        return {'analysis': analysis,
                'errors': int(m.group(1)),
                'warnings': int(m.group(2))}
    else:
        raise Exception, 'scanlog.pl failed.'

####
    
def run_command_and_check_log(cmd, verbose=1, ignore_errors=0):
    print 'Running "%s"'%cmd
    try:
        out=run_shell_command(cmd,mixed_stderr=1,verbose=verbose)
        if BUILDCONFIG['S60_VERSION']>12: # no scanlog on S60 1.2...
            scanlog_result=scanlog(out['stdout'])
            n_errors=scanlog_result['errors']
        else:
            n_errors=0
    except:
        if ignore_errors:
            print 'Ignoring exception "%s" raised by command "%s"'%(traceback.format_exception_only(sys.exc_info()[0],sys.exc_info()[1]),cmd)
            return
        raise
    #print "Errors: %(errors)d Warnings: %(warnings)d"%scanlog_result
    if n_errors>0:
        if ignore_errors:
            print 'Ignoring errors of command "%s"'%cmd
        else:
            raise BuildFailedException, 'Command "%s" failed:\n%s'%(cmd,out['stdout'])

def enter(dir):
    absdir=os.path.join(topdir,dir)
    print 'Entering "%s"'%absdir
    os.chdir(absdir)

def run_in(relative_dir, cmd, verbose=1, ignore_errors=0):
    enter(relative_dir)
    run_command_and_check_log(cmd,verbose=verbose,ignore_errors=ignore_errors)

###
    
def cmd_configure(params):
    global BUILDCONFIG
    BUILDCONFIG={}
    BUILDCONFIG.update(buildconfig_defaults)
    if len(params)==0:
        print '''Please specify SDK configuration:'''
        for k in sorted(buildconfig_sdks.keys()):
            print '%5s - %s'%(k,buildconfig_sdks[k]['SDK_NAME'])
        return
    if params[0] not in buildconfig_sdks:
        print 'Unsupported SDK configuration "%s"'%params[0]
        sys.exit(2)
    BUILDCONFIG.update(buildconfig_sdks[params[0]])
    BUILDCONFIG['SDK_TAG']=params[0]
    print "Configuring for %s"%BUILDCONFIG['SDK_NAME']

    for item in params[1:]:
        (name,value)=item.split('=')
        try:
            value=int(value)
        except ValueError:
            pass # not an integer
        BUILDCONFIG[name]=value

    BUILDCONFIG['PYS60_VERSION']='%(PYS60_VERSION_MAJOR)s.%(PYS60_VERSION_MINOR)s.%(PYS60_VERSION_MICRO)s %(PYS60_VERSION_TAG)s'%BUILDCONFIG

    # Check if a directory for build dependencies was given
    if BUILDCONFIG.has_key('BUILD_DEPS'):
        # Form the build dependencies include directory path. The
        # drive letter and colon are stripped from the path since the
        # Symbian build system doesn't understand drive letters in
        # paths.
        builddep_includes=os.path.abspath(os.path.join(BUILDCONFIG['BUILD_DEPS'],BUILDCONFIG['SDK_TAG'],'include'))[2:]
        if os.path.exists(builddep_includes):
            print "Adding extra include directory %s"%builddep_includes
            BUILDCONFIG['EXTRA_SYSTEMINCLUDE_DIRS'].append(builddep_includes)
    buildconfig_save()

    print "Build configuration:"
    for name,value in sorted(BUILDCONFIG.items()):
        print "  %s=%s"%(name,repr(value))
    BUILDCONFIG['ConfigureError']=ConfigureError
    for f in template_engine.templatefiles_in_tree(topdir):
        print "Processing template %s"%f
        template_engine.process_file(f,BUILDCONFIG)
        
    run_in('.','bldmake bldfiles')

def cmd_build(params):
    cmd_build_emu(params)
    cmd_build_device(params)

def cmd_build_emu(params):
    buildconfig_load()
    run_in('.','abld build %(EMU_PLATFORM)s %(EMU_BUILD)s '%BUILDCONFIG+' '.join(params))

def cmd_build_device(params):
    buildconfig_load()
    run_in('.','abld build %(DEVICE_PLATFORM)s %(DEVICE_BUILD)s '%BUILDCONFIG+' '.join(params))

def cmd_target(params):
    cmd_target_emu(params)
    cmd_target_device(params)

def cmd_target_emu(params):
    buildconfig_load()
    run_in('.','abld target %(EMU_PLATFORM)s %(EMU_BUILD)s '%BUILDCONFIG+' '.join(params))

def cmd_target_device(params):
    buildconfig_load()
    run_in('.','abld target %(DEVICE_PLATFORM)s %(DEVICE_BUILD)s '%BUILDCONFIG+' '.join(params))

def rename_file(fromfile,tofile):
    fromfile=normpath(fromfile)
    tofile=normpath(tofile)
    print "Renaming: %s -> %s"%(fromfile,tofile)
    os.rename(fromfile,tofile)

def copy_file(fromfile,tofile):
    fromfile=normpath(fromfile)
    tofile=normpath(tofile)
    if fromfile==tofile:
        print "No need to copy, source and target are the same: %s -> %s"%(fromfile,tofile)
    else:                                                                          
        print "Copying: %s -> %s"%(fromfile,tofile)
        targetdir=os.path.dirname(os.path.abspath(tofile))
        if not os.path.exists(targetdir):
            os.makedirs(targetdir)
        content=open(fromfile,'rb').read()
        open(tofile,'wb').write(content)

def delete_file(filename):
    print "Deleting: %s"%filename
    os.remove(filename)

def deltree(top):
    for root, dirs, files in os.walk(top, topdown=False):
        for name in files:
            os.remove(os.path.join(root, name))
        for name in dirs:   
            os.rmdir(os.path.join(root, name))
    os.rmdir(top)

def deltree_if_exists(top):
    top=normpath(top)
    if os.path.exists(top):
        print "Removing directory "+top
        deltree(top)

def create_zip_from_directory(zipname, topdir):
    """Creates a ZIP file from the contents of the given directory"""
    zipname=os.path.normpath(zipname)
    topdir=os.path.normpath(topdir)
    print "Creating ZIP %s from directory %s..."%(zipname,topdir)
    zip=ZipFile(zipname,'w')
    abs_topdir=os.path.abspath(topdir)
    for root, dirs, files in os.walk(topdir):
        abs_root=os.path.abspath(root)
        # remove the common part from the directory name, leaving just the relative part
        relative_path=abs_root[len(abs_topdir)+1:]
        for name in files:
            absolute_filename=os.path.join(abs_root,name)
            archive_filename=os.path.join(relative_path,name)
            print "Adding %s as %s"%(absolute_filename, archive_filename)
            zip.write(absolute_filename, archive_filename)
    zip.close()
    print "Created: %s"%zipname

def install_sdk_files_to(directory):
    print "Installing SDK files to directory %s"%directory
    # Copy files to sdk_files directory
    execfile('tools/sdk_files.py',BUILDCONFIG)    
    missing=[]
    n_copied=0
    for fromfile,tofile in BUILDCONFIG['SDK_FILES']:
        if not os.path.exists(fromfile):
            print "WARNING: file %s not found"%fromfile
            missing.append(fromfile)
            continue
        abs_tofile=os.path.normpath(os.path.join(directory,tofile))
        copy_file(fromfile,abs_tofile)
        n_copied+=1
    print "Installed SDK files to directory %s"%directory
    if missing:
        if not allow_missing:
            raise BuildFailedException('Files not found:\n  '+'\n  '.join(missing))
        else:
            print "** Warning: Following %d files were not found:\n  "%len(missing)+"\n  ".join(missing)
    return missing
        
def cmd_bdist_sdk(params):
    buildconfig_load()
    sdk_files_dir=os.path.normpath(topdir+'/install/sdk_files')
    if os.path.exists(sdk_files_dir):
        print "Removing old %s"%sdk_files_dir
        deltree(sdk_files_dir)

    missing=install_sdk_files_to(sdk_files_dir)
    
    zipname=topdir+'/install/sdk_files.zip'    
    create_zip_from_directory(zipname,sdk_files_dir)
    #sdk_full_name='PyS60_SDK_%(SDK_MARKETING_VERSION_SHORT)s-%(PYS60_VERSION_MAJOR)d.%(PYS60_VERSION_MINOR)d.%(PYS60_VERSION_MICRO)d'%BUILDCONFIG
    sdk_full_name='PythonForS60_SDK_%(SDK_MARKETING_VERSION_SHORT)s'%BUILDCONFIG

    # Clean the SDK build directory
    deltree_if_exists(topdir+'/install/sdk')

    # Create the full SDK package
    sdk_dir=os.path.normpath(topdir+'/install/sdk/'+sdk_full_name)
    copy_file(zipname,sdk_dir+"/sdk_files.zip")

    # Generate uninstaller
    f=open(sdk_dir+'/uninstall_%s.cmd'%sdk_full_name,'wt')
    zip=ZipFile(zipname,'r')
    print >>f,'''@echo Uninstalling %s'''%sdk_full_name
    for filename in zip.namelist():
        print >>f,'del '+os.path.normpath(filename)
    f.close()
    create_zip_from_directory(topdir+'/install/%s.zip'%sdk_full_name,
                              topdir+'/install/sdk')
    if missing:
        print "** warning: Your SDK package may be incomplete. The following %d files were not found:\n  "%len(missing)+"\n  ".join(missing)
    copy_file(topdir+'/install/%s.zip'%sdk_full_name,".\%s.zip"%sdk_full_name)

def cmd_uninstall(params):
    buildconfig_load()
    # Load the file list
    execfile('tools/sdk_files.py',BUILDCONFIG)
    n_removed=0
    for fromfile,tofile in BUILDCONFIG['SDK_FILES']:
        abs_tofile=os.path.normpath(os.path.join('\\',tofile))
        if os.path.exists(abs_tofile):
            delete_file(abs_tofile)
            n_removed+=1
    print "Uninstall finished, %d files removed."%n_removed

def cmd_install(params):
    buildconfig_load()
    print "Installing SDK files directly to emulator environment..."
    install_sdk_files_to('\\')
    print "Install finished."

def cmd_bdist_sis(params):
    buildconfig_load()
    if BUILDCONFIG['S60_VERSION']==12:
        run_in('.','makesis pythonfors60.pkg')
    else:
        run_in('core','makesis pythonforsymbianos.pkg')
        run_in('appmgr','makesis appmgr.pkg')
        run_in('.','makesis pythonfors60.pkg')
    final_sis='PythonForS60_%(SDK_MARKETING_VERSION_SHORT)s.SIS'%BUILDCONFIG
    if os.path.exists(final_sis):
        os.remove(final_sis)
    rename_file('pythonfors60.SIS',final_sis)

def cmd_obb(params):
    cmd_configure(params)
    cmd_build(())
    cmd_bdist_sdk(())
    cmd_bdist_sis(())

def cmd_clean(params):
    run_in('.','abld reallyclean',ignore_errors=1)
    run_in('.','bldmake clean',ignore_errors=1)
    for f in template_engine.templatefiles_in_tree(topdir):
        outfile=template_engine.outfilename_from_infilename(f)
        if os.path.exists(outfile):
            delete_file(outfile)
    deltree_if_exists('install')
    buildconfig_clean()    

def cmd_help(params):
    print '''Usage: %s <command> [<options>]
Commands:
    configure <SDK configuration> [SETTING=value ...]
        Configure the source for the given SDK. This must be done before build.
        
    build [<subsystem>] [<additional abld parameters> ...]    
        Build for device and emulator, with dependency
        checking. If subsystem is given, compile just that module.        
    build_device [<subsystem>] [<additional abld parameters> ...]
    build_emu [<subsystem>] [<additional abld parameters> ...]
        Build just for the device or the emulator.

    target [<subsystem>] [<additional abld parameters> ...]
    target_device [<subsystem>] [<additional abld parameters> ...]
    target_emu [<subsystem>] [<additional abld parameters> ...]    
        Same as build, with no dependency checking. A build must have
        been done before.
        
    bdist_sdk
        Build a binary SDK ZIP package.       
    bdist_sis
        Build a SIS package of the compiled files. 
        
    install
        Install directly to the emulator environment on your current drive.        
    uninstall
        Uninstall from the emulator environment on your current drive.

    obb <SDK configuration> [SETTING=value ...]
        "One-button build": do configure, build, bdisk_sdk and bdist_sis.

    clean
        Cleans everything configure and build did.

Examples:

    setup.py configure 26
        Configure for S60 version 2.6.
    setup.py obb 28cw
        Configure, build and package for S60 SDK 2.8.        
'''%sys.argv[0]

if len(sys.argv)<2:
    cmd_help(())
    sys.exit(2)

cmd=sys.argv[1]
funcname='cmd_'+cmd
if hasattr(sys.modules['__main__'],funcname):
    try:
        getattr(sys.modules['__main__'],funcname)(sys.argv[2:])
    except:
        traceback.print_exc()
        print "*** BUILD FAILED ***"
else:
    print "Unknown command %s"%cmd
    cmd_help(())
    sys.exit(2)
    
