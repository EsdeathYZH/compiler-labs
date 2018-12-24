#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "table.h"
#include "tree.h"
#include "frame.h"
#include "assem.h"

/*Lab5: Your implementation here.*/
const int F_wordSize = 8;
Temp_map specialregs = NULL;
Temp_map argregs = NULL;
Temp_map calleesaves = NULL;
Temp_map callersaves = NULL;

static F_access InFrame(int offset);
static F_access InReg(Temp_temp reg);

Temp_temp F_RSP(void);
Temp_temp F_RAX(void);
Temp_temp F_RBP(void);
Temp_temp F_RBX(void);
Temp_temp F_RDI(void);
Temp_temp F_RSI(void);
Temp_temp F_RDX(void);
Temp_temp F_RCX(void);
Temp_temp F_R8(void);
Temp_temp F_R9(void);
Temp_temp F_R10(void);
Temp_temp F_R11(void);
Temp_temp F_R12(void);
Temp_temp F_R13(void);
Temp_temp F_R14(void);
Temp_temp F_R15(void);

Temp_temp F_RV(void){
	return F_RAX();
}

Temp_temp F_FP(void){
	return F_RSP();
}

Temp_temp F_SP(void){
	return F_RSP();
}

Temp_temp F_RSP(void){
	static Temp_temp rsp = NULL;
	if(!rsp){
		rsp = Temp_newtemp();
	}
	return rsp;
}

Temp_temp F_RAX(void){
	static Temp_temp rax = NULL;
	if(!rax){
		rax = Temp_newtemp();
	}
	return rax;
}

Temp_temp F_RBP(void){
	static Temp_temp rbp = NULL;
	if(!rbp){
		rbp = Temp_newtemp();
	}
	return rbp;
}

Temp_temp F_RBX(void){
	static Temp_temp rbx = NULL;
	if(!rbx){
		rbx = Temp_newtemp();
	}
	return rbx;
}

Temp_temp F_RDI(void){
	static Temp_temp rdi = NULL;
	if(!rdi){
		rdi = Temp_newtemp();
	}
	return rdi;
}

Temp_temp F_RSI(void){
	static Temp_temp rsi = NULL;
	if(!rsi){
		rsi = Temp_newtemp();
	}
	return rsi;
}

Temp_temp F_RDX(void){
	static Temp_temp rdx = NULL;
	if(!rdx){
		rdx = Temp_newtemp();
	}
	return rdx;
}

Temp_temp F_RCX(void){
	static Temp_temp rcx = NULL;
	if(!rcx){
		rcx = Temp_newtemp();
	}
	return rcx;
}

Temp_temp F_R8(void){
	static Temp_temp r8 = NULL;
	if(!r8){
		r8 = Temp_newtemp();
	}
	return r8;
}

Temp_temp F_R9(void){
	static Temp_temp r9 = NULL;
	if(!r9){
		r9 = Temp_newtemp();
	}
	return r9;
}

Temp_temp F_R10(void){
	static Temp_temp r10 = NULL;
	if(!r10){
		r10 = Temp_newtemp();
	}
	return r10;
}

Temp_temp F_R11(void){
	static Temp_temp r11 = NULL;
	if(!r11){
		r11 = Temp_newtemp();
	}
	return r11;
}

Temp_temp F_R12(void){
	static Temp_temp r12 = NULL;
	if(!r12){
		r12 = Temp_newtemp();
	}
	return r12;
}

Temp_temp F_R13(void){
	static Temp_temp r13 = NULL;
	if(!r13){
		r13 = Temp_newtemp();
	}
	return r13;
}

Temp_temp F_R14(void){
	static Temp_temp r14 = NULL;
	if(!r14){
		r14 = Temp_newtemp();
	}
	return r14;
}

Temp_temp F_R15(void){
	static Temp_temp r15 = NULL;
	if(!r15){
		r15 = Temp_newtemp();
	}
	return r15;
}

