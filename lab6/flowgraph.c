#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "tree.h"
#include "absyn.h"
#include "assem.h"
#include "frame.h"
#include "graph.h"
#include "flowgraph.h"
#include "errormsg.h"
#include "table.h"

static AS_instr findLabelStm(AS_instrList instrList, Temp_label label){
	while(instrList){
		assert(instrList);
		if(instrList->head->kind == I_LABEL && instrList->head->u.LABEL.label == label){
			return instrList->head;
		}
		instrList = instrList->tail;
	}
	return NULL;
}

static G_node findNodeByInstr(G_nodeList nodeList, AS_instr instr){
	while(nodeList){
		assert(nodeList->head);
		if(G_nodeInfo(nodeList->head) == (void*) instr){
			return nodeList->head;
		}
		nodeList = nodeList->tail;
	}
	return NULL;
}

Temp_tempList FG_def(G_node n) {
	//your code here.
	AS_instr instruction = (AS_instr) G_nodeInfo(n);
	assert(instruction);
	if(instruction->kind == I_OPER){
		return instruction->u.OPER.dst;
	}else if(instruction->kind == I_MOVE){
		return instruction->u.MOVE.dst;
	}else {
		return NULL;
	}
}

Temp_tempList FG_use(G_node n) {
	//your code here.
	AS_instr instruction = (AS_instr) G_nodeInfo(n);
	assert(instruction);
	if(instruction->kind == I_OPER){
		return instruction->u.OPER.src;
	}else if(instruction->kind == I_MOVE){
		return instruction->u.MOVE.src;
	}else {
		return NULL;
	}
}

bool FG_isMove(G_node n) {
	//your code here.
	AS_instr instruction = (AS_instr) G_nodeInfo(n);
	return (instruction && instruction->kind == I_MOVE);
}

G_graph FG_AssemFlowGraph(AS_instrList il, F_frame f) {
	//your code here.
	G_graph flowGraph = G_Graph();
	//insert all instructions in graph before whole process, so that we don't need to use FindorCreateNode...
	AS_instrList tempInstrList = il;
	while(tempInstrList){
		AS_instr instruction = tempInstrList->head;
		G_Node(flowGraph, (void*)instruction);
		tempInstrList = tempInstrList->tail;
	}
	//get a nodelist
	G_nodeList nodeList= G_nodes(flowGraph);
	tempInstrList = il;
	while(tempInstrList){
		AS_instr instruction = tempInstrList->head;
		if(instruction->kind == I_MOVE || instruction->kind == I_LABEL){
			G_node from = findNodeByInstr(nodeList, instruction);
			//the end instruction of a basic block is always a jump instruction
			//assert(tempInstrList->tail); 
			if(tempInstrList->tail){
				G_node to = findNodeByInstr(nodeList, tempInstrList->tail->head);
				assert(from && to);
				G_addEdge(from, to);
			}
		}
		//instruction->kind == I_OPER
		else{
			G_node from = findNodeByInstr(nodeList, instruction);
			if(instruction->u.OPER.jumps == NULL){
				if(!tempInstrList->tail){
					G_node to = findNodeByInstr(nodeList, tempInstrList->tail->head);
					assert(from && to);
					G_addEdge(from, to);
				}
			}else{
				Temp_labelList labelList = instruction->u.OPER.jumps->labels;
				while(labelList){
					G_node to = findNodeByInstr(nodeList, findLabelStm(il, labelList->head));
					assert(to);
					G_addEdge(from, to);
					labelList = labelList->tail;
				}
			}
		}
		tempInstrList = tempInstrList->tail;
	}
	return flowGraph;
}
