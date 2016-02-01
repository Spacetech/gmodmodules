/*
    MRecipientFilter
    Edited by Spacetech
	Found on sourcemods forum
*/ 

#ifndef _MRECIIENT_FILTER_H
#define _MRECIPIENT_FILTER_H

#include <interface.h>
#include "eiface.h"
#include "irecipientfilter.h"
#include "tier1/utlvector.h"

class MRecipientFilter : public IRecipientFilter
{
public:
   MRecipientFilter(IVEngineServer*);
   ~MRecipientFilter(void);

   virtual bool IsReliable( void ) const;
   virtual bool IsInitMessage( void ) const;

   virtual int GetRecipientCount( void ) const;
   virtual int GetRecipientIndex( int slot ) const;
   void AddPlayer(int pIndex);

private:
	bool m_bReliable;
	bool m_bInitMessage;
	IVEngineServer *engine;
	CUtlVector< int > m_Recipients;
};

#endif 