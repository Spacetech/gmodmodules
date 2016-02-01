/*
    gm_navigation
    By Spacetech
*/

#include "main.h"

//#include "node.h"
#include "nav.h"
#include "tier0/memdbgon.h"

GMOD_MODULE(Init, Shutdown);

typedef CUtlVector<Node*> NodeList_t;

CUtlVector<JobInfo_t*> JobQueue;

///////////////////////////////////////////////
// @Jinto (Referenced by Spacetech)

ILuaObject *objVector = NULL;

ILuaObject* NewVectorObject(lua_State* L, Vector& vec)
{
	if(objVector == NULL)
	{
		objVector = Lua()->GetGlobal("Vector");
	}

	objVector->Push();
		Lua()->Push(vec.x);
		Lua()->Push(vec.y);
		Lua()->Push(vec.z);
	Lua()->Call(3, 1);

	return Lua()->GetReturn(0);
}

void LUA_PushVector(lua_State* L, Vector& vec)
{
	ILuaObject* obj = NewVectorObject(L, vec);
		Lua()->Push(obj);
	obj->UnReference();
}

Vector& LUA_GetVector(lua_State* L, int stackPos)
{
	return *reinterpret_cast<Vector*>(Lua()->GetUserData(stackPos));
}

///////////////////////////////////////////////

LUA_FUNCTION(nav_Create)
{
	Lua()->CheckType(1, GLua::TYPE_NUMBER);

	LUA_PushNav(L, new Nav((int)Lua()->GetNumber(1)));

	return 1;
}

#ifdef USE_BOOST_THREADS
boost::posix_time::time_duration timeout = boost::posix_time::milliseconds(0);
boost::posix_time::time_duration sleep_time = boost::posix_time::milliseconds(50);
#endif

LUA_FUNCTION(nav_Poll)
{
	for(int i=0; i < JobQueue.Size(); i++)
	{
		JobInfo_t* info = JobQueue[i];
#ifdef USE_BOOST_THREADS
		if(info->finished || info->thread.timed_join(timeout))
#else
		if(info->job->IsFinished())
#endif
		{
#ifdef FILEBUG
			FILEBUG_WRITE("Job Finished\n");
#endif

			info->objFunc->Push();
#ifdef FILEBUG
			FILEBUG_WRITE("Pushed Reference\n");
#endif

			if(Lua()->GetType(-1) != GLua::TYPE_FUNCTION)
			{
				Lua()->Pop();
				info->finished = true;
#ifdef FILEBUG
				FILEBUG_WRITE("Not a function!?\n");
#endif

				// Emergency abort :/
				delete info;
				JobQueue.Remove(i);

				i = 0;

				continue;
			}

#ifdef FILEBUG
			FILEBUG_WRITE("Pushing nav\n");
#endif

			LUA_PushNav(L, info->nav);

#ifdef FILEBUG
			FILEBUG_WRITE("Pushed nav\n");
#endif

			if(info->findPath)
			{
#ifdef FILEBUG
				FILEBUG_WRITE("Find Path\n");
#endif

				Lua()->Push(info->foundPath);

				ILuaObject *pathTable = Lua()->GetNewTable();
				pathTable->Push();
				pathTable->UnReference();

				for(int i = 0; i < info->path.Count(); i++)
				{
					LUA_PushNode(L, info->path[i]);

					ILuaObject *node = Lua()->GetObject();
					pathTable = Lua()->GetObject(-2);
						pathTable->SetMember((float)i + 1, node);
						Lua()->Pop();
					pathTable->UnReference();
					node->UnReference();
				}

#ifdef FILEBUG
				FILEBUG_WRITE("Calling Callback 1\n");
#endif

				for(int i=0; i <= Lua()->Top(); i++)
				{
					FILEBUG_WRITE("Stack: %d | Type: %d | Type Name: %s\n", i, Lua()->GetType(i), Lua()->GetTypeName(Lua()->GetType(i)));
					if(Lua()->GetType(i) == GLua::TYPE_NUMBER)
					{
						FILEBUG_WRITE("\t%f\n", Lua()->GetNumber(i));
					}
				}

				Lua()->Call(3);

#ifdef FILEBUG
				FILEBUG_WRITE("Calling Callback 2\n");
#endif
			}
			else
			{
#ifdef FILEBUG
				FILEBUG_WRITE("Calling Callback 3\n");

				for(int i=0; i <= Lua()->Top(); i++)
				{
					FILEBUG_WRITE("Stack: %d | Type: %d | Type Name: %s\n", i, Lua()->GetType(i), Lua()->GetTypeName(Lua()->GetType(i)));
					if(Lua()->GetType(i) == GLua::TYPE_NUMBER)
					{
						FILEBUG_WRITE("\t%f\n", Lua()->GetNumber(i));
					}
				}
#endif

				Lua()->Call(1);

#ifdef FILEBUG
				FILEBUG_WRITE("Calling Callback 4\n");
#endif
			}

#ifdef FILEBUG
			FILEBUG_WRITE("Freeing Reference\n");
#endif

			info->objFunc->UnReference();

			if(info->objUpdate != NULL)
			{
				info->objUpdate->UnReference();
			}

#ifndef USE_BOOST_THREADS
			SafeRelease(info->job);
#endif

			delete info;
			JobQueue.Remove(i);

			i = 0;

#ifdef FILEBUG
			FILEBUG_WRITE("Job Completely Finished\n");
#endif
		}
		else if(!info->findPath && info->updateTime > 0)
		{
			time_t now = time(NULL);

			if(difftime(now, info->updateTime) >= 1)
			{
				info->objUpdate->Push();
				if(Lua()->GetType(-1) != GLua::TYPE_FUNCTION)
				{
					Lua()->Pop();
					continue;
				}
				LUA_PushNav(L, info->nav);
				Lua()->Push((float)info->nav->GetNodes().Size());
				Lua()->Call(2);

				info->updateTime = now;
			}
		}
	}

	return 0;
}

