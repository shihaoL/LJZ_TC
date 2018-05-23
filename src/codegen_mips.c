#include <assert.h>
#include <stdlib.h>
#include <util.h>
#include "codegen.h"

static AS_instrList iList = NULL, last = NULL;
static const IMM_MAX = 32767;
static const IMM_MIN = -32768;

static void emit(AS_instr inst);
static void munchStm(T_stm s);
static Temp_temp munchExp(T_exp e);

static void emit(AS_instr inst) {
    if (last) {
        last = last->tail = AS_InstrList(inst, NULL);
    }
    else {
        last = iList = AS_InstrList(inst, NULL);
    }
}


static void munchStm(T_stm s) {
    switch (s->kind) {
    case T_LABEL:
        emit(AS_Label(FormatString("%s:\n", Temp_labelstring(s->u.LABEL)), s->u.LABEL));
        return;
    case T_JUMP:
        assert(s->u.JUMP.exp->kind == T_NAME);
        emit(AS_Oper(FormatString("j %s\n", Temp_labelstring(s->u.JUMP.exp->u.NAME)), NULL, NULL, AS_Targets(s->u.JUMP.jumps)));
        // emit(AS_Oper("j `j0\n", NULL, NULL, AS_Targets(Temp_LabelList(s->u.JUMP.exp->u.NAME, NULL))));
        emit(AS_Oper("nop\n", NULL, NULL, NULL));
        return;
    case T_CJUMP:
        AS_targets targets = AS_Targets(Temp_LabelList(s->u.CJUMP.true, NULL));
        Temp_temp left = munchExp(s->u.CJUMP.left);
        Temp_temp right = munchExp(s->u.CJUMP.right);
        string labelStr = Temp_labelstring(s->u.CJUMP.true);
        switch (s->u.CJUMP.op) {
        case T_eq:
            emit(AS_Oper(FormatString("beq `s0, `s1, %s\n", labelStr), NULL,
                         Temp_TempList(left, Temp_TempList(right, NULL)), targets
                        ));
            emit(AS_Oper("nop\n", NULL, NULL, NULL));
            return;
        case T_ne:
            emit(AS_Oper(FormatString("bne `s0, `s1, %s\n", labelStr), NULL,
                         Temp_TempList(left, Temp_TempList(right, NULL)), targets
                        ));
            emit(AS_Oper("nop\n", NULL, NULL, NULL));
            return;
        case T_lt:
            emit(AS_Oper("slt $at, `s0, `s1\n", NULL, Temp_TempList(left, Temp_TempList(right, NULL)), NULL));
            emit(AS_Oper(FormatString("bne $at, $zero, %s\n", labelStr), NULL, NULL, NULL));
            emit(AS_Oper("nop\n", NULL, NULL, NULL));
            return;
        case T_gt:
            emit(AS_Oper("slt $at, `s0, `s1\n", NULL, Temp_TempList(right, Temp_TempList(left, NULL)), NULL));
            emit(AS_Oper(FormatString("bne $at, $zero, %s\n", labelStr), NULL, NULL, NULL));
            emit(AS_Oper("nop\n", NULL, NULL, NULL));
            return;
        case T_le:
            emit(AS_Oper("slt $at, `s0, `s1\n", NULL, Temp_TempList(right, Temp_TempList(left, NULL)), NULL));
            emit(AS_Oper(FormatString("beq $at, $zero, %s\n", labelStr), NULL, NULL, NULL));
            emit(AS_Oper("nop\n", NULL, NULL, NULL));
            return;
        case T_ge:
            emit(AS_Oper("slt $at, `s0, `s1\n", NULL, Temp_TempList(left, Temp_TempList(right, NULL)), NULL));
            emit(AS_Oper(FormatString("beq $at, $zero, %s\n", labelStr), NULL, NULL, NULL));
            emit(AS_Oper("nop\n", NULL, NULL, NULL));
            return;
        default:
            assert(0);
        }
    case T_MOVE:
    {
        if (s->u.MOVE.dst->kind == T_MEM) {
            T_exp e = s->u.MOVE.dst->u.MEM;
            if (e->kind == T_BINOP && e->u.BINOP.op == T_plus) {
                if (e->u.BINOP.left->kind == T_CONST && e->u.BINOP.left->u.CONST <= IMM_MAX && e->u.BINOP.left->u.CONST >= IMM_MIN) {
                    emit(AS_Oper(FormatString("sw `s0, %d(`d0)\n", e->u.BINOP.left->u.CONST),
                                 Temp_TempList(munchExp(e->u.BINOP.right), NULL), Temp_TempList(munchExp(s->u.MOVE.src), NULL), NULL
                                ));
                }
                else if (e->u.BINOP.right->kind == T_CONST && e->u.BINOP.right->u.CONST <= IMM_MAX && e->u.BINOP.left->u.CONST >= IMM_MIN) {
                    emit(AS_Oper(FormatString("sw `s0, %d(`d0)\n", e->u.BINOP.right->u.CONST),
                                 Temp_TempList(munchExp(e->u.BINOP.left), NULL), Temp_TempList(munchExp(s->u.MOVE.src), NULL), NULL
                                ));
                }
                else {
                    emit(AS_Oper("sw `s0, 0(`d0)\n", Temp_TempList(munchExp(e), NULL), Temp_TempList(munchExp(s->u.MOVE.src), NULL), NULL));
                }
            }
            else if (e->kind == T_BINOP && e->u.BINOP.op == T_minus && e->u.BINOP.right->kind == T_CONST
                     && e->u.BINOP.right->u.CONST <= IMM_MAX + 1 && e->u.BINOP.right->u.CONST > IMM_MIN
                    ) {
                emit(AS_Oper(FormatString("sw `s0, %d(`d0)\n", -e->u.BINOP.right->u.CONST),
                             Temp_TempList(munchExp(e->u.BINOP.left), NULL), Temp_TempList(munchExp(s->u.MOVE.src), NULL), NULL
                            ));
            }
            else if (e->kind == T_CONST && e->u.CONST <= IMM_MAX && e->u.CONST >= IMM_MIN) {
                emit(AS_Oper(FormatString("sw `s0, %d($zero)\n", e->u.CONST),
                             NULL, Temp_TempList(munchExp(s->u.MOVE.src), NULL), NULL
                            ));
            }
            else {
                emit(AS_Oper("sw `s0, 0(`d0)\n", Temp_TempList(munchExp(e), NULL), Temp_TempList(munchExp(s->u.MOVE.src), NULL), NULL));
            }
        }
        else if (s->u.MOVE.src->kind == T_MEM) {
            T_exp e = s->u.MOVE.src->u.MEM;
            if (e->kind == T_BINOP && e->u.BINOP.op == T_plus) {
                if (e->u.BINOP.left->kind == T_CONST && e->u.BINOP.left->u.CONST <= IMM_MAX && e->u.BINOP.left->u.CONST >= IMM_MIN) {
                    emit(AS_Oper(FormatString("lw `d0, %d(`s0)\n", e->u.BINOP.left->u.CONST),
                                 Temp_TempList(munchExp(s->u.MOVE.dst), NULL), Temp_TempList(munchExp(e->u.BINOP.right), NULL), NULL
                                ));
                }
                else if (e->u.BINOP.right->kind == T_CONST && e->u.BINOP.right->u.CONST <= IMM_MAX && e->u.BINOP.left->u.CONST >= IMM_MIN) {
                    emit(AS_Oper(FormatString("lw `d0, %d(`s0)\n", e->u.BINOP.right->u.CONST),
                                 Temp_TempList(munchExp(s->u.MOVE.dst), NULL), Temp_TempList(munchExp(e->u.BINOP.left), NULL), NULL
                                ));
                }
                else {
                    emit(AS_Oper("lw `d0, 0(`s0)\n", Temp_TempList(munchExp(s->u.MOVE.dst), NULL), Temp_TempList(munchExp(e), NULL), NULL));
                }
            }
            else if (e->kind == T_BINOP && e->u.BINOP.op == T_minus && e->u.BINOP.right->kind == T_CONST
                     && e->u.BINOP.right->u.CONST <= IMM_MAX + 1 && e->u.BINOP.right->u.CONST > IMM_MIN
                    ) {
                emit(AS_Oper(FormatString("lw `d0, %d(`s0)\n", -e->u.BINOP.right->u.CONST),
                             Temp_TempList(munchExp(s->u.MOVE.dst), NULL), Temp_TempList(munchExp(e->u.BINOP.left), NULL), NULL
                            ));
            }
            else if (e->kind == T_CONST && e->u.CONST <= IMM_MAX && e->u.CONST >= IMM_MIN) {
                emit(AS_Oper(FormatString("lw `d0, %d($zero)\n", e->u.CONST),
                             Temp_TempList(munchExp(s->u.MOVE.dst), NULL), NULL, NULL
                            ));
            }
            else {
                emit(AS_Oper("lw `d0, 0(`s0)\n", Temp_TempList(munchExp(s->u.MOVE.dst->u.MEM), NULL), Temp_TempList(munchExp(e), NULL), NULL));
            }
        }
        else if (s->u.MOVE.src->kind == T_CONST) {
            emit(AS_Move(FormatString("li `d0, %d\n", s->u.MOVE.src->u.CONST),
                         Temp_TempList(munchExp(s->u.MOVE.dst), NULL), NULL, NULL));
        }
        else {
            // emit(AS_Move("move `d0, `s0\n", Temp_TempList(munchExp(s->u.MOVE.dst), NULL), Temp_TempList(munchExp(s->u.MOVE.src), NULL), NULL));
            T_exp e = s->u.MOVE.src;
            if (e->kind == T_BINOP && e->u.BINOP.op == T_plus) {
                if (e->u.BINOP.left->kind == T_CONST && e->u.BINOP.left->u.CONST <= IMM_MAX && e->u.BINOP.left->u.CONST >= IMM_MIN) {
                    emit(AS_Oper(FormatString("addi `d0, `s0, %d\n", e->u.BINOP.left->u.CONST),
                                 Temp_TempList(munchExp(s->u.MOVE.dst), NULL), Temp_TempList(munchExp(e->u.BINOP.right), NULL), NULL
                                ));
                }
                else if (e->u.BINOP.right->kind == T_CONST && e->u.BINOP.right->u.CONST <= IMM_MAX && e->u.BINOP.left->u.CONST >= IMM_MIN) {
                    emit(AS_Oper(FormatString("addi `d0, `s0, %d\n", e->u.BINOP.right->u.CONST),
                                 Temp_TempList(munchExp(s->u.MOVE.dst), NULL), Temp_TempList(munchExp(e->u.BINOP.left), NULL), NULL
                                ));
                }
                else {
                    emit(AS_Move("move `d0, `s0\n", Temp_TempList(munchExp(s->u.MOVE.dst), NULL), Temp_TempList(munchExp(s->u.MOVE.src), NULL), NULL));
                }
            }
            else if (e->kind == T_BINOP && e->u.BINOP.op == T_minus && e->u.BINOP.right->kind == T_CONST
                     && e->u.BINOP.right->u.CONST <= IMM_MAX + 1 && e->u.BINOP.right->u.CONST > IMM_MIN
                    ) {
                emit(AS_Oper(FormatString("addi `d0, `s0, %d\n", -e->u.BINOP.right->u.CONST),
                             Temp_TempList(munchExp(s->u.MOVE.dst), NULL), Temp_TempList(munchExp(e->u.BINOP.left), NULL), NULL
                            ));
            }
            else {
                emit(AS_Move("move `d0, `s0\n", Temp_TempList(munchExp(s->u.MOVE.dst), NULL), Temp_TempList(munchExp(s->u.MOVE.src), NULL), NULL));
            }
        }
        return;
    }
    case T_EXP:
        munchExp(s->u.EXP);
        return;
    default:
        assert(0);
    }

}


