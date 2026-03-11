/*
 *  MIL-STD-1750A code generator for TCC
 *
 *  Copyright (c) 2024
 */

#ifdef TARGET_DEFS_ONLY

/* number of available registers */
#define NB_REGS            16

/* a register can belong to several classes. The classes must be
   sorted from more general to more precise (see gv2() code which does
   assumptions on it). */
#define RC_INT     0x0001
#define RC_FLOAT   0x0002
#define RC_R0      0x0004
#define RC_R1      0x0008
#define RC_R2      0x0010
#define RC_R3      0x0020
#define RC_R4      0x0040
#define RC_R5      0x0080
#define RC_R6      0x0100
#define RC_R7      0x0200
#define RC_R8      0x0400
#define RC_R9      0x0800
#define RC_R10     0x1000
#define RC_R11     0x2000
#define RC_R12     0x4000
#define RC_R13     0x8000
#define RC_R14     0x10000
#define RC_R15     0x20000

#define RC_FLOAT_PAIR (RC_R0 | RC_R2 | RC_R4 | RC_R6 | RC_R8 | RC_R10 | RC_R12)

#define RC_IRET    RC_R0    /* function return: integer register */
#define RC_IRE2    RC_R1    /* function return: second integer register */
#define RC_FRET    RC_R0    /* function return: float register */

/* pretty names for the registers */
enum {
    TREG_R0 = 0,
    TREG_R1,
    TREG_R2,
    TREG_R3,
    TREG_R4,
    TREG_R5,
    TREG_R6,
    TREG_R7,
    TREG_R8,
    TREG_R9,
    TREG_R10,
    TREG_R11,
    TREG_R12,
    TREG_R13,
    TREG_R14,
    TREG_R15
};

#define REG_IRET TREG_R0
#define REG_IRE2 TREG_R1
#define REG_FRET TREG_R0

#define PTR_SIZE 1

#define LDOUBLE_SIZE  3
#define LDOUBLE_ALIGN 1
#define MAX_ALIGN     1

/* MIL-STD-1750A architecture definitions */
/* Data types:
 * char = 16-bit
 * short = 16-bit
 * int = 16-bit
 * long = 32-bit
 * long long = 32-bit (disabled 64-bit)
 * float = 32-bit (2 words)
 * double = 48-bit (3 words)
 * long double = 48-bit (3 words)
 * pointers = 16-bit
 */

#undef CONFIG_TCC_BCHECK

/******************************************************/
#else /* ! TARGET_DEFS_ONLY */
/******************************************************/
#define USING_GLOBALS
#include "tcc.h"

ST_DATA const char * const target_machine_defs =
    "__1750A__\0"
    ;

ST_DATA const int reg_classes[NB_REGS] = {
    RC_INT | RC_FLOAT | RC_R0,
    RC_INT | RC_FLOAT | RC_R1,
    RC_INT | RC_FLOAT | RC_R2,
    RC_INT | RC_FLOAT | RC_R3,
    RC_INT | RC_FLOAT | RC_R4,
    RC_INT | RC_FLOAT | RC_R5,
    RC_INT | RC_FLOAT | RC_R6,
    RC_INT | RC_FLOAT | RC_R7,
    RC_INT | RC_FLOAT | RC_R8,
    RC_INT | RC_FLOAT | RC_R9,
    RC_INT | RC_FLOAT | RC_R10,
    RC_INT | RC_FLOAT | RC_R11,
    RC_INT | RC_FLOAT | RC_R12,
    RC_INT | RC_FLOAT | RC_R13,
    RC_INT | RC_FLOAT | RC_R14,
    RC_INT | RC_FLOAT | RC_R15,
};

static int func_ret_sub;
static int func_args_size;

#define MAX_REGS_ARGS 4
static int func_args_regs[MAX_REGS_ARGS] = {TREG_R0, TREG_R1, TREG_R2, TREG_R3};

void g(int c)
{
    int ind1;
    if (nocode_wanted)
        return;
    ind1 = ind + 2;
    if (ind1 > (int) cur_text_section->data_allocated)
        section_realloc(cur_text_section, ind1);
    cur_text_section->data[ind] = (c >> 8) & 0xff;
    cur_text_section->data[ind + 1] = c & 0xff;
    ind = ind1;
}

void o(unsigned int c)
{
    g(c);
}

