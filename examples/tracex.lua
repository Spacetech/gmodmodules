-------------------
-- gm_tracex
-- Spacetech
-------------------

-- Traces with callbacks

-- require("profiler")
require("tracex")

concommand.Add("tracex_line", function(ply)
	local Trace = {}
	Trace.start = ply:GetShootPos()
	Trace.endpos = Trace.start + (ply:GetAimVector() * 2048)
	Trace.mask = MASK_PLAYERSOLID
	
	local Count = 0
	
	local Callback = function(HitEntity)
		print("ahhh", HitEntity)
		if(HitEntity != ply) then
			print(HitEntity)
			Count = Count + 1
			return Count >= 3
		end
		return false
	end
	
	-- profiler.Start()
	local tr = tracex.TraceLine(Trace, Callback, true) -- Last arg is ShouldIgnoreWorld
	-- profiler.Dump()
	
	PrintTable(tr)
	
	if(IsValid(tr.Entity)) then
		print(tr.Entity:GetModel())
	end
	
	print("ORIGINAL:")
	-- Implementing a filter is just useless, use the callback on tracex instead
	Trace.filter = ply
	
	-- profiler.Start()
	local tr = util.TraceLine(Trace)
	-- profiler.Dump()
	
	PrintTable(tr)
end)

concommand.Add("tracex_hull", function(ply)
	local Trace = {}
	Trace.start = ply:GetShootPos()
	Trace.endpos = Trace.start + (ply:GetAimVector() * 2048)
	Trace.mask = MASK_PLAYERSOLID
	Trace.mins = Vector(-16, -16, 10)
	Trace.maxs = Vector(16, 16, 72)
	
	-- Let's go through 1 entity
	local IgnoreOne = false
	
	local Callback = function(HitEntity)
		if(HitEntity == ply) then -- aka Trace.filter
			return false
		end
		
		if(HitEntity != ply) then 
			if(IgnoreOne) then
				print("Returning", HitEntity)
				return true
			end
			IgnoreOne = true
			print("Hit", HitEntity)
		end
	end
	
	//profiler.Start()
	local tr = tracex.TraceHull(Trace, Callback, false)
	//profiler.Dump()
	
	//PrintTable(tr)
	
	print(tr.Entity)
	
	print("ORIGINAL:")
	
	-- Implementing a filter is just useless, use the callback on tracex instead
	Trace.filter = ply
	
	//profiler.Start()
	local tr = util.TraceHull(Trace)
	//profiler.Dump()
	print(tr.Entity)
	
	//PrintTable(tr)
end)
