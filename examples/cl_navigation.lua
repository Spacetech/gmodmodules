---------------
-- gm_navigation
-- Spacetech
---------------

require("navigation")

-- If running clientside hold down alt to see it.

local GridSize = 64 -- Space between the nodes

local Nav = nav.Create(GridSize)

local Diagonal = true

-- Diagonal Linking is disabled by default
Nav:SetDiagonal(Diagonal) -- Enable / Disable Diagonal linking (Will DOUBLE the time it takes for the generation)

-- Nav:SetGridSize(256) -- You can change the grid size this way.

-- The module will ignore COLLISION_GROUP_PLAYER during generation

local PrintPath = {}
local Start, End
local NormalUp = Vector(0, 0, 1)
local Mask = MASK_PLAYERSOLID
local HitWorld, Pos, ONormal

Nav:SetMask(Mask) -- Set the mask for the nav traces - Default is MASK_PLAYERSOLID_BRUSHONLY

local mins = Vector(-16, -16, 20) -- min z should be > 0 so that it doesn't get stuck in ground / screw up on angles
local maxs = Vector(16, 16, 72)

local function TraceDown(Pos)
	local trace = {}
	trace.start = Pos + Vector(0, 0, 1)
	trace.endpos = trace.start - Vector(0, 0, 9000)
	trace.ignore = LocalPlayer()
	trace.mask = Mask
	local tr = util.TraceLine(trace)
	return tr.HitWorld, tr.HitPos, tr.HitNormal
end

local function ComputePath()
	ErrorNoHalt("ComputePath\n")
	local StartTime = os.time()
	Nav:FindPath(function(Nav, FoundPath, Path)
		if(FoundPath) then
			ErrorNoHalt("Found Path in "..string.ToMinutesSeconds(os.time() - StartTime).." Path Size: "..table.Count(Path).."\n")
			-- PrintTable(Path)
		else
			ErrorNoHalt("Failed to Find Path "..table.Count(Path).."\n")
			PrintTable(Path)
			if(table.Count(Path) > 0) then
				print(Path[1]:GetPosition(), Path[table.Count(Path)]:GetPosition())
			end
		end
		PrintPath = Path
		Start = Nav:GetStart()
		End = Nav:GetEnd()
	end)
end

local function ComputePathHull()
	ErrorNoHalt("ComputePathHull\n")
	local StartTime = os.time()
	Nav:FindPathHull(mins, maxs, function(Nav, FoundPath, Path)
		if(FoundPath) then
			ErrorNoHalt("Found Path in "..string.ToMinutesSeconds(os.time() - StartTime).." Path Size: "..table.Count(Path).."\n")
			-- PrintTable(Path)
		else
			ErrorNoHalt("Failed to Find Path "..table.Count(Path).."\n")
			PrintTable(Path)
			if(table.Count(Path) > 0) then
				print(Path[1]:GetPosition(), Path[table.Count(Path)]:GetPosition())
			end
		end
		PrintPath = Path
		Start = Nav:GetStart()
		End = Nav:GetEnd()
	end)
end

