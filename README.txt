--------------
PYTHON FOR S60
--------------

Python(r) for S60 is a scripting language environment for the S60
smartphone platform, based on Python 2.2.2.


TRADEMARKS
----------

Python is a registered trademark of the Python Software Foundation.


LICENSING
---------

Copyright (c) 2005 Nokia Corporation. This is Python for S60 created
by Nokia Corporation. Files added by Nokia Corporation are licensed
under Apache License Version 2.0. The original software, including
modifications of Nokia Corporation therein, is licensed under the
applicable license(s) for Python 2.2.2, unless specifically indicated
otherwise in the relevant source code file.

See http://www.apache.org/licenses/LICENSE-2.0
and http://www.python.org/2.2.2/license.html


CHANGES TO PYTHON 2.2.2
-----------------------

See the file changes.txt for a list of changes to Python 2.2.2.


REQUIREMENTS
------------

To build PyS60, you need:

- Python 2.4
- S60 C++ SDK. Currently supported versions for building are:
  - S60 SDK 1.2 (1st Ed. FP1)
  - S60 SDK 2.0 (2nd Ed.)
  - S60 SDK 2.6 (2nd Ed. FP2)
  - S60 SDK 2.8 for CodeWarrior (2nd Ed. FP3)

Adding support for other SDK versions is probably not very
difficult. See the beginning of setup.py for the configuration
parameters required.

The location and messaging modules depend on files that aren't
distributed with all SDK versions and that we can't distribute with
the PyS60 source. 

If you don't have the required files, you can compile the code without
those modules by giving the additional parameters:

  WITH_MESSAGING_MODULE=0 WITH_LOCATION_MODULE=0

to the "setup.py configure" or "setup.py obb" commands.

To compile the code for multiple SDK's and for easy internal
distribution of the build dependencies it is suggested that you place
the missing dependencies in the following directory hierarchy:

build_dep\
  12\
    include\
      etelbgsm.h from 9200 SDK \epoc32\include directory
      etelgprs.h with the following contents:
--cut--
class RGprs : public RTelSubSessionBase
{
 public:
  enum TSmsBearer
    {
      ESmsBearerGprsOnly,
      ESmsBearerCircuitSwitchedOnly,
      ESmsBearerGprsPreferred,
      ESmsBearerCircuitSwitchedPreferred
      };
};
--cut--
For more information see (Symbian Developer FAQ-0811):
http://www3.symbian.com/faq.nsf/0/2335530DAA4C160680256C1E0056EAE1?OpenDocument     

  20\
    include\
      etelbgsm.h from 9200 SDK \epoc32\include directory
      The following files from S60 2.6 SDK \epoc32\include directory:
        smutset.h	      
        smuthdr.h	      
        smutset.inl     
        smuthdr.inl     
        gsmumsg.h	      
        gsmupdu.h	      
        etelmm.h	      
        gsmuetel.h      
        gsmuelem.h      
        gsmuetel.inl    
        gsmupdu.inl     
        emsinformationelement.h
        gsmuelem.inl   
        emsinformationelement.inl
        gsmumsg.inl    
  26\
    include\
      etelbgsm.h from 9200 SDK \epoc32\include directory
    armi\
      urel\
        gsmbas.lib from S60 2.0 SDK \epoc32\release\armi\urel directory
    wins\
      udeb\
        gsmbas.lib from S60 2.0 SDK \epoc32\release\wins\udeb directory
  28cw\
    include\
      etelbgsm.h from 9200 SDK \epoc32\include directory
    armi\
      urel\
        gsmbas.lib from S60 2.0 SDK \epoc32\release\armi\urel directory
    winscw\
      udeb\
        gsmbas.lib from S60 2.0 SDK \epoc32\release\wins\udeb directory

It is probably a good idea to make a ZIP package of this directory for
your internal use once you have gathered all the dependencies. The
include directories are used directly by the build system, while the
armi, wins and winscw directories are just suggestions for standard
places for storing the missing libraries in the build dependency ZIP
package. You will need to copy the .lib files into the library
directories of your SDK:

  - On SDK 2.6 place the wins version of the library in
  \epoc32\release\wins\udeb\ and the armi version in
  \epoc32\release\armi\urel.

  - On SDK 2.8 place the wins version of the library in
  \epoc32\release\winscw\udeb and the armi version in
  \epoc32\release\armi\urel. The original version of the library
  isn't compatible.