static Temp_temp munchExp(T_exp e) {
    switch (e->kind) {
    case T_BINOP:
        switch (e->u.BINOP.op) {
        case T_mul:
        {
            Temp_temp d = Temp_newtemp();
            emit(AS_Oper("mul `d0, `s0, `s1\n", Temp_TempList(d, NULL),
                         Temp_TempList(munchExp(e->u.BINOP.left),
                                       Temp_TempList(munchExp(e->u.BINOP.right), NULL)), NULL));
            return d;
        }
        case T_div:
        {
            Temp_temp d = Temp_newtemp();
            emit(AS_Oper("div `s0, `s1\n", NULL,
                         Temp_TempList(munchExp(e->u.BINOP.left),
                                       Temp_TempList(munchExp(e->u.BINOP.right), NULL)),
                         NULL));
            emit(AS_Oper("mflo `d0\n", Temp_TempList(d, NULL), NULL, NULL));

            return d;
        }
        case T_plus:
        {
            Temp_temp d = Temp_newtemp();
            if (e->u.BINOP.left->kind == T_CONST && e->u.BINOP.left->u.CONST <= IMM_MAX && e->u.BINOP.left->u.CONST >= IMM_MIN) {
                emit(AS_Oper(FormatString("addi `d0, `s0, %d\n", e->u.BINOP.left->u.CONST),
                             Temp_TempList(d, NULL), Temp_TempList(munchExp(e->u.BINOP.right), NULL), NULL));
            }
            else if (e->u.BINOP.right->kind == T_CONST && e->u.BINOP.right->u.CONST <= IMM_MAX && e->u.BINOP.right->u.CONST >= IMM_MIN) {
                emit(AS_Oper(FormatString("addi `d0, `s0, %d\n", e->u.BINOP.right->u.CONST),
                             Temp_TempList(d, NULL), Temp_TempList(munchExp(e->u.BINOP.left), NULL), NULL));
            }
            else {
                emit(AS_Oper("add `d0, `s0, `s1\n", Temp_TempList(d, NULL),
                             Temp_TempList(munchExp(e->u.BINOP.left), Temp_TempList(munchExp(e->u.BINOP.right), NULL)), NULL));
            }
            return d;
        }
        case T_minus:
        {
            Temp_temp d = Temp_newtemp();
            if (e->u.BINOP.right->kind == T_CONST && e->u.BINOP.right->u.CONST <= IMM_MAX + 1 && e->u.BINOP.right->u.CONST > IMM_MIN) {
                emit(AS_Oper(FormatString("addi `d0, `s0, %d\n", -e->u.BINOP.right->u.CONST),
                             Temp_TempList(d, NULL), Temp_TempList(munchExp(e->u.BINOP.left), NULL), NULL));
            }
            else {
                emit(AS_Oper("sub `d0, `s0, `s1\n", Temp_TempList(d, NULL), Temp_TempList(munchExp(e->u.BINOP.left),
                             Temp_TempList(munchExp(e->u.BINOP.right), NULL)), NULL));
            }
            return d;
        }
        default:
            assert(0);
        }
    case T_MEM:
    {
        Temp_temp d = Temp_newtemp();
        if (e->u.MEM->kind == T_BINOP) {
            if (e->u.MEM->u.BINOP.op == T_plus) {
                if (e->u.MEM->u.BINOP.left->kind == T_CONST && e->u.MEM->u.BINOP.left->u.CONST <= IMM_MAX && e->u.MEM->u.BINOP.left->u.CONST >= IMM_MIN) {
                    emit(AS_Oper(FormatString("lw `d0, %d(`s0)\n", e->u.MEM->u.BINOP.left->u.CONST),
                                 Temp_TempList(d, NULL), Temp_TempList(munchExp(e->u.MEM->u.BINOP.right), NULL), NULL
                                ));
                }
                else if (e->u.MEM->u.BINOP.right->kind == T_CONST && e->u.MEM->u.BINOP.right->u.CONST <= IMM_MAX && e->u.MEM->u.BINOP.right->u.CONST >= IMM_MIN) {
                    emit(AS_Oper(FormatString("lw `d0, %d(`s0)\n", e->u.MEM->u.BINOP.right->u.CONST),
                                 Temp_TempList(d, NULL), Temp_TempList(munchExp(e->u.MEM->u.BINOP.left), NULL), NULL
                                ));
                }
                else {
                    emit(AS_Oper("lw `d0, 0(`s0)\n", Temp_TempList(d, NULL), Temp_TempList(munchExp(e->u.MEM), NULL), NULL));
                }
            }
            else if (e->u.MEM->u.BINOP.op == T_minus && e->u.MEM->u.BINOP.right->kind == T_CONST && e->u.MEM->u.BINOP.right->u.CONST <= IMM_MAX + 1 && e->u.MEM->u.BINOP.right->u.CONST > IMM_MIN) {
                emit(AS_Oper(FormatString("lw `d0, %d(`s0)\n", -e->u.MEM->u.BINOP.right->u.CONST),
                             Temp_TempList(d, NULL), Temp_TempList(munchExp(e->u.MEM->u.BINOP.left), NULL), NULL
                            ));
            }
            else {
                emit(AS_Oper("lw `d0, 0(`s0)\n", Temp_TempList(d, NULL), Temp_TempList(munchExp(e->u.MEM), NULL), NULL));
            }
        }
        else if (e->u.MEM->kind == T_CONST && e->u.MEM->u.CONST <= IMM_MAX && e->u.MEM->u.CONST >= IMM_MIN) {
            emit(AS_Oper(FormatString("lw `d0, %d($zero)\n", e->u.MEM->u.CONST),
                         Temp_TempList(d, NULL), NULL, NULL
                        ));
        }
        else {
            emit(AS_Oper("lw `d0, 0(`s0)\n", Temp_TempList(d, NULL), Temp_TempList(munchExp(e->u.MEM), NULL), NULL));
        }
        return d;
    }
    case T_TEMP:
        return e->u.TEMP;
    case T_NAME: {
        Temp_temp d = Temp_newtemp();
        emit(AS_Oper(FormatString("la `d0, %s\n", Temp_labelstring(e->u.NAME)), Temp_TempList(d, NULL), NULL, NULL));
        return d;
    }
    case T_CONST:
    {
        Temp_temp d = Temp_newtemp();
        emit(AS_Oper(FormatString("li `d0, %d\n", e->u.CONST), Temp_TempList(d, NULL), NULL, NULL));
        return d;
    }
    case T_CALL:
    {
        //TODO:
        Temp_temp d = Temp_newtemp();
        emit(AS_Oper(FormatString("jal %s\n", Temp_labelstring(e->u.CALL.fun->u.NAME)), NULL, NULL, NULL));
        emit(AS_Oper("nop\n", NULL, NULL, NULL));
        return d;
    }
    default:
        assert(0);
    }
}

AS_instrList F_codegen(F_frame f, T_stmList stmList) {
    T_stmList sl;
    AS_instrList il;
    for (sl = stmList; sl; sl = sl->tail) {
        munchStm(sl->head);
    }
    il = iList;
    iList = last = NULL;
    return il;
}