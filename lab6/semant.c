#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "util.h"
#include "errormsg.h"
#include "symbol.h"
#include "absyn.h"
#include "types.h"
#include "env.h"
#include "semant.h"
#include "helper.h"
#include "translate.h"

/*Lab5: Your implementation of lab5.*/

struct expty 
{
	Tr_exp exp; 
	Ty_ty ty;
};

//In Lab4, the first argument exp should always be **NULL**.
struct expty expTy(Tr_exp exp, Ty_ty ty)
{
	struct expty e;

	e.exp = exp;
	e.ty = ty;

	return e;
}

F_fragList SEM_transProg(A_exp exp){

	//TODO LAB5: do not forget to add the main frame
	Tr_level main_level = Tr_newLevel(Tr_outermost(), S_Symbol(String("tigermain")), NULL);
	//Tr_level main_level = Tr_outermost();
	struct expty main_exp = transExp(E_base_venv(), E_base_tenv(), exp, main_level, NULL);
	Tr_procEntryExit(main_level, main_exp.exp, NULL);
	return Tr_getResult();
}

Ty_ty actual_ty(Ty_ty ty){
	while(ty->kind == Ty_name){
		ty = ty->u.name.ty;
	}
	return ty;
}

bool is_same_type(Ty_ty ty1, Ty_ty ty2){
	ty1 = actual_ty(ty1);
	ty2 = actual_ty(ty2);

	if(ty1->kind == Ty_record && (ty2->kind == Ty_nil || ty2->kind == Ty_int)){
		return TRUE;
	}else if(ty2->kind == Ty_record && (ty1->kind == Ty_nil || ty1->kind == Ty_int)){
		return TRUE;
	}else if(ty1 == ty2){
		return TRUE;
	}
	return FALSE;
}

U_boolList makeFormalBoolList(A_fieldList fieldList, int index){
	if(!fieldList) return NULL;
	//modified: (index > 6) -> fieldList->head->escape
	return U_BoolList(fieldList->head->escape, makeFormalBoolList(fieldList->tail, index+1));
}

Ty_tyList makeFormalTyList(S_table tenv, A_fieldList fieldList){
	if(!fieldList){
		return NULL;
	}
	A_field field = fieldList->head; 
	Ty_ty type = S_look(tenv, field->typ);
	//If we can't find a typeid, we use Int.
	if(!type){
		EM_error(field->pos, "argument type mismatch.");
		type = Ty_Int();
	}
	return Ty_TyList(type, makeFormalTyList(tenv, fieldList->tail));
}

Ty_fieldList makeTypeFieldList(S_table tenv, A_fieldList fieldList){
	if(!fieldList){
		return NULL;
	}
	A_field field = fieldList->head; 
	Ty_ty type = S_look(tenv, field->typ);
	if(!type){
		EM_error(field->pos, "undefined type %s", S_name(field->typ));
		type = Ty_Int();
	}
	Ty_field field_ty = Ty_Field(field->name, type);
	return Ty_FieldList(field_ty, makeTypeFieldList(tenv, fieldList->tail));
}

Ty_ty transTy (S_table tenv, A_ty a){
	switch(a->kind){
		case A_nameTy:{
			Ty_ty type = S_look(tenv, get_ty_name(a));
			if(!type){
				EM_error(a->pos, "undefined type %s", S_name(get_ty_name(a)));
				type = Ty_Int();
			}
			while(type && type->kind == Ty_name && type->u.name.ty){
				type = type->u.name.ty;
			}
			return type;
		}
		case A_recordTy:{
			return Ty_Record(makeTypeFieldList(tenv, get_ty_record(a)));
		}
		case A_arrayTy:{
			return Ty_Array(S_look(tenv, get_ty_array(a)));
		}
	}
	assert(0);
}

struct expty transSimpleVar(S_table venv, S_table tenv, A_var var, Tr_level level, Temp_label looplabel){
	E_enventry x = S_look(venv, get_simplevar_sym(var));
	if(x && x->kind == E_varEntry){
		return expTy(Tr_simpleVar(x->u.var.access, level), actual_ty(get_varentry_type(x)));
	}else{
		EM_error(var->pos, "undefined variable %s", S_name(var->u.simple));
		return expTy(NULL, Ty_Int());
	}
}

