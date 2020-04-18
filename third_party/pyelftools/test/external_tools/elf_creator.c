/* Loosely based on the code in a Knol by Roberto Garcia Lopez:
** 
** http://knol.google.com/k/roberto-garca-lpez/creating-elf-relocatable-object-files/1ohwel4gqkcn2/3#
**
** Note: This file is released under the terms of the LGPL2 license.
*/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <libelf.h>
#include <err.h>
#include <sysexits.h>


const char* OUTFILE = "generated.o";

// Definition of the default string table section ".shstrtab"
const char defaultStrTable[] = 
{
    /* offset 00 */ '\0',  // The NULL section
    /* offset 01 */ '.', 's', 'h', 's', 't', 'r', 't', 'a', 'b', '\0',
    /* offset 11 */ '.', 's', 't', 'r', 't', 'a', 'b', '\0',
    /* offset 19 */ '.', 's', 'y', 'm', 't', 'a', 'b', '\0',
    /* offset 27 */ '.', 'c', 'o', 'm', 'm', 'e', 'n', 't', '\0',
    /* offset 36 */ '.', 'b', 's', 's', '\0',
    /* offset 41 */ '.', 'd', 'a', 't', 'a', '\0',
    /* offset 47 */ '.', 'r', 'e', 'l', '.', 't', 'e', 'x', 't', '\0',
    /* offset 57 */ '.', 't', 'e', 'x', 't', '\0'
};

const char defaultStrTableLen = sizeof(defaultStrTable);

// Offsets of section names in the string table
const char _shstrtab_offset = 1;
const char _strtab_offset = 11;
const char _symtab_offset = 19;
const char _text_offset = 57;

// Position of sections within the object file
const char _shstrtab = 1;
const char _strtab = 2;
const char _symtab = 3;
const char _text = 4;

const char TEXT_CONTENTS[] = {0x91, 0x92, 0x93, 0x94};


//----------------------------------------------------------------------------

