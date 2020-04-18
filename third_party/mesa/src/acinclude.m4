# A few convenience macros for Mesa, mostly to keep all the platform
# specifics out of configure.ac.

# MESA_PIC_FLAGS()
#
# Find out whether to build PIC code using the option --enable-pic and
# the configure enable_static/enable_shared settings. If PIC is needed,
# figure out the necessary flags for the platform and compiler.
#
# The platform checks have been shamelessly taken from libtool and
# stripped down to just what's needed for Mesa. See _LT_COMPILER_PIC in
# /usr/share/aclocal/libtool.m4 or
# http://git.savannah.gnu.org/gitweb/?p=libtool.git;a=blob;f=libltdl/m4/libtool.m4;hb=HEAD
#
AC_DEFUN([MESA_PIC_FLAGS],
[AC_REQUIRE([AC_PROG_CC])dnl
AC_ARG_VAR([PIC_FLAGS], [compiler flags for PIC code])
AC_ARG_ENABLE([pic],
    [AS_HELP_STRING([--disable-pic],
        [compile PIC objects @<:@default=enabled for shared builds
        on supported platforms@:>@])],
    [enable_pic="$enableval"
    test "x$enable_pic" = x && enable_pic=auto],
    [enable_pic=auto])
# disable PIC by default for static builds
if test "$enable_pic" = auto && test "$enable_static" = yes; then
    enable_pic=no
fi
# if PIC hasn't been explicitly disabled, try to figure out the flags
if test "$enable_pic" != no; then
    AC_MSG_CHECKING([for $CC option to produce PIC])
    # allow the user's flags to override
    if test "x$PIC_FLAGS" = x; then
        # see if we're using GCC
        if test "x$GCC" = xyes; then
            case "$host_os" in
            aix*|beos*|cygwin*|irix5*|irix6*|osf3*|osf4*|osf5*)
                # PIC is the default for these OSes.
                ;;
            mingw*|os2*|pw32*)
                # This hack is so that the source file can tell whether
                # it is being built for inclusion in a dll (and should
                # export symbols for example).
                PIC_FLAGS="-DDLL_EXPORT"
                ;;
            darwin*|rhapsody*)
                # PIC is the default on this platform
                # Common symbols not allowed in MH_DYLIB files
                PIC_FLAGS="-fno-common"
                ;;
            hpux*)
                # PIC is the default for IA64 HP-UX and 64-bit HP-UX,
                # but not for PA HP-UX.
                case $host_cpu in
                hppa*64*|ia64*)
                    ;;
                *)
                    PIC_FLAGS="-fPIC"
                    ;;
                esac
                ;;
            *)
                # Everyone else on GCC uses -fPIC
                PIC_FLAGS="-fPIC"
                ;;
            esac
        else # !GCC
            case "$host_os" in
            hpux9*|hpux10*|hpux11*)
                # PIC is the default for IA64 HP-UX and 64-bit HP-UX,
                # but not for PA HP-UX.
                case "$host_cpu" in
                hppa*64*|ia64*)
                    # +Z the default
                    ;;
                *)
                    PIC_FLAGS="+Z"
                    ;;
                esac
                ;;
            linux*|k*bsd*-gnu)
                case `basename "$CC"` in
                icc*|ecc*|ifort*)
                    PIC_FLAGS="-KPIC"
                    ;;
                pgcc*|pgf77*|pgf90*|pgf95*)
                    # Portland Group compilers (*not* the Pentium gcc
                    # compiler, which looks to be a dead project)
                    PIC_FLAGS="-fpic"
                    ;;
                ccc*)
                    # All Alpha code is PIC.
                    ;;
                xl*)
                    # IBM XL C 8.0/Fortran 10.1 on PPC
                    PIC_FLAGS="-qpic"
                    ;;
                *)
                    case `$CC -V 2>&1 | sed 5q` in
                    *Sun\ C*|*Sun\ F*)
                        # Sun C 5.9 or Sun Fortran
                        PIC_FLAGS="-KPIC"
                        ;;
                    esac
                esac
                ;;
            solaris*)
                PIC_FLAGS="-KPIC"
                ;;
            sunos4*)
                PIC_FLAGS="-PIC"
                ;;
            esac
        fi # GCC
    fi # PIC_FLAGS
    AC_MSG_RESULT([$PIC_FLAGS])
fi
AC_SUBST([PIC_FLAGS])
])# MESA_PIC_FLAGS