void gsym_addr(int t, int a)
{
    int n, *ptr;
    while (t) {
        ptr = (int *) (cur_text_section->data + t);
        n = *ptr; /* next link is stored in the offset/address field */
        *ptr = a; /* replace with actual address */
        t = n;
    }
}

ST_FUNC void gen_fill_nops(int bytes)
{
    if ((bytes & 1))
      tcc_error("alignment of code section not multiple of 2");
    while (bytes > 0) {
        g(0x0000); /* NOP for 1750a is usually a branch to next instruction or a zeroed instruction if interpreted as such */
        bytes -= 2;
    }
}

/* return registers for function */
ST_FUNC int gfunc_sret(CType *vt, int variadic, CType *ret, int *ret_align, int *regsize) {
    *ret_align = 1;
    return 0;
}

void load(int r, SValue * sv)
{
    int v, ft, fc, fr;
    SValue v1;

    fr = sv->r;
    ft = sv->type.t;
    fc = sv->c.i;

    v = fr & VT_VALMASK;
    if (fr & VT_LVAL) {
        if (v == VT_LLOCAL) {
            v1.type.t = VT_INT;
            v1.r = VT_LOCAL | VT_LVAL;
            v1.c.i = fc;
            load(r, &v1);
            fr = r;
        } else if (v == VT_LOCAL) {
            /* Load from frame pointer + offset. R14 is FP */
            if ((ft & VT_BTYPE) == VT_BYTE) {
                g(0x4000 | (r << 4) | 14); /* LB Rr, fc, R14 */
                g(fc);
            } else if ((ft & VT_BTYPE) == VT_DOUBLE || (ft & VT_BTYPE) == VT_LDOUBLE) {
                g(0x5000 | (r << 4) | 14); g(fc);     /* L Rr, fc, R14 */
                g(0x5000 | ((r+1) << 4) | 14); g(fc+1); /* L Rr+1, fc+1, R14 */
                g(0x5000 | ((r+2) << 4) | 14); g(fc+2); /* L Rr+2, fc+2, R14 */
            } else if ((ft & VT_BTYPE) == VT_FLOAT || (ft & VT_BTYPE) == VT_LONG || (ft & VT_BTYPE) == VT_LLONG) {
                g(0x5000 | (r << 4) | 14); g(fc);     /* L Rr, fc, R14 */
                g(0x5000 | ((r+1) << 4) | 14); g(fc+1); /* L Rr+1, fc+1, R14 */
            } else {
                g(0x5000 | (r << 4) | 14); /* L Rr, fc, R14 */
                g(fc);
            }
        } else if (v < VT_CONST) {
            /* indirect load */
            if ((ft & VT_BTYPE) == VT_BYTE) {
                g(0x4000 | (r << 4) | v); /* LB Rr, 0, Rv */
                g(0);
            } else if ((ft & VT_BTYPE) == VT_DOUBLE || (ft & VT_BTYPE) == VT_LDOUBLE) {
                g(0x5000 | (r << 4) | v); g(0);     /* L Rr, 0, Rv */
                g(0x5000 | ((r+1) << 4) | v); g(1); /* L Rr+1, 1, Rv */
                g(0x5000 | ((r+2) << 4) | v); g(2); /* L Rr+2, 2, Rv */
            } else if ((ft & VT_BTYPE) == VT_FLOAT || (ft & VT_BTYPE) == VT_LONG || (ft & VT_BTYPE) == VT_LLONG) {
                g(0x5000 | (r << 4) | v); g(0);     /* L Rr, 0, Rv */
                g(0x5000 | ((r+1) << 4) | v); g(1); /* L Rr+1, 1, Rv */
            } else {
                g(0x5000 | (r << 4) | v); /* L Rr, 0, Rv */
                g(0);
            }
        } else if (v == VT_CONST) {
            /* absolute load */
            if (fr & VT_SYM) {
                greloc(cur_text_section, sv->sym, ind + 2, R_C60_32);
            }
            if ((ft & VT_BTYPE) == VT_BYTE) {
                g(0x4000 | (r << 4) | 0); /* LB Rr, fc, R0 */
                g(fc);
            } else if ((ft & VT_BTYPE) == VT_DOUBLE || (ft & VT_BTYPE) == VT_LDOUBLE) {
                g(0x5000 | (r << 4) | 0); g(fc);     /* L Rr, fc, R0 */
                g(0x5000 | ((r+1) << 4) | 0); g(fc+1); /* L Rr+1, fc+1, R0 */
                g(0x5000 | ((r+2) << 4) | 0); g(fc+2); /* L Rr+2, fc+2, R0 */
            } else if ((ft & VT_BTYPE) == VT_FLOAT || (ft & VT_BTYPE) == VT_LONG || (ft & VT_BTYPE) == VT_LLONG) {
                g(0x5000 | (r << 4) | 0); g(fc);     /* L Rr, fc, R0 */
                g(0x5000 | ((r+1) << 4) | 0); g(fc+1); /* L Rr+1, fc+1, R0 */
            } else {
                g(0x5000 | (r << 4) | 0); /* L Rr, fc, R0 */
                g(fc);
            }
        }
    } else {
        if (v == VT_CONST) {
            if (fr & VT_SYM) {
                greloc(cur_text_section, sv->sym, ind + 2, R_C60_32);
            }
            /* LI (Load Immediate) */
            if ((ft & VT_BTYPE) == VT_DOUBLE || (ft & VT_BTYPE) == VT_LDOUBLE) {
                g(0x5F00 | (r << 4)); g(fc);
                g(0x5F00 | ((r+1) << 4)); g(sv->c.tab[1]);
                g(0x5F00 | ((r+2) << 4)); g(sv->c.tab[2]);
            } else if ((ft & VT_BTYPE) == VT_FLOAT || (ft & VT_BTYPE) == VT_LONG || (ft & VT_BTYPE) == VT_LLONG) {
                g(0x5F00 | (r << 4)); g(fc);
                g(0x5F00 | ((r+1) << 4)); g(sv->c.tab[1]);
            } else {
                g(0x5F00 | (r << 4)); /* LI Rr, fc */
                g(fc);
            }
        } else if (v != r) {
            /* LR (Load Register) */
            if ((ft & VT_BTYPE) == VT_DOUBLE || (ft & VT_BTYPE) == VT_LDOUBLE) {
                g(0x1800 | (r << 4) | v); /* LR Rr, Rv */
                g(0x1800 | ((r+1) << 4) | (v+1));
                g(0x1800 | ((r+2) << 4) | (v+2));
            } else if ((ft & VT_BTYPE) == VT_FLOAT || (ft & VT_BTYPE) == VT_LONG || (ft & VT_BTYPE) == VT_LLONG) {
                g(0x1800 | (r << 4) | v); /* LR Rr, Rv */
                g(0x1800 | ((r+1) << 4) | (v+1));
            } else {
                g(0x1800 | (r << 4) | v); /* LR Rr, Rv */
            }
        }
    }
}

