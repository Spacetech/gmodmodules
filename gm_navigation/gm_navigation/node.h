/*
    gm_navigation
    By Spacetech
*/

#ifndef NODE_H
#define NODE_H

#include "eiface.h"
#include "defines.h"
#include "ILuaModuleManager.h"

#ifdef USE_BOOST_THREADS
#include <boost/thread/thread.hpp>
#endif

class Node
{
public:
	Node(const Vector &Position, const Vector &Norm, Node *Par);
	~Node();
	Node *GetParent();
	void ConnectTo(Node *node, NavDirType Dir);
	Node *GetConnectedNode(NavDirType Dir);
	const Vector *GetPosition();
	const Vector *GetNormal();
	void MarkAsVisited(NavDirType Dir);
	bool HasVisited(NavDirType Dir);
	
	// Stuff for AStar
	void SetStatus(Node* P, float F, float G);
	bool IsOpened();
	void SetOpened(bool Open);
	bool IsDisabled();
	void SetDisabled(bool bDisabled);
	bool IsClosed();
	void SetClosed(bool Close);
	float GetScoreF();
	float GetScoreG();
	int GetID();
	void SetID(int id);
	void SetAStarParent(Node* P);
	Node *GetAStarParent();
	void SetNormal(const Vector &Norm);
	void SetPosition(const Vector &Position);

	Vector vecPos;
	Vector vecNormal;

	void* customData;

private:
	int iID;
	Node *nodeParent;

	unsigned short visited;
	Node *connections[NUM_DIRECTIONS_MAX];

	Node *nodeAStarParent;
	bool bOpened;
	bool bClosed;
	bool bDisabled;
	float scoreF;
	float scoreG;
};

Node* LUA_GetNode(lua_State* L, int Pos);
void LUA_PushNode(lua_State* L, Node *node);

class Nav;
class CJob;
struct JobInfo_t
{
#ifdef USE_BOOST_THREADS
	boost::thread thread;
#else
	CJob* job;
#endif
	Nav* nav;
	bool abort;
	bool finished;
	bool findPath;
	bool foundPath;
	bool hull;
	Vector mins;
	Vector maxs;
	CUtlVector<Node*> path;
	ILuaObject *objFunc;
	ILuaObject *objUpdate;
	time_t updateTime;
};

#endif
