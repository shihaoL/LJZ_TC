#include <stdio.h>
#include <stdlib.h>
#include <regalloc.h>
#include "frame.h"
#include "canon.h"
#include "util.h"
#include "errormsg.h"
#include "absyn.h"
#include "semant.h"
#include "frame.h"
#include "assem.h"
#include "codegen.h"
#include "flowgraph.h"
#include "liveness.h"
#include "color.h"


extern int yyparse(void);

extern A_exp absyn_root;

#ifdef _DEBUG

#include "temp.h"

static void pr_tree_exp(FILE *out, T_exp exp, int d);

static void indent(FILE *out, int d) {
    int i;
    for (i = 0; i <= d; i++)
        fprintf(out, " ");
}

static char bin_oper[][12] = {
        "PLUS", "MINUS", "TIMES", "DIVIDE",
        "AND", "OR", "LSHIFT", "RSHIFT", "ARSHIFT", "XOR"
};

static char rel_oper[][12] = {
        "EQ", "NE", "LT", "GT", "LE", "GE", "ULT", "ULE", "UGT", "UGE"
};

static void pr_stm(FILE *out, T_stm stm, int d) {
    switch (stm->kind) {
        case T_SEQ:
            indent(out, d);
            fprintf(out, "SEQ(\n");
            pr_stm(out, stm->u.SEQ.left, d + 1);
            fprintf(out, ",\n");
            pr_stm(out, stm->u.SEQ.right, d + 1);
            fprintf(out, ")");
            break;
        case T_LABEL:
            indent(out, d);
            fprintf(out, "LABEL %s", S_name(stm->u.LABEL));
            break;
        case T_JUMP:
            indent(out, d);
            fprintf(out, "JUMP(\n");
            pr_tree_exp(out, stm->u.JUMP.exp, d + 1);
            fprintf(out, ")");
            break;
        case T_CJUMP:
            indent(out, d);
            fprintf(out, "CJUMP(%s,\n", rel_oper[stm->u.CJUMP.op]);
            pr_tree_exp(out, stm->u.CJUMP.left, d + 1);
            fprintf(out, ",\n");
            pr_tree_exp(out, stm->u.CJUMP.right, d + 1);
            fprintf(out, ",\n");
            indent(out, d + 1);
            fprintf(out, "%s,", S_name(stm->u.CJUMP.true));
            fprintf(out, "%s", S_name(stm->u.CJUMP.false));
            fprintf(out, ")");
            break;
        case T_MOVE:
            indent(out, d);
            fprintf(out, "MOVE(\n");
            pr_tree_exp(out, stm->u.MOVE.dst, d + 1);
            fprintf(out, ",\n");
            pr_tree_exp(out, stm->u.MOVE.src, d + 1);
            fprintf(out, ")");
            break;
        case T_EXP:
            indent(out, d);
            fprintf(out, "EXP(\n");
            pr_tree_exp(out, stm->u.EXP, d + 1);
            fprintf(out, ")");
            break;
    }
}

static void pr_tree_exp(FILE *out, T_exp exp, int d) {
    switch (exp->kind) {
        case T_BINOP:
            indent(out, d);
            fprintf(out, "BINOP(%s,\n", bin_oper[exp->u.BINOP.op]);
            pr_tree_exp(out, exp->u.BINOP.left, d + 1);
            fprintf(out, ",\n");
            pr_tree_exp(out, exp->u.BINOP.right, d + 1);
            fprintf(out, ")");
            break;
        case T_MEM:
            indent(out, d);
            fprintf(out, "MEM");
            fprintf(out, "(\n");
            pr_tree_exp(out, exp->u.MEM, d + 1);
            fprintf(out, ")");
            break;
        case T_TEMP:
            indent(out, d);
            fprintf(out, "TEMP t%d", getTmpnum(exp->u.TEMP));
            break;
        case T_ESEQ:
            indent(out, d);
            fprintf(out, "ESEQ(\n");
            pr_stm(out, exp->u.ESEQ.stm, d + 1);
            fprintf(out, ",\n");
            pr_tree_exp(out, exp->u.ESEQ.exp, d + 1);
            fprintf(out, ")");
            break;
        case T_NAME:
            indent(out, d);
            fprintf(out, "NAME %s", S_name(exp->u.NAME));
            break;
        case T_CONST:
            indent(out, d);
            fprintf(out, "CONST %d", exp->u.CONST);
            break;
        case T_CALL: {
            T_expList args = exp->u.CALL.args;
            indent(out, d);
            fprintf(out, "CALL(\n");
            pr_tree_exp(out, exp->u.CALL.fun, d + 1);
            for (; args; args = args->tail) {
                fprintf(out, ",\n");
                pr_tree_exp(out, args->head, d + 2);
            }
            fprintf(out, ")");
            break;
        }
    } /* end of switch */
}

