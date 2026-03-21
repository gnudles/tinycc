/*
 *  MIL-STD-1750A linker for TCC
 *
 *  Copyright (c) 2024
 */

#ifdef TARGET_DEFS_ONLY

#define EM_TCC_TARGET EM_1750A /* TCC pseudo-magic */

/* Relocation types */
#define R_1750A_32   R_C60_32 /* Using existing COFF generic relocations */
#define R_DATA_32    R_1750A_32
#define R_DATA_PTR   R_1750A_32
#define R_JMP_SLOT   R_1750A_32
#define R_GLOB_DAT   R_1750A_32
#define R_COPY       R_1750A_32
#define R_RELATIVE   R_1750A_32

#define R_NUM        100

#define ELF_START_ADDR 0x00000000
#define ELF_PAGE_SIZE  0x1000

#define PCRELATIVE_DLLPLT 0
#define RELOCATE_DLLPLT 0

#else /* !TARGET_DEFS_ONLY */

#include "tcc.h"

ST_FUNC int code_reloc (int reloc_type)
{
    switch (reloc_type) {
        case R_C60_32:
            return 0;
    }
    return -1;
}

ST_FUNC int gotplt_entry_type (int reloc_type)
{
    switch (reloc_type) {
        case R_C60_32:
            return NO_GOTPLT_ENTRY;
    }
    return -1;
}

ST_FUNC unsigned create_plt_entry(TCCState *s1, unsigned got_offset, struct sym_attr *attr)
{
    tcc_error_noabort("1750a got not implemented");
    return 0;
}

ST_FUNC void relocate_plt(TCCState *s1)
{
}

ST_FUNC void relocate(TCCState *s1, ElfW_Rel *rel, int type, unsigned char *ptr, addr_t addr, addr_t val)
{
    switch(type) {
        case R_C60_32:
            *(int *)ptr += val;
            break;
        default:
            fprintf(stderr,"FIXME: handle reloc type %x at %x [%p] to %x\n",
                    type, (unsigned) addr, ptr, (unsigned) val);
            break;
    }
}

#endif /* !TARGET_DEFS_ONLY */
