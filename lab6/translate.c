#include <stdio.h>
#include "util.h"
#include "table.h"
#include "symbol.h"
#include "absyn.h"
#include "temp.h"
#include "tree.h"
#include "printtree.h"
#include "frame.h"
#include "translate.h"

//LAB5: you can modify anything you want.
//global fraglist
F_fragList globalFragList;

static Tr_exp Tr_Ex(T_exp ex);
static Tr_exp Tr_Nx(T_stm nx);
static Tr_exp Tr_Cx(patchList trues, patchList falses, T_stm stm);
static T_exp unEx(Tr_exp e);
static T_stm unNx(Tr_exp e);
static struct Cx unCx(Tr_exp e);
static patchList PatchList(Temp_label *head, patchList tail);
static void addFragToGlobalList(F_frag frag);

Tr_expList Tr_ExpList(Tr_exp head, Tr_expList tail){
	Tr_expList tr_expList = (Tr_expList) checked_malloc(sizeof(*tr_expList));
	tr_expList->head = head;
	tr_expList->tail = tail;
	return tr_expList;
}

static Tr_exp Tr_Ex(T_exp ex){
	Tr_exp tr_exp = (Tr_exp) checked_malloc(sizeof(*tr_exp));
	tr_exp->kind = Tr_ex;
	tr_exp->u.ex = ex;
	return tr_exp;
}

static Tr_exp Tr_Nx(T_stm nx){
	Tr_exp tr_exp = (Tr_exp) checked_malloc(sizeof(*tr_exp));
	tr_exp->kind = Tr_nx;
	tr_exp->u.nx = nx;
	return tr_exp;
}

static Tr_exp Tr_Cx(patchList trues, patchList falses, T_stm stm){
	Tr_exp tr_exp = (Tr_exp) checked_malloc(sizeof(*tr_exp));
	tr_exp->kind = Tr_cx;
	tr_exp->u.cx.trues = trues;
	tr_exp->u.cx.falses = falses;
	tr_exp->u.cx.stm = stm;
	return tr_exp;
}

static T_exp unEx(Tr_exp e){
	switch(e->kind){
		case Tr_ex:
			return e->u.ex;
		case Tr_cx:{
			Temp_temp r = Temp_newtemp();
			Temp_label t = Temp_newlabel(), f = Temp_newlabel();
			doPatch(e->u.cx.trues, t);
			doPatch(e->u.cx.falses, f);
			return T_Eseq(T_Move(T_Temp(r), T_Const(1)),
					T_Eseq(e->u.cx.stm,
					T_Eseq(T_Label(f),
					T_Eseq(T_Move(T_Temp(r), T_Const(0)),
					T_Eseq(T_Label(t),
							T_Temp(r))))));
		}
		case Tr_nx:
			return T_Eseq(e->u.nx, T_Const(0));
	}
	assert(0);
}

static T_stm unNx(Tr_exp e){
	switch(e->kind){
		case Tr_ex:
			return T_Exp(e->u.ex);
		case Tr_cx:{
			Temp_temp r = Temp_newtemp();
			Temp_label t = Temp_newlabel(), f = Temp_newlabel();
			doPatch(e->u.cx.trues, t);
			doPatch(e->u.cx.falses, f);
			return T_Exp(T_Eseq(T_Move(T_Temp(r), T_Const(1)),
					T_Eseq(e->u.cx.stm,
					T_Eseq(T_Label(f),
					T_Eseq(T_Move(T_Temp(r), T_Const(0)),
					T_Eseq(T_Label(t),
							T_Temp(r)))))));
		}
		case Tr_nx:
			return e->u.nx;
	}
	assert(0);
}

static struct Cx unCx(Tr_exp e){
	switch(e->kind){
		case Tr_ex:{
			struct Cx cx;
			if(e->u.ex->kind == T_CONST 
			  && e->u.ex->u.CONST == 0){
				cx.stm = T_Jump(T_Name(NULL), NULL);
				cx.trues = PatchList(NULL, NULL);
				cx.falses = PatchList(&cx.stm->u.JUMP.exp->u.NAME, NULL);
			}else if(e->u.ex->kind == T_CONST 
			  && e->u.ex->u.CONST == 1){
				cx.stm = T_Jump(T_Name(NULL), NULL);
				cx.trues = PatchList(&cx.stm->u.JUMP.exp->u.NAME, NULL);
				cx.falses = PatchList(NULL, NULL);
			}else{
				cx.stm = T_Cjump(T_ne, e->u.ex, T_Const(0), NULL, NULL);
				cx.trues = PatchList(&cx.stm->u.CJUMP.true, NULL);
				cx.falses = PatchList(&cx.stm->u.CJUMP.false, NULL);
			}
			return cx;
		}
		case Tr_cx:{
			return e->u.cx;
		}
		//impossible case
		case Tr_nx:{
			assert(0);
		}
	}
	assert(0);
}

