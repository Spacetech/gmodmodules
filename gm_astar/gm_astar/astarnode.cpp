/*
    gm_astar
    By Spacetech
*/

#include "astarnode.h"

AStarNode::AStarNode(Vector& P)
{
	ID = -1;
	Parent = NULL;
	Opened = false;
	Closed = false;
	Pos = Vector(P.x, P.y, P.z);
}

AStarNode::~AStarNode()
{
}

void AStarNode::AddLink(AStarNode* Link)
{
	Links.AddToTail(Link);
}

bool AStarNode::IsLink(AStarNode* Link)
{
	return Links.HasElement(Link);
}

CUtlVector<AStarNode*>& AStarNode::GetLinks()
{
	return Links;
}

void AStarNode::SetStatus(AStarNode* P, float F, float G, float H)
{
	Parent = P;
	ScoreF = F;
	ScoreG = G;
	ScoreH = H;
}

bool AStarNode::IsOpened()
{
	return Opened;
}

void AStarNode::SetOpened(bool Open)
{
	Opened = Open;
}

bool AStarNode::IsClosed()
{
	return Closed;
}

void AStarNode::SetClosed(bool Close)
{
	Closed = Close;
}

Vector AStarNode::GetPos()
{
	return Pos;
}

AStarNode* AStarNode::GetParent()
{
	return Parent;
}

void AStarNode::SetParent(AStarNode* P)
{
	Parent = P;
}

float AStarNode::GetScoreH()
{
	return ScoreH;
}

float AStarNode::GetScoreF()
{
	return ScoreF;
}

float AStarNode::GetScoreG()
{
	return ScoreG;
}

int AStarNode::GetID()
{
	return ID;
}

void AStarNode::SetID(int id)
{
	ID = id;
}
