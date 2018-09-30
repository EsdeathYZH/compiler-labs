#include "prog1.h"
#include<stdio.h>
#include<string.h>
#include<stdlib.h>

typedef struct table *Table_;

int max(int,int);
int length(A_expList);
int maxargsExp(A_exp);
int maxargsExpList(A_expList);
int maxargs(A_stm);
Table_ Table(string, int, Table_);
struct IntAndTable interpExp(A_exp,Table_);
Table_ interpStm(A_stm,Table_);
int lookup(Table_,string);

struct table {string id; int value; Table_ tail;};
Table_ Table(string id, int value, Table_ tail){
	Table_ t = malloc(sizeof(*t));
	t->id = id;
	t->value = value;
	t->tail = tail;
	return t;
}

struct IntAndTable {int i;Table_ t;};

int maxargs(A_stm stm)
{
	if(stm->kind == A_compoundStm){
		return max(maxargs(stm->u.compound.stm1),maxargs(stm->u.compound.stm2));
	}else if(stm->kind == A_assignStm){
		return maxargsExp(stm->u.assign.exp);
	}else{ // stm->kind = A_printStm
		return max(length(stm->u.print.exps),maxargsExpList(stm->u.print.exps));
	}
	return 0;
}

int maxargsExpList(A_expList explist){
	if(explist->kind == A_lastExpList){
		return maxargsExp(explist->u.last);
	}else{ // explist->kind == A_pairExpList
		return max(maxargsExpList(explist->u.pair.tail),maxargsExp(explist->u.pair.head));
	}
}

int maxargsExp(A_exp exp){
	if(exp->kind == A_idExp || exp->kind == A_numExp){
		return 0;
	}else if(exp->kind == A_opExp){
		return max(maxargsExp(exp->u.op.left),maxargsExp(exp->u.op.right));
	}else{ //exp->kind == A_eseqExp
		return max(maxargs(exp->u.eseq.stm),maxargsExp(exp->u.eseq.exp));
	}
}

int max(int num1,int num2){
	return num1 > num2 ? num1 : num2;
}

int length(A_expList explist){
	A_expList templist = explist;
	int len = 1;
	while(templist->kind != A_lastExpList){
		templist = templist->u.pair.tail;
		len += 1;
	}
	return len;
}

void interp(A_stm stm)
{
	if(stm->kind == A_compoundStm){
		Table_ table1 = checked_malloc(sizeof(*table1));
		Table_ table2 = interpStm(stm->u.compound.stm1, table1);
		interpStm(stm->u.compound.stm2, table2);
	}else if(stm->kind == A_assignStm){
		Table_ table1 = checked_malloc(sizeof(*table1));
		interpExp(stm->u.assign.exp, table1);
	}else{ // stm->kind = A_printStm
		A_expList templist = stm->u.print.exps;
		Table_ table = checked_malloc(sizeof(*table));
		while(templist->kind != A_lastExpList){
			struct IntAndTable intAndTable = interpExp(templist->u.pair.head,table);
			printf("%d ",intAndTable.i);
			table = intAndTable.t;
			templist = templist->u.pair.tail;
		}
		printf("%d\n",interpExp(templist->u.last,table).i);
	}
}

Table_ interpStm(A_stm stm,Table_ table){
	if(stm->kind == A_compoundStm){
		Table_ table1 = interpStm(stm->u.compound.stm1, table);
		return interpStm(stm->u.compound.stm2, table1);
	}else if(stm->kind == A_assignStm){
		struct IntAndTable intAndTable = interpExp(stm->u.assign.exp,table);
		return Table(stm->u.assign.id, intAndTable.i, intAndTable.t);
	}else{ // stm->kind = A_printStm
		A_expList templist = stm->u.print.exps;
		while(templist->kind != A_lastExpList){
			struct IntAndTable intAndTable = interpExp(templist->u.pair.head,table);
			printf("%d ",intAndTable.i);
			table = intAndTable.t;
			templist = templist->u.pair.tail;
		}
		struct IntAndTable intAndTable = interpExp(templist->u.last,table);
		printf("%d\n",intAndTable.i);
		return intAndTable.t;
	}
}

struct IntAndTable interpExp(A_exp exp,Table_ table){
	if(exp->kind == A_idExp){
		struct IntAndTable* intAndTable = checked_malloc(sizeof(intAndTable));
		intAndTable->i = lookup(table,exp->u.id);
		intAndTable->t = table;
		return *intAndTable;
	}else if(exp->kind == A_numExp){
		struct IntAndTable* intAndTable = checked_malloc(sizeof(intAndTable));
		intAndTable->i = exp->u.num;
		intAndTable->t = table;
		return *intAndTable;
	}else if(exp->kind == A_opExp){
		struct IntAndTable intAndTable1 = interpExp(exp->u.op.left,table);
		struct IntAndTable intAndTable2 = interpExp(exp->u.op.right,intAndTable1.t);
		switch(exp->u.op.oper){
			case A_plus:
			  intAndTable2.i = intAndTable1.i + intAndTable2.i;
			  break;
			case A_minus:
			  intAndTable2.i = intAndTable1.i - intAndTable2.i;
			  break;
			case A_times:
			  intAndTable2.i = intAndTable1.i * intAndTable2.i;
			  break;
			case A_div:
			  intAndTable2.i = intAndTable1.i / intAndTable2.i;
			  break;
		}
		return intAndTable2;
	}else{ //exp->kind == A_eseqExp
		return interpExp(exp->u.eseq.exp,interpStm(exp->u.eseq.stm,table));
	}
}

int lookup(Table_ table,string id){
	Table_ tempTable = table;
	while(strcmp(tempTable->id,id)){
		tempTable = tempTable->tail;
	}
	return tempTable->value;
}
