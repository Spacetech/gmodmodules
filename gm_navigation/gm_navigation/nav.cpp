/*
    gm_navigation
    By Spacetech
*/

#include "nav.h"
#include "ILuaModuleManager.h"
#include "tier0/memdbgon.h"

extern IEngineTrace *enginetrace;

#ifndef USE_BOOST_THREADS
extern IThreadPool *threadPool;
#endif

extern FileHandle_t fh;
extern FILE* pDebugFile;

Nav* LUA_GetNav(lua_State* L, int pos)
{
	Nav* nav = (Nav*)Lua()->GetUserData(pos);
	if(nav == NULL)
	{
		Lua()->LuaError("Invalid Nav", pos);
	}
	return nav;
}

void LUA_PushNav(lua_State* L, Nav *nav)
{
	if(nav)
	{
		ILuaObject* meta = Lua()->GetMetaTable(NAV_NAME, NAV_TYPE);
		if(!meta->isTable())
		{
			Lua()->Push(false);
		}
		else
		{
			Lua()->PushUserData(meta, nav, NAV_TYPE);
		}
		meta->UnReference();
	}
	else
	{
		Lua()->Push(false);
	}
}

class GMOD_TraceFilter : public CTraceFilter
{
public:
	GMOD_TraceFilter()
	{
	}

	virtual bool ShouldHitEntity(IHandleEntity *pHandleEntity, int contentsMask)
	{
#ifdef LUA_SERVER
		IServerUnknown *pUnk = (IServerUnknown*)pHandleEntity;
#else
		IClientUnknown *pUnk = (IClientUnknown*)pHandleEntity;
#endif
		pCollide = pUnk->GetCollideable();
		return CheckCollisionGroup(pCollide->GetCollisionGroup());
	}

	bool CheckCollisionGroup(int collisionGroup)
	{
		if(collisionGroup == COLLISION_GROUP_PLAYER)
		{
			return false;
		}
		return true;
	}

private:
	ICollideable *pCollide;
};

inline void UTIL_TraceLine_GMOD(const Vector& vecAbsStart, const Vector& vecAbsEnd, unsigned int mask, trace_t *ptr)
{
	if(enginetrace == NULL)
	{
		return;
	}
	Ray_t ray;
	ray.Init(vecAbsStart, vecAbsEnd);
	GMOD_TraceFilter traceFilter;
	enginetrace->TraceRay(ray, mask, &traceFilter, ptr);
}

inline void UTIL_TraceHull_GMOD(const Vector& vecAbsStart, const Vector& vecAbsEnd, const Vector& vecMins, const Vector& vecMaxs, unsigned int mask, trace_t *ptr)
{
	if(enginetrace == NULL)
	{
		return;
	}
	Ray_t ray;
	ray.Init(vecAbsStart, vecAbsEnd, vecMins, vecMaxs);
	GMOD_TraceFilter traceFilter;
	enginetrace->TraceRay(ray, mask, &traceFilter, ptr);
}

Nav::Nav(int GridSize)
{
	heuristic = HEURISTIC_MANHATTAN;

	nodeStart = NULL;
	nodeEnd = NULL;

	generated = false;
	generating = false;
	generationMaxDistance = -1;

	mask = MASK_PLAYERSOLID_BRUSHONLY;

	SetDiagonal(false);
	SetGridSize(GridSize);

	nodes.EnsureCapacity(10240);
	groundSeedList.EnsureCapacity(16);
	airSeedList.EnsureCapacity(16);

	nodeTree = kd_create(3);

#ifdef FILEBUG
	FILEBUG_WRITE("Created Nav\n");
#endif
}

Nav::~Nav()
{
#ifdef FILEBUG
	FILEBUG_WRITE("Deconstructing Nav\n");
#endif
	
	Msg("Deconstructing Nav: %p\n", this);

#ifdef FILEBUG
	FILEBUG_WRITE("Freeing nodes\n");
#endif

	kd_free(nodeTree);

	for(int i=0; i < nodes.Count(); i++)
	{
		Node *pNode = nodes[i];
		delete pNode;
		pNode = NULL;
	}
	nodes.Purge();

#ifdef FILEBUG
	FILEBUG_WRITE("Freed nodes\n");
#endif
}

