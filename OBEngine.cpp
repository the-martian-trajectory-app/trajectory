#include "OBEngine.h"
#include "FGDataWriter.h"
#include "FGDataReader.h"
#include "FGDoubleGeometry.h"

#define PLAYBACK_STEP_TIME 50

OBEngine::OBEngine()
{
}

OBEngine::~OBEngine()
{
}

void OBEngine::init()
{
	m_screenW = getScreenWidth();
	m_screenH = getScreenHeight();

	// init the font
	m_font.init("smallfont.png", "smallfont.font");

	// init graphics metrics
	double r = MARS_APOGEE*1.05;
	double width = r*2.0;
	m_kmPerPixel = width/(double)m_screenW;

	// start off centered at 0,0
	m_center.setXY(0.0, 0.0);

	// sun and earth
	FGDoubleVector pos;
	FGDoubleVector vel;

	// the sun is in the middle and doesn't move
	pos.setXY(0.0, 0.0);
	vel.setXY(0.0, 0.0);
	m_sun.initOrbitee(SUN_SGP, pos, 0xffffff, 8);

	// start the planets
	m_venus.initOrbiter(&m_sun, VENUS_APOGEE, VENUS_APOGEE_VEL, VENUS_AOP, 0xffff7f, 1);
	m_earth.initOrbiter(&m_sun, EARTH_APOGEE, EARTH_APOGEE_VEL, EARTH_AOP, 0x7f7fff, 1);
	m_mars.initOrbiter(&m_sun, MARS_APOGEE, MARS_APOGEE_VEL, MARS_AOP, 0xff7f7f, 1);

	/************ PATHS *****************/
	// July 7, 2035
	m_venusPath.initNoAcc(&m_venus, 1.4623927498);
	m_earthPath.initNoAcc(&m_earth, 4.9745875522);
	m_marsPath.initNoAcc(&m_mars, 5.4429575522);
	m_ship.init(&m_sun, m_earthPath.m_startPos, m_earthPath.m_startVel, 0x7f7f7f, 5);

	// internals
	m_hoverPathPointIdx = -1;
	m_hoverAccelPoint = NULL;
	m_uiMode = UI_INERT;
	m_bShowVenus = false;
}

void OBEngine::onKeyPressed(int key)
{
	if ( key == 16 ) 
	{
		m_hoverPathPointIdx = -1;
		m_hoverAccelPoint = NULL;
		m_uiMode = UI_ADDINGPOINT;
	}
}

void OBEngine::onKeyReleased(int key)
{
	m_msg.set("");

	if ( key == 'V' )
	{
		m_bShowVenus = !m_bShowVenus;
	}

	if ( key == 16 ) 
	{
		m_hoverPathPointIdx = -1;
		m_hoverAccelPoint = NULL;
		m_uiMode = UI_INERT;
	}

	if ( (key==8) || (key==46) )
	{
		// delete/backspace
		if ( m_hoverAccelPoint != NULL )
		{
			m_ship.removeAccelerationPoint(m_hoverAccelPoint);
			m_ship.calcPoints();
			m_hoverAccelPoint = NULL;
		}
	}

	if ( key=='0' )
	{
		// kill the thrust for this point
		if ( m_hoverAccelPoint != NULL )
		{
			m_hoverAccelPoint->m_mag = 0.0;
			m_ship.calcPoints();
			m_hoverAccelPoint = NULL;
		}
	}

	if ( key=='1' )
	{
		// max out the thrust for this point
		if ( m_hoverAccelPoint != NULL )
		{
			m_hoverAccelPoint->m_mag = PATH_ACCELERATION;
			m_ship.calcPoints();
			m_hoverAccelPoint = NULL;
		}
	}
	if ( key == 'X' )
	{
		if ( m_hoverAccelPoint != NULL )
		{
			if ( m_hoverAccelPoint->m_type == ACCTYPE_NORMAL )
			{
				m_hoverAccelPoint->m_type = ACCTYPE_STOPTRACE;
			}
			else if ( m_hoverAccelPoint->m_type == ACCTYPE_STOPTRACE )
			{
				m_hoverAccelPoint->m_type = ACCTYPE_NORMAL;
			}
			m_ship.calcPoints();
			m_hoverAccelPoint = NULL;
		}
	}
	if ( key == 'R' )
	{
		if ( m_hoverAccelPoint != NULL )
		{
			if ( m_hoverAccelPoint->m_type == ACCTYPE_NORMAL )
			{
				m_hoverAccelPoint->m_type = ACCTYPE_REDIRECT;
			}
			else if ( m_hoverAccelPoint->m_type == ACCTYPE_REDIRECT )
			{
				m_hoverAccelPoint->m_type = ACCTYPE_NORMAL;
			}
			m_ship.calcPoints();
			m_hoverAccelPoint = NULL;
		}
	}

	if ( key == ' ' )
	{
		// toggle playback
		if ( m_uiMode == UI_INERT )
		{
			m_playbackStepIdx = 0;
			m_playbackTimer.start(PLAYBACK_STEP_TIME);
			m_uiMode = UI_PLAYBACK;
		}
		else
		{
			// halt playback
			m_uiMode = UI_INERT;
		}
	}

	if ( key == 116 )
	{
		// save
		FGDataWriter out;
		out.init();

		m_earthPath.save(out);
		m_marsPath.save(out);
		m_ship.save(out);

		FGData *toSave = out.getData();
		getFileSystem()->putFile("path.sav", toSave);
		delete toSave;

		m_msg.set("Saved");
	}
	if ( key == 117 )
	{
		// load
		load("path.sav");
	}
}

