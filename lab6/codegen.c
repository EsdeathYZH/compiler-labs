#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "symbol.h"
#include "absyn.h"
#include "temp.h"
#include "errormsg.h"
#include "tree.h"
#include "assem.h"
#include "frame.h"
#include "codegen.h"
#include "table.h"

#define TEMP_STR_LENGTH 100

extern Temp_temp F_RSP(void);
extern Temp_temp F_RAX(void);
extern Temp_temp F_RBP(void);
extern Temp_temp F_RBX(void);
extern Temp_temp F_RDI(void);
extern Temp_temp F_RSI(void);
extern Temp_temp F_RDX(void);
extern Temp_temp F_RCX(void);
extern Temp_temp F_R8(void);
extern Temp_temp F_R9(void);
extern Temp_temp F_R10(void);
extern Temp_temp F_R11(void);
extern Temp_temp F_R12(void);
extern Temp_temp F_R13(void);
extern Temp_temp F_R14(void);
extern Temp_temp F_R15(void);

static void munchStm(T_stm s);
static Temp_temp munchExp(T_exp e);
static Temp_tempList munchArgs(int index, T_expList args);

static AS_instrList iList = NULL, last = NULL;
static F_frame frame = NULL;
static void emit(AS_instr inst){
    if(last != NULL){
        last->tail = AS_InstrList(inst, NULL);
        last = last->tail;
    }else{
        iList = AS_InstrList(inst, NULL);
        last = iList;
    }
}

//Lab 6: put your code here
AS_instrList F_codegen(F_frame f, T_stmList stmList) {
    AS_instrList list;
    T_stmList sl;
    frame = f;
    char prolog[TEMP_STR_LENGTH], epilog[TEMP_STR_LENGTH];

    //stack change
    sprintf(prolog, " subq $?0#, %%rsp");
    emit(AS_Oper(String(prolog), Temp_TempList(F_RSP(), NULL), 
            Temp_TempList(F_RSP(), NULL), NULL));
    for(sl = stmList; sl; sl = sl->tail){
        munchStm(sl->head);
    }
    //stack restore
    sprintf(epilog, " addq $?0#, %%rsp");
    emit(AS_Oper(String(epilog), Temp_TempList(F_RSP(), NULL), 
            Temp_TempList(F_RSP(), NULL), NULL));

    list = iList;
    last = NULL;
    iList = last;

    //add returnsink
    iList = F_procEntryExit2(f, iList);
    frame = NULL;
    return list;
}

