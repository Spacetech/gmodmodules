-------------------
-- gm_astar
-- Spacetech
-------------------

-- You shouldn't really be using this module anymore
-- gm_navigation is cooler

-- require("profiler")
require("astar")

local TotalNodes = 999
local NodeDistance = 5
local AutoLinkDistance = 10

local function TestCreateNodeList(Size)
	local Nodes = {}
	for i=1,Size do
		table.insert(Nodes, Vector() * (NodeDistance * i))
	end
	return Nodes
end

function PrintAStarInfo(astar)
	local LinkTotal = 0
	for i=1,astar:GetNodeTotal() do
		LinkTotal = LinkTotal + table.Count(astar:GetNode(i):GetLinks())
	end
	print("Node Total", astar:GetNodeTotal(), "Link Total", LinkTotal)
end

function TestAStar(astar)
	-- profiler.Start()
	
	print("NearestNode", astar:NearestNode(Vector(0, 0, 0)))
	
	print("GetNodes", astar:GetNodes(), table.Count(astar:GetNodes()))
	
	print("GetNodeTotal", astar:GetNodeTotal())
	print("Last Node", astar:GetNode(astar:GetNodeTotal()):GetPos())
	
	print("GetNodeTotal 1", astar:GetNodeTotal())
	astar:SetStart(astar:GetNode(1))
	print("GetNodeTotal 2", astar:GetNodeTotal())
	
	astar:SetEnd(astar:GetNode(astar:GetNodeTotal()))
	
	-- HEURISTIC_MANHATTAN
	-- HEURISTIC_EUCLIDEAN
	-- HEURISTIC_DISTANCE (Pretty much same as Euclidean)
	-- HEURISTIC_CUSTOM (Not Implemented)
	
	-- TODO: Fix this?
	-- astar:LinkNodes(function(Node1, Node2)
		-- return Node1:Distance(Node2) <= 100
	-- end)
	
	for i=HEURISTIC_MANHATTAN,HEURISTIC_DISTANCE do
		print("FindPath", i)
		astar:SetHeuristic(i)
		local FoundPath, Path = astar:FindPath()
		if(FoundPath) then
			print("Found Path", Path)
			if(Path) then
				print("Path Count", table.Count(Path))
				print("First Pos", Path[1])
				-- PrintTable(Path)
			else
				--print("Path is too large :(")
			end
		else
			print("Failed to Find Path")
		end
	end
	
	-- profiler.Dump()
end

concommand.Add("astar", function()
	print("\nAStar")
	
	local AStar = CreateAStar()
	
	local Nodes = TestCreateNodeList(TotalNodes)
	
	for k,v in pairs(Nodes) do
		AStar:AddNode(v)
	end
	
	AStar:AutoLinkNodes(AutoLinkDistance)
	
	//profiler.Start()
	print("Save", AStar:Save("test.astar"))
	//profiler.Dump()
	
	TestAStar(AStar)
	PrintAStarInfo(AStar)
	
	print("\nOtherAStar")
	
	local OtherAStar = CreateAStar()
	
	//profiler.Start()
	OtherAStar:Load("test.astar")
	//profiler.Dump()
	
	TestAStar(OtherAStar)
	PrintAStarInfo(OtherAStar)
end)