void OBEngine::load(const char *filename)
{
	FGData *inData = getFileSystem()->getFile(filename);
	if ( inData == NULL ) return;

	FGDataReader in;
	in.init(inData);

	m_earthPath.load(in);
	m_marsPath.load(in);
	m_ship.load(in);
	delete inData;

	m_msg.set("Loaded ");
	m_msg.add(filename);

	m_uiMode = UI_INERT;

	// note the angle diff between Mars and Earth
	double angleE = m_earthPath.m_startPos.getAngle();
	double angleM = m_marsPath.m_startPos.getAngle();
	double diff = FGDoubleGeometry::angleDiff(angleE, angleM);
	int a=5;
}

void OBEngine::onDrawSelf(FGGraphics &g)
{
	if ( m_uiMode == UI_PLAYBACK )
	{
		drawPlayback(g);
	}
	else
	{
		drawNormal(g);
	}
}

void OBEngine::drawPathObject(FGGraphics &g, Path *toDraw, int pointIdx)
{
	// dont' draw past the stop point
	int stopPoint = toDraw->getStopPoint();
	if ( pointIdx > stopPoint ) return;

	int x = modelToViewX(toDraw->m_points[pointIdx].m_fixX);
	int y = modelToViewY(toDraw->m_points[pointIdx].m_fixY);
	g.fillRect(x-2, y-2, 5, 5);
}

void OBEngine::drawPlayback(FGGraphics &g)
{
	// draw
	g.setColor(0);
	g.fillRect(0, 0, m_screenW, m_screenH);

	// draw the traces
	m_sun.drawSelf(g);
	if ( m_bShowVenus )
	{
		m_venusPath.drawSelf(g, NULL);
	}
	m_earthPath.drawSelf(g, NULL);
	m_marsPath.drawSelf(g, NULL);


	m_ship.drawProgressivePath(g, m_playbackStepIdx);

	// draw the objects
	if ( m_bShowVenus )
	{
		g.setColor(m_venus.m_color);
		drawPathObject(g, &m_venusPath, m_playbackStepIdx);
	}

	g.setColor(m_earth.m_color);
	drawPathObject(g, &m_earthPath, m_playbackStepIdx);

	g.setColor(m_mars.m_color);
	drawPathObject(g, &m_marsPath, m_playbackStepIdx);

	g.setColor(m_ship.m_color);
	drawPathObject(g, &m_ship, m_playbackStepIdx);

	// note the day and sol
	FGString out;
	out.set("Mission day: ");
	int missionDay = m_playbackStepIdx+1;
	out.add(missionDay);
	if ( missionDay >= 133 )
	{
		// work out the sol
		double sol = (double)(missionDay-132);
		sol *= 86400.0;
		sol /= 88775.24409;
		int intSol = (int)sol;
		out.add("\nSol: ");
		out.add(intSol);
	}

	m_font.drawText(g, out.getNativeString(), 0, 0, m_screenW);
}

void OBEngine::drawNormal(FGGraphics &g)
{
	// draw
	g.setColor(0);
	g.fillRect(0, 0, m_screenW, m_screenH);

	m_sun.drawSelf(g);
	if ( m_bShowVenus )
	{
		m_venusPath.drawSelf(g, NULL);
	}

	m_earthPath.drawSelf(g, NULL);
	m_marsPath.drawSelf(g, NULL);

	// ship
	m_ship.drawSelf(g, m_hoverAccelPoint);

	// output
	bool bShowPlanets = false;
	if ( m_hoverPathPointIdx != -1 )
	{
		g.setColor(0xff0000);
		m_ship.drawThrustLine(g, m_hoverPathPointIdx);

		// also note the day
		int daynum = (m_hoverPathPointIdx*86400)/(int)POINTS_TIME + 1;
		FGString out;
		out.set("Day ");
		out.add(daynum);

		// report distances
		int emDist = (int)FGDoubleGeometry::getDistance(m_earthPath.m_points[m_hoverPathPointIdx], m_marsPath.m_points[m_hoverPathPointIdx]);
		int ehDist = (int)FGDoubleGeometry::getDistance(m_earthPath.m_points[m_hoverPathPointIdx], m_ship.m_points[m_hoverPathPointIdx]);
		int mhDist = (int)FGDoubleGeometry::getDistance(m_marsPath.m_points[m_hoverPathPointIdx], m_ship.m_points[m_hoverPathPointIdx]);
		out.add("\nE-M Dist: ");
		addDistInfo(out, emDist);
		out.add("\nE-H Dist: ");
		addDistInfo(out, ehDist);
		out.add("\nH-M Dist: ");
		addDistInfo(out, mhDist);
		m_font.drawText(g, out.getNativeString(), 0, 0, m_screenW);

		bShowPlanets = true;
	}

	if ( m_uiMode == UI_ADJUSTINGMARS ) bShowPlanets = true;
	if ( isKeyDown(17) ) bShowPlanets = true;

	if ( bShowPlanets )
	{
		int idx = m_hoverPathPointIdx;
		if ( idx == -1 )
		{
			// just show the start point
			idx = 0;
		}

		// draw planets
		if ( m_bShowVenus )
		{
			g.setColor(m_venus.m_color);
			drawPathObject(g, &m_venusPath, m_hoverPathPointIdx);
		}

		g.setColor(m_earth.m_color);
		drawPathObject(g, &m_earthPath, m_hoverPathPointIdx);

		g.setColor(m_mars.m_color);
		drawPathObject(g, &m_marsPath, m_hoverPathPointIdx);
	}

	// message
	if ( m_msg.length() > 0 )
	{
		m_font.setJustify(FGFont::JUSTIFY_CENTER);
		m_font.drawText(g, m_msg.getNativeString(), 0, 0, m_screenW);
		m_font.setJustify(FGFont::JUSTIFY_LEFT);
	}
}

