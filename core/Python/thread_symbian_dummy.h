/*
 * Thread support on Symbian
 */
/* Copyright (c) 2005 Nokia Corporation
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
 */

/*
 * Initialization.
 */
static void
PyThread__init_thread(void)
{
}

/*
 * Thread support.
 */

extern "C" long
PyThread_start_new_thread(void (*func)(void *), void *arg)
{
  return 0;
}

extern "C" long
PyThread_get_thread_ident(void)
{
  return 123;
}

static
void do_PyThread_exit_thread(int no_cleanup)
{
  if (!initialized)
    if (no_cleanup)
      _exit(0);
    else
      exit(0);
}

extern "C" void
PyThread_exit_thread(void)
{
  do_PyThread_exit_thread(0);
}

extern "C" void
PyThread__exit_thread(void)
{
  do_PyThread_exit_thread(1);
}

#ifndef NO_EXIT_PROG
static
void do_PyThread_exit_prog(int status, int no_cleanup)
{
        dprintf(("PyThread_exit_prog(%d) called\n", status));
        if (!initialized)
                if (no_cleanup)
                        _exit(status);
                else
                        exit(status);
}

void
PyThread_exit_prog(int status)
{
        do_PyThread_exit_prog(status, 0);
}

void
PyThread__exit_prog(int status)
{
        do_PyThread_exit_prog(status, 1);
}
#endif /* NO_EXIT_PROG */

/*
 * Lock support.
 */

extern "C" PyThread_type_lock
PyThread_allocate_lock(void)
{
  if (!initialized)
    PyThread_init_thread();

  return (PyThread_type_lock)123;
}

extern "C" void
PyThread_free_lock(PyThread_type_lock lock)
{
  return;
}

extern "C" int
PyThread_acquire_lock(PyThread_type_lock lock, int waitflag)
{
  return 1;
}

extern "C" void
PyThread_release_lock(PyThread_type_lock lock)
{
  return;
}

#ifdef not_def
/*
 * Semaphore support.
 */
PyThread_type_sema
PyThread_allocate_sema(int value)
{
        dprintf(("PyThread_allocate_sema called\n"));
        if (!initialized)
                PyThread_init_thread();

        dprintf(("PyThread_allocate_sema() -> %p\n",  sema));
        return (PyThread_type_sema) sema;
}

void
PyThread_free_sema(PyThread_type_sema sema)
{
        dprintf(("PyThread_free_sema(%p) called\n",  sema));
}

int
PyThread_down_sema(PyThread_type_sema sema, int waitflag)
{
        dprintf(("PyThread_down_sema(%p, %d) called\n",  sema, waitflag));
        dprintf(("PyThread_down_sema(%p) return\n",  sema));
        return -1;
}

void
PyThread_up_sema(PyThread_type_sema sema)
{
        dprintf(("PyThread_up_sema(%p)\n",  sema));
}

#endif /* not_def */
