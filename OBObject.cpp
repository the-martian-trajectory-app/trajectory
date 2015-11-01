#include "OBObject.h"
#include "OBEngine.h"

#include "FGDoubleGeometry.h"

OBObject::OBObject()
{
}

OBObject::~OBObject()
{
}

void OBObject::initOrbitee(double sgp, FGDoubleVector &pos, int color, int size)
{
	m_pos.set(pos);
	m_vel.setXY(0.0, 0.0);
	m_color = color;
	m_size = size;
	m_sgp = sgp;
	m_orbitee = NULL;

	m_engine = (OBEngine *)FGEngine::getEngine();
}

void OBObject::initOrbiter(OBObject *orbitee, double apogeeDist, double apogeeVel, double aop, int color, int size)
{
	// work out the position and velocity of apogee
	FGDoubleVector pos;
	FGDoubleVector vel;

	// we know the angle of periapsis. That minus PI will be
	// the andge of apogee. And we are given the distance at apogee.
	double angleOfApogee = aop - 3.14159;
	pos.setRTheta(apogeeDist, angleOfApogee);
	pos.addVector(orbitee->m_pos); // offset by the position of the orbitee

	// the vel is PI/2 radians off from the position.
	vel.set(pos);
	vel.rotate(-3.14159/2.0);
	vel.setLength(apogeeVel);

	// now we have the position and velocity of apogee. We can feed it to the main initter.
	initOrbiter(orbitee, pos, vel, color, size);
}

void OBObject::initOrbiter(OBObject *orbitee, FGDoubleVector &pos, FGDoubleVector &vel, int color, int size)
{
	m_pos.set(pos);
	m_vel.set(vel);
	m_color = color;
	m_size = size;
	m_sgp = 0.0;
	m_orbitee = orbitee;
	m_engine = (OBEngine *)FGEngine::getEngine();

	// prep the orbit
	m_orbit.initPV(m_orbitee->m_sgp, m_pos, m_vel);
	m_orbit.setColorFromObjectColor(m_color);
}

void OBObject::initOrbiter(OBObject *orbitee, Orbit &orbit, double theta, int color, int size)
{
	// note the basics
	m_color = color;
	m_size = size;
	m_sgp = 0.0;
	m_orbitee = orbitee;
	m_engine = (OBEngine *)FGEngine::getEngine();

	// adopt that orbit, and note color stuff
	m_orbit.set(orbit);
	m_orbit.setColorFromObjectColor(m_color);

	// work out our position and velocity from the sent-in angle
	m_orbit.getPos(theta, m_pos);
	m_orbit.getVel(theta, m_vel);
}

void OBObject::tick(double seconds)
{
	if ( m_orbitee == NULL ) return;

	// accelerate based on the gravitic object
	// start with a vector pointed at the orbitee
	FGDoubleVector acc;
	acc.set(m_orbitee->m_pos);
	acc.subtractVector(m_pos);

	// work out the strength of the acceleration. 
	// It'll be u/d^2 where u = SGP, and d = distance to the object
	// this is a good time to include the seconds value
	double accLen = (seconds * m_orbitee->m_sgp)/acc.getLengthSq();
	
	// set the new length
	acc.setLength(accLen);

	// apply the acceleration
	m_vel.addVector(acc);

	// apply the velocity. Our vel is in km/s, so we need to multiply
	FGDoubleVector toAdd;
	toAdd.set(m_vel);
	toAdd.scalarMultiply(seconds);
	m_pos.addVector(toAdd);
}

void OBObject::drawSelf(FGGraphics &g)
{
	m_orbit.drawSelf(g);

	// note our location, offset by half our size
	int x = m_engine->modelToViewX(m_pos.m_fixX) - m_size/2;
	int y = m_engine->modelToViewY(m_pos.m_fixY) - m_size/2;

	g.setColor(m_color);
	g.fillRect(x, y, m_size, m_size);
}

void OBObject::recalcOrbit()
{
	m_orbit.initPV(m_orbit.m_u, m_pos, m_vel);
}


