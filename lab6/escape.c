#include <stdio.h>
#include <string.h>
#include "util.h"
#include "symbol.h"
#include "absyn.h"
#include "escape.h"
#include "table.h"

struct EscapeEntry_ {
	int depth;
	bool* escape;
};

typedef struct EscapeEntry_* EscapeEntry;

static EscapeEntry escapeEntry(int depth, bool* escape){
	EscapeEntry entry = (EscapeEntry) checked_malloc(sizeof(*entry));
	entry->depth = depth;
	entry->escape = escape;
	return entry;
}

static void traverseExp(S_table env, int depth, A_exp exp){
	switch(exp->kind){
		case A_varExp:{
			traverseVar(env, depth, exp->u.var);
			return;
		}
		case A_nilExp:{
			return;
		}
		case A_intExp:{
			return;
		}
		case A_stringExp:{
			return;
		}
		case A_callExp:{
			A_expList args_exps = exp->u.call.args;
			//traverse every argument
			while(args_exps){
				traverseExp(env, depth, args_exps->head);
				args_exps = args_exps->tail;
			}
			return;
		}
		case A_opExp:{
			traverseExp(env, depth, exp->u.op.left);
			traverseExp(env, depth, exp->u.op.right);
			return;
		}
		case A_recordExp:{	
			A_efieldList efield_list = exp->u.record.fields;
			while(efield_list){
				A_efield field = efield_list->head;
				traverseExp(env, depth, field->exp);
				efield_list = efield_list->tail;
			}
			return;
		}
		case A_seqExp:{
			A_expList explist = exp->u.seq;
			//tranverse sequential exps
			while(explist){
				traverseExp(env, depth, explist->head);
				explist = explist->tail;
			}
			return;
		}
		case A_assignExp:{
			traverseVar(env, depth, exp->u.assign.var);
			traverseExp(env, depth, exp->u.assign.exp);
			return;
		}
		case A_ifExp:{
			traverseExp(env, depth, exp->u.iff.test);
			traverseExp(env, depth, exp->u.iff.then);
			if(exp->u.iff.elsee){
				traverseExp(env, depth, exp->u.iff.elsee);
			}
			return;
		}
		case A_whileExp:{
			traverseExp(env, depth, exp->u.whilee.test);
			traverseExp(env, depth, exp->u.whilee.body);
			return;
		}
		case A_forExp:{
			traverseExp(env, depth, exp->u.forr.lo);
			traverseExp(env, depth, exp->u.forr.hi);
			S_beginScope(env);
			exp->u.forr.escape = FALSE;
			S_enter(env, exp->u.forr.var, escapeEntry(depth, &exp->u.forr.escape));
			traverseExp(env, depth, exp->u.forr.body);
			S_endScope(env);
			return;
		}
		case A_breakExp:{
			return;
		}
		case A_letExp:{
			S_beginScope(env);
			for(A_decList d = exp->u.let.decs; d; d = d->tail){
				traverseDec(env, depth, d->head);
			}
			traverseExp(env, depth, exp->u.let.body);
			S_endScope(env);
			return;
		}
		case A_arrayExp:{
			traverseExp(env, depth, exp->u.array.size);
			traverseExp(env, depth, exp->u.array.init);
			return;
		}
	}
	assert(0);
}

static void traverseDec(S_table env, int depth, A_dec dec){
	switch(dec->kind){
		case A_varDec:{
			traverseExp(env, depth, dec->u.var.init);
			dec->u.var.escape = FALSE;
			S_enter(env, dec->u.var.var, escapeEntry(depth, &dec->u.var.escape));
			return;
		}
		case A_typeDec:{
			return;
		}
		case A_functionDec:{
			A_fundecList funcdecList = get_funcdec_list(dec);
			while(funcdecList){
				A_fundec f = funcdecList->head;
				S_beginScope(env);
				for(A_fieldList l = f->params; l; l = l->tail){
					l->head->escape = FALSE;
					S_enter(env, l->head->name, escapeEntry(depth+1, &l->head->escape));
				}
				//change depth
				traverseExp(env, depth+1, f->body);
				S_endScope(env);
				funcdecList = funcdecList->tail;
			}
			return;
		}
	}
	assert(0);
}


static void traverseVar(S_table env, int depth, A_var var){
	switch(var->kind){
		case A_simpleVar:{
			EscapeEntry entry = S_look(env, get_simplevar_sym(var));
			if(entry){
				if(depth > entry->depth){
					*(entry->escape) = TRUE;
				}
				return;
			}else{
				EM_error(var->pos, "undefined variable %s", S_name(var->u.simple));
				return;
			}
		}
		case A_fieldVar:{
			traverseVar(env, depth, get_fieldvar_var(var));
			return;
		}
		case A_subscriptVar:{
			traverseVar(env, depth, get_subvar_var(var));
			traverseExp(env, depth, get_subvar_exp(var));
			return;
		}
	}
	assert(0);
}

void Esc_findEscape(A_exp exp) {
	//your code here
	S_table escape_env = S_empty();	
	traverseExp(escape_env, 0, exp);
}
