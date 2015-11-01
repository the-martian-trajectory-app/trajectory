#ifndef __PATH__
#define __PATH__

#include "FGDoubleVector.h"
#include "FGGraphics.h"
#include <list>

class OBObject;
class OBEngine;

// how long each point represents, and how many points there are
#define POINTS_TIME (86400.0) 
#define PATH_NUM_POINTS 900 

// due to the units of this simulator, this is in km/s^2. 
// So it's a very small number
#define PATH_ACCELERATION (0.000002)

#define DISPLAY_THRUSTLINE_LENGTH 50

// acceleration point types
#define ACCTYPE_NORMAL 0    // normal acceleration
#define ACCTYPE_REDIRECT 1  // reset the direction of travel, keep the current velocity
#define ACCTYPE_STOPTRACE 2 // stop showing the trace beyond this point

#define FATAL_SUN_APPROACH (35000000.0)

// an acceleration point
class AccelerationPoint
{
public:
	int m_pointIdx; // the point index that this acceleration takes effect

	// the acceleration, relative to the vector from the object to the thing its orbiting
	int m_type;
	double m_angle;
	double m_mag;
};

typedef std::list<AccelerationPoint *> AccelerationPointList;
typedef AccelerationPointList::iterator AccelerationPointIter;

class Path
{
public:
	Path();
	virtual ~Path();

	void init(OBObject *orbitee, FGDoubleVector &pos, FGDoubleVector &vel, int color, int size);
	void initNoAcc(OBObject *orbitee, FGDoubleVector &pos, FGDoubleVector &vel, int color, int size);

	// takes angle in J200 coordinate system
	void initNoAcc(OBObject *orbiter, double angle);

	void drawSelf(FGGraphics &g, AccelerationPoint *sel);
	void drawThrustLine(FGGraphics &g, int pointIDX);
	void drawProgressivePath(FGGraphics &g, int pointIdx);

	int getNearestPointIdx(int viewX, int viewY);
	AccelerationPoint *getNearestAccelPoint(int viewX, int viewY);

	void getThrustForPoint(int pointIdx, FGDoubleVector &result);
	void getGravForPoint(int pointIdx, FGDoubleVector &result);
	int getStopPoint();
	void calcPoints();

	AccelerationPoint *createAccelerationPoint(int pointIdx);
	void removeAccelerationPoint(AccelerationPoint *ap);
	void adjustAccelerationPoint(AccelerationPoint *ap, int mx, int my, double newMag);

	// persistance
	void save(FGDataWriter &out);
	void load(FGDataReader &in);

	// data
	FGDoubleVector m_startPos;
	FGDoubleVector m_startVel;
	FGDoubleVector m_points[PATH_NUM_POINTS]; // in model coordinates
	OBObject *m_orbitee;
	OBEngine *m_engine;

	// acceleration points
	AccelerationPointList m_accelerationPoints;

	// display stuff
	int m_color;
	int m_size; 
};

#endif

