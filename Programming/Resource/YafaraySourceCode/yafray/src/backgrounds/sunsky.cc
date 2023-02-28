
#include "sunsky.h"
// sunsky, from 'A Practical Analytic Model For DayLight" by Preetham, Shirley & Smits.
// http://www.cs.utah.edu/vissim/papers/sunsky/
// based on the actual code by Brian Smits
// and a thread on gamedev.net on skycolor algorithms

using namespace std;

__BEGIN_YAFRAY

background_t *constBackground_t::factory(paramMap_t &params,renderEnvironment_t &render)
{
	color_t color(1.0);
	params.getParam("color", color);
	return new constBackground_t(color);
}

sunskyBackground_t::sunskyBackground_t(const point3d_t dir, PFLOAT turb,
		PFLOAT a_var, PFLOAT b_var, PFLOAT c_var, PFLOAT d_var, PFLOAT e_var)
{
	sunDir.set(dir.x, dir.y, dir.z);
	sunDir.normalize();
	thetaS = acos(sunDir.z);
	theta2 = thetaS*thetaS;
	theta3 = theta2*thetaS;
	phiS = atan2(sunDir.y, sunDir.x);
	T = turb;
	T2 = turb*turb;
	double chi = (4.0 / 9.0 - T / 120.0) * (M_PI - 2.0 * thetaS);
	zenith_Y = (4.0453 * T - 4.9710) * tan(chi) - 0.2155 * T + 2.4192;
	zenith_Y *= 1000;  // conversion from kcd/m^2 to cd/m^2

	zenith_x =	( 0.00165*theta3 - 0.00375*theta2 + 0.00209*thetaS)* T2 +
			(-0.02903*theta3 + 0.06377*theta2 - 0.03202*thetaS + 0.00394) * T +
			( 0.11693*theta3 - 0.21196*theta2 + 0.06052*thetaS + 0.25886);

	zenith_y =	( 0.00275*theta3 - 0.00610*theta2 + 0.00317*thetaS)* T2 +
			(-0.04214*theta3 + 0.08970*theta2 - 0.04153*thetaS + 0.00516) * T +
			( 0.15346*theta3 - 0.26756*theta2 + 0.06670*thetaS + 0.26688);

	perez_Y[0] = ( 0.17872 * T - 1.46303) * a_var;
	perez_Y[1] = (-0.35540 * T + 0.42749) * b_var;
	perez_Y[2] = (-0.02266 * T + 5.32505) * c_var;
	perez_Y[3] = ( 0.12064 * T - 2.57705) * d_var;
	perez_Y[4] = (-0.06696 * T + 0.37027) * e_var;

	perez_x[0] = (-0.01925 * T - 0.25922) * a_var;
	perez_x[1] = (-0.06651 * T + 0.00081) * b_var;
	perez_x[2] = (-0.00041 * T + 0.21247) * c_var;
	perez_x[3] = (-0.06409 * T - 0.89887) * d_var;
	perez_x[4] = (-0.00325 * T + 0.04517) * e_var;

	perez_y[0] = (-0.01669 * T - 0.26078) * a_var;
	perez_y[1] = (-0.09495 * T + 0.00921) * b_var;
	perez_y[2] = (-0.00792 * T + 0.21023) * c_var;
	perez_y[3] = (-0.04405 * T - 1.65369) * d_var;
	perez_y[4] = (-0.01092 * T + 0.05291) * e_var;
};


double sunskyBackground_t::PerezFunction(const double *lam, double theta, double gamma, double lvz) const
{
  double e1=0, e2=0, e3=0, e4=0;
  if (lam[1]<=230.)
    e1 = exp(lam[1]);
  else
    e1 = 7.7220185e99;
  if ((e2 = lam[3]*thetaS)<=230.)
    e2 = exp(e2);
  else
    e2 = 7.7220185e99;
  if ((e3 = lam[1]/cos(theta))<=230.)
    e3 = exp(e3);
  else
    e3 = 7.7220185e99;
  if ((e4 = lam[3]*gamma)<=230.)
    e4 = exp(e4);
  else
    e4 = 7.7220185e99;
  double den = (1 + lam[0]*e1) * (1 + lam[2]*e2 + lam[4]*cos(thetaS)*cos(thetaS));
  double num = (1 + lam[0]*e3) * (1 + lam[2]*e4 + lam[4]*cos(gamma)*cos(gamma));
  return (lvz * num / den);
}


