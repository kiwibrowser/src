#-------------------------------------------------------------------------------
# elftools: elf/descriptions.py
#
# Textual descriptions of the various enums and flags of ELF
#
# Eli Bendersky (eliben@gmail.com)
# This code is in the public domain
#-------------------------------------------------------------------------------
from .enums import (
    ENUM_D_TAG, ENUM_E_VERSION, ENUM_RELOC_TYPE_i386, ENUM_RELOC_TYPE_x64,
        ENUM_RELOC_TYPE_ARM, ENUM_RELOC_TYPE_AARCH64)
from .constants import P_FLAGS, SH_FLAGS, SUNW_SYMINFO_FLAGS, VER_FLAGS
from ..common.py3compat import iteritems


def describe_ei_class(x):
    return _DESCR_EI_CLASS.get(x, _unknown)

def describe_ei_data(x):
    return _DESCR_EI_DATA.get(x, _unknown)

def describe_ei_version(x):
    s = '%d' % ENUM_E_VERSION[x]
    if x == 'EV_CURRENT':
        s += ' (current)'
    return s

def describe_ei_osabi(x):
    return _DESCR_EI_OSABI.get(x, _unknown)

def describe_e_type(x):
    return _DESCR_E_TYPE.get(x, _unknown)

def describe_e_machine(x):
    return _DESCR_E_MACHINE.get(x, _unknown)

def describe_e_version_numeric(x):
    return '0x%x' % ENUM_E_VERSION[x]

def describe_p_type(x):
    return _DESCR_P_TYPE.get(x, _unknown)

def describe_p_flags(x):
    s = ''
    for flag in (P_FLAGS.PF_R, P_FLAGS.PF_W, P_FLAGS.PF_X):
        s += _DESCR_P_FLAGS[flag] if (x & flag) else ' '
    return s

def describe_sh_type(x):
    return _DESCR_SH_TYPE.get(x, _unknown)

def describe_sh_flags(x):
    s = ''
    for flag in (
            SH_FLAGS.SHF_WRITE, SH_FLAGS.SHF_ALLOC, SH_FLAGS.SHF_EXECINSTR,
            SH_FLAGS.SHF_MERGE, SH_FLAGS.SHF_STRINGS, SH_FLAGS.SHF_INFO_LINK,
            SH_FLAGS.SHF_LINK_ORDER, SH_FLAGS.SHF_OS_NONCONFORMING,
            SH_FLAGS.SHF_GROUP, SH_FLAGS.SHF_TLS, SH_FLAGS.SHF_EXCLUDE):
        s += _DESCR_SH_FLAGS[flag] if (x & flag) else ''
    return s

def describe_symbol_type(x):
    return _DESCR_ST_INFO_TYPE.get(x, _unknown)

def describe_symbol_bind(x):
    return _DESCR_ST_INFO_BIND.get(x, _unknown)

def describe_symbol_visibility(x):
    return _DESCR_ST_VISIBILITY.get(x, _unknown)

def describe_symbol_shndx(x):
    return _DESCR_ST_SHNDX.get(x, '%3s' % x)

def describe_reloc_type(x, elffile):
    arch = elffile.get_machine_arch()
    if arch == 'x86':
        return _DESCR_RELOC_TYPE_i386.get(x, _unknown)
    elif arch == 'x64':
        return _DESCR_RELOC_TYPE_x64.get(x, _unknown)
    elif arch == 'ARM':
        return _DESCR_RELOC_TYPE_ARM.get(x, _unknown)
    elif arch == 'AArch64':
        return _DESCR_RELOC_TYPE_AARCH64.get(x, _unknown)
    else:
        return 'unrecognized: %-7x' % (x & 0xFFFFFFFF)

def describe_dyn_tag(x):
    return _DESCR_D_TAG.get(x, _unknown)


def describe_syminfo_flags(x):
    return ''.join(_DESCR_SYMINFO_FLAGS[flag] for flag in (
        SUNW_SYMINFO_FLAGS.SYMINFO_FLG_CAP,
        SUNW_SYMINFO_FLAGS.SYMINFO_FLG_DIRECT,
        SUNW_SYMINFO_FLAGS.SYMINFO_FLG_FILTER,
        SUNW_SYMINFO_FLAGS.SYMINFO_FLG_AUXILIARY,
        SUNW_SYMINFO_FLAGS.SYMINFO_FLG_DIRECTBIND,
        SUNW_SYMINFO_FLAGS.SYMINFO_FLG_COPY,
        SUNW_SYMINFO_FLAGS.SYMINFO_FLG_LAZYLOAD,
        SUNW_SYMINFO_FLAGS.SYMINFO_FLG_NOEXTDIRECT,
        SUNW_SYMINFO_FLAGS.SYMINFO_FLG_INTERPOSE,
        SUNW_SYMINFO_FLAGS.SYMINFO_FLG_DEFERRED) if x & flag)

def describe_symbol_boundto(x):
    return _DESCR_SYMINFO_BOUNDTO.get(x, '%3s' % x)

