#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "table.h"
#include "tree.h"
#include "frame.h"

/*Lab5: Your implementation here.*/
const int F_wordSize = 8;
//Frame
struct F_frame_ {
	Temp_label label;
	F_accessList formalList;
	F_accessList localList;
	int current_size;
	T_stm view_shift;
};

//varibales
struct F_access_ {
	enum {inFrame, inReg} kind;
	union {
		int offset; //inFrame
		Temp_temp reg; //inReg
	} u;
};

static F_access InFrame(int offset);
static F_access InReg(Temp_temp reg);

Temp_temp F_FP(void){
	static Temp_temp fp = NULL;
	if(!fp){
		fp = Temp_newtemp();
	}
	return fp;
}

Temp_temp F_SP(void){
	static Temp_temp fp = NULL;
	if(!fp){
		fp = Temp_newtemp();
	}
	return fp;
}

Temp_temp F_RV(void){
	static Temp_temp fp = NULL;
	if(!fp){
		fp = Temp_newtemp();
	}
	return fp;
}

F_access InFrame(int offset){
	F_access frameVar = (F_access) checked_malloc(sizeof(*frameVar));
	frameVar->kind = inFrame;
	frameVar->u.offset = offset;
	return frameVar;
}

F_access InReg(Temp_temp reg){
	F_access regVar = (F_access) checked_malloc(sizeof(*regVar));
	regVar->kind = inReg;
	regVar->u.reg = reg;
	return regVar;
}

F_accessList F_AccessList(F_access head, F_accessList tail){
	F_accessList f_accessList = (F_accessList) checked_malloc(sizeof(*f_accessList));
	f_accessList->head = head;
	f_accessList->tail = tail;
	return f_accessList;
}

F_access F_allocLocal(F_frame f, bool escape){
	F_accessList* temp_accessList = &(f->localList);
	while(*temp_accessList){
		temp_accessList = &((*temp_accessList)->tail);
	}
	if(escape){
		f->current_size += F_wordSize;
		F_access frameVar = InFrame(-f->current_size);
		*temp_accessList = F_AccessList(frameVar, NULL);
		return frameVar;
	}else{
		F_access regVar = InReg(Temp_newtemp());
		*temp_accessList = F_AccessList(regVar, NULL);
		return regVar;
	}
}

F_accessList F_formals(F_frame f){
	return f->formalList;
}

F_frame F_newFrame(Temp_label name, U_boolList formals){
	F_frame frame = (F_frame) checked_malloc(sizeof(*frame));
	frame->label = name;
	frame->current_size = 0;
	frame->localList = NULL;
	//Handle static link first
	frame->formalList = F_AccessList(InFrame(0), NULL);
	F_accessList* temp_accessList = &(frame->formalList->tail);
	int offset = 0;
	while(formals->tail){
		//argument in memory
		if(formals->tail->head){
			*temp_accessList = F_AccessList(InReg(Temp_newtemp()), NULL);
		}else{
			*temp_accessList = F_AccessList(InFrame((2+offset)*F_wordSize), NULL);
		}
		offset++;
		formals = formals->tail;
		temp_accessList = &((*temp_accessList)->tail);
	}
	return frame;
}

// Must handle
// - Underscore function
// - different calling conventions(such as no static link)
T_exp F_externalCall(string s, T_expList args){
	return T_Call(T_Name(Temp_namedlabel(s)), args);
}

T_exp F_Exp(F_access access, T_exp framePtr){
	if(access->kind == inReg){
		return T_Temp(access->u.reg);
	}else{
		return T_Mem(T_Binop(T_plus, framePtr, T_Const(access->u.offset)));
	}
}

F_frag F_StringFrag(Temp_label label, string str) {
	F_frag frag = (F_frag) checked_malloc(sizeof(*frag));
	frag->kind = F_stringFrag;
	frag->u.stringg.label = label;
	frag->u.stringg.str = str;
	return frag;                                      
}                                                     
                                                      
F_frag F_ProcFrag(T_stm body, F_frame frame) {        
	F_frag frag = (F_frag) checked_malloc(sizeof(*frag));
	frag->kind = F_procFrag;
	frag->u.proc.body = body;
	frag->u.proc.frame = frame;
	return frag;                                     
}                                                     
                                                      
F_fragList F_FragList(F_frag head, F_fragList tail) { 
	F_fragList fraglist = (F_fragList) checked_malloc(sizeof(*fraglist));
	fraglist->head = head;
	fraglist->tail = tail;
	return fraglist;                                      
}               

T_stm F_procEntryExit1(F_frame frame, T_stm stm){
	return stm;
}

