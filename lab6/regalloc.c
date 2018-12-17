#include <stdio.h>
#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "tree.h"
#include "absyn.h"
#include "assem.h"
#include "frame.h"
#include "graph.h"
#include "liveness.h"
#include "color.h"
#include "regalloc.h"
#include "table.h"
#include "flowgraph.h"

/*
目前所有操作暂时都是对无序列表进行的操作！！！
*/

#define K 16

int Degree(G_node node);
void IncDegree(G_node node);
void DecDegree(G_node node);
void PushSelectStack(G_node node);
G_node PopSelectStack();
void RA_initAll(F_frame f, AS_instrList il);
void AddEdge(G_node u, G_node v);
void MakeWorklist();
G_nodeList Adjacent(G_node n);
Live_moveList NodeMoves(G_node n);
bool MoveRelated(G_node n);
void Simplify();
void DecrementDegree(G_node m);
void EnableMoves(G_nodeList nodes);
void Coalesce();
void AddWorkList(G_node u);
bool AllOk(G_nodeList list, G_node r);
bool OK(G_node t, G_node r);
bool Conservative(G_nodeList nodes);
G_node GetAlias(G_node n);
void Combine(G_node u, G_node v);
void Freeze();
void FreezeMoves(G_node u);
void SelectSpill();
void AssignColors();
void RewriteProgram();
void LivenessAnalysis();
void Build();
void RA_main();

struct Live_graph liveGraph;
Temp_tempList registers;
G_nodeList precolored;
G_nodeList initial;
G_nodeList simplifyWorklist;
G_nodeList freezeWorklist;
G_nodeList spillWorklist;
G_nodeList spilledNodes;
G_nodeList coalescedNodes;
G_nodeList coloredNodes;
G_nodeList selectStack;

Live_moveList coalescedMoves;
Live_moveList constrainedMoves;
Live_moveList frozenMoves;
Live_moveList worklistMoves;
Live_moveList activeMoves;

//G_table adjList;  //value:Temp_tempList
G_table degree;   //value:int*
G_table alias;    //value:G_node
G_table moveList; //value:Live_moveList
G_table color;

Temp_map coloring;//temp->String
AS_instrList instructions;
F_frame frame;

struct RA_result RA_regAlloc(F_frame f, AS_instrList il) {
	//your code here
	struct RA_result ret;
	RA_initAll(f, il);
	RA_main();
	ret.coloring = coloring;
	ret.il = instructions;
	return ret;
}

void RA_initAll(F_frame f, AS_instrList il){
	//还没用到orz
	frame = f;
	//指令序列，初始值就是传进来的指令, 之后rewrite会修改
	instructions = il;
	//算法为节点选择的颜色，初始值是那些机器寄存器 Temp->String
	coloring = F_tempMap;

	//TODO: now is just a naive intialization

	//initialize registers
	registers = F_registers();
	
	//TODO:adjSet should be a bool-bitmap???
	//adjList = G_empty();
	degree = G_empty();
	alias = G_empty();
	moveList = G_empty();

	simplifyWorklist = NULL;
 	freezeWorklist = NULL;
 	spillWorklist = NULL;
 	spilledNodes = NULL;
 	coalescedNodes = NULL;
	coloredNodes = NULL;

	coalescedMoves = NULL;
	constrainedMoves = NULL;
	frozenMoves = NULL;
	worklistMoves = NULL;
	activeMoves = NULL;
}

void RA_main(){
	LivenessAnalysis();
	Build();
	MakeWorklist();
	while(simplifyWorklist != NULL && worklistMoves != NULL
		&& freezeWorklist != NULL && spillWorklist != NULL){
		if(!simplifyWorklist){
			Simplify();
		}else if(!worklistMoves){
			Coalesce();
		}else if(!freezeWorklist){
			Freeze();
		}else if(!spillWorklist){
			SelectSpill();
		}
	}
	AssignColors();
	if(!spilledNodes){
		RewriteProgram(spilledNodes);
		RA_main();
	}
}

void LivenessAnalysis(){
	G_graph flowGraph = FG_AssemFlowGraph(instructions, frame);
	liveGraph = Live_liveness(flowGraph);
}

