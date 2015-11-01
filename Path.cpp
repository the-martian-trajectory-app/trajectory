#include "Path.h"
#include "OBEngine.h"
#include "FGDoubleGeometry.h"
#include "FGDataWriter.h"
#include "FGDataReader.h"

Path::Path()
{
	m_orbitee = NULL;
	m_engine = NULL;
	m_color = 0;
	m_size = 2; 
}

Path::~Path()
{
}

void Path::init(OBObject *orbitee, FGDoubleVector &pos, FGDoubleVector &vel, int color, int size)
{
	initNoAcc(orbitee, pos, vel, color, size);

	// start off with a max acceleration oberth point
	AccelerationPoint *newPoint = createAccelerationPoint(0);
	newPoint->m_pointIdx = 0;
	newPoint->m_type = ACCTYPE_NORMAL;
	newPoint->m_angle = PI/2.0;
	newPoint->m_mag = PATH_ACCELERATION;

	calcPoints();
}

void Path::initNoAcc(OBObject *orbitee, FGDoubleVector &pos, FGDoubleVector &vel, int color, int size)
{
	// set up the basics
	m_engine = (OBEngine *)FGEngine::getEngine();
	m_orbitee = orbitee;
	m_startPos.set(pos);
	m_startVel.set(vel);
	m_color = color;
	m_size = size;

	calcPoints();
}

void Path::initNoAcc(OBObject *orbiter, double angle)
{
	// convert from J2000 
	angle = -PI/2.0-angle;

	// basics, harvestable from the orbiter
	OBObject *orbitee = orbiter->m_orbitee;
	int color = orbiter->m_orbit.m_color;
	int size = orbiter->m_color;

	// work out the position and velocity from the angle
	FGDoubleVector pos;
	orbiter->m_orbit.getPos(angle, pos);
	FGDoubleVector vel;
	orbiter->m_orbit.getVel(angle, vel);

	// init with these values
	initNoAcc(orbitee, pos, vel, color, size);
}

void Path::save(FGDataWriter &out)
{
	// start and end points
	m_startPos.writeOut(&out);
	m_startVel.writeOut(&out);

	// number of points
	out.writeInt(m_accelerationPoints.size());

	// point info
	for ( AccelerationPointIter iter = m_accelerationPoints.begin() ; iter != m_accelerationPoints.end() ; iter++ )
	{
		// if we have *passed* the current idx, then we know our location
		AccelerationPoint *ap= *iter;
		out.writeInt(ap->m_pointIdx);
		out.writeInt(ap->m_type);
		out.writeDouble(ap->m_angle);
		out.writeDouble(ap->m_mag);
	}
}

void Path::load(FGDataReader &in)
{
	// start and end points
	m_startPos.readIn(&in);
	m_startVel.readIn(&in);

	// clear the old points
	for ( AccelerationPointIter iter = m_accelerationPoints.begin() ; iter != m_accelerationPoints.end() ; iter++ )
	{
		delete *iter;
	}
	m_accelerationPoints.clear();

	// number of points
	int count = in.readInt();

	// point info
	for ( int i=0 ; i<count ; i++ )
	{
		AccelerationPoint *ap = new AccelerationPoint();
		ap->m_pointIdx = in.readInt();
		ap->m_type = in.readInt();
		ap->m_angle = in.readDouble();
		ap->m_mag = in.readDouble();
		m_accelerationPoints.push_back(ap);
	}

	calcPoints();
}

void Path::removeAccelerationPoint(AccelerationPoint *ap)
{
	// you can not remove the initial acceleration point
	if ( ap->m_pointIdx == 0 ) return;

	m_accelerationPoints.remove(ap);
	delete ap;
}

void Path::adjustAccelerationPoint(AccelerationPoint *ap, int mx, int my, double newMag)
{
	// work out the view x,y for this acceleration point
	int pointIdx = ap->m_pointIdx;
	int apX = m_engine->modelToViewX(m_points[pointIdx].m_fixX);
	int apY = m_engine->modelToViewY(m_points[pointIdx].m_fixY);

	// make a vector that goes from the ap to mx, my
	FGDoubleVector newLine;
	// newLine.setXY(apX-mx, apY-my);
	newLine.setXY(mx-apX, my-apY);

	// note that angle
	double unadjustedAng = newLine.getAngle();

	// get the angle from that point to the orbitee
	FGDoubleVector gravDir;
	getGravForPoint(pointIdx, gravDir);
	double gravAng = gravDir.getAngle();
	
	// set the specifics
	ap->m_angle = FGDoubleGeometry::angleDiff(gravAng, unadjustedAng);
	ap->m_mag = newMag;
}

