#ifndef LIVENESS_H
#define LIVENESS_H
#include "graph.h"
#include "temp.h"
typedef struct Live_moveList_ *Live_moveList;//������move��dst��src����ͬ���Ĵ���������������ܴ����ظ�ż��
struct Live_moveList_ {
	G_node src, dst;
	Live_moveList tail;
};

struct Live_graph {
	G_graph graph;
	Live_moveList moves;
};

Temp_temp Live_gtemp(G_node n);

struct Live_graph Live_liveness(G_graph flow);

#endif
