
/*Lab5: This header file is not complete. Please finish it with more definition.*/

#ifndef FRAME_H
#define FRAME_H

#include "tree.h"
#include "assem.h"

typedef struct F_frame_ *F_frame;

//Frame
struct F_frame_ {
	S_symbol funcname;
	Temp_label label;
	F_accessList formalList;
	F_accessList localList;
	int current_size;
	T_stm view_shift;
};


typedef struct F_access_ *F_access;
typedef struct F_accessList_ *F_accessList;

struct F_accessList_ {F_access head; F_accessList tail;};

//varibales
struct F_access_ {
	enum {inFrame, inReg} kind;
	union {
		int offset; //inFrame
		Temp_temp reg; //inReg
	} u;
};

Temp_map F_tempMap;

void Init_F_TempMap();

Temp_temp F_FP(void);
Temp_temp F_SP(void);
Temp_temp F_RV(void);

Temp_tempList specialRegs();
Temp_tempList argRegs();
Temp_tempList calleeSaves();
Temp_tempList callerSaves();

Temp_tempList F_registers();

F_accessList F_AccessList(F_access head, F_accessList tail);

extern const int F_wordSize;
T_exp F_Exp(F_access access, T_exp framePtr);

F_frame F_newFrame(Temp_label name, U_boolList formals);

Temp_label F_name(F_frame f);
F_accessList F_formals(F_frame f);
F_access F_allocLocal(F_frame f, bool escape);
T_exp F_externalCall(string s, T_expList args);

/* declaration for fragments */
typedef struct F_frag_ *F_frag;
struct F_frag_ {enum {F_stringFrag, F_procFrag} kind;
			union {
				struct {Temp_label label; string str;} stringg;
				struct {T_stm body; F_frame frame;} proc;
			} u;
};

F_frag F_StringFrag(Temp_label label, string str);
F_frag F_ProcFrag(T_stm body, F_frame frame);

typedef struct F_fragList_ *F_fragList;
struct F_fragList_ 
{
	F_frag head; 
	F_fragList tail;
};

F_fragList F_FragList(F_frag head, F_fragList tail);

T_stm F_procEntryExit1(F_frame feame, T_stm stm);
AS_instrList F_procEntryExit2(AS_instrList body);
AS_proc F_ProcEntryExit3(F_frame frame, AS_instrList body);
#endif
