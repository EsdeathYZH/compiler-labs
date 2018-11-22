#ifndef __SEMANT_H_
#define __SEMANT_H_

#include "absyn.h"
#include "symbol.h"
#include "temp.h"
#include "frame.h"
#include "translate.h"

struct expty;

struct expty transVar(S_table venv, S_table tenv, A_var v, Tr_level l, Temp_label);
struct expty transExp(S_table venv, S_table tenv, A_exp a, Tr_level l, Temp_label);
Tr_exp transDec(S_table venv, S_table tenv, A_dec d, Tr_level l, Temp_label);
Ty_ty		 transTy (              S_table tenv, A_ty a);
Ty_fieldList transFieldList(S_table tenv, A_fieldList fieldList);
Ty_ty actual_ty(Ty_ty);

struct expty transSimpleVar(S_table venv, S_table tenv, A_var v, Tr_level l, Temp_label);
struct expty transFieldVar(S_table venv, S_table tenv, A_var v, Tr_level l, Temp_label);
struct expty transSubscriptVar(S_table venv, S_table tenv, A_var v, Tr_level l, Temp_label);
//void SEM_transProg(A_exp exp);
F_fragList SEM_transProg(A_exp exp);

#endif
