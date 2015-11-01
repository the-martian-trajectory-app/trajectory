#ifndef __ORBIT__
#define __ORBIT__

#include "FGDoubleVector.h"
#include "FGGraphics.h"

class OBEngine;

class DrawPoint
{
public:
	int m_viewX;
	int m_viewY;
};

// note: the gravitic body is presumed to be at 0,0. This means that the f1 point
// will *always* be at (0.0, 0.0).

class Orbit
{
public:
	Orbit();
	~Orbit();

	void set(Orbit &other); // copy all the values of the sent-in orbit

	// init the orbit with the key traits:
	// sgp = Standard Gravitational Parameter of the gravitic mass
	// e = eccentricity of the orbit
	// a = semi-major axis length. Note this is NOT apogee distance.
	//     This is the distance from apogee to the center of the 
	//     ellipse. (Not from the gravitation object at a focus)
	// w = Angle of the major axis. This is also the angle of apogee.
	void init(double sgp, double e, double a, double w);

	// init the orbit with the gravitic mass, sgp, and an orbiter's position and velocity
	void initPV(double sgp, FGDoubleVector &orbiterPos, FGDoubleVector &orbiterVel);

	// init the orbit with known apogee and perigee points
	void initAP(double sgp, FGDoubleVector &apogee, FGDoubleVector &perigee);

	// information about the orbit
	bool isValid() { return m_bValid; }
	double getR(double theta); // send in the theta, this will return the r (distance from F1 to the ellipse point)
	double calcDeviance(Orbit &other); // very time consuming. Gives the total area of the two orbits that does *not* overlap.
	void getVel(double theta, FGDoubleVector &outVel); // get the velocity vector for the body when its at angle theta.
	void getPos(double theta, FGDoubleVector &outPos); // get the position vector for the body when its at angle theta.

	// display settings
	void setColorFromObjectColor(int objectColor); // work out a color based on the orbiter's color
	void setColor(int color); // set the color directly

	// draw
	void drawSelf(FGGraphics &g);


	// helpers
	void calcDrawPoints();

	// display
	int m_color;
	OBEngine *m_engine;
	DrawPoint *m_drawPoints;
	int m_numDrawPoints;

	// relevant orbit data. However the orbit is initted, all
	// these values will be calculated and stored. 
	FGDoubleVector m_f1; // focus 1 (the location of the gravitic object)
	FGDoubleVector m_f2; // focus 2 (no object here)
	FGDoubleVector m_center; // center of the ellipse
	double m_u; // the Standard Gravitation Parameter of the gravitic mass
	double m_e; // eccentricity of the ellipse
	double m_a; // semi-major axis length of the ellipse
	double m_b; // semi-minor axis length of the ellipse
	double m_f; // distance from C to either focus
	double m_w; // angle (radians) of the line through the foci
	double m_energy; // specific orbital energy of this orbit
	double m_apogee; // distance from the gravitic body at apogee
	double m_perigee; // distance from the gravitic body at perigee
	double m_orbitArea; // the total area of this orbit (area of the ellipse)

	// if this is false, it means it's not an orbit. Usually this means it's an escape,
	// but it can also mean the path comes too close to the gravity object for calculation. 
	// If this is true, you should consider all functions inoperative.
	bool m_bValid;
};


#endif

