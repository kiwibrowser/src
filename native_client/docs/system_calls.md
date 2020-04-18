The syscall numbers are defined in
[src/trusted/service\_runtime/include/bits/nacl\_syscalls.h]
(http://code.google.com/p/nativeclient/source/browse/trunk/src/native_client/src/trusted/service_runtime/include/bits/nacl_syscalls.h).

File descriptor operations: * close * read * write * lseek

Memory allocation: * sysbrk * mmap * munmap

*   getdents

*   exit

*   getpid

*   sched\_yield

*   sysconf

Timers: * gettimeofday * clock * nanosleep

[IMC socket](imc_sockets.md) calls: * imc\_makeboundsock * imc\_accept *
imc\_connect * imc\_sendmsg * imc\_recvmsg * imc\_mem\_obj\_create: create
shared memory segment * imc\_socketpair

Synchronisation operations: * mutex\_create * mutex\_lock * mutex\_trylock *
mutex\_unlock * cond\_create * cond\_wait * cond\_signal * cond\_broadcast *
cond\_timed\_wait\_abs * sem\_create * sem\_wait * sem\_post * sem\_get\_value

Threading operations: * thread\_create * thread\_exit * tls\_init * thread\_nice
* tls\_get

*   srpc\_get\_fd: what is this for?

## No-ops

*   null: for testing purposes only
*   ioctl: not implemented

## Debug mode syscalls

*   open
*   stat
*   chmod

Sound/graphics interfaces for standalone mode; superseded by NPAPI plugin
interfaces: * multimedia\_init * multimedia\_shutdown * video\_init *
video\_shutdown * video\_update * video\_poll\_event * audio\_init *
audio\_shutdown * audio\_stream

## Omissions

*   dup
*   dup2
