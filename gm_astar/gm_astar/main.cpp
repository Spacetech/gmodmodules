/*
    gm_astar
    By Spacetech
*/

#include <interface.h>
#include "eiface.h"
#include "filesystem.h"
#include "tier0/memdbgon.h"

#include "sigscan.h"

#include "gmluamodule.h"

#define ASTAR_NAME "AStar"
#define ASTAR_TYPE 2626

#define ASTAR_NODE_NAME "AStarNode"
#define ASTAR_NODE_TYPE 2627

#include "astarnode.h"
#include "astar.h"

#include "main.h"

GMOD_MODULE(Init, Shutdown);

int VectorMetaRef = -1;
ILuaInterface *gLua = NULL;
IVEngineServer *engine = NULL;
IFileSystem *filesystem = NULL;

///////////////////////////////////////////////
// Jinto
ILuaObject* NewVectorObject(Vector& vec)
{
	gLua->PushReference(VectorMetaRef);
		gLua->Push(vec.x);
		gLua->Push(vec.y);
		gLua->Push(vec.z);
	gLua->Call(3, 1);
	return gLua->GetReturn(0);
}

void GMOD_PushVector(Vector& vec)
{
	ILuaObject* obj = NewVectorObject(vec);
		gLua->Push(obj);
	obj->UnReference();
}

Vector& GMOD_GetVector(int stackPos)
{
	return *reinterpret_cast<Vector*>(gLua->GetUserData(stackPos));
}

///////////////////////////////////////////////

AStar* GetAStar(int Pos)
{
	return (AStar*)gLua->GetUserData(Pos);
}

AStarNode* GetAStarNode(int Pos)
{
	return (AStarNode*)gLua->GetUserData(Pos);
}

void PushAStar(AStar *astar)
{
	if(astar)
	{
		ILuaObject* meta = gLua->GetMetaTable(ASTAR_NAME, ASTAR_TYPE);
			gLua->PushUserData(meta, astar);
		meta->UnReference();
	}
	else
	{
		gLua->Push(false);
	}
}

void PushAStarNode(AStarNode *node)
{
	if(node)
	{
		ILuaObject* meta = gLua->GetMetaTable(ASTAR_NODE_NAME, ASTAR_NODE_TYPE);
			gLua->PushUserData(meta, node);
		meta->UnReference();
	}
	else
	{
		gLua->Push(false);
	}
}

/*
LUA_FUNCTION(TableTest)
{
	Msg("GetTop 1: %i", gLua->Top());

	gLua->CheckType(1, GLua::TYPE_NUMBER);

	ILuaObject* TestTable = gLua->GetNewTable();

	int Limit = gLua->GetNumber(1);

	for(int i = 0; i < Limit; i++)
	{
		if(TestTable->isNil())
		{
			Msg("GetTop 2: %i", gLua->Top());
			Msg("TestTable Failed: %i\n", i);
			gLua->Push(false);
			return 1;
		}

		ILuaObject* ReturnVector = NewVectorObject(Vector(i, i, i));

		TestTable->SetMember(i + 1, (float)i);

		ReturnVector->UnReference();
	}

	gLua->Push(TestTable);

	TestTable->UnReference();

	Msg("GetTop 3: %i", gLua->Top());

	return 1;
}
*/

///////////////////////////////////////////////

LUA_FUNCTION(CreateAStar)
{
	PushAStar(new AStar());
	return 1;
}

///////////////////////////////////////////////

LUA_FUNCTION(AStar_GetNode)
{
	gLua->CheckType(1, ASTAR_TYPE);
	gLua->CheckType(2, GLua::TYPE_NUMBER);

	int ID = gLua->GetNumber(2) - 1;

	NodeList_t& nodes = GetAStar(1)->GetNodes();

	if(nodes.IsValidIndex(ID))
	{
		PushAStarNode(nodes[ID]);
	}
	else
	{
		gLua->Push(false);
	}

	return 1;
}

LUA_FUNCTION(AStar_GetNodeTotal)
{
	gLua->CheckType(1, ASTAR_TYPE);

	gLua->Push((float)GetAStar(1)->GetNodes().Count());

	return 1;
}

