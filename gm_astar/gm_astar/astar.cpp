/*
    gm_astar
    By Spacetech
*/

#include "astar.h"

AStar::AStar()
{
	FoundPath = false;
	Heuristic = HEURISTIC_MANHATTAN;
}

AStar::~AStar()
{
	Msg("Deconstructing AStar\n");

	/*
	// Purge seems to still deconstruct elements (But not delete them)
	Path.Purge();
	Opened.Purge();
	Closed.Purge();
	Nodes.PurgeAndDeleteElements();
	*/
	
	Nodes.PurgeAndDeleteElements();
	Path.PurgeAndDeleteElements(); // I think it might crash here. Might try to delete null astarnode
	Opened.PurgeAndDeleteElements();
	Closed.PurgeAndDeleteElements();
}

void AStar::Reset()
{
	AStarNode *node;
	for(int i = 0; i < Nodes.Count(); i++)
	{
		node = Nodes[i];
		node->SetClosed(false);
		node->SetOpened(false);
	}
	Path.RemoveAll();
	Opened.RemoveAll();
	Closed.RemoveAll();
}

float AStar::HeuristicDistance(Vector StartPos, Vector EndPos)
{
	// Seems like HEURISTIC_EUCLIDEAN == HEURISTIC_DISTANCE
	if(Heuristic == HEURISTIC_EUCLIDEAN)
	{
		return EuclideanDistance(StartPos, EndPos);
	}
	else if(Heuristic == HEURISTIC_DISTANCE)
	{
		return (StartPos - EndPos).Length();
	}
	else if(Heuristic == HEURISTIC_CUSTOM)
	{
		//gLua->PushReference(HeuristicRef);
			//GMOD_PushVector(StartPos);
			//GMOD_PushVector(EndPos);
		//gLua->Call(2, 1);
		//return gLua->GetNumber(1);
	}
	return ManhattanDistance(StartPos, EndPos);
}

// http://en.wikipedia.org/wiki/Manhattan_distance
float AStar::ManhattanDistance(Vector StartPos, Vector EndPos)
{
	return (abs(EndPos.x - StartPos.x) + abs(EndPos.y - StartPos.y) + abs(EndPos.z - StartPos.z));
}

// u clid ean
// http://en.wikipedia.org/wiki/Euclidean_distance
float AStar::EuclideanDistance(Vector StartPos, Vector EndPos)
{
	return sqrt(pow(EndPos.x - StartPos.x, 2) + pow(EndPos.y - StartPos.y, 2) + pow(EndPos.z - StartPos.z, 2));
}

AStarNode *AStar::FindLowestF()
{
	float BestScoreF = NULL;
	AStarNode *node, *winner = NULL;
	for(int i = 0; i < Opened.Count(); i++)
	{
		node = Opened[i];
		if(BestScoreF == NULL || node->GetScoreF() < BestScoreF)
		{
			winner = node;
			BestScoreF = node->GetScoreF();
		}
	}
	return winner;
}

NodeList_t& AStar::FindPath()
{
	Reset();

	FoundPath = false;

	//Msg("Reset Variables: %i, %i, %i\n", Path.Count(), Opened.Count(), Closed.Count());

	if(GetStart() == GetEnd())
	{
		//Msg("Start Is End\n");
		return Path;
	}

	// 1) Add the starting node to the open list.
	AddOpenedNode(Start);
	Start->SetStatus(NULL, 0, 0, 0);

	float CurrentScoreG;
	float ScoreF, ScoreG, ScoreH;
	NodeList_t Links;
	AStarNode *Current = NULL, *LastNode = NULL, *Link = NULL;

	//Msg("Added Node\n");

	// 2) Repeat the following:
	while(true)
	{
		// a) Look for the lowest F cost square on the open list. We refer to this as the current square.
		Current = FindLowestF();

		if(Current != NULL)
		{
			LastNode = Current;

			if(Current->GetPos() == End->GetPos())
			{
				FoundPath = true;
				break;
			}
			else
			{
				CurrentScoreG = Current->GetScoreG();

				// b) Switch it to the closed list.
				AddClosedNode(Current);

				// c) For each of the nodes linked to this node...
				Links = Current->GetLinks();

				ScoreH = HeuristicDistance(Current->GetPos(), End->GetPos());

				//Msg("Found Lowest F: %i | %f, %f, %f\n", Links.Count(), Current->GetPos().x, Current->GetPos().y, Current->GetPos().z);

				for(int i = 0; i < Links.Count(); i++)
				{
					Link = Links[i];

					if(Link != NULL)
					{

						// If it is not walkable or if it is on the closed list, ignore it.
						if(!Link->IsClosed())
						{

							// If it isn’t on the open list, add it to the open list. Make the current node the parent of this node. Record the F, G, and H costs of the node. 
							if(!Link->IsOpened())
							{
								//Msg("SetStatus\n");
								AddOpenedNode(Link);
								ScoreG = CurrentScoreG + HeuristicDistance(Current->GetPos(), Link->GetPos());
								ScoreF = ScoreG + ScoreH;
								Link->SetStatus(Current, ScoreF, ScoreG, ScoreH);
							}

							//if(Link->Opened()) Always true?
							//{
								if(CurrentScoreG > ScoreG)
								{
									Link->SetParent(Current);
									//Msg("Set Parent\n");
								}
								else
								{
									//Msg("%f | %f\n", CurrentScoreG, ScoreG);
								}
							//}
						}
					}
				}
			}
		}
		else
		{
			//Msg("Failed to find Lowest F\n");
			break;
		}
	}

	return CalcPath(LastNode);
}

