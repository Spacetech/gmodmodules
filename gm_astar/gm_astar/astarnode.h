/*
    gm_astar
    By Spacetech
*/

#ifndef AStarNode_H
#define AStarNode_H

#include "eiface.h"

class AStarNode
{
public:
	AStarNode(Vector& P);
	~AStarNode();
	void AddLink(AStarNode* Link);
	bool IsLink(AStarNode* Link);
	CUtlVector<AStarNode*>& GetLinks();
	void SetStatus(AStarNode* P, float F, float G, float H);
	bool IsOpened();
	void SetOpened(bool Open);
	bool IsClosed();
	void SetClosed(bool Close);
	Vector GetPos();
	AStarNode* GetParent();
	void SetParent(AStarNode* P);
	float GetScoreH();
	float GetScoreF();
	float GetScoreG();
	int GetID();
	void SetID(int id);

private:
	int ID;
	bool Opened;
	bool Closed;
	Vector Pos;
	CUtlVector<AStarNode*> Links;
	AStarNode* Parent;
	float ScoreF;
	float ScoreG;
	float ScoreH;
};

#endif
