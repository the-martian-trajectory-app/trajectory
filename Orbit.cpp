#include <math.h>
#include "Orbit.h"
#include "OBEngine.h"
#include "OBGlobals.h"
#include "FGDoubleGeometry.h"

Orbit::Orbit()
{
	m_drawPoints = NULL;
	m_numDrawPoints = 0;
	m_color = 0x7f7f7f;
	m_bValid = false;
}

Orbit::~Orbit()
{
	delete[] m_drawPoints;
}

void Orbit::set(Orbit &other)
{
	// display
	m_color = other.m_color;
	m_engine = other.m_engine;

	m_f1.set(other.m_f1);
	m_f2.set(other.m_f2);
	m_center.set(other.m_center);
	m_u = other.m_u;
	m_e = other.m_e;
	m_a = other.m_a;
	m_b = other.m_b;
	m_f = other.m_f;
	m_w = other.m_w;
	m_energy = other.m_energy;
	m_apogee = other.m_apogee;
	m_perigee = other.m_perigee;
	m_orbitArea = other.m_orbitArea;
	m_bValid = other.m_bValid;

	if ( other.m_drawPoints == NULL )
	{
		delete[] m_drawPoints;
		m_drawPoints = NULL;
		m_numDrawPoints = 0;
	}
	else
	{
		// deal with draw points
		m_numDrawPoints = other.m_numDrawPoints;
		delete[] m_drawPoints;
		m_drawPoints = new DrawPoint[m_numDrawPoints];
		for ( int i=0 ; i<m_numDrawPoints ; i++ )
		{
			m_drawPoints[i] = other.m_drawPoints[i];
		}
	}
}

void Orbit::setColorFromObjectColor(int objectColor)
{
	// cut each of the r, g, and b faluse in half
	int r = (objectColor&0xff0000)>>16;
	int g = (objectColor&0x00ff00)>>8;
	int b = (objectColor&0x0000ff);

	r/=4;
	g/=4;
	b/=4;

	m_color = (r<<16) + (g<<8) + b;
}

void Orbit::setColor(int color)
{
	m_color = color;
}

void Orbit::init(double sgp, double e, double a, double w)
{
	m_engine = (OBEngine *)FGEngine::getEngine();

	m_bValid = true;
	if ( e > 1.0 ) 
	{
		m_bValid = false;
		return;
	}
	if ( a <= 0.0 )
	{
		m_bValid = false;
		return;
	}

	// start with what we know
	m_f1.setXY(0.0, 0.0);
	m_u = sgp;
	m_e = e;
	m_a = a;
	m_w = w;

	// work out what we don't know

	// e = f/a, so f = ea
	m_f = m_e*m_a;

	// f = sqrt(a^2 - b^2). 
	// From that we can derive: b = sqrt (a^2 - f^2)
	m_b = sqrt(m_a*m_a - m_f*m_f);

	// the center is f distance from f1 along angle w
	m_center.setRTheta(m_f, w);

	// f2 is 2f distance from f1 along angle w
	m_f2.setRTheta(2.0*m_f, w);

	// the specific orbital energy is (by definition) -u/2a
	m_energy = -m_u/(2.0*m_a);

	// work our apogee and perigee.
	// apogee is just a+f. Perigee is just a-f
	m_apogee = m_a + m_f;
	m_perigee = m_a - m_f;

	// note the area
	m_orbitArea = PI*m_a*m_b;

	// precalculate the draw points
	calcDrawPoints();
}

void Orbit::initPV(double sgp, FGDoubleVector &orbiterPos, FGDoubleVector &orbiterVel)
{
	// note what we were given
	m_f1.setXY(0.0, 0.0);
	m_u = sgp;

	// note the orbiter position relative to F1. The vector will be
	// pointing FROM the orbiter TOWARD F1. So we're just reversing it.
	FGDoubleVector relativePos;
	relativePos.set(orbiterPos);
	relativePos.scalarMultiply(-1.0);

	// note the distance from F1 of the orbiter
	double r = relativePos.getLength();
	if ( r <= 0.0 )
	{
		m_bValid = false;
		return;
	}

	// note the length of the velocity vector
	double v = orbiterVel.getLength();

	// note the specific orbital energy from the position and velocity of the orbiter
	// this is one of the definitions of specific orbital energy (it will make a negative
	// number)
	m_energy = (v*v/2.0) - (m_u/r);
	if ( m_energy >= 0.0 )
	{
		// non-negative means an escape
		m_bValid = false;
		return;
	}

	// from the energy, we can calculate the semi-major axis length 
	// (note the negation).
	m_a = -m_u/(2.0*m_energy);

	// work out the distance from the orbiter to F2
	double d = 2.0*m_a - r;

	// work out the angle toward F2 from the velocity vector.
	// This works out to be PI-Theta, where Theta is the angle
	// between the velocity vector and the line from the orbiter
	// to F1.
	double orbiterAngle = relativePos.getAngle();
	double velAngle = orbiterVel.getAngle();
	double theta = FGDoubleGeometry::angleDiff(velAngle, orbiterAngle);
	double phi = PI - theta;

	// F2 will be d units along that angle from the orbiter 
	m_f2.set(orbiterVel);
	m_f2.setLength(d);
	m_f2.rotate(phi);
	m_f2.addVector(orbiterPos);

	// w will be the angle of F1F2
	// F1 is always 0,0 so it's just the angle to F2
	m_w = m_f2.getAngle();

	// f will be half the distance from F1 to F2
	// f1 is always 0,0, so it's just half the length of f2
	m_f = m_f2.getLength()/2.0;

	// eccentricity is f/a
	m_e = m_f/m_a;

	// now we have e, a, and w. We can call the primary init
	init(sgp, m_e, m_a, m_w);
}