void store(int r, SValue * v)
{
    int fr, ft, fc;

    ft = v->type.t;
    fc = v->c.i;
    fr = v->r & VT_VALMASK;

    if (fr == VT_LOCAL) {
        /* Store to frame pointer + offset. R14 is FP */
        if ((ft & VT_BTYPE) == VT_BYTE) {
            g(0x6000 | (r << 4) | 14); /* STB Rr, fc, R14 */
            g(fc);
        } else if ((ft & VT_BTYPE) == VT_DOUBLE || (ft & VT_BTYPE) == VT_LDOUBLE) {
            g(0x7000 | (r << 4) | 14); g(fc);
            g(0x7000 | ((r+1) << 4) | 14); g(fc+1);
            g(0x7000 | ((r+2) << 4) | 14); g(fc+2);
        } else if ((ft & VT_BTYPE) == VT_FLOAT || (ft & VT_BTYPE) == VT_LONG || (ft & VT_BTYPE) == VT_LLONG) {
            g(0x7000 | (r << 4) | 14); g(fc);
            g(0x7000 | ((r+1) << 4) | 14); g(fc+1);
        } else {
            g(0x7000 | (r << 4) | 14); /* ST Rr, fc, R14 */
            g(fc);
        }
    } else if (fr < VT_CONST) {
        /* indirect store */
        if ((ft & VT_BTYPE) == VT_BYTE) {
            g(0x6000 | (r << 4) | fr); /* STB Rr, 0, Rfr */
            g(0);
        } else if ((ft & VT_BTYPE) == VT_DOUBLE || (ft & VT_BTYPE) == VT_LDOUBLE) {
            g(0x7000 | (r << 4) | fr); g(0);
            g(0x7000 | ((r+1) << 4) | fr); g(1);
            g(0x7000 | ((r+2) << 4) | fr); g(2);
        } else if ((ft & VT_BTYPE) == VT_FLOAT || (ft & VT_BTYPE) == VT_LONG || (ft & VT_BTYPE) == VT_LLONG) {
            g(0x7000 | (r << 4) | fr); g(0);
            g(0x7000 | ((r+1) << 4) | fr); g(1);
        } else {
            g(0x7000 | (r << 4) | fr); /* ST Rr, 0, Rfr */
            g(0);
        }
    } else if (fr == VT_CONST) {
        /* absolute store */
        if (v->r & VT_SYM) {
            greloc(cur_text_section, v->sym, ind + 2, R_C60_32);
        }
        if ((ft & VT_BTYPE) == VT_BYTE) {
            g(0x6000 | (r << 4) | 0); /* STB Rr, fc, R0 */
            g(fc);
        } else if ((ft & VT_BTYPE) == VT_DOUBLE || (ft & VT_BTYPE) == VT_LDOUBLE) {
            g(0x7000 | (r << 4) | 0); g(fc);
            g(0x7000 | ((r+1) << 4) | 0); g(fc+1);
            g(0x7000 | ((r+2) << 4) | 0); g(fc+2);
        } else if ((ft & VT_BTYPE) == VT_FLOAT || (ft & VT_BTYPE) == VT_LONG || (ft & VT_BTYPE) == VT_LLONG) {
            g(0x7000 | (r << 4) | 0); g(fc);
            g(0x7000 | ((r+1) << 4) | 0); g(fc+1);
        } else {
            g(0x7000 | (r << 4) | 0); /* ST Rr, fc, R0 */
            g(fc);
        }
    }
}