static void addFragToGlobalList(F_frag frag){
	globalFragList = F_FragList(frag, globalFragList);
}

static patchList PatchList(Temp_label *head, patchList tail)
{
	patchList list;

	list = (patchList)checked_malloc(sizeof(struct patchList_));
	list->head = head;
	list->tail = tail;
	return list;
}

void doPatch(patchList tList, Temp_label label)
{
	for(; tList; tList = tList->tail)
		*(tList->head) = label;
}

patchList joinPatch(patchList first, patchList second)
{
	if(!first) return second;
	for(; first->tail; first = first->tail);
	first->tail = second;
	return first;
}

Tr_access Tr_Access(Tr_level level, F_access access){
	Tr_access tr_access = (Tr_access) checked_malloc(sizeof(*tr_access));
	tr_access->level = level;
	tr_access->access = access;
	return tr_access;
}

Tr_accessList Tr_AccessList(Tr_access head, Tr_accessList tail){
	Tr_accessList tr_accessList = (Tr_accessList) checked_malloc(sizeof(*tr_accessList));
	tr_accessList->head = head;
	tr_accessList->tail = tail;
	return tr_accessList;
}

Tr_level Tr_newLevel(Tr_level parent, Temp_label name, U_boolList formals){
	Tr_level newLevel = (Tr_level) checked_malloc(sizeof(*newLevel));
	newLevel->parent = parent;
	//add static link as first parameter
	newLevel->frame = F_newFrame(name, U_BoolList(TRUE, formals));
	return newLevel;
}

Tr_access Tr_allocLocal(Tr_level level, bool escape){
	Tr_access tr_access = (Tr_access) checked_malloc(sizeof(*tr_access));
	tr_access->level = level;
	tr_access->access = F_allocLocal(level->frame, escape);
	return tr_access;
}

Tr_accessList convertFListToTrList(F_accessList f_accessList, Tr_level level){
	if(f_accessList == NULL){
		return NULL;
	}
	Tr_access head = Tr_Access(level, f_accessList->head);
	Tr_accessList tr_accessList = Tr_AccessList(head, convertFListToTrList(f_accessList->tail, level));
	return tr_accessList;
}

Tr_accessList Tr_formals(Tr_level level){
	//skip static link
	F_accessList f_accessList =  F_formals(level->frame)->tail;
	return convertFListToTrList(f_accessList, level);
}

Tr_level Tr_outermost(void){
	static Tr_level outerLevel = NULL;
	if(outerLevel == NULL){
		outerLevel = (Tr_level) checked_malloc(sizeof(*outerLevel));
		outerLevel->frame = NULL;
		outerLevel->parent = NULL;
	}
	return outerLevel;
}

Tr_exp Tr_int(int num){
	return Tr_Ex(T_Const(num));
}

Tr_exp Tr_string(string str){
	Temp_label lab = Temp_newlabel();
	F_frag str_frag = F_StringFrag(lab, str);
	//put fragment onto a global list
	addFragToGlobalList(str_frag);
	return Tr_Ex(T_Name(lab));
} 

Tr_exp Tr_simpleVar(Tr_access access, Tr_level level){
	if(access->level == level){
		return Tr_Ex(F_Exp(access->access, T_Temp(F_FP())));
	}else{
		Tr_level temp_level = level;
		T_exp exp = T_Temp(F_FP());
		while(access->level != temp_level){
			exp = T_Mem(exp);
			temp_level = temp_level->parent;
		}
		return Tr_Ex(F_Exp(access->access, exp));
	}
}

Tr_exp Tr_fieldVar(Tr_exp rec, int offset){
	return Tr_Ex(T_Mem(
			T_Binop(T_plus, unEx(rec), 
			T_Binop(T_mul, T_Const(offset), T_Const(F_wordSize)))));
}

Tr_exp Tr_subscriptVar(Tr_exp array, Tr_exp subscript){
	return Tr_Ex(T_Mem(
			T_Binop(T_plus, unEx(array),
			T_Binop(T_mul, unEx(subscript), T_Const(F_wordSize)))));
}

Tr_exp Tr_arithmatic(A_oper op, Tr_exp left, Tr_exp right){
	switch(op){
		case A_plusOp:
			return Tr_Ex(T_Binop(T_plus, unEx(left), unEx(right)));
		case A_minusOp:
			return Tr_Ex(T_Binop(T_minus, unEx(left), unEx(right)));
		case A_timesOp:
			return Tr_Ex(T_Binop(T_mul, unEx(left), unEx(right)));
		case A_divideOp:
			return Tr_Ex(T_Binop(T_div, unEx(left), unEx(right)));
	}
	assert(0);
}

