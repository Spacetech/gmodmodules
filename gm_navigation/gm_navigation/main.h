/*
    gm_navigation
    By Spacetech
*/

#include <interface.h>
#include "eiface.h"
#include "engine/ienginetrace.h"
#include "defines.h"

#ifdef USE_BOOST_THREADS
#include <boost/thread/thread.hpp>
#else
#include <jobthread.h>
IThreadPool* threadPool;
#endif

#ifdef FILEBUG
	FILE *pDebugFile = NULL;
#endif

IVEngineServer *engine = NULL;
IEngineTrace *enginetrace = NULL;