void Path::getGravForPoint(int pointIdx, FGDoubleVector &result)
{
	result.set(m_orbitee->m_pos);

	// we can't trust the location of the current point. We need to
	// calculate from the location of the previous point. 
	if ( pointIdx == 0 )
	{
		result.subtractVector(m_startPos);
	}
	else
	{
		result.subtractVector(m_points[pointIdx-1]);
	}
}

AccelerationPoint *Path::createAccelerationPoint(int pointIdx)
{
	// if the acceleration point is out of range, we fail
	if ( pointIdx < 0 ) return NULL;
	if ( pointIdx >= PATH_NUM_POINTS ) return NULL;

	// find the iterator *after* the point we want to insert
	AccelerationPointIter insertIter;
	for ( insertIter = m_accelerationPoints.begin() ; insertIter != m_accelerationPoints.end() ; insertIter++ )
	{
		AccelerationPoint *ap= *insertIter;
		if ( ap->m_pointIdx == pointIdx )
		{
			// it's already there. 
			return ap;
		}

		if ( ap->m_pointIdx > pointIdx )
		{
			// this is the place we'll be inserting 
			break;
		}
	}

	// note, even a value of end() for insertIter is valid.
	AccelerationPoint *newPoint = new AccelerationPoint();
	newPoint->m_pointIdx = pointIdx;

	// set it to what that point was, but only if we have some basis for setting it
	if ( m_accelerationPoints.size() > 0 )
	{
		// set it up to be the current value for that point
		// note the apparent thrust for that location.
		FGDoubleVector thrust;
		getThrustForPoint(pointIdx, thrust);

		// note the magnitude of thrust
		newPoint->m_mag = thrust.getLength();

		// work out the angle to the orbitee at that point, using the same
		// method the thrust method uses.
		FGDoubleVector gravDir;
		getGravForPoint(pointIdx, gravDir);

		// note the angle difference
		newPoint->m_angle = FGDoubleGeometry::angleDiff(gravDir.getAngle(), thrust.getAngle());
	}

	// presume a normal point
	newPoint->m_type = ACCTYPE_NORMAL;

	// ready to turn it loose.
	m_accelerationPoints.insert(insertIter, newPoint);
	return newPoint;
}

void Path::drawProgressivePath(FGGraphics &g, int pointIdx)
{
	// find the first stop point
	int stopIdx = getStopPoint();
	if ( stopIdx > pointIdx )
	{
		stopIdx = pointIdx;
	}

	// run through the points and draw the path
	int lastX;
	int lastY;
	g.setColor(m_color);
	for ( int i=0 ; i<=stopIdx ; i++ )
	{
		int x = m_engine->modelToViewX(m_points[i].m_fixX);
		int y = m_engine->modelToViewY(m_points[i].m_fixY);

		bool bDraw = false;
		if ( i != 0 )
		{
			// if we aren't thrusting, don't draw this segment
			// but DO draw it if we're past day 170
			FGDoubleVector thrust;
			getThrustForPoint(i, thrust);
			if ( thrust.getLength() != 0.0 )
			{
				bDraw = true;
			}

			if ( i > 170 ) bDraw = true;
		}

		if ( bDraw )
		{
			g.drawLine(lastX, lastY, x, y);
		}

		lastX = x;
		lastY = y;
	}
}