struct expty transFieldVar(S_table venv, S_table tenv, A_var var, Tr_level level, Temp_label looplabel){
	struct expty var_ty = transVar(venv, tenv, get_fieldvar_var(var), level, looplabel);
	if(get_expty_kind(var_ty) == Ty_record){
		//search this field
		Ty_fieldList fieldList = get_record_fieldlist(var_ty);
		int offset = 0;
		while(fieldList){
			Ty_field field = fieldList->head;
			if(field->name == get_fieldvar_sym(var)){
				return expTy(Tr_fieldVar(var_ty.exp, offset), actual_ty(field->ty));
			}
			fieldList = fieldList->tail;
			offset++;
		}
		EM_error(var->pos, "field name doesn't exist");
		return expTy(NULL, Ty_Int());
	}else{
		EM_error(var->pos, "not a record type");
		return expTy(NULL, Ty_Int());
	}
}

struct expty transSubscriptVar(S_table venv, S_table tenv, A_var var, Tr_level level, Temp_label looplabel){
	struct expty var_ty = transVar(venv, tenv, get_subvar_var(var), level, looplabel);
	if(get_expty_kind(var_ty) == Ty_array){
		struct expty subscript_ty = transExp(venv, tenv, get_subvar_exp(var), level, looplabel);
		if(subscript_ty.ty != Ty_Int()){
			EM_error(get_subvar_exp(var)->pos, "integer required");
			return expTy(NULL, Ty_Int());
		}
		int element_size;
		if(actual_ty(var_ty.ty->u.array) == Ty_Int()){
			element_size = 4;
		}else{
			element_size = 8;
		}
		return expTy(Tr_subscriptVar(var_ty.exp, subscript_ty.exp, element_size), actual_ty(get_array(var_ty)));
	}else{
		EM_error(var->pos, "array type required");
		return expTy(NULL, Ty_Int());
	}
}

void checkLoopVariable(S_table venv, A_var var){
	switch(var->kind){
		case A_simpleVar:{
			E_enventry var_entry = S_look(venv, get_simplevar_sym(var));
			if(var_entry->readonly){
				EM_error(var->pos, "loop variable can't be assigned");
			}
		}
		case A_fieldVar:{
			checkLoopVariable(venv, var->u.field.var);
		}
		case A_subscriptVar:{
			checkLoopVariable(venv, var->u.subscript.var);
		} 
	}
}

struct expty transVar(S_table venv, S_table tenv, A_var var, Tr_level level, Temp_label looplabel){
	switch(var->kind){
		case A_simpleVar:{
			return transSimpleVar(venv, tenv, var, level, looplabel);
		}
		case A_fieldVar:{
			return transFieldVar(venv, tenv, var, level, looplabel);
		}
		case A_subscriptVar:{
			return transSubscriptVar(venv, tenv, var, level, looplabel);
		}
	}
	assert(0);
}

Tr_exp transVarDec(S_table venv, S_table tenv, A_dec d, Tr_level level, Temp_label looplabel){
	struct expty e = transExp(venv, tenv, get_vardec_init(d), level, looplabel);
	//now we assume every local var is escape.
	Tr_access access = Tr_allocLocal(level, TRUE);
	if(get_vardec_typ(d)){
		Ty_ty type_ty = S_look(tenv, get_vardec_typ(d));
		if(!type_ty){
			EM_error(d->pos, "undefined type %s", S_name(get_vardec_typ(d)));
			type_ty = Ty_Int();
		}
		if(e.ty != actual_ty(type_ty) && !(e.ty == Ty_Nil() && actual_ty(type_ty)->kind == Ty_record)){
			EM_error(d->pos, "init type mismatch type specified");
		}
		S_enter(venv, d->u.var.var, E_VarEntry(access, actual_ty(type_ty)));
	}else{
		if(e.ty == Ty_Nil()){
			EM_error(d->pos, "init should not be nil without type specified");
		}
		S_enter(venv, d->u.var.var, E_VarEntry(access, e.ty));
	}
	return Initial_exp(access, e.exp);
}

