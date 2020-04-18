/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.system;

import android.util.MutableInt;
import android.util.MutableLong;
import java.io.FileDescriptor;
import java.io.InterruptedIOException;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.SocketAddress;
import java.net.SocketException;
import java.nio.ByteBuffer;
import libcore.io.Libcore;

/**
 * Access to low-level system functionality. Most of these are system calls. Most users will want
 * to use higher-level APIs where available, but this class provides access to the underlying
 * primitives used to implement the higher-level APIs.
 *
 * <p>The corresponding constants can be found in {@link OsConstants}.
 */
public final class Os {
  private Os() {}

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/accept.2.html">accept(2)</a>.
   */
  public static FileDescriptor accept(FileDescriptor fd, InetSocketAddress peerAddress) throws ErrnoException, SocketException { return Libcore.os.accept(fd, peerAddress); }

  /**
   * TODO Change the public API by removing the overload above and unhiding this version.
   * @hide
   */
  public static FileDescriptor accept(FileDescriptor fd, SocketAddress peerAddress) throws ErrnoException, SocketException { return Libcore.os.accept(fd, peerAddress); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/access.2.html">access(2)</a>.
   */
  public static boolean access(String path, int mode) throws ErrnoException { return Libcore.os.access(path, mode); }

  /** @hide */ public static InetAddress[] android_getaddrinfo(String node, StructAddrinfo hints, int netId) throws GaiException { return Libcore.os.android_getaddrinfo(node, hints, netId); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/bind.2.html">bind(2)</a>.
   */
  public static void bind(FileDescriptor fd, InetAddress address, int port) throws ErrnoException, SocketException { Libcore.os.bind(fd, address, port); }

  /** @hide */ public static void bind(FileDescriptor fd, SocketAddress address) throws ErrnoException, SocketException { Libcore.os.bind(fd, address); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/chmod.2.html">chmod(2)</a>.
   */
  public static void chmod(String path, int mode) throws ErrnoException { Libcore.os.chmod(path, mode); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/chown.2.html">chown(2)</a>.
   */
  public static void chown(String path, int uid, int gid) throws ErrnoException { Libcore.os.chown(path, uid, gid); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/close.2.html">close(2)</a>.
   */
  public static void close(FileDescriptor fd) throws ErrnoException { Libcore.os.close(fd); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/connect.2.html">connect(2)</a>.
   */
  public static void connect(FileDescriptor fd, InetAddress address, int port) throws ErrnoException, SocketException { Libcore.os.connect(fd, address, port); }

  /** @hide */ public static void connect(FileDescriptor fd, SocketAddress address) throws ErrnoException, SocketException { Libcore.os.connect(fd, address); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/dup.2.html">dup(2)</a>.
   */
  public static FileDescriptor dup(FileDescriptor oldFd) throws ErrnoException { return Libcore.os.dup(oldFd); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/dup2.2.html">dup2(2)</a>.
   */
  public static FileDescriptor dup2(FileDescriptor oldFd, int newFd) throws ErrnoException { return Libcore.os.dup2(oldFd, newFd); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man3/environ.3.html">environ(3)</a>.
   */
  public static String[] environ() { return Libcore.os.environ(); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/execv.2.html">execv(2)</a>.
   */
  public static void execv(String filename, String[] argv) throws ErrnoException { Libcore.os.execv(filename, argv); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/execve.2.html">execve(2)</a>.
   */
  public static void execve(String filename, String[] argv, String[] envp) throws ErrnoException { Libcore.os.execve(filename, argv, envp); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/fchmod.2.html">fchmod(2)</a>.
   */
  public static void fchmod(FileDescriptor fd, int mode) throws ErrnoException { Libcore.os.fchmod(fd, mode); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/fchown.2.html">fchown(2)</a>.
   */
  public static void fchown(FileDescriptor fd, int uid, int gid) throws ErrnoException { Libcore.os.fchown(fd, uid, gid); }

  /** @hide */ public static int fcntlFlock(FileDescriptor fd, int cmd, StructFlock arg) throws ErrnoException, InterruptedIOException { return Libcore.os.fcntlFlock(fd, cmd, arg); }
  /** @hide */ public static int fcntlInt(FileDescriptor fd, int cmd, int arg) throws ErrnoException { return Libcore.os.fcntlInt(fd, cmd, arg); }
  /** @hide */ public static int fcntlVoid(FileDescriptor fd, int cmd) throws ErrnoException { return Libcore.os.fcntlVoid(fd, cmd); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/fdatasync.2.html">fdatasync(2)</a>.
   */
  public static void fdatasync(FileDescriptor fd) throws ErrnoException { Libcore.os.fdatasync(fd); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/fstat.2.html">fstat(2)</a>.
   */
  public static StructStat fstat(FileDescriptor fd) throws ErrnoException { return Libcore.os.fstat(fd); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/fstatvfs.2.html">fstatvfs(2)</a>.
   */
  public static StructStatVfs fstatvfs(FileDescriptor fd) throws ErrnoException { return Libcore.os.fstatvfs(fd); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/fsync.2.html">fsync(2)</a>.
   */
  public static void fsync(FileDescriptor fd) throws ErrnoException { Libcore.os.fsync(fd); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/ftruncate.2.html">ftruncate(2)</a>.
   */
  public static void ftruncate(FileDescriptor fd, long length) throws ErrnoException { Libcore.os.ftruncate(fd, length); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man3/gai_strerror.3.html">gai_strerror(3)</a>.
   */
  public static String gai_strerror(int error) { return Libcore.os.gai_strerror(error); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/getegid.2.html">getegid(2)</a>.
   */
  public static int getegid() { return Libcore.os.getegid(); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/geteuid.2.html">geteuid(2)</a>.
   */
  public static int geteuid() { return Libcore.os.geteuid(); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/getgid.2.html">getgid(2)</a>.
   */
  public static int getgid() { return Libcore.os.getgid(); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man3/getenv.3.html">getenv(3)</a>.
   */
  public static String getenv(String name) { return Libcore.os.getenv(name); }

  /** @hide */ public static String getnameinfo(InetAddress address, int flags) throws GaiException { return Libcore.os.getnameinfo(address, flags); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/getpeername.2.html">getpeername(2)</a>.
   */
  public static SocketAddress getpeername(FileDescriptor fd) throws ErrnoException { return Libcore.os.getpeername(fd); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/getpgid.2.html">getpgid(2)</a>.
   */
  /** @hide */ public static int getpgid(int pid) throws ErrnoException { return Libcore.os.getpgid(pid); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/getpid.2.html">getpid(2)</a>.
   */
  public static int getpid() { return Libcore.os.getpid(); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/getppid.2.html">getppid(2)</a>.
   */
  public static int getppid() { return Libcore.os.getppid(); }

  /** @hide */ public static StructPasswd getpwnam(String name) throws ErrnoException { return Libcore.os.getpwnam(name); }

  /** @hide */ public static StructPasswd getpwuid(int uid) throws ErrnoException { return Libcore.os.getpwuid(uid); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/getsockname.2.html">getsockname(2)</a>.
   */
  public static SocketAddress getsockname(FileDescriptor fd) throws ErrnoException { return Libcore.os.getsockname(fd); }

  /** @hide */ public static int getsockoptByte(FileDescriptor fd, int level, int option) throws ErrnoException { return Libcore.os.getsockoptByte(fd, level, option); }
  /** @hide */ public static InetAddress getsockoptInAddr(FileDescriptor fd, int level, int option) throws ErrnoException { return Libcore.os.getsockoptInAddr(fd, level, option); }
  /** @hide */ public static int getsockoptInt(FileDescriptor fd, int level, int option) throws ErrnoException { return Libcore.os.getsockoptInt(fd, level, option); }
  /** @hide */ public static StructLinger getsockoptLinger(FileDescriptor fd, int level, int option) throws ErrnoException { return Libcore.os.getsockoptLinger(fd, level, option); }
  /** @hide */ public static StructTimeval getsockoptTimeval(FileDescriptor fd, int level, int option) throws ErrnoException { return Libcore.os.getsockoptTimeval(fd, level, option); }
  /** @hide */ public static StructUcred getsockoptUcred(FileDescriptor fd, int level, int option) throws ErrnoException { return Libcore.os.getsockoptUcred(fd, level, option); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/gettid.2.html">gettid(2)</a>.
   */
  public static int gettid() { return Libcore.os.gettid(); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/getuid.2.html">getuid(2)</a>.
   */
  public static int getuid() { return Libcore.os.getuid(); }

  /** @hide */ public static int getxattr(String path, String name, byte[] outValue) throws ErrnoException { return Libcore.os.getxattr(path, name, outValue); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man3/if_indextoname.3.html">if_indextoname(3)</a>.
   */
  public static String if_indextoname(int index) { return Libcore.os.if_indextoname(index); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man3/inet_pton.3.html">inet_pton(3)</a>.
   */
  public static InetAddress inet_pton(int family, String address) { return Libcore.os.inet_pton(family, address); }

  /** @hide */ public static InetAddress ioctlInetAddress(FileDescriptor fd, int cmd, String interfaceName) throws ErrnoException { return Libcore.os.ioctlInetAddress(fd, cmd, interfaceName); }
  /** @hide */ public static int ioctlInt(FileDescriptor fd, int cmd, MutableInt arg) throws ErrnoException { return Libcore.os.ioctlInt(fd, cmd, arg); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man3/isatty.3.html">isatty(3)</a>.
   */
  public static boolean isatty(FileDescriptor fd) { return Libcore.os.isatty(fd); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/kill.2.html">kill(2)</a>.
   */
  public static void kill(int pid, int signal) throws ErrnoException { Libcore.os.kill(pid, signal); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/lchown.2.html">lchown(2)</a>.
   */
  public static void lchown(String path, int uid, int gid) throws ErrnoException { Libcore.os.lchown(path, uid, gid); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/link.2.html">link(2)</a>.
   */
  public static void link(String oldPath, String newPath) throws ErrnoException { Libcore.os.link(oldPath, newPath); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/listen.2.html">listen(2)</a>.
   */
  public static void listen(FileDescriptor fd, int backlog) throws ErrnoException { Libcore.os.listen(fd, backlog); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/lseek.2.html">lseek(2)</a>.
   */
  public static long lseek(FileDescriptor fd, long offset, int whence) throws ErrnoException { return Libcore.os.lseek(fd, offset, whence); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/lstat.2.html">lstat(2)</a>.
   */
  public static StructStat lstat(String path) throws ErrnoException { return Libcore.os.lstat(path); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/mincore.2.html">mincore(2)</a>.
   */
  public static void mincore(long address, long byteCount, byte[] vector) throws ErrnoException { Libcore.os.mincore(address, byteCount, vector); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/mkdir.2.html">mkdir(2)</a>.
   */
  public static void mkdir(String path, int mode) throws ErrnoException { Libcore.os.mkdir(path, mode); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man3/mkfifo.3.html">mkfifo(3)</a>.
   */
  public static void mkfifo(String path, int mode) throws ErrnoException { Libcore.os.mkfifo(path, mode); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/mlock.2.html">mlock(2)</a>.
   */
  public static void mlock(long address, long byteCount) throws ErrnoException { Libcore.os.mlock(address, byteCount); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/mmap.2.html">mmap(2)</a>.
   */
  public static long mmap(long address, long byteCount, int prot, int flags, FileDescriptor fd, long offset) throws ErrnoException { return Libcore.os.mmap(address, byteCount, prot, flags, fd, offset); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/msync.2.html">msync(2)</a>.
   */
  public static void msync(long address, long byteCount, int flags) throws ErrnoException { Libcore.os.msync(address, byteCount, flags); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/munlock.2.html">munlock(2)</a>.
   */
  public static void munlock(long address, long byteCount) throws ErrnoException { Libcore.os.munlock(address, byteCount); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/munmap.2.html">munmap(2)</a>.
   */
  public static void munmap(long address, long byteCount) throws ErrnoException { Libcore.os.munmap(address, byteCount); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/open.2.html">open(2)</a>.
   */
  public static FileDescriptor open(String path, int flags, int mode) throws ErrnoException { return Libcore.os.open(path, flags, mode); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/pipe.2.html">pipe(2)</a>.
   */
  public static FileDescriptor[] pipe() throws ErrnoException { return Libcore.os.pipe2(0); }

  /** @hide */ public static FileDescriptor[] pipe2(int flags) throws ErrnoException { return Libcore.os.pipe2(flags); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/poll.2.html">poll(2)</a>.
   *
   * <p>Note that in Lollipop this could throw an {@code ErrnoException} with {@code EINTR}.
   * In later releases, the implementation will automatically just restart the system call with
   * an appropriately reduced timeout.
   */
  public static int poll(StructPollfd[] fds, int timeoutMs) throws ErrnoException { return Libcore.os.poll(fds, timeoutMs); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/posix_fallocate.2.html">posix_fallocate(2)</a>.
   */
  public static void posix_fallocate(FileDescriptor fd, long offset, long length) throws ErrnoException { Libcore.os.posix_fallocate(fd, offset, length); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/prctl.2.html">prctl(2)</a>.
   */
  public static int prctl(int option, long arg2, long arg3, long arg4, long arg5) throws ErrnoException { return Libcore.os.prctl(option, arg2, arg3, arg4, arg5); };

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/pread.2.html">pread(2)</a>.
   */
  public static int pread(FileDescriptor fd, ByteBuffer buffer, long offset) throws ErrnoException, InterruptedIOException { return Libcore.os.pread(fd, buffer, offset); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/pread.2.html">pread(2)</a>.
   */
  public static int pread(FileDescriptor fd, byte[] bytes, int byteOffset, int byteCount, long offset) throws ErrnoException, InterruptedIOException { return Libcore.os.pread(fd, bytes, byteOffset, byteCount, offset); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/pwrite.2.html">pwrite(2)</a>.
   */
  public static int pwrite(FileDescriptor fd, ByteBuffer buffer, long offset) throws ErrnoException, InterruptedIOException { return Libcore.os.pwrite(fd, buffer, offset); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/pwrite.2.html">pwrite(2)</a>.
   */
  public static int pwrite(FileDescriptor fd, byte[] bytes, int byteOffset, int byteCount, long offset) throws ErrnoException, InterruptedIOException { return Libcore.os.pwrite(fd, bytes, byteOffset, byteCount, offset); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/read.2.html">read(2)</a>.
   */
  public static int read(FileDescriptor fd, ByteBuffer buffer) throws ErrnoException, InterruptedIOException { return Libcore.os.read(fd, buffer); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/read.2.html">read(2)</a>.
   */
  public static int read(FileDescriptor fd, byte[] bytes, int byteOffset, int byteCount) throws ErrnoException, InterruptedIOException { return Libcore.os.read(fd, bytes, byteOffset, byteCount); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/readlink.2.html">readlink(2)</a>.
   */
  public static String readlink(String path) throws ErrnoException { return Libcore.os.readlink(path); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/readv.2.html">readv(2)</a>.
   */
  public static int readv(FileDescriptor fd, Object[] buffers, int[] offsets, int[] byteCounts) throws ErrnoException, InterruptedIOException { return Libcore.os.readv(fd, buffers, offsets, byteCounts); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/recvfrom.2.html">recvfrom(2)</a>.
   */
  public static int recvfrom(FileDescriptor fd, ByteBuffer buffer, int flags, InetSocketAddress srcAddress) throws ErrnoException, SocketException { return Libcore.os.recvfrom(fd, buffer, flags, srcAddress); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/recvfrom.2.html">recvfrom(2)</a>.
   */
  public static int recvfrom(FileDescriptor fd, byte[] bytes, int byteOffset, int byteCount, int flags, InetSocketAddress srcAddress) throws ErrnoException, SocketException { return Libcore.os.recvfrom(fd, bytes, byteOffset, byteCount, flags, srcAddress); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man3/remove.3.html">remove(3)</a>.
   */
  public static void remove(String path) throws ErrnoException { Libcore.os.remove(path); }

  /** @hide */ public static void removexattr(String path, String name) throws ErrnoException { Libcore.os.removexattr(path, name); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/rename.2.html">rename(2)</a>.
   */
  public static void rename(String oldPath, String newPath) throws ErrnoException { Libcore.os.rename(oldPath, newPath); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/sendfile.2.html">sendfile(2)</a>.
   */
  public static long sendfile(FileDescriptor outFd, FileDescriptor inFd, MutableLong inOffset, long byteCount) throws ErrnoException { return Libcore.os.sendfile(outFd, inFd, inOffset, byteCount); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/sendto.2.html">sendto(2)</a>.
   */
  public static int sendto(FileDescriptor fd, ByteBuffer buffer, int flags, InetAddress inetAddress, int port) throws ErrnoException, SocketException { return Libcore.os.sendto(fd, buffer, flags, inetAddress, port); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/sendto.2.html">sendto(2)</a>.
   */
  public static int sendto(FileDescriptor fd, byte[] bytes, int byteOffset, int byteCount, int flags, InetAddress inetAddress, int port) throws ErrnoException, SocketException { return Libcore.os.sendto(fd, bytes, byteOffset, byteCount, flags, inetAddress, port); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/sendto.2.html">sendto(2)</a>.
   */
  /** @hide */ public static int sendto(FileDescriptor fd, byte[] bytes, int byteOffset, int byteCount, int flags, SocketAddress address) throws ErrnoException, SocketException { return Libcore.os.sendto(fd, bytes, byteOffset, byteCount, flags, address); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/setegid.2.html">setegid(2)</a>.
   */
  public static void setegid(int egid) throws ErrnoException { Libcore.os.setegid(egid); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man3/setenv.3.html">setenv(3)</a>.
   */
  public static void setenv(String name, String value, boolean overwrite) throws ErrnoException { Libcore.os.setenv(name, value, overwrite); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/seteuid.2.html">seteuid(2)</a>.
   */
  public static void seteuid(int euid) throws ErrnoException { Libcore.os.seteuid(euid); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/setgid.2.html">setgid(2)</a>.
   */
  public static void setgid(int gid) throws ErrnoException { Libcore.os.setgid(gid); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/setpgid.2.html">setpgid(2)</a>.
   */
  /** @hide */ public static void setpgid(int pid, int pgid) throws ErrnoException { Libcore.os.setpgid(pid, pgid); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/setregid.2.html">setregid(2)</a>.
   */
  /** @hide */ public static void setregid(int rgid, int egid) throws ErrnoException { Libcore.os.setregid(rgid, egid); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/setreuid.2.html">setreuid(2)</a>.
   */
  /** @hide */ public static void setreuid(int ruid, int euid) throws ErrnoException { Libcore.os.setreuid(ruid, euid); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/setsid.2.html">setsid(2)</a>.
   */
  public static int setsid() throws ErrnoException { return Libcore.os.setsid(); }

  /** @hide */ public static void setsockoptByte(FileDescriptor fd, int level, int option, int value) throws ErrnoException { Libcore.os.setsockoptByte(fd, level, option, value); }
  /** @hide */ public static void setsockoptIfreq(FileDescriptor fd, int level, int option, String value) throws ErrnoException { Libcore.os.setsockoptIfreq(fd, level, option, value); }
  /** @hide */ public static void setsockoptInt(FileDescriptor fd, int level, int option, int value) throws ErrnoException { Libcore.os.setsockoptInt(fd, level, option, value); }
  /** @hide */ public static void setsockoptIpMreqn(FileDescriptor fd, int level, int option, int value) throws ErrnoException { Libcore.os.setsockoptIpMreqn(fd, level, option, value); }
  /** @hide */ public static void setsockoptGroupReq(FileDescriptor fd, int level, int option, StructGroupReq value) throws ErrnoException { Libcore.os.setsockoptGroupReq(fd, level, option, value); }
  /** @hide */ public static void setsockoptGroupSourceReq(FileDescriptor fd, int level, int option, StructGroupSourceReq value) throws ErrnoException { Libcore.os.setsockoptGroupSourceReq(fd, level, option, value); }
  /** @hide */ public static void setsockoptLinger(FileDescriptor fd, int level, int option, StructLinger value) throws ErrnoException { Libcore.os.setsockoptLinger(fd, level, option, value); }
  /** @hide */ public static void setsockoptTimeval(FileDescriptor fd, int level, int option, StructTimeval value) throws ErrnoException { Libcore.os.setsockoptTimeval(fd, level, option, value); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/setuid.2.html">setuid(2)</a>.
   */
  public static void setuid(int uid) throws ErrnoException { Libcore.os.setuid(uid); }

  /** @hide */ public static void setxattr(String path, String name, byte[] value, int flags) throws ErrnoException { Libcore.os.setxattr(path, name, value, flags); };

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/shutdown.2.html">shutdown(2)</a>.
   */
  public static void shutdown(FileDescriptor fd, int how) throws ErrnoException { Libcore.os.shutdown(fd, how); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/socket.2.html">socket(2)</a>.
   */
  public static FileDescriptor socket(int domain, int type, int protocol) throws ErrnoException { return Libcore.os.socket(domain, type, protocol); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/socketpair.2.html">socketpair(2)</a>.
   */
  public static void socketpair(int domain, int type, int protocol, FileDescriptor fd1, FileDescriptor fd2) throws ErrnoException { Libcore.os.socketpair(domain, type, protocol, fd1, fd2); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/stat.2.html">stat(2)</a>.
   */
  public static StructStat stat(String path) throws ErrnoException { return Libcore.os.stat(path); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/statvfs.2.html">statvfs(2)</a>.
   */
  public static StructStatVfs statvfs(String path) throws ErrnoException { return Libcore.os.statvfs(path); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man3/strerror.3.html">strerror(2)</a>.
   */
  public static String strerror(int errno) { return Libcore.os.strerror(errno); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man3/strsignal.3.html">strsignal(3)</a>.
   */
  public static String strsignal(int signal) { return Libcore.os.strsignal(signal); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/symlink.2.html">symlink(2)</a>.
   */
  public static void symlink(String oldPath, String newPath) throws ErrnoException { Libcore.os.symlink(oldPath, newPath); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man3/sysconf.3.html">sysconf(3)</a>.
   */
  public static long sysconf(int name) { return Libcore.os.sysconf(name); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man3/tcdrain.3.html">tcdrain(3)</a>.
   */
  public static void tcdrain(FileDescriptor fd) throws ErrnoException { Libcore.os.tcdrain(fd); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man3/tcsendbreak.3.html">tcsendbreak(3)</a>.
   */
  public static void tcsendbreak(FileDescriptor fd, int duration) throws ErrnoException { Libcore.os.tcsendbreak(fd, duration); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/umask.2.html">umask(2)</a>.
   */
  public static int umask(int mask) { return Libcore.os.umask(mask); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/uname.2.html">uname(2)</a>.
   */
  public static StructUtsname uname() { return Libcore.os.uname(); }

  /**
   * @hide See <a href="http://man7.org/linux/man-pages/man2/unlink.2.html">unlink(2)</a>.
   */
  public static void unlink(String pathname) throws ErrnoException { Libcore.os.unlink(pathname); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man3/unsetenv.3.html">unsetenv(3)</a>.
   */
  public static void unsetenv(String name) throws ErrnoException { Libcore.os.unsetenv(name); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/waitpid.2.html">waitpid(2)</a>.
   */
  public static int waitpid(int pid, MutableInt status, int options) throws ErrnoException { return Libcore.os.waitpid(pid, status, options); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/write.2.html">write(2)</a>.
   */
  public static int write(FileDescriptor fd, ByteBuffer buffer) throws ErrnoException, InterruptedIOException { return Libcore.os.write(fd, buffer); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/write.2.html">write(2)</a>.
   */
  public static int write(FileDescriptor fd, byte[] bytes, int byteOffset, int byteCount) throws ErrnoException, InterruptedIOException { return Libcore.os.write(fd, bytes, byteOffset, byteCount); }

  /**
   * See <a href="http://man7.org/linux/man-pages/man2/writev.2.html">writev(2)</a>.
   */
  public static int writev(FileDescriptor fd, Object[] buffers, int[] offsets, int[] byteCounts) throws ErrnoException, InterruptedIOException { return Libcore.os.writev(fd, buffers, offsets, byteCounts); }
}
