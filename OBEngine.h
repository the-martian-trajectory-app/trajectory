#ifndef __ACENGINE__
#define __ACENGINE__

#include "FGEngine.h"
#include "FGFont.h"
#include "FGDoubleVector.h"
#include "FGTimer.h"

#include "OBGlobals.h"
#include "OBObject.h"
#include "Path.h"

#define UI_INERT 0
#define UI_ADDINGPOINT 1
#define UI_ADJUSTINGPOINT 2
#define UI_ADJUSTINGMARS 3
#define UI_PLAYBACK 4

class OBEngine : public FGEngine
{
public:

	OBEngine();
	virtual ~OBEngine();
	virtual void init();
	virtual void onKeyPressed(int key);
	virtual void onKeyReleased(int key);
	virtual void onDrawSelf(FGGraphics &g);
	virtual void onTick();
	virtual void onPause();
	virtual void onResume();
	virtual void onExitApp();
	virtual void onMousePressed(int button);
	virtual void onMouseReleased(int button);
	virtual void onMouseWheel(int delta);
	virtual void onFileDrop(const char *filePath);
	virtual const char *getTitle() { return "Orbits"; }
	virtual int getForcedScreenWidth() { return 768; }
	virtual int getForcedScreenHeight() { return 768; }

	int modelToViewX(double modelX);
	int modelToViewY(double modelY);

	void drawPlayback(FGGraphics &g);
	void drawNormal(FGGraphics &g);
	void drawPathObject(FGGraphics &g, Path *toDraw, int pointIdx);
	void addDistInfo(FGString &str, int dist);

	void load(const char *filename);

	// font
	FGFont m_font;

	// scale and translation
	FGDoubleVector m_center; // this x,y location will be centered on screen
	double m_kmPerPixel;

	// cached for perf
	int m_screenW;
	int m_screenH;

	// bodies
	OBObject m_sun;
	OBObject m_venus;
	OBObject m_earth;
	OBObject m_mars;
	Path m_ship;

	// paths
	Path m_venusPath;
	Path m_earthPath;
	Path m_marsPath;

	// UI stuff
	int m_uiMode; // a UI_XXXX constant
	int m_hoverPathPointIdx;
	AccelerationPoint *m_hoverAccelPoint;
	FGString m_msg;
	bool m_bShowVenus;

	// playback stuff
	FGTimer m_playbackTimer;
	int m_playbackStepIdx;
};

#endif
