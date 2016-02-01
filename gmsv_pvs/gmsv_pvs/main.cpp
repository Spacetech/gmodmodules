/*
	gmsv_pvs
	By Spacetech
	Original by Darkspider / Train
*/

// Function Offset
#define GMMODULE 1
#define ISERVERGAMEENTS_CHECKTRANSMIT_OFFSET 6

#include <iostream>
#include <map>

#include <windows.h>
#include <GarrysMod\Lua\Interface.h>

// Probably don't even need half of these for this...
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

#include "engine/IEngineSound.h"

#include "vfnhook.h"

#include "mrecpfilter.h"

#include "steam/steamclientpublic.h"

#include "tier0/memdbgon.h"

using namespace GarrysMod::Lua;

float UpdateTime = 1.0;
float PVS_CONNECTED = 26.0;

edict_t *pBaseEdict = NULL;
int entityRef = -1;
int hookCallRef = -1;
int entIndexRef = -1;

IVEngineServer *engine = NULL;
IEngineSound *sound = NULL;
IServerGameEnts *gameents = NULL;
ILuaBase *gLua;

struct TransmitInfo_t
{
	float NextUpdate;
	int nNewEdicts;
	unsigned short *pNewEdictIndices;
};

static std::map<int, TransmitInfo_t> ClientTransmitInfo;

static const char *sAlwaysInstance[] =
{
	"bodyque",

	"team_manager",
	"scene_manager",
	"player_manager",

	"viewmodel",
	"predicted_viewmodel",

	"prop_door_rotating",

	"soundent",
	"water_lod_control",

	"worldspawn",

	"", // END Marker

	/*
	"infodecal",
	"info_projecteddecal",
	"info_node",
	"info_target",
	"info_node_hint",
	"info_player_deathmatch",
	"info_player_combine",
	"info_player_rebel",
	"info_map_parameters",
	"info_ladder",

	"keyframe_rope",
	"move_rope",

	"shadow_control",
	"sky_camera",

	"trigger_soundscape",
	*/
};

////////////////////////////////////////////////////////////////////////////////
// I'll give you this one rival!
FORCEINLINE bool NamesMatch(const char *pszQuery, string_t nameToMatch)
{
	if (nameToMatch == NULL_STRING)
		return (*pszQuery == 0 || *pszQuery == '*');

	const char *pszNameToMatch = STRING(nameToMatch);

	// If the pointers are identical, we're identical
	if (pszNameToMatch == pszQuery)
		return true;

	while (*pszNameToMatch && *pszQuery)
	{
		char cName = *pszNameToMatch;
		char cQuery = *pszQuery;
		if (cName != cQuery && tolower(cName) != tolower(cQuery)) // people almost always use lowercase, so assume that first
			break;
		++pszNameToMatch;
		++pszQuery;
	}

	if (*pszQuery == 0 && *pszNameToMatch == 0)
		return true;

	// @TODO (toml 03-18-03): Perhaps support real wildcards. Right now, only thing supported is trailing *
	if (*pszQuery == '*')
		return true;

	return false;
}

