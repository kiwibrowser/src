/* This program is used to test that one-time-construction
 * works correctly, even in the presence of several threads.
 */

#include <new>
#include <pthread.h>
#include <stdio.h>

#define MAX_THREADS 100

class Foo {
public:
    Foo() { mValue++; }
    int getValue() { return mValue; }
private:
    static int  mValue;
};

int Foo::mValue;

static Foo*  getInstance(void)
{
    // This construct forces the static creation of _instance
    // the first time that getInstance() is called, in a thread-safe
    // way.
    static Foo  _instance;
    return &_instance;
}

static Foo*       sInstances[MAX_THREADS];
static pthread_t  sThreads[MAX_THREADS];

static void* thread_run(void* arg)
{
    int index = (int)(intptr_t)arg;
    sInstances[index] = getInstance();
    return NULL;
}

int main(void)
{
    /* Create all the threads */
    for (int nn = 0; nn < MAX_THREADS; nn++) {
        pthread_create( &sThreads[nn], NULL, thread_run, reinterpret_cast<void*>(nn) );
    }
    /* Wait for their completion */
    for (int nn = 0; nn < MAX_THREADS; nn++) {
        void* dummy;
        pthread_join( sThreads[nn], &dummy );
    }
    /* Get the instance */
    Foo*  foo = getInstance();

    if (foo == NULL) {
        fprintf(stderr, "ERROR: Foo instance is NULL!\n");
        return 1;
    }

    if (foo->getValue() != 1) {
        fprintf(stderr, "ERROR: Foo constructor called %d times (1 expected)\n",
                foo->getValue());
        return 2;
    }

    int count = 0;
    for (int nn = 0; nn < MAX_THREADS; nn++) {
        if (sInstances[nn] != foo)
            count++;
    }

    if (count != 0) {
        fprintf(stderr, "ERROR: There are %d invalid instance pointers!\n", count);
        return 3;
    }
    return 0;
}