void gfunc_call(int nb_args)
{
    int i, r, arg_regs;

    arg_regs = nb_args > MAX_REGS_ARGS ? MAX_REGS_ARGS : nb_args;

    /* Push arguments onto the stack (reverse order usually, but TCC evaluates LTR and pushes RTL) */
    /* Wait, TCC evaluates them and they are on the vstack. We pop from vtop. */
    /* Thus we iterate from last to first argument. */

    for (i = 0; i < nb_args; i++) {
        if (i < arg_regs) {
            /* Argument goes in register, load it to corresponding reg */
            r = gv(1 << func_args_regs[arg_regs - 1 - i]);
        } else {
            /* Argument goes on stack */
            r = gv(RC_INT);
            /* PSHM Rr, R15 */
            g(0x1100 | (r << 4) | 15);
        }
        vtop--;
    }

    if ((vtop->r & (VT_VALMASK | VT_LVAL)) == VT_CONST) {
        if (vtop->r & VT_SYM) {
            /* JC (Jump to Subroutine) */
            greloc(cur_text_section, vtop->sym, ind + 2, R_C60_32);
            g(0x4C00); /* SJS */
            g(0);
        }
    } else {
        r = gv(RC_INT);
        /* Jump to Subroutine Register */
        g(0x1C00 | (r << 4)); /* SJS (indirect) via reg */
    }

    vtop--;

    if (nb_args > MAX_REGS_ARGS) {
        int stack_args = nb_args - MAX_REGS_ARGS;
        /* adjust stack pointer */
        /* AIS R15, stack_args */
        g(0x1600 | (15 << 4) | stack_args);
    }
}

void gfunc_prolog(Sym *func_sym)
{
    CType *func_type = &func_sym->type;
    int addr, align, size, func_call, i;
    Sym *sym;
    CType *type;

    sym = func_type->ref;
    func_call = sym->f.func_call;
    addr = 0;

    /* define parameters */
    i = 0;
    while ((sym = sym->next) != NULL) {
        type = &sym->type;
        size = type_size(type, &align);
        if (i < MAX_REGS_ARGS) {
            /* Parameter is passed in register */
            /* In a real implementation we would save it to the stack frame or map it properly */
            sym->r = VT_LOCAL | VT_LVAL;
            sym->c = loc;
            /* PSHM R_param, R15 */
            g(0x1100 | (func_args_regs[i] << 4) | 15);
            loc -= size;
        } else {
            /* Parameter is passed on stack */
            sym->r = VT_LOCAL | VT_LVAL;
            sym->c = addr + 4; /* offset for old fp and ret addr */
            addr += size;
        }
        i++;
    }

    func_ret_sub = 0;

    /* save frame pointer (R14) */
    g(0x1100 | (14 << 4) | 15); /* PSHM R14, R15 */
    /* set new frame pointer */
    g(0x1800 | (14 << 4) | 15); /* LR R14, R15 */
}