Tr_exp transTypeDec(S_table venv, S_table tenv, A_dec d, Tr_level level, Temp_label looplabel){
	A_nametyList typedecList = get_typedec_list(d);
	while(typedecList){
		Ty_ty type = S_look(tenv, typedecList->head->name);
		if(type && type->kind == Ty_name && !type->u.name.ty){
			EM_error(d->pos, "two types have the same name");
		}
		S_enter(tenv, typedecList->head->name, Ty_Name(typedecList->head->name, NULL));
		typedecList = typedecList->tail;
	}
	typedecList = get_typedec_list(d);
	while(typedecList){
		Ty_ty type = S_look(tenv, typedecList->head->name);
		Ty_ty actual_type = transTy(tenv, typedecList->head->ty);
		if(actual_type == type){
			EM_error(d->pos, "illegal type cycle");
			type->u.name.ty = Ty_Int();
		}else{
			type->u.name.ty = actual_type;
		}
		typedecList = typedecList->tail;
	}
	return Empty_exp();
}

Tr_exp transFunctionDec(S_table venv, S_table tenv, A_dec d, Tr_level level, Temp_label looplabel){
	A_fundecList funcdecList = get_funcdec_list(d);
	while(funcdecList){
		A_fundec f = funcdecList->head;
		//test if there are other functions with same name
		//Not a elegant solution!
		A_fundecList tempDecList = funcdecList->tail;
		while(tempDecList){
			if(tempDecList->head->name == f->name){
				EM_error(f->pos, "two functions have the same name");
			}
			tempDecList = tempDecList->tail;
		}
		Ty_tyList formalTys = makeFormalTyList(tenv, f->params);
		U_boolList formalBools = makeFormalBoolList(f->params, 0);
		//test if return type is void
		Ty_ty result_ty;
		if(f->result){
			result_ty = S_look(tenv, f->result);
			//check result type is defined
			if(!result_ty){
				EM_error(f->pos, "undefined type %s", f->result);
				result_ty = Ty_Void();
			}
		}else{
			result_ty = Ty_Void();
		}
		Temp_label func_label = f->name;
		Tr_level func_level = Tr_newLevel(level, func_label, formalBools);
		assert(func_label);
		S_enter(venv, f->name, E_FunEntry(func_level, func_label,formalTys, result_ty));
		funcdecList = funcdecList->tail;
	}
	funcdecList = get_funcdec_list(d);
	while(funcdecList){
		A_fundec f = funcdecList->head;
		E_enventry funcEntry = S_look(venv, f->name);
		Tr_level func_level = funcEntry->u.fun.level;
		Ty_ty result_ty = funcEntry->u.fun.result;
		S_beginScope(venv);
		A_fieldList l;
		Ty_tyList t;
		Tr_accessList formalsAccess = Tr_formals(func_level);
		for(l = f->params, t = funcEntry->u.fun.formals; l; l = l->tail, t = t->tail){
			S_enter(venv, l->head->name, E_VarEntry(formalsAccess->head, t->head));
			formalsAccess = formalsAccess->tail;
		}
		//Notice!!:change level as func_level
		struct expty body_ty = transExp(venv, tenv, f->body, func_level, looplabel);
		if(result_ty->kind == Ty_record && (body_ty.ty->kind == Ty_nil || body_ty.ty->kind == Ty_int)){
				//handle record:(int | nil)
		}
		else if(body_ty.ty != actual_ty(result_ty)){
			EM_error(f->body->pos, "procedure returns value");
		}
		Tr_procEntryExit(func_level, body_ty.exp, formalsAccess);
		S_endScope(venv);
		funcdecList = funcdecList->tail;
	}
	return Empty_exp();
}

Tr_exp transDec(S_table venv, S_table tenv, A_dec d, Tr_level level, Temp_label looplabel){
	switch(d->kind){
		case A_varDec:{
			return transVarDec(venv, tenv, d, level, looplabel);
		}
		case A_typeDec:{
			return transTypeDec(venv, tenv, d, level, looplabel);
		}
		case A_functionDec:{
			return transFunctionDec(venv, tenv, d, level, looplabel);
		}
	}
	assert(0);
}
struct expty transCallExp(S_table venv, S_table tenv, A_exp exp, Tr_level level, Temp_label looplabel){
	E_enventry func = S_look(venv, exp->u.call.func);
	if(func && func->kind == E_funEntry){
		Ty_tyList args_types = func->u.fun.formals;
		A_expList args_exps = exp->u.call.args;
		Tr_expList tr_expList = NULL;
		//Check every argument
		while(args_types){
			//Argument number is small
			if(!args_exps){
				EM_error(exp->pos, "too little params in function %s", S_name(exp->u.call.func));
				break;
			}
			struct expty arg = transExp(venv, tenv, args_exps->head, level, looplabel);
			if(!is_same_type(arg.ty, args_types->head)){
				EM_error(exp->u.call.args->head->pos, "para type mismatch");
			}
			tr_expList = Tr_ExpList(arg.exp, tr_expList);
			args_types = args_types->tail;
			args_exps = args_exps->tail;
		}
		//Arguemnt number is big
		if(args_exps){
			EM_error(args_exps->head->pos, "too many params in function %s", S_name(exp->u.call.func));
		}
		//assert(func->u.fun.label);
		return expTy(Tr_functionCall(level, func->u.fun.level, exp->u.call.func, tr_expList)
					, actual_ty(func->u.fun.result));
	}else{	
		EM_error(exp->pos, "undefined function %s", S_name(exp->u.call.func));
		return expTy(NULL, Ty_Int());
	} 
}

