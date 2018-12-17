#include <stdio.h>
#include <stdlib.h>
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

extern Temp_temp F_RAX();
extern Temp_temp F_RDX();

static AS_instrList iList = NULL, last = NULL;
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

    for(sl = stmList; sl; sl = sl->tail){
        munchStm(sl->head);
    }
    list = iList;
    last = NULL;
    iList = last;
    return list;
}

static void munchStm(T_stm s){
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
                    char str[TEMP_STR_LENGTH];
                    sprintf(str, " movq `s1, %d(`s0)\n", dst->u.MEM->u.BINOP.right->u.CONST);
                    emit(AS_Oper(String(str),NULL,
                        Temp_TempList(munchExp(e1), Temp_TempList(munchExp(e2), NULL)), NULL));
                }
                /* MOVE(MEM(BINOP(PLUS,CONST(i),e1)),e2) */
                /* movq %rdi, -24(%rbp)*/
                else if(dst->u.MEM->kind == T_BINOP
                        && dst->u.MEM->u.BINOP.op == T_plus
                        && dst->u.MEM->u.BINOP.left->kind == T_CONST){
                    T_exp e1 = dst->u.MEM->u.BINOP.right, e2 = src;
                    char str[TEMP_STR_LENGTH];
                    sprintf(str, " movq `s1, %d(`s0)\n", dst->u.MEM->u.BINOP.left->u.CONST);
                    emit(AS_Oper(String(str), NULL,
                        Temp_TempList(munchExp(e1), Temp_TempList(munchExp(e2), NULL)), NULL));
                }
                /* MOVE(MEM(e1), MEM(e2)) */
                else if(src->kind == T_MEM){//TODO: a very naive solution:can be optimized
                    T_exp e1 = dst->u.MEM, e2 = src->u.MEM;
                    char str[TEMP_STR_LENGTH] = " movq (`s2), `s0\n movq `s0, (`s1)\n";
                    emit(AS_Oper(String(str), NULL,
                        Temp_TempList(Temp_newtemp(), Temp_TempList(munchExp(e1), Temp_TempList(munchExp(e2), NULL))), NULL));
                }
                /* MOVE(MEM(CONST(i)),e2) */
                else if(dst->u.MEM->kind == T_CONST){
                    T_exp e2 = src->u.MEM;
                    char str[TEMP_STR_LENGTH];
                    sprintf(str, " movq `s0, %d\n", dst->u.MEM->u.CONST);
                    emit(AS_Oper(String(str), NULL,
                        Temp_TempList(munchExp(e2), NULL), NULL));
                }
                /* MOVE(MEM(e1),e2) */
                else{
                    T_exp e1 = dst->u.MEM, e2 = src;
                    char str[TEMP_STR_LENGTH] = " movq `s1, (`s0)\n";
                    emit(AS_Oper(String(str), NULL,
                        Temp_TempList(munchExp(e1), Temp_TempList(munchExp(e2), NULL)), NULL));
                }
            }
            /* MOVE(TEMP(i),e2) */
            else if(dst->kind == T_TEMP){
                T_exp e2 = src;
                char str[TEMP_STR_LENGTH] = " movq `s0, `d0\n";
                emit(AS_Move(String(str), Temp_TempList(dst->u.TEMP, NULL), Temp_TempList(munchExp(e2), NULL)));
            }
            else assert(0);
            break;
        }
        case T_JUMP:{
            char str[TEMP_STR_LENGTH] = " jmp  `j0\n";
            emit(AS_Oper(String(str), NULL, NULL, AS_Targets(s->u.JUMP.jumps)));
            //TODO: not completed
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
            sprintf(str, " %s  `j0\n", jump_str);
            //TODO: not completed!!
            emit(AS_Oper(String(" cmpq `s0, `s1\n"), NULL, 
                    Temp_TempList(munchExp(s->u.CJUMP.left), Temp_TempList(s->u.CJUMP.right, NULL)), NULL));
            emit(AS_Oper(String(str), NULL, NULL,
            //TODO:s->u.CJUMP.false here?
                    AS_Targets(Temp_LabelList(s->u.CJUMP.true, s->u.CJUMP.false))));
            break;
        }
        case T_LABEL:{
            char str[TEMP_STR_LENGTH];
            sprintf(str, "%s:\n", Temp_labelstring(s->u.LABEL));
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
    switch(e->kind){
        case T_MEM:{
            Temp_temp temp = Temp_newtemp();
            if(e->u.MEM->kind == T_BINOP
                && e->u.MEM->u.BINOP.op == T_plus
                && e->u.MEM->u.BINOP.right->kind == T_CONST){
                T_exp e1 = e->u.MEM->u.BINOP.left;
                char str[TEMP_STR_LENGTH];
                sprintf(str, " movq %d(`s0), `d0\n", e->u.MEM->u.BINOP.right->u.CONST);
                emit(AS_Oper(String(str), Temp_TempList(temp, NULL), Temp_TempList(munchExp(e1), NULL), NULL));
            }
            else if(e->u.MEM->kind == T_BINOP
                && e->u.MEM->u.BINOP.op == T_plus
                && e->u.MEM->u.BINOP.left->kind == T_CONST){
                T_exp e1 = e->u.MEM->u.BINOP.right;
                char str[TEMP_STR_LENGTH];
                sprintf(str, " movq %d(`s0), `d0\n", e->u.MEM->u.BINOP.left->u.CONST);
                emit(AS_Oper(String(str), Temp_TempList(temp, NULL), Temp_TempList(munchExp(e1), NULL), NULL));
            }
            else if(e->u.MEM->kind == T_CONST){
                char str[TEMP_STR_LENGTH];
                sprintf(str, " movq %d, `d0\n", e->u.MEM->u.CONST);
                emit(AS_Oper(String(str), Temp_TempList(temp, NULL), NULL, NULL));
            }
            else{
                T_exp e1 = e->u.MEM;
                char str[TEMP_STR_LENGTH] = " movq (`s0), `d0\n";
                emit(AS_Oper(String(str), Temp_TempList(temp, NULL), Temp_TempList(munchExp(e1), NULL), NULL));
            }
            return temp;
        }
        case T_BINOP:{
            switch(e->u.BINOP.op){
                case T_plus:{
                    Temp_temp temp = Temp_newtemp();
                    //TODO: can we use leaq here?
                    if(e->u.BINOP.right->kind == T_CONST){
                        T_exp e1 = e->u.BINOP.left;
                        char str[TEMP_STR_LENGTH];
                        emit(AS_Move(String(" movq `s0, `d0"), Temp_TempList(temp, NULL), 
                            Temp_TempList(munchExp(e1), NULL)));
                        sprintf(str, " addq $%d, `d0\n", e->u.BINOP.right->u.CONST);
                        emit(AS_Oper(String(str), Temp_TempList(temp, NULL),
                            Temp_TempList(temp, NULL), NULL));
                    }else if(e->u.BINOP.left->kind == T_CONST){
                        T_exp e1 = e->u.BINOP.right;
                        char str[TEMP_STR_LENGTH];
                        emit(AS_Move(String(" movq `s0, `d0"), Temp_TempList(temp, NULL), 
                            Temp_TempList(munchExp(e1), NULL)));
                        sprintf(str, " addq $%d, `d0\n", e->u.BINOP.left->u.CONST);
                        emit(AS_Oper(String(str), Temp_TempList(temp, NULL),
                            Temp_TempList(temp, NULL), NULL));
                    }else{
                        T_exp e1 = e->u.BINOP.left, e2 = e->u.BINOP.right;
                        emit(AS_Move(String(" movq `s0, `d0\n"), Temp_TempList(temp, NULL),
                            Temp_TempList(munchExp(e1), NULL)));
                        emit(AS_Oper(String(" addq `s1, `d0\n"), Temp_TempList(temp, NULL),
                            Temp_TempList(temp, Temp_TempList(munchExp(e2), NULL)), NULL));
                    }
                    return temp;
                }
                case T_minus:{
                    Temp_temp temp = Temp_newtemp();
                    //TODO: can we use leaq here?
                    if(e->u.BINOP.right->kind == T_CONST){
                        T_exp e1 = e->u.BINOP.left;
                        char str[TEMP_STR_LENGTH];
                        emit(AS_Move(String(" movq `s0, `d0\n"), Temp_TempList(temp, NULL), 
                            Temp_TempList(munchExp(e1), NULL)));
                        sprintf(str, " subq $%d, `d0\n", e->u.BINOP.right->u.CONST);
                        emit(AS_Oper(String(str), Temp_TempList(temp, NULL), NULL, NULL));
                    }else{
                        T_exp e1 = e->u.BINOP.left, e2 = e->u.BINOP.right;
                        emit(AS_Move(String(" movq `s0, `d0\n"), Temp_TempList(temp, NULL), Temp_TempList(munchExp(e1), NULL)));
                        emit(AS_Oper(String(" subq `s1, `d0\n"), Temp_TempList(temp, NULL),
                            Temp_TempList(temp, Temp_TempList(munchExp(e2), NULL)), NULL));
                    }
                    return temp;
                }
                case T_mul:{
                    Temp_temp temp = Temp_newtemp();
                    T_exp e1 = e->u.BINOP.left, e2 = e->u.BINOP.right;
                    //TODO: should put %rax and %rdx in dest TempList?
                    emit(AS_Move(String(" movq `s0, %%rax\n"), Temp_TempList(F_RAX(), NULL), 
                        Temp_TempList(munchExp(e1),NULL)));
                    emit(AS_Oper(String(" mulq `s0\n"), Temp_TempList(F_RAX(), Temp_TempList(F_RDX(), NULL)),
                        Temp_TempList(munchExp(e2), NULL), NULL));
                    emit(AS_Move(String(" movq %%rax, `d0"), Temp_TempList(temp, NULL),
                        Temp_TempList(F_RAX(), NULL)));
                    return temp;
                }
                case T_div:{
                    Temp_temp temp = Temp_newtemp();
                    T_exp e1 = e->u.BINOP.left, e2 = e->u.BINOP.right;
                    //TODO: should put %rax and %rdx in dest TempList?
                    emit(AS_Move(String(" movq `s0, %%rax\n"), Temp_TempList(F_RAX(), NULL), 
                        Temp_TempList(munchExp(e1),NULL)));
                    emit(AS_Oper(String(" divq `s0\n"), Temp_TempList(F_RAX(), Temp_TempList(F_RDX(), NULL)),
                        Temp_TempList(munchExp(e2), NULL), NULL));
                    emit(AS_Move(String(" movq %%rax, `d0"), Temp_TempList(temp, NULL),
                        Temp_TempList(F_RAX(), NULL)));
                    return temp;
                }
                case T_and:{
                    Temp_temp temp = Temp_newtemp();
                    //TODO: can we use leaq here?
                    if(e->u.BINOP.right->kind == T_CONST){
                        T_exp e1 = e->u.BINOP.left;
                        char str[TEMP_STR_LENGTH];
                        emit(AS_Move(String(" movq `s0, `d0"), Temp_TempList(temp, NULL), 
                            Temp_TempList(munchExp(e1), NULL)));
                        sprintf(str, " andq $%d, `d0\n", e->u.BINOP.right->u.CONST);
                        emit(AS_Oper(String(str), Temp_TempList(temp, NULL),
                            Temp_TempList(temp, NULL), NULL));
                    }else if(e->u.BINOP.left->kind == T_CONST){
                        T_exp e1 = e->u.BINOP.right;
                        char str[TEMP_STR_LENGTH];
                        emit(AS_Move(String(" movq `s0, `d0"), Temp_TempList(temp, NULL), 
                            Temp_TempList(munchExp(e1), NULL)));
                        sprintf(str, " andq $%d, `d0\n", e->u.BINOP.left->u.CONST);
                        emit(AS_Oper(String(str), Temp_TempList(temp, NULL),
                            Temp_TempList(temp, NULL), NULL));
                    }else{
                        T_exp e1 = e->u.BINOP.left, e2 = e->u.BINOP.right;
                        emit(AS_Move(String(" movq `s0, `d0\n"), Temp_TempList(temp, NULL),
                            Temp_TempList(munchExp(e1), NULL)));
                        emit(AS_Oper(String(" andq `s1, `d0\n"), Temp_TempList(temp, NULL),
                            Temp_TempList(temp, Temp_TempList(munchExp(e2), NULL)), NULL));
                    }
                    return temp;
                }
                case T_or:{
                    Temp_temp temp = Temp_newtemp();
                    //TODO: can we use leaq here?
                    if(e->u.BINOP.right->kind == T_CONST){
                        T_exp e1 = e->u.BINOP.left;
                        char str[TEMP_STR_LENGTH];
                        emit(AS_Move(String(" movq `s0, `d0"), Temp_TempList(temp, NULL), 
                            Temp_TempList(munchExp(e1), NULL)));
                        sprintf(str, " orq $%d, `d0\n", e->u.BINOP.right->u.CONST);
                        emit(AS_Oper(String(str), Temp_TempList(temp, NULL),
                            Temp_TempList(temp, NULL), NULL));
                    }else if(e->u.BINOP.left->kind == T_CONST){
                        T_exp e1 = e->u.BINOP.right;
                        char str[TEMP_STR_LENGTH];
                        emit(AS_Move(String(" movq `s0, `d0"), Temp_TempList(temp, NULL), 
                            Temp_TempList(munchExp(e1), NULL)));
                        sprintf(str, " orq $%d, `d0\n", e->u.BINOP.left->u.CONST);
                        emit(AS_Oper(String(str), Temp_TempList(temp, NULL),
                            Temp_TempList(temp, NULL), NULL));
                    }else{
                        T_exp e1 = e->u.BINOP.left, e2 = e->u.BINOP.right;
                        emit(AS_Move(String(" movq `s0, `d0\n"), Temp_TempList(temp, NULL),
                            Temp_TempList(munchExp(e1), NULL)));
                        emit(AS_Oper(String(" orq `s1, `d0\n"), Temp_TempList(temp, NULL),
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
                    emit(AS_Move(String(" movq `s0, `d0\n"), Temp_TempList(temp, NULL),
                        Temp_TempList(munchExp(e1), NULL)));
                    sprintf(str, " shlq $%d, `d0\n", e->u.BINOP.right->u.CONST);
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
                    emit(AS_Move(String(" movq `s0, `d0\n"), Temp_TempList(temp, NULL),
                        Temp_TempList(munchExp(e1), NULL)));
                    sprintf(str, " shrq $%d, `d0\n", e->u.BINOP.right->u.CONST);
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
                    emit(AS_Move(String(" movq `s0, `d0\n"), Temp_TempList(temp, NULL),
                        Temp_TempList(munchExp(e1), NULL)));
                    sprintf(str, " sarq $%d, `d0\n", e->u.BINOP.right->u.CONST);
                    emit(AS_Oper(String(str), Temp_TempList(temp, NULL),
                        Temp_TempList(temp, NULL), NULL));
                    return temp;
                }
                case T_xor:{
                    Temp_temp temp = Temp_newtemp();
                    //TODO: can we use leaq here?
                    if(e->u.BINOP.right->kind == T_CONST){
                        T_exp e1 = e->u.BINOP.left;
                        char str[TEMP_STR_LENGTH];
                        emit(AS_Move(String(" movq `s0, `d0"), Temp_TempList(temp, NULL), 
                            Temp_TempList(munchExp(e1), NULL)));
                        sprintf(str, " xorq $%d, `d0\n", e->u.BINOP.right->u.CONST);
                        emit(AS_Oper(String(str), Temp_TempList(temp, NULL),
                            Temp_TempList(temp, NULL), NULL));
                    }else if(e->u.BINOP.left->kind == T_CONST){
                        T_exp e1 = e->u.BINOP.right;
                        char str[TEMP_STR_LENGTH];
                        emit(AS_Move(String(" movq `s0, `d0"), Temp_TempList(temp, NULL), 
                            Temp_TempList(munchExp(e1), NULL)));
                        sprintf(str, " xorq $%d, `d0\n", e->u.BINOP.left->u.CONST);
                        emit(AS_Oper(String(str), Temp_TempList(temp, NULL),
                            Temp_TempList(temp, NULL), NULL));
                    }else{
                        T_exp e1 = e->u.BINOP.left, e2 = e->u.BINOP.right;
                        emit(AS_Move(String(" movq `s0, `d0\n"), Temp_TempList(temp, NULL),
                            Temp_TempList(munchExp(e1), NULL)));
                        emit(AS_Oper(String(" xorq `s1, `d0\n"), Temp_TempList(temp, NULL),
                            Temp_TempList(temp, Temp_TempList(munchExp(e2), NULL)), NULL));
                    }
                    return temp;
                }
            }
        }
        case T_CONST:{
            Temp_temp temp = Temp_newtemp();
            char str[TEMP_STR_LENGTH];
            sprintf(str, "movq $%d, `d0", e->u.CONST);
            emit(AS_Oper(String(str), Temp_TempList(temp, NULL), NULL, NULL));
            return temp;
        }
        case T_TEMP:{
            return e->u.TEMP;
        }
        case T_NAME:{
            Temp_temp temp = Temp_newtemp();
            char str[TEMP_STR_LENGTH];
            sprintf(str, "movq %s, `d0", Temp_labelstring(e->u.NAME));
            emit(AS_Oper(String(str), Temp_TempList(temp, NULL), NULL, NULL));
            return temp;
        }
        case T_ESEQ:{
            munchStm(e->u.ESEQ.stm);
            return munchExp(e->u.ESEQ.exp);
        }
        case T_CALL:{
            Temp_temp r = munchExp(e->u.CALL.fun);
            Temp_tempList l = munchArgs(0, e->u.CALL.args);
            //TODO: calldefs is not sure
            emit(AS_Oper(" call `s0\n", Temp_TempList(F_RV(), callerSaves()), Temp_TempList(r, l), NULL));
            // return %rax
            return F_RV();
        }
        default:
            assert(0);
    }
}

//TODO:现在是一个stub,这里是不是要生成存储参数的指令？？？
static Temp_tempList munchArgs(int index, T_expList args){
    if(!args) return NULL;
    if(index < 6){ // argument should be store in register first
        int tempIndex = index;
        Temp_tempList argregs = argRegs();
        while(tempIndex > 0){
            argregs = argregs->tail;
            tempIndex--;
        }
        emit(AS_Move(" movq `s0, `d0\n", Temp_TempList(argregs->head, NULL), Temp_TempList(munchExp(args->head), NULL)));
        //TODO:第一个是机器寄存器还是munch的结果？我认为因该是机器寄存器
        return Temp_TempList(argregs->head, munchArgs(index+1, args->tail));
    }else{
        
    }
}