///////////////////////////////////////////////

LUA_FUNCTION(Nav_GetNodeByID)
{
	Lua()->CheckType(1, NAV_TYPE);
	Lua()->CheckType(2, GLua::TYPE_NUMBER);

	LUA_PushNode(L, LUA_GetNav(L, 1)->GetNodeByID((int)Lua()->GetNumber(2) - 1));

	return 1;
}

LUA_FUNCTION(Nav_GetNodeTotal)
{
	Lua()->CheckType(1, NAV_TYPE);

	Lua()->Push((float)LUA_GetNav(L, 1)->GetNodes().Count());

	return 1;
}

LUA_FUNCTION(Nav_GetNodes)
{
	Lua()->CheckType(1, NAV_TYPE);

	NodeList_t& Nodes = LUA_GetNav(L, 1)->GetNodes();

	ILuaObject *NodeTable = Lua()->GetNewTable();
	NodeTable->Push();
	NodeTable->UnReference();

	for(int i = 0; i < Nodes.Count(); i++)
	{
		LUA_PushNode(L, Nodes[i]);
		ILuaObject *ObjNode = Lua()->GetObject();
		NodeTable = Lua()->GetObject(-2);
			NodeTable->SetMember((float)i + 1, ObjNode);
			Lua()->Pop();
		NodeTable->UnReference();
		ObjNode->UnReference();
	}

	return 1;
}

LUA_FUNCTION(Nav_GetNearestNodes)
{
	Lua()->CheckType(1, NAV_TYPE);
	Lua()->CheckType(2, GLua::TYPE_VECTOR);
	Lua()->CheckType(3, GLua::TYPE_NUMBER);

	Vector pos = LUA_GetVector(L, 2);

	int count = 1;
	double pt[3] = { pos.x, pos.y, pos.z };
	
	ILuaObject *NodeTable = Lua()->GetNewTable();
	NodeTable->Push();
	NodeTable->UnReference();

	Nav *nav = LUA_GetNav(L, 1);
	
	kdres *results = kd_nearest_range(nav->GetNodeTree(), pt, Lua()->GetNumber(3));

	while(results != NULL && !kd_res_end(results))
	{
		LUA_PushNode(L, (Node*)kd_res_item_data(results));
		ILuaObject *ObjNode = Lua()->GetObject();
		NodeTable = Lua()->GetObject(-2);
			NodeTable->SetMember((float)count++, ObjNode);
			Lua()->Pop();
		NodeTable->UnReference();
		ObjNode->UnReference();

		kd_res_next(results);
	}
	
	if(results != NULL)
	{
		kd_res_free(results);
	}

	return 1;
}

LUA_FUNCTION(Nav_ResetGeneration)
{
	Lua()->CheckType(1, NAV_TYPE);

	LUA_GetNav(L, 1)->ResetGeneration();

	return 0;
}

LUA_FUNCTION(Nav_SetupMaxDistance)
{
	Lua()->CheckType(1, NAV_TYPE);
	Lua()->CheckType(2, GLua::TYPE_VECTOR);
	Lua()->CheckType(3, GLua::TYPE_NUMBER);

	LUA_GetNav(L, 1)->SetupMaxDistance(LUA_GetVector(L, 2), Lua()->GetNumber(3));

	return 0;
}

LUA_FUNCTION(Nav_AddGroundSeed)
{
	Lua()->CheckType(1, NAV_TYPE);
	Lua()->CheckType(2, GLua::TYPE_VECTOR);
	Lua()->CheckType(3, GLua::TYPE_VECTOR);

	LUA_GetNav(L, 1)->AddGroundSeed(LUA_GetVector(L, 2), LUA_GetVector(L, 3));

	return 0;
}