LUA_FUNCTION(AStar_GetNodes)
{
	gLua->CheckType(1, ASTAR_TYPE);

	NodeList_t& Nodes = GetAStar(1)->GetNodes();

	// Push the table on the stack to survive past 510 calls!
	ILuaObject *NodeTable = gLua->GetNewTable();
	NodeTable->Push();
	NodeTable->UnReference();

	//Msg("GetTop 1: %i", gLua->Top());

	for(int i = 0; i < Nodes.Count(); i++)
	{
		PushAStarNode(Nodes[i]);

		ILuaObject *ObjAStarNode = gLua->GetObject();
		//Msg("ObjAStarNode: %d %d\n", ObjAStarNode->isUserData(), ObjAStarNode->isTable());

		NodeTable = gLua->GetObject(2); // 0 and 1 is userdata

		//Msg("NodeTable: %d %d\n", NodeTable->isUserData(), NodeTable->isTable());
		//Msg("GetTop 1.1: %i ", gLua->Top());
		NodeTable->SetMember((float)i + 1, ObjAStarNode);
		//Msg("GetTop 1.2: %i ", gLua->Top());
		gLua->Pop();
		//Msg("GetTop 1.3: %i\n", gLua->Top());

		NodeTable->UnReference();
		ObjAStarNode->UnReference();
	}

	//gLua->Push(NodeTable);

	//NodeTable->UnReference();

	//Msg("GetTop 2: %i\n", gLua->Top());

	return 1;
}

LUA_FUNCTION(AStar_FindPath)
{
	//Msg("GetTop 1: %i", gLua->Top());

	gLua->CheckType(1, ASTAR_TYPE);

	AStar *astar = GetAStar(1);

	NodeList_t& Path = astar->FindPath();

	if(astar->HasFoundPath())
	{
		gLua->Push(true);
	}
	else
	{
		gLua->Push(false);
	}

	// Push the table on the stack to survive past 510 calls!
	ILuaObject *PosTable = gLua->GetNewTable();
	PosTable->Push();
	PosTable->UnReference();

	//Msg("GetTop 0: %i\n", gLua->Top());

	for(int i = 0; i < Path.Count(); i++)
	{
		/*
		if(PosTable->isNil())
		{
			Msg("PosTable Failed: %i\n", i);
			break;
		}
		*/

		//Msg("GetTop 1: %i ", gLua->Top());

		ILuaObject* ReturnVector = NewVectorObject(Path[i]->GetPos());

		//Msg("GetTop 2: %i ", gLua->Top());
		PosTable = gLua->GetObject();

		//if(PosTable->isTable())
		//{
			//Msg("GetTop 3: %i ", gLua->Top());
			PosTable->SetMember(i + 1, ReturnVector);

			//Msg("GetTop 4: %i\n", gLua->Top());

			PosTable->UnReference();
		//}

		ReturnVector->UnReference();

		//Msg("%i | %f, %f, %f\n", i, Path[i]->GetPos().x, Path[i]->GetPos().y, Path[i]->GetPos().z);
	}

	/*
	if(PosTable->isNil())
	{
		gLua->Push(false);
	}
	else
	{
		gLua->Push(PosTable);
	}
	PosTable->UnReference();
	*/

	//Msg("GetTop 5: %i", gLua->Top());

	return 2;
}

LUA_FUNCTION(AStar_AddNode)
{
	gLua->CheckType(1, ASTAR_TYPE);
	gLua->CheckType(2, GLua::TYPE_VECTOR);

	AStarNode *node = new AStarNode(GMOD_GetVector(2));

	GetAStar(1)->AddNode(node);

	PushAStarNode(node);

	return 1;
}

LUA_FUNCTION(AStar_GetHeuristic)
{
	gLua->CheckType(1, ASTAR_TYPE);

	gLua->Push((float)GetAStar(1)->GetHeuristic());

	return 1;
}

LUA_FUNCTION(AStar_GetStart)
{
	gLua->CheckType(1, ASTAR_TYPE);

	PushAStarNode(GetAStar(1)->GetStart());

	return 1;
}

LUA_FUNCTION(AStar_GetEnd)
{
	gLua->CheckType(1, ASTAR_TYPE);

	PushAStarNode(GetAStar(1)->GetEnd());

	return 1;
}

LUA_FUNCTION(AStar_SetHeuristic)
{
	gLua->CheckType(1, ASTAR_TYPE);
	gLua->CheckType(2, GLua::TYPE_NUMBER);

	GetAStar(1)->SetHeuristic(gLua->GetNumber(2));

	return 0;
}

LUA_FUNCTION(AStar_SetStart)
{
	gLua->CheckType(1, ASTAR_TYPE);
	gLua->CheckType(2, ASTAR_NODE_TYPE);

	GetAStar(1)->SetStart(GetAStarNode(2));

	return 0;
}

LUA_FUNCTION(AStar_SetEnd)
{
	gLua->CheckType(1, ASTAR_TYPE);
	gLua->CheckType(2, ASTAR_NODE_TYPE);

	GetAStar(1)->SetEnd(GetAStarNode(2));

	return 0;
}

//////////////////////
// Helpful Functions

LUA_FUNCTION(AStar_Save)
{
	gLua->CheckType(1, ASTAR_TYPE);
	gLua->CheckType(2, GLua::TYPE_STRING);

	gLua->Push(GetAStar(1)->Save(filesystem, gLua->GetString(2)));

	return 1;
}

