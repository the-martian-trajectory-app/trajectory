#ifndef __OBGLOBALS__
#define __OBGLOBALS__

// NOTE: All units in this simulation are kg, km, and seconds

// SGPs are in km^3/s^2
#define SUN_SGP (132712440018.0) 


// distances are in km
#define VENUS_APOGEE  (108942109.0)
#define EARTH_APOGEE  (152098232.0)
#define MARS_APOGEE   (249209300.0)

// velocities are in km/s
#define VENUS_APOGEE_VEL (35.02)
#define EARTH_APOGEE_VEL (29.3)
#define MARS_APOGEE_VEL (21.97)

// angles are in radians
// AOP = Argument of Periapsis
#define VENUS_AOP (0.9581) // 54.9 degrees
#define EARTH_AOP (1.99330) // 114.20783 degrees
#define MARS_AOP (4.9997) // 286.4623 degrees

// how much time is in a tick, in seconds
// some helpful values:
// One day:  86400.0 
// half-day: 43200.0 
// hour:     3600.0
#define TICK_SECONDS (43200.0) 

// how much acceleration the ship has. This is in km/s^2
#define SHIP_ACC (0.0000001)

// pi
#define PI    (3.1415926535)
#define TWOPI (6.2831853071)

// helper functions
bool fnear(double a, double b, double slop = 0.001);

#endif