LUA_FUNCTION(Nav_AddAirSeed)
{
	Lua()->CheckType(1, NAV_TYPE);
	Lua()->CheckType(2, GLua::TYPE_VECTOR);

	LUA_GetNav(L, 1)->AddAirSeed(LUA_GetVector(L, 2));

	return 0;
}

LUA_FUNCTION(Nav_ClearGroundSeeds)
{
	Lua()->CheckType(1, NAV_TYPE);

	LUA_GetNav(L, 1)->ClearGroundSeeds();

	return 0;
}

LUA_FUNCTION(Nav_ClearAirSeeds)
{
	Lua()->CheckType(1, NAV_TYPE);

	LUA_GetNav(L, 1)->ClearAirSeeds();

	return 0;
}

LUA_FUNCTION(Nav_Generate)
{
	Lua()->CheckType(1, NAV_TYPE);
	Lua()->CheckType(2, GLua::TYPE_FUNCTION);
	
	Nav *nav = LUA_GetNav(L, 1);

	JobInfo_t *info = new JobInfo_t;
	info->nav = nav;
	info->abort = false;
	info->finished = false;
	info->findPath = false;
	info->objFunc = Lua()->GetObject(2);

	if(Lua()->GetType(3) == GLua::TYPE_FUNCTION)
	{
		info->objUpdate = Lua()->GetObject(3);
		info->updateTime = time(NULL);
	}
	else
	{
		info->objUpdate = NULL;
		info->updateTime = 0;
	}

	nav->GenerateQueue(info);

#ifndef USE_BOOST_THREADS
	if(info->job != NULL)
	{
#endif
		JobQueue.AddToTail(info);
#ifndef USE_BOOST_THREADS
	}
	else
	{
#ifdef FILEBUG
		FILEBUG_WRITE("Invalid job!\n");
#endif
		// free ref info->updateRef
		delete info;
	}
	Lua()->Push((bool)(job != NULL));
#else
	Lua()->Push((bool)true);
#endif

#ifdef FILEBUG
	FILEBUG_WRITE("Nav_Generate\n");
#endif

	return 1;
}

LUA_FUNCTION(Nav_FullGeneration)
{
	Lua()->CheckType(1, NAV_TYPE);

	Nav *nav = LUA_GetNav(L, 1);

	float StartTime = engine->Time();

	nav->ResetGeneration();
	nav->FullGeneration(NULL);

	Lua()->Push(engine->Time() - StartTime);

	return 1;
}

LUA_FUNCTION(Nav_IsGenerated)
{
	Lua()->CheckType(1, NAV_TYPE);

	Lua()->Push(LUA_GetNav(L, 1)->IsGenerated());

	return 1;
}

LUA_FUNCTION(Nav_FindPath)
{
	Lua()->CheckType(1, NAV_TYPE);
	Lua()->CheckType(2, GLua::TYPE_FUNCTION);

	Nav* nav = LUA_GetNav(L, 1);

	JobInfo_t *info = new JobInfo_t;
	info->nav = nav;
	info->abort = false;
	info->finished = false;
	info->findPath = true;
	info->hull = false;
	info->objFunc = Lua()->GetObject(2);
	info->objUpdate = NULL;

	nav->FindPathQueue(info);

#ifndef USE_BOOST_THREADS
	if(info->job != NULL)
	{
#endif
		JobQueue.AddToTail(info);
#ifndef USE_BOOST_THREADS
	}
	else
	{
		// free ref info->funcRef
		delete info;
	}

	Lua()->Push((bool)(job != NULL));
#else
	Lua()->Push((bool)true);
#endif

	return 1;
}

LUA_FUNCTION(Nav_FindPathImmediate)
{
	Lua()->CheckType(1, NAV_TYPE);

	Nav* nav = LUA_GetNav(L, 1);

	JobInfo_t *info = new JobInfo_t;
	info->nav = nav;
	info->abort = false;
	info->finished = false;
	info->findPath = true;
	info->hull = false;

	nav->ExecuteFindPath(info, nav->GetStart(), nav->GetEnd());

	if(info->foundPath)
	{
		ILuaObject *pathTable = Lua()->GetNewTable();
		pathTable->Push();
		pathTable->UnReference();

		for(int i = 0; i < info->path.Count(); i++)
		{
			LUA_PushNode(L, info->path[i]);

			ILuaObject *node = Lua()->GetObject();
			pathTable = Lua()->GetObject(-2);
				pathTable->SetMember((float)i + 1, node);
				Lua()->Pop();
			pathTable->UnReference();
			node->UnReference();
		}
	}
	else
	{
		Lua()->Push((bool)false);
	}

	delete info;

	return 1;
}