local function OnGenerated(Loaded)
	print("\n")
	-- I'm using node:GetPosition() because it makes it easier to see what node is which
	
	local LinkTotal = 0
	
	for k,v in pairs(Nav:GetNodes()) do
		LinkTotal = LinkTotal + table.Count(v:GetConnections())
	end
	
	print("Node Total", Nav:GetNodeTotal(), "Link Total", LinkTotal)
	
	local Node = Nav:GetNodes()[math.random(1, Nav:GetNodeTotal())]
	
	print("GetNode", Nav:GetNode(Node:GetPosition()):GetPosition(), Node:GetPosition())
	
	print("GetNodeByID", Nav:GetNodeByID(1), Nav:GetNodeByID(Nav:GetNodeByID(1):GetID()), Nav:GetNodeByID(1) == Nav:GetNodeByID(Nav:GetNodeByID(1):GetID())) 
	
	print("GetClosestNode", Nav:GetClosestNode(Vector(0, 0, 0)))
	
	print("Node Info", Nav:GetNodes(), Nav:GetNodeTotal(), table.Count(Nav:GetNodes()))
	
	print("First Node", Nav:GetNodes()[1]:GetPosition(), Nav:GetNodeByID(1):GetPosition())
	print("Last Node", Nav:GetNodes()[Nav:GetNodeTotal()]:GetPosition(), Nav:GetNodeByID(Nav:GetNodeTotal()):GetPosition())
	
	print("GetNodeTotal 1", Nav:GetNodeTotal())
	Nav:SetStart(Nav:GetNodeByID(1))
	print("GetNodeTotal 2", Nav:GetNodeTotal())
	
	Nav:SetEnd(Nav:GetNodeByID(Nav:GetNodeTotal()))
	
	print("Start", Nav:GetStart():GetPosition(), "End", Nav:GetEnd():GetPosition())
	
	print("Diagonal 1", Nav:GetDiagonal())
	
	Nav:SetDiagonal(!Diagonal)
	
	print("Diagonal 2", Nav:GetDiagonal())
	
	Nav:SetDiagonal(Diagonal)

	print("GridSize 1", Nav:GetGridSize())
	
	Nav:SetGridSize(256)
	
	print("GridSize 2", Nav:GetGridSize())
	
	Nav:SetGridSize(GridSize)
	
	-- HEURISTIC_MANHATTAN
	-- HEURISTIC_EUCLIDEAN
	Nav:SetHeuristic(nav.HEURISTIC_MANHATTAN)
	
	-- ComputePath()
	
	print("Save", Nav:Save("data/test.nav"))
	
	if(!Loaded) then
		print("Load", Nav:Load("data/test.nav"), "\n")
		OnGenerated(true)
	end
end

-- Make sure you are nocliped and above the ground
concommand.Add("snav_generate_ground", function(ply)
	HitWorld, Pos, Normal = TraceDown((IsValid(ply) and (ply:GetPos() + Vector(0, 0, 5))) or Vector(0, 0, 1))
	
	if(HitWorld) then
		ErrorNoHalt("Creating Nav\n")
		
		if(IsValid(ply)) then
			-- Remove this line if you don't want a max distance
			Nav:SetupMaxDistance(ply:GetPos(), 1024) -- All nodes must stay within 256 vector distance from the players position
		end
		
		Nav:ClearGroundSeeds()
		Nav:ClearAirSeeds()
		
		-- Once 1 seed runs out, it will go onto the next seed
		Nav:AddGroundSeed(Pos, Normal)
		
		HitWorld, Pos, Normal = TraceDown(Vector(0, 0, 0))
		Nav:AddGroundSeed(Pos, Normal)
		
		-- The module will account for node overlapping
		Nav:AddGroundSeed(Pos, NormalUp)
		Nav:AddGroundSeed(Pos, NormalUp)
		
		local StartTime = os.time()
		
		Nav:Generate(function(Nav)
			ErrorNoHalt("Generated "..Nav:GetNodeTotal().." nodes in "..string.ToMinutesSeconds(os.time() - StartTime).."\n")
		end, function(Nav, GeneratedNodes)
			ErrorNoHalt("Generated "..GeneratedNodes.." nodes so far\n")
		end)
		
		--ErrorNoHalt("Generated in "..string.ToMinutesSeconds(Nav:FullGeneration()).."\n")
	end
end)

concommand.Add("snav_generate_air_only", function(ply)
	HitWorld, Pos, Normal = TraceDown((IsValid(ply) and (ply:GetPos() + Vector(0, 0, 5))) or Vector(0, 0, 1))
	
	if(HitWorld) then
		ErrorNoHalt("Creating Nav\n")
		
		if(IsValid(ply)) then
			-- Remove this line if you don't want a max distance
			Nav:SetupMaxDistance(ply:GetPos(), 1024) -- All nodes must stay within 256 vector distance from the players position
		end
		
		Nav:ClearGroundSeeds()
		Nav:ClearAirSeeds()
		
		-- generate air nodes
		Nav:AddAirSeed(Pos + Vector(0, 0, GridSize + 1))
		
		local StartTime = os.time()
		
		Nav:Generate(function(Nav)
			ErrorNoHalt("Generated "..Nav:GetNodeTotal().." nodes in "..string.ToMinutesSeconds(os.time() - StartTime).."\n")
		end, function(Nav, GeneratedNodes)
			ErrorNoHalt("Generated "..GeneratedNodes.." nodes so far\n")
		end)
		
		--ErrorNoHalt("Generated in "..string.ToMinutesSeconds(Nav:FullGeneration()).."\n")
	end
end)