void gfunc_epilog(void)
{
    /* restore frame pointer */
    g(0x1800 | (15 << 4) | 14); /* LR R15, R14 */
    /* POPM R14, R15 */
    g(0x1200 | (14 << 4) | 15);
    /* URS (Unstack Return Subroutine) */
    g(0x1D00 | 15);
}

int gjmp(int t)
{
    int ind1 = ind;
    if (nocode_wanted)
        return t;
    /* B (Branch unconditionally) */
    g(0x4000);
    g(t);
    return ind1;
}

void gjmp_addr(int a)
{
    /* B (Branch unconditionally) */
    g(0x4000);
    g(a);
}

ST_FUNC int gjmp_cond(int op, int t)
{
    int ind1 = ind;
    if (nocode_wanted)
        return t;

    /* 1750A Branch conditions */
    int cond = 0;
    switch(op) {
        case TOK_EQ: cond = 0x8; break; /* BEZ */
        case TOK_NE: cond = 0x9; break; /* BNZ */
        case TOK_LT: cond = 0xA; break; /* BLZ */
        case TOK_GE: cond = 0xB; break; /* BGEZ */
        case TOK_GT: cond = 0xC; break; /* BGZ */
        case TOK_LE: cond = 0xD; break; /* BLEZ */
        default: cond = 0x0; break; /* NOP/Unconditional */
    }

    g(0x4000 | (cond << 8));
    g(t);

    return ind1;
}

ST_FUNC int gjmp_append(int n0, int t)
{
    if (n0) {
        int n = n0, *p;
        while (n != 0) {
            p = (int *) (cur_text_section->data + n + 2); /* offset of branch address */
            n = *p;
        }
        *p = t;
        t = n0;
    }
    return t;
}

void gen_opi(int op)
{
    int r, fr, opc;

    gv2(RC_INT, RC_INT);
    r = vtop[-1].r;
    fr = vtop[0].r;
    vtop--;

    switch(op) {
        case '+':
            g(0x1A00 | (r << 4) | fr); /* A Rr, Rfr */
            break;
        case '-':
            g(0x1B00 | (r << 4) | fr); /* S Rr, Rfr */
            break;
        case '*':
            g(0x1C00 | (r << 4) | fr); /* M Rr, Rfr */
            break;
        case '/':
            g(0x1D00 | (r << 4) | fr); /* D Rr, Rfr */
            break;
        case '&':
            g(0x1E00 | (r << 4) | fr); /* N Rr, Rfr (AND) */
            break;
        case '|':
            g(0x1F00 | (r << 4) | fr); /* O Rr, Rfr (OR) */
            break;
        case '^':
            g(0x2000 | (r << 4) | fr); /* X Rr, Rfr (XOR) */
            break;
        case TOK_SHL:
            g(0x2100 | (r << 4) | fr); /* SLL Rr, Rfr */
            break;
        case TOK_SHR:
            g(0x2200 | (r << 4) | fr); /* SRL Rr, Rfr */
            break;
        case TOK_SAR:
            g(0x2300 | (r << 4) | fr); /* SRA Rr, Rfr */
            break;
        case TOK_EQ:
        case TOK_NE:
        case TOK_LT:
        case TOK_GE:
        case TOK_GT:
        case TOK_LE:
            g(0x1900 | (r << 4) | fr); /* C Rr, Rfr (Compare) */
            vset_VT_CMP(op);
            break;
        default:
            tcc_error("unsupported integer operation: %d", op);
    }
}

