/*
 *  MIL-STD-1750A assembler for TCC
 *
 *  Copyright (c) 2024
 */

#include "tcc.h"

static int parse_reg(void) {
    int reg = -1;
    if (tok >= TOK_IDENT) {
        /* simplistic register parsing: assuming identifiers like r0, R15, etc. */
        const char *name = get_tok_str(tok, &tokc);
        if ((name[0] == 'r' || name[0] == 'R') && isnum(name[1])) {
            reg = atoi(name + 1);
            if (reg < 0 || reg > 15) reg = -1;
        }
        next();
    }
    if (reg == -1) expect("register");
    return reg;
}

ST_FUNC void asm_instr(void)
{
    int op;
    op = tok;
    next();

    switch (op) {
        case TOK_ASM_li: {
            /* Example: li r0, 123 */
            int reg = parse_reg();
            int val = 0;
            if (tok == ',') next();
            if (tok == TOK_CINT) {
                val = tokc.i;
                next();
            } else {
                expect("integer constant");
            }
            /* emit opcode */
            g(0x5F00 | (reg << 4));
            g(val);
            break;
        }
        case TOK_ASM_lr: {
            int r1 = parse_reg();
            if (tok == ',') next();
            int r2 = parse_reg();
            g(0x1800 | (r1 << 4) | r2);
            break;
        }
        case TOK_ASM_a: {
            int r1 = parse_reg();
            if (tok == ',') next();
            int r2 = parse_reg();
            g(0xA100 | (r1 << 4) | r2);
            break;
        }
        case TOK_ASM_s: {
            int r1 = parse_reg();
            if (tok == ',') next();
            int r2 = parse_reg();
            g(0xB100 | (r1 << 4) | r2);
            break;
        }
        case TOK_ASM_l: {
            /* L R, Addr */
            int reg = parse_reg();
            int val = 0;
            if (tok == ',') next();
            if (tok == TOK_CINT) {
                val = tokc.i;
                next();
            } else {
                expect("address/integer constant");
            }
            g(0x5000 | (reg << 4));
            g(val);
            break;
        }
        case TOK_ASM_st: {
            /* ST R, Addr */
            int reg = parse_reg();
            int val = 0;
            if (tok == ',') next();
            if (tok == TOK_CINT) {
                val = tokc.i;
                next();
            } else {
                expect("address/integer constant");
            }
            g(0x7000 | (reg << 4));
            g(val);
            break;
        }
        case TOK_ASM_br: {
            /* BR Addr */
            int val = 0;
            if (tok == TOK_CINT) {
                val = tokc.i;
                next();
            } else {
                expect("address/integer constant");
            }
            g(0x7400);
            g(val);
            break;
        }
        case TOK_ASM_jc: {
            /* JC cond, Addr */
            int cond = 0, val = 0;
            if (tok == TOK_CINT) {
                cond = tokc.i;
                next();
            } else {
                expect("condition constant");
            }
            if (tok == ',') next();
            if (tok == TOK_CINT) {
                val = tokc.i;
                next();
            } else {
                expect("address/integer constant");
            }
            g(0x7000 | (cond << 4));
            g(val);
            break;
        }
        case TOK_ASM_soj: {
            /* SOJ R, Addr */
            int reg = parse_reg();
            int val = 0;
            if (tok == ',') next();
            if (tok == TOK_CINT) {
                val = tokc.i;
                next();
            } else {
                expect("address/integer constant");
            }
            g(0x7300 | (reg << 4));
            g(val);
            break;
        }
        case TOK_ASM_sjs: {
            /* SJS Addr */
            int val = 0;
            if (tok == TOK_CINT) {
                val = tokc.i;
                next();
            } else {
                expect("address/integer constant");
            }
            g(0x7E00);
            g(val);
            break;
        }
        case TOK_ASM_urs: {
            int reg = parse_reg();
            g(0xD100 | reg);
            break;
        }
        default:
            expect("known 1750A assembly instruction");
    }
}

ST_FUNC void asm_global_instr(void)
{
    asm_instr();
}

ST_FUNC int tcc_assemble(TCCState *s1, int do_preprocess)
{
    for(;;) {
        if (tok == TOK_EOF) break;
        asm_instr();
    }
    return 0;
}


#ifdef CONFIG_TCC_ASM
ST_FUNC int find_constraint(ASMOperand *operands, int nb_operands, const char *name, const char **pp)
{
    return -1;
}

ST_FUNC Sym* get_asm_sym(int name, Sym *csym)
{
    return NULL;
}

ST_FUNC void asm_expr(TCCState *s1, ExprValue *pe)
{
    int sym_index, sym_type;
    int v;

    switch(tok) {
    case TOK_CINT:
        pe->v = tokc.i;
        pe->sym = NULL;
        pe->pcrel = 0;
        next();
        break;
    case TOK_IDENT:
        v = tok;
        next();
        if (tok == '@') {
            next();
            if (tok == TOK_IDENT) {
                pe->v = 0;
                pe->sym = sym_find(tok);
                pe->pcrel = 0;
                next();
            } else {
                expect("identifier");
            }
        } else {
            pe->v = 0;
            pe->sym = sym_find(v);
            pe->pcrel = 0;
        }
        break;
    default:
        expect("constant or identifier");
    }
}

ST_FUNC int asm_int_expr(TCCState *s1)
{
    return 0;
}

ST_FUNC void asm_compute_constraints(ASMOperand *operands, int nb_operands, int nb_outputs, const uint8_t *clobber_regs, int *pout_reg)
{
}

ST_FUNC void subst_asm_operand(CString *add_str, SValue *sv, int modifier)
{
}

ST_FUNC void asm_gen_code(ASMOperand *operands, int nb_operands, int nb_outputs, int is_output, uint8_t *clobber_regs, int out_reg)
{
}

ST_FUNC void asm_clobber(uint8_t *clobber_regs, const char *str)
{
}
#endif