concommand.Add("snav_generate_ground_air", function(ply)
	HitWorld, Pos, Normal = TraceDown((IsValid(ply) and (ply:GetPos() + Vector(0, 0, 5))) or Vector(0, 0, 1))
	
	if(HitWorld) then
		ErrorNoHalt("Creating Nav\n")
		
		if(IsValid(ply)) then
			-- Remove this line if you don't want a max distance
			Nav:SetupMaxDistance(ply:GetPos(), 1024) -- All nodes must stay within 256 vector distance from the players position
		end
		
		Nav:ClearGroundSeeds()
		Nav:ClearAirSeeds()
		
		Nav:AddGroundSeed(Pos, Normal)
		
		-- generate air nodes
		Nav:AddAirSeed(Pos + Vector(0, 0, GridSize + 1))
		
		local StartTime = os.time()
		
		Nav:Generate(function(Nav)
			ErrorNoHalt("Generated "..Nav:GetNodeTotal().." nodes in "..string.ToMinutesSeconds(os.time() - StartTime).."\n")
		end, function(Nav, GeneratedNodes)
			ErrorNoHalt("Generated "..GeneratedNodes.." nodes so far\n")
		end)
		
		--ErrorNoHalt("Generated in "..string.ToMinutesSeconds(Nav:FullGeneration()).."\n")
	end
end)

concommand.Add("snav_setstart", function(ply)
	print(Nav:GetClosestNode(ply:GetPos()):GetPos())
	Nav:SetStart(Nav:GetClosestNode(ply:GetPos()))
	Start = Nav:GetStart()
end)

concommand.Add("snav_setend", function(ply)
	Nav:SetEnd(Nav:GetClosestNode(ply:GetPos()))
	End = Nav:GetEnd()
	-- ComputePath()
	ComputePathHull()
end)

concommand.Add("snav_disable_node", function(ply)
	Nav:GetClosestNode(ply:GetPos()):SetDisabled(true)
end)

concommand.Add("snav_remove_node", function(ply)
	local ClosestNode = Nav:GetClosestNode(ply:GetPos())
	Nav:RemoveNode(ClosestNode)
end)

concommand.Add("snav_debug", function(ply)
	print(Nav:GetNodes(), Nav:GetStart(), Nav:GetEnd(), table.Count(Nav:GetNodes()))
	
	-- local ClosestNode = Nav:GetClosestNode(ply:GetPos())
	-- ClosestNode:RemoveConnection(NORTH)
	
	-- local NodeA = Nav:GetNodeByID(1)
	
	-- local DIR = NORTH
	-- for k,NodeB in ipairs(Nav:GetNodes()) do
		-- if(NodeA != NodeB) then
			-- NodeA:ConnectTo(NodeB, DIR)
			-- DIR = DIR + 1
		-- end
		-- if(DIR == NUM_DIRECTIONS_DIAGONAL) then
			-- break
		-- end
	-- end
	
	-- PrintTable(NodeA:GetConnections())
end)

concommand.Add("snav_debug_2", function()
	for k,v in pairs(Nav:GetNodes()) do
		print(v, v:GetPosition(), table.Count(v:GetConnections()))
	end
end)

concommand.Add("snav_manual", function(ply)
	local HitWorld, Pos, Normal = TraceDown((IsValid(ply) and ply:GetPos()) or Vector(0, 0, 1))
	
	if(!HitWorld) then
		return
	end
	
	local Node1 = Nav:CreateNode(Pos, Normal)
	
	if(!Node1) then
		print("I went out of the max distance. I'm so sorry!!")
		return
	end
	
	print("GetPosition", Node1:GetPosition())
	print("GetNormal", Node1:GetNormal())
	
	Node1:SetPosition(Vector(333, 444, 555))
	Node1:SetNormal(Normal * -1)
	
	print("GetPosition", Node1:GetPosition())
	print("GetNormal", Node1:GetNormal())
	
	Node1:SetPosition(Pos)
	Node1:SetNormal(Normal)
	
	Node1:ConnectTo(table.Random(Nav:GetNodes()), NORTH)
end)