static void munchStm(T_stm s){
    char str[TEMP_STR_LENGTH];
    switch(s->kind){
        case T_MOVE:{
            T_exp dst = s->u.MOVE.dst, src = s->u.MOVE.src;
            if(dst->kind == T_MEM){
                /* MOVE(MEM(BINOP(PLUS,e1,CONST(i))),e2) */
                /* movq %rdi, -24(%rbp)*/
                if(dst->u.MEM->kind == T_BINOP
                    && dst->u.MEM->u.BINOP.op == T_plus
                    && dst->u.MEM->u.BINOP.right->kind == T_CONST){
                    T_exp e1 = dst->u.MEM->u.BINOP.left, e2 = src;
                    Temp_temp e2temp = munchExp(e2);
                    Temp_temp possible_fp = munchExp(e1);
                    if(possible_fp == F_FP()){
                        sprintf(str, " movq `s1, ?%d#(`s0)", dst->u.MEM->u.BINOP.right->u.CONST);
                    }else{
                        sprintf(str, " movq `s1, %d(`s0)", dst->u.MEM->u.BINOP.right->u.CONST);
                    }
                    emit(AS_Oper(String(str),NULL,
                        Temp_TempList(possible_fp, Temp_TempList(e2temp, NULL)), NULL));
                }
                /* MOVE(MEM(BINOP(PLUS,CONST(i),e1)),e2) */
                /* movq %rdi, -24(%rbp)*/
                else if(dst->u.MEM->kind == T_BINOP
                        && dst->u.MEM->u.BINOP.op == T_plus
                        && dst->u.MEM->u.BINOP.left->kind == T_CONST){
                    T_exp e1 = dst->u.MEM->u.BINOP.right, e2 = src;
                    Temp_temp possible_fp = munchExp(e1);
                    if(possible_fp == F_FP()){
                        sprintf(str, " movq `s1, ?%d#(`s0)", dst->u.MEM->u.BINOP.left->u.CONST);
                    }else{
                        sprintf(str, " movq `s1, %d(`s0)", dst->u.MEM->u.BINOP.left->u.CONST);
                    }
                    emit(AS_Oper(String(str), NULL,
                        Temp_TempList(possible_fp, Temp_TempList(munchExp(e2), NULL)), NULL));
                }
                /* MOVE(MEM(e1), MEM(e2)) */
                else if(src->kind == T_MEM){//TODO: a very naive solution:can be optimized
                    T_exp e1 = dst->u.MEM, e2 = src->u.MEM;
                    //char str[TEMP_STR_LENGTH] = " movq (`s2), `s0\n movq `s0, (`s1)\n";
                    Temp_temp temp = Temp_newtemp();
                    Temp_temp possible_fp1 = munchExp(e2), possible_fp2 = munchExp(e1);
                    if(possible_fp1 == F_FP()){
                        sprintf(str, " movq ?0#(`s0), `d0");
                    }else{
                        sprintf(str, " movq (`s0), `d0");
                    }
                    emit(AS_Oper(String(str), Temp_TempList(temp, NULL), 
                        Temp_TempList(possible_fp1, NULL), NULL));
                    if(possible_fp2 == F_FP()){
                        sprintf(str, " movq `s0, ?0#(`s1)");
                    }else{
                        sprintf(str, " movq `s0, (`s1)");
                    }
                    emit(AS_Oper(String(str), NULL,
                        Temp_TempList(temp, Temp_TempList(possible_fp2, NULL)), NULL));
                }
                /* MOVE(MEM(CONST(i)),e2) */
                else if(dst->u.MEM->kind == T_CONST){
                    T_exp e2 = src->u.MEM;
                    sprintf(str, " movq `s0, %d", dst->u.MEM->u.CONST);
                    emit(AS_Oper(String(str), NULL,
                        Temp_TempList(munchExp(e2), NULL), NULL));
                }
                /* MOVE(MEM(e1),e2) */
                else{
                    T_exp e1 = dst->u.MEM, e2 = src;
                    Temp_temp possible_fp = munchExp(e1);
                    if(possible_fp == F_FP()){
                        sprintf(str, " movq `s1, ?0#(`s0)");
                    }else{
                        sprintf(str, " movq `s1, (`s0)");
                    }
                    emit(AS_Oper(String(str), NULL,
                        Temp_TempList(possible_fp, Temp_TempList(munchExp(e2), NULL)), NULL));
                }
            }
            /* MOVE(TEMP(i),e2) */
            else if(dst->kind == T_TEMP){
                T_exp e2 = src;
                Temp_temp possible_fp = munchExp(e2);
                if(possible_fp == F_FP()){
                    sprintf(str," leaq ?0#(`s0), `d0");
                }else{
                    sprintf(str, " movq `s0, `d0");
                }
                emit(AS_Move(String(str), Temp_TempList(dst->u.TEMP, NULL), Temp_TempList(possible_fp, NULL)));
            }
            else assert(0);
            break;
        }
        case T_JUMP:{
            char str[TEMP_STR_LENGTH] = " jmp  `j0";
            emit(AS_Oper(String(str), NULL, NULL, AS_Targets(s->u.JUMP.jumps)));
            break;
        }
        case T_CJUMP:{
            //T_eq, T_ne, T_lt, T_gt, T_le, T_ge,T_ult, T_ule, T_ugt, T_uge
            char str[TEMP_STR_LENGTH];
            char jump_str[10];
            switch(s->u.CJUMP.op){
                case T_eq:{
                    strcpy(jump_str, "je");
                    break;
                }
                case T_ne:{
                    strcpy(jump_str, "jne");
                    break;
                }
                case T_lt:{
                    strcpy(jump_str, "jl");
                    break;
                }
                case T_gt:{
                    strcpy(jump_str, "jg");
                    break;
                }
                case T_le:{
                    strcpy(jump_str, "jle");
                    break;
                }
                case T_ge:{
                    strcpy(jump_str, "jge");
                    break;
                }
                case T_ult:{
                    strcpy(jump_str, "jb");
                    break;
                }
                case T_ule:{
                    strcpy(jump_str, "jbe");
                    break;
                }
                case T_ugt:{
                    strcpy(jump_str, "ja");
                    break;
                }
                case T_uge:{
                    strcpy(jump_str, "jae");
                    break;
                }
                default:
                    assert(0);
            }
            sprintf(str, " cmpq `s1, `s0");
            emit(AS_Oper(String(str), NULL, 
                    Temp_TempList(munchExp(s->u.CJUMP.left), Temp_TempList(munchExp(s->u.CJUMP.right), NULL)), NULL));
            sprintf(str, " %s  `j0", jump_str);
            emit(AS_Oper(String(str), NULL, NULL,
                    AS_Targets(Temp_LabelList(s->u.CJUMP.true, Temp_LabelList(s->u.CJUMP.false, NULL)))));
            break;
        }
        case T_LABEL:{
            char str[TEMP_STR_LENGTH];
            sprintf(str, "%s", Temp_labelstring(s->u.LABEL));
            emit(AS_Label(String(str), s->u.LABEL));
            break;
        }
        case T_SEQ:{
            munchStm(s->u.SEQ.left);
            munchStm(s->u.SEQ.right);
            break;
        }
        case T_EXP:{
            munchExp(s->u.EXP);
            break;
        }
        default:
            assert(0);
    }
}