void Build(){
	//因为在liveness analysis中已经分析出了冲突图与movelist，所以在不更改liveness模块接口的前提下
	//利用live graph将需要的数据结构全部构造出来

	//the following data structure should be NULL
	assert(!simplifyWorklist);
 	assert(!freezeWorklist);
 	assert(!spillWorklist);
 	assert(!spilledNodes);
 	assert(!coalescedNodes);
	assert(!coloredNodes);

	assert(!coalescedMoves);
	assert(!constrainedMoves);
	assert(!frozenMoves);
	assert(!worklistMoves);
	assert(!activeMoves);

	//graph-related data structure need to be re-initialized.
	color = G_empty();
	//adjList = G_empty();
	degree = G_empty();
	alias = G_empty();
	moveList = G_empty();

	G_graph conflictGraph = liveGraph.graph;
	Live_moveList tempMoveList = liveGraph.moves;
	
	//build initial and precolored
	G_nodeList allNodes = G_nodes(conflictGraph);
	while(allNodes){
		G_node node = allNodes->head;
		Temp_temp temp = (Temp_temp) G_nodeInfo(node);
		if(Temp_inList(precolored, temp)){
			precolored = G_insertNode(precolored, node);
		}else{
			initial = G_insertNode(initial, node);
		}
		allNodes = allNodes->tail;
	}

	//build color
	G_nodeList tempPrecolored = precolored;
	while(tempPrecolored){
		G_enter(color, tempPrecolored->head, G_nodeInfo(tempPrecolored->head));
		tempPrecolored = tempPrecolored->tail;
	}

	//build worklistMoves
	worklistMoves = tempMoveList;
	//build moveList
	while(tempMoveList){
		Live_moveList srcMoveList = (Live_moveList) G_look(moveList, tempMoveList->src);
		Live_moveList dstMoveList = (Live_moveList) G_look(moveList, tempMoveList->dst);
		G_enter(moveList, tempMoveList->src, Move_insertMove(srcMoveList, tempMoveList->src, tempMoveList->dst));
		G_enter(moveList, tempMoveList->dst, Move_insertMove(dstMoveList, tempMoveList->src, tempMoveList->dst));
		tempMoveList = tempMoveList->tail;
	}
	//build degree & adjList(deleted now)
	G_nodeList tempInitial = initial;
	while(tempInitial){
		int* intPtr = (int*) checked_malloc(sizeof(int));
		*intPtr = G_degree(tempInitial->head);
		G_enter(degree, tempInitial->head, intPtr);
		//put succ U pred in adjList
		//G_enter(adjList, tempInitial->head, G_adj(tempInitial->head));
		tempInitial = tempInitial->tail;
	}
}

void IncDegree(G_node node){
	int* deg = (int*) G_look(degree, node);
	assert(deg);
	*deg = (*deg + 1);
}

void DecDegree(G_node node){
	int* deg = (int*) G_look(degree, node);
	assert(deg);
	*deg = (*deg - 1);
}

int Degree(G_node node){
	int* deg = (int*) G_look(degree, node);
	assert(deg);
	return *deg;
}

void PushSelectStack(G_node node){
	selectStack = G_NodeList(node, selectStack);
}

G_node PopSelectStack(){
	G_node node = selectStack->head;
	selectStack = selectStack->tail;
	return node;
}

void AddEdge(G_node u, G_node v){
	if(!G_inNodeList(u, G_adj(v)) && u != v){
		G_addEdge(u, v);
		if(G_inNodeList(u, precolored)){
			//G_enter(adjList, u, G_insertNode(G_look(adjList, u), v));
			IncDegree(u);
		}

		if(G_inNodeList(v, precolored)){
			//G_enter(adjList, v, G_insertNode(G_look(adjList, v), u));
			IncDegree(v);
		}
	}
}

void MakeWorklist(){
	while(initial){
		G_node n = initial->head;
		initial = initial->tail;
		if(Degree(n) >= K){
			spillWorklist = G_insertNode(spillWorklist, n);
		}else if(MoveRelated(n)){
			freezeWorklist = G_insertNode(freezeWorklist, n);
		}else{
			simplifyWorklist = G_insertNode(simplifyWorklist, n);
		}
	}
}

G_nodeList Adjacent(G_node n){
	//G_nodeList adjListn = (G_nodeList) G_look(adjList, n);
	G_nodeList adjListN = G_adj(n);
	return G_exclusiveNodeList(adjListN, G_unionNodeList(selectStack, coalescedNodes));
}

Live_moveList NodeMoves(G_node n){
	Live_moveList moveListn = (Live_moveList) G_look(moveList, n);
	assert(moveListn);
	return Move_intersectList(moveListn, Move_unionList(activeMoves, worklistMoves));
}