- The armi version of ECAM.LIB is missing from S60 SDK 2.6. Copy the
thumb version of the library from \epoc32\release\thumb\urel to
\epoc32\release\armi\urel.


COMPILING
---------

- The build system assumes that it is being run from a subst'ed drive
pointed at the root of your SDK and that EPOCROOT is \. For example if
you are using S60 SDK 2.6, you can create a substed drive T: with the
command:

  subst t: c:\symbian\8.0a\s60_2nd_fp2

- Make sure that Python 2.4 is in your PATH. 

- To configure the source for a particular SDK, run

  setup.py configure <your SDK>

"setup.py configure" will list the available SDK's. For example, to
configure for the S60 SDK 2.6, use:

  setup.py configure 26

If you have created the appropriate build_dep directory as described
above, you can use it in the build by giving the BUILD_DEPS parameter:

  setup.py configure 26 BUILD_DEPS=..\build_dep

- To compile for the device and the emulator, run:

  setup.py build 

build_device and build_emu build just for the device or the emulator.

- To recompile without dependency checking, run:
 
  setup.py target

target_device and target_emu do the same for just the device or the
emulator.

You can also compile one subsystem at a time by giving the subsystem 
name after the build or target command, e.g. 
 
 setup.py build graphics

will rebuild just the graphics module.

Note: Unless you specify a PYS60_VERSION_TAG parameter, setup.py will
automatically assign a development_build_YYYYMMDD_HHMM tag to it. The
PYS60_VERSION_TAG parameter must be explicitly specified to produce
builds that identify themselves as anything else than a development
build.


INSTALLING TO THE EMULATOR
--------------------------

To install the compiled code directly into your emulator environment, run

  setup.py install

When you start the emulator there should now be a Python icon in the
main menu.

INSTALLING TO THE DEVICE
------------------------

To package the software into a SIS package for installation your device, run

  setup.py bdist_sis

This will create a SIS package of the compiled code.

NOTE: This command packages the code as it exists on disk at the
time. It does not recompile anything automatically.


CREATING A BINARY SDK PACKAGE
-----------------------------

To create a binary SDK package of the compiled code, run:

  setup.py bdist_sdk

This will package the compiled code into a ZIP file.

NOTE: This command packages the code as it exists on disk at the
time. It does not recompile anything automatically.


ONE BUTTON BUILD
----------------

To invoke the steps configure, build, bdisk_sdk and bdist_sis in one step, run:
  
  setup.py obb <your SDK> [<other configure parameters>]



ADDING NEW MODULES
------------------

To add a new module to the distribution, you need to the do the following:

- create a directory for it under ext

- take a look at one of the existing extensions to see the required
files. The inbox module is a good example.

- add the mmp file to bld.inf.in

- add the .pyd file and possible .py files your extension requires to
pythonfors60.pkg.in and tools/sdk_files.py.



BUILDING PY2SIS
---------------

Two steps are needed:

1.	Build the templates (not necessary if e.g. CreateAmarettoAppUi has not 
    changed)
2.	Build the py2sis (not necessary if you don't need executable format)

1. Templates

The following templates:

pyapp_template.tmp
pyrsc_template.tmp

are needed by the py2sis program. These binary files are compiled from:

\src\app\

without the icons included and with UID 0x00000000. You need to generate these 
templates also for versions prior SDK 2.0.

2. Wrapping py2sis into executable form

If you need py2sis in executable form, you need to do the following:

Prerequisites:

 -	Python (2.2+),
 -	py2exe (http://py2exe.sourceforge.net/) and 
 -	The templates generated in step 1.

After this:

1.	Run in the py2sis/ directory: 

python setup_nogui.py py2exe

2.	copy templates/ folder to dist-folder/

You may also use the build_all.cmd located in src\py2sis.

DOCUMENTATION
-------------

For building the documentation and the needed prerequisites, refer to README in 
src\Doc.