void OBEngine::addDistInfo(FGString &str, int dist)
{
	int lightSeconds = dist/300000;
	int lightMinutes = lightSeconds/60;
	lightSeconds = lightSeconds%60;

	str.add(dist);
	str.add("km, ");
	str.add(lightMinutes);
	str.add("m ");
	str.add(lightSeconds);
	str.add("s");
}

void OBEngine::onTick()
{
	if ( m_uiMode == UI_PLAYBACK )
	{
		// do playback and nothing else
		if ( m_playbackTimer.isOver() )
		{
			m_playbackStepIdx+=2;

			int stopIdx = m_ship.getStopPoint();
			if ( m_playbackStepIdx > stopIdx )
			{
				m_playbackStepIdx = stopIdx;
			}
			m_playbackTimer.start(PLAYBACK_STEP_TIME);
		}
	}

	int mx = getMouseX();
	int my = getMouseY();

	if ( m_uiMode == UI_ADJUSTINGPOINT )
	{
		if ( m_hoverAccelPoint == NULL ) return;
		m_ship.adjustAccelerationPoint(m_hoverAccelPoint, mx, my, m_hoverAccelPoint->m_mag);
		m_ship.calcPoints();
	}
	else if ( m_uiMode == UI_ADJUSTINGMARS)
	{
		// place mars's startling position. 
		// First get the angle from the center
		FGDoubleVector mouse;
		mouse.setXY(mx-m_screenW/2, my-m_screenH/2);
		double angle = mouse.getAngle();

		// note mars's position and velocity at that angle
		FGDoubleVector newPos;
		FGDoubleVector newVel;
		m_mars.m_orbit.getPos(angle, newPos);
		m_mars.m_orbit.getVel(angle, newVel);

		// set it
		m_marsPath.m_startPos.set(newPos);
		m_marsPath.m_startVel.set(newVel);

		// recalc
		m_marsPath.calcPoints();
	}
	else
	{
		if ( m_uiMode == UI_ADDINGPOINT )
		{
			m_hoverPathPointIdx = m_ship.getNearestPointIdx(mx, my);
		}
		else
		{
			m_hoverAccelPoint = m_ship.getNearestAccelPoint(mx, my);
		}
	}
}

void OBEngine::onPause()
{
}

void OBEngine::onResume()
{
}

void OBEngine::onExitApp()
{
}

void OBEngine::onMousePressed(int button)
{
	if ( m_uiMode == UI_ADDINGPOINT ) return;

	if ( isKeyDown(17) ) // ctrl
	{
		m_uiMode = UI_ADJUSTINGMARS;
	}
	else 
	{
		m_uiMode = UI_ADJUSTINGPOINT;
	}
}

void OBEngine::onMouseReleased(int button)
{
	if ( (m_uiMode == UI_ADDINGPOINT) && (m_hoverPathPointIdx != -1) )
	{
		// time to add a point
		AccelerationPoint *newPoint = m_ship.createAccelerationPoint(m_hoverPathPointIdx);
		m_ship.calcPoints();
	}
	m_uiMode = UI_INERT;
}

void OBEngine::onMouseWheel(int delta)
{
}

void OBEngine::onFileDrop(const char *filePath)
{
	// load that file
	load(filePath);
}

int OBEngine::modelToViewX(double modelX)
{
	// offset to the centerpoint
	double x = modelX - m_center.m_fixX;

	// scale to the screen
	x /= m_kmPerPixel;
	int ret = (int)x;

	// offset to center it
	ret += m_screenW/2;
	return ret;
}

int OBEngine::modelToViewY(double modelY)
{
	// offset to the centerpoint
	double y = modelY - m_center.m_fixY;

	// scale to the screen
	y /= m_kmPerPixel;
	int ret = (int)y;

	// offset to center it
	ret += m_screenH/2;
	return ret;
}

// global helpers
bool fnear(double a, double b, double slop)
{
	double diff = fabs(a-b);
	if ( diff < slop ) return true;
	return false;
}