void Path::drawSelf(FGGraphics &g, AccelerationPoint *sel)
{
	// find the first stop point
	int stopIdx = getStopPoint();

	// run through the points and draw the path
	int lastX;
	int lastY;
	g.setColor(m_color);
	for ( int i=0 ; i<=stopIdx ; i++ )
	{
		int x = m_engine->modelToViewX(m_points[i].m_fixX);
		int y = m_engine->modelToViewY(m_points[i].m_fixY);

		if ( i != 0 )
		{
			// draw
			g.drawLine(lastX, lastY, x, y);
		}

		lastX = x;
		lastY = y;
	}

	// run through the acceleration points and draw them
	for ( AccelerationPointIter iter = m_accelerationPoints.begin() ; iter != m_accelerationPoints.end() ; iter++ )
	{
		// if we have *passed* the current idx, then we know our location
		AccelerationPoint *ap= *iter;

		// draw a box around the point
		int pointIdx = ap->m_pointIdx;
		int x = m_engine->modelToViewX(m_points[pointIdx].m_fixX);
		int y = m_engine->modelToViewY(m_points[pointIdx].m_fixY);

		// draw the box
		if ( ap == sel )
		{
			g.setColor(0xffff00);
		}
		else
		{
			if ( ap->m_type == ACCTYPE_REDIRECT )
			{
				g.setColor(0x0000ff);
			}
			else
			{
				g.setColor(0xff0000);
			}
		}
		g.fillRect(x-m_size/2, y-m_size/2, m_size, m_size);

		// draw the acceleration line
		drawThrustLine(g, pointIdx);

		// if this was a stopper, we stop
		if ( ap->m_type == ACCTYPE_STOPTRACE ) break;
	}
}

int Path::getStopPoint()
{
	int highestIdx = 0;
	for ( AccelerationPointIter iter = m_accelerationPoints.begin() ; iter != m_accelerationPoints.end() ; iter++ )
	{
		// if we have *passed* the current idx, then we know our location
		AccelerationPoint *ap= *iter;
		if ( ap->m_type == ACCTYPE_STOPTRACE )
		{
			return ap->m_pointIdx;
		}
	}
	return PATH_NUM_POINTS-1;
}

void Path::drawThrustLine(FGGraphics &g, int pointIDX)
{
	if ( pointIDX == -1 ) return;

	// show the thrust vector at this point
	FGDoubleVector thrust;
	getThrustForPoint(pointIDX, thrust);

	if ( thrust.getLength() == 0.0 ) return;

	thrust.setLength(DISPLAY_THRUSTLINE_LENGTH);
	int x1 = m_engine->modelToViewX(m_points[pointIDX].m_fixX);
	int y1 = m_engine->modelToViewY(m_points[pointIDX].m_fixY);
	int x2 = x1 + (int)thrust.m_fixX;
	int y2 = y1 + (int)thrust.m_fixY;

	g.drawLine(x1, y1, x2, y2);
}

AccelerationPoint *Path::getNearestAccelPoint(int viewX, int viewY)
{
	// see what point idx is closest to this point
	int MAX_DIST = DISPLAY_THRUSTLINE_LENGTH + DISPLAY_THRUSTLINE_LENGTH/10;
	int MAX_DIST_SQ = MAX_DIST*MAX_DIST;

	AccelerationPoint *closestAP = NULL;
	int closestDistSq = 0;

	for ( AccelerationPointIter iter = m_accelerationPoints.begin() ; iter != m_accelerationPoints.end() ; iter++ )
	{
		AccelerationPoint *ap= *iter;
		int accelPointIdx = ap->m_pointIdx;

		int dx = viewX - m_engine->modelToViewX(m_points[accelPointIdx].m_fixX);
		int dy = viewY - m_engine->modelToViewY(m_points[accelPointIdx].m_fixY);
		int distSq = dx*dx + dy*dy;
		if ( distSq < MAX_DIST_SQ )
		{
			if ( (closestAP == NULL) || (distSq < closestDistSq) )
			{
				closestDistSq = distSq;
				closestAP = ap;
			}
		}

		// if this was a stopper, we stop
		if ( ap->m_type == ACCTYPE_STOPTRACE ) break;
	}

	return closestAP;
}

int Path::getNearestPointIdx(int viewX, int viewY)
{
	// see what point idx is closest to this point
	int MAX_DIST = DISPLAY_THRUSTLINE_LENGTH + DISPLAY_THRUSTLINE_LENGTH/10;
	int MAX_DIST_SQ = MAX_DIST*MAX_DIST;

	int stopIdx = getStopPoint();
	int closestIdx = -1;
	int closestDistSq = 0;

	for ( int i=0 ; i<=stopIdx ; i++ )
	{
		int dx = viewX - m_engine->modelToViewX(m_points[i].m_fixX);
		int dy = viewY - m_engine->modelToViewY(m_points[i].m_fixY);
		int distSq = dx*dx + dy*dy;
		if ( distSq < MAX_DIST_SQ )
		{
			if ( (closestIdx == -1) || (distSq < closestDistSq) )
			{
				closestDistSq = distSq;
				closestIdx = i;
			}
		}
	}

	return closestIdx;
}

