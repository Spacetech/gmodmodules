/*
    gm_tracex
    By Spacetech
*/ 

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
#include "entitylist.h"
#include "engine/ienginetrace.h"

#include "tier0/memdbgon.h"

#include "gmodinterface/gmluamodule.h"

GMOD_MODULE(Init, Shutdown);

int EntityRef, VectorRef, GetWorldEntityRef;

ILuaInterface *gLua = NULL;
IVEngineServer *engine = NULL;
IEngineTrace *enginetrace = NULL;

////////////////////////////////////////////////////////////
// Jinto (Edits by Spacetech)

ILuaObject* NewVectorObject(Vector& vec)
{
	// Get a reference to the function to survive past 510 calls! (Azu)
	gLua->PushReference(VectorRef);
		gLua->Push(vec.x);
		gLua->Push(vec.y);
		gLua->Push(vec.z);
	gLua->Call(3, 1);
	return gLua->GetReturn(0);
}

void GMOD_PushVector(Vector& vec)
{
	ILuaObject* obj = NewVectorObject(vec);
		gLua->Push(obj);
	obj->UnReference();
}

Vector& GMOD_GetVector(void *userdata)
{
	return *reinterpret_cast<Vector*>(userdata);
}

Vector& GMOD_GetVector(int stackPos)
{
	return GMOD_GetVector(gLua->GetUserData(stackPos));
}

ILuaObject* GMOD_PushEntity(int Index)
{
	if(Index == 0 && gLua->IsServer())
	{
		gLua->PushReference(GetWorldEntityRef);
		gLua->Call(0, 1);
		return gLua->GetReturn(0);
	}
	gLua->PushReference(EntityRef);
		gLua->Push((float)Index);
	gLua->Call(1, 1);
	return gLua->GetReturn(0);
}

////////////////////////////////////////////////////////////

class GMOD_TraceFilter : public ITraceFilter
{
public:
	GMOD_TraceFilter(int ShouldHitEntityRef, bool ShouldIgnoreWorld) :
		m_bFinished(false),
		m_bShouldIgnoreWorld(ShouldIgnoreWorld),
		m_iShouldHitEntityRef(ShouldHitEntityRef)
	{}

	bool ShouldHitEntity(IHandleEntity *pHandleEntity, int contentsMask)
	{
		if(m_bFinished)
		{
			return false;
		}

		CBaseHandle eHandle = pHandleEntity->GetRefEHandle();
		edict_t *pEdict = engine->PEntityOfEntIndex(eHandle.GetEntryIndex());
		if(pEdict && !pEdict->IsFree())
		{
			CBaseEntity *pEntity = CBaseEntity::Instance(pEdict);
			if(pEntity)
			{
				gLua->PushReference(m_iShouldHitEntityRef);
						ILuaObject* oEntity = GMOD_PushEntity(pEntity->entindex());
					gLua->Push(oEntity);
				gLua->Call(1, 1);

				ILuaObject* oRet = gLua->GetReturn(0);

				oEntity->UnReference();

				bool bRet = false;

				if(!oRet->isNil())
				{
					bRet = oRet->GetBool();
				}

				oRet->UnReference();

				if(bRet)
				{
					m_bFinished = true;
				}

				return bRet;
			}
		}

		return true;
	}

	TraceType_t	GetTraceType() const
	{
		if(m_bShouldIgnoreWorld)
		{
			return TRACE_ENTITIES_ONLY;
		}
		return TRACE_EVERYTHING;
	}

private:
	bool m_bFinished;
	bool m_bShouldIgnoreWorld;
	int m_iShouldHitEntityRef;
};

////////////////////////////////////////////////////////////

void GMOD_TraceLine(const Vector& StartPos, const Vector& EndPos, unsigned int Mask, ITraceFilter *filter, trace_t *ptr)
{
	Ray_t ray;
	//ray.Init(EndPos, StartPos);
	ray.Init(StartPos, EndPos);
	enginetrace->TraceRay(ray, Mask, filter, ptr);
}