NavDirType Nav::OppositeDirection(NavDirType Dir)
{
	switch(Dir)
	{
		case NORTH: return SOUTH;
		case SOUTH: return NORTH;
		case EAST:	return WEST;
		case WEST:	return EAST;
		case NORTHEAST: return SOUTHWEST;
		case NORTHWEST: return SOUTHEAST;
		case SOUTHEAST:	return NORTHWEST;
		case SOUTHWEST: return NORTHEAST;
		case UP: return DOWN;
		case DOWN: return UP;
		case LEFT:	return RIGHT;
		case RIGHT:	return LEFT;
		case FORWARD: return BACKWARD;
		case BACKWARD: return FORWARD;
	}

	return NORTH;
}

void Nav::SetGridSize(int GridSize)
{
	generationStepSize = GridSize;
	vecAddOffset = Vector(0, 0, GridSize / 2);
}

Node *Nav::GetNode(const Vector &Pos)
{
	const float Tolerance = 0.45f * generationStepSize;

	Node *Node;
	for(int i = 0; i < nodes.Count(); i++)
	{
		Node = nodes[i];
		float dx = fabs(Node->vecPos.x - Pos.x);
		float dy = fabs(Node->vecPos.y - Pos.y);
		float dz = fabs(Node->vecPos.z - Pos.z);
		if(dx < Tolerance && dy < Tolerance && dz < Tolerance)
		{
			return Node;
		}
	}

	return NULL;
}

Node *Nav::GetNodeByID(int ID)
{
	if(!nodes.IsValidIndex(ID))
	{
		return NULL;
	}
	return nodes.Element(ID);
}

Node *Nav::AddNode(const Vector &Pos, const Vector &Normal, NavDirType Dir, Node *Source)
{
	if(generationMaxDistance > 0)
	{
		if(vecOrigin.DistTo(Pos) > generationMaxDistance)
		{
			return NULL;
		}
	}

	// check if a node exists at this location
	Node *node = GetNode(Pos);
	
	// if no node exists, create one
	bool UseNew = false;
	if(node == NULL)
	{
		UseNew = true;
		node = new Node(Pos, Normal, Source);
		node->SetID(nodes.AddToTail(node));

		kd_insert3f(nodeTree, Pos.x, Pos.y, Pos.z, node);

#ifdef FILEBUG
		FILEBUG_WRITE("Adding Node <%f, %f>\n", Pos.x, Pos.y);
#endif
	}
	else
	{
#ifdef FILEBUG
		FILEBUG_WRITE("Using Existing Node <%f, %f>\n", Pos.x, Pos.y);
#endif
	}

	if(Source != NULL)
	{
		// connect source node to new node
		Source->ConnectTo(node, Dir);

		node->ConnectTo(Source, OppositeDirection(Dir));
		node->MarkAsVisited(OppositeDirection(Dir));

		if(UseNew)
		{
			// new node becomes current node
			currentNode = node;
		}
	}

	/* hmmmmm
	// optimization: if deltaZ changes very little, assume connection is commutative
	const float zTolerance = 50.0f;
	if(fabs(Source->GetPosition()->z - Pos.z) < zTolerance)
	{
		Node->ConnectTo(Source, OppositeDirection(Dir));
		Node->MarkAsVisited(OppositeDirection(Dir));
	}
	*/

	return node;
}

void Nav::RemoveNode(Node *node)
{
	Node *Connection;
	for(int Dir = NORTH; Dir < NUM_DIRECTIONS_MAX; Dir++)
	{
		Connection = node->GetConnectedNode((NavDirType)Dir);
		if(Connection != NULL)
		{
			Connection->ConnectTo(NULL, OppositeDirection((NavDirType)Dir));
		}
	}

	nodes.Remove(node->GetID());

	delete node;
	node = NULL;

	// Update all node ids, removing a element from a utlvector will shift all the elements positions
	for(int i = 0; i < nodes.Count(); i++)
	{
		nodes[i]->SetID(i);
	}

	kd_clear(nodeTree);

	Node *pNode;
	for(int i = 0; i < nodes.Count(); i++)
	{
		pNode = nodes[i];
		kd_insert3f(nodeTree, pNode->vecPos.x, pNode->vecPos.y, pNode->vecPos.z, pNode);
	}

}

int Nav::GetGridSize()
{
	return generationStepSize;
}

bool Nav::IsGenerated()
{
	return generated;
}