void Path::getThrustForPoint(int pointIdx, FGDoubleVector &result)
{
	if ( m_accelerationPoints.size() == 0 )
	{
		// no points at all
		result.setXY(0,0);
		return;
	}

	// note the vector to the orbitee. 
	FGDoubleVector gravDir;
	getGravForPoint(pointIdx, gravDir);

	// find out which acceleration point we're affected by
	AccelerationPoint *ap = NULL;
	for ( AccelerationPointIter iter = m_accelerationPoints.begin() ; iter != m_accelerationPoints.end() ; iter++ )
	{
		// if we have *passed* the current idx, then we know our location
		AccelerationPoint *thisAP = *iter;
		if ( thisAP->m_pointIdx > pointIdx )
		{
			// this is our guy
			iter--;
			ap = *iter;
			break;
		}
	}

	if ( ap == NULL )
	{
		// we are on the last acceleration point
		AccelerationPointIter iter = m_accelerationPoints.end();
		iter--;
		ap = *iter;
	}

	// create a relative acceleration vector based on the deflection angle and magnitude
	FGDoubleVector acc;
	acc.set(gravDir);
	acc.rotate(ap->m_angle);
	acc.setLength(ap->m_mag);

	// done
	result.set(acc);
}

void Path::calcPoints()
{
	// note the vel and pos. 
	FGDoubleVector pos;
	FGDoubleVector vel;
	pos.set(m_startPos);
	vel.set(m_startVel);

	// start off at the start pos
	int stopIdx = getStopPoint();
	m_points[0].set(pos);

	bool bHalt = false;

	for ( int i=1 ; i<=stopIdx ; i++ )
	{
		if ( bHalt )
		{
			m_points[i].set(m_points[i-1]);
			continue;
		}

		// first, accelerate based on the thing we're orbiting
		// start with a vector pointed at the orbitee
		FGDoubleVector gravAcc;
		gravAcc.set(m_orbitee->m_pos);
		gravAcc.subtractVector(pos);

		// work out the strength of the acceleration. 
		// It'll be u/d^2 where u = SGP, and d = distance to the object
		// this is a good time to include the seconds value
		double gravAccLen = (POINTS_TIME * m_orbitee->m_sgp)/gravAcc.getLengthSq();
	
		// set the new length
		gravAcc.setLength(gravAccLen);

		// apply the acceleration
		vel.addVector(gravAcc);

		FGDoubleVector thrustAcc;
		getThrustForPoint(i, thrustAcc);

		// we now know the acceleration vector. Apply it to the vel
		// first we'll have to multiply by the number of seconds in a step
		thrustAcc.scalarMultiply(POINTS_TIME);
		vel.addVector(thrustAcc);

		// run through the acceleration points to see if this point is a redirect
		for ( AccelerationPointIter iter = m_accelerationPoints.begin() ; iter != m_accelerationPoints.end() ; iter++ )
		{
			// if we have *passed* the current idx, then we know our location
			AccelerationPoint *ap = *iter;
			if ( ap->m_pointIdx != i ) continue;

			// the current point
			if ( ap->m_type == ACCTYPE_REDIRECT )
			{
				// it's a redirect. Change the angle of the velocity
				// vector to the angle of the thrust vector
				vel.setRTheta(vel.getLength(), thrustAcc.getAngle());
			}
			break;
		}

		// now apply the vel to the pos. But we don't want to ruin the value we have
		// for vel, so we use an intermediate vector for the mult
		FGDoubleVector work;
		work.set(vel);
		work.scalarMultiply(POINTS_TIME);
		pos.addVector(work);

		// note the point
		m_points[i].set(pos);

		// if the pos gets closer than a certain distance from the sun, we just stop
		if ( FGDoubleGeometry::getDistance(m_points[i], m_orbitee->m_pos) < FATAL_SUN_APPROACH )
		{
			bHalt = true;
		}
	}
}

