#ifndef LIVENESS_H
#define LIVENESS_H

typedef struct Live_moveList_ *Live_moveList;
struct Live_moveList_ {
	G_node src, dst;
	Live_moveList tail;
};

Live_moveList Live_MoveList(G_node src, G_node dst, Live_moveList tail);

//my helper code
bool Move_inList(Live_moveList list, Temp_temp src, Temp_temp dst);
bool Move_isSameList(Live_moveList list1, Live_moveList list2);
Live_moveList Move_unionList(Live_moveList list1, Live_moveList list2);
Live_moveList Move_intersectList(Live_moveList list1, Live_moveList list2);
Live_moveList Move_exclusiveList(Live_moveList list1, Live_moveList list2);
Live_moveList Move_insertMove(Live_moveList list, Temp_temp src, Temp_temp dst);
Live_moveList Move_deleteMove(Live_moveList list, Temp_temp src, Temp_temp dst);

struct Live_graph {
	G_graph graph;
	Live_moveList moves;
};
Temp_temp Live_gtemp(G_node n);

struct Live_graph Live_liveness(G_graph flow);

#endif