Tr_exp Tr_condition(A_oper op, Tr_exp left, Tr_exp right){
	T_stm stm = NULL;
	switch(op){
		case A_ltOp:{
			stm = T_Cjump(T_lt, unEx(left), unEx(right), NULL, NULL);
			break;
		}
		case A_leOp:{
			stm = T_Cjump(T_le, unEx(left), unEx(right), NULL, NULL);
			break;
		}
		case A_gtOp:{
			stm = T_Cjump(T_gt, unEx(left), unEx(right), NULL, NULL);
			break;
		}
		case A_geOp:{
			stm = T_Cjump(T_ge, unEx(left), unEx(right), NULL, NULL);
			break;
		}
		case A_eqOp:{
			stm = T_Cjump(T_eq, unEx(left), unEx(right), NULL, NULL);
			break;
		}
		case A_neqOp:{
			stm = T_Cjump(T_ne, unEx(left), unEx(right), NULL, NULL);
			break;
		}
		default:
			assert(0);
	}
	patchList trues = PatchList(&stm->u.CJUMP.true, NULL);
	patchList falses = PatchList(&stm->u.CJUMP.false, NULL);
	return Tr_Cx(trues, falses, stm);
}

Tr_exp Tr_if(Tr_exp test, Tr_exp then, Tr_exp elsee){
	if(elsee == NULL){
		//TODO:表示空语句是否应该用NULL？
		elsee = Tr_Nx(NULL);
	}
	if(then->kind == Tr_ex && elsee->kind == Tr_ex){
		Temp_temp result = Temp_newtemp();
		Temp_label true_label = Temp_newlabel();
		Temp_label false_label = Temp_newlabel();
		Temp_label merge_label = Temp_newlabel();
		struct Cx cx = unCx(test);
		doPatch(cx.trues, true_label);
		doPatch(cx.falses, false_label);
		return Tr_Ex(T_Eseq(T_Seq(cx.stm, 
						T_Seq(T_Label(true_label),
						T_Seq(T_Move(T_Temp(result), unEx(then)),
						T_Seq(T_Jump(T_Name(merge_label), NULL),
						T_Seq(T_Label(false_label),
						T_Seq(T_Move(T_Temp(result), unEx(elsee)),
						T_Label(merge_label))))))), 
					T_Temp(result)));
	}else if(then->kind == Tr_nx && elsee->kind == Tr_nx){
		Temp_label true_label = Temp_newlabel();
		Temp_label false_label = Temp_newlabel();
		Temp_label merge_label = Temp_newlabel();
		struct Cx cx = unCx(test);
		doPatch(cx.trues, true_label);
		doPatch(cx.falses, false_label);
		return Tr_Nx(T_Seq(cx.stm, 
				T_Seq(T_Label(true_label),
				T_Seq(unNx(then),
				T_Seq(T_Jump(T_Name(merge_label), NULL),
				T_Seq(T_Label(false_label),
				T_Seq(unNx(elsee),
				T_Label(merge_label))))))));
	}else{
		//这里暂时不知道怎么处理,先按照Ex来
		Temp_temp result = Temp_newtemp();
		Temp_label true_label = Temp_newlabel();
		Temp_label false_label = Temp_newlabel();
		Temp_label merge_label = Temp_newlabel();
		struct Cx cx = unCx(test);
		doPatch(cx.trues, true_label);
		doPatch(cx.falses, false_label);
		return Tr_Ex(T_Eseq(T_Seq(cx.stm, 
						T_Seq(T_Label(true_label),
						T_Seq(T_Move(T_Temp(result), unEx(then)),
						T_Seq(T_Jump(T_Name(merge_label), NULL),
						T_Seq(T_Label(false_label),
						T_Seq(T_Move(T_Temp(result), unEx(elsee)),
						T_Label(merge_label))))))), 
					T_Temp(result)));
	}
}

Tr_exp Tr_break(Temp_label loopLabel){
	return Tr_Nx(T_Jump(T_Name(loopLabel), NULL));
}

Tr_exp Tr_recordExp(int size, Tr_expList tr_expList){
	int record_size = size;
	Temp_temp record = Temp_newtemp();
	T_stm statement = T_Move(T_Mem(
		T_Binop(T_plus, T_Temp(record), T_Const((size-1)*F_wordSize))),
		unEx(tr_expList->head));
	size--;
	tr_expList = tr_expList->tail;
	while(tr_expList){
		if(size == 0) assert(0);
		statement = T_Seq(T_Move(T_Mem(
					T_Binop(T_plus, T_Temp(record), T_Const((size-1)*F_wordSize))),
					unEx(tr_expList->head)), statement);
		size--;
		tr_expList = tr_expList->tail;
	}
	T_exp callInitRecord = F_externalCall("malloc", T_ExpList(T_Const(record_size * F_wordSize), NULL));
	statement = T_Seq(T_Move(T_Temp(record),callInitRecord), statement);
	return Tr_Ex(T_Eseq(statement, T_Temp(record)));
}