LUA_FUNCTION(Nav_FindPathHull)
{
	Lua()->CheckType(1, NAV_TYPE);
	Lua()->CheckType(2, GLua::TYPE_VECTOR);
	Lua()->CheckType(3, GLua::TYPE_VECTOR);
	Lua()->CheckType(4, GLua::TYPE_FUNCTION);

	Nav* nav = LUA_GetNav(L, 1);

	JobInfo_t *info = new JobInfo_t;
	info->nav = nav;
	info->abort = false;
	info->finished = false;
	info->findPath = true;
	info->hull = true;
	info->mins = LUA_GetVector(L, 2);
	info->maxs = LUA_GetVector(L, 3);
	info->objFunc = Lua()->GetObject(4);
	info->objUpdate = NULL;

	nav->FindPathQueue(info);

#ifndef USE_BOOST_THREADS
	if(info->job != NULL)
	{
#endif
		JobQueue.AddToTail(info);
#ifndef USE_BOOST_THREADS
	}
	else
	{
		// free ref info->funcRef
		delete info;
	}

	Lua()->Push((bool)(job != NULL));
#else
	Lua()->Push((bool)true);
#endif

	return 1;
}

LUA_FUNCTION(Nav_FindPathHullImmediate)
{
	Lua()->CheckType(1, NAV_TYPE);
	Lua()->CheckType(2, GLua::TYPE_VECTOR);
	Lua()->CheckType(3, GLua::TYPE_VECTOR);

	Nav* nav = LUA_GetNav(L, 1);

	JobInfo_t *info = new JobInfo_t;
	info->nav = nav;
	info->abort = false;
	info->finished = false;
	info->findPath = true;
	info->hull = true;
	info->mins = LUA_GetVector(L, 2);
	info->maxs = LUA_GetVector(L, 3);

	nav->ExecuteFindPath(info, nav->GetStart(), nav->GetEnd());

	if(info->foundPath)
	{
		ILuaObject *pathTable = Lua()->GetNewTable();
		pathTable->Push();
		pathTable->UnReference();

		for(int i = 0; i < info->path.Count(); i++)
		{
			LUA_PushNode(L, info->path[i]);

			ILuaObject *node = Lua()->GetObject();
			pathTable = Lua()->GetObject(-2);
				pathTable->SetMember((float)i + 1, node);
				Lua()->Pop();
			pathTable->UnReference();
			node->UnReference();
		}
	}
	else
	{
		Lua()->Push((bool)false);
	}

	delete info;

	return 1;
}

LUA_FUNCTION(Nav_GetHeuristic)
{
	Lua()->CheckType(1, NAV_TYPE);

	Lua()->Push((float)LUA_GetNav(L, 1)->GetHeuristic());

	return 1;
}

LUA_FUNCTION(Nav_GetStart)
{
	Lua()->CheckType(1, NAV_TYPE);

	LUA_PushNode(L, LUA_GetNav(L, 1)->GetStart());

	return 1;
}

LUA_FUNCTION(Nav_GetEnd)
{
	Lua()->CheckType(1, NAV_TYPE);

	LUA_PushNode(L, LUA_GetNav(L, 1)->GetEnd());

	return 1;
}

LUA_FUNCTION(Nav_SetHeuristic)
{
	Lua()->CheckType(1, NAV_TYPE);
	Lua()->CheckType(2, GLua::TYPE_NUMBER);

	LUA_GetNav(L, 1)->SetHeuristic(Lua()->GetNumber(2));

	return 0;
}

LUA_FUNCTION(Nav_SetStart)
{
	Lua()->CheckType(1, NAV_TYPE);
	Lua()->CheckType(2, NODE_TYPE);

	LUA_GetNav(L, 1)->SetStart(LUA_GetNode(L, 2));

	return 0;
}

LUA_FUNCTION(Nav_SetEnd)
{
	Lua()->CheckType(1, NAV_TYPE);
	Lua()->CheckType(2, NODE_TYPE);

	LUA_GetNav(L, 1)->SetEnd(LUA_GetNode(L, 2));

	return 0;
}

LUA_FUNCTION(Nav_GetNode)
{
	Lua()->CheckType(1, NAV_TYPE);
	Lua()->CheckType(2, GLua::TYPE_VECTOR);

	LUA_PushNode(L, LUA_GetNav(L, 1)->GetNode(LUA_GetVector(L, 2)));

	return 1;
}

LUA_FUNCTION(Nav_GetClosestNode)
{
	Lua()->CheckType(1, NAV_TYPE);
	Lua()->CheckType(2, GLua::TYPE_VECTOR);

	LUA_PushNode(L, LUA_GetNav(L, 1)->GetClosestNode(LUA_GetVector(L, 2)));

	return 1;
}