bool FindInList(const char **pStrings, const char *pToFind)
{
	int i = 0;
	while (pStrings[i][0] != 0)
	{
		if (Q_stricmp(pStrings[i], pToFind) == 0)
		{
			return true;
		}
		i++;
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////

bool LUACheckTransmit(int playerIndex, int entIndex)
{
	gLua->ReferencePush(entityRef);
		gLua->PushNumber(playerIndex);
	gLua->Call(1, 1);

	gLua->ReferencePush(entityRef);
		gLua->PushNumber(entIndex);
	gLua->Call(1, 1);

	gLua->ReferencePush(hookCallRef);
		gLua->PushString("SameInstance");
		gLua->PushNil();
		gLua->Push(-5);
		gLua->Push(-5);
	gLua->Call(4, 1);

	bool result = gLua->GetBool(-1);

	gLua->Pop(3);

	return result;
}

////////////////////////////////////////////////////////////////////////////////

int PlayerUpdatePVS(lua_State* state)
{
	LUA->CheckType(1, Type::ENTITY);

	LUA->ReferencePush(entIndexRef);
	LUA->Push(1);
	LUA->Call(1, 1);

	LUA->CheckType(-1, Type::NUMBER);

	int EntIndex = (int)LUA->GetNumber(-1);

	if (EntIndex > 0)
	{
		ClientTransmitInfo[EntIndex].NextUpdate = engine->Time() - 1;
	}

	return 0;
}

int SetUpdateTime(lua_State* state)
{
	LUA->CheckType(1, Type::NUMBER);

	UpdateTime = LUA->GetNumber(1);

	return 0;
}

////////////////////////////////////////////////////////////////////////////////

void UpdatePlayerEdictIndices(int clientIndex, unsigned short *pEdictIndices, int nEdicts)
{
	ClientTransmitInfo[clientIndex].nNewEdicts = nEdicts;
	ClientTransmitInfo[clientIndex].pNewEdictIndices = pEdictIndices;
}

// virtual void CheckTransmit( CCheckTransmitInfo *pInfo, const unsigned short *pEdictIndices, int nEdicts ) = 0;
DEFVFUNC_(origCheckTransmit, void, (IServerGameEnts *ge, CCheckTransmitInfo *pInfo, const unsigned short *pEdictIndices, int nEdicts));
void VFUNC newCheckTransmit(IServerGameEnts *ge, CCheckTransmitInfo *pInfo, const unsigned short *pEdictIndices, int nEdicts)
{
	if (!pBaseEdict || pBaseEdict->IsFree())
	{
		return origCheckTransmit(ge, pInfo, pEdictIndices, nEdicts);
	}

	CBaseEntity *pRecipientEntity = CBaseEntity::Instance(pInfo->m_pClientEnt);
	if (!pRecipientEntity || pRecipientEntity->GetElasticity() != PVS_CONNECTED)
	{
		return origCheckTransmit(ge, pInfo, pEdictIndices, nEdicts);
	}

	int nNewEdicts = 0;
	int clientIndex = engine->IndexOfEdict(pInfo->m_pClientEnt);

	if (ClientTransmitInfo[clientIndex].NextUpdate != NULL)
	{
		if (ClientTransmitInfo[clientIndex].NextUpdate > engine->Time())
		{
			unsigned short *pNewEdictIndices = new unsigned short[ClientTransmitInfo[clientIndex].nNewEdicts];

			for (int i = 0; i < ClientTransmitInfo[clientIndex].nNewEdicts; i++)
			{
				int iEdict = ClientTransmitInfo[clientIndex].pNewEdictIndices[i];
				edict_t *pEdict = &pBaseEdict[iEdict];
				if (pEdict && !pEdict->IsFree() && pEdict->GetUnknown() != NULL)
				{
					CBaseEntity *pEntity = pEdict->GetUnknown()->GetBaseEntity();
					if (pEntity)
					{
						pNewEdictIndices[nNewEdicts] = iEdict;
						nNewEdicts = nNewEdicts + 1;
					}
				}
			}

			UpdatePlayerEdictIndices(clientIndex, pNewEdictIndices, nNewEdicts);

			return origCheckTransmit(ge, pInfo, pNewEdictIndices, nNewEdicts);
		}
	}

	ClientTransmitInfo[clientIndex].NextUpdate = engine->Time() + UpdateTime;

	unsigned short *pNewEdictIndices = new unsigned short[nEdicts];

	for (int i = 0; i < nEdicts; i++)
	{
		int iEdict = pEdictIndices[i];
		edict_t *pEdict = &pBaseEdict[iEdict];

		if (pEdict && !pEdict->IsFree())
		{
			int nFlags = pEdict->m_fStateFlags & (FL_EDICT_DONTSEND | FL_EDICT_ALWAYS | FL_EDICT_PVSCHECK | FL_EDICT_FULLCHECK);

			if (nFlags & FL_EDICT_DONTSEND)
			{
				continue;
			}

			CBaseEntity *pEntity = pEdict->GetUnknown()->GetBaseEntity();
			if (pEntity)
			{
				string_t szClassName = pEntity->m_iClassname;

				int entIndex = engine->IndexOfEdict(pEdict);

				bool AlwaysInstance = (entIndex == 0
					|| clientIndex == entIndex
					|| FindInList(sAlwaysInstance, szClassName.ToCStr())
					|| NamesMatch("point_*", szClassName)
					|| NamesMatch("func_*", szClassName)
					|| NamesMatch("env_*", szClassName)
					|| NamesMatch("spotlight_*", szClassName));
				
				if (AlwaysInstance || (FStrEq(szClassName.ToCStr(), "player") && pEntity->GetElasticity() != PVS_CONNECTED) || LUACheckTransmit(clientIndex, entIndex))
				{
					//Msg("Manually Set %i:%i | Classname: %s\n", nNewEdicts, iEdict, ClassName);
					pNewEdictIndices[nNewEdicts] = iEdict;
					nNewEdicts = nNewEdicts + 1;
				}
			}
		}
	}

	UpdatePlayerEdictIndices(clientIndex, pNewEdictIndices, nNewEdicts);

	return origCheckTransmit(ge, pInfo, pNewEdictIndices, nNewEdicts);
}

////////////////////////////////////////

/*
	virtual void EmitSound( IRecipientFilter& filter, int iEntIndex, int iChannel, const char *pSample,
		float flVolume, float flAttenuation, int iFlags = 0, int iPitch = PITCH_NORM,
		const Vector *pOrigin = NULL, const Vector *pDirection = NULL, CUtlVector< Vector >* pUtlVecOrigins = NULL, bool bUpdatePositions = true, float soundtime = 0.0f, int speakerentity = -1 ) = 0;
*/
/*
	virtual void EmitSound( IRecipientFilter& filter, int iEntIndex, int iChannel, const char *pSample,
		float flVolume, float flAttenuation, int iFlags = 0, int iPitch = PITCH_NORM, int iSpecialDSP = 0,
		const Vector *pOrigin = NULL, const Vector *pDirection = NULL, CUtlVector< Vector >* pUtlVecOrigins = NULL, bool bUpdatePositions = true, float soundtime = 0.0f, int speakerentity = -1 ) = 0;
*/
DEFVFUNC_(origEmitSound1, void, (IEngineSound *es, IRecipientFilter& filter, int iEntIndex, int iChannel, const char *pSample,
	float flVolume, float flAttenuation, int iFlags, int iPitch, int iSpecialDSP,
	const Vector *pOrigin, const Vector *pDirection, CUtlVector< Vector >* pUtlVecOrigins, bool bUpdatePositions, float soundtime, int speakerentity));

void VFUNC newEmitSound1(IEngineSound *es, IRecipientFilter& filter, int iEntIndex, int iChannel, const char *pSample,
	float flVolume, float flAttenuation, int iFlags = 0, int iPitch = PITCH_NORM, int iSpecialDSP = 0,
	const Vector *pOrigin = NULL, const Vector *pDirection = NULL, CUtlVector< Vector >* pUtlVecOrigins = NULL, bool bUpdatePositions = true, float soundtime = 0.0f, int speakerentity = -1)
{
	int count = filter.GetRecipientCount();

	MRecipientFilter MFilter(engine);

	for (int i = 0; i < count; i++)
	{
		int index = filter.GetRecipientIndex(i);
		if (LUACheckTransmit(iEntIndex, index))
		{
			MFilter.AddPlayer(index);
		}
	}

	return origEmitSound1(es, MFilter, iEntIndex, iChannel, pSample, flVolume, flAttenuation, iFlags, iPitch, iSpecialDSP, pOrigin, pDirection, pUtlVecOrigins, bUpdatePositions, soundtime, speakerentity);
}

/*
	virtual void EmitSound( IRecipientFilter& filter, int iEntIndex, int iChannel, const char *pSample,
		float flVolume, soundlevel_t iSoundlevel, int iFlags = 0, int iPitch = PITCH_NORM,
		const Vector *pOrigin = NULL, const Vector *pDirection = NULL, CUtlVector< Vector >* pUtlVecOrigins = NULL, bool bUpdatePositions = true, float soundtime = 0.0f, int speakerentity = -1 ) = 0;

DEFVFUNC_(origEmitSound2, void, (IEngineSound *es, IRecipientFilter& filter, int iEntIndex, int iChannel, const char *pSample,
		float flVolume, soundlevel_t iSoundlevel, int iFlags, int iPitch,
		const Vector *pOrigin, const Vector *pDirection, CUtlVector< Vector >* pUtlVecOrigins, bool bUpdatePositions, float soundtime, int speakerentity));

void VFUNC newEmitSound2(IEngineSound *es, IRecipientFilter& filter, int iEntIndex, int iChannel, const char *pSample,
		float flVolume, soundlevel_t iSoundlevel, int iFlags = 0, int iPitch = PITCH_NORM,
		const Vector *pOrigin = NULL, const Vector *pDirection = NULL, CUtlVector< Vector >* pUtlVecOrigins = NULL, bool bUpdatePositions = true, float soundtime = 0.0f, int speakerentity = -1)
{
	int Count = filter.GetRecipientCount();

	MRecipientFilter MFilter(engine);

	for(int i=0; i < Count; i++)
	{
		int Index = filter.GetRecipientIndex(i);
		if(LUACheckTransmit(iEntIndex, Index))
		{
			MFilter.AddPlayer(Index);
		}
	}

	return origEmitSound2(es, MFilter, iEntIndex, iChannel, pSample, flVolume, iSoundlevel, iFlags, iPitch, pOrigin, pDirection,  pUtlVecOrigins, bUpdatePositions, soundtime, speakerentity);
}
*/

////////////////////////////////////////

GMOD_MODULE_OPEN()
{
	gLua = LUA;

	CreateInterfaceFn interfaceFactory = Sys_GetFactory("engine.dll");
	CreateInterfaceFn gameServerFactory = Sys_GetFactory("server.dll");

	engine = (IVEngineServer*)interfaceFactory(INTERFACEVERSION_VENGINESERVER_VERSION_21, NULL);
	if (engine == NULL)
	{
		LUA->ThrowError("gmsv_pvs: Missing IVEngineServer interface.\n");
	}

	sound = (IEngineSound*)interfaceFactory(IENGINESOUND_SERVER_INTERFACE_VERSION, NULL);
	if (sound == NULL)
	{
		LUA->ThrowError("gmsv_pvs: Missing IEngineSound interface.\n");
	}

	gameents = (IServerGameEnts*)gameServerFactory(INTERFACEVERSION_SERVERGAMEENTS, NULL);
	if (gameents == NULL)
	{
		LUA->ThrowError("gmsv_pvs: Missing IServerGameEnts interface.\n");
	}

	pBaseEdict = engine->PEntityOfEntIndex(0);

	LUA->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);
		LUA->PushString("PVS_CONNECTED");
		LUA->PushNumber(PVS_CONNECTED);
	LUA->SetTable(-3);

	LUA->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);
		LUA->GetField(-1, "Entity");
		entityRef = LUA->ReferenceCreate();
	LUA->Pop();

	LUA->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);
		LUA->GetField(-1, "hook");
		LUA->GetField(-1, "Call");
		hookCallRef = LUA->ReferenceCreate();
	LUA->Pop();

	LUA->CreateMetaTableType("Entity", Type::ENTITY);
		LUA->GetField(-1, "EntIndex");
		entIndexRef = LUA->ReferenceCreate();
	LUA->Pop();

	LUA->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);
		LUA->PushString("pvsSetUpdateTime");
		LUA->PushCFunction(SetUpdateTime);
	LUA->SetTable(-3);

	LUA->CreateMetaTableType("Player", Type::ENTITY);
		LUA->PushCFunction(PlayerUpdatePVS);
		LUA->SetField(-2, "UpdatePVS");
	LUA->Pop();

	if (sound != NULL) {
		HOOKVFUNC(sound, 4, origEmitSound1, newEmitSound1);
	}

	// wasn't called in my testsHookCallRef
	// HOOKVFUNC(sound, 5, origEmitSound2, newEmitSound2);

	HOOKVFUNC(gameents, ISERVERGAMEENTS_CHECKTRANSMIT_OFFSET, origCheckTransmit, newCheckTransmit);

	Msg("gmsv_pvs: Programmed by Spacetech | Entity: %i | hook.Call: %i | EntIndex: %i\n", entityRef, hookCallRef, entIndexRef);

	return 0;
}

GMOD_MODULE_CLOSE()
{
	if (entityRef != -1)
	{
		LUA->ReferenceFree(entityRef);
	}

	if (entIndexRef != -1)
	{
		LUA->ReferenceFree(entIndexRef);
	}

	if (hookCallRef != -1)
	{
		LUA->ReferenceFree(hookCallRef);
	}

	if (sound != NULL)
	{
		Msg("gmsv_pvs: Unhooking Sound\n");
		UNHOOKVFUNC(sound, 4, origEmitSound1);
	}

	if (gameents != NULL)
	{
		Msg("gmsv_pvs: Unhooking PVS\n");
		UNHOOKVFUNC(gameents, ISERVERGAMEENTS_CHECKTRANSMIT_OFFSET, origCheckTransmit);
	}

	return 0;
}