//%rax %rsp %rbp
Temp_tempList specialRegs(){
	static Temp_tempList regs = NULL;
    if(regs == NULL){
        regs = Temp_TempList(F_SP(), Temp_TempList(F_RV(), Temp_TempList(F_FP(), NULL)));
    }
    return regs;
}

//%rdi，%rsi，%rdx，%rcx，%r8，%r9
Temp_tempList argRegs(){
    static Temp_tempList regs = NULL;
    if(regs == NULL){
        regs = Temp_TempList(F_RDI(), Temp_TempList(F_RSI(), Temp_TempList(F_RDX(), 
				Temp_TempList(F_RCX(), Temp_TempList(F_R8(), Temp_TempList(F_R9(), NULL))))));
    }
    return regs;
}

//%rbx, %rbp, %r12, %r13, %r14, %r15
Temp_tempList calleeSaves(){
    static Temp_tempList regs = NULL;
    if(regs == NULL){
        regs = Temp_TempList(F_RBX(), Temp_TempList(F_RBP(), Temp_TempList(F_R12(), 
				Temp_TempList(F_R13(), Temp_TempList(F_R14(), Temp_TempList(F_R15(), NULL))))));
    }
    return regs;
}

//%r10, %r11
Temp_tempList callerSaves(){
    static Temp_tempList regs = NULL;
    if(regs == NULL){
        regs = Temp_TempList(F_R10(), Temp_TempList(F_R11(), NULL));
    }
    return regs;
}

Temp_tempList F_registers(){
	static Temp_tempList registers = NULL;
	if(registers == NULL){
		registers = Temp_TempList(F_RV(), Temp_TempList(F_RDI(), Temp_TempList(F_RSI(), Temp_TempList(F_RDX(), 
			Temp_TempList(F_RCX(), Temp_TempList(F_R8(), Temp_TempList(F_R9(), Temp_TempList(F_RBX(), Temp_TempList(F_RBP(), Temp_TempList(F_R12(), 
			Temp_TempList(F_R13(), Temp_TempList(F_R14(), Temp_TempList(F_R15(), Temp_TempList(F_R10(), Temp_TempList(F_R11(), Temp_TempList(F_SP(), NULL))))))))))))))));
	}
	return registers;
}