concommand.Add("snav_save", function()
	print("Save", Nav:Save("data/nav/"..game.GetMap()..".nav"))
end)

concommand.Add("snav_load", function()
	print("Load", Nav:Load("data/nav/"..game.GetMap()..".nav"))
end)

concommand.Add("snav_debug_2", function()
	for k,v in pairs(Nav:GetNodes()) do
		print(v, v:GetPosition(), table.Count(v:GetConnections()))
	end
end)

if(SERVER) then
	return
end

local Alpha = 200
local ColNORMAL = Color(255, 255, 255, Alpha) -- White
local ColNORTH = Color(255, 255, 0, Alpha) -- Pink?
local ColSOUTH = Color(255, 0, 0, Alpha) -- RED
local ColEAST = Color(0, 255, 0, Alpha) -- GREEN
local ColWEST = Color(0, 0, 255, Alpha) -- BLUE
local ColOTHER = Color(0, 255, 255, Alpha) -- Black
local ColDisabled = Color(50, 50, 50, Alpha)

local PathOffset = Vector(0, 0, 10)
local ColPath = Color(255, 0, 0, 255)

local function DrawNodeLines(Table, PlyPos)
	for k,v in pairs(Table) do
		local pos = v:GetPosition()
		if(PlyPos:Distance(pos) <= 512) then
			local connections = v:GetConnections()
			for k2,v2 in pairs(connections) do
				local Col = ColOTHER
				if(k2 == nav.NORTH) then
					Col = ColNORTH
				elseif(k2 == nav.SOUTH) then
					Col = ColSOUTH
				elseif(k2 == nav.EAST) then
					Col = ColEAST
				elseif(k2 == nav.WEST) then
					Col = ColWEST
				end
				
				render.DrawBeam(pos, pos + (v2:GetPosition() - pos) * 0.3, 4, 0.25, 0.75, Col)
			end
			
			local ColNorm = ColNORMAL
			if(v:IsDisabled()) then
				ColNorm = ColDisabled
			end
			
			local normal = v:GetNormal()
			if(normal != 0 and normal.y != 0 and normal.z != 0) then
				render.DrawBeam(pos, pos + (normal * 13), 4, 0.25, 0.75, ColNorm)
			end
		end
	end
end

local function DrawNodePath(Table)
	for k,v in pairs(Table) do
		if(Table[k + 1]) then
			render.DrawBeam(v:GetPosition() + PathOffset, Table[k + 1]:GetPosition() + PathOffset, 4, 0.25, 0.75, ColPath)
		end
	end
end

local nodes = {}
local nodeTotal = 0

local Mat = Material("effects/laser_tracer")
hook.Add("RenderScreenspaceEffects", "NavRenderScreenspaceEffects", function()
	if(!Nav) then
		return
	end
	if(Nav:GetNodeTotal() != nodeTotal) then
		nodes = Nav:GetNodes()
		nodeTotal = Nav:GetNodeTotal()
	end
	local alt = input.IsKeyDown(KEY_LALT)
	local shift = input.IsKeyDown(KEY_LSHIFT)
	if(alt || shift) then
		render.SetMaterial(Mat)
		cam.Start3D(EyePos(), EyeAngles())
			if(alt) then
				DrawNodeLines(nodes, LocalPlayer():GetPos())
			end
			if(shift) then
				DrawNodePath(PrintPath)
				if(Start) then
					local normal = Start:GetNormal()
					if(normal.x != 0 or normal.y != 0 or normal.z != 0) then
						render.DrawBeam(Start:GetPosition(), Start:GetPosition() + (Start:GetNormal() * 64), 4, 0.25, 0.75, ColPath)
					else
						render.DrawBeam(Start:GetPosition(), Start:GetPosition() + (vector_up * 32), 4, 0.25, 0.75, ColPath)
					end
				end
				if(End) then
					local normal = End:GetNormal()
					if(normal.x != 0 or normal.y != 0 or normal.z != 0) then
						render.DrawBeam(End:GetPosition(), End:GetPosition() + (End:GetNormal() * 64), 4, 0.25, 0.75, ColPath)
					else
						render.DrawBeam(End:GetPosition(), End:GetPosition() + (vector_up * 32), 4, 0.25, 0.75, ColPath)
					end
				end
			end
		cam.End3D()
	end
end)