void GMOD_TraceHull(const Vector& StartPos, const Vector& EndPos, const Vector& Mins, const Vector& Maxs, unsigned int Mask, ITraceFilter *filter, trace_t *ptr)
{
	Ray_t ray;
	//ray.Init(EndPos, StartPos, Mins, Maxs);
	ray.Init(StartPos, EndPos, Mins, Maxs);
	enginetrace->TraceRay(ray, Mask, filter, ptr);
}

////////////////////////////////////////////////////////////

ILuaObject *FillTraceTable(trace_t *ptr)
{
	ILuaObject *tr = gLua->GetNewTable();

	int EntIndex = -1;

	CBaseEntity *Ent = ptr->m_pEnt;
	if(Ent != NULL)
	{
		EntIndex = Ent->entindex();
	}
	else
	{
		tr->SetMember("HitNothing", true);
	}

	tr->SetMember("FractionLeftSolid", ptr->fractionleftsolid);
	
	tr->SetMember("HitWorld", EntIndex == 0 ? false : true);
	tr->SetMember("HitNonWorld", EntIndex == 0 ? true : false); // EntIndex > 0 ? false : true

	tr->SetMember("Fraction", ptr->fraction);
	tr->SetMember("HitTexture", ptr->surface.name);

	if(EntIndex >= 0)
	{
		ILuaObject* oEntity = GMOD_PushEntity(EntIndex);
		tr->SetMember("Entity", oEntity);
	}

	tr->SetMember("StartSolid", ptr->startsolid);

	tr->SetMember("HitNoDraw", ptr->surface.name == "TOOLS/TOOLSNODRAW");

	tr->SetMember("HitBoxBone", (float)ptr->physicsbone);

	tr->SetMember("HitSky", ptr->surface.name == "TOOLS/TOOLSSKYBOX");

	ILuaObject* oEndPos = NewVectorObject(ptr->endpos);
	tr->SetMember("HitPos", oEndPos);
	oEndPos->UnReference();

	tr->SetMember("StartSolid", ptr->startsolid);

	// I don't know :/
	// Who even uses this?
	// tr->SetMember("Normal", ptr.plane.normal);

	tr->SetMember("HitBox", (float)ptr->hitbox);

	ILuaObject* oHitNormal = NewVectorObject(ptr->plane.normal);
	tr->SetMember("HitNormal", oHitNormal);
	oHitNormal->UnReference();

	tr->SetMember("Hit", ptr->DidHit());

	tr->SetMember("HitGroup", (float)ptr->hitgroup);

	ILuaObject* oStartPos = NewVectorObject(ptr->startpos);
	tr->SetMember("StartPos", oStartPos);
	oStartPos->UnReference();

	tr->SetMember("PhysicsBone", (float)ptr->physicsbone);

	return tr;
}

////////////////////////////////////////////////////////////

LUA_FUNCTION(TXTraceLine)
{
	gLua->CheckType(1, GLua::TYPE_TABLE);
	gLua->CheckType(2, GLua::TYPE_FUNCTION);
	gLua->CheckType(3, GLua::TYPE_BOOL);

	////////////////////////////////////////////////////////////

	ILuaObject *Table = gLua->GetObject(1);
	int ShouldHitEntityRef = gLua->GetReference(2);
	bool ShouldIgnoreWorld = gLua->GetBool(3);

	////////////////////////////////////////////////////////////

	Vector StartPos = GMOD_GetVector(Table->GetMemberUserData("start"));
	Vector EndPos = GMOD_GetVector(Table->GetMemberUserData("endpos"));

	if(StartPos == NULL || EndPos == NULL)
	{
		gLua->Error("tracex.TraceLine: Missing StartPos / EndPos\n");
	}

	int Mask = Table->GetMemberInt("mask", MASK_ALL);

	Table->UnReference();

	////////////////////////////////////////////////////////////
	
	trace_t ptr;
	GMOD_TraceFilter filter(ShouldHitEntityRef, ShouldIgnoreWorld);
	GMOD_TraceLine(StartPos, EndPos, Mask, &filter, &ptr);

	gLua->FreeReference(ShouldHitEntityRef);

	FillTraceTable(&ptr)->Push();

	return 1;
}