LUA_FUNCTION(AStar_Load)
{
	gLua->CheckType(1, ASTAR_TYPE);
	gLua->CheckType(2, GLua::TYPE_STRING);

	gLua->Push(GetAStar(1)->Load(filesystem, gLua->GetString(2)));

	return 1;
}

LUA_FUNCTION(AStar_NearestNode)
{
	gLua->CheckType(1, ASTAR_TYPE);
	gLua->CheckType(2, GLua::TYPE_VECTOR);

	PushAStarNode(GetAStar(1)->NearestNode(GMOD_GetVector(2)));

	return 1;
}

LUA_FUNCTION(AStar_LinkNodes)
{
	gLua->CheckType(1, ASTAR_TYPE);
	gLua->CheckType(2, GLua::TYPE_FUNCTION);

	AStar *astar = GetAStar(1);

	int Ref = gLua->GetReference(2);

	NodeList_t& Nodes = astar->GetNodes();

	AStarNode *node1, *node2;
	for(int i = 0; i < Nodes.Count(); i++)
	{
		node1 = Nodes[i];
		for(int i2 = 0; i2 < Nodes.Count(); i2++)
		{
			node2 = Nodes[i2];

			gLua->PushReference(Ref);
				GMOD_PushVector(node1->GetPos());
				GMOD_PushVector(node2->GetPos());
			gLua->Call(2, 1);

			if(gLua->GetReturn(0)->GetBool())
			{
				astar->Link(node1, node2);
			}
		}
	}

	gLua->FreeReference(Ref);

	return 0;
}

LUA_FUNCTION(AStar_AutoLinkNodes)
{
	gLua->CheckType(1, ASTAR_TYPE);
	gLua->CheckType(2, GLua::TYPE_NUMBER);

	GetAStar(1)->AutoLinkNodes(gLua->GetNumber(2));

	return 0;
}

///////////////////////////////////////////////

LUA_FUNCTION(AStarNode_Link)
{
	gLua->CheckType(1, ASTAR_NODE_TYPE);
	gLua->CheckType(2, ASTAR_NODE_TYPE);

	AStarNode *node1 = GetAStarNode(1), *node2 = GetAStarNode(2);

	if(!node1->IsLink(node2))
	{
		node1->AddLink(node2);
		node2->AddLink(node1);
		gLua->Push(true);
	}
	else
	{
		gLua->Push(false);
	}

	return 1;
}

LUA_FUNCTION(AStarNode_IsLink)
{
	gLua->CheckType(1, ASTAR_NODE_TYPE);
	gLua->CheckType(2, ASTAR_NODE_TYPE);

	gLua->Push(GetAStarNode(1)->IsLink(GetAStarNode(2)));

	return 1;
}

LUA_FUNCTION(AStarNode_GetPos)
{
	gLua->CheckType(1, ASTAR_NODE_TYPE);

	GMOD_PushVector(GetAStarNode(1)->GetPos());

	return 1;
}

LUA_FUNCTION(AStarNode_GetLinks)
{
	gLua->CheckType(1, ASTAR_NODE_TYPE);

	NodeList_t& Links = GetAStarNode(1)->GetLinks();

	// Push the table on the stack to survive past 510 calls!
	ILuaObject *LinksTable = gLua->GetNewTable();
	LinksTable->Push();
	LinksTable->UnReference();

	for(int i = 0; i < Links.Count(); i++)
	{
		PushAStarNode(Links[i]);

		ILuaObject *ObjAStarNode = gLua->GetObject();

		LinksTable = gLua->GetObject(2);
		LinksTable->SetMember((float)i + 1, ObjAStarNode);

		gLua->Pop();

		LinksTable->UnReference();
		ObjAStarNode->UnReference();
	}

	return 1;
}

///////////////////////////////////////////////

struct factorylist_t
{
	CreateInterfaceFn engineFactory;
	CreateInterfaceFn physicsFactory;
	CreateInterfaceFn fileSystemFactory;
};

void (*FactoryList_Retrieve)( factorylist_t &destData );

factorylist_t GetFactories()
{
	CSigScan::sigscan_dllfunc = Sys_GetFactory("server.dll");

	if(!CSigScan::GetDllMemInfo())
	{
		gLua->Error("Could not get access to factories.");
	}

	CSigScan sigscanFactories;
	sigscanFactories.Init((unsigned char *)"\x8B\x44\x24\x04\x8B\x0D\xC0\x33\x6D\x10\x8B\x15\xC4\x33\x6D\x10\x89\x08\x8B\x0D\xC8\x33\x6D\x10\x89\x50\x04\x89\x48\x08\xC3", "xxxxxx????xx????xxxx????xxxxxxx", 31);

	if(!sigscanFactories.is_set)
	{
		gLua->Error("Could not get access to factories.");
	}

	FactoryList_Retrieve = (void (*)(factorylist_t &))sigscanFactories.sig_addr;

	factorylist_t factories;
	FactoryList_Retrieve(factories);

	return factories;
}

