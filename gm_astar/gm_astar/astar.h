/*
    gm_astar
    By Spacetech
*/

#ifndef ASTAR_H
#define ASTAR_H

#include "filesystem.h"
#include "utlbuffer.h"

#include "astarnode.h"

#include "main.h"

class AStar
{
public:
	AStar();
	~AStar();
	void Reset();
	float HeuristicDistance(Vector StartPos, Vector EndPos);
	float ManhattanDistance(Vector StartPos, Vector EndPos);
	float EuclideanDistance(Vector StartPos, Vector EndPos);
	AStarNode *FindLowestF();
	NodeList_t& FindPath();
	NodeList_t& CalcPath(AStarNode *current);
	bool Save(IFileSystem *filesystem, const char *Filename);
	bool Load(IFileSystem *filesystem, const char *Filename);
	bool Link(AStarNode *node1, AStarNode *node2);
	void AddNode(AStarNode *node);
	void AddOpenedNode(AStarNode *node);
	void AddClosedNode(AStarNode *node);
	bool HasFoundPath();
	int GetHeuristic();
	AStarNode *GetStart();
	AStarNode *GetEnd();
	void SetHeuristic(int H);
	void SetStart(AStarNode *start);
	void SetEnd(AStarNode *end);
	AStarNode *NearestNode(Vector NearPos);
	void AutoLinkNodes(float Distance);
	NodeList_t& GetNodes();
	enum Heuristic {
		HEURISTIC_MANHATTAN,
		HEURISTIC_EUCLIDEAN,
		HEURISTIC_DISTANCE,
		HEURISTIC_CUSTOM
	};

private:
	int Heuristic;
	int HeuristicRef;
	bool FoundPath;
	AStarNode *Start;
	AStarNode *End;
	NodeList_t Nodes;
	NodeList_t Path;
	NodeList_t Opened;
	NodeList_t Closed;
};

#endif