void Orbit::initAP(double sgp, FGDoubleVector &apogee, FGDoubleVector &perigee)
{
	// we need eccentricity, semomajor axis, and angle. All are pretty easy to get
	// when you know the apogee, perigee, and a focus. 

	// sanity check: The apogee and perigee have to be in line with the focus (0,0).
	// we'll just check the angles and their difference
	double angleDiff = FGDoubleGeometry::angleDiff(apogee.getAngle(), perigee.getAngle());
	angleDiff = fabs(angleDiff);
	if ( !fnear(angleDiff, PI, 0.01) )
	{
		// not close enough to opposition.
		m_bValid = false;
		return;
	}

	// pretty straight forward, and we'll need them in a minute.
	m_apogee = apogee.getLength();
	m_perigee = perigee.getLength();

	// the semimajor axis is just half the distance from perigee to apogee. 
	m_a = (m_apogee + m_perigee)/2.0;

	// w is the angle from f1 to apogee.
	m_w = apogee.getAngle();

	// and e is defined as (a-p)/(a+p) where a and p are apogee and perigee distances
	m_e = (m_apogee-m_perigee)/(m_apogee+m_perigee);

	// we now have the criticals
	init(sgp, m_e, m_a, m_w);
}

void Orbit::calcDrawPoints()
{
	if ( !m_bValid ) return; 

	// working vector
	FGDoubleVector work;

	// note how far to walk along the ellipse per step
	// we'll shoot for a number of pixels
	double xStep = 2.0*m_engine->m_kmPerPixel; // 2 pixels per step

	// prep the points array. 
	int numPoints = int((2.0*m_a)/xStep) + 2; // the +2 is for rounding safety
	int maxDrawPoints = numPoints*2;

	// set up the drawpoints array (trashing any old one that may have been there)
	delete[] m_drawPoints;
	m_drawPoints = new DrawPoint[maxDrawPoints];

	// working arrays
	DrawPoint *firstHalf = new DrawPoint[numPoints];
	DrawPoint *secondHalf = new DrawPoint[numPoints];
	int halfPos = 0;

	// work out the points to connect to draw the ellipse
	// The relevant thing here is the definition of an ellipse.
	// from this we can calculate that y = sqrt(b^2*(1-x^2/a^2))
	// so we start x at perihelion, stroll along to aphelion, and 
	// note the y values. This gives us half the ellipse

	// the total width of the ellipse will be the semi-major axis times 2
	double x = -m_a;
	while ( true )
	{
		// calculate the y for this x
		double alpha = 1.0-(x*x)/(m_a*m_a);
		double y = sqrt(m_b*m_b*alpha);

		if ( halfPos >= numPoints )
		{
			FGEngine::fatal("Draw point overflow");
		}

		// make a vector
		work.setXY(x, y);

		// rotate by the orbital angle
		work.rotate(m_w);

		// the x,y values are deltas from the ellipse center
		// so we have to offset to the center of this ellipse
		work.addVector(m_center);

		// what we just worked out was the first half
		firstHalf[halfPos].m_viewX = m_engine->modelToViewX(work.m_fixX);
		firstHalf[halfPos].m_viewY = m_engine->modelToViewY(work.m_fixY);

		// now work out the other half's location. Same process as documented
		// above, but we negate the y value before translating and rotating it.
		work.setXY(x, -y);
		work.rotate(m_w);
		work.addVector(m_center);
		secondHalf[halfPos].m_viewX = m_engine->modelToViewX(work.m_fixX);
		secondHalf[halfPos].m_viewY = m_engine->modelToViewY(work.m_fixY);

		// advance the half pos
		halfPos++;

		if ( x == m_a )
		{
			// finished the last point
			break;
		}

		// advance x
		x += xStep;
		if ( x > m_a ) 
		{
			x = m_a;
		}
	}

	// assemble all the points in to a single sequence.
	// we run the first half in order, and the second half in reverse
	// so the line draws are always in one direction around the arc.
	m_numDrawPoints = 0;
	for ( int i=0 ; i<halfPos ; i++ )
	{
		if ( m_numDrawPoints >= maxDrawPoints )
		{
			FGEngine::fatal("draw point overflow");
		}

		m_drawPoints[m_numDrawPoints] = firstHalf[i];
		m_numDrawPoints++;
	}

	for ( int i=halfPos-1 ; i>=0 ; i-- )
	{
		if ( m_numDrawPoints >= maxDrawPoints )
		{
			FGEngine::fatal("draw point overflow");
		}

		m_drawPoints[m_numDrawPoints] = secondHalf[i];
		m_numDrawPoints++;
	}

	// done with the working arrays
	delete[] firstHalf;
	delete[] secondHalf;
}