LUA_FUNCTION(Nav_Save)
{
	Lua()->CheckType(1, NAV_TYPE);
	Lua()->CheckType(2, GLua::TYPE_STRING);

	Lua()->Push(LUA_GetNav(L, 1)->Save(Lua()->GetString(2)));

	return 1;
}

LUA_FUNCTION(Nav_Load)
{
	Lua()->CheckType(1, NAV_TYPE);
	Lua()->CheckType(2, GLua::TYPE_STRING);

	Lua()->Push(LUA_GetNav(L, 1)->Load(Lua()->GetString(2)));

	return 1;
}

LUA_FUNCTION(Nav_GetDiagonal)
{
	Lua()->CheckType(1, NAV_TYPE);

	Lua()->Push(LUA_GetNav(L, 1)->GetDiagonal());

	return 1;
}

LUA_FUNCTION(Nav_SetDiagonal)
{
	Lua()->CheckType(1, NAV_TYPE);

	LUA_GetNav(L, 1)->SetDiagonal(Lua()->GetBool(2));

	return 0;
}

LUA_FUNCTION(Nav_GetGridSize)
{
	Lua()->CheckType(1, NAV_TYPE);
	
	Lua()->Push((float)LUA_GetNav(L, 1)->GetGridSize());

	return 1;
}

LUA_FUNCTION(Nav_SetGridSize)
{
	Lua()->CheckType(1, NAV_TYPE);
	Lua()->CheckType(2, GLua::TYPE_NUMBER);

	LUA_GetNav(L, 1)->SetGridSize(Lua()->GetInteger(2));

	return 0;
}

LUA_FUNCTION(Nav_GetMask)
{
	Lua()->CheckType(1, NAV_TYPE);
	
	Lua()->Push((float)LUA_GetNav(L, 1)->GetMask());

	return 1;
}

LUA_FUNCTION(Nav_SetMask)
{
	Lua()->CheckType(1, NAV_TYPE);
	Lua()->CheckType(2, GLua::TYPE_NUMBER);

	LUA_GetNav(L, 1)->SetMask(Lua()->GetInteger(2));

	return 0;
}

LUA_FUNCTION(Nav_CreateNode)
{
	Lua()->CheckType(1, NAV_TYPE);
	Lua()->CheckType(2, GLua::TYPE_VECTOR);
	Lua()->CheckType(3, GLua::TYPE_VECTOR);

	LUA_PushNode(L, LUA_GetNav(L, 1)->AddNode(LUA_GetVector(L, 2), LUA_GetVector(L, 3), NORTH, NULL)); // This dir doesn't matter

	return 1;
}

LUA_FUNCTION(Nav_RemoveNode)
{
	Lua()->CheckType(1, NAV_TYPE);
	Lua()->CheckType(2, NODE_TYPE);

	LUA_GetNav(L, 1)->RemoveNode(LUA_GetNode(L, 2));

	return 0;
}

///////////////////////////////////////////////

LUA_FUNCTION(Node__eq)
{
	Lua()->CheckType(1, NODE_TYPE);
	Lua()->CheckType(2, NODE_TYPE);

	Lua()->Push((bool)(LUA_GetNode(L, 1)->GetID() == LUA_GetNode(L, 2)->GetID()));

	return 1;
}

LUA_FUNCTION(Node_GetID)
{
	Lua()->CheckType(1, NODE_TYPE);

	Lua()->Push((float)LUA_GetNode(L, 1)->GetID() + 1);

	return 1;
}

LUA_FUNCTION(Node_GetPosition)
{
	Lua()->CheckType(1, NODE_TYPE);

	LUA_PushVector(L, LUA_GetNode(L, 1)->vecPos);

	return 1;
}

LUA_FUNCTION(Node_GetNormal)
{
	Lua()->CheckType(1, NODE_TYPE);

	LUA_PushVector(L, LUA_GetNode(L, 1)->vecNormal);

	return 1;
}

LUA_FUNCTION(Node_IsDisabled)
{
	Lua()->CheckType(1, NODE_TYPE);

	Lua()->Push(LUA_GetNode(L, 1)->IsDisabled());

	return 1;
}

LUA_FUNCTION(Node_SetDisabled)
{
	Lua()->CheckType(1, NODE_TYPE);
	Lua()->CheckType(2, GLua::TYPE_BOOL);

	LUA_GetNode(L, 1)->SetDisabled(Lua()->GetBool(2));

	return 0;
}

LUA_FUNCTION(Node_GetConnections)
{
	Lua()->CheckType(1, NODE_TYPE);

	Node *node = LUA_GetNode(L, 1);

	// Push the table on the stack to survive past 510 calls!
	ILuaObject *NodeTable = Lua()->GetNewTable();
	NodeTable->Push();
	NodeTable->UnReference();
	
	Node *Connection;

	for(int Dir = NORTH; Dir < NUM_DIRECTIONS_MAX; Dir++)
	{
		Connection = node->GetConnectedNode((NavDirType)Dir);
		if(Connection != NULL)
		{
			LUA_PushNode(L, Connection);
			ILuaObject *ObjNode = Lua()->GetObject();
			NodeTable = Lua()->GetObject(-2);
				NodeTable->SetMember((float)Dir, ObjNode);
				Lua()->Pop();
			NodeTable->UnReference();
			ObjNode->UnReference();
		}
	}


	return 1;
}