struct expty transOpExp(S_table venv, S_table tenv, A_exp exp, Tr_level level, Temp_label looplabel){
	A_oper oper = exp->u.op.oper;
	struct expty left = transExp(venv, tenv, exp->u.op.left, level, looplabel);
	struct expty right = transExp(venv, tenv, exp->u.op.right, level, looplabel);
	switch(oper){
		case A_plusOp:
		case A_minusOp:
		case A_timesOp:
		case A_divideOp:{
			if(left.ty->kind != Ty_int){
				EM_error(exp->u.op.left->pos, "integer required");
			}
			if(right.ty->kind != Ty_int){
				EM_error(exp->u.op.right->pos, "integer required");
			}
			return expTy(Tr_arithmatic(oper, left.exp, right.exp), Ty_Int());
		}
		case A_ltOp:
		case A_leOp:
		case A_gtOp:
		case A_geOp:{
			if(left.ty->kind != Ty_int && left.ty->kind != Ty_string){
				EM_error(exp->u.op.left->pos, "integer or string required");
			}
			if(right.ty->kind != Ty_int && right.ty->kind != Ty_string){
				EM_error(exp->u.op.right->pos, "integer or string required");
			}
			if(left.ty->kind != right.ty->kind){
				EM_error(exp->u.op.right->pos, "same type required");//TODO: should use is_same_type?
			}
			return expTy(Tr_condition(oper, left.exp, right.exp), Ty_Int());
		}
		case A_eqOp:
		case A_neqOp:{
			if(left.ty->kind != Ty_int && left.ty->kind != Ty_string && left.ty->kind != Ty_record && left.ty->kind != Ty_array){
				EM_error(exp->u.op.left->pos, "integer or record or array required");
			}
			if(right.ty->kind != Ty_int && left.ty->kind != Ty_string && right.ty->kind != Ty_record && right.ty->kind != Ty_array && right.ty->kind != Ty_nil){
				EM_error(exp->u.op.right->pos, "integer or record or array requirred");
			}
			if(left.ty->kind != right.ty->kind && !(left.ty->kind == Ty_record && right.ty->kind == Ty_nil)){
				EM_error(exp->u.op.right->pos, "same type required");//TODO: should use is_same_type?
			}
			return expTy(Tr_condition(oper, left.exp, right.exp), Ty_Int());
		}
	}
	assert(0);
}

struct expty transRecordExp(S_table venv, S_table tenv, A_exp exp, Tr_level level, Temp_label looplabel){
	Ty_ty rec_type = S_look(tenv, exp->u.record.typ);
	if(!rec_type){
		EM_error(exp->pos, "undefined type %s", S_name(exp->u.record.typ));
		return expTy(NULL, Ty_Int());
	}
	rec_type = actual_ty(rec_type);
	if(rec_type && rec_type->kind == Ty_record){
		A_efieldList efield_list = exp->u.record.fields;
		Ty_fieldList efield_type_list = rec_type->u.record;
		int size = 0;
		Tr_expList tr_expList = NULL;
		while(efield_type_list){
			if(!efield_list){
				EM_error(exp->pos, "record field length mismatch");
				break;
			}
			Ty_field field_type = efield_type_list->head;
			A_efield field = efield_list->head;
			struct expty efield_exp = transExp(venv, tenv, field->exp, level, looplabel);
			if(field->name != field_type->name){
				EM_error(exp->pos, "record field id mismatch");
			}
			if(!is_same_type(efield_exp.ty, field_type->ty)){
				EM_error(exp->pos, "record field type mismatch");
			}
			size++;
			tr_expList = Tr_ExpList(efield_exp.exp, tr_expList);
			efield_type_list = efield_type_list->tail;
			efield_list = efield_list->tail;
		}
		if(efield_list){
			EM_error(exp->pos, "record field length mismatch");
		}
		return expTy(Tr_recordExp(size, tr_expList), rec_type);
	}else{
		EM_error(exp->pos, "record required");
		return expTy(NULL, Ty_Int());
	}
}