static Temp_temp munchExp(T_exp e){
    char str[TEMP_STR_LENGTH];
    switch(e->kind){
        case T_MEM:{
            Temp_temp temp = Temp_newtemp();
            if(e->u.MEM->kind == T_BINOP
                && e->u.MEM->u.BINOP.op == T_plus
                && e->u.MEM->u.BINOP.right->kind == T_CONST){
                T_exp e1 = e->u.MEM->u.BINOP.left;
                Temp_temp possible_fp = munchExp(e1);
                if(possible_fp == F_FP()){
                    sprintf(str, " movq ?%d#(`s0), `d0", e->u.MEM->u.BINOP.right->u.CONST);
                }else{
                    sprintf(str, " movq %d(`s0), `d0", e->u.MEM->u.BINOP.right->u.CONST);
                }
                emit(AS_Oper(String(str), Temp_TempList(temp, NULL), Temp_TempList(possible_fp, NULL), NULL));
            }
            else if(e->u.MEM->kind == T_BINOP
                && e->u.MEM->u.BINOP.op == T_plus
                && e->u.MEM->u.BINOP.left->kind == T_CONST){
                T_exp e1 = e->u.MEM->u.BINOP.right;
                Temp_temp possible_fp = munchExp(e1);
                if(possible_fp == F_FP()){
                    sprintf(str, " movq ?%d#(`s0), `d0", e->u.MEM->u.BINOP.left->u.CONST);
                }else{
                    sprintf(str, " movq %d(`s0), `d0", e->u.MEM->u.BINOP.left->u.CONST);
                }
                emit(AS_Oper(String(str), Temp_TempList(temp, NULL), Temp_TempList(possible_fp, NULL), NULL));
            }
            else if(e->u.MEM->kind == T_CONST){
                sprintf(str, " movq %d, `d0", e->u.MEM->u.CONST);
                emit(AS_Oper(String(str), Temp_TempList(temp, NULL), NULL, NULL));
            }
            else{
                T_exp e1 = e->u.MEM;
                Temp_temp possible_fp = munchExp(e1);
                if(possible_fp == F_FP()){
                    sprintf(str, " movq ?0#(`s0), `d0");
                }else{
                    sprintf(str, " movq (`s0), `d0");
                }
                emit(AS_Oper(String(str), Temp_TempList(temp, NULL), Temp_TempList(possible_fp, NULL), NULL));
            }
            return temp;
        }
        case T_BINOP:{
            switch(e->u.BINOP.op){
                case T_plus:{
                    Temp_temp temp = Temp_newtemp();
                    if(e->u.BINOP.right->kind == T_CONST){
                        T_exp e1 = e->u.BINOP.left;
                        Temp_temp possible_fp = munchExp(e1);
                        if(possible_fp == F_FP()){
                            sprintf(str," leaq ?0#(`s0), `d0");
                        }else{
                            sprintf(str, " movq `s0, `d0");
                        }
                        emit(AS_Move(String(str), Temp_TempList(temp, NULL), 
                            Temp_TempList(possible_fp, NULL)));
                        sprintf(str, " addq $%d, `d0", e->u.BINOP.right->u.CONST);
                        emit(AS_Oper(String(str), Temp_TempList(temp, NULL),
                            Temp_TempList(temp, NULL), NULL));
                    }else if(e->u.BINOP.left->kind == T_CONST){
                        T_exp e1 = e->u.BINOP.right;
                        Temp_temp possible_fp = munchExp(e1);
                        if(possible_fp == F_FP()){
                            sprintf(str," leaq ?0#(`s0), `d0");
                        }else{
                            sprintf(str, " movq `s0, `d0");
                        }
                        emit(AS_Move(String(str), Temp_TempList(temp, NULL), 
                            Temp_TempList(possible_fp, NULL)));
                        sprintf(str, " addq $%d, `d0", e->u.BINOP.left->u.CONST);
                        emit(AS_Oper(String(str), Temp_TempList(temp, NULL),
                            Temp_TempList(temp, NULL), NULL));
                    }else{
                        T_exp e1 = e->u.BINOP.left, e2 = e->u.BINOP.right;
                        Temp_temp possible_fp = munchExp(e1);
                        if(possible_fp == F_FP()){
                            sprintf(str," leaq ?0#(`s0), `d0");
                        }else{
                            sprintf(str, " movq `s0, `d0");
                        }
                        emit(AS_Move(String(str), Temp_TempList(temp, NULL),
                            Temp_TempList(possible_fp, NULL)));
                        sprintf(str, " addq `s1, `d0");
                        emit(AS_Oper(String(str), Temp_TempList(temp, NULL),
                            Temp_TempList(temp, Temp_TempList(munchExp(e2), NULL)), NULL));
                    }
                    return temp;
                }
                case T_minus:{
                    Temp_temp temp = Temp_newtemp();
                    if(e->u.BINOP.right->kind == T_CONST){
                        T_exp e1 = e->u.BINOP.left;
                        char str[TEMP_STR_LENGTH];
                        Temp_temp possible_fp = munchExp(e1);
                        if(possible_fp == F_FP()){
                            sprintf(str," leaq ?0#(`s0), `d0");
                        }else{
                            sprintf(str, " movq `s0, `d0");
                        }
                        emit(AS_Move(String(str), Temp_TempList(temp, NULL), 
                            Temp_TempList(possible_fp, NULL)));
                        sprintf(str, " subq $%d, `d0", e->u.BINOP.right->u.CONST);
                        emit(AS_Oper(String(str), Temp_TempList(temp, NULL), NULL, NULL));
                    }else{
                        T_exp e1 = e->u.BINOP.left, e2 = e->u.BINOP.right;
                        Temp_temp possible_fp = munchExp(e1);
                        if(possible_fp == F_FP()){
                            sprintf(str," leaq ?0#(`s0), `d0");
                        }else{
                            sprintf(str, " movq `s0, `d0");
                        }
                        emit(AS_Move(String(str), Temp_TempList(temp, NULL), Temp_TempList(possible_fp, NULL)));
                        sprintf(str, " subq `s1, `d0");
                        emit(AS_Oper(String(str), Temp_TempList(temp, NULL),
                            Temp_TempList(temp, Temp_TempList(munchExp(e2), NULL)), NULL));
                    }
                    return temp;
                }
                case T_mul:{
                    Temp_temp temp = Temp_newtemp();
                    //Temp_temp saveRDX = Temp_newtemp();
                    T_exp e1 = e->u.BINOP.left, e2 = e->u.BINOP.right;
                    //save rdx
                    // sprintf(str, " movq %%rdx, `d0");
                    // emit(AS_Move(String(str), Temp_TempList(saveRDX, NULL), 
                    //     Temp_TempList(F_RDX(),NULL)));
                    sprintf(str, " movq `s0, %%rax");
                    emit(AS_Move(String(str), Temp_TempList(F_RAX(), NULL), 
                        Temp_TempList(munchExp(e1),NULL)));
                    //multiple operation
                    sprintf(str, " imulq `s0");
                    emit(AS_Oper(String(str), Temp_TempList(F_RAX(), Temp_TempList(F_RDX(), NULL)),
                        Temp_TempList(munchExp(e2), Temp_TempList(F_RAX(), NULL)), NULL));
                    //restore rdx
                    // sprintf(str, " movq `s0, %%rdx");
                    // emit(AS_Move(String(str), Temp_TempList(F_RDX(),NULL), Temp_TempList(saveRDX, NULL)));
                    sprintf(str, " movq %%rax, `d0");
                    emit(AS_Move(String(str), Temp_TempList(temp, NULL),
                        Temp_TempList(F_RAX(), NULL)));
                    return temp;
                }
                case T_div:{
                    Temp_temp temp = Temp_newtemp();
                    //Temp_temp saveRDX = Temp_newtemp();
                    T_exp e1 = e->u.BINOP.left, e2 = e->u.BINOP.right;
                    //save rdx
                    // sprintf(str, " movq %%rdx, `d0");
                    // emit(AS_Move(String(str), Temp_TempList(saveRDX, NULL), 
                    //     Temp_TempList(F_RDX(),NULL)));
                    sprintf(str, " movq `s0, %%rax");
                    emit(AS_Move(String(str), Temp_TempList(F_RAX(), NULL), 
                        Temp_TempList(munchExp(e1),NULL)));
                    //clear rdx
                    sprintf(str, " cqto");
                    emit(AS_Oper(String(str), Temp_TempList(F_RAX(), Temp_TempList(F_RDX(), NULL)),
                        Temp_TempList(F_RAX(), NULL), NULL));
                    //div operation  orz...
                    sprintf(str, " idivq `s0");
                    emit(AS_Oper(String(str), Temp_TempList(F_RAX(), Temp_TempList(F_RDX(), NULL)),
                        Temp_TempList(munchExp(e2), Temp_TempList(F_RAX(), Temp_TempList(F_RDX(), NULL))), NULL));
                    //restore rdx
                    // sprintf(str, " movq `s0, %%rdx");
                    // emit(AS_Move(String(str), Temp_TempList(F_RDX(),NULL), Temp_TempList(saveRDX, NULL)));
                    sprintf(str, " movq %rax, `d0");
                    emit(AS_Move(String(str), Temp_TempList(temp, NULL),
                        Temp_TempList(F_RAX(), NULL)));
                    return temp;
                }
                case T_and:{
                    Temp_temp temp = Temp_newtemp();
                    if(e->u.BINOP.right->kind == T_CONST){
                        T_exp e1 = e->u.BINOP.left;
                        char str[TEMP_STR_LENGTH];
                        Temp_temp possible_fp = munchExp(e1);
                        if(possible_fp == F_FP()){
                            sprintf(str," leaq ?0#(`s0), `d0");
                        }else{
                            sprintf(str, " movq `s0, `d0");
                        }
                        emit(AS_Move(String(str), Temp_TempList(temp, NULL), 
                            Temp_TempList(possible_fp, NULL)));
                        sprintf(str, " andq $%d, `d0", e->u.BINOP.right->u.CONST);
                        emit(AS_Oper(String(str), Temp_TempList(temp, NULL),
                            Temp_TempList(temp, NULL), NULL));
                    }else if(e->u.BINOP.left->kind == T_CONST){
                        T_exp e1 = e->u.BINOP.right;
                        char str[TEMP_STR_LENGTH];
                        Temp_temp possible_fp = munchExp(e1);
                        if(possible_fp == F_FP()){
                            sprintf(str," leaq ?0#(`s0), `d0");
                        }else{
                            sprintf(str, " movq `s0, `d0");
                        }
                        emit(AS_Move(String(str), Temp_TempList(temp, NULL), 
                            Temp_TempList(possible_fp, NULL)));
                        sprintf(str, " andq $%d, `d0", e->u.BINOP.left->u.CONST);
                        emit(AS_Oper(String(str), Temp_TempList(temp, NULL),
                            Temp_TempList(temp, NULL), NULL));
                    }else{
                        T_exp e1 = e->u.BINOP.left, e2 = e->u.BINOP.right;
                        Temp_temp possible_fp = munchExp(e1);
                        if(possible_fp == F_FP()){
                            sprintf(str," leaq ?0#(`s0), `d0");
                        }else{
                            sprintf(str, " movq `s0, `d0");
                        }
                        emit(AS_Move(String(str), Temp_TempList(temp, NULL),
                            Temp_TempList(possible_fp, NULL)));
                        sprintf(str, " andq `s1, `d0");
                        emit(AS_Oper(String(str), Temp_TempList(temp, NULL),
                            Temp_TempList(temp, Temp_TempList(munchExp(e2), NULL)), NULL));
                    }
                    return temp;
                }
                case T_or:{
                    Temp_temp temp = Temp_newtemp();
                    if(e->u.BINOP.right->kind == T_CONST){
                        T_exp e1 = e->u.BINOP.left;
                        char str[TEMP_STR_LENGTH];
                        Temp_temp possible_fp = munchExp(e1);
                        if(possible_fp == F_FP()){
                            sprintf(str," leaq ?0#(`s0), `d0");
                        }else{
                            sprintf(str, " movq `s0, `d0");
                        }
                        emit(AS_Move(String(str), Temp_TempList(temp, NULL), 
                            Temp_TempList(possible_fp, NULL)));
                        sprintf(str, " orq $%d, `d0", e->u.BINOP.right->u.CONST);
                        emit(AS_Oper(String(str), Temp_TempList(temp, NULL),
                            Temp_TempList(temp, NULL), NULL));
                    }else if(e->u.BINOP.left->kind == T_CONST){
                        T_exp e1 = e->u.BINOP.right;
                        char str[TEMP_STR_LENGTH];
                        Temp_temp possible_fp = munchExp(e1);
                        if(possible_fp == F_FP()){
                            sprintf(str," leaq ?0#(`s0), `d0");
                        }else{
                            sprintf(str, " movq `s0, `d0");
                        }
                        emit(AS_Move(String(str), Temp_TempList(temp, NULL), 
                            Temp_TempList(possible_fp, NULL)));
                        sprintf(str, " orq $%d, `d0", e->u.BINOP.left->u.CONST);
                        emit(AS_Oper(String(str), Temp_TempList(temp, NULL),
                            Temp_TempList(temp, NULL), NULL));
                    }else{
                        T_exp e1 = e->u.BINOP.left, e2 = e->u.BINOP.right;
                        Temp_temp possible_fp = munchExp(e1);
                        if(possible_fp == F_FP()){
                            sprintf(str," leaq ?0#(`s0), `d0");
                        }else{
                            sprintf(str, " movq `s0, `d0");
                        }
                        emit(AS_Move(String(str), Temp_TempList(temp, NULL),
                            Temp_TempList(possible_fp, NULL)));
                        sprintf(str, " orq `s1, `d0");
                        emit(AS_Oper(String(str), Temp_TempList(temp, NULL),
                            Temp_TempList(temp, Temp_TempList(munchExp(e2), NULL)), NULL));
                    }
                    return temp;
                }
                case T_lshift:{
                    Temp_temp temp = Temp_newtemp();
                    if(e->u.BINOP.right->kind != T_CONST){
                        assert(0);
                    }
                    T_exp e1 = e->u.BINOP.left;
                    char str[TEMP_STR_LENGTH];
                    Temp_temp possible_fp = munchExp(e1);
                    if(possible_fp == F_FP()){
                        sprintf(str," leaq ?0#(`s0), `d0");
                    }else{
                        sprintf(str, " movq `s0, `d0");
                    }
                    emit(AS_Move(String(str), Temp_TempList(temp, NULL),
                        Temp_TempList(possible_fp, NULL)));
                    sprintf(str, " shlq $%d, `d0", e->u.BINOP.right->u.CONST);
                    emit(AS_Oper(String(str), Temp_TempList(temp, NULL),
                        Temp_TempList(temp, NULL), NULL));
                    return temp;
                }
                case T_rshift:{
                    Temp_temp temp = Temp_newtemp();
                    if(e->u.BINOP.right->kind != T_CONST){
                        assert(0);
                    }
                    T_exp e1 = e->u.BINOP.left;
                    char str[TEMP_STR_LENGTH];
                    Temp_temp possible_fp = munchExp(e1);
                    if(possible_fp == F_FP()){
                        sprintf(str," leaq ?0#(`s0), `d0");
                    }else{
                        sprintf(str, " movq `s0, `d0");
                    }
                    emit(AS_Move(String(str), Temp_TempList(temp, NULL),
                        Temp_TempList(possible_fp, NULL)));
                    sprintf(str, " shrq $%d, `d0", e->u.BINOP.right->u.CONST);
                    emit(AS_Oper(String(str), Temp_TempList(temp, NULL),
                        Temp_TempList(temp, NULL), NULL));
                    return temp;
                }
                case T_arshift:{
                    Temp_temp temp = Temp_newtemp();
                    if(e->u.BINOP.right->kind != T_CONST){
                        assert(0);
                    }
                    T_exp e1 = e->u.BINOP.left;
                    char str[TEMP_STR_LENGTH];
                    Temp_temp possible_fp = munchExp(e1);
                    if(possible_fp == F_FP()){
                        sprintf(str," leaq ?0#(`s0), `d0");
                    }else{
                        sprintf(str, " movq `s0, `d0");
                    }
                    emit(AS_Move(String(str), Temp_TempList(temp, NULL),
                        Temp_TempList(possible_fp, NULL)));
                    sprintf(str, " sarq $%d, `d0", e->u.BINOP.right->u.CONST);
                    emit(AS_Oper(String(str), Temp_TempList(temp, NULL),
                        Temp_TempList(temp, NULL), NULL));
                    return temp;
                }
                case T_xor:{
                    Temp_temp temp = Temp_newtemp();
                    if(e->u.BINOP.right->kind == T_CONST){
                        T_exp e1 = e->u.BINOP.left;
                        char str[TEMP_STR_LENGTH];
                        Temp_temp possible_fp = munchExp(e1);
                        if(possible_fp == F_FP()){
                            sprintf(str," leaq ?0#(`s0), `d0");
                        }else{
                            sprintf(str, " movq `s0, `d0");
                        }
                        emit(AS_Move(String(str), Temp_TempList(temp, NULL), 
                            Temp_TempList(possible_fp, NULL)));
                        sprintf(str, " xorq $%d, `d0", e->u.BINOP.right->u.CONST);
                        emit(AS_Oper(String(str), Temp_TempList(temp, NULL),
                            Temp_TempList(temp, NULL), NULL));
                    }else if(e->u.BINOP.left->kind == T_CONST){
                        T_exp e1 = e->u.BINOP.right;
                        char str[TEMP_STR_LENGTH];
                        Temp_temp possible_fp = munchExp(e1);
                        if(possible_fp == F_FP()){
                            sprintf(str," leaq ?0#(`s0), `d0");
                        }else{
                            sprintf(str, " movq `s0, `d0");
                        }
                        emit(AS_Move(String(str), Temp_TempList(temp, NULL), 
                            Temp_TempList(possible_fp, NULL)));
                        sprintf(str, " xorq $%d, `d0", e->u.BINOP.left->u.CONST);
                        emit(AS_Oper(String(str), Temp_TempList(temp, NULL),
                            Temp_TempList(temp, NULL), NULL));
                    }else{
                        T_exp e1 = e->u.BINOP.left, e2 = e->u.BINOP.right;
                        Temp_temp possible_fp = munchExp(e1);
                        if(possible_fp == F_FP()){
                            sprintf(str," leaq ?0#(`s0), `d0");
                        }else{
                            sprintf(str, " movq `s0, `d0");
                        }
                        emit(AS_Move(String(str), Temp_TempList(temp, NULL),
                            Temp_TempList(possible_fp, NULL)));
                        sprintf(str, " xorq `s1, `d0");
                        emit(AS_Oper(String(str), Temp_TempList(temp, NULL),
                            Temp_TempList(temp, Temp_TempList(munchExp(e2), NULL)), NULL));
                    }
                    return temp;
                }
            }
        }
        case T_CONST:{
            Temp_temp temp = Temp_newtemp();
            sprintf(str, " movq $%d, `d0", e->u.CONST);
            emit(AS_Oper(String(str), Temp_TempList(temp, NULL), NULL, NULL));
            return temp;
        }
        case T_TEMP:{
            return e->u.TEMP;
        }
        case T_NAME:{ 
            Temp_temp temp = Temp_newtemp();
            sprintf(str, " leaq %s(%%rip), `d0", Temp_labelstring(e->u.NAME));
            emit(AS_Oper(String(str), Temp_TempList(temp, NULL), NULL, NULL));
            return temp;
        }
        case T_ESEQ:{
            munchStm(e->u.ESEQ.stm);
            return munchExp(e->u.ESEQ.exp);
        }
        case T_CALL:{
            //Temp_temp r = munchExp(e->u.CALL.fun);
            //generate static link
            Temp_temp temp = Temp_newtemp();
            //这里没有判断是不是rsp的原因是发现有的时候会生成先把rsp move到一个temp，然后再用那个temp的情况
            if(e->u.CALL.args->head->kind == T_TEMP ){
                sprintf(str, " leaq ?0#(%%rsp), `d0");
                emit(AS_Oper(String(str), Temp_TempList(temp, NULL),
                        Temp_TempList(F_RSP(), NULL), NULL));
                sprintf(str, " movq `s0, (%%rsp)");
                emit(AS_Oper(String(str), NULL, Temp_TempList(temp, Temp_TempList(F_RSP(), NULL)), NULL));
            }else{
                sprintf(str, " movq `s0, (%%rsp)");
                emit(AS_Oper(String(str), NULL, Temp_TempList(munchExp(e->u.CALL.args->head), Temp_TempList(F_RSP(), NULL)), NULL));
            }
            //skip static link
            Temp_tempList l = munchArgs(0, e->u.CALL.args->tail);
            sprintf(str, " callq %s", Temp_labelstring(e->u.CALL.fun->u.NAME));
            emit(AS_Oper(String(str), Temp_unionList(argRegs(), Temp_TempList(F_RV(), callerSaves())), l, NULL));
            // return %rax
            return F_RV();
        }
        default:
            assert(0);
    }
}

static Temp_tempList munchArgs(int index, T_expList args){
    char str[TEMP_STR_LENGTH];
    if(!args) return NULL;
    if(index < 6){ // argument should be store in register first
        int tempIndex = index;
        Temp_tempList argregs = argRegs();
        while(tempIndex > 0){
            argregs = argregs->tail;
            tempIndex--;
        }
        sprintf(str, " movq `s0, `d0");
        emit(AS_Move(String(str), Temp_TempList(argregs->head, NULL), Temp_TempList(munchExp(args->head), NULL)));
        //return argument registers
        return Temp_TempList(argregs->head, munchArgs(index+1, args->tail));
    }else{
        sprintf(str, " movq `s0, %d(%%rsp)", (index-5)*F_wordSize);
        emit(AS_Move(String(str), NULL, Temp_TempList(munchExp(args->head), Temp_TempList(F_RSP(), NULL))));
        return munchArgs(index+1, args->tail);                                     
    }
}