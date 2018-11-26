#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "util.h"
#include "errormsg.h"
#include "symbol.h"
#include "absyn.h"
#include "types.h"
#include "helper.h"
#include "env.h"
#include "semant.h"

typedef void* Tr_exp;
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

/*Lab4: Your implementation of lab4*/
void SEM_transProg(A_exp exp){
	transExp(E_base_venv(), E_base_tenv(), exp, FALSE, NULL);
}

Ty_ty actual_ty(Ty_ty ty){
	while(ty->kind == Ty_name){
		ty = ty->u.name.ty;
	}
	return ty;
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

struct expty transVar(S_table venv, S_table tenv, A_var var, bool inloop, E_enventry loopVariable, bool lvalue){
	switch(var->kind){
		case A_simpleVar:{
			E_enventry x = S_look(venv, get_simplevar_sym(var));
			if(x && x->kind == E_varEntry){
				if(inloop && lvalue && loopVariable == x){
					EM_error(var->pos, "loop variable can't be assigned");
				}
				return expTy(NULL, actual_ty(get_varentry_type(x)));
			}else{
				EM_error(var->pos, "undefined variable %s", S_name(var->u.simple));
				return expTy(NULL, Ty_Int());
			}
		}
		case A_fieldVar:{
			struct expty var_ty = transVar(venv, tenv, get_fieldvar_var(var), inloop, loopVariable, FALSE);
			if(get_expty_kind(var_ty) == Ty_record){
				//search this field
				Ty_fieldList fieldList = get_record_fieldlist(var_ty);
				while(fieldList){
					Ty_field field = fieldList->head;
					if(field->name == get_fieldvar_sym(var)){
						return expTy(NULL, actual_ty(field->ty));
					}
					fieldList = fieldList->tail;
				}
				EM_error(var->pos, "field nam doesn't exist");
				return expTy(NULL, Ty_Int());
			}else{
				EM_error(var->pos, "not a record type");
				return expTy(NULL, Ty_Int());
			}
		}
		case A_subscriptVar:{
			struct expty var_ty = transVar(venv, tenv, get_subvar_var(var), inloop, loopVariable, FALSE);
			if(get_expty_kind(var_ty) == Ty_array){
				struct expty subscript_ty = transExp(venv, tenv, get_subvar_exp(var), inloop, loopVariable);
				if(subscript_ty.ty != Ty_Int()){
					EM_error(get_subvar_exp(var)->pos, "integer required");
					return expTy(NULL, Ty_Int());
				}
				return expTy(NULL, actual_ty(get_array(var_ty)));
			}else{
				EM_error(var->pos, "array type required");
				return expTy(NULL, Ty_Int());
			}
		}
	}
	assert(0);
}

void transDec(S_table venv, S_table tenv, A_dec d, bool inloop, E_enventry loopVariable){
	switch(d->kind){
		case A_varDec:{
			struct expty e = transExp(venv, tenv, get_vardec_init(d), inloop, loopVariable);
			if(get_vardec_typ(d)){
				Ty_ty type_ty = S_look(tenv, get_vardec_typ(d));
				if(!type_ty){
					EM_error(d->pos, "undefined type %s", S_name(get_vardec_typ(d)));
					type_ty = Ty_Int();
				}
				if(e.ty != actual_ty(type_ty) && !(e.ty == Ty_Nil() && actual_ty(type_ty)->kind == Ty_record)){
					EM_error(d->pos, "init type mismatch type specified");
				}
				S_enter(venv, d->u.var.var, E_VarEntry(actual_ty(type_ty)));
			}else{
				if(e.ty == Ty_Nil()){
					EM_error(d->pos, "init should not be nil without type specified");
				}
				S_enter(venv, d->u.var.var, E_VarEntry(e.ty));
			}
			return;
		}
		case A_typeDec:{
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
			return;
		}
		case A_functionDec:{
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
				S_enter(venv, f->name, E_FunEntry(formalTys, result_ty));
				funcdecList = funcdecList->tail;
			}
			funcdecList = get_funcdec_list(d);
			while(funcdecList){
				A_fundec f = funcdecList->head;
				E_enventry funcEntry = S_look(venv, f->name);
				Ty_ty result_ty = funcEntry->u.fun.result;
				S_beginScope(venv);
				A_fieldList l;
				Ty_tyList t;
				for(l = f->params, t = funcEntry->u.fun.formals; l; l = l->tail, t = t->tail){
					S_enter(venv, l->head->name, E_VarEntry(t->head));
				}
				struct expty body_ty = transExp(venv, tenv, f->body, inloop, loopVariable);
				if(body_ty.ty != result_ty){
					EM_error(f->body->pos, "procedure returns value");
				}
				S_endScope(venv);
				funcdecList = funcdecList->tail;
			}
			return;
		}
	}
}