double sunskyBackground_t::AngleBetween(double thetav, double phiv) const
{
  double cospsi = sin(thetav) * sin(thetaS) * cos(phiS-phiv) + cos(thetav) * cos(thetaS);
  if (cospsi > 1)  return 0;
  if (cospsi < -1) return M_PI;
  return acos(cospsi);
}

color_t sunskyBackground_t::operator() (const vector3d_t &dir, renderState_t &state, bool filtered) const
{
	vector3d_t Iw = dir;
	Iw.normalize();

	double theta, phi, hfade=1, nfade=1;

	color_t skycolor(0.0);

	theta = acos(Iw.z);
	if (theta>(0.5*M_PI)) {
		// this stretches horizon color below horizon, must be possible to do something better...
		// to compensate, simple fade to black
		hfade = 1.0-(theta*M_1_PI-0.5)*2.0;
		hfade = hfade*hfade*(3.0-2.0*hfade);
		theta = 0.5*M_PI;
	}
	// compensation for nighttime exaggerated blue
	// also simple fade out towards zenith
	if (thetaS>(0.5*M_PI)) {
		if (theta<=0.5*M_PI) {
			nfade = 1.0-(0.5-theta*M_1_PI)*2.0;
			nfade *= 1.0-(thetaS*M_1_PI-0.5)*2.0;
			nfade = nfade*nfade*(3.0-2.0*nfade);
		}
	}

	if ((Iw.y==0.0) && (Iw.x==0.0))
		phi = M_PI*0.5;
	else
		phi = atan2(Iw.y, Iw.x);

	double gamma = AngleBetween(theta, phi);
	// Compute xyY values
	double x = PerezFunction(perez_x, theta, gamma, zenith_x);
	double y = PerezFunction(perez_y, theta, gamma, zenith_y);
	// Luminance scale 1.0/15000.0
	double Y = 6.666666667e-5 * nfade * hfade * PerezFunction(perez_Y, theta, gamma, zenith_Y);

	// conversion to RGB, from gamedev.net thread on skycolor computation
	double X = (x / y) * Y;
	double Z = ((1.0 - x - y) / y) * Y;

	skycolor.set((3.240479 * X - 1.537150 * Y - 0.498535 * Z),
							(-0.969256 * X + 1.875992 * Y + 0.041556 * Z),
							( 0.055648 * X - 0.204043 * Y + 1.057311 * Z));
  return skycolor;
}


background_t *sunskyBackground_t::factory(paramMap_t &params,renderEnvironment_t &render)
{
	point3d_t dir(1,1,1);	// same as sunlight, position interpreted as direction
	CFLOAT turb = 4.0;	// turbidity of atmosphere
	bool add_sun = false;	// automatically add real sunlight
	PFLOAT pw = 1.0;	// sunlight power
	PFLOAT av, bv, cv, dv, ev;
	av = bv = cv = dv = ev = 1.0;	// color variation parameters, default is normal

	params.getParam("from", dir);
	params.getParam("turbidity", turb);

	// new color variation parameters
	params.getParam("a_var", av);
	params.getParam("b_var", bv);
	params.getParam("c_var", cv);
	params.getParam("d_var", dv);
	params.getParam("e_var", ev);

	// create sunlight with correct color and position?
	params.getParam("add_sun", add_sun);
	params.getParam("sun_power", pw);

	background_t * new_sunsky = new sunskyBackground_t(dir, turb, av, bv, cv, dv, ev);
	/*
	if (add_sun) {
		color_t suncol = (*new_sunsky)(vector3d_t(dir.x, dir.y, dir.z));
		light_t * sunsky_SUN = new sunLight_t(dir, suncol, pw, true);

		string name = "sunsky_SUN";
		// if light already in table with same name, append 'X' until unique
		while (light_table.find(name)!=light_table.end())
			name += "X";

		light_table[name] = sunsky_SUN;
		INFO << "(background_sunsky) Added sunlight " << name << endl;
	}
*/
	return new_sunsky;
}

extern "C"
{
	
YAFRAYPLUGIN_EXPORT void registerPlugin(renderEnvironment_t &render)
{
	render.registerFactory("constant",constBackground_t::factory);
	render.registerFactory("sunsky",sunskyBackground_t::factory);
	
	std::cout<<"Registered sunsky\n";
}

}
__END_YAFRAY