def describe_ver_flags(x):
    return ' | '.join(_DESCR_VER_FLAGS[flag] for flag in (
        VER_FLAGS.VER_FLG_WEAK,
        VER_FLAGS.VER_FLG_BASE,
        VER_FLAGS.VER_FLG_INFO) if x & flag)

#-------------------------------------------------------------------------------
_unknown = '<unknown>'


_DESCR_EI_CLASS = dict(
    ELFCLASSNONE='none',
    ELFCLASS32='ELF32',
    ELFCLASS64='ELF64',
)

_DESCR_EI_DATA = dict(
    ELFDATANONE='none',
    ELFDATA2LSB="2's complement, little endian",
    ELFDATA2MSB="2's complement, big endian",
)

_DESCR_EI_OSABI = dict(
    ELFOSABI_SYSV='UNIX - System V',
    ELFOSABI_HPUX='UNIX - HP-UX',
    ELFOSABI_NETBSD='UNIX - NetBSD',
    ELFOSABI_LINUX='UNIX - Linux',
    ELFOSABI_HURD='UNIX - GNU/Hurd',
    ELFOSABI_SOLARIS='UNIX - Solaris',
    ELFOSABI_AIX='UNIX - AIX',
    ELFOSABI_IRIX='UNIX - IRIX',
    ELFOSABI_FREEBSD='UNIX - FreeBSD',
    ELFOSABI_TRU64='UNIX - TRU64',
    ELFOSABI_MODESTO='Novell - Modesto',
    ELFOSABI_OPENBSD='UNIX - OpenBSD',
    ELFOSABI_OPENVMS='VMS - OpenVMS',
    ELFOSABI_NSK='HP - Non-Stop Kernel',
    ELFOSABI_AROS='AROS',
    ELFOSABI_ARM='ARM',
    ELFOSABI_STANDALONE='Standalone App',
)

_DESCR_E_TYPE = dict(
    ET_NONE='NONE (None)',
    ET_REL='REL (Relocatable file)',
    ET_EXEC='EXEC (Executable file)',
    ET_DYN='DYN (Shared object file)',
    ET_CORE='CORE (Core file)',
    PROC_SPECIFIC='Processor Specific',
)

_DESCR_E_MACHINE = dict(
    EM_NONE='None',
    EM_M32='WE32100',
    EM_SPARC='Sparc',
    EM_386='Intel 80386',
    EM_68K='MC68000',
    EM_88K='MC88000',
    EM_860='Intel 80860',
    EM_MIPS='MIPS R3000',
    EM_S370='IBM System/370',
    EM_MIPS_RS4_BE='MIPS 4000 big-endian',
    EM_IA_64='Intel IA-64',
    EM_X86_64='Advanced Micro Devices X86-64',
    EM_AVR='Atmel AVR 8-bit microcontroller',
    EM_ARM='ARM',
    EM_AARCH64='AArch64',
    EM_BLAFKIN='Analog Devices Blackfin',
    RESERVED='RESERVED',
)

_DESCR_P_TYPE = dict(
    PT_NULL='NULL',
    PT_LOAD='LOAD',
    PT_DYNAMIC='DYNAMIC',
    PT_INTERP='INTERP',
    PT_NOTE='NOTE',
    PT_SHLIB='SHLIB',
    PT_PHDR='PHDR',
    PT_GNU_EH_FRAME='GNU_EH_FRAME',
    PT_GNU_STACK='GNU_STACK',
    PT_GNU_RELRO='GNU_RELRO',
    PT_ARM_ARCHEXT='ARM_ARCHEXT',
    PT_ARM_EXIDX='ARM_EXIDX',
    PT_ARM_UNWIND='ARM_UNWIND',
    PT_AARCH64_ARCHEXT='AARCH64_ARCHEXT',
    PT_AARCH64_UNWIND='AARCH64_UNWIND',
)

_DESCR_P_FLAGS = {
    P_FLAGS.PF_X: 'E',
    P_FLAGS.PF_R: 'R',
    P_FLAGS.PF_W: 'W',
}

_DESCR_SH_TYPE = dict(
    SHT_NULL='NULL',
    SHT_PROGBITS='PROGBITS',
    SHT_SYMTAB='SYMTAB',
    SHT_STRTAB='STRTAB',
    SHT_RELA='RELA',
    SHT_HASH='HASH',
    SHT_DYNAMIC='DYNAMIC',
    SHT_NOTE='NOTE',
    SHT_NOBITS='NOBITS',
    SHT_REL='REL',
    SHT_SHLIB='SHLIB',
    SHT_DYNSYM='DYNSYM',
    SHT_INIT_ARRAY='INIT_ARRAY',
    SHT_FINI_ARRAY='FINI_ARRAY',
    SHT_PREINIT_ARRAY='PREINIT_ARRAY',
    SHT_GNU_HASH='GNU_HASH',
    SHT_GROUP='GROUP',
    SHT_SYMTAB_SHNDX='SYMTAB SECTION INDICIES',
    SHT_GNU_verdef='VERDEF',
    SHT_GNU_verneed='VERNEED',
    SHT_GNU_versym='VERSYM',
    SHT_GNU_LIBLIST='GNU_LIBLIST',
    SHT_ARM_EXIDX='ARM_EXIDX',
    SHT_ARM_PREEMPTMAP='ARM_PREEMPTMAP',
    SHT_ARM_ATTRIBUTES='ARM_ATTRIBUTES',
    SHT_ARM_DEBUGOVERLAY='ARM_DEBUGOVERLAY',
)