void gen_opf(int op)
{
    int r, fr, is_double;

    is_double = (vtop->type.t & VT_BTYPE) == VT_DOUBLE || (vtop->type.t & VT_BTYPE) == VT_LDOUBLE;

    gv2(RC_FLOAT, RC_FLOAT);
    r = vtop[-1].r;
    fr = vtop[0].r;
    vtop--;

    switch(op) {
        case '+':
            g((is_double ? 0x8A00 : 0x8000) | (r << 4) | fr); /* FA/EFA */
            break;
        case '-':
            g((is_double ? 0x8B00 : 0x8100) | (r << 4) | fr); /* FS/EFS */
            break;
        case '*':
            g((is_double ? 0x8C00 : 0x8200) | (r << 4) | fr); /* FM/EFM */
            break;
        case '/':
            g((is_double ? 0x8D00 : 0x8300) | (r << 4) | fr); /* FD/EFD */
            break;
        case TOK_EQ:
        case TOK_NE:
        case TOK_LT:
        case TOK_GE:
        case TOK_GT:
        case TOK_LE:
            g((is_double ? 0x8900 : 0x8400) | (r << 4) | fr); /* FC/EFC (Compare) */
            vset_VT_CMP(op);
            break;
        default:
            tcc_error("unsupported floating point operation: %d", op);
    }
}

void gen_cvt_itof(int t)
{
    int r;
    gv(RC_INT);
    r = vtop->r;
    if ((t & VT_BTYPE) == VT_FLOAT) {
        g(0x8F00 | (r << 4) | r); /* FLT (Float) - Assuming Rr -> Rr */
    } else {
        /* Double Float not directly single instruction, assuming EFLT */
        g(0x9F00 | (r << 4) | r);
    }
    vtop->type.t = t;
}

void gen_cvt_ftoi(int t)
{
    int r;
    gv(RC_FLOAT);
    r = vtop->r;
    if ((vtop->type.t & VT_BTYPE) == VT_FLOAT) {
        g(0x8E00 | (r << 4) | r); /* FIX (Fix floating point) */
    } else {
        g(0x9E00 | (r << 4) | r); /* EFIX */
    }
    vtop->type.t = t;
}

void gen_cvt_ftof(int t)
{
    int r;
    gv(RC_FLOAT);
    r = vtop->r;
    if ((vtop->type.t & VT_BTYPE) == VT_FLOAT && ((t & VT_BTYPE) == VT_DOUBLE || (t & VT_BTYPE) == VT_LDOUBLE)) {
        /* Convert Float to Double (pad with 0s conceptually, but typically not natively single-instruction) */
        /* Placeholder for actual sequence or library call if 1750A lacks native F->D */
        g(0x1800 | (r << 4) | r); /* LR (dummy) */
    } else if (((vtop->type.t & VT_BTYPE) == VT_DOUBLE || (vtop->type.t & VT_BTYPE) == VT_LDOUBLE) && (t & VT_BTYPE) == VT_FLOAT) {
        /* Convert Double to Float */
        g(0x1800 | (r << 4) | r); /* LR (dummy) */
    }
    vtop->type.t = t;
}

void gcall_or_jmp(int is_jmp)
{
    int r;
    Sym *sym;
    if ((vtop->r & (VT_VALMASK | VT_LVAL)) == VT_CONST) {
        if (vtop->r & VT_SYM) {
            /* jump with relocation */
            greloc(cur_text_section, vtop->sym, ind + 2, R_C60_32);
            if (is_jmp) {
                g(0x4000); /* B */
                g(0);
            } else {
                g(0x4C00); /* SJS */
                g(0);
            }
        }
    } else {
        r = gv(RC_INT);
        if (is_jmp) {
            g(0x1D00 | (r << 4)); /* B via reg */
        } else {
            g(0x1C00 | (r << 4)); /* SJS via reg */
        }
    }
}

void ggoto(void)
{
    gcall_or_jmp(1);
    vtop--;
}

ST_FUNC void gen_vla_sp_save(int addr) {
    /* Save R15 (SP) to addr (relative to R14/FP) */
    /* ST R15, addr, R14 */
    g(0x7000 | (15 << 4) | 14);
    g(addr);
}

ST_FUNC void gen_vla_sp_restore(int addr) {
    /* Restore R15 (SP) from addr (relative to R14/FP) */
    /* L R15, addr, R14 */
    g(0x5000 | (15 << 4) | 14);
    g(addr);
}

ST_FUNC void gen_vla_alloc(CType *type, int align) {
    int r;
    r = gv(RC_INT);
    /* Subtract the size from SP */
    /* S R15, r */
    g(0x1B00 | (15 << 4) | r);
    /* Align SP if needed. Usually SP must be aligned to 1 word for 1750a so no big alignment instructions needed. */
    vtop--;
    vset(&int_type, VT_LOCAL | VT_LVAL, 0);
}

#endif