float Nav::SnapToGrid(float x)
{
	return Round(x, GetGridSize());
}

Vector Nav::SnapToGrid(const Vector& in, bool snapX, bool snapY)
{
	int scale = GetGridSize();

	Vector out(in);

	if(snapX)
	{
		out.x = Round(in.x, scale);
	}

	if(snapY)
	{
		out.y = Round(in.y, scale);
	}

	return out;
}

float Nav::Round(float Val, float Unit)
{
	Val = Val + ((Val < 0.0f) ? -Unit*0.5f : Unit*0.5f);
	return (float)(Unit * ((int)Val) / (int)Unit);
}

bool Nav::GetGroundHeight(const Vector &pos, float *height, Vector *normal)
{
	Vector to;
	to.x = pos.x;
	to.y = pos.y;
	to.z = pos.z - 9999.9f;

	float offset;
	Vector from;
	trace_t result;

	const float maxOffset = 100.0f;
	const float inc = 10.0f;

	struct GroundLayerInfo
	{
		float ground;
		Vector normal;
	}
	layer[ MAX_GROUND_LAYERS ];
	int layerCount = 0;

	for( offset = 1.0f; offset < maxOffset; offset += inc )
	{
		from = pos + Vector( 0, 0, offset );

		//GMU->TraceLine( from, to, mask, &result);
		UTIL_TraceLine_GMOD(from, to, mask, &result);

		// if the trace came down thru a door, ignore the door and try again
		// also ignore breakable floors

		if (result.startsolid == false)
		{
			// if we didnt start inside a solid area, the trace hit a ground layer

			// if this is a new ground layer, add it to the set
			if (layerCount == 0 || result.endpos.z > layer[ layerCount-1 ].ground)
			{
				layer[ layerCount ].ground = result.endpos.z;
				if (result.plane.normal.IsZero())
					layer[ layerCount ].normal = Vector( 0, 0, 1 );
				else
					layer[ layerCount ].normal = result.plane.normal;

				++layerCount;
						
				if (layerCount == MAX_GROUND_LAYERS)
					break;
			}
		}
	}

	if (layerCount == 0)
		return false;

	// find the lowest layer that allows a player to stand or crouch upon it
	int i;
	for( i=0; i<layerCount-1; ++i )
	{
		if (layer[i+1].ground - layer[i].ground >= HumanHeight)
			break;		
	}

	*height = layer[ i ].ground;

	if (normal)
		*normal = layer[ i ].normal;

	return true;
}

#ifdef USE_BOOST_THREADS
void Nav::GenerateQueue(JobInfo_t *info)
{
	if(generating)
	{
		return;
	}

#ifdef FILEBUG
	FILEBUG_WRITE("GenerateQueue 1\n");
#endif

	ResetGeneration();

#ifdef FILEBUG
	FILEBUG_WRITE("GenerateQueue 2\n");
#endif

	info->thread = boost::thread(&Nav::FullGeneration, this, &info->abort);

#ifdef FILEBUG
	FILEBUG_WRITE("Created Job\n");
#endif
}
#else
void Nav::GenerateQueue(JobInfo_t *info)
{
	if(generating)
	{
		return;
	}

#ifdef FILEBUG
	FILEBUG_WRITE("GenerateQueue 1\n");
#endif

	ResetGeneration();

#ifdef FILEBUG
	FILEBUG_WRITE("GenerateQueue 2\n");
#endif

	info->job = threadPool->QueueCall(this, &Nav::FullGeneration, &info->abort);

#ifdef FILEBUG
	FILEBUG_WRITE("Created Job\n");
#endif
}
#endif

void Nav::FullGeneration(bool *abort)
{
#ifdef FILEBUG
	FILEBUG_WRITE("FullGeneration nodeStart\n");
#endif

	while(!SampleStep() && (abort == NULL || *abort == false))
	{
	}

#ifdef FILEBUG
	FILEBUG_WRITE("FullGeneration nodeEnd\n");
#endif
}

void Nav::ResetGeneration()
{
	groundSeedIndex = 0;
	airSeedIndex = 0;
	currentNode = NULL;

	nodeStart = NULL;
	nodeEnd = NULL;

	generating = true;
	generated = false;
	generatingGround = true;

	kd_clear(nodeTree);

	for(int i=0; i < nodes.Count(); i++)
	{
		Node *pNode = nodes[i];
		delete pNode;
		pNode = NULL;
	}

	nodes.Purge();	
}