LUA_FUNCTION(Node_GetConnectedNode)
{
	Lua()->CheckType(1, NODE_TYPE);
	Lua()->CheckType(2, GLua::TYPE_NUMBER);

	int dir = Lua()->GetInteger(2);

	if(dir >= 0 && dir < NUM_DIRECTIONS_MAX)
	{
		Node *pNode = LUA_GetNode(L, 1);

		Node *pNodeConnection = pNode->GetConnectedNode((NavDirType)dir);
		if(pNodeConnection != NULL)
		{
			LUA_PushNode(L, pNodeConnection);
			return 1;
		}
	}

	Lua()->Push(false);

	return 1;
}

LUA_FUNCTION(Node_GetScoreG)
{
	Lua()->CheckType(1, NODE_TYPE);

	Lua()->Push(LUA_GetNode(L, 1)->GetScoreG());

	return 1;
}

LUA_FUNCTION(Node_GetScoreF)
{
	Lua()->CheckType(1, NODE_TYPE);

	Lua()->Push(LUA_GetNode(L, 1)->GetScoreF());

	return 1;
}

LUA_FUNCTION(Node_IsConnected)
{
	Lua()->CheckType(1, NODE_TYPE);
	Lua()->CheckType(2, NODE_TYPE);

	Node *Node1 = LUA_GetNode(L, 1);
	Node *Node2 = LUA_GetNode(L, 2);

	Node *Connection;
	for(int Dir = NORTH; Dir < NUM_DIRECTIONS_MAX; Dir++)
	{
		Connection = Node1->GetConnectedNode((NavDirType)Dir);
		if(Connection != NULL && Connection == Node2)
		{
			Lua()->Push(true);

			return 1;
		}
	}

	Lua()->Push(false);

	return 1;
}

LUA_FUNCTION(Node_SetNormal)
{
	Lua()->CheckType(1, NODE_TYPE);
	Lua()->CheckType(2, GLua::TYPE_VECTOR);

	LUA_GetNode(L, 1)->SetNormal(LUA_GetVector(L, 2));

	return 0;
}

LUA_FUNCTION(Node_SetPosition)
{
	Lua()->CheckType(1, NODE_TYPE);
	Lua()->CheckType(2, GLua::TYPE_VECTOR);

	LUA_GetNode(L, 1)->SetPosition(LUA_GetVector(L, 2));

	return 0;
}

LUA_FUNCTION(Node_ConnectTo)
{
	Lua()->CheckType(1, NODE_TYPE);
	Lua()->CheckType(2, NODE_TYPE);
	Lua()->CheckType(3, GLua::TYPE_NUMBER);

	Node *Node1 = LUA_GetNode(L, 1);
	Node *Node2 = LUA_GetNode(L, 2);
	NavDirType Dir = (NavDirType)Lua()->GetInteger(3);

	Node1->ConnectTo(Node2, Dir);
	Node2->ConnectTo(Node1, Nav::OppositeDirection(Dir));

	Node1->MarkAsVisited(Dir);
	Node1->MarkAsVisited(Nav::OppositeDirection(Dir));

	Node2->MarkAsVisited(Dir);

	return 0;
}

LUA_FUNCTION(Node_RemoveConnection)
{
	Lua()->CheckType(1, NODE_TYPE);
	Lua()->CheckType(2, GLua::TYPE_NUMBER);

	Node *Node1 = LUA_GetNode(L, 1);
	NavDirType Dir = (NavDirType)Lua()->GetInteger(2);

	Node *Node2 = Node1->GetConnectedNode(Dir);
	if(Node2 != NULL)
	{
		Node1->ConnectTo(NULL, Dir);
		Node2->ConnectTo(NULL, Nav::OppositeDirection(Dir));

		// UnMarkAsVisited?
		// I don't really know bitwise too well
	}

	return 0;
}

///////////////////////////////////////////////

