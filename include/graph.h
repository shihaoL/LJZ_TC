#ifndef GRAPH_H
#define GRAPH_H
#include "util.h"
#include "table.h"
typedef struct G_graph_ *G_graph;
typedef struct G_node_ *G_node;

typedef struct G_nodeList_ *G_nodeList;
struct G_nodeList_ { G_node head; G_nodeList tail; };

/* �����յ�ͼ*/
G_graph G_Graph(void);

/* ���ͼ*/
G_graph G_copyGraph(G_graph g);

/*ɾ��ͼ�ڵ�*/
G_node G_rmNode(G_graph g, G_node n);

/*����ڵ㲢���뵽ͼ��*/
G_node G_Node(G_graph g, void *info);

/*����ڵ��б�*/
G_nodeList G_NodeList(G_node head, G_nodeList tail);

/*����ͼ��ȫ���ڵ�*/
G_nodeList G_nodes(G_graph g);

/*�жϽڵ��Ƿ����б���*/
bool G_inNodeList(G_node a, G_nodeList l);

/*���һ�������*/
void G_addEdge(G_node from, G_node to);

/*ɾ��һ�������*/
void G_rmEdge(G_node from, G_node to);

/*��ӡͼ��Ϣ��������*/
void G_show(FILE *out, G_nodeList p, void showInfo(FILE* out, void *));

/*�ڵ�ĳ������ڽڵ�*/
G_nodeList G_succ(G_node n);

/*�ڵ��������ڽڵ�*/
G_nodeList G_pred(G_node n);

/*ȫ�������ڽڵ㣬��Ҫ�ã�liveness�����ĳ�ͻͼ������ͼ����succ����ȫ���ڵ�,����������ظ�������*/
G_nodeList G_adj(G_node n);

/*a�Ƿ���������ָ��b*/
bool G_goesTo(G_node a, G_node b);

/*�ڵ�Ķȣ���ȼӳ���*/
int G_degree(G_node n);

/*��ȡ�ڵ���Ϣ*/
void *G_nodeInfo(G_node n);

typedef struct TAB_table_  *G_table;

G_table G_empty(void);

void G_enter(G_table t, G_node n, void *v);

void *G_look(G_table t, G_node n);

#endif // !GRAPH_H
