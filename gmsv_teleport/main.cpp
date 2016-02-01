/*
    gmsv_teleport
    By Spacetech
*/ 

#define WIN32_LEAN_AND_MEAN
#define CBASEFLEX_TELEPORT 107
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

#include "vfnhook.h"

#include "tier0/memdbgon.h"

#ifdef GetObject
#undef GetObject
#endif

GMOD_MODULE(Init, Shutdown);

int entIndex = 0;
CBaseFlex *pFlex = NULL;
ILuaInterface *gLua	= NULL;
IVEngineServer *engine = NULL;

bool hooked = false;
int refEntity, refHookCall, refEntIndex;

bool ShouldTeleport(int playerIndex)
{
	gLua->PushReference(refEntity);
		gLua->Push((float)playerIndex);
	gLua->Call(1, 1);
	ILuaObject *pPlayer = gLua->GetReturn(0);

	gLua->PushReference(refHookCall);
		gLua->Push("ShouldTeleport");
		gLua->PushNil();
		gLua->Push(pPlayer);
	gLua->Call(3, 1);
	ILuaObject *ret = gLua->GetReturn(0);

	pPlayer->UnReference();

	bool result = ret->GetBool();
	
	ret->UnReference();

	return result;
}

// 	virtual void Teleport( const Vector *newPosition, const QAngle *newAngles, const Vector *newVelocity );
DEFVFUNC_(origTeleport, void, (CBaseFlex *pFlex, const Vector *newPosition, const QAngle *newAngles, const Vector *newVelocity));
void VFUNC newTeleport(CBaseFlex *pFlex, const Vector *newPosition, const QAngle *newAngles, const Vector *newVelocity)
{
	if(ShouldTeleport(pFlex->entindex()))
	{
		return origTeleport(pFlex, newPosition, newAngles, newVelocity);
	}
}

LUA_FUNCTION(SetupTeleportHook)
{
	gLua->CheckType(1, GLua::TYPE_ENTITY);
	gLua->CheckType(2, GLua::TYPE_BOOL);

	bool hook = gLua->GetBool(2);

	if((hook && hooked) || (!hook && !hooked))
	{
		gLua->Push(true);
		return 1;
	}

	gLua->PushReference(refEntIndex);
		gLua->Push(gLua->GetObject(1));
	gLua->Call(1, 1);

	ILuaObject *ret = gLua->GetReturn(0);

	int index = ret->GetInt();

	ret->UnReference();

	if(index > 0)
	{
		if(!hook)
		{
			if(entIndex == index)
			{
				hooked = false;
				entIndex = 0;
				UNHOOKVFUNC(pFlex, CBASEFLEX_TELEPORT, origTeleport);
				pFlex = NULL;

				gLua->Push(true);
				return 1;
			}
		}
		else
		{
			CBaseFlex *pFlexCur = dynamic_cast<CBaseFlex*>(CBaseEntity::Instance(index));
			if(pFlexCur != NULL)
			{
				edict_t *pEdict = pFlexCur->edict();
				if(pEdict && !pEdict->IsFree() && hook && pFlex == NULL)
				{
					hooked = true;
					entIndex = index;
					pFlex = pFlexCur;

					HOOKVFUNC(pFlex, CBASEFLEX_TELEPORT, origTeleport, newTeleport);

					gLua->Push(true);
					return 1;
				}
			}
		}
	}
	
	gLua->Push(false);

	return 1;
}

int Init(lua_State* L)
{
	gLua = Lua();

	CreateInterfaceFn engineFactory	= Sys_GetFactory("engine.dll");

	engine = (IVEngineServer*)engineFactory(INTERFACEVERSION_VENGINESERVER, NULL);

	if(!engine)
	{
		gLua->Error("gmsv_teleport: Failed to load IVEngineServer\n");
	}

	Msg("gmsv_teleport: Loaded\n");

	ILuaObject *oEntity = gLua->GetGlobal("Entity");
	oEntity->Push();
	refEntity = gLua->GetReference(-1, true);
	oEntity->UnReference();

	ILuaObject *ohook = gLua->GetGlobal("hook");
		ILuaObject *oCall = ohook->GetMember("Call");
		oCall->Push();
		refHookCall = gLua->GetReference(-1, true);
		oCall->UnReference();
	ohook->UnReference();

	ILuaObject *oEntityMeta = gLua->GetMetaTable("Entity", GLua::TYPE_ENTITY);
		ILuaObject *oEntIndex = oEntityMeta->GetMember("EntIndex");
		oEntIndex->Push();
		refEntIndex = gLua->GetReference(-1, true);
		oEntIndex->UnReference();
	oEntityMeta->UnReference();

	ILuaObject *entityMeta = gLua->GetMetaTable("Entity", GLua::TYPE_ENTITY);
	if(entityMeta)
	{
		entityMeta->SetMember("SetupTeleportHook", SetupTeleportHook);
	}
    entityMeta->UnReference();

	return 0;
}

int Shutdown(lua_State* L)
{
	if(refEntity)
	{
		gLua->FreeReference(refEntity);
	}

	if(refHookCall)
	{
		gLua->FreeReference(refHookCall);
	}

	if(refEntIndex)
	{
		gLua->FreeReference(refEntIndex);
	}

	return 0;
}