_DESCR_SH_FLAGS = {
    SH_FLAGS.SHF_WRITE: 'W',
    SH_FLAGS.SHF_ALLOC: 'A',
    SH_FLAGS.SHF_EXECINSTR: 'X',
    SH_FLAGS.SHF_MERGE: 'M',
    SH_FLAGS.SHF_STRINGS: 'S',
    SH_FLAGS.SHF_INFO_LINK: 'I',
    SH_FLAGS.SHF_LINK_ORDER: 'L',
    SH_FLAGS.SHF_OS_NONCONFORMING: 'O',
    SH_FLAGS.SHF_GROUP: 'G',
    SH_FLAGS.SHF_TLS: 'T',
    SH_FLAGS.SHF_EXCLUDE: 'E',
}

_DESCR_ST_INFO_TYPE = dict(
    STT_NOTYPE='NOTYPE',
    STT_OBJECT='OBJECT',
    STT_FUNC='FUNC',
    STT_SECTION='SECTION',
    STT_FILE='FILE',
    STT_COMMON='COMMON',
    STT_TLS='TLS',
    STT_NUM='NUM',
    STT_RELC='RELC',
    STT_SRELC='SRELC',
)

_DESCR_ST_INFO_BIND = dict(
    STB_LOCAL='LOCAL',
    STB_GLOBAL='GLOBAL',
    STB_WEAK='WEAK',
)

_DESCR_ST_VISIBILITY = dict(
    STV_DEFAULT='DEFAULT',
    STV_INTERNAL='INTERNAL',
    STV_HIDDEN='HIDDEN',
    STV_PROTECTED='PROTECTED',
    STV_EXPORTED='EXPORTED',
    STV_SINGLETON='SINGLETON',
    STV_ELIMINATE='ELIMINATE',
)

_DESCR_ST_SHNDX = dict(
    SHN_UNDEF='UND',
    SHN_ABS='ABS',
    SHN_COMMON='COM',
)

_DESCR_SYMINFO_FLAGS = {
    SUNW_SYMINFO_FLAGS.SYMINFO_FLG_DIRECT: 'D',
    SUNW_SYMINFO_FLAGS.SYMINFO_FLG_DIRECTBIND: 'B',
    SUNW_SYMINFO_FLAGS.SYMINFO_FLG_COPY: 'C',
    SUNW_SYMINFO_FLAGS.SYMINFO_FLG_LAZYLOAD: 'L',
    SUNW_SYMINFO_FLAGS.SYMINFO_FLG_NOEXTDIRECT: 'N',
    SUNW_SYMINFO_FLAGS.SYMINFO_FLG_AUXILIARY: 'A',
    SUNW_SYMINFO_FLAGS.SYMINFO_FLG_FILTER: 'F',
    SUNW_SYMINFO_FLAGS.SYMINFO_FLG_INTERPOSE: 'I',
    SUNW_SYMINFO_FLAGS.SYMINFO_FLG_CAP: 'S',
    SUNW_SYMINFO_FLAGS.SYMINFO_FLG_DEFERRED: 'P',
}

_DESCR_SYMINFO_BOUNDTO = dict(
    SYMINFO_BT_SELF='<self>',
    SYMINFO_BT_PARENT='<parent>',
    SYMINFO_BT_NONE='',
    SYMINFO_BT_EXTERN='<extern>',
)

_DESCR_VER_FLAGS = {
    0: '',
    VER_FLAGS.VER_FLG_BASE: 'BASE',
    VER_FLAGS.VER_FLG_WEAK: 'WEAK',
    VER_FLAGS.VER_FLG_INFO: 'INFO',
}

_DESCR_RELOC_TYPE_i386 = dict(
        (v, k) for k, v in iteritems(ENUM_RELOC_TYPE_i386))

_DESCR_RELOC_TYPE_x64 = dict(
        (v, k) for k, v in iteritems(ENUM_RELOC_TYPE_x64))

_DESCR_RELOC_TYPE_ARM = dict(
        (v, k) for k, v in iteritems(ENUM_RELOC_TYPE_ARM))

_DESCR_RELOC_TYPE_AARCH64 = dict(
        (v, k) for k, v in iteritems(ENUM_RELOC_TYPE_AARCH64))

_DESCR_D_TAG = dict(
        (v, k) for k, v in iteritems(ENUM_D_TAG))