int Init(lua_State* L)
{
	CreateInterfaceFn interfaceFactory = Sys_GetFactory("engine.dll");

	engine = (IVEngineServer*)interfaceFactory(INTERFACEVERSION_VENGINESERVER, NULL);

	if(!engine)
	{
		Lua()->Error("gm_navigation: Missing IVEngineServer interface.\n");
	}

#ifdef LUA_SERVER
	enginetrace = (IEngineTrace*)interfaceFactory(INTERFACEVERSION_ENGINETRACE_SERVER, NULL);
#else
	enginetrace = (IEngineTrace*)interfaceFactory(INTERFACEVERSION_ENGINETRACE_CLIENT, NULL);
#endif
	
	if(!enginetrace)
	{
		Lua()->Error("gm_navigation: Missing IEngineTrace interface.\n");
	}

#ifndef USE_BOOST_THREADS
	threadPool = CreateThreadPool();

	ThreadPoolStartParams_t params;
	params.nThreads = 2;

	if(!threadPool->Start(params))
	{
		SafeRelease(threadPool);
		Lua()->Error("gm_navigation: Thread pool is null\n");
	}
#endif

	ILuaObject* navTable = Lua()->GetNewTable();
		navTable->SetMember("Create", nav_Create);
		navTable->SetMember("Poll", nav_Poll);

		navTable->SetMember("NORTH", (float)NORTH);
		navTable->SetMember("SOUTH", (float)SOUTH);
		navTable->SetMember("EAST", (float)EAST);
		navTable->SetMember("WEST", (float)WEST);

		navTable->SetMember("NORTHEAST", (float)NORTHEAST);
		navTable->SetMember("NORTHWEST", (float)NORTHWEST);
		navTable->SetMember("SOUTHEAST", (float)SOUTHEAST);
		navTable->SetMember("SOUTHWEST", (float)SOUTHWEST);

		navTable->SetMember("UP", (float)UP);
		navTable->SetMember("DOWN", (float)DOWN);
		navTable->SetMember("LEFT", (float)LEFT);
		navTable->SetMember("RIGHT", (float)RIGHT);
		navTable->SetMember("FORWARD", (float)FORWARD);
		navTable->SetMember("BACKWARD", (float)BACKWARD);

		navTable->SetMember("NUM_DIRECTIONS", (float)NUM_DIRECTIONS);
		navTable->SetMember("NUM_DIRECTIONS_DIAGONAL", (float)NUM_DIRECTIONS_DIAGONAL);
		navTable->SetMember("NUM_DIRECTIONS_MAX", (float)NUM_DIRECTIONS_MAX);

		navTable->SetMember("HEURISTIC_MANHATTAN", (float)Nav::HEURISTIC_MANHATTAN);
		navTable->SetMember("HEURISTIC_EUCLIDEAN", (float)Nav::HEURISTIC_EUCLIDEAN);
		navTable->SetMember("HEURISTIC_CUSTOM", (float)Nav::HEURISTIC_CUSTOM);

	Lua()->SetGlobal("nav", navTable);
	navTable->UnReference();

	ILuaObject *MetaNav = Lua()->GetMetaTable(NAV_NAME, NAV_TYPE);
		ILuaObject *NavIndex = Lua()->GetNewTable();
			NavIndex->SetMember("GetNodeByID", Nav_GetNodeByID);
			NavIndex->SetMember("GetNodes", Nav_GetNodes);
			NavIndex->SetMember("GetNearestNodes", Nav_GetNearestNodes);

			NavIndex->SetMember("GetNodeTotal", Nav_GetNodeTotal);

			NavIndex->SetMember("AddWalkableSeed", Nav_AddGroundSeed); // TODO: Remove
			NavIndex->SetMember("AddGroundSeed", Nav_AddGroundSeed);
			NavIndex->SetMember("AddAirSeed", Nav_AddAirSeed);

			NavIndex->SetMember("ClearWalkableSeeds", Nav_ClearGroundSeeds); // TODO: Remove
			NavIndex->SetMember("ClearGroundSeeds", Nav_ClearGroundSeeds);
			NavIndex->SetMember("ClearAirSeeds", Nav_ClearAirSeeds);

			NavIndex->SetMember("SetupMaxDistance", Nav_SetupMaxDistance);
			NavIndex->SetMember("Generate", Nav_Generate);
			NavIndex->SetMember("FullGeneration", Nav_FullGeneration);
			NavIndex->SetMember("IsGenerated", Nav_IsGenerated);
			NavIndex->SetMember("FindPath", Nav_FindPath);
			NavIndex->SetMember("FindPathImmediate", Nav_FindPathImmediate);
			NavIndex->SetMember("FindPathHull", Nav_FindPathHull);
			NavIndex->SetMember("GetHeuristic", Nav_GetHeuristic);
			NavIndex->SetMember("GetStart", Nav_GetStart);
			NavIndex->SetMember("GetEnd", Nav_GetEnd);
			NavIndex->SetMember("SetHeuristic", Nav_SetHeuristic);
			NavIndex->SetMember("SetStart", Nav_SetStart);
			NavIndex->SetMember("SetEnd", Nav_SetEnd);
			NavIndex->SetMember("GetNode", Nav_GetNode);
			NavIndex->SetMember("GetClosestNode", Nav_GetClosestNode);
			NavIndex->SetMember("GetNearestNodes", Nav_GetNearestNodes);
			NavIndex->SetMember("GetNodesInSphere", Nav_GetNearestNodes);
			NavIndex->SetMember("Save", Nav_Save);
			NavIndex->SetMember("Load", Nav_Load);
			NavIndex->SetMember("GetDiagonal", Nav_GetDiagonal);
			NavIndex->SetMember("SetDiagonal", Nav_SetDiagonal);
			NavIndex->SetMember("GetGridSize", Nav_GetGridSize);
			NavIndex->SetMember("SetGridSize", Nav_SetGridSize);
			NavIndex->SetMember("GetMask", Nav_GetMask);
			NavIndex->SetMember("SetMask", Nav_SetMask);
			NavIndex->SetMember("CreateNode", Nav_CreateNode);
			NavIndex->SetMember("RemoveNode", Nav_RemoveNode);
		MetaNav->SetMember("__index", NavIndex);
		NavIndex->UnReference();
	MetaNav->UnReference();

	ILuaObject *MetaNode = Lua()->GetMetaTable(NODE_NAME, NODE_TYPE);
		ILuaObject *NodeIndex = Lua()->GetNewTable();
			NodeIndex->SetMember("GetID", Node_GetID);
			NodeIndex->SetMember("GetPosition", Node_GetPosition);
			NodeIndex->SetMember("GetPos", Node_GetPosition);
			NodeIndex->SetMember("GetNormal", Node_GetNormal);
			NodeIndex->SetMember("GetConnections", Node_GetConnections);
			NodeIndex->SetMember("GetConnectedNode", Node_GetConnectedNode);
			NodeIndex->SetMember("GetScoreF", Node_GetScoreF);
			NodeIndex->SetMember("GetScoreG", Node_GetScoreG);
			NodeIndex->SetMember("IsConnected", Node_IsConnected);
			NodeIndex->SetMember("IsDisabled", Node_IsDisabled);
			NodeIndex->SetMember("SetDisabled", Node_SetDisabled);
			NodeIndex->SetMember("SetNormal", Node_SetNormal);
			NodeIndex->SetMember("SetPosition", Node_SetPosition);
			NodeIndex->SetMember("ConnectTo", Node_ConnectTo);
			NodeIndex->SetMember("RemoveConnection", Node_RemoveConnection);
		MetaNode->SetMember("__index", NodeIndex);
		MetaNode->SetMember("__eq", Node__eq);
		NodeIndex->UnReference();
	MetaNode->UnReference();

	// Based on azuisleet's method.
	// hook.Add("Think", "NavPoll", nav.Poll)
	ILuaObject *hook = Lua()->GetGlobal("hook");
	ILuaObject *add = hook->GetMember("Add");
	add->Push();
		Lua()->Push("Think");
		Lua()->Push("NavPoll");
		Lua()->Push(nav_Poll);
	Lua()->Call(3);
	hook->UnReference();
	add->UnReference();

#ifdef LUA_SERVER
	Msg("gmsv_navigation_win32: Loaded\n");
#else
	Msg("gmcl_navigation_win32: Loaded\n");
#endif

#ifdef FILEBUG
	pDebugFile = fopen("garrysmod/filebug.txt", "a+");
	FILEBUG_WRITE("Opened Module\n");
#endif

	return 0;
}

