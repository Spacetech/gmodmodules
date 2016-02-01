/*
    MRecipientFilter
    Edited by Spacetech
	Found on sourcemods forum
*/ 

#include "mrecpfilter.h"

MRecipientFilter::MRecipientFilter(IVEngineServer *m_Engine)
{
	engine = m_Engine;
}

MRecipientFilter::~MRecipientFilter()
{
}

int MRecipientFilter::GetRecipientCount() const
{
   return m_Recipients.Size();
}

int MRecipientFilter::GetRecipientIndex(int slot) const
{
   if ( slot < 0 || slot >= GetRecipientCount() )
      return -1;

   return m_Recipients[ slot ];
}

bool MRecipientFilter::IsInitMessage() const
{
   return false;
}

bool MRecipientFilter::IsReliable() const
{
   return false;
}

void MRecipientFilter::AddPlayer(int pIndex)
{
	edict_t *pPlayer = engine->PEntityOfEntIndex(pIndex);
	if(!pPlayer || pPlayer->IsFree())
	{
		return;
	}
	m_Recipients.AddToTail(pIndex);
}