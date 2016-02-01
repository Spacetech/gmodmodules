/*
    gm_navigation
    By Spacetech
*/

#include "node.h"
#include "tier0/memdbgon.h"

Node* LUA_GetNode(lua_State* L, int pos)
{
	void* pUserData = Lua()->GetUserData(pos);
	if(pUserData == NULL)
	{
		Lua()->LuaError("Invalid Node Err 1: ", pos);
	}

	Node *pNode = reinterpret_cast<Node*>(pUserData);
	if(pNode == NULL)
	{
		Lua()->LuaError("Invalid Node Err 2: ", pos);
	}

	return pNode;
}

void LUA_PushNode(lua_State* L, Node *node)
{
	if(node)
	{
		ILuaObject* meta = Lua()->GetMetaTable(NODE_NAME, NODE_TYPE);
			Lua()->PushUserData(meta, node, NODE_TYPE);
		meta->UnReference();
	}
	else
	{
		Lua()->Push(false);
	}
}

Node::Node(const Vector &Position, const Vector &Norm, Node *Par)
{
	iID = -1;
	vecPos = Position;
	vecNormal = Norm;
	nodeParent = Par;

	// Not sure if theres any memory problems with this.
	// If it's not diagonal your still setting 4 of them to NULL
	for(int i = NORTH; i < NUM_DIRECTIONS_MAX; i++)
	{
		connections[i] = NULL;
	}

	visited = 0;

	bOpened = false;
	bClosed = false;
	bDisabled = false;
	nodeAStarParent = NULL;
	scoreF = 0;
	scoreG = 0;

	customData = NULL;
}

Node::~Node()
{
	if(customData != NULL)
	{
		delete customData;
		customData = NULL;
	}
}

void Node::ConnectTo(Node *node, NavDirType Dir)
{
	connections[Dir] = node;
}

Node *Node::GetConnectedNode(NavDirType Dir)
{
	return connections[Dir];
}

const Vector *Node::GetPosition()
{
	return &vecPos;
}

const Vector *Node::GetNormal()
{
	return &vecNormal;
}

Node *Node::GetParent()
{
	return nodeParent;
}

void Node::MarkAsVisited(NavDirType Dir)
{
	visited |= (1 << Dir);
}

bool Node::HasVisited(NavDirType Dir)
{
	if(visited & (1 << Dir))
	{
		return true;
	}
	return false;
}

void Node::SetStatus(Node* P, float F, float G)
{
	nodeAStarParent = P;
	scoreF = F;
	scoreG = G;
}

bool Node::IsOpened()
{
	return bOpened;
}

void Node::SetOpened(bool Open)
{
	bOpened = Open;
}

bool Node::IsDisabled()
{
	return bDisabled;
}

void Node::SetDisabled(bool bDisabled)
{
	this->bDisabled = bDisabled;
}

bool Node::IsClosed()
{
	return bClosed;
}

void Node::SetClosed(bool Close)
{
	bClosed = Close;
}

Node *Node::GetAStarParent()
{
	return nodeAStarParent;
}

void Node::SetAStarParent(Node* P)
{
	nodeAStarParent = P;
}

float Node::GetScoreF()
{
	return scoreF;
}

float Node::GetScoreG()
{
	return scoreG;
}

int Node::GetID()
{
	return iID;
}

void Node::SetID(int id)
{
	iID = id;
}

void Node::SetNormal(const Vector &Norm)
{
	vecNormal = Norm;
}

void Node::SetPosition(const Vector &Position)
{
	vecPos = Position;
}