int Shutdown(lua_State* L)
{
#ifdef USE_BOOST_THREADS
	Msg("gm_navigation: Aborting threads...\n");
	for(int i=0; i < JobQueue.Size(); i++)
	{
		JobQueue[i]->abort = true;

		/*
		if(JobQueue[i]->thread.joinable())
		{
			JobQueue[i]->thread.join();
		}
		*/
		
		Msg("gm_navigation: Aborting Thread %d\n", i);

#ifdef FILEBUG
		FILEBUG_WRITE("Aborting...\n");
#endif

		while(!JobQueue[i]->finished && !JobQueue[i]->thread.timed_join(timeout))
		{
			boost::this_thread::sleep(sleep_time);
			Msg("Waiting...\n");
		}

#ifdef FILEBUG
		FILEBUG_WRITE("Aborted\n");
#endif

		Msg("gm_navigation: Aborted\n");
	}

	Msg("gm_navigation: Done\n");

#else
	if(threadPool != NULL)
	{
		for(int i=0; i < JobQueue.Size(); i++)
		{
			JobQueue[i]->abort = true;
		}
		threadPool->Stop();
		DestroyThreadPool(threadPool);
		threadPool = NULL;
	}
#endif

	if(objVector)
	{
		objVector->UnReference();
		objVector = NULL;
	}

#ifdef FILEBUG
	if(pDebugFile != NULL)
	{
		fputs("Closed Module\n", pDebugFile);
		fclose(pDebugFile);
	}
#endif

	return 0;
}
