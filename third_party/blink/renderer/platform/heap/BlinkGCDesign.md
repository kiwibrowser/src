# Blink GC Design

Oilpan is a garbage collection system for Blink objects.
This document explains the design of the GC.
If you're just interested in how to use Oilpan,
see [BlinkGCAPIReference](BlinkGCAPIReference.md).

[TOC]

## Overview

Oilpan is a single-threaded mark-and-sweep GC.
It doesn't (yet) implement a generational or incremental GC.

Blink has multiple threads including the main thread, HTML parser threads,
database threads and worker threads. Threads that touch Oilpan's heap need
to be attached to Oilpan. These threads can have cross-thread pointers.
Oilpan scans the object graph spanning these threads and collects
unreachable objects.

## Threading model

Oilpan runs a GC in the following steps:

Step 1. A thread decides to trigger a GC. The thread can be any thread
(it is likely to be the main thread because most allocations take place
on the main thread). The thread is called a GCing thread.

Step 2. The GCing thread waits for all other threads to enter safe points.
A safe point is a place where it's guaranteed that the thread does not
mutate the object graph of objects in Oilpan's heap. In common cases the thread
stops at the safe point but doesn't necessarily need to stop. For example,
the thread is allowed to execute a synchronous IO at the safe point as
long as it doesn't mutate the object graph. Safe points are inserted into
many places so that the thread can enter the safe point as soon as possible
when the GCing thread requests the thread to do so. For example, safe points
are inserted into V8 interruptors, at the end of event loops,
before acquiring a mutex, before starting a synchronous IO operation etc.

Step 3. Once all the threads enter the safe points, the GCing thread starts
a marking phase. The GCing thread marks all objects reachable from the root
set by calling trace() methods defined on each object. This means that the
GCing thread marks objects owned by all the threads. This doesn't cause any
threading race because all the other threads are at the safe points.

Step 4. Once the marking is complete, the GCing thread resumes executions of
all the other threads. Each thread starts a sweeping phase. Each thread is
responsible for destructing objects that the thread has allocated.
That way objects are guaranteed to get destructed on the thread that has
allocated the objects. The sweeping is done by each thread lazily.
Instead of completing the sweeping phase in one go, the thread sweeps
objects incrementally as much as it allocates. Lazy sweeping is helpful
to distribute a long pause time of the sweeping phase into small chunks.

The step 2 and 3 is the pause time of the GC.
The pause time is proportional to the number of objects marked
in the marking phase, meaning that it is proportional to the number of
live objects.

Notes:

* If the GCing thread fails at stopping all the other threads in a
certain period of time, it gives up triggering a GC. That way we avoid
introducing an unacceptably long pause time. (This will rarely happen
because each thread enters safe points very frequently.)

* It is not really nice that the GCing thread has to stop all the other threads.
For example, a worker thread has to get involved in a GC
caused by a lot of allocations happening on the main thread.
To resolve the issue, we have a plan to split the Oilpan's heap
into per-thread heaps. Once it's implemented, each thread can run
GCs independently.

## Precise GC and conservative GC

Oilpan has two kinds of GCs.

When all threads are stopped at the safe points at the end of event loops,
a precise GC is triggered. At this point it is guaranteed that
there are no on-stack pointers pointing to Oilpan's heap.
Thus Oilpan runs a precise GC. The root set of a precise GC is
persistent handles.

Otherwise, a conservative GC is triggered. In this case, the GC scans
a native stack of the threads (which are not stopped at the safe points
at the end of event loops) and push the pointers discovered via the native
stacks into the root set. (That's why you can use raw pointers on the
native stack.) The root set of a conservative GC is persistent handles
and the native stacks of the threads.

A conservative GC is more expensive than a precise GC because
the conservative GC needs to scan the native stacks.
Thus Oilpan tries its best to trigger GCs at the end of an event loop.
In particular, Oilpan tries its best to trigger GCs in idle tasks.

## Marking phase

The marking phase (the step 3 in the above description) consists of
the following steps. The marking phase is executed in a stop-the-world manner.

Step 3-1. The GCing thread marks all objects reachable from the root set
by calling trace() methods defined on each object.

Step 3-2. The GCing thread clears out all trivial WeakMembers.

