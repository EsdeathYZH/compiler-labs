#ifndef TRANSLATE_H
#define TRANSLATE_H

#include "util.h"
#include "absyn.h"
#include "temp.h"
#include "frame.h"

/* Lab5: your code below */

typedef struct Tr_exp_ *Tr_exp;

typedef struct Tr_expList_ *Tr_expList;

typedef struct Tr_access_ *Tr_access;

typedef struct Tr_accessList_ *Tr_accessList;

typedef struct Tr_level_ *Tr_level;

typedef struct patchList_ *patchList;

struct Tr_access_ {
	Tr_level level;
	F_access access;
};


struct Tr_accessList_ {
	Tr_access head;
	Tr_accessList tail;	
};

struct Tr_level_ {
	F_frame frame;
	Tr_level parent;
};

struct patchList_ 
{
	Temp_label *head; 
	patchList tail;
};

struct Cx 
{
	patchList trues; 
	patchList falses; 
	T_stm stm;
};

struct Tr_exp_ {
	enum {Tr_ex, Tr_nx, Tr_cx} kind;
	union {T_exp ex; T_stm nx; struct Cx cx; } u;
};

struct Tr_expList_ {
	Tr_exp head;
	Tr_expList tail;
};

Tr_access Tr_Access(Tr_level level, F_access access);

Tr_accessList Tr_AccessList(Tr_access head, Tr_accessList tail);

Tr_expList Tr_ExpList(Tr_exp head, Tr_expList tail);

Tr_level Tr_outermost(void);

Tr_level Tr_newLevel(Tr_level parent, Temp_label name, U_boolList formals);

Tr_accessList Tr_formals(Tr_level level);

Tr_access Tr_allocLocal(Tr_level level, bool escape);

void doPatch(patchList tList, Temp_label label);

Tr_accessList convertFListToTrList(F_accessList, Tr_level);

Tr_exp Tr_int(int num);

Tr_exp Tr_string(string str);

Tr_exp Tr_simpleVar(Tr_access, Tr_level);

Tr_exp Tr_subscriptVar(Tr_exp array, Tr_exp subscript, int element_size);

Tr_exp Tr_fieldVar(Tr_exp rec, int offset);

Tr_exp Tr_arithmatic(A_oper op, Tr_exp left, Tr_exp right);

Tr_exp Tr_condition(A_oper op, Tr_exp left, Tr_exp right);

Tr_exp Tr_recordExp(int size, Tr_expList tr_expList);

Tr_exp Tr_arrayExp(Tr_exp size_exp, Tr_exp init_exp);

Tr_exp Tr_if(Tr_exp test, Tr_exp then, Tr_exp elsee);

Tr_exp Tr_for(Tr_access loopVar, Tr_exp low, Tr_exp high, Tr_exp body_exp, Temp_label done);

Tr_exp Tr_while(Tr_exp test_exp, Tr_exp body_exp ,Temp_label done);

Tr_exp Tr_let(Tr_expList decExpList, Tr_exp body_exp);

Tr_exp Tr_functionCall(Tr_level current_level, Tr_level function_level, Temp_label func, Tr_expList args);

Tr_exp Tr_break(Temp_label loopLabel);

Tr_exp Tr_assign(Tr_exp lvalue, Tr_exp rvalue);

void Tr_procEntryExit(Tr_level level, Tr_exp body, Tr_accessList formals);

Tr_exp Empty_exp();

Tr_exp Initial_exp(Tr_access access, Tr_exp init_exp);

F_fragList Tr_getResult(void);
#endif
