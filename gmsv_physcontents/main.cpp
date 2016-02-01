/*
    gmsv_physcontents
    By Spacetech
*/ 

#define WIN32_LEAN_AND_MEAN
#include <gmluamodule.h>

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

#include "baseanimating.h"
#include "baseflex.h"

#include "vphysics_interface.h"

#include "tier0/memdbgon.h"

GMOD_MODULE(Init, Shutdown);

IVEngineServer *engine = NULL;

LUA_FUNCTION(LUA_GetContents)
{
	Lua()->CheckType(1, GLua::TYPE_PHYSOBJ);

	IPhysicsObject *physObj = (IPhysicsObject *)Lua()->GetUserData(1);
	
	if(!physObj)
	{
		return 0;
	}

	Lua()->Push((float)physObj->GetContents());
	
	return 1;
}

LUA_FUNCTION(LUA_SetContents)
{
	Lua()->CheckType(1, GLua::TYPE_PHYSOBJ);
	Lua()->CheckType(2, GLua::TYPE_NUMBER);

	IPhysicsObject *physObj = (IPhysicsObject *)Lua()->GetUserData(1);
	
	if(!physObj)
	{
		return 0;
	}

	physObj->SetContents(Lua()->GetInteger(2));
	physObj->RecheckCollisionFilter();

	Lua()->Push(true);

	return 1;
}


int Init(lua_State* L)
{
	CreateInterfaceFn engineFactory	= Sys_GetFactory("engine.dll");

	engine = (IVEngineServer*)engineFactory(INTERFACEVERSION_VENGINESERVER, NULL);

	if(!engine)
	{
		Lua()->Error("gmsv_physcontents: Failed to load IVEngineServer\n");
	}

	Msg("gmsv_physcontents: Loaded\n");

	ILuaObject *physMeta = Lua()->GetMetaTable("PhysObj", GLua::TYPE_PHYSOBJ);
	if(physMeta)
	{
		physMeta->SetMember("GetContents", LUA_GetContents);
		physMeta->SetMember("SetContents", LUA_SetContents);
	}
    physMeta->UnReference();

	return 0;
}

int Shutdown(lua_State* L)
{
	return 0;
}