int main()
{
    int FileDes;
    Elf *pElf;
    Elf32_Ehdr *pEhdr;
    Elf32_Shdr *pShdr;
    Elf_Scn *pScn;
    Elf_Data *pData;

    // Create the ELF header
    if (elf_version(EV_CURRENT) == EV_NONE) // It must appear before "elf_begin()"
        errx(EX_SOFTWARE, "ELF library initialization failed: %s", elf_errmsg(-1));

    if ((FileDes = open(OUTFILE, O_CREAT | O_WRONLY | O_TRUNC, 0777)) < 0)
        errx(EX_OSERR, "open \"%s\" failed", "compiled.o");

    if ((pElf = elf_begin(FileDes, ELF_C_WRITE, NULL)) == NULL)  // 3rd argument is ignored for "ELF_C_WRITE"
        errx(EX_SOFTWARE, "elf_begin() failed: %s.", elf_errmsg(-1));

    if ((pEhdr = elf32_newehdr(pElf)) == NULL)
        errx(EX_SOFTWARE, "elf32_newehdr() failed: %s", elf_errmsg(-1));

    pEhdr->e_ident[EI_CLASS] = ELFCLASS32;  // Defined by Intel architecture
    pEhdr->e_ident[EI_DATA] = ELFDATA2LSB;  // Defined by Intel architecture
    pEhdr->e_machine = EM_386;  // Intel architecture
    pEhdr->e_type = ET_REL;   // Relocatable file (object file)
    pEhdr->e_shstrndx = _shstrtab;    // Point to the shstrtab section

    // Create the section "default section header string table (.shstrtab)"
    if ((pScn = elf_newscn(pElf)) == NULL)
        errx(EX_SOFTWARE, "elf_newdata() failed: %s.", elf_errmsg(-1));
    if ((pData = elf_newdata(pScn)) == NULL)
        errx(EX_SOFTWARE, "elf_newdata() failed: %s.", elf_errmsg(-1));

    pData->d_align = 1;
    pData->d_buf = (void *) defaultStrTable;
    pData->d_type = ELF_T_BYTE;
    pData->d_size = defaultStrTableLen;

    if ((pShdr = elf32_getshdr(pScn)) == NULL)
        errx(EX_SOFTWARE, "elf32_etshdr() failed: %s.", elf_errmsg(-1));

    pShdr->sh_name = _shstrtab_offset;  // Point to the name of the section
    pShdr->sh_type = SHT_STRTAB;
    pShdr->sh_flags = 0;

    // Create the section ".strtab"
    if ((pScn = elf_newscn(pElf)) == NULL)
        errx(EX_SOFTWARE, "elf_newdata() failed: %s.", elf_errmsg(-1));
    if ((pData = elf_newdata(pScn)) == NULL)
        errx(EX_SOFTWARE, "elf_newdata() failed: %s.", elf_errmsg(-1));

    const char strtab[] = {0, 'g', 'e', 'n', 'e', 'r', 'a', 't', 'e', 'd', '.','x', 0, '_', 's', 't', 'a', 'r', 't', 0};

    pData->d_align = 1;
    pData->d_buf = (void *) strtab;
    pData->d_type = ELF_T_BYTE;
    pData->d_size = sizeof(strtab);

    if ((pShdr = elf32_getshdr(pScn)) == NULL)
        errx(EX_SOFTWARE, "elf32_etshdr() failed: %s.", elf_errmsg(-1));

    pShdr->sh_name = _strtab_offset;
    pShdr->sh_type = SHT_STRTAB;
    pShdr->sh_flags = 0;

    // Create the section ".symtab"
    if ((pScn = elf_newscn(pElf)) == NULL)
        errx(EX_SOFTWARE, "elf_newdata() failed: %s.", elf_errmsg(-1));
    if ((pData = elf_newdata(pScn)) == NULL)
        errx(EX_SOFTWARE, "elf_newdata() failed: %s.", elf_errmsg(-1));

    Elf32_Sym x[4];

    // Definition of the undefined section (this must be the first item by the definition of TIS ELF)
    x[0].st_name = 0;
    x[0].st_value = 0;
    x[0].st_size = 0;
    x[0].st_info = 0;
    x[0].st_other = 0;
    x[0].st_shndx = SHN_UNDEF;

    // Definition of the name of the source file (this must be the second item by the definition in TIS ELF)
    x[1].st_name = 1;
    x[1].st_value = 0;
    x[1].st_size = 0;
    x[1].st_info = ELF32_ST_INFO(STB_LOCAL, STT_FILE); // This is the value that st_info must have (because of TIS ELF)
    x[1].st_other = 0;
    x[1].st_shndx = SHN_ABS;  // The section where the symbol is

    // Definition of the ".text" section as a section in the ".symtab" section
    x[2].st_name = 0;
    x[2].st_value = 0;
    x[2].st_size = 0;
    x[2].st_info = ELF32_ST_INFO(STB_LOCAL, STT_SECTION);
    x[2].st_other = 0;
    x[2].st_shndx = _text;  // The section where the symbol is

    // Definition of the "_start" symbol
    x[3].st_name = 13;  // Offset in the "strtab" section where the name start
    x[3].st_value = 0;
    x[3].st_size = 0;
    x[3].st_info = ELF32_ST_INFO(STB_GLOBAL, STT_NOTYPE);
    x[3].st_other = 0;
    x[3].st_shndx = _text;  // The section where the symbol is

    pData->d_align = 4;
    pData->d_buf = (void *) x;
    pData->d_type = ELF_T_BYTE;
    pData->d_size = sizeof(x);

    if ((pShdr = elf32_getshdr(pScn)) == NULL)
        errx(EX_SOFTWARE, "elf32_etshdr() failed: %s.", elf_errmsg(-1));

    pShdr->sh_name = _symtab_offset;  // Point to the name of the section
    pShdr->sh_type = SHT_SYMTAB;
    pShdr->sh_flags = 0;
    pShdr->sh_link = _strtab;  // point to the section .strtab (the section that contain the strings)
    pShdr->sh_info = ELF32_ST_INFO(STB_LOCAL, 3);  // the second argument is beause of TIS ELF (One greater than the symbol table index of the last local symbol (binding STB_LOCAL))

    // Create many sections named .text
    for (int i = 0; i < 70000; ++i) {
        if ((pScn = elf_newscn(pElf)) == NULL)
            errx(EX_SOFTWARE, "elf_newdata() failed: %s.", elf_errmsg(-1));
        if ((pData = elf_newdata(pScn)) == NULL)
            errx(EX_SOFTWARE, "elf_newdata() failed: %s.", elf_errmsg(-1));

        pData->d_align = 4;
        pData->d_buf = (void *)TEXT_CONTENTS;
        pData->d_type = ELF_T_BYTE;
        pData->d_size = sizeof(TEXT_CONTENTS);

        if ((pShdr = elf32_getshdr(pScn)) == NULL)
            errx(EX_SOFTWARE, "elf32_etshdr() failed: %s.", elf_errmsg(-1));

        pShdr->sh_name = _text_offset;
        pShdr->sh_type = SHT_PROGBITS;
        pShdr->sh_flags = SHF_ALLOC | SHF_EXECINSTR;
    }

    // Update the sections internally
    if (elf_update(pElf, ELF_C_NULL) < 0)
        errx(EX_SOFTWARE, "elf_update(NULL) failed: %s.", elf_errmsg(-1));
    // Write the object file
    if (elf_update(pElf, ELF_C_WRITE) < 0)
        errx(EX_SOFTWARE, "elf_update() failed: %s.", elf_errmsg(-1));
    // Close all handles
    elf_end(pElf);
    close(FileDes);
    printf("Generated file: %s\n", OUTFILE);
    
    return 0;
}

