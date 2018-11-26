#ifndef __SEMANT_H_
#define __SEMANT_H_

#include "absyn.h"
#include "symbol.h"
#include "env.h"

struct expty;

struct expty transVar(S_table venv, S_table tenv, A_var v, bool inloop, E_enventry loopVariable, bool lvalue);
struct expty transExp(S_table venv, S_table tenv, A_exp a, bool inloop, E_enventry loopVariable);
void		 transDec(S_table venv, S_table tenv, A_dec d, bool inloop, E_enventry loopVariable);
Ty_ty		 transTy (              S_table tenv, A_ty a);
Ty_fieldList transFieldList(S_table tenv, A_fieldList fieldList);
Ty_ty actual_ty(Ty_ty);

void SEM_transProg(A_exp exp);

#endif
