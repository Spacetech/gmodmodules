-------------------
-- gmsv_timeout
-- Spacetech
-------------------

require("timeout")

print("Timeout Seconds", INET_TIMEOUT_SECONDS)

timer.Create("Timeout", 1, 0, function()
	for k,v in pairs(player.GetAll()) do
		print("Timeout", v, v:IsTimingOut(), v:GetTimeSinceLastReceived())
		if(v:IsTimingOut() and v:GetTimeSinceLastReceived() > 10) then
			v:Kick("Timed Out")
		end
	end
end)