bool MoveRelated(G_node n){
	return !NodeMoves(n);
}

void Simplify(){
	G_node n = simplifyWorklist->head;
	simplifyWorklist = simplifyWorklist->tail;
	PushSelectStack(n);
	G_nodeList Adjacentn = Adjacent(n);
	while(Adjacentn){
		DecrementDegree(Adjacentn->head);
		Adjacentn = Adjacentn->tail;
	}
}

void DecrementDegree(G_node m){
	int d = Degree(m);
	DecDegree(m);
	if(d == K){
		EnableMoves(G_insertNode(Adjacent(m), m));
		//TODO:write a delete function?
		spillWorklist = G_deleteNode(spillWorklist, m);
		if(MoveRelated(m)){
			freezeWorklist = G_insertNode(freezeWorklist, m);
		}else{
			simplifyWorklist = G_insertNode(simplifyWorklist, m);
		}
	}
}

void EnableMoves(G_nodeList nodes){
	while(nodes){
		G_node n = nodes->head;
		Live_moveList NodeMovesn = NodeMoves(n);
		while(NodeMoves){
			if(Move_inList(activeMoves, NodeMovesn->src, NodeMovesn->dst)){
				activeMoves = Move_deleteMove(activeMoves, NodeMovesn->src, NodeMovesn->dst);
				worklistMoves = Move_insertMove(worklistMoves, NodeMovesn->src, NodeMovesn->dst);
			}
			NodeMovesn = NodeMovesn->tail;
		}
		nodes = nodes->tail;
	}
}

void Coalesce(){
	//select first move
	G_node u, v;
	G_node x = worklistMoves->src;
	G_node y = worklistMoves->dst;
	x = GetAlias(x);
	y = GetAlias(y);
	if(G_inNodeList(y, precolored)){
		u = y;
		v = x;
	}else{
		u = x;
		v = y;
	}
	worklistMoves = worklistMoves->tail;
	if(u == v){
		coalescedMoves = Move_insertMove(coalescedMoves, x, y);
		AddWorkList(u);
	}else if(G_inNodeList(v, precolored) || G_inNodeList(v, G_adj(u))){
		constrainedMoves = Move_insertMove(constrainedMoves, x, y);
		AddWorkList(u);
		AddWorkList(v);
	}else if((G_inNodeList(u, precolored) && AllOk(Adjacent(v), u)) ||
			(!G_inNodeList(u, precolored) && Conservative(G_unionNodeList(Adjacent(u),Adjacent(v))))){
		coalescedMoves = Move_insertMove(coalescedMoves, x, y);
		Combine(u, v);
		AddWorkList(u);
	}else{
		activeMoves = Move_insertMove(activeMoves, x, y);
	}
}

void AddWorkList(G_node u){
	if(!G_inNodeList(u, precolored) && !MoveRelated(u) && Degree(u) < K){
		freezeWorklist = G_deleteNode(freezeWorklist, u);
		simplifyWorklist = G_insertNode(simplifyWorklist, u);
	}
}


bool AllOk(G_nodeList list, G_node r){
	bool result = TRUE;
	while(list){
		if(!OK(list->head, r)){
			result = FALSE;
		}
		list = list->tail;
	}
	return result;
}

bool OK(G_node t, G_node r){
	return (Degree(t) < K || G_inNodeList(t, precolored) || G_inNodeList(r, G_adj(t)));
}

bool Conservative(G_nodeList nodes){
	int k = 0;
	while(nodes){
		if(Degree(nodes->head) >= K){
			k++;
		}
		nodes = nodes->tail;
	}
	return (k < K);
}

G_node GetAlias(G_node n){
	if(G_inNodeList(n, coalescedNodes)){
		G_node aliasNode = (G_node) G_look(alias, n);
		assert(aliasNode);
		return GetAlias(aliasNode);
	}else{
		return n;
	}
}

void Combine(G_node u, G_node v){
	if(G_inNodeList(v, freezeWorklist)){
		freezeWorklist = G_deleteNode(freezeWorklist, v);
	}else{
		spillWorklist = G_deleteNode(spillWorklist, v);
	}
	coalescedNodes = G_insertNode(coalescedNodes, v);
	G_enter(alias, v, (void*)u);

	Live_moveList moveListu = (Live_moveList) G_look(moveList, u);
	Live_moveList moveListv = (Live_moveList) G_look(moveList, v);
	G_enter(moveList, u, Move_unionList(moveListu, moveListv));
	EnableMoves(v);

	G_nodeList adjv = Adjacent(v);
	while(adjv){
		AddEdge(adjv->head, u);
		DecrementDegree(adjv->head);
		adjv = adjv->tail;
	}

	if(Degree(u) >= K && G_inNodeList(u, freezeWorklist)){
		freezeWorklist = G_deleteNode(freezeWorklist, u);
		spillWorklist = G_insertNode(spillWorklist, u);
	}
}