Tr_exp Tr_arrayExp(Tr_exp size_exp, Tr_exp init_exp){
	Temp_temp array = Temp_newtemp();
	T_exp callInitArray = F_externalCall("initArray", T_ExpList(unEx(size_exp),T_ExpList(unEx(init_exp), NULL)));
	return Tr_Ex(T_Eseq(T_Move(T_Temp(array),callInitArray),
						 T_Temp(array)));
}

Tr_exp Tr_assign(Tr_exp lvalue, Tr_exp rvalue){
	return Tr_Ex(T_Eseq(T_Move(unEx(lvalue), unEx(rvalue)), unEx(lvalue)));
}

/*
test:
	if not(condition) goto done
	body
	goto test
done:
*/
Tr_exp Tr_while(Tr_exp test_exp, Tr_exp body_exp ,Temp_label done_label){
	Temp_label test_label = Temp_newlabel();
	Temp_label body_label = Temp_newlabel();
	return Tr_Nx(T_Seq(
		T_Label(test_label), T_Seq(
		T_Cjump(T_eq, unEx(test_exp), T_Const(0), done_label, body_label), T_Seq(
		T_Label(body_label), T_Seq(
		unNx(body_exp), T_Seq(
		T_Jump(T_Name(test_label), NULL), T_Label(done_label)
		))))));
}

/*
  if i > limit goto done
  body
  if i == limit goto done
Loop:	
	i := i + 1
	body
	if i <= limit goto Loop TODO: Here should be '<'?  
done:

*/
Tr_exp Tr_for(Tr_access loopVar, Tr_exp low, Tr_exp high, Tr_exp body_exp, Temp_label done_label){
	Temp_label loop_label = Temp_newlabel();
	Temp_label body_label = Temp_newlabel();
	T_exp loop_exp = F_Exp(loopVar->access, T_Temp(F_FP()));
	return Tr_Nx(T_Seq(
			T_Move(loop_exp, unEx(low)), T_Seq(
			T_Cjump(T_gt, loop_exp, unEx(high), done_label, body_label), T_Seq(
			T_Label(body_label), T_Seq(
			unNx(body_exp), T_Seq(
			T_Cjump(T_eq, loop_exp, unEx(high), done_label, loop_label), T_Seq(
			T_Label(loop_label), T_Seq(
			T_Move(loop_exp, T_Binop(T_plus, loop_exp, T_Const(1))), T_Seq(
			unNx(body_exp), T_Seq(
			T_Cjump(T_lt, loop_exp, unEx(high), loop_label, done_label),
			T_Label(done_label)))))))))));
}

//Just like translate a series of exps, and translate the last exp 
Tr_exp Tr_let(Tr_expList decExpList, Tr_exp body_exp){
	if(!decExpList){
		return body_exp;
	}
	T_stm decStm = unNx(decExpList->head);
	decExpList = decExpList->tail;
	while(decExpList){
		decStm = T_Seq(unNx(decExpList->head), decStm);
		decExpList = decExpList->tail;
	}
	return Tr_Ex(T_Eseq(decStm, unEx(body_exp)));
}

Tr_exp Tr_functionCall(Tr_level current_level, Tr_level function_level, Temp_label func, Tr_expList args){
	T_exp static_link = T_Temp(F_FP());
	while(current_level != function_level->parent){
		static_link = T_Mem(static_link);
		current_level = current_level->parent;
		//if(current_level == NULL) assert(0);
	}
	T_expList argsExpList = NULL;
	while(args){
		argsExpList = T_ExpList(unEx(args->head), argsExpList);
		args = args->tail;
	}
	//add static link as first implicit parameter
	argsExpList = T_ExpList(static_link, argsExpList);
	return Tr_Ex(T_Call(T_Name(func), argsExpList));
}

//for transVarDec
Tr_exp Initial_exp(Tr_access access, Tr_exp init_exp){
	return Tr_Nx(T_Move(
			F_Exp(access->access, T_Temp(F_FP())), 
			unEx(init_exp)));
}

//for transTypeDec & transFuncDec
Tr_exp Empty_exp(){
	return Tr_Ex(T_Const(0));
}

void Tr_procEntryExit(Tr_level level, Tr_exp body, Tr_accessList formals){
	//transmit body value to RV register
	T_stm final_stm = T_Move(T_Temp(F_RV()), unEx(body));
	addFragToGlobalList(F_ProcFrag(F_procEntryExit1(level->frame, final_stm), level->frame));
}

// get global fragment list
F_fragList Tr_getResult(void){ 
	return globalFragList;
}