void Nav::SetupMaxDistance(const Vector &Pos, int MaxDistance)
{
	vecOrigin = Pos;
	generationMaxDistance = MaxDistance;
}

void Nav::AddGroundSeed(const Vector &pos, const Vector &normal)
{
	GroundSeedSpot seed;
	seed.pos = SnapToGrid(pos);
	seed.normal = normal;
	groundSeedList.AddToTail(seed);
}

void Nav::AddAirSeed(const Vector &pos)
{
	AirSeedSpot seed;
	seed.pos = SnapToGrid(pos);
	airSeedList.AddToTail(seed);
}

void Nav::ClearGroundSeeds()
{
	groundSeedList.RemoveAll();
}

void Nav::ClearAirSeeds()
{
	airSeedList.RemoveAll();
}

Node* Nav::GetNextGroundSeedNode()
{
#ifdef FILEBUG
	FILEBUG_WRITE("GetNextGroundSeedNode: %i\n", groundSeedIndex);
#endif

	if(groundSeedIndex == -1)
	{
#ifdef FILEBUG
		FILEBUG_WRITE("Invalid Seed Index 1\n");
#endif
		return NULL;
	}

	if(!groundSeedList.IsValidIndex(groundSeedIndex))
	{
#ifdef FILEBUG
		FILEBUG_WRITE("Invalid Seed Index 2\n");
#endif
		return NULL;
	}

#ifdef FILEBUG
	FILEBUG_WRITE("GetNextGroundSeedNode: Continuing 1\n");
#endif

	GroundSeedSpot spot = groundSeedList.Element(groundSeedIndex);

#ifdef FILEBUG
	FILEBUG_WRITE("GetNextGroundSeedNode: Continuing 2\n");
#endif

	groundSeedIndex = groundSeedList.Next(groundSeedIndex);

#ifdef FILEBUG
	FILEBUG_WRITE("GetNextGroundSeedNode: Continuing 3\n");
#endif

	if(GetNode(spot.pos) == NULL)
	{
		if(generationMaxDistance > 0)
		{
			if(vecOrigin.DistTo(spot.pos) > generationMaxDistance)
			{
#ifdef FILEBUG
				FILEBUG_WRITE("GetNextGroundSeedNode: Skipping Seed\n");
#endif
				return GetNextGroundSeedNode();
			}
		}
#ifdef FILEBUG
		FILEBUG_WRITE("GetNextGroundSeedNode: Adding Node\n");
#endif
		Node *node = new Node(spot.pos, spot.normal, NULL);
		node->SetID(nodes.AddToTail(node));
		return node;
	}

#ifdef FILEBUG
	FILEBUG_WRITE("GetNextWalkableSeedNode: Next Seed\n");
#endif

	return GetNextGroundSeedNode();
}

Node *Nav::GetNextAirSeedNode()
{
	if(airSeedIndex == -1)
	{
		return NULL;
	}

	if(!airSeedList.IsValidIndex(airSeedIndex))
	{
		return NULL;
	}

	AirSeedSpot spot = airSeedList.Element(airSeedIndex);

	airSeedIndex = airSeedList.Next(airSeedIndex);

	if(GetNode(spot.pos) == NULL)
	{
		if(generationMaxDistance > 0)
		{
			if(vecOrigin.DistTo(spot.pos) > generationMaxDistance)
			{
				return GetNextAirSeedNode();
			}
		}
		Node *node = new Node(spot.pos, vector_origin, NULL);
		node->SetID(nodes.AddToTail(node));
		return node;
	}

	return GetNextAirSeedNode();
}