void Orbit::drawSelf(FGGraphics &g)
{
	if ( !m_bValid ) return; 
	if ( m_drawPoints == NULL ) return; 
	if ( m_numDrawPoints < 2 ) return; 

	g.setColor(m_color);

	int lastX = m_drawPoints[0].m_viewX;
	int lastY = m_drawPoints[0].m_viewY;
	for ( int i=1 ; i<m_numDrawPoints ; i++ )
	{
		int thisX = m_drawPoints[i].m_viewX;
		int thisY = m_drawPoints[i].m_viewY;
		g.drawLine(lastX, lastY, thisX, thisY);

		lastX = thisX;
		lastY = thisY;
	}
}

double Orbit::getR(double theta)
{
	if ( !m_bValid ) return 0.0; 

	// applying a straightforward polar ellipse formula
	// broken in to chunks for readability
	double numerator = m_a*(1.0-m_e*m_e);
	double denominator = 1.0-m_e*(cos(theta-m_w));
	double r = numerator/denominator;
	return r;
}

void Orbit::getVel(double theta, FGDoubleVector &outVel)
{
	// we'll need the position for this theta
	FGDoubleVector pos;
	getPos(theta, pos);

	// how far is this from F1? F1 is 0,0, so
	// it's just the length of the vector
	double r = pos.getLength();

	// e = v^2/2 - u/r
	// solve that for v and we get: v=sqrt(2(e+u/r));
	double vSquared= 2.0*(m_energy+m_u/r);
	double v = sqrt(vSquared);

	// we now have the velocity, but we need to work out the 
	// vector angle. A body in orbit will always travel perpendicular to
	// a line that bisects the lines from the object to the two
	// foci of the ellipse. 
	// yeah, it's complicated to write, but easy to understand when you
	// draw it out. 

	// note the angle from the object to f1. That's PI + theta
	double angleToF1 = PI+theta;

	// now the angle from the object to F2. 
	FGDoubleVector lookAtF2;
	lookAtF2.set(m_f2);
	lookAtF2.subtractVector(pos);
	double angleToF2 = lookAtF2.getAngle();

	// bisect those angles. Careful now, we can't just take the average.
	// one might be negative and the other positive. So we'll use angleDiff.
	// anglediff is the angle required to go FROM angle1 TO angle2. 
	double angleDiff = FGDoubleGeometry::angleDiff(angleToF1, angleToF2);
	double angle = angleToF1 + angleDiff/2.0; // go half-way to angle 2 and you're at the bisection point.

	// the "angle" we have now is the bisection angle. We need to be PI/2 off from that to have the 
	// tangent.
	double velAngle = angle + (PI/2.0);

	// now we have the velocity and angle
	outVel.setRTheta(v, velAngle);
}

void Orbit::getPos(double theta, FGDoubleVector &outPos)
{
	// get the R for that theta
	double r = getR(theta);

	// make the vector
	outPos.setRTheta(r, theta);
}

double Orbit::calcDeviance(Orbit &other)
{
	if ( !m_bValid ) return 0.0; 

	// special case: if one orbit's apogee is smaller than the other orbit's
	// perigee, the first orbit is completely contained within the second.
	if ( m_apogee < other.m_perigee )
	{
		// we are completely contained within other
		return other.m_orbitArea - m_orbitArea;
	}
	if ( other.m_apogee < m_perigee )
	{
		// other is completely contained within us
		return m_orbitArea - other.m_orbitArea;
	}

	// do a lap around and note the difference between the r values
	double thetaStep = TWOPI/1440.0; // one fourth of a degree per step. Arbitrarily chosen.

	double deviance = 0.0;
	for ( double theta=0.0 ; theta<TWOPI ; theta+=thetaStep )
	{
		// note the distances at this location
		double rThis = getR(theta);
		double rOther = other.getR(theta);

		// make them in to volumes. Each is a stepFraction chunk of a circle
		// good old r^2*dTheta/2
		double areaThis = rThis*rThis*thetaStep*0.5;
		double areaOther = rOther*rOther*thetaStep*0.5;
		double diff = fabs(areaThis - areaOther);
		deviance += diff;
	}

	return deviance;
}