struct expty transExp(S_table venv, S_table tenv, A_exp exp, bool inloop, E_enventry loopVariable){
	switch(exp->kind){
		case A_varExp:{
			return transVar(venv, tenv, exp->u.var, inloop, loopVariable, FALSE);
		}
		case A_nilExp:{
			return expTy(NULL, Ty_Nil());
		}
		case A_intExp:{
			return expTy(NULL, Ty_Int());
		}
		case A_stringExp:{
			return expTy(NULL, Ty_String());
		}
		case A_callExp:{
			E_enventry func = S_look(venv, exp->u.call.func);
			if(func && func->kind == E_funEntry){
				Ty_tyList args_types = func->u.fun.formals;
				A_expList args_exps = exp->u.call.args;
				//Check every argument
				while(args_types){
					//Argument number is small
					if(!args_exps){
						EM_error(exp->pos, "too little params in function %s", S_name(exp->u.call.func));
						break;
					}
					struct expty arg = transExp(venv, tenv, args_exps->head, inloop, loopVariable);
					if(arg.ty != actual_ty(args_types->head)){
						EM_error(exp->u.call.args->head->pos, "para type mismatch");
					}//TODO: record and nil
					args_types = args_types->tail;
					args_exps = args_exps->tail;
				}
				//Arguemnt number is big
				if(args_exps){
					EM_error(args_exps->head->pos, "too many params in function %s", S_name(exp->u.call.func));
				}
				return expTy(NULL, actual_ty(func->u.fun.result));
			}else{
				EM_error(exp->pos, "undefined function %s", S_name(exp->u.call.func));
				return expTy(NULL, Ty_Int());
			} 
		}
		case A_opExp:{
			A_oper oper = exp->u.op.oper;
			struct expty left = transExp(venv, tenv, exp->u.op.left, inloop, loopVariable);
			struct expty right = transExp(venv, tenv, exp->u.op.right, inloop, loopVariable);
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
					return expTy(NULL, Ty_Int());
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
						EM_error(exp->u.op.right->pos, "same type required");
					}
					return expTy(NULL, Ty_Int());
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
						EM_error(exp->u.op.right->pos, "same type required");
					}
					return expTy(NULL, Ty_Int());
				}
			}
		}
		case A_recordExp:{
			Ty_ty rec_type = S_look(tenv, exp->u.record.typ);
			if(!rec_type){
				EM_error(exp->pos, "undefined type %s", S_name(exp->u.record.typ));
				return expTy(NULL, Ty_Int());
			}
			rec_type = actual_ty(rec_type);
			if(rec_type && rec_type->kind == Ty_record){
				A_efieldList efield_list = exp->u.record.fields;
				Ty_fieldList efield_type_list = rec_type->u.record;
				while(efield_type_list){
					if(!efield_list){
						EM_error(exp->pos, "record field length mismatch");
						break;
					}
					Ty_field field_type = efield_type_list->head;
					A_efield field = efield_list->head;
					struct expty efield_exp = transExp(venv, tenv, field->exp, inloop, loopVariable);
					if(field->name != field_type->name){
						EM_error(exp->pos, "record field id mismatch");
					}
					if(efield_exp.ty != actual_ty(field_type->ty)){
						EM_error(exp->pos, "record field type mismatch");
					}//TODO:record and nil
					efield_type_list = efield_type_list->tail;
					efield_list = efield_list->tail;
				}
				if(efield_list){
					EM_error(exp->pos, "record field length mismatch");
				}
				return expTy(NULL, rec_type);
			}else{
				EM_error(exp->pos, "record required");
				return expTy(NULL, Ty_Int());
			}
		}
		case A_seqExp:{
			A_expList explist = exp->u.seq; 
			if(!explist){
				return expTy(NULL, Ty_Void());
			}
			while(explist->tail){
				transExp(venv, tenv, explist->head, inloop, loopVariable);
				explist = explist->tail;
			}
			return transExp(venv, tenv, explist->head, inloop, loopVariable);
		}
		case A_assignExp:{
			struct expty lvalue = transVar(venv, tenv, exp->u.assign.var, inloop, loopVariable, TRUE);
			struct expty rvalue = transExp(venv, tenv, exp->u.assign.exp, inloop, loopVariable);
			if(lvalue.ty == rvalue.ty){
				return expTy(NULL, Ty_Void());
			}else if(lvalue.ty->kind == Ty_record && rvalue.ty == Ty_Nil()){
				return expTy(NULL, Ty_Void());
			}else {
				EM_error(exp->u.assign.exp->pos, "unmatched assign exp");
				return expTy(NULL, Ty_Void());
			}
		}
		case A_ifExp:{
			struct expty test_ty = transExp(venv, tenv, exp->u.iff.test, inloop, loopVariable);
			struct expty then_ty = transExp(venv, tenv, exp->u.iff.then, inloop, loopVariable);
			if(test_ty.ty != Ty_Int()){
				EM_error(exp->u.iff.test->pos, "integer required");
			}
			if(exp->u.iff.elsee){
				struct expty else_ty = transExp(venv, tenv, exp->u.iff.elsee, inloop, loopVariable);
				if(then_ty.ty != else_ty.ty){
					EM_error(exp->u.iff.then->pos, "then exp and else exp type mismatch");
				}
			}else{
				if(then_ty.ty != Ty_Void()){
					EM_error(exp->u.iff.then->pos, "if-then exp's body must produce no value");
				}
			}
			return expTy(NULL, then_ty.ty);
		}
		case A_whileExp:{
			struct expty test_ty = transExp(venv, tenv, exp->u.whilee.test, inloop, loopVariable);
			struct expty body_ty = transExp(venv, tenv, exp->u.whilee.body, TRUE, loopVariable);
			if(test_ty.ty != Ty_Int()){
				EM_error(exp->u.whilee.test->pos, "integer required");
			}
			if(body_ty.ty != Ty_Void()){
				EM_error(exp->u.whilee.body->pos, "while body must produce no value");
			}
			return expTy(NULL, Ty_Void());
		}
		case A_forExp:{
			struct expty low_ty = transExp(venv, tenv, exp->u.forr.lo, inloop, loopVariable);
			struct expty high_ty = transExp(venv, tenv, exp->u.forr.hi, inloop, loopVariable);
			if(low_ty.ty != Ty_Int()){
				EM_error(exp->u.forr.lo->pos, "for exp's range type is not integer");
			}
			if(high_ty.ty != Ty_Int()){
				EM_error(exp->u.forr.hi->pos, "for exp's range type is not integer");
			}
			S_beginScope(venv);
			S_beginScope(tenv);
			E_enventry loopEntry = E_VarEntry(Ty_Int());
			S_enter(venv, exp->u.forr.var, loopEntry);
			struct expty body_ty = transExp(venv, tenv, exp->u.forr.body, TRUE, loopEntry);
			if(body_ty.ty != Ty_Void()){
				EM_error(exp->u.let.body->pos, "nil type required");
			}
			S_endScope(tenv);
			S_endScope(venv);
			return expTy(NULL, Ty_Void());
		}
		case A_breakExp:{
			return expTy(NULL, Ty_Void());
		}
		case A_letExp:{
			S_beginScope(venv);
			S_beginScope(tenv);
			for(A_decList d = exp->u.let.decs; d; d = d->tail){
				transDec(venv, tenv, d->head, inloop, loopVariable);
			}
			struct expty body_ty = transExp(venv, tenv, exp->u.let.body, inloop, loopVariable);
			S_endScope(tenv);
			S_endScope(venv);
			return body_ty;
		}
		case A_arrayExp:{
			Ty_ty array_type = actual_ty(S_look(tenv, exp->u.array.typ));
			if(array_type && array_type->kind == Ty_array){
				struct expty size_ty = transExp(venv, tenv, exp->u.array.size, inloop, loopVariable);
				if(size_ty.ty != Ty_Int()){
					EM_error(exp->pos, "size should be a integer");
				}
				struct expty init_ty = transExp(venv, tenv, exp->u.array.init, inloop, loopVariable);
				if(init_ty.ty != actual_ty(array_type->u.array)){
					EM_error(exp->pos, "init exp type mismatch");
				}//TODO:record and nil
				return expTy(NULL, array_type);
			}else{
				EM_error(exp->pos, "undefined array type %s", S_name(exp->u.array.typ));
				return expTy(NULL, Ty_Int());
			}
		}
	}
	assert(0);
}