void Freeze(){
	//select the first node
	G_node u = freezeWorklist->head;
	freezeWorklist = freezeWorklist->tail;

	simplifyWorklist = G_insertNode(simplifyWorklist, u);
	FreezeMoves(u);
}

void FreezeMoves(G_node u){
	Live_moveList NodeMovesU = NodeMoves(u);
	while(NodeMovesU){
		G_node x = NodeMovesU->src, y = NodeMovesU->dst;
		G_node v;
		if(GetAlias(y) == GetAlias(u)){
			v = GetAlias(x);
		}else{
			v = GetAlias(y);
		}
		activeMoves = Move_deleteMove(activeMoves, x, y);
		frozenMoves = Move_insertMove(frozenMoves, x, y);
		if(!NodeMoves(v) && Degree(v) < K){
			freezeWorklist = G_deleteNode(freezeWorklist, v);
			simplifyWorklist = G_insertNode(simplifyWorklist, v);
		}
		NodeMovesU = NodeMovesU->tail;
	}
}

void SelectSpill(){
	//TODO:
	/* 用所喜好的启发式从spillWorkList选出一个节点，
	注意：要避免选择那种由读取前面已溢出的寄存器产生的、活跃范围很小的节点*/
	G_node m = NULL; //TODO:Haven't select a node
	spillWorklist = G_deleteNode(spillWorklist, m);
	simplifyWorklist = G_insertNode(simplifyWorklist, m);
	FreezeMoves(m);
}

void AssignColors(){
	Temp_map map = Temp_empty();
	while(selectStack){
		G_node node = PopSelectStack();
		Temp_temp node_temp = (Temp_temp) G_nodeInfo(node);
		Temp_tempList okColors = registers;
		//G_nodeList adjListN = (G_nodeList) G_look(adjList, node);
		G_nodeList adjListN = G_adj(node);
		while(adjListN){
			G_node w = adjListN->head;
			if(G_inNodeList(GetAlias(w), G_unionNodeList(coloredNodes, precolored))){
				Temp_temp temp = (Temp_temp) G_look(color, GetAlias(w));
				assert(temp);
				okColors = Temp_deleteTemp(okColors, temp);
			}
			adjListN = adjListN->tail;
		}
		if(!okColors){
			spilledNodes = G_insertNode(spilledNodes, node);
		}else{
			coloredNodes = G_insertNode(coloredNodes, node);
			G_enter(color, node, okColors->head);
			assert(!Temp_inList(registers, node_temp));
			Temp_enter(map, node_temp, Temp_look(coloring, okColors->head));
		}
	}
	G_nodeList tempCoalescedNodes = coalescedNodes;
	while(tempCoalescedNodes){
		Temp_temp temp = G_look(color, GetAlias(tempCoalescedNodes->head));
		Temp_temp node_temp = (Temp_temp) G_nodeInfo(tempCoalescedNodes->head);
		assert(temp);
		//G_enter(color, tempCoalescedNodes->head, temp); 
		Temp_enter(map, node_temp, Temp_look(coloring, temp));
		tempCoalescedNodes = tempCoalescedNodes->tail;
	}
	//每一轮的分配颜色迭代都会产生一个新层
	coloring = Temp_layerMap(map, coloring);
}

void RewriteProgram(){
	//为每个属于spilledNodes的v节点分配一个存储单元
	//为每一个定值和每一个使用创建一个新的临时变量vi
	//在程序中（指令序列中）vi的每一个定值之后插入一条存储指令
	//在vi的每一个使用之前插入一条取数指令
	//将所有的vi放入集合newTemps
	G_table accessTab = G_empty();
	while(spilledNodes){
		F_access access = F_allocLocal(frame, TRUE);
		instructions = RewriteOneSpill(instructions, (Temp_temp)G_nodeInfo(spilledNodes->head), access);
		spilledNodes = spilledNodes->tail;
	}
	spilledNodes = NULL;
	coloredNodes = NULL;
	coalescedNodes = NULL;
}