LUA_FUNCTION(TXTraceHull)
{
	gLua->CheckType(1, GLua::TYPE_TABLE);
	gLua->CheckType(2, GLua::TYPE_FUNCTION);
	gLua->CheckType(3, GLua::TYPE_BOOL);

	////////////////////////////////////////////////////////////

	ILuaObject *Table = gLua->GetObject(1);
	int ShouldHitEntityRef = gLua->GetReference(2);
	bool ShouldIgnoreWorld = gLua->GetBool(3);

	////////////////////////////////////////////////////////////

	Vector StartPos = GMOD_GetVector(Table->GetMemberUserData("start"));
	Vector EndPos = GMOD_GetVector(Table->GetMemberUserData("endpos"));
	Vector Mins = GMOD_GetVector(Table->GetMemberUserData("mins"));
	Vector Maxs = GMOD_GetVector(Table->GetMemberUserData("maxs"));

	if(StartPos == NULL || EndPos == NULL)
	{
		gLua->Error("tracex.TraceLine: Missing start / endpos\n");
	}

	if(Mins == NULL || Maxs == NULL)
	{
		gLua->Error("tracex.TraceLine: Missing mins / maxs\n");
	}

	int Mask = Table->GetMemberInt("mask", MASK_ALL);

	Table->UnReference();

	////////////////////////////////////////////////////////////
	
	trace_t ptr;
	GMOD_TraceFilter filter(ShouldHitEntityRef, ShouldIgnoreWorld);
	GMOD_TraceHull(StartPos, EndPos, Mins, Maxs, Mask, &filter, &ptr);

	gLua->FreeReference(ShouldHitEntityRef);

	FillTraceTable(&ptr)->Push();

	return 1;
}

int Init(lua_State* L)
{
	gLua = Lua();

	CreateInterfaceFn interfaceFactory = Sys_GetFactory("engine.dll");
	CreateInterfaceFn gameServerFactory = Sys_GetFactory("server.dll");

	engine = (IVEngineServer*)interfaceFactory(INTERFACEVERSION_VENGINESERVER, NULL);
	if(!engine)
	{
		gLua->Error("gm_tracex: Missing IVEngineServer interface.\n");
	}

	if(gLua->IsServer())
	{
		enginetrace = (IEngineTrace*)interfaceFactory(INTERFACEVERSION_ENGINETRACE_SERVER, NULL);
	}
	else
	{
		enginetrace = (IEngineTrace*)interfaceFactory(INTERFACEVERSION_ENGINETRACE_CLIENT, NULL);
	}
	
	if(!enginetrace)
	{
		gLua->Error("gm_tracex: Missing IEngineTrace interface.\n");
	}

	ILuaObject *oEntity = gLua->GetGlobal("Entity");
	oEntity->Push();
	EntityRef = gLua->GetReference(-1, true);
	oEntity->UnReference();

	ILuaObject *oVector = gLua->GetGlobal("Vector");
	oVector->Push();
	VectorRef = gLua->GetReference(-1, true);
	oVector->UnReference();

	if(gLua->IsServer())
	{
		ILuaObject *oGetWorldEntity = gLua->GetGlobal("GetWorldEntity");
		oGetWorldEntity->Push();
		GetWorldEntityRef = gLua->GetReference(-1, true);
		oGetWorldEntity->UnReference();
	}
	else
	{
		GetWorldEntityRef = NULL;
	}

	ILuaObject* txTable = gLua->GetNewTable();
		txTable->SetMember("TraceLine", TXTraceLine);
		txTable->SetMember("TraceHull", TXTraceHull);
	gLua->SetGlobal("tracex", txTable);
	txTable->UnReference();

	Msg("gm_tracex: Programmed by Spacetech | Entity Ref: %i | Vector Ref: %i\n", EntityRef, VectorRef);

	return 0;
}

int Shutdown(lua_State* L)
{
	if(EntityRef)
	{
		gLua->FreeReference(EntityRef);
	}

	if(VectorRef)
	{
		gLua->FreeReference(VectorRef);	
	}
	
	if(GetWorldEntityRef)
	{
		gLua->FreeReference(GetWorldEntityRef);
	}

	return 0;
}
