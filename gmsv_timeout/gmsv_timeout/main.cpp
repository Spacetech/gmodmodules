/*
    gmsv_timeout
    By Spacetech
*/ 

#include "gmluamodule.h"
#include <interface.h>
#include "eiface.h"
#include "inetchannelinfo.h"
#include "tier0/memdbgon.h"

GMOD_MODULE(Init, Shutdown);

ILuaInterface *gLua	= NULL;
IVEngineServer *engine = NULL;

//TODO: Reference it up

// Yay Jinto
// http://www.facepunch.com/showpost.php?p=11830936&postcount=11
int GetPlayerIndex(int arg)
{
	ILuaObject* EntityObj = gLua->GetObject(arg);

	// get the entity metatable
	ILuaObject* Entity = gLua->GetMetaTable("Entity", GLua::TYPE_ENTITY);

	// get EntIndex.
	ILuaObject* EntIndex = Entity->GetMember("EntIndex");

	// push
	EntIndex->Push();
	EntityObj->Push();

	// call
	gLua->Call(1, 1);

	// get return
	ILuaObject* ret = gLua->GetReturn(0);

	int returnEntIndex = ret->GetInt();

	ret->UnReference();
	EntIndex->UnReference();
	Entity->UnReference();
	EntityObj->UnReference();

	return returnEntIndex;
}

LUA_FUNCTION(IsTimingOut)
{
	gLua->CheckType(1, GLua::TYPE_ENTITY);

	int EntIndex = GetPlayerIndex(1);

	if(EntIndex > 0)
	{
		INetChannelInfo *pInfo = engine->GetPlayerNetInfo(EntIndex);
		if(pInfo != NULL)
		{
			Lua()->Push((bool)pInfo->IsTimingOut());
		}
		else
		{
			Lua()->PushNil();
		}
	}
	else
	{
		Lua()->PushNil();
	}

	return 1;
}

LUA_FUNCTION(GetTimeSinceLastReceived)
{
	gLua->CheckType(1, GLua::TYPE_ENTITY);

	int EntIndex = GetPlayerIndex(1);

	if(EntIndex > 0)
	{
		INetChannelInfo *pInfo = engine->GetPlayerNetInfo(EntIndex);
		if(pInfo != NULL)
		{
			Lua()->Push((float)pInfo->GetTimeSinceLastReceived());
		}
		else
		{
			Lua()->PushNil();
		}
	}
	else
	{
		Lua()->PushNil();
	}

	return 1;
}

int Init(lua_State* L)
{
	gLua = Lua();

	CreateInterfaceFn engineFactory	= Sys_GetFactory("engine.dll");

	engine = (IVEngineServer*)engineFactory(INTERFACEVERSION_VENGINESERVER, NULL);

	if(!engine)
	{
		gLua->Error("gmsv_timeout: Missing IVEngineServer interface.\n");
		return 0;
	}

	Msg("gmsv_timeout: Loaded\n");

	gLua->SetGlobal("INET_TIMEOUT_SECONDS", (float)180);

	ILuaObject* playerMeta = gLua->GetMetaTable("Player", GLua::TYPE_ENTITY);
	if(playerMeta)
	{
		playerMeta->SetMember("IsTimingOut", IsTimingOut);
		playerMeta->SetMember("GetTimeSinceLastReceived", GetTimeSinceLastReceived);
	}
    playerMeta->UnReference();

	return 0;
}

int Shutdown(lua_State* L)
{
	return 0;
}