struct expty transSeqExp(S_table venv, S_table tenv, A_exp exp, Tr_level level, Temp_label looplabel){
	A_expList explist = exp->u.seq;
	Tr_expList tr_expList = NULL; 
	if(!explist){
		return expTy(Empty_exp(), Ty_Void());
	}
	//translate sequential exps
	while(explist->tail){
		struct expty temp_exp = transExp(venv, tenv, explist->head, level, looplabel);
		tr_expList = Tr_ExpList(temp_exp.exp, tr_expList);
		assert(temp_exp.exp);
		explist = explist->tail;
	}
	//translate the last exp
	struct expty value_exp = transExp(venv, tenv, explist->head, level, looplabel);
	value_exp.exp = Tr_let(tr_expList, value_exp.exp);
	return value_exp;
}

struct expty transAssignExp(S_table venv, S_table tenv, A_exp exp, Tr_level level, Temp_label looplabel){
	struct expty lvalue = transVar(venv, tenv, exp->u.assign.var, level, looplabel);
	struct expty rvalue = transExp(venv, tenv, exp->u.assign.exp, level, looplabel);
	checkLoopVariable(venv, exp->u.assign.var);
	if(lvalue.ty == rvalue.ty){
		return expTy(Tr_assign(lvalue.exp, rvalue.exp), Ty_Void());
	}else if(lvalue.ty->kind == Ty_record && rvalue.ty == Ty_Nil()){
		return expTy(Tr_assign(lvalue.exp, rvalue.exp), Ty_Void());
	}else {
		EM_error(exp->u.assign.exp->pos, "unmatched assign exp");//TODO:should use is_same_type?
		return expTy(NULL, Ty_Void());
	}
}
struct expty transIfExp(S_table venv, S_table tenv, A_exp exp, Tr_level level, Temp_label looplabel){
	struct expty test_ty = transExp(venv, tenv, exp->u.iff.test, level, looplabel);
	struct expty then_ty = transExp(venv, tenv, exp->u.iff.then, level, looplabel);
	struct expty else_ty = expTy(NULL, NULL);
	if(test_ty.ty != Ty_Int()){
		EM_error(exp->u.iff.test->pos, "integer required");
	}
	if(exp->u.iff.elsee){
		else_ty = transExp(venv, tenv, exp->u.iff.elsee, level, looplabel);
		if(!is_same_type(then_ty.ty, else_ty.ty)){
			EM_error(exp->u.iff.then->pos, "then exp and else exp type mismatch");
		}
	}else{
		if(then_ty.ty != Ty_Void()){
			EM_error(exp->u.iff.then->pos, "if-then exp's body must produce no value");
		}
	}
	return expTy(Tr_if(test_ty.exp, then_ty.exp, else_ty.exp), then_ty.ty);
}

struct expty transWhileExp(S_table venv, S_table tenv, A_exp exp, Tr_level level, Temp_label looplabel){
	Temp_label done_label = Temp_newlabel();
	struct expty test_ty = transExp(venv, tenv, exp->u.whilee.test, level, looplabel);
	struct expty body_ty = transExp(venv, tenv, exp->u.whilee.body, level, done_label);
	if(test_ty.ty != Ty_Int()){
		EM_error(exp->u.whilee.test->pos, "integer required");
	}
	if(body_ty.ty != Ty_Void()){
		EM_error(exp->u.whilee.body->pos, "while body must produce no value");
	}
	return expTy(Tr_while(test_ty.exp, body_ty.exp, done_label), Ty_Void());
}

