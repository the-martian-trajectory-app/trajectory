#ifndef __OBOJECT__
#define __OBOJECT__

#include "FGDoubleVector.h"
#include "FGGraphics.h"
#include "Orbit.h"

class OBEngine;

class OBObject
{
public:
	OBObject();
	~OBObject();

	// you can send NULL for the orbitee. It's ok to send 0 for SGP
	void initOrbitee(double sgp, FGDoubleVector &pos, int color, int size);
	void initOrbiter(OBObject *orbitee, FGDoubleVector &pos, FGDoubleVector &vel, int color, int size);
	void initOrbiter(OBObject *orbitee, double apogeeDist, double apogeeVel, double aop, int color, int size);
	void initOrbiter(OBObject *orbitee, Orbit &orbit, double theta, int color, int size);
	void drawSelf(FGGraphics &g);
	void tick(double seconds); 

	// something external has affected the position or velocity. 
	// recalculate the orbit based on our current pos and vel
	void recalcOrbit();

	// data
	FGDoubleVector m_pos;
	FGDoubleVector m_vel;

	// display stuff
	int m_color;
	int m_size; 

	// relevant only if something is orbiting this
	double m_sgp;

	// cached for perf
	OBEngine *m_engine;

	// the thing we're orbiting
	OBObject *m_orbitee;

	Orbit m_orbit;
};

#endif