bool Nav::SampleStep()
{
	if(IsGenerated())
	{
		return true;
	}

	if(!generating)
	{
		return true;
	}

	// take a ground step
	while(generatingGround)
	{
		if(currentNode == NULL)
		{
			currentNode = GetNextGroundSeedNode();

			if(currentNode == NULL)
			{
				generatingGround = false;
				break;
			}
		}

		//
		// Take a step from this node
		//

#ifdef FILEBUG
		FILEBUG_WRITE("Stepping From Node\n");
#endif
		for(int dir = NORTH; dir < numDirections; dir++)
		{
#ifdef FILEBUG
			FILEBUG_WRITE("Checking Direction: %i\n", dir);
#endif
			if(!currentNode->HasVisited((NavDirType)dir))
			{
				// have not searched in this direction yet

#ifdef FILEBUG
				FILEBUG_WRITE("Unsearched Direction: %i\n", dir);
#endif

				// start at current node position
				Vector Pos = *currentNode->GetPosition();

#ifdef FILEBUG
				FILEBUG_WRITE("1 <%f, %f>\n", Pos.x, Pos.y);
#endif
				switch(dir)
				{
					case NORTH:	Pos.y += generationStepSize; break;
					case SOUTH:	Pos.y -= generationStepSize; break;
					case EAST:	Pos.x += generationStepSize; break;
					case WEST:	Pos.x -= generationStepSize; break;
					case NORTHEAST:	Pos.x += generationStepSize; Pos.y += generationStepSize; break;
					case NORTHWEST:	Pos.x -= generationStepSize; Pos.y += generationStepSize; break;
					case SOUTHEAST: Pos.x += generationStepSize; Pos.y -= generationStepSize; break;
					case SOUTHWEST:	Pos.x -= generationStepSize; Pos.y -= generationStepSize; break;
				}

#ifdef FILEBUG
				FILEBUG_WRITE("2 <%f, %f>\n", Pos.x, Pos.y);
#endif

				generationDir = (NavDirType)dir;

				// mark direction as visited
				currentNode->MarkAsVisited(generationDir);

				// test if we can move to new position
				Vector to;

				// modify position to account for change in ground level during step
				to.x = Pos.x;
				to.y = Pos.y;

				Vector toNormal;

				if(GetGroundHeight(Pos, &to.z, &toNormal) == false)
				{
#ifdef FILEBUG
					FILEBUG_WRITE("Ground Height Fail\n");
#endif
					return false;
				}

				Vector from = *currentNode->GetPosition();

				Vector fromOrigin = from + vecAddOffset;
				Vector toOrigin = to + vecAddOffset;

				trace_t result;

				//GMU->TraceLine(fromOrigin, toOrigin, mask, &result);
				UTIL_TraceLine_GMOD(fromOrigin, toOrigin, mask, &result);

				bool walkable;

				if(result.fraction == 1.0f && !result.startsolid)
				{
					// the trace didnt hit anything - clear

					float toGround = to.z;
					float fromGround = from.z;

					float epsilon = 0.1f;

					// check if ledge is too high to reach or will cause us to fall to our death
					// Using generationStepSize instead of JumpCrouchHeight so that stairs will work with different grid sizes
					if(toGround - fromGround > generationStepSize + epsilon || fromGround - toGround > DeathDrop)
					{
						walkable = false;
#ifdef FILEBUG
						FILEBUG_WRITE("Bad Ledge\n");
#endif
					}
					else
					{
						// check surface normals along this step to see if we would cross any impassable slopes
						Vector delta = to - from;
						const float inc = 2.0f;
						float along = inc;
						bool done = false;
						float ground;
						Vector normal;

						walkable = true;

						while(!done)
						{
							Vector p;

							// need to guarantee that we test the exact edges
							if(along >= generationStepSize)
							{
								p = to;
								done = true;
							}
							else
							{
								p = from + delta * (along / generationStepSize);
							}

							if(GetGroundHeight(p, &ground, &normal) == false)
							{
								walkable = false;
#ifdef FILEBUG
								FILEBUG_WRITE("Bad Node Path\n");
#endif
								break;
							}

							// check for maximum allowed slope
							if(normal.z < 0.65)
							{
								walkable = false;
#ifdef FILEBUG
								FILEBUG_WRITE("Slope\n");
#endif
								break;
							}

							along += inc;					
						}
					}
				}
				else // TraceLine hit something...
				{
					walkable = false;
#ifdef FILEBUG
					FILEBUG_WRITE("Hit Something\n");
#endif
				}

				if(walkable)
				{
#ifdef FILEBUG
					FILEBUG_WRITE("Walkable 1!\n");
#endif
					AddNode(to, toNormal, generationDir, currentNode);
#ifdef FILEBUG
					FILEBUG_WRITE("Walkable 2!\n");
#endif
				}

				return false;
			}
		}

		// all directions have been searched from this node - pop back to its parent and continue
		currentNode = currentNode->GetParent();
	}

	// Step through air
	while(true)
	{
		if(currentNode == NULL)
		{
			currentNode = GetNextAirSeedNode();
			//Msg("GetNextAirSeedNode\n");
			if(currentNode == NULL)
			{
				break;
			}
		}

		for(int dir = UP; dir < NUM_DIRECTIONS_MAX; dir++) // UP to the end!
		{
			if(!currentNode->HasVisited((NavDirType)dir))
			{
				// start at current node position
				Vector pos = *currentNode->GetPosition();

				switch(dir)
				{
					case UP:		pos.z += generationStepSize; break;
					case DOWN:		pos.z -= generationStepSize; break;
					case LEFT:		pos.x -= generationStepSize; break;
					case RIGHT:		pos.x += generationStepSize; break;
					case FORWARD:	pos.y += generationStepSize; break;
					case BACKWARD:	pos.y -= generationStepSize; break;
				}

				generationDir = (NavDirType)dir;
				currentNode->MarkAsVisited(generationDir);
				
				// Trace from the current node to the pos (Check if we hit anything)
				trace_t result;

				//GMU->TraceLine(*currentNode->GetPosition(), pos, mask, &result);
				UTIL_TraceLine_GMOD(*currentNode->GetPosition(), pos, mask, &result);

				if(!result.DidHit())
				{
					//Msg("added node\n");
					AddNode(pos, vector_origin, generationDir, currentNode);
				}

				//Msg("HasVisited: %d %d\n", currentNode->GetID(), dir);

				return false;
			}
		}

		// all directions have been searched from this node - pop back to its parent and continue
		currentNode = currentNode->GetParent();

		//Msg("back to parent\n");
	}

	generated = true;
	generating = false;

	return true;
}

