/* **********************************************************
 * Copyright (c) 2011 Google, Inc.  All rights reserved.
 * Copyright (c) 2009 VMware, Inc.  All rights reserved.
 * **********************************************************/

/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * * Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * 
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * 
 * * Neither the name of VMware, Inc. nor the names of its contributors may be
 *   used to endorse or promote products derived from this software without
 *   specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL VMWARE, INC. OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

/*
 * test exiting entire thread group
 */

#include <unistd.h>
#include <sys/types.h> /* for wait and mmap */
#include <sys/wait.h>  /* for wait */
#include <linux/sched.h>     /* for clone and CLONE_ flags */
#include <time.h>      /* for nanosleep */
#include <sys/mman.h>  /* for mmap */
#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <sys/syscall.h> /* for SYS_* */

#include "tools.h"  /* for nolibc_* wrappers. */

#ifdef USE_DYNAMO
#include "dynamorio.h"
#endif

/* FIXME: our toolchain headers are too old for these so hardcoding: */
/* linux/sched.h is hard to include properly */
#define CLONE_THREAD    0x00010000      /* Same thread group? */
#define CLONE_CHILD_CLEARTID 0x00200000      /* clear the TID in the child */

#define THREAD_STACK_SIZE   (32*1024)

#define NUM_THREADS 8

/* forward declarations */
static pid_t create_thread(int (*fcn)(void *), void *arg, void **stack);
static void delete_thread(pid_t pid, void *stack);
int run(void *arg);
static void *stack_alloc(int size);
static void stack_free(void *p, int size);

/* vars for child thread */
static pid_t child[NUM_THREADS];
static void *stack[NUM_THREADS];

/* these are used solely to provide deterministic output */
/* this is read by child, written by parent, tells child whether to exit */
static volatile bool child_exit[NUM_THREADS];
/* this is read by parent, written by child, tells parent whether child started */
static volatile bool child_started[NUM_THREADS];

static struct timespec sleeptime;

int main()
{
    int i;
#ifdef USE_DYNAMO
    dynamorio_app_init();
    dynamorio_app_start();
#endif

    sleeptime.tv_sec = 0;
    sleeptime.tv_nsec = 10*1000*1000; /* 10ms */

    for (i = 0; i < NUM_THREADS; i++) {
        child_started[i] = false;
        child_exit[i] = false;
        child[i] = create_thread(run, (void *)(long)i, &stack[i]);
        assert(child[i] > -1);
    }

    for (i = 0; i < NUM_THREADS; i++) {
        while (!child_started[i]) {
            /* waste some time: FIXME: should use futex */
            nanosleep(&sleeptime, NULL);
        }
    }

    /* we have the 1st child exit the whole group to test DR's synchall-at-exit */
    child_exit[0] = true;
    /* parent doesn't exit on its own */
    while (true)
        nanosleep(&sleeptime, NULL);

    fprintf(stderr, "Should never get here\n");
#ifdef USE_DYNAMO
    dynamorio_app_stop();
    dynamorio_app_exit();
#endif
}

/* Procedure executed by sideline threads.
 * XXX i#500: Cannot use libc routines (printf) in the child process.
 */
int run(void *arg)
{
    int threadnum = (int)(long) arg;
    int i = 0;
    /* for CLONE_CHILD_CLEARTID for signaling parent.  if we used raw
     * clone system call we could get kernel to do this for us. 
     */
    child[threadnum] = nolibc_syscall(SYS_gettid, 0);
    nolibc_syscall(SYS_set_tid_address, 1, &child[threadnum]);
    child_started[threadnum] = true;
    nolibc_print("Sideline thread started\n");
    while (true) {
        /* do nothing for now */
        i++;
        if (i % 25000000 == 0)
            break;
    }
    while (!child_exit[threadnum])
        nolibc_nanosleep(&sleeptime);
    nolibc_print("Sideline thread finished, exiting whole group\n");
    /* We deliberately bring down the whole group.  Note that this is
     * the default on x64 on returning for some reason which seems
     * like a bug in _clone() (xref i#94).
     */
    nolibc_syscall(SYS_exit_group, 0);
    return 0;
}

/* Create a new thread. It should be passed "fcn", a function which
 * takes two arguments, (the second one is a dummy, always 4). The
 * first argument is passed in "arg". Returns the TID of the new
 * thread */
static pid_t
create_thread(int (*fcn)(void *), void *arg, void **stack)
{
    pid_t newpid; 
    int flags;
    void *my_stack;
    
    my_stack = stack_alloc(THREAD_STACK_SIZE);
    /* need SIGCHLD so parent will get that signal when child dies,
     * else have errors doing a wait */
    flags = SIGCHLD | CLONE_THREAD | CLONE_VM |
        /* CLONE_THREAD => no signal to parent on termination; have to use
         * CLONE_CHILD_CLEARTID to get that.  Since we're using library call
         * instead of raw system call we don't have child_tidptr argument,
         * so we set the location in the child itself via set_tid_address(). */
        CLONE_CHILD_CLEARTID | 
        CLONE_FS | CLONE_FILES | CLONE_SIGHAND;
    newpid = clone(fcn, my_stack, flags, arg);
    /* this is really a tid since we passed CLONE_THREAD: child has same pid as us */
  
    if (newpid == -1) {
        fprintf(stderr, "smp.c: Error calling clone\n");
        stack_free(my_stack, THREAD_STACK_SIZE);
        return -1;
    }

    *stack = my_stack;
    return newpid;
}

static void 
delete_thread(pid_t pid, void *stack)
{
    pid_t result;
    /* do not print out pids to make diff easy */
    fprintf(stderr, "Waiting for child to exit\n");
    /* pid is really a tid, and since we used CLONE_THREAD, we cannot use
     * any wait() routine since our parent has the child not us.
     * so we rely on CLONE_CHILD_CLEARTID.  FIXME: use futex here.
     * for now being really simple.
     */
    while (child != 0)
        nanosleep(&sleeptime, NULL);
    fprintf(stderr, "Child has exited\n");
    stack_free(stack, THREAD_STACK_SIZE);
}

/* allocate stack storage on the app's heap */
static void *
stack_alloc(int size)
{
    size_t sp;
    void *q = NULL;
    void *p;

#if STACK_OVERFLOW_PROTECT
    /* allocate an extra page and mark it non-accessible to trap stack overflow */
    q = mmap(0, PAGE_SIZE, PROT_NONE, MAP_ANON|MAP_PRIVATE, -1, 0);
    assert(q);
    stack_redzone_start = (size_t) q;
#endif 

    p = mmap(q, size, PROT_READ|PROT_WRITE, MAP_ANON|MAP_PRIVATE, -1, 0);
    assert(p);
#ifdef DEBUG
    memset(p, 0xab, size);
#endif 

    /* stack grows from high to low addresses, so return a ptr to the top of the
       allocated region */
    sp = (size_t)p + size;

    return (void*) sp;
}

/* free memory-mapped stack storage */
static void
stack_free(void *p, int size)
{
    size_t sp = (size_t)p - size;

#ifdef DEBUG
    memset((void *)sp, 0xcd, size);
#endif 
    munmap((void *)sp, size);

#if STACK_OVERFLOW_PROTECT
    sp = sp - PAGE_SIZE;
    munmap((void*) sp, PAGE_SIZE);
#endif 
}
