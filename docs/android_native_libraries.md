# Shared Libraries on Android
This doc outlines some tricks / gotchas / features of how we ship native code in Chrome on Android.

[TOC]

## Library Packaging
 * Android J & K (ChromePublic.apk):
   * `libchrome.so` is stored compressed and extracted by Android during installation.
 * Android L & M (ChromeModernPublic.apk):
   * `libchrome.so` is stored uncompressed within the apk (with the name `crazy.libchrome.so` to avoid extraction).
   * It is loaded directly from the apk (without extracting) by `mmap()`'ing it.
 * Android N+ (MonochromePublic.apk):
   * `libmonochrome.so` is stored uncompressed (AndroidManifest.xml attribute disables extraction) and loaded directly from the apk (functionality now supported by the system linker).

## Debug Information
**What is it?**
 * Sections of an ELF that provide debugging and symbolization information (e.g. ability convert addresses to function & line numbers).

**How we use it:**
 * ELF debug information is too big to push to devices, even for local development.
 * All of our APKs include `.so` files with debug information removed via `strip`.
 * Unstripped libraries are stored at `out/Default/lib.unstripped`.
   * Many of our scripts are hardcoded to look for them there.

## Unwind Info & Frame Pointers
**What are they:**
 * Unwind info is data that describes how to unwind the stack. It is:
   * It is required to support C++ exceptions (which Chrome doesn't use).
   * It can also be used to produce stack traces.
   * It is generally stored in an ELF section called `.eh_frame` & `.eh_frame_hdr`, but arm32 stores it in `.ARM.exidx` and `.ARM.extab`.
     * You can see these sections via: `readelf -S libchrome.so`
 * "Frame Pointers" is a calling convention that ensures every function call has the return address pushed onto the stack.
   * Frame Pointers can also be used to produce stack traces (but without entries for inlined functions).

**How we use them:**
 * We disable unwind information (search for [`exclude_unwind_tables`](https://cs.chromium.org/search/?q=exclude_unwind_tables+file:%5C.gn&type=cs)).
 * For all architectures except arm64, we disable frame pointers in order to reduce binary size (search for [`enable_frame_pointers`](https://cs.chromium.org/search/?q=enable_frame_pointers+file:%5C.gn&type=cs)).
 * Crashes are unwound offline using `minidump_stackwalk`, which can create a stack trace given a snapshot of stack memory and the unstripped library (see [//docs/testing/using_breakpad_with_content_shell.md](testing/using_breakpad_with_content_shell.md))
 * To facilitate heap profiling, we ship unwind information to arm32 canary & dev channels as a separate file: `assets/unwind_cfi_32`

## JNI Native Methods Resolution
 * For ChromePublic.apk and ChromeModernPublic.apk:
   * `JNI_OnLoad()` is the only exported symbol (enforced by a linker script).
   * Native methods registered explicitly during start-up by generated code.
     * Explicit generation is required because the Android runtime uses the system's `dlsym()`, which doesn't know about Crazy-Linker-opened libraries.
 * For MonochromePublic.apk:
   * `JNI_OnLoad()` and `Java_*` symbols are exported by linker script.
   * No manual JNI registration is done. Symbols are resolved lazily by the runtime.

## Packed Relocations
 * All flavors of `lib(mono)chrome.so` enable "packed relocations", or "APS2 relocations" in order to save binary size.
   * Refer to [this source file](https://android.googlesource.com/platform/bionic/+/refs/heads/master/tools/relocation_packer/src/delta_encoder.h) for an explanation of the format.
 * To process these relocations:
   * Pre-M Android: Our custom linker must be used.
   * M+ Android: The system linker understands the format.
 * To see if relocations are packed, look for `LOOS+#` when running: `readelf -S libchrome.so`
 * Android P+ [supports an even better format](https://android.googlesource.com/platform/bionic/+/8b14256/linker/linker.cpp#2620) known as RELR.
   * We'll likely switch non-Monochrome apks over to using it once it is implemented in `lld`.

## RELRO Sharing
**What is it?**
 * RELRO refers to the ELF segment `GNU_RELRO`. It contains data that the linker marks as read-only after it applies relocations.
   * To inspect the size of the segment: `readelf --segments libchrome.so`
   * For `lib(mono)chrome.so` on arm32, it's about 2mb.
 * If two processes map this segment to the same virtual address space, then pages of memory within the segment which contain only relative relocations (99% of them) will be byte-for-byte identical.
   * Note: For `fork()`ed processes, all pages are already shared (via `fork()`'s copy-on-write semantics), so RELRO sharing does not apply to them.
 * "RELRO sharing" is when this segment is copied into shared memory and shared by multiple processes.

**How does it work?**
 * For Android < N (crazy linker):
   1. Browser Process: `libchrome.so` loaded normally.
   2. Browser Process: `GNU_RELRO` segment copied into `ashmem` (shared memory).
   3. Browser Process (low-end only): RELRO private memory pages swapped out for ashmem ones (using `munmap()` & `mmap()`).
   4. Browser Process: Load address and shared memory fd passed to renderers / gpu process.
   5. Renderer Process: Crazy linker tries to load to the given load address.
      * Loading can fail due to address space randomization causing something else to already by loaded at the address.
   6. Renderer Process: If loading to the desired address succeeds:
      * Linker puts `GNU_RELRO` into private memory and applies relocations as per normal.
      * Afterwards, memory pages are compared against the shared memory and all identical pages are swapped out for ashmem ones (using `munmap()` & `mmap()`).
 * For a more detailed description, refer to comments in [Linker.java](https://cs.chromium.org/chromium/src/base/android/java/src/org/chromium/base/library_loader/Linker.java).
 * For Android N+:
   * The OS maintains a RELRO file on disk with the contents of the GNU_RELRO segment.
   * All Android apps that contain a WebView load `libmonochrome.so` at the same virtual address and apply RELRO sharing against the memory-mapped RELRO file.
   * Chrome uses `MonochromeLibraryPreloader` to call into the same WebView library loading code.
     * When Monochrome is the WebView provider, `libmonochrome.so` is loaded with the system's cached RELRO's applied.
   * `System.loadLibrary()` is called afterwards.
     * When Monochrome is the WebView provider, this only calls JNI_OnLoad, since the library is already loaded. Otherwise, this loads the library and no RELRO sharing occurs.
 * For non-low-end Android O+ (where there's a WebView zygote):
   * For non-renderer processes, the above Android N+ logic applies.
   * For renderer processes, the OS starts all Monochrome renderer processes by `fork()`ing the WebView zygote rather than the normal application zygote.
     * In this case, RELRO sharing would be redundant since the entire process' memory is shared with the zygote with copy-on-write semantics.

## Library Prefetching
 * During start-up, we `fork()` a process that reads a byte from each page of the library's memory (or just the ordered range of the library).
   * See [//base/android/library_loader/](../base/android/library_loader/).

## Historical Tidbits
 * We used to use the system linker on M (`ModernLinker.java`).
   * This was removed due to [poor performance](https://bugs.chromium.org/p/chromium/issues/detail?id=719977).
 * We used to use `relocation_packer` to pack relocations after linking, which complicated our build system and caused many problems for our tools because it caused logical addresses to differ from physical addresses.
   * We now link with `lld`, which supports packed relocations natively and doesn't have these problems.

## See Also
 * [//docs/android_build_instructions.md#Multiple-Chrome-APK-Targets](android_build_instructions.md#Multiple-Chrome-APK-Targets)
 * [//third_party/android_crazy_linker/README.chromium](../third_party/android_crazy_linker/README.chromium)
 * [//base/android/linker/BUILD.gn](../base/android/linker/BUILD.gn)
