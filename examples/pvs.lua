-------------------
-- gm_pvs
-- Spacetech
-------------------

require("pvs")

-- Lets make sure all clients have time to actually recognise the player
hook.Add("PlayerInitialSpawn", "PVSPlayerInitialSpawn", function(ply)
	ply:SetColor(255, 255, 255, 0)
	timer.Simple(5, function(ply) -- 5 seconds should be fine
		if(IsValid(ply)) then
			-- print("SetElasticity")
			ply:SetColor(255, 255, 255, 255)
			ply:SetElasticity(PVS_CONNECTED) -- Once this is set SameInstance will be called for the player
		end
	end, ply)
end)

-- Force update pvs for a player
concommand.Add("pvs_update", function(ply)
	ply:UpdatePVS()
end)

concommand.Add("pvs_setupdate", function(ply, cmd, args)
	pvsSetUpdateTime(tonumber(args[1]))
end)

local Owner, Parent
hook.Add("SameInstance", "SameInstanceCheck", function(ply, ent)
	-- print(ply, ent)
	
	Owner = ent:GetOwner()
	if(IsValid(Owner)) then
		if(Owner == ply) then
			return true
		elseif(Owner:IsPlayer()) then
			return false
		end
	end
	
	Parent = ent:GetParent()
	if(IsValid(Parent)) then
		if(Parent == ply) then
			return true
		elseif(Parent:IsPlayer()) then
			return false
		end
	end
	
	return !ent:IsPlayer()
end)