bool Nav::Save(const char *Filename)
{
	if(generating)
	{
		return false;
	}

	char path[MAX_PATH];
	Q_snprintf(path, MAX_PATH, "garrysmod/%s", Filename);

	FILE *pFile = fopen(path, "w");
	if(pFile == NULL)
	{
		return false;
	}

	lock.Lock();

	Node *node, *connection;

	int iNodeConnections = 0;
	int iNodeTotal = nodes.Count();

	// Save current nav file version
	fprintf(pFile, "GM_NAVIGATION\t%d\n", NAV_VERSION);

	////////////////
	// Nodes
	fprintf(pFile, "%d\n", iNodeTotal);

	for(int i = 0; i < iNodeTotal; i++)
	{
		node = nodes[i];

		fprintf(pFile, "%f\t%f\t%f\t%f\t%f\t%f\n", node->vecPos.x, node->vecPos.y, node->vecPos.z, node->vecNormal.x, node->vecNormal.y, node->vecNormal.z);

		for(int Dir = NORTH; Dir < NUM_DIRECTIONS_MAX; Dir++)
		{
			if(node->GetConnectedNode((NavDirType)Dir) != NULL)
			{
				iNodeConnections++;
			}
		}
	}
	////////////////

	////////////////
	// Connections
	fprintf(pFile, "%d\n", iNodeConnections);

	for(int i = 0; i < iNodeTotal; i++)
	{
		node = nodes[i];
		for(int Dir = NORTH; Dir < NUM_DIRECTIONS_MAX; Dir++)
		{
			connection = node->GetConnectedNode((NavDirType)Dir);
			if(connection != NULL)
			{
				fprintf(pFile, "%d\t%d\t%d\n", Dir, node->GetID(), connection->GetID());
			}
		}
	}
	////////////////

	fclose(pFile);

	lock.Unlock();

	return true;
}