int Init(lua_State* L)
{
	gLua = Lua();

	CreateInterfaceFn interfaceFactory = Sys_GetFactory("engine.dll");

	engine = (IVEngineServer*)interfaceFactory(INTERFACEVERSION_VENGINESERVER, NULL);
	if(!engine)
	{
		gLua->Error("gm_astar: Missing IVEngineServer interface.\n");
	}
	
	CreateInterfaceFn fsFactory;

	if(gLua->IsDedicatedServer())
	{
		fsFactory = GetFactories().fileSystemFactory;
	}
	else
	{
		fsFactory = Sys_GetFactory("filesystem_steam.dll");
	}

	if(!fsFactory)
	{
		gLua->Error("gm_astar: Missing fsFactory\n");
	}

	filesystem = (IFileSystem*)fsFactory(FILESYSTEM_INTERFACE_VERSION, NULL);
	if(!filesystem)
	{
		gLua->Error("gm_astar: Missing IFileSystem interface.\n");
	}

	// Get a reference to the function to survive past 510 calls!
	ILuaObject *VectorMeta = gLua->GetGlobal("Vector");
		VectorMeta->Push();
		VectorMetaRef = gLua->GetReference(-1, true);
	VectorMeta->UnReference();

	gLua->SetGlobal("HEURISTIC_MANHATTAN", (float)AStar::HEURISTIC_MANHATTAN);
	gLua->SetGlobal("HEURISTIC_EUCLIDEAN", (float)AStar::HEURISTIC_EUCLIDEAN);
	gLua->SetGlobal("HEURISTIC_DISTANCE", (float)AStar::HEURISTIC_DISTANCE);
	gLua->SetGlobal("HEURISTIC_CUSTOM", (float)AStar::HEURISTIC_CUSTOM);

	gLua->SetGlobal("CreateAStar", CreateAStar);

	ILuaObject *MetaAStar = gLua->GetMetaTable(ASTAR_NAME, ASTAR_TYPE);
		ILuaObject *AStarIndex = gLua->GetNewTable();
			AStarIndex->SetMember("GetNode", AStar_GetNode);
			AStarIndex->SetMember("GetNodeTotal", AStar_GetNodeTotal);
			AStarIndex->SetMember("GetNodes", AStar_GetNodes);
			AStarIndex->SetMember("FindPath", AStar_FindPath);
			AStarIndex->SetMember("AddNode", AStar_AddNode);
			AStarIndex->SetMember("GetHeuristic", AStar_GetHeuristic);
			AStarIndex->SetMember("GetStart", AStar_GetStart);
			AStarIndex->SetMember("GetEnd", AStar_GetEnd);
			AStarIndex->SetMember("SetHeuristic", AStar_SetHeuristic);
			AStarIndex->SetMember("SetStart", AStar_SetStart);
			AStarIndex->SetMember("SetEnd", AStar_SetEnd);
			AStarIndex->SetMember("Save", AStar_Save);
			AStarIndex->SetMember("Load", AStar_Load);
			AStarIndex->SetMember("NearestNode", AStar_NearestNode);
			AStarIndex->SetMember("LinkNodes", AStar_LinkNodes);
			AStarIndex->SetMember("AutoLinkNodes", AStar_AutoLinkNodes);
		MetaAStar->SetMember("__index", AStarIndex);
		AStarIndex->UnReference();
	MetaAStar->UnReference();

	ILuaObject *MetaAStarNode = gLua->GetMetaTable(ASTAR_NODE_NAME, ASTAR_NODE_TYPE);
		ILuaObject *AStarNodeIndex = gLua->GetNewTable();
			AStarNodeIndex->SetMember("Link", AStarNode_Link);
			AStarNodeIndex->SetMember("IsLink", AStarNode_IsLink);
			AStarNodeIndex->SetMember("GetPos", AStarNode_GetPos);
			AStarNodeIndex->SetMember("GetLinks", AStarNode_GetLinks);
		MetaAStarNode->SetMember("__index", AStarNodeIndex);
		AStarNodeIndex->UnReference();
	MetaAStarNode->UnReference();

	Msg("gm_astar: Programmed by Spacetech; Not for Troin's eyes!\n");

	return 0;
}

int Shutdown(lua_State* L)
{
	if(VectorMetaRef)
	{
		gLua->FreeReference(VectorMetaRef);
	}
	return 0;
}