To prevent a use-after-free from happening, it is very important to
make sure that Oilpan doesn't mis-trace any edge of the object graph.
This means that all pointers except on-stack pointers must be wrapped
with Oilpan's handles (i.e., Persistent<>, Member<>, WeakMember<> etc).
Raw pointers to on-heap objects have a risk of creating an edge Oilpan
cannot understand and causing a use-after-free. You should not use raw pointers
to on-heap objects (except raw pointers on native stacks) unless you're pretty
sure that the target objects are guaranteed to be kept alive in other ways.

## Sweeping phase

The sweeping phase (the step 4 in the above description) consists of
the following steps. The sweeping phase is executed by each thread in parallel.

Step 4-1. The thread clears out all non-trivial WeakMembers.
Non-trivial WeakMembers are the ones that have manual weak processing
(registered by registerWeakMembers()) and the ones embedded in HeapHashMap etc.
The reason we don't run non-trivial WeakMembers in the marking phase is that
clearing out the non-trivial WeakMembers can invoke some destructors
(e.g., if you have HeapHashMap<WeakMember<X>, OwnPtr<Y>>, Y's destructor
is invoked when the weak processing removes the key).
The destructors must run in the same thread that has allocated the objects.

Step 4-2. The thread invokes pre-finalizers.
At this point, no destructors have been invoked.
Thus the pre-finalizers are allowed to touch any other on-heap objects
(which may get destructed in this sweeping phase).

Step 4-3. The thread invokes destructors of dead objects that are marked
as eagerly-finalized. See the following notes for more details about the
eagerly-finalized objects.

Step 4-4. The thread resumes mutator's execution. (A mutator means user code.)

Step 4-5. As the mutator allocates new objects, lazy sweeping invokes
destructors of the remaining dead objects incrementally.

There is no guarantee of the order in which the destructors are invoked.
That's why destructors must not touch any other on-heap objects
(which might have already been destructed). If some destructor unavoidably
needs to touch other on-heap objects, you need to use a pre-finalizer.
The pre-finalizer is allowed to touch other on-heap objects.

The mutator is resumed before all the destructors has run.
For example, imagine a case where X is a client of Y, and Y holds
a list of clients. If you rely on X's destructor removing X from the list,
there is a risk that Y iterates the list and calls some method of X
which may touch other on-heap objects. This causes a use-after-free.
You need to make sure that X is explicitly removed from the list
before the mutator resumes its execution in a way that doesn't rely on
X's destructor.

Either way, the most important thing is that there is no guarantee of
when destructors run. You shouldn't put any assumption about
the order and the timing.
(In general, it's dangerous to do something complicated in a destructor.)

Notes (The followings are features you'll need only when you have
unusual destruction requirements):

* Weak processing runs only when the holder object of the WeakMember
outlives the pointed object. If the holder object and the pointed object die
at the same time, the weak processing doesn't run. It is wrong to write code
assuming that the weak processing always runs.

* Pre-finalizers are heavy because the thread needs to scan all pre-finalizers
at each sweeping phase to determine which pre-finalizers to be invoked
(the thread needs to invoke pre-finalizers of dead objects).
You should avoid adding pre-finalizers to frequently created objects.

* Eagerly-finalized objects are guaranteed to get destructed before the
mutator resumes its execution. This means that a destructor of
an eagerly-finalized object is allowed to touch other not-eagerly-finalized
objects whereas it's not allowed to touch other eagerly-finalized objects.
This notion is useful for some objects, but nasty.
We're planning to replace most eagerly-finalized objects with pre-finalizers.

* There is a subtle scenario where a next GC is triggered before
the thread finishes lazy sweeping. In that case, the not-yet-swept objects
are marked as dead and the next GC starts. The objects marked as dead are
swept in the sweeping phase of the next GC. This means that you cannot assume
that some two objects get destructed in the same GC cycle.

## Heap structures

Each thread has its dedicated heap so that the thread can allocate an object
without acquiring a lock. For example, an object allocated on thread 1 goes
to a different heap than an object allocated on thread 2.

In addition, each thread provides multiple arenas to group objects by their type
and thus improves locality.
For example, a Node object allocated on thread 1 goes to a different heap than
a CSSValue object allocated on thread 1. (See BlinkGC.h to get the list of
the typed arenas.)