bool Nav::Load(const char *Filename)
{
	if(generating)
	{
		return false;
	}

	char path[MAX_PATH];
	Q_snprintf(path, MAX_PATH, "garrysmod/%s", Filename);

	FILE *pFile = fopen(path , "r");
	if(pFile == NULL)
	{
		return false;
	}

	lock.Lock();

	Node *node;

	nodeStart = NULL;
	nodeEnd = NULL;

	kd_clear(nodeTree);

	for(int i=0; i < nodes.Count(); i++)
	{
		Node *pNode = nodes[i];
		delete pNode;
		pNode = NULL;
	}

	nodes.Purge();

	int fileVersion;
	fscanf(pFile, "GM_NAVIGATION\t%d\n", &fileVersion);

	////////////////
	// Nodes

	int iNodeTotal;
	fscanf(pFile, "%d\n", &iNodeTotal);

	for(int i = 0; i < iNodeTotal; i++)
	{
		Vector pos, normal;

		fscanf(pFile, "%f\t%f\t%f\t%f\t%f\t%f\n", &pos.x, &pos.y, &pos.z, &normal.x, &normal.y, &normal.z);

		node = new Node(pos, normal, NULL);

		node->SetID(nodes.AddToTail(node));

		kd_insert3f(nodeTree, pos.x, pos.y, pos.z, node);
	}
	////////////////

	////////////////
	// Connections

	int iTotalConnections;
	fscanf(pFile, "%d\n", &iTotalConnections);

	int Dir, SrcID, DestID;
	for(int i = 0; i < iTotalConnections; i++)
	{
		fscanf(pFile, "%d\t%d\t%d\n", &Dir, &SrcID, &DestID);

		// Should never be a problem...
		if(Dir <= NUM_DIRECTIONS_MAX - 1 && DestID - 1 <= nodes.Count() && SrcID - 1 <= nodes.Count())
		{
			nodes[SrcID]->ConnectTo(nodes[DestID], (NavDirType)Dir);
		}
		else
		{
			//FILEBUG_WRITE("Invalid connection\n", SrcID);
		}
	}

	////////////////

	lock.Unlock();

	fclose(pFile);

	return true;
}

CUtlVector<Node*>& Nav::GetNodes()
{
	return nodes;
}

const CUtlVector<Node*>& Nav::GetOpenedNodes() const
{
	return opened;
}

Node *Nav::GetClosestNode(const Vector &Pos) const
{
	/*
	float fNodeDist;
	float fDistance = -1;
	
	Node *pNode = NULL;
	Node *pNodeClosest = NULL;

	for(int i = 0; i < nodes.Count(); i++)
	{
		pNode = nodes[i];
		fNodeDist = Pos.DistTo(pNode->vecPos);
		if(fDistance == -1 || fNodeDist < fDistance)
		{
			pNodeClosest = pNode;
			fDistance = fNodeDist;
		}
	}

	return pNodeClosest;
	*/

	Node *pNode = NULL;

	kdres *results = kd_nearest3f(nodeTree, Pos.x, Pos.y, Pos.z);
	if(results != NULL && !kd_res_end(results))
	{
		pNode = (Node*)kd_res_item_data(results);
	}

	if(results != NULL)
	{
		kd_res_free(results);
	}

	return pNode;
}

void Nav::Reset()
{
	Node *node;
	for(int i = 0; i < nodes.Count(); i++)
	{
		node = nodes[i];
		node->SetOpened(false);
		node->SetClosed(false);
	}
	opened.RemoveAll();
	closed.RemoveAll();
}

void Nav::AddOpenedNode(Node *node)
{
	node->SetOpened(true);
	opened.AddToTail(node);
}

void Nav::AddClosedNode(Node *node)
{
	node->SetClosed(true);
	bool Removed = opened.FindAndRemove(node);
	if(!Removed)
	{
		Msg("Failed to remove Node!?\n");
	}
}

float Nav::HeuristicDistance(const Vector *vecStartPos, const Vector *EndPos)
{
	if(heuristic == HEURISTIC_MANHATTAN)
	{
		return ManhattanDistance(vecStartPos, EndPos);
	}
	else if(heuristic == HEURISTIC_EUCLIDEAN)
	{
		return EuclideanDistance(vecStartPos, EndPos);
	}
	else if(heuristic == HEURISTIC_CUSTOM)
	{
		//gLua->PushReference(heuristicRef);
			//GMOD_PushVector(vecStartPos);
			//GMOD_PushVector(EndPos);
		//gLua->Call(2, 1);
		//return gLua->GetNumber(1);
	}
	return NULL;
}

float Nav::ManhattanDistance(const Vector *vecStartPos, const Vector *EndPos)
{
	return (abs(EndPos->x - vecStartPos->x) + abs(EndPos->y - vecStartPos->y) + abs(EndPos->z - vecStartPos->z));
}

// u clid e an
float Nav::EuclideanDistance(const Vector *vecStartPos, const Vector *EndPos)
{
	return sqrt(pow(EndPos->x - vecStartPos->x, 2) + pow(EndPos->y - vecStartPos->y, 2) + pow(EndPos->z - vecStartPos->z, 2));
}

