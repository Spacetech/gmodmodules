/*
    gm_stuckpos
    By Spacetech
*/ 

#include <windows.h>
#include <gmodinterface/gmluamodule.h>

#include <interface.h>
#include "eiface.h"
#include "ehandle.h"
#include "isaverestore.h"
#include "touchlink.h"
#include "groundlink.h"
#include "variant_t.h"
#include "predictableid.h"
#include "shareddefs.h"
#include "util.h"
#include "predictable_entity.h"
#include "takedamageinfo.h"
#include "baseentity_shared.h"
#include "baseentity.h"
#include "imovehelper.h"

#include "tier0/memdbgon.h"

GMOD_MODULE(Init, Shutdown);

ILuaInterface *gLua = NULL;
IVEngineServer *engine = NULL;
IEngineTrace *enginetrace = NULL;

class GMOD_TraceHitMask : public CTraceFilter
{
public:
	GMOD_TraceHitMask(unsigned int mask): m_iMask(mask)
	{
	}
	virtual bool ShouldHitEntity(IHandleEntity *pServerEntity, int contentsMask)
	{ 
		if(contentsMask != m_iMask)
		{
			return true;
		}
		return false;
	}
private:
	unsigned int m_iMask;
};

Vector& GMOD_GetVector(int stackPos)
{
	return *reinterpret_cast<Vector*>(gLua->GetUserData(stackPos));
}

CBaseHandle GMOD_TestPlayerPosition(const Vector& pos, Vector const& mins, Vector const& maxs, unsigned int mask)
{
	trace_t pm;

	Ray_t ray;
	ray.Init(pos, pos, mins, maxs);

	GMOD_TraceHitMask traceFilter(mask);

	enginetrace->TraceRay(ray, mask, &traceFilter, &pm);

	if((pm.contents & mask) && pm.m_pEnt)
	{
		return pm.m_pEnt->GetRefEHandle();
	}
	else
	{	
		return INVALID_EHANDLE_INDEX;
	}
}

LUA_FUNCTION(IsStuckPosition)
{
	gLua->CheckType(1, GLua::TYPE_VECTOR);
	gLua->CheckType(2, GLua::TYPE_VECTOR);
	gLua->CheckType(3, GLua::TYPE_VECTOR);
	gLua->CheckType(4, GLua::TYPE_NUMBER);

	EntityHandle_t hitent = GMOD_TestPlayerPosition(GMOD_GetVector(1), GMOD_GetVector(2), GMOD_GetVector(3), gLua->GetNumber(4));
	if(hitent == INVALID_ENTITY_HANDLE)
	{
		gLua->Push(false);
	}
	else
	{
		gLua->Push(true);
	}

	return 1;
}

int Init(lua_State* L)
{
	gLua = Lua();

	CreateInterfaceFn interfaceFactory = Sys_GetFactory("engine.dll");

	engine = (IVEngineServer*)interfaceFactory(INTERFACEVERSION_VENGINESERVER, NULL);
	if(!engine)
	{
		gLua->Error("gm_stuckpos: Missing IVEngineServer interface.\n");
	}

	enginetrace = (IEngineTrace*)interfaceFactory(INTERFACEVERSION_ENGINETRACE_SERVER, NULL);
	if(!enginetrace)
	{
		gLua->Error("gm_stuckpos: Missing IEngineTrace interface.\n");
	}

	gLua->SetGlobal("IsStuckPosition", IsStuckPosition);

	Msg("gm_stuckpos: Programmed by Spacetech\n");

	return 0;
}

int Shutdown(lua_State* L)
{
	return 0;
}