struct expty transForExp(S_table venv, S_table tenv, A_exp exp, Tr_level level, Temp_label looplabel){
	struct expty low_ty = transExp(venv, tenv, exp->u.forr.lo, level, looplabel);
	struct expty high_ty = transExp(venv, tenv, exp->u.forr.hi, level, looplabel);
	if(low_ty.ty != Ty_Int()){
		EM_error(exp->u.forr.lo->pos, "for exp's range type is not integer");
	}
	if(high_ty.ty != Ty_Int()){
		EM_error(exp->u.forr.hi->pos, "for exp's range type is not integer");
	}
	S_beginScope(venv);
	S_beginScope(tenv);
	Temp_label done_label = Temp_newlabel(); 
	Tr_access loopVar = Tr_allocLocal(level, FALSE);
	E_enventry loopVarEntry = E_ROVarEntry(loopVar, Ty_Int());
	S_enter(venv, exp->u.forr.var, loopVarEntry);
	struct expty body_ty = transExp(venv, tenv, exp->u.forr.body, level, done_label);
	if(body_ty.ty != Ty_Void()){
		EM_error(exp->u.forr.body->pos, "nil type required");
	}
	S_endScope(tenv);
	S_endScope(venv);
	return expTy(Tr_for(loopVar, low_ty.exp, high_ty.exp, body_ty.exp, done_label), Ty_Void());
}

struct expty transLetExp(S_table venv, S_table tenv, A_exp exp, Tr_level level, Temp_label looplabel){
	S_beginScope(venv);
	S_beginScope(tenv);
	Tr_expList decExpList = NULL;
	for(A_decList d = exp->u.let.decs; d; d = d->tail){
		Tr_exp decExp = transDec(venv, tenv, d->head, level, looplabel);
		decExpList = Tr_ExpList(decExp, decExpList);
	}
	struct expty body_ty = transExp(venv, tenv, exp->u.let.body, level, looplabel);
	body_ty.exp = Tr_let(decExpList, body_ty.exp);
	S_endScope(tenv);
	S_endScope(venv);
	return body_ty;
}

struct expty transArrayExp(S_table venv, S_table tenv, A_exp exp, Tr_level level, Temp_label looplabel){
	Ty_ty array_type = actual_ty(S_look(tenv, exp->u.array.typ));
	if(array_type && array_type->kind == Ty_array){
		struct expty size_ty = transExp(venv, tenv, exp->u.array.size, level, looplabel);
		if(size_ty.ty != Ty_Int()){
			EM_error(exp->pos, "size should be a integer");
		}
		struct expty init_ty = transExp(venv, tenv, exp->u.array.init, level, looplabel);
		if(!is_same_type(init_ty.ty, array_type->u.array)){
			EM_error(exp->pos, "init exp type mismatch");
		}
		return expTy(Tr_arrayExp(size_ty.exp, init_ty.exp), array_type);
	}else{
		EM_error(exp->pos, "undefined array type %s", S_name(exp->u.array.typ));
		return expTy(NULL, Ty_Int());
	}
}

struct expty transExp(S_table venv, S_table tenv, A_exp exp, Tr_level level, Temp_label looplabel){
	switch(exp->kind){
		case A_varExp:{
			return transVar(venv, tenv, exp->u.var, level, looplabel);
		}
		case A_nilExp:{
			//TODO: give a correct code
			return expTy(Tr_int(0), Ty_Nil());
		}
		case A_intExp:{
			return expTy(Tr_int(exp->u.intt), Ty_Int());
		}
		case A_stringExp:{
			return expTy(Tr_string(exp->u.stringg), Ty_String());
		}
		case A_callExp:{
			return transCallExp(venv, tenv, exp, level, looplabel);
		}
		case A_opExp:{
			return transOpExp(venv, tenv, exp, level, looplabel);
		}
		case A_recordExp:{
			return transRecordExp(venv, tenv, exp, level, looplabel);
		}
		case A_seqExp:{
			return transSeqExp(venv, tenv, exp, level, looplabel);
		}
		case A_assignExp:{
			return transAssignExp(venv, tenv, exp, level, looplabel);
		}
		case A_ifExp:{
			return transIfExp(venv, tenv, exp, level, looplabel);
		}
		case A_whileExp:{
			return transWhileExp(venv, tenv, exp, level, looplabel);
		}
		case A_forExp:{
			return transForExp(venv, tenv, exp, level, looplabel);
		}
		case A_breakExp:{
			// check if break is in a loop
			if(!looplabel){
				EM_error(exp->pos, "break should be in a loop");
			}
			return expTy(Tr_break(looplabel), Ty_Void());
		}
		case A_letExp:{
			return transLetExp(venv, tenv, exp, level, looplabel);
		}
		case A_arrayExp:{
			return transArrayExp(venv, tenv, exp, level, looplabel);
		}
	}
	assert(0);
}
