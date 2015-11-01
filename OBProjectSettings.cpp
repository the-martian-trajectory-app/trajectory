#include "FGProjectSettings.h"

#include "OBEngine.h"
FGEngine *createEngine()
{
	FGEngine *newEngine = new OBEngine();
	return newEngine;
}