void printStmList(FILE *out, T_stmList stmList) {
    for (; stmList; stmList = stmList->tail) {
        pr_stm(out, stmList->head, 0);
        fprintf(out, "\n\n\n");
    }
}

void show_nodeinfo(FILE *out, void *info) {
    Temp_map m = F_get_tempmap();
    string name = Temp_look(m, (Temp_temp) info);
    if (name == NULL)
        fprintf(out, "t%d\t", getTmpnum((Temp_temp) info));
    else fprintf(out, "%s\t", name);

}

void doProc(FILE *file, FILE *assemFile, F_frame frame, T_stm stm) {
    T_stmList stmList = C_linearize(stm);

    FILE *linearBlockFile = fopen("linear_tree.txt", "w");
    printStmList(linearBlockFile, stmList);
    fclose(linearBlockFile);

    struct C_block block = C_basicBlocks(stmList);
    Temp_label done = block.label;
    T_stmList tracedStmList = C_traceSchedule(block);
//    printStmList(file, tracedStmList);

    /***������һ������***/
    AS_instrList instrList = F_codegen(frame, tracedStmList);
    instrList = F_progEntryExit2(instrList,done);
    AS_proc proc = F_progEntryExit3(frame, instrList);
    /******/
    /***�ϲ���һ����ӡ����***/
    fprintf(file, "%s", proc->prolog);
    AS_printInstrList(file, instrList, F_get_tempmap());
    fprintf(file, "%s", proc->epilog);
    /******/
    fprintf(file, "\n\n\nliveness\n\n");
    //fclose(file);
    G_graph flowgraph = FG_AssemFlowGraph(instrList);
    struct Live_graph lg = Live_liveness(flowgraph);
    G_show(file, G_nodes(lg.graph), show_nodeinfo);
    Live_moveList livelist = lg.moves;
    for (; livelist; livelist = livelist->tail) {
        show_nodeinfo(file, G_nodeInfo(livelist->dst));
        fprintf(file, "\t");
        show_nodeinfo(file, G_nodeInfo(livelist->src));
        fprintf(file, "\n");
    }

    // coloring
    struct RA_result ra_result = RA_regAlloc(frame, instrList);
    fprintf(assemFile,".text\n.align 2\n.globl %s\n",Temp_labelstring(F_name(frame)));
    fprintf(assemFile, "%s", proc->prolog);
    AS_printInstrList(assemFile, instrList, ra_result.coloring);
    fprintf(assemFile, "%s", proc->epilog);
	fprintf(assemFile, "\n\n\n\n");
}

#endif // _DEBUG

void parse(string fname) {
    EM_reset(fname);
    if (yyparse() == 0) /* parsing worked */
    {
        fprintf(stderr, "Parsing successful!\n");
#ifdef _DEBUG
        F_fragList res = SEM_transProg(absyn_root);
        F_fragList tmp = res;
        T_stmList show = NULL;
        for (; tmp; tmp = tmp->tail) {
            if (tmp->head->kind == F_progFrag) {
                show = T_StmList(tmp->head->u.prog.body, show);
            }
        }
        FILE *fp = fopen("ir_tree.txt", "w");
        printStmList(fp, show);
        fclose(fp);

        //FILE *fp2 = fopen("canon_tree.txt", "w");

        FILE *instrFp = fopen("instr_b4_allocation.txt", "w");
        FILE *assemFile = fopen("tigerMainL.s", "w");
		tmp = res;
		fprintf(assemFile, ".data\n");
		for (; tmp; tmp = tmp->tail) {
			if (tmp->head->kind == F_stringFrag) {
                fprintf(assemFile,".align 2\n");
				fprintf(assemFile, "%s", F_string(tmp->head));
			}
		}
		//fprintf(assemFile, ".text\n.globl tigerMain\n");
        tmp = res;
        for (; tmp; tmp = tmp->tail)
            if (tmp->head->kind == F_progFrag)
                doProc(instrFp, assemFile, tmp->head->u.prog.frame, tmp->head->u.prog.body);
        //            else if (tmp->head->kind == F_stringFrag)
        //                fprintf(fp2, "%s\n", tmp->head->u.stringg.str);
        //fclose(fp2);
        fclose(instrFp);
        fclose(assemFile);


#endif // _DEBUG
    } else {
        fprintf(stderr, "Parsing failed\n");
    }
}


int main(int argc, char **argv) {
    /*if (argc != 2)
    {
      fprintf(stderr, "usage: a.out filename\n");
      exit(1);
    }*/
//  parse("testcases/queens.tig");
    //parse("customtests/func.tig");
    //parse("customtests/cjump.tig");
    //parse("testcases/test1.tig"); 
    parse("customtests/isprime.tig");
    printf("Done//:~");
    return 0;
}