Node *Nav::FindLowestF()
{
	float BestScoreF = NULL;
	Node *node, *winner = NULL;
	for(int i = 0; i < opened.Count(); i++)
	{
		node = opened[i];
		if(BestScoreF == NULL || node->GetScoreF() < BestScoreF)
		{
			winner = node;
			BestScoreF = node->GetScoreF();
		}
	}
	return winner;
}

#ifdef USE_BOOST_THREADS
void Nav::FindPathQueue(JobInfo_t *info)
{
	info->thread = boost::thread(&Nav::ExecuteFindPath, this, info, nodeStart, nodeEnd);  
}

#else
void Nav::FindPathQueue(JobInfo_t *info)
{
	info->job = threadPool->QueueCall(this, &Nav::ExecuteFindPath, info, nodeStart, nodeEnd);
}
#endif

void Nav::ExecuteFindPath(JobInfo_t *info, Node *start, Node *end)
{
#ifdef FILEBUG
	FILEBUG_WRITE("ExecuteFindPath\n");
#endif

	lock.Lock();
	Reset();

	info->foundPath = false;

	if(start == NULL || end == NULL || start == end)
	{
#ifdef FILEBUG
		FILEBUG_WRITE("Stopped\n");
#endif
		lock.Unlock();
		return;
	}

	AddOpenedNode(start);
	start->SetStatus(NULL, HeuristicDistance(start->GetPosition(), end->GetPosition()), 0);

	float currentScoreG, scoreG;
	Node *current = NULL, *connection = NULL;
	
#ifdef FILEBUG
	FILEBUG_WRITE("Added Node\n");
#endif

	while(!info->abort)
	{
		current = FindLowestF();

		if(current == NULL)
		{
			break;
		}

		if(current == end)
		{
			info->foundPath = true;
			break;
		}
		else
		{
			AddClosedNode(current);

			currentScoreG = current->GetScoreG();

			for(int Dir = NORTH; Dir < NUM_DIRECTIONS_MAX; Dir++) // numDirections
			{
				connection = current->GetConnectedNode((NavDirType)Dir);

				if(connection == NULL)
				{
					continue;
				}

				if(connection->IsClosed() || connection->IsDisabled())
				{
					continue;
				}
				
				scoreG = currentScoreG + EuclideanDistance(current->GetPosition(), connection->GetPosition()); // dist_between here

				if(!connection->IsOpened() || scoreG < connection->GetScoreG())
				{
					if(info->hull)
					{
						trace_t result;
						UTIL_TraceHull_GMOD(*current->GetPosition(), *connection->GetPosition(), info->mins, info->maxs, mask, &result);
						if(result.DidHit())
						{
							continue;
						}
					}

					AddOpenedNode(connection);
					connection->SetStatus(current, scoreG + HeuristicDistance(connection->GetPosition(), end->GetPosition()), scoreG);
				}
			}
		}
	}

	while(true)
	{
		if(current == NULL)
		{
			break;
		}
		info->path.AddToHead(current);
		current = current->GetAStarParent();
	}

#ifdef FILEBUG
	FILEBUG_WRITE("FindPath Done\n");
#endif

	lock.Unlock();
}

int Nav::GetMask()
{
	return mask;
}

void Nav::SetMask(int M)
{
	mask = M;
}

bool Nav::GetDiagonal()
{
	return bDiagonalMode;
}

void Nav::SetDiagonal(bool Diagonal)
{
	bDiagonalMode = Diagonal;
	if(bDiagonalMode)
	{
		numDirections = NUM_DIRECTIONS_DIAGONAL;
	}
	else
	{
		numDirections = NUM_DIRECTIONS;
	}
}

int Nav::GetNumDir()
{
	return numDirections;
}

int Nav::GetHeuristic()
{
	return heuristic;
}

Node *Nav::GetStart()
{
	return nodeStart;
}

Node *Nav::GetEnd()
{
	return nodeEnd;
}

void Nav::SetHeuristic(int H)
{
	heuristic = H;
}

void Nav::SetStart(Node *start)
{
	nodeStart = start;
}

void Nav::SetEnd(Node *end)
{
	nodeEnd = end;
}

kdtree* Nav::GetNodeTree()
{
	return nodeTree;
}

CThreadMutex& Nav::GetLock()
{
	return lock;
}
