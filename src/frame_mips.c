#include "frame.h"

#define F_WORDSIZE 4
struct F_access_ {
	enum {inFrame,inReg} kind;
	union {
		int offset;
		Temp_temp reg;
	} u;
};

struct F_frame_ {
	Temp_label name;
	unsigned int frame_size;
	unsigned int max_argnum;
	F_accessList formals;
	/*��ʱ�����ӽ�λ��*/
};

void F_set_maxarg(F_frame f, int n) {
	if(n>f->max_argnum)
	f->max_argnum = n;
}

static F_access InFrame(int offset) {
	F_access tmp = checked_malloc(sizeof(*tmp));
	tmp->kind = inFrame;
	tmp->u.offset = offset;
	return tmp;
}
static F_access InReg(Temp_temp reg) {
	F_access tmp = checked_malloc(sizeof(*tmp));
	tmp->kind = inReg;
	tmp->u.reg = reg;
	return tmp;
}

F_accessList make_formals_list(F_frame f, U_boolList formals) {
	F_accessList ret = NULL,p=NULL;
	U_boolList tf = formals;
	int count = 0;
	for (; tf; tf = tf->tail,count++) {
		F_access a;
		if (tf->head) {
			a = InFrame(count*F_WORDSIZE);
		}
		else {
			a = InReg(Temp_newtemp());
		}
		if (ret == NULL) {
			ret = F_AccessList(a, NULL);
			p = ret;
		}
		else {
			p->tail = F_AccessList(a, NULL);
			p = p->tail;
		}
	}
	return ret;
}

F_frame F_newFrame(Temp_label name, U_boolList formals) {
	F_frame f = checked_malloc(sizeof(*f));
	f->name = name;
	f->formals = make_formals_list(f, formals);
	f->frame_size = 0;
	f->max_argnum = 0;
	return f;
}

Temp_label F_name(F_frame f) {
	return f->name;
}

F_accessList F_formals(F_frame f) {
	return f->formals;
}

F_access F_allocLocal(F_frame f, bool escape) {
	if (escape) {
		f->frame_size++;
		return InFrame(-F_WORDSIZE * (f->frame_size));
	}
	else {
		return InReg(Temp_newtemp());
	}
}

F_accessList F_AccessList(F_access head, F_accessList tail) {
	F_accessList tmp = checked_malloc(sizeof(*tmp));
	tmp->head = head;
	tmp->tail = tail;
	return tmp;
}



T_exp F_Exp(F_access acc, T_exp frameptr) {
	if (acc->kind == inFrame) {
		return T_Mem(T_Binop(T_plus, frameptr, T_Const(acc->u.offset)));
	}
	else {
		return T_Temp(acc->u.reg);
	}
}

int get_wordsize(void) {
	return F_WORDSIZE;
}

T_exp F_externalCall(string s, T_expList args) {
	return T_Call(T_Name(Temp_namedlabel(s)),args);
}


F_frag F_StringFrag(Temp_label label, string str) {
	F_frag tmp = checked_malloc(sizeof(*tmp));
	tmp->kind = F_stringFrag;
	tmp->u.stringg.label = label;
	tmp->u.stringg.str = str;
	return tmp;
}

F_frag F_ProcFrag(T_stm body, F_frame frame) {
	F_frag tmp = checked_malloc(sizeof(*tmp));
	tmp->kind = F_progFrag;
	tmp->u.prog.body = body;
	tmp->u.prog.frame = frame;
	return tmp;
}

F_fragList F_FragList(F_frag head, F_fragList tail) {
	F_fragList tmp = checked_malloc(sizeof(*tmp));
	tmp->head = head;
	tmp->tail = tail;
	return tmp;
}

static Temp_temp rv = NULL;
Temp_temp F_RV(void) {
	if (!rv) rv = Temp_newtemp();
	return rv;
}

static Temp_temp sp = NULL;
Temp_temp F_SP(void) {
	if (!sp) sp = Temp_newtemp();
	return sp;
}

static Temp_temp fp = NULL;
Temp_temp F_FP(void) {
	if (!fp) fp = Temp_newtemp();
	return fp;
}

static Temp_temp ra = NULL;
Temp_temp F_RA(void) {
	if (!ra) ra = Temp_newtemp();
	return ra;
}

static Temp_temp zero = NULL;
Temp_temp F_ZERO(void) {
	if (!zero) zero = Temp_newtemp();
	return zero;
}

static Temp_map tempMap = NULL;

static Temp_map F_get_tempmap_() {
	if (!tempMap) {
		tempMap = Temp_empty();
	}
	return tempMap;
}

static Temp_tempList specialregs = NULL;
static Temp_tempList argregs = NULL;
static Temp_tempList calleesaves = NULL;
static Temp_tempList callersaves = NULL;
static Temp_tempList registers = NULL;