void Init_F_TempMap(){
	//TODO:rbp???
	//DONE: use one temp to provide rbp and fp
	//specialregs
	Temp_enter(F_tempMap, F_FP(), String("%rbp"));
	Temp_enter(F_tempMap, F_SP(), String("%rsp"));
	Temp_enter(F_tempMap, F_RV(), String("%rax"));

	//argRegs:%rdi，%rsi，%rdx，%rcx，%r8，%r9
	Temp_enter(F_tempMap, F_RDI(), String("%rdi"));
	Temp_enter(F_tempMap, F_RSI(), String("%rsi"));
	Temp_enter(F_tempMap, F_RDX(), String("%rdx"));
	Temp_enter(F_tempMap, F_RCX(), String("%rcx"));
	Temp_enter(F_tempMap, F_R8(), String("%r8"));
	Temp_enter(F_tempMap, F_R9(), String("%r9"));

	//callersaveregs:%r10, %r11
	Temp_enter(F_tempMap, F_R10(), String("%r10"));
	Temp_enter(F_tempMap, F_R11(), String("%r11"));

	//calleesaveregs:%rbx, %rbp, %r12, %r13, %r14, %r15
	Temp_enter(F_tempMap, F_RBP(), String("%rbp"));
	Temp_enter(F_tempMap, F_RBX(), String("%rbx"));
	Temp_enter(F_tempMap, F_R12(), String("%r12"));
	Temp_enter(F_tempMap, F_R13(), String("%r13"));
	Temp_enter(F_tempMap, F_R14(), String("%r14"));
	Temp_enter(F_tempMap, F_R15(), String("%r15"));

	//initialize specialregs
    if(specialregs == NULL){
        specialregs = Temp_empty();
        Temp_enter(specialregs, F_FP(), String("%rbp"));
        Temp_enter(specialregs, F_RV(), String("%rax"));
        Temp_enter(specialregs, F_SP(), String("%rsp"));
    }
    //initialize argregs
    if(argregs == NULL){
        argregs = Temp_empty();
        Temp_enter(argregs, F_RDI(), String("%rdi"));
        Temp_enter(argregs, F_RSI(), String("%rsi"));
        Temp_enter(argregs, F_RDX(), String("%rdx"));
        Temp_enter(argregs, F_RCX(), String("%rcx"));
        Temp_enter(argregs, F_R8(), String("%r8"));
        Temp_enter(argregs, F_R9(), String("%r9"));
    }
    //initialize calleesaveregs
    if(calleesaves == NULL){
        calleesaves = Temp_empty();
        Temp_enter(calleesaves, F_RBP(), String("%rbp"));
        Temp_enter(calleesaves, F_RBX(), String("%rbx"));
        Temp_enter(calleesaves, F_R12(), String("%r12"));
        Temp_enter(calleesaves, F_R13(), String("%r13"));
        Temp_enter(calleesaves, F_R14(), String("%r14"));
        Temp_enter(calleesaves, F_R15(), String("%r15"));
    }
    //initialize callersaveregs
    if(callersaves == NULL){
        callersaves = Temp_empty();
        Temp_enter(callersaves, F_R10(), String("%r10"));
	    Temp_enter(callersaves, F_R11(), String("%r11"));
    }
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

Temp_label F_name(F_frame f){
	return f->label;
}

F_accessList F_formals(F_frame f){
	return f->formalList;
}

F_frame F_newFrame(Temp_label name, U_boolList formals){
	F_frame frame = (F_frame) checked_malloc(sizeof(*frame));
	assert(name);
	frame->label = name;
	frame->current_size = 0;
	frame->localList = NULL;
	frame->prologue = T_Exp(T_Const(0));
	frame->epilogue = NULL;
	//Handle static link first
	frame->formalList = F_AccessList(InFrame(0), NULL);
	F_accessList* temp_accessList = &(frame->formalList->tail);
	int offset = 0;
	//skip static link
	formals = formals->tail;
	Temp_tempList argregs = argRegs();
	//arguments view change
	while(formals){
		//argument in memory
		//如果实参在内存但是本身不逃逸，就move到寄存器中
		if(!formals->head && offset > 5){
			F_access localVar = F_allocLocal(frame, FALSE);
			*temp_accessList = F_AccessList(localVar, NULL);
			frame->prologue = T_Seq(frame->prologue, T_Move(T_Temp(localVar->u.reg), 
						T_Mem(T_Binop(T_plus, T_Const((2+offset)*F_wordSize), T_Temp(F_FP())))));
		}//如果实参在寄存器，也要mov到临时变量中，原因见bug-report.txt
		else if(!formals->head && offset <= 5){
			F_access localVar = F_allocLocal(frame, FALSE);
			*temp_accessList = F_AccessList(localVar, NULL);
			frame->prologue = T_Seq(frame->prologue, T_Move(T_Temp(localVar->u.reg), T_Temp(argregs->head)));
		}//如果1实参在内存并且逃逸，就在原位置就好
		else if(formals->head && offset > 5){
			*temp_accessList = F_AccessList(InFrame((2+offset)*F_wordSize), NULL);
		}
		//如果实参在寄存器但是本身逃逸，就move到内存中
		else{
			F_access localVar = F_allocLocal(frame, TRUE);
			*temp_accessList = F_AccessList(localVar, NULL);
			frame->prologue = T_Seq(frame->prologue, T_Move(T_Mem(
				T_Binop(T_plus, T_Const(localVar->u.offset), T_Temp(F_FP()))), T_Temp(argregs->head)));
		}
		offset++;
		formals = formals->tail;
		argregs = argregs->tail;
		temp_accessList = &((*temp_accessList)->tail);
	}
	return frame;
}

// Must handle
// - Underscore function
// - different calling conventions(such as no static link)
T_exp F_externalCall(string s, T_expList args){
	//const 0 represent a fate static link
	return T_Call(T_Name(Temp_namedlabel(s)), T_ExpList(T_Const(0), args));
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
	Temp_tempList calleeSaveRegs = calleeSaves();
	T_stm saveStm = NULL, unsaveStm = NULL;
	Temp_tempList saveTemps = NULL;
	Temp_tempList* saveTempsPtr = &saveTemps;
	//save stm
	while(calleeSaveRegs){
		Temp_temp temp = Temp_newtemp();
		saveStm = (saveStm ? T_Seq(saveStm, T_Move(T_Temp(temp), T_Temp(calleeSaveRegs->head)))
						: T_Move(T_Temp(temp), T_Temp(calleeSaveRegs->head)));
		*saveTempsPtr = Temp_TempList(temp, NULL);
		saveTempsPtr = &((*saveTempsPtr)->tail);
		calleeSaveRegs = calleeSaveRegs->tail;
	}
	calleeSaveRegs = calleeSaves();
	//unsave stm
	while(calleeSaveRegs){
		unsaveStm = (unsaveStm ? T_Seq(unsaveStm, T_Move(T_Temp(calleeSaveRegs->head), T_Temp(saveTemps->head)))
							: T_Move(T_Temp(calleeSaveRegs->head), T_Temp(saveTemps->head)));
		calleeSaveRegs = calleeSaveRegs->tail;
		saveTemps = saveTemps->tail;
	}
	//there should no temps left
	assert(!saveTemps);
	return T_Seq(saveStm,T_Seq(frame->prologue, T_Seq(stm, unsaveStm)));
}
static Temp_tempList returnSink = NULL;
AS_instrList F_procEntryExit2(F_frame frame, AS_instrList body){
	//scan the whole proc body to find max arg's number
	AS_instrList tempBody = body;
	int max_num = 0;
	while(tempBody){
		AS_instr instr = tempBody->head;
		if(instr->kind == I_OPER && !strncmp(instr->u.OPER.assem, " call", 4)){
			int num = 0;
			Temp_tempList tempList = instr->u.OPER.src;
			while(tempList){
				tempList = tempList->tail;
				num++;
			}
			max_num = (max_num > num ? max_num : num);
		}
		tempBody = tempBody->tail;
	}
	frame->max_arg_num = max_num;
	if(!returnSink){
		returnSink = Temp_TempList(F_RV(), Temp_TempList(F_SP(), calleeSaves()));
	}
	return AS_splice(body, AS_InstrList(AS_Oper("", NULL, returnSink, NULL), NULL));
}

AS_proc F_procEntryExit3(F_frame frame, AS_instrList body){
	//static link
	frame->current_size += 8;
	if(frame->max_arg_num > 6){
		frame->current_size += (frame->max_arg_num - 6) * F_wordSize;
	}
	//fix all framesize
	RewriteFrameSize(frame, body);
	char prolog_buf[100], epilog_buf[100];
	//sprintf(prolog_buf, "%s:\n .cfi_startproc\n subq $%d, %%rsp\n", S_name(frame->label), frame->current_size);
	//sprintf(prolog_buf, "%s:\n subq $%d, %%rsp\n", S_name(frame->label), frame->current_size);
	sprintf(prolog_buf, "%s:\n", S_name(frame->label));
	//sprintf(epilog_buf, " addq $%d, %%rsp\n ret\n .cfi_endproc\n\n", frame->current_size);
	//sprintf(epilog_buf, " addq $%d, %%rsp\n retq\n\n", frame->current_size);
	sprintf(epilog_buf, " retq\n\n");
    return AS_Proc(String(prolog_buf), body, String(epilog_buf));
}