NodeList_t& AStar::CalcPath(AStarNode *Current)
{
	while(true)
	{
		if(Current == NULL)
		{
			break;
		}
		Path.AddToHead(Current);
		Current = Current->GetParent();
		//Msg("Adding Node to Path\n");
	}
	return Path;
}

bool AStar::Save(IFileSystem *filesystem, const char *Filename)
{
	CUtlBuffer buf;

	AStarNode *Node;
	NodeList_t Links;

	int TotalNumLinks = 0;
	int NodeTotal = Nodes.Count();

	//////////////////////////////////////////////
	// Nodes
	buf.PutInt(NodeTotal);

	for(int i = 0; i < NodeTotal; i++)
	{
		Node = Nodes[i];

		buf.PutFloat(Node->GetPos().x);
		buf.PutFloat(Node->GetPos().y);
		buf.PutFloat(Node->GetPos().z);

		TotalNumLinks += Node->GetLinks().Count();
	}
	//////////////////////////////////////////////

	//////////////////////////////////////////////
	// Links
	buf.PutInt(TotalNumLinks);

	for(int i = 0; i < NodeTotal; i++)
	{
		Node = Nodes[i];
		Links = Node->GetLinks();
		for(int li = 0; li < Links.Count(); li++)
		{
			buf.PutInt(Node->GetID());
			buf.PutInt(Links[li]->GetID());
		}
	}
	//////////////////////////////////////////////

	//////////////////////////////////////////////
	// Write File

	FileHandle_t fh = filesystem->Open(Filename, "wb");
	if(!fh)
	{
		return false;
	}

	filesystem->Write(buf.Base(), buf.TellPut(), fh);
	filesystem->Close(fh);
	//////////////////////////////////////////////

	return true;
}

bool AStar::Load(IFileSystem *filesystem, const char *Filename)
{
	CUtlBuffer buf;

	if(!filesystem->ReadFile(Filename, "MOD", buf))
	{
		return false;
	}

	buf.SeekGet(CUtlBuffer::SEEK_HEAD, 0);

	int SrcID, DestID;

	//////////////////////////////////////////////
	// Nodes
	int NodeTotal = buf.GetInt();

	for(int i = 0; i < NodeTotal; i++)
	{
		Vector origin;
		origin.x = buf.GetFloat();
		origin.y = buf.GetFloat();
		origin.z = buf.GetFloat();
		AddNode(new AStarNode(origin));
	}
	//////////////////////////////////////////////

	//////////////////////////////////////////////
	// Links
	int TotalNumLinks = buf.GetInt();

	for(int i = 0; i < TotalNumLinks; i++)
	{
		SrcID = buf.GetInt();
		DestID = buf.GetInt();
		Nodes[SrcID]->AddLink(Nodes[DestID]);
	}

	//////////////////////////////////////////////

	return true;
}

bool AStar::Link(AStarNode *node1, AStarNode *node2)
{
	if(!node1->IsLink(node2))
	{
		node1->AddLink(node2);
		node2->AddLink(node1);
		return true;
	}
	return false;
}

void AStar::AddNode(AStarNode *node)
{
	node->SetID(Nodes.AddToTail(node));
}

void AStar::AddOpenedNode(AStarNode *node)
{
	node->SetOpened(true);
	Opened.AddToTail(node);
}

void AStar::AddClosedNode(AStarNode *node)
{
	node->SetClosed(true);
	bool Removed = Opened.FindAndRemove(node);
	if(!Removed)
	{
		Msg("Failed to remove Node!?\n");
	}
}

bool AStar::HasFoundPath()
{
	return FoundPath;
}

int AStar::GetHeuristic()
{
	return Heuristic;
}

AStarNode *AStar::GetStart()
{
	return Start;
}

AStarNode *AStar::GetEnd()
{
	return End;
}

void AStar::SetHeuristic(int H)
{
	Heuristic = H;
}

void AStar::SetStart(AStarNode *start)
{
	Start = start;
}

void AStar::SetEnd(AStarNode *end)
{
	End = end;
}

AStarNode *AStar::NearestNode(Vector NearPos)
{
	float DistanceCalc, Distance = NULL;
	AStarNode *node, *winner = NULL;
	for(int i = 0; i < Nodes.Count(); i++)
	{
		node = Nodes[i];
		DistanceCalc = (node->GetPos() - NearPos).Length();
		if(Distance == NULL || DistanceCalc < Distance)
		{
			winner = node;
			Distance = DistanceCalc;
		}
	}
	return winner;
}

void AStar::AutoLinkNodes(float Distance)
{
	AStarNode *node1, *node2;
	for(int i = 0; i < Nodes.Count(); i++)
	{
		node1 = Nodes[i];
		for(int i2 = 0; i2 < Nodes.Count(); i2++)
		{
			node2 = Nodes[i2];
			if((node1->GetPos() - node2->GetPos()).Length() <= Distance)
			{
				Link(node1, node2);
			}
		}
	}
}

NodeList_t& AStar::GetNodes()
{
	return Nodes;
}
