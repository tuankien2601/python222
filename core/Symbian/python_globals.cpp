/*
* ====================================================================
*  python_globals.cpp
*  
*  Facilities for storing static writable variables to thread local
*  storage in Symbian environment.
*     
* Copyright (c) 2005 Nokia Corporation
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
* ====================================================================
*/

#include <e32std.h>
#include "python_globals.h"

extern "C" {
#ifdef USE_GLOBAL_DATA_HACK 
  static SPy_Python_globals *__python_globals=NULL;
  //static int __python_globals=0;
#endif
  DL_EXPORT(SPy_Python_globals*) SPy_get_globals()
  {
#ifdef USE_GLOBAL_DATA_HACK
    if (__python_globals) 
      return __python_globals;
#endif
    //SPy_Python_globals *globals=((((SPy_Tls*)Dll::Tls())->globals));    
    //globals->globptr=&__python_globals;    
    //if (!__python_globals)
    //  __python_globals=((((SPy_Tls*)Dll::Tls())->globals));
    //return __python_globals;
    return ((((SPy_Tls*)Dll::Tls())->globals));
  }

  DL_EXPORT(SPy_Python_thread_locals*) SPy_get_thread_locals()
  {
    return ((((SPy_Tls*)Dll::Tls())->thread_locals));
  }

  extern void _Py_None_Init();             // Objects\object.c
  extern void _Py_NotImplemented_Init();
  extern void _Py_EllipsisObject_Init();   // Objects\sliceobject.c
  extern void _Py_Zero_Init();             // Objects\intobject.c
  extern void _Py_True_Init();

#ifdef WITH_PYMALLOC
  extern int obmalloc_globals_init();      // Objects\obmalloc.c
  extern void obmalloc_globals_fini();
#endif

  extern int exceptions_globals_init();    // Python\exceptions.c
  extern void exceptions_globals_fini();

  extern int typeobject_globals_init();    // Objects\typeobject.c
  extern int typeobject_globals_fini();

  extern const grammar _PyParser_Grammar;
  extern const struct _inittab _PyImport_Inittab[];

  static void init_type_objects();

} /* extern "C" */

static int init_globals();

int SPy_tls_initialize(SPy_Python_globals* pg)
{
  SPy_Tls* ptls = new SPy_Tls;
  if (!ptls)
    return (-1);
  
  if (!(ptls->thread_locals = new SPy_Python_thread_locals)) {
    delete ptls;
    return (-1);
  }
  
  ptls->globals = pg;

  memset(ptls->thread_locals, 0, sizeof(SPy_Python_thread_locals));
  Dll::SetTls(ptls);
  
  return 0;
}

void SPy_tls_finalize(int fini_globals)
{
  SPy_Tls* ptls = (SPy_Tls*)Dll::Tls();
  delete ptls->thread_locals;
  if (fini_globals)
    delete ptls->globals;
  delete ptls;
  Dll::SetTls(0);
}

int SPy_globals_initialize(void* interpreter)
{
  SPy_Python_globals* pg = new SPy_Python_globals;
  
  if (!pg)
    return (-1);
  
  memset(pg, 0, sizeof(SPy_Python_globals));

  if (SPy_tls_initialize(pg)) {
    delete pg;
    return (-1);
  }
#ifdef USE_GLOBAL_DATA_HACK
  Dll::InitialiseData();  
  __python_globals=pg;
#endif
  pg->interpreter = interpreter;

  if (init_globals()) {
    SPy_tls_finalize(1);
    return (-1);
  }

  return 0;
}

void SPy_globals_finalize()
{
  PyObject_Del(PYTHON_GLOBALS->global_dict);
  typeobject_globals_fini();
  exceptions_globals_fini();
#ifdef WITH_PYMALLOC
  obmalloc_globals_fini();
#endif
  SPy_tls_finalize(1);
}

static int init_globals()
{
  PyImport_Inittab = (struct _inittab *)&(_PyImport_Inittab[0]);
#ifdef WITH_PYMALLOC
  // initialize object allocator
  if (obmalloc_globals_init())
    return (-1);
#endif

  init_type_objects();   // init type object copies in RAM
  
  _Py_None_Init();
  _Py_NotImplemented_Init();
  _Py_EllipsisObject_Init();

  _Py_Zero_Init();
  _Py_True_Init();
  
  if (exceptions_globals_init()) {
#ifdef WITH_PYMALLOC
    obmalloc_globals_fini();
#endif
    return (-1);
  }

  if (typeobject_globals_init()) {
#ifdef WITH_PYMALLOC
    obmalloc_globals_fini();
#endif
    exceptions_globals_fini();
    return (-1);
  }
  
  return 0;
}