Temp_tempList F_specialregs(void) {
	if (specialregs==NULL) {
		specialregs =
			Temp_TempList(F_FP(),
				Temp_TempList(F_SP(),
					Temp_TempList(F_RV(),
						Temp_TempList(F_RA(),
							Temp_TempList(F_ZERO(), NULL)))));
		Temp_map m = F_get_tempmap_();
		Temp_enter(m, F_FP(), String("$fp"));
		Temp_enter(m, F_SP(), String("$sp"));
		Temp_enter(m, F_RV(), String("$v0"));
		Temp_enter(m, F_RA(), String("$ra"));
		Temp_enter(m, F_ZERO(), String("$zero"));
	}
	return specialregs;
}



Temp_tempList F_argregs(void) {
	if (argregs==NULL) {
		Temp_temp a0 = Temp_newtemp();
		Temp_temp a1 = Temp_newtemp();
		Temp_temp a2 = Temp_newtemp();
		Temp_temp a3 = Temp_newtemp();
		argregs =
			Temp_TempList(a3,
				Temp_TempList(a2,
					Temp_TempList(a1,
						Temp_TempList(a0, NULL))));
		Temp_map m = F_get_tempmap_();
		Temp_enter(m, a0, String("$a0"));
		Temp_enter(m, a1, String("$a1"));
		Temp_enter(m, a2, String("$a2"));
		Temp_enter(m, a3, String("$a3"));
	}
	return argregs;
}


Temp_tempList F_calleesaves(void) {
	if (calleesaves==NULL) {
		Temp_temp t[10];
		int i;
		for (i = 0; i < 10; i++) t[i] = Temp_newtemp();
		for (i = 9; i >= 0; i--) calleesaves = Temp_TempList(t[i], calleesaves);
		Temp_map m = F_get_tempmap_();
		Temp_enter(m, t[0], String("$t0"));
		Temp_enter(m, t[1], String("$t1"));
		Temp_enter(m, t[2], String( "$t2"));
		Temp_enter(m, t[3], String("$t3"));
		Temp_enter(m, t[4], String("$t4"));
		Temp_enter(m, t[5], String("$t5"));
		Temp_enter(m, t[6], String("$t6"));
		Temp_enter(m, t[7], String("$t7"));
		Temp_enter(m, t[8], String("$t8"));
		Temp_enter(m, t[9], String("$t9"));
	}
	return calleesaves;
}


Temp_tempList F_callersaves(void) {
	if (callersaves==NULL) {
		Temp_temp s[8];
		int i;
		for (i = 0; i < 8; i++) s[i] = Temp_newtemp();
		for (i = 7; i >= 0; i--) callersaves = Temp_TempList(s[i], callersaves);
		Temp_map m = F_get_tempmap_();
		Temp_enter(m, s[0], String("$s0"));
		Temp_enter(m, s[1], String("$s1"));
		Temp_enter(m, s[2], String("$s2"));
		Temp_enter(m, s[3], String("$s3"));
		Temp_enter(m, s[4], String("$s4"));
		Temp_enter(m, s[5], String("$s5"));
		Temp_enter(m, s[6], String("$s6"));
		Temp_enter(m, s[7], String("$s7"));
	}
	return callersaves;
}


Temp_tempList F_registers(void) {
	if (!registers) {
		Temp_tempList special = F_specialregs();
		Temp_tempList arg = F_argregs();
		Temp_tempList callee = F_calleesaves();
		Temp_tempList caller = F_callersaves();
		registers = NULL;
		Temp_tempList tp = NULL;
		for (; special; special = special->tail) {
			if (registers == NULL) {
				registers = Temp_TempList(special->head, NULL);
				tp = registers;
			}
			else {
				tp->tail = Temp_TempList(special->head, NULL);
				tp = tp->tail;
			}
		}
		for (; arg; arg = arg->tail) {
			tp->tail = Temp_TempList(arg->head, NULL);
			tp = tp->tail;
		}
		for (; callee; callee = callee->tail) {
			tp->tail = Temp_TempList(callee->head, NULL);
			tp = tp->tail;
		}
		for (; caller; caller =caller->tail) {
			tp->tail = Temp_TempList(caller->head, NULL);
			tp = tp->tail;
		}
	}
	return registers;
}


Temp_map F_get_tempmap() {
	static int flag = 0;
	Temp_map ret = F_get_tempmap_();
	if (flag == 0) {
		F_registers();
		flag = 1;
	}
	return ret;
}


T_stm F_progEntryExit1(F_frame frame, T_stm stm);

static Temp_tempList returnSink = NULL;
AS_instrList F_progEntryExit2(AS_instrList body) {
	if (!returnSink) returnSink =
		Temp_TempList(F_ZERO(), Temp_TempList(F_RA(), Temp_TempList(F_SP(), F_calleesaves())));
	
	return AS_splice(body, AS_InstrList(AS_Oper("", NULL, returnSink, NULL), NULL));
}
AS_proc F_progEntryExit3(F_frame frame, AS_instrList body);