/*******************************************************************************
 * Call polyCub_iso from polyCubAPI.h for a specific intrfr function
 *
 * Copyright (C) 2017 Sebastian Meyer
 *
 * This file is part of the R package "surveillance",
 * free software under the terms of the GNU General Public License, version 2,
 * a copy of which is available at http://www.R-project.org/Licenses/.
 ******************************************************************************/

#include <math.h>
#include <polyCubAPI.h>


/*** C-implementation of "intrfr" functions ***/

// power-law kernel
static double intrfr_powerlaw(double R, double *logpars)
{
    double sigma = exp(logpars[0]);
    double d = exp(logpars[1]);
    if (d == 1.0) {
        return R - sigma * log1p(R/sigma);
    } else if (d == 2.0) {
        return log1p(R/sigma) - R/(R+sigma);
    } else {
        return (R*pow(R+sigma,1.0-d) - (pow(R+sigma,2.0-d) - pow(sigma,2.0-d))/(2.0-d)) / (1.0-d);
    }
}

static double intrfr_powerlaw_dlogsigma(double R, double *logpars)
{
    double newlogpars[2] = {logpars[0], log1p(exp(logpars[1]))};
    // sigma*d = exp(logsigma+logd)
    return -exp(logpars[0]+logpars[1]) * intrfr_powerlaw(R, newlogpars);
}

static double intrfr_powerlaw_dlogd(double R, double *logpars)
{
    double sigma = exp(logpars[0]);
    double d = exp(logpars[1]);
    if (d == 1.0) {
        return sigma * logpars[0] * (1.0-logpars[0]/2.0) - log(R+sigma) * (R+sigma) +
            sigma/2.0 * pow(log(R+sigma),2.0) + R;
    } else if (d == 2.0) {
        return (-log(R+sigma) * ((R+sigma)*log(R+sigma) + 2.0*sigma) +
                (R+sigma)*logpars[0]*(logpars[0]+2.0) + 2.0*R) / (R+sigma);
    } else {
        return (pow(sigma,2.0-d) * (logpars[0]*(-d*d + 3.0*d - 2.0) - 2.0*d + 3.0) +
                pow(R+sigma,1.0-d) * (log(R+sigma)*(d-1.0)*(d-2.0) * (R*(d-1.0) + sigma) +
                                      R*(d*d+1.0) + 2.0*d*(sigma-R) - 3.0*sigma)
                ) * d / pow(d-1.0,2.0) / pow(d-2.0,2.0);
    }
}


/*** function to be called from R ***/

void C_siaf_polyCub1_iso(
    double *x, double *y,  // vertex coordinates (open)
    int *L,                // number of vertices
    int *intrfr_code,      // F(R) identifier
    double *pars,          // parameters for F(R)
    int *subdivisions, double *epsabs, double *epsrel, // Rdqags options
    int *stop_on_error,
    double *value, double *abserr, int *neval) // results
{
    intrfr_fn intrfr;
    switch(*intrfr_code) { // = INTRFR_CODE in ../R/twinstim_siaf_polyCub_iso.R
    case 10: intrfr = intrfr_powerlaw; break;
    case 11: intrfr = intrfr_powerlaw_dlogsigma; break;
    case 12: intrfr = intrfr_powerlaw_dlogd; break;
    }
    double center_x = 0.0;
    double center_y = 0.0;
    polyCub_iso(x, y, L, intrfr, pars, &center_x, &center_y,
                subdivisions, epsabs, epsrel, stop_on_error,
                value, abserr, neval);
    return;
}