extern "C" {

#define GTO_DEF(type_obj) \
extern const PyTypeObject c_##type_obj##_Type;

#define GTO_INI(type_obj) \
(pt->t_##type_obj)=c_##type_obj##_Type; \
(pt->t_##type_obj).ob_type=&(pt->t_PyType);
  
  static void init_type_objects()     // shallow copy
  {
    SPy_type_objects *pt = &(PYTHON_GLOBALS->tobj);

    GTO_DEF(PyBuffer)
    GTO_DEF(PyType)
    GTO_DEF(PyBaseObject)
    GTO_DEF(PySuper)
    GTO_DEF(PyCell)
    GTO_DEF(PyClass)
    GTO_DEF(PyInstance)
    GTO_DEF(PyMethod)
    GTO_DEF(PyCObject)
#ifndef WITHOUT_COMPLEX
    GTO_DEF(PyComplex)
#endif
    GTO_DEF(PyWrapperDescr)
    GTO_DEF(PyProperty)
    GTO_DEF(PyMethodDescr)
    GTO_DEF(PyMemberDescr)
    GTO_DEF(PyGetSetDescr)
    extern const PyTypeObject c_proxytype;
    extern const PyTypeObject c_wrappertype;
    extern const PyTypeObject c_immutable_list_type;
    extern const PyTypeObject _c_PyWeakref_RefType;
    extern const PyTypeObject _c_PyWeakref_ProxyType;
    extern const PyTypeObject _c_PyWeakref_CallableProxyType;
    extern const PyTypeObject _c_struct_sequence_template;
    extern const PyTypeObject c_gentype;
    GTO_DEF(PyDict)
    GTO_DEF(PyDictIter)
    GTO_DEF(PyFile)
    GTO_DEF(PyFloat)
    GTO_DEF(PyFrame)
    GTO_DEF(PyFunction)
    GTO_DEF(PyClassMethod)
    GTO_DEF(PyStaticMethod)
    GTO_DEF(PyInt)
    GTO_DEF(PyList)
    GTO_DEF(PyLong)
    GTO_DEF(PyCFunction)
    GTO_DEF(PyModule)
    GTO_DEF(PyNone)
    GTO_DEF(PyNotImplemented)
    GTO_DEF(PyRange)
    GTO_DEF(PySlice)
    GTO_DEF(PyString)
    GTO_DEF(PyTuple)
    GTO_DEF(PyUnicode)
    GTO_DEF(PySeqIter)
    GTO_DEF(PyCallIter)
    GTO_DEF(PyEllipsis)
    GTO_DEF(PyCode)
    GTO_DEF(PySymtableEntry)
    GTO_DEF(PyTraceBack)

    GTO_INI(PyBuffer)
    GTO_INI(PyType)
    GTO_INI(PyBaseObject)
    GTO_INI(PySuper)
    GTO_INI(PyCell)
    GTO_INI(PyClass)
    GTO_INI(PyInstance)
    GTO_INI(PyMethod)
    GTO_INI(PyCObject)
#ifndef WITHOUT_COMPLEX
    GTO_INI(PyComplex)
#endif
    GTO_INI(PyWrapperDescr)
    GTO_INI(PyProperty)
    GTO_INI(PyMethodDescr)
    GTO_INI(PyMemberDescr)
    GTO_INI(PyGetSetDescr)
    (pt->t_proxytype) = c_proxytype;
    (pt->t_proxytype).ob_type = &(pt->t_PyType);
    (pt->t_wrappertype) = c_wrappertype;
    (pt->t_wrappertype).ob_type = &(pt->t_PyType);

    GTO_INI(PyDict)
    GTO_INI(PyDictIter)
    GTO_INI(PyFile)
    GTO_INI(PyFloat)
    GTO_INI(PyFrame)
    GTO_INI(PyFunction)
    GTO_INI(PyClassMethod)
    GTO_INI(PyStaticMethod)
    GTO_INI(PyInt)
    GTO_INI(PyList)

    (pt->t_immutable_list_type) = c_immutable_list_type;
    (pt->t_immutable_list_type).ob_type = &(pt->t_PyType);

    GTO_INI(PyLong)
    GTO_INI(PyCFunction)
    GTO_INI(PyModule)
    GTO_INI(PyNone)
    GTO_INI(PyNotImplemented)
    GTO_INI(PyRange)
    GTO_INI(PySlice)
    GTO_INI(PyString)
    GTO_INI(PyTuple)
    GTO_INI(PyUnicode)
    GTO_INI(PySeqIter)
    GTO_INI(PyCallIter)

    (pt->t__PyWeakref_Ref) = _c_PyWeakref_RefType;
    (pt->t__PyWeakref_Ref).ob_type = &(pt->t_PyType);
    (pt->t__PyWeakref_Proxy) = _c_PyWeakref_ProxyType;
    (pt->t__PyWeakref_Proxy).ob_type = &(pt->t_PyType);
    (pt->t__PyWeakref_CallableProxy) = _c_PyWeakref_CallableProxyType;
    (pt->t__PyWeakref_CallableProxy).ob_type = &(pt->t_PyType);

    (pt->t_struct_sequence_template) = _c_struct_sequence_template;
    (pt->t_struct_sequence_template).ob_type = &(pt->t_PyType);

    GTO_INI(PyEllipsis)

    (pt->t_gentype) = c_gentype;
    (pt->t_gentype).ob_type = &(pt->t_PyType);

    GTO_INI(PyCode)
    GTO_INI(PySymtableEntry)
    GTO_INI(PyTraceBack)
  }
} /* extern "C" */
