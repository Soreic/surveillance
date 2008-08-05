/*******************************************************************
 * Author: Michael H�hle <hoehle@stat.uni-muenchen.de>
 *         Mathias Hofmann
 *         Volker Schmid
 * Date:   April 2005 *
 *
 * Markov Chain Monte Carlo (MCMC) estimation in the Branching Process
 * like Epidemic Model. Instead of a slow R solution this code
 * provides a faster C++ solution. Can be invoked through R or be
 * programmed as a librrary. This code uses the Gnu Scientific Library
 * (GSL) available from http://sources.redhat.com/gsl/
 *
 * For now this code is quick & dirty. A more OO framework would be nice
 * to enable better programming, but this will probably be speedwise slower.
 *******************************************************************/

//#include <iostream.h>
#include <iostream.h>
#include <fstream.h>
//#include "twins.hh"

#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

/* removing GSL code
#include <gsl/gsl_math.h>
#include <gsl/gsl_randist.h>
#include <gsl/gsl_rng.h>

//My own gamma distribution
//#include "gamma.h"
#include "gsl_hoehle.h"

#define PI 3.141592654

//The random generator
gsl_rng *r;

*/

/* Replaced GSL library with R internal functions */
#include <R.h>
#include <Rmath.h>

//////////////////////////////////
//Globals
/////////////////////////////////


/* new definitions to replace GSL code */
int r;

double gsl_rng_uniform (int RNG) {
  GetRNGstate();
  double res = runif(0,1);
  PutRNGstate();
  return(res);
}

double gsl_ran_gaussian(int RNG, double sigma) {
  GetRNGstate();
  double res = rnorm(0.0,sigma);
  PutRNGstate();
  return(res);
}

double gsl_ran_gamma(int RNG, double a, double b) {
  GetRNGstate();
  double res = rgamma(a,b);
  PutRNGstate();
  return(res);
}

double gsl_ran_poisson(int RNG, double lambda) {
  GetRNGstate();
  double res = rpois(lambda);
  PutRNGstate();
  return(res);
}


double gsl_ran_binomial(int RNG, double p, unsigned int n) {
  GetRNGstate();
  double res = rbinom(n,p);
  PutRNGstate();
  return(res);
}

//hoehle: The original function assumes mu>0, which needs not be the case!
//This version handles that part. This is the log version.

double
gsl_ran_poisson_log_pdf (const unsigned int k, const double mu)
{
  double p;
  if (mu==0) {
    return(log(k == 0));
  } else {
    double lf = lgammafn(k+1); /*gsl2R: gsl_sf_lnfact(k) */

    p = k*log(mu) - lf - mu;
    return p;
  }
}

double gsl_sf_lngamma(double x) {
  return(lgammafn(x));
}

double gsl_ran_beta_pdf (double x, double a, double b) {
  return(dbeta(x,a,b,0));
}

/**********************************************************************
 * Log version of the Gamma pdf with mean a*b and variance a*b^2.
 *
 **********************************************************************/

double
gsl_ran_gamma_log_pdf (const double x, const double a, const double b)
{
  if (x < 0)
    {
      //This is problematic!
      return log(0) ;
    }
  else if (x == 0)
    {
      if (a == 1)
        return log(1/b) ;
      else
        return log(0) ;
    }
  else if (a == 1)
    {
      return -x/b - log(b) ;
    }
  else 
    {
      double p;
      /*gsl2R:      double lngamma = gsl_sf_lngamma (a);*/
      double lngamma = lgammafn(a);
      p = (a-1)*log(x) - x/b - lngamma - a*log(b);
      return p;
    }
}





//Setup params
long seed;
int overdispersion;
int varnu;
int la_rev;
int K_geom;
int la_estim;
int nu_trend;
int theta_pred_estim;
int xi_estim;
int delta_rev;
int xi_estim_delta;
int epsilon_rev;
int xi_estim_epsilon;
int xi_estim_psi;
double psiRWSigma = 0.25;
double xRWSigma = 0.25;
double taubetaRWSigma = 0.25;

//Priors
double alpha_lambda = 1.0;
double beta_lambda = 1.0;

double alpha_xi = 1.0;
double beta_xi = 1.0;

double p_K = 1.0;

double alpha_nu = 1.0;
double beta_nu = 1.0;

double alpha_psi = 1.0;
double beta_psi = 10.0;


  double alpha_a=1;
  double alpha_b=0.001;
  double beta_a=1.0;
  double beta_b=.00001;
  double gamma_a=1;
  double gamma_b=0.001;
  double delta_a=1;
  double delta_b=0.001;
  double epsilon_a=1;
  double epsilon_b=0.001;


double* my;
double* my2;
double* temp;
double* z;
double* theta;
double* beta0;
double* Q;
double* Q2;
double* L;
double* L2;
double* P;
double* P2;
double* gammaalt;
double* z2;
int n1;
int n2;





/*********************************************************************
 * Compute sum from 1 to I and 1 to n of a vektor with indices 0,...,I 
 * of a vektor with indices 0,...,n
 * Parameters:
 *
 * X a vector with indices 0,..,I of a vector with indices 0,...,n
 * I "length" of vector (true length due to zero indice is I+1)
 *********************************************************************/
double sumIn(long** X, long I, long n) {
  double res = 0;
  for (register long i=1; i<=I; i++){
    for (register long t=1; t<=n; t++) {
      res += X[i][t]; 
    }
  }
  return(res);
}


/*********************************************************************
 * Compute sum from 1 to I and 1 to n of a vektor with indices 0,...,I 
 * of a vektor with indices 0,...,n
 * This is the double version
 * Parameters:
 *
 * X a vector with indices 0,..,I of a vector with indices 0,...,n
 * I "length" of vector (true length due to zero indice is I+1)
 *********************************************************************/
double sumIn(double** X, long I, long n) {
  double res = 0;
  for (register long i=1; i<=I; i++){
    for (register long t=1; t<=n; t++) {
      res += X[i][t]; 
    }
  }
  return(res);
}

/*********************************************************************
 * Compute sum from 1 to I and 1 to n of a vektor with indices 0,...,I 
 * of a vektor with indices 0,...,n
 * Parameters:
 *
 * X a vector with indices 0,..,I of a vector with indices 0,...,n
 * I "length" of vector (true length due to zero indice is I+1)
 *********************************************************************/
double sumIn2(long** X, long I, long n) {
  double res = 0;
  for (register long i=1; i<=I; i++){
    for (register long t=2; t<=n; t++) {
      res += X[i][t]; 
    }
  }
  return(res);
}


/*********************************************************************
 * Compute sum from 1 to I and 1 to n of a vektor with indices 0,...,I 
 * of a vektor with indices 0,...,n
 * This is the double version
 * Parameters:
 *
 * X a vector with indices 0,..,I of a vector with indices 0,...,n
 * I "length" of vector (true length due to zero indice is I+1)
 *********************************************************************/
double sumIn2(double** X, long I, long n) {
  double res = 0;
  for (register long i=1; i<=I; i++){
    for (register long t=2; t<=n; t++) {
      res += X[i][t]; 
    }
  }
  return(res);
}


/*********************************************************************
 * Compute sum from 1 to I of a vektor with indices 0,...,I 
 * of a vektor with indices 0,...,n
 * Parameters:
 *
 * X a vector with indices 0,..,I of a vector with indices 0,...,n
 * I "length" of vector (true length due to zero indice is I+1)
 *********************************************************************/
double sumI1(long** X, long I, long t) {
  double res = 0;
  for (register long i=1; i<=I; i++) { res += X[i][t]; }
  return(res);
}

/*********************************************************************
 * Compute sum from 1 to I of a vektor with indices 0,...,I 
 * of a vektor with indices 0,...,n
 * This is the double version
 * Parameters:
 *
 * X a vector with indices 0,..,I of a vector with indices 0,...,n
 * I "length" of vector (true length due to zero indice is I+1)
 *********************************************************************/
double sumI1(double** X, long I, long t) {
  double res = 0;
  for (register long i=1; i<=I; i++) { res += X[i][t]; }
  return(res);
}

/*********************************************************************
 * factorial function
 *********************************************************************/
long factorial(long x){
  long fac=1;
  if(x<0){cout<<"negative value passed to factorial function" << endl; exit(-1);}
  else{
    if(x=0){fac=1;}
    else{
      for(int i=1;i<=x;i++){
        fac*=i;
      }
    }
  }
  return(fac);
}

/*********************************************************************
 * logit function
 *********************************************************************/
double logit(double y){
  if(y <= 0 || y >= 1){
    cout << "y <= 0 or y >= 1 in logit function." << endl;
    exit(-1);
  }
  double logit;
  logit = log(y/(1-y));
    return(logit);
}




/*********************************************************************
 * inverse logit function
 *********************************************************************/
double invlogit(double y){
  double invlogit;
  invlogit = 1/(1 + exp(-y));
    return(invlogit);
}


/*********************************************************************
 * inverse logit function diff.
 *********************************************************************/
double invlogitd(double y){
  double invlogitd;
  invlogitd = exp(-y)/pow((1.0 + exp(-y)),2);
    return(invlogitd);
}




/*********************************************************************
 * Makes one Metropolis-Hastings update step, log-scale
 *********************************************************************/
double updateMHlog(double &par, double parStar, double logFpar, double logFparStar, double &acceptedpar) {
  double accpar = exp(logFparStar - logFpar); 
  if (gsl_rng_uniform (r) <= accpar) {par = parStar; acceptedpar++;}
  return(0);
}

/*********************************************************************
 * Makes one Metropolis-Hastings update step
 *********************************************************************/
double updateMH(double &par, double parStar, double Fpar, double FparStar, double &acceptedpar) {
  double accpar = FparStar/Fpar; 
  if (gsl_rng_uniform (r) <= accpar) {par = parStar; acceptedpar++;}
  return(0);
}






/*********************************************************************
 * Tunes a parameter
 *********************************************************************/
double tune(double& parameter, double accepted, double samples, double& tunepar, double a=0.3, double b=0.4){
      tunepar=1;     
      if ((accepted/samples>a) && (accepted/samples<b)) {
        tunepar=0;
      } else if (accepted/samples>b) {
        parameter *= 1.5;
      } else if (accepted/samples<a) {
        parameter /= 2.0;
      }
      return(0);
}

double ABS(double x)
{
  if (x>0){return x;}else{return -x;}
}
double MIN(double a, double b)
{
  if (a<b){return a;}else{return b;}
}

double xMx(double* Q, double* x, int noa, int b)
{
  double erg=0.0;
  
  for (int i=0; i<noa; i++)
    {
      for (int j=0; j<noa; j++)
	{
	  if (ABS(i-j) < b)
	    {
	      erg += x[i]*x[j]*Q[(int)(MIN(i,j)*b+ABS(i-j))];
	      if (i==j)
		{
		  erg -= 0.5*x[i]*x[j]*Q[(int)(MIN(i,j)*b+ABS(i-j))];
		}
	    }
	}
    }

  return erg;
}

double xMx2(double* Q, double* x, int n, int b)
{
  double erg=0.0;
  
  for (int i=0; i<=n; i++)
    {
      for (int j=0; j<=n; j++)
	{
	  if (ABS(i-j) < b)
	    {
	      erg += x[i]*x[j]*Q[(int)(MIN(i,j)*b+ABS(i-j))];
	    }
	}
    }

  return erg;
}


// BERECHNET A-1, k ist matrixlaenge
void invers(double* A,int k)
{
  //cout << "invers ! K=" << k;

double* ergebnis=new double[k*k];
if (k==1)
  {
    ergebnis[0] = 1.0/A[0];
   
  }
if (k==2)
  {
    double det = A[0]*A[3]-A[1]*A[2];
    ergebnis[0] = A[3]/det;
    ergebnis[1] = 0.0-A[1]/det;
    ergebnis[2] = 0.0-A[2]/det;
    ergebnis[3] = A[0]/det;
  }
if (k>2)
  {
    cout << "PROGRAMMFEHLER ! in invers()";
  }
for (int i=0; i< k*k; i++)
 {
   A[i]=ergebnis[i];
 }

//delete[] ergebnis;
return;

}

void mxschreibe(double* A, int a, int b)
{
for (int i=0; i<a; i++)
  {
    for (int j=0; j<b; j++)
      {
	cout << A[i*b+j] << " ";
      }
    cout << endl;
  }
 cout << endl;
return;
}


int mxcheck(int n, int** matrix)
{
int zs=0;
for (int i=0; i<n; i++)
  {
    zs=0;
    for (int j=0; j<n; j++)
      {
	zs+=matrix[i][j];
	if (matrix[i][j]!=matrix[j][i])
	  {
	    cout << "FEHLER: Matrix nicht symmetrisch ! (Zeile:" << i << ", Spalte:" <<j << endl;
	    return 1;
	  }
      }
    if (zs != 0)
      {
	cout << "FEHLER: Zeilensumme nicht Null in Zeile "<< i << endl;
        return 1;
      }
  }
return 0;
}





//updatealphabeta
//Erzeugt Normalverteilten Zufallsvektor der L�nge noa
void gausssample(double* temp, int noa)
{

for (int i=0; i< noa; i++)
  {
    temp[i]=gsl_ran_gaussian(r,1);
  }

return;
}

double hyper(int rw, double* theta, double k_a, double k_b, int n)
{

//double k_a = 1.0;
//double k_b = 0.00005;

double aa;
double bb;
double kappa_neu=0;

if (rw==1)
{
double summe = 0.0;

aa = k_a + 0.5 * double(n-1-rw);

for (int i=3; i <= n; i++)
{
    summe = summe + (theta[i] - theta[i-1]) * (theta[i] - theta[i-1]);
}
bb = k_b + 0.5 * summe;

kappa_neu = gsl_ran_gamma(r,aa,1/bb);
}
if (rw==2)
{

double dopp_diff;
double summe = 0.0;

aa = k_a + 0.5 * double(n-1-rw);

for (int i=3; i < n; i++)
{
	dopp_diff = theta[i-1] - 2*theta[i] + theta[i+1];
        summe = summe + dopp_diff * dopp_diff;
// 	cout << dopp_diff<<endl;
}
//  cout << theta[0]<<theta[1]<<theta[n]<<theta[n-1]<<endl;
bb = k_b + 0.5 * summe;



kappa_neu = gsl_ran_gamma(r, aa, 1/bb);
}
if (rw==0)
{
double summe = 0.0;

aa = k_a + 0.5 * double(n-1-rw);

for (int i=2; i <= n; i++){
    summe = summe + (theta[i] * theta[i]);
}

bb = k_b + 0.5 * summe;

kappa_neu = gsl_ran_gamma(r,aa,1/bb);
}

return kappa_neu;
}


double update_tau_alpha(double* alpha, long I, double aa, double bb, double* xreg)
{

aa += double(I);
for (int i=1; i<=I; i++)
{
  bb += (alpha[i]-xreg[i])*(alpha[i]-xreg[i]);
}
double neu=gsl_ran_gamma(r, aa, 1/bb);
return neu;
}
double update_tau_gamma(double* alpha, long ncov, double aa, double bb)
{

aa += double(ncov);
for (int i=0; i<ncov; i++)
{
	bb += alpha[i]*alpha[i];
}
double neu=gsl_ran_gamma(r, aa, 1/bb);
return neu;
}

void berechneQ(double* temp, int age_block, double kappa, int noa,int nop, double delta)
{

if (age_block==1)
  {
    int index=0;
    temp[index]=kappa+delta*nop;
    index++;
    temp[index]=-kappa;
    index++;
    for (int i=1; i<noa-1; i++)
      {
	temp[index]=2*kappa+delta*nop;
	index++;
	temp[index]=-kappa;
	index++;
      }
    temp[index]=kappa+delta*nop;
  }
if (age_block==2)
  {
    int index=0;
    temp[index]=kappa+delta*nop;
    index++;
    temp[index]=-2*kappa;
    index++;
    temp[index]=kappa;
    index++;
    temp[index]=5*kappa+delta*nop;
    index++;
    temp[index]=-4*kappa;
    index++;
    temp[index]=kappa;
    index++;
    for (int i=2; i<noa-2; i++)
      {
	temp[index]=6*kappa+delta*nop;
	index++;
	temp[index]=-4*kappa;
	index++;
	temp[index]=kappa;
	index++;
      }
    temp[index]=5*kappa+delta*nop;
    index++;
    temp[index]=-2*kappa;
    index++;
    //temp[index]=0;
    index++;
    temp[index]=kappa+delta*nop;
  }

return;
}

double sumg(int ncov, double** xcov, double* gamma, int t, int scov)
{
  double sum=0;
  for (int i=scov; i<ncov; i++)
    {
      sum += xcov[i][t]*gamma[i];
    }
  return sum;
}




void alphaupdate(double* gamma, double* alpha, double* beta, double* delta, double** lambda, double p, long I, long n, long** Y, long** X, long& acc_alpha, double taualpha, int ncov, double** xcov, double* xreg, double** omega, double** omegaX, int scov, int mode){


for (int i=1; i<=I; i++)
{
  
  double tau=taualpha;
  double my=0;
   for (int t=2; t<=n; t++)
   {
     tau+=omegaX[i][t]*delta[t]*exp(sumg(ncov,xcov,gamma,t,scov)+alpha[i]+beta[t]);
     my+=(X[i][t]);
     my-=(1-alpha[i])*omegaX[i][t]*delta[t]*exp(sumg(ncov,xcov,gamma,t,scov)+alpha[i]+beta[t]);
   }
   my=my+xreg[i]*taualpha;
   my=my/tau;
   double alphaneu=gsl_ran_gaussian(r,sqrt(1/tau))+my;
   double tauneu=taualpha;
   double myneu=0;
   for (int t=2; t<=n; t++)
   {
    	tauneu+=omegaX[i][t]*delta[t]*exp(sumg(ncov,xcov,gamma,t,scov)+alphaneu+beta[t]);
	myneu+=(X[i][t]);
	myneu-=(1-alphaneu)*omegaX[i][t]*delta[t]*exp(sumg(ncov,xcov,gamma,t,scov)+alphaneu+beta[t]);
   }
   myneu=myneu+xreg[i]*taualpha;
   myneu=myneu/tauneu;
   double akzw=0.5*log(tauneu/(2*PI))-0.5*tauneu*(alphaneu-myneu)*(alphaneu-myneu); // log Proposalw. alt|neu
   akzw -= ((0.5*log(tau/(2*PI))-0.5*tau*(alpha[i]-my)*(alpha[i]-my))); // log Proposalw. neu|alt
   akzw += (-0.5*taualpha*(alpha[i]-xreg[i])*(alpha[i]-xreg[i]));
   akzw -= (-0.5*taualpha*(alphaneu-xreg[i])*(alphaneu-xreg[i]));
   for (int t=2; t<=n; t++)
	{
	akzw += (alpha[i]*X[i][t]-omegaX[i][t]*delta[t]*exp(sumg(ncov,xcov,gamma,t,scov)+alpha[i]+beta[t]));
	akzw -= (alphaneu*X[i][t]-omegaX[i][t]*delta[t]*exp(sumg(ncov,xcov,gamma,t,scov)+alphaneu+beta[t]));
	}

   if (exp(akzw) >= gsl_rng_uniform(r))
	{
		alpha[i]=alphaneu;
		acc_alpha += 1;
	}
}
//  double sum=0;
//  for (int i=1; i<=I; i++)
//    {
//      sum+=alpha[i]-xreg[i];
//    }
//  for (int i=1;i<=I; i++)
//    {
//      alpha[i]=alpha[i]-(sum/I);
//    }

return;
}



void erzeuge_b_Q(double* gamma  , double* my, double* Q, double* alpha, double* delta, double* beta, long** X, long** Z, long** Y,
		 long n, long I, double taubeta, int rw, double** lambda, double p, double** xcov, int ncov, double** omega, double** omegaX,int scov, int mode)
{
  if (mode==1)
    {
// b-vektor des Proposals
for (int t=0;t<n; t++)
{
	my[t]=0.0;
	for (int i=1; i<=I; i++)
	{
		my[t]+=X[i][t+2];
		my[t]-=(1-beta[t+2])*omegaX[i][t+2]*delta[t+2]*(exp(sumg(ncov,xcov,gamma,t+2,scov)+alpha[i]+beta[t+2]));
	}
}
    }
 if (mode==2)
    {
// b-vektor des Proposals
for (int t=2;t<=n; t++)
{
	my[t-2]=0.0;
	for (int i=1; i<=I; i++)
	{
		my[t-2]+=Y[i][t];
		my[t-2]-=(1-beta[t])*(omega[i][t]*Z[i][t-1]*exp(sumg(ncov,xcov,gamma,t,scov)+alpha[i]+beta[t]));
	}
}


}

//mxschreibe(my,1,n-1);

// Pr�zisionsmatrix
berechneQ(Q, rw, taubeta, n, 1, 0.0);

// mxschreibe(Q,n-1,rw+1);

 if (mode==1)
   {
     for (int i=1; i<=I; i++)
       {
	 for (int t=0; t<n; t++)
	   {
	     Q[(rw+1)*(t)]+= omegaX[i][t+2]*delta[t+2]*exp(sumg(ncov,xcov,gamma,t+2,scov)+alpha[i]+beta[t+2]);
	          }	
   }
   }

//mxschreibe(Q,n-1,rw+1);

 if (mode==2)
   {
     for (int i=1; i<=I; i++)
       {
	 for (int t=2; t<=n; t++)
	   {
	     Q[(rw+1)*(t-2)]+= omega[i][t]*Z[i][t-1]*exp(sumg(ncov,xcov,gamma,t,scov)+alpha[i]+beta[t]);
// 	     cout << omega[i][t]<<Z[i][t-1]<<endl;
	   }
       }	
   }

return;
   }


void erzeuge_b_Q_2(double* my, double* Q, double* alpha, double* beta, double* gamma, double* delta, long** X,
		 long n, long I, double taubeta, int rw, double** xcov, int ncov, int scov, double** omega)
{

// b-vektor des Proposals
for (int t=0;t<=n; t++)
{
	my[t]=0.0;
	for (int i=1; i<=I; i++)
	{
		my[t]+=X[i][t+2];
		my[t]-=(1-beta[t])*omega[i][t+2]*delta[t+2]*exp(sumg(ncov,xcov,gamma,t+2,scov)+alpha[i]+beta[t]);
	}
}


//mxschreibe(my,1,n-1);

// Pr�zisionsmatrix
berechneQ(Q, rw, taubeta, n+1, 1, 0.0);

// mxschreibe(Q,n+1,rw+1);
// cout << taubeta << endl;
     for (int i=1; i<=I; i++)
       {
	 for (int t=0; t<=n; t++)
	   {
	     Q[(rw+1)*(t)]+= omega[i][t+2]*delta[t+2]*exp(sumg(ncov,xcov,gamma,t+2,scov)+alpha[i]+beta[t]);
           }	
       }


//mxschreibe(Q,n-1,rw+1);

return;
}



// double updatemu(double mu, double* alpha, double* beta, double lambda, double p, long I, long n, long** Y, long** S, long** X, long& acc_mu, int doit){

//   double quot=(lambda*p/(1-lambda));
//   double term1=0.0;
//   for (int i=1; i<=I; i++)
//     {
//       term1 +=Y[i][1];
//       for (int t=1; t<=n; t++)
// 	{
// 	  term1+=(X[i][t]+S[i][t]);
// 	}
//     }

// //     double XSum = sumIn(X,I,n);
// //     double SSum = sumIn(S,I,n);

// //     double Y1Sum = 0;
// //     for (register long i=1;i<=I; i++) {
// //       Y1Sum = Y1Sum + Y[i][1];
// //     }

// //     double praemu=0.0;//log((XSum+SSum+Y1Sum)/(I*n+I*(lambda*p)/(1-lambda)));
  

// //   double v=0;
// //   for (int i=1; i<=I; i++)
// //     {
// //       v += (quot*exp(mu+alpha[i]+beta[1]));

// //         for (int t=1; t<=n; t++)
// // 	  {
// // 	    v += exp(mu+alpha[i]+beta[t]);
// // 	  }

// //     }  
// //   double m=praemu+(term1-v)/v;
// //   double muneu=gsl_ran_gaussian(r,sqrt(1/v))+m;

// //   double vneu=0;
// //   for (int i=1; i<=I; i++)
// //     {
// //       vneu += quot*exp(muneu+alpha[i]+beta[1]);
// //         for (int t=1; t<=n; t++)
// // 	  {
// // 	    vneu += exp(muneu+alpha[i]+beta[t]);
// // 	  }
// //     }  
// //   double mneu=praemu+(term1-vneu)/vneu;

// //   double akzw=term1*muneu-term1*mu;
 
// //   for (int i=1; i<=I; i++)
// //     {
// //       akzw-=quot*exp(muneu+alpha[i]+beta[1]);
// //       akzw+=quot*exp(mu+alpha[i]+beta[1]);
// //       for (int t=1; t<=n; t++)
// // 	  {
// // 	    akzw -= exp(muneu+alpha[i]+beta[t]);
// // 	    akzw += exp(mu+alpha[i]+beta[t]);  
// // 	  }
// //     }

// //   akzw -= (0.5*log(v)-0.5*v*(muneu-m)*(muneu-m));
// //   akzw += (0.5*log(vneu)-0.5*vneu*(mu-mneu)*(mu-mneu));

 
// //   if (exp(akzw) >= gsl_rng_uniform(r)&&doit>2)
// // 	{
// // 		mu=muneu;
// // 		acc_mu++;
// // 	}
// //     for (int i=0; i<50000000; i++){acc_mu++;}
// //     cout << mu << " " << muneu<< " " << term1 <<" "<<v<<" "<<akzw<< " " << alpha[3]<< " " << beta[25] <<endl;

//   double muneu = mu+gsl_ran_gaussian(r,.05);
//   double akzw=term1*muneu-term1*mu;
 
//   for (int i=1; i<=I; i++)
//     {
//       akzw-=quot*exp(muneu+alpha[i]+beta[1]);
//       akzw+=quot*exp(mu+alpha[i]+beta[1]);
//       for (int t=1; t<=n; t++)
// 	  {
// 	    akzw -= exp(muneu+alpha[i]+beta[t]);
// 	    akzw += exp(mu+alpha[i]+beta[t]);  
// 	  }
//     }

// //     for (int i=0; i<50000000; i++){acc_mu++;}
// //     cout << mu << " " << muneu<< " " << term1 <<" "<<akzw<< " " << alpha[3]<< " " << beta[25] <<endl;

//   if (exp(akzw) >= gsl_rng_uniform(r)&&doit>2)
// 	{
// 		mu=muneu;
// 		acc_mu++;
// 	}

//   return mu;
// }

//mxs.cc
extern "C" {
int gpskca_(int& n,int degree[],int rstart[],int connec[], int& ioptpro, int& worklen, int permut[], int work[], int& bandwidth, int& profile, int& error, int& space);        
int dpbtrf_(char& uplo,int& n,int& bw, double mx_neu[], int& bw1,int& info);
int dtbsv_(char& uplo, char& trans, char& diag, int& n, int& bw, double A[], int& bw1, double z[], int& incx);
}

void loese(double* A, double* z, int &n, int& bw)     
{
	
char uplo='L';
char trans='T';
char diag='N';
int bw1 = bw+1;
int incx=1;


dtbsv_(uplo,trans,diag, n, bw, A, bw1, z, incx);
return;
} 

void loese2(double* A, double* z, int &n, int& bw)     
{
	
char uplo='L';
char trans='N';
char diag='N';
int bw1=bw+1;
int incx=1;


dtbsv_(uplo,trans,diag, n, bw, A, bw1, z, incx);
return;
} 



double* cholesky(int n, double* matrix,  int& bw)
{
// double* mx_neu = new double[bw+1*n];
// for (int i=0; i<n; i++)
//   {
//     mx_neu[i] = new double[n];
//   }

char uplo='L';
// for (int j=0; j < bw+1; j++)
//    {
// 	for (int i=0; i<n-j; i++)
// 	  {
// 	    mx_neu[j][i]=(double)matrix(i+j,i);
// 	  }
//    }

// for (int j=0; j < bw+1; j++)
//    {
// 	for (int i=0; i<n-j; i++)
// 	  {
// 	    mx_neu[j+((bw+1)*i)]=(double)matrix(i+j,i);
// 	  }
//    }
//  cout << endl;
//  for(int a3=0;a3<((bw+1)*n);a3++)
//    {
// //   for(int a4=0;a4<n;a4++)
// //    {
// //      if (mx_neu[a3][a4]>-1)
// //      {
// //      cout << " ";
// //      }
//       cout << mx_neu[a3];
// //    }
//     cout << endl;
//   }

int info=0;
int bw1=bw+1;
dpbtrf_(uplo, n, bw, matrix, bw1, info);

//  cout << "Info: "<< info << endl;


// double** zerlegung = new double*[n];
// for (int i=0; i<n; i++)
//   {
//     zerlegung[i]=new double[n];
//   }

// for (int i=0; i<n; i++)
//   {
//     for (int j=0; j<n; j++)
//       {
// 	zerlegung[i][j]=0.0;
//       }
//   }


// LaGenMatDouble zerlegung(n,0.0);

//    for (int j=0; j<bw+1; j++)
//      {
//        for (int i=0; i<n-j; i++)
//          {
//    	zerlegung[i+j][i]=mx_neu[(bw+1)*i+j];
//          }
//      }

	
return matrix;

}


void update_beta_nurrw(double* gamma, double* alpha, double* beta, double* delta, long** X, long** Z, long** Y, long n, long I, double taubeta, int rw, double p, double** lambda, long& acc_beta, long doit, double* my, double* my2, double* temp, double* z, double* theta, double* Q, double* Q2, double* L, double* L2, double** xcov, int ncov, int scov, double** omega, double** omegaX, int mode)
{
int bandw=rw+1;
int n2=n-1;




 erzeuge_b_Q(gamma, theta, Q, alpha, delta, beta, X, Z, Y, n2, I, taubeta, rw, lambda, p, xcov, ncov, omega, omegaX,scov, mode); //erzeugt b und Q, theta ist b

 // cout << theta[0]<<" " <<Q[0]<< " "<< Q[rw+1]<<endl;
 //  mxschreibe(Q,n2,bandw);

for (int i=0; i< (bandw*n2); i++)
  {
    L[i]=Q[i];
  }

 cholesky(n2, L, rw); // erzeugt L

//  mxschreibe(Q,n2,bandw);
//  mxschreibe(L,n2,bandw);


//Lv=b
 loese2(L, theta, n2, rw); // theta ist v oder w

//L^T my=v
 loese(L,theta, n2, rw); //theta ist mu

 // mxschreibe(beta,1,n+1);
 // mxschreibe(theta,1,n);
//z
gausssample(temp, n2);

 for (int i=0; i<n2; i++)
   {
     z[i]=temp[i];
   }

//L^T y=z
 loese(L,temp,n2, rw); //temp ist y oder v

//x=my + y
for (int i=n+1; i>0; i--)
  {
    theta[i]=theta[i-2]+temp[i-2]; // theta = mu + v
  }

// mxschreibe(theta,1,n+1);
// mxschreibe(z,1,n+1);

// theta ist der vorgeschlagene Vektor

//mxschreibe(beta,1,n+1);
 erzeuge_b_Q(gamma, my2, Q2, alpha, delta, theta, X, Z, Y, n2, I, taubeta, rw, lambda, p, xcov, ncov, omega, omegaX,scov, mode); //erzeugt b^0 und Q^0, my2 ist b^0

 //mxschreibe(beta,1,n+1);

 for (int i=0; i< (bandw*n2); i++)
   {
     L2[i]=Q2[i]; //Q2 ist Q^0
   }
//  mxschreibe(beta,1,n);
//  mxschreibe(theta,1,n);
double akzw=0.0;  // loglik Proposal

 cholesky(n2,L2,rw); //L2 ist L^0

 loese2(L2, my2, n2, rw); //my2 ist jetzt v^0 oder w^0

//L^T my=v
 loese(L2,my2, n2, rw);//my2 ist jetzt mittelwertsvektor,  my2 ist jetzt mu^0

 for (int i=0; i<n2; i++)
   {
     my2[i]=beta[i+2]-my2[i]; //my2 ist jetzt (beta^0 - mu^0)
   }
//  mxschreibe(L,n2,bandw);
//  mxschreibe(my2,1,n2);
//  akzw -= xMx(Q2,my2,n2,bandw); // -1/2*(beta^0 - mu^0)'Q^0(beta^0 - mu^0)
  akzw -= 0.5*xMx2(Q2,my2,n2,bandw);
  //   cout << akzw << endl;
for (int i=0; i<n2; i++)  
{
  akzw -= log(L2[bandw*i]); // - log(L^0_tt)	
	akzw += log(L[bandw*i]); // log(L_tt)
 }
//  cout << akzw << endl;
for (int i=0; i<n2; i++)  
  {
	akzw += 0.5*z[i]*z[i];
}

//  mxschreibe(z,1,n2);

//  cout << akzw << endl;
// Die Loglikelihoods
for (int i=4; i<=n; i++)
{
  akzw += 0.5*taubeta*(beta[i]-2*beta[i-1]+beta[i-2])*(beta[i]-2*beta[i-1]+beta[i-2]); //beta^0' R beta^0
	akzw -= 0.5*taubeta*(theta[i]-2*theta[i-1]+theta[i-2])*(theta[i]-2*theta[i-1]+theta[i-2]); // - beta' R beta
}

// berechneQ(Q, rw, taubeta, n2, 1, 0.0); //Q ist R
// for(int i=0;i<n2;i++){
//   my[i]=theta[i+2];
//   my2[i]=beta[i+2];
//  }
// akzw -= xMx(Q,my,n2,bandw);
// akzw += xMx(Q,my2,n2,bandw);

//  cout << akzw << endl;

 if (mode==1)
   {
 
     for (int i=1; i<=I; i++)
       {
	 double temp;
	 temp=alpha[i];
	 for (int t=2; t<=n; t++)
	   {
	     akzw -= beta[t]*(X[i][t]); // - beta^0 c	     
	     akzw += omegaX[i][t]*delta[t]*exp(temp+beta[t]+sumg(ncov,xcov,gamma,t,scov)); // - h(beta^0)
	     akzw += theta[t]*(X[i][t]); // beta c
	     akzw -= omegaX[i][t]*delta[t]*exp(temp+theta[t]+sumg(ncov,xcov,gamma,t,scov)); // + h(beta)
	   }
       }

   }

 if (mode==2)
   {
     for (int i=1; i<=I; i++)
       {
	 double temp;
	 temp=alpha[i];
	 for (int t=2; t<=n; t++)
	   {
	     akzw -= beta[t]*(Y[i][t]);
	     
	     akzw -= omega[i][t]*Z[i][t-1]*exp(temp+theta[t]+sumg(ncov,xcov,gamma,t,scov));
	     akzw += theta[t]*(Y[i][t]);
	     akzw += omega[i][t]*Z[i][t-1]*exp(temp+beta[t]+sumg(ncov,xcov,gamma,t,scov));
	   }
       }
   }
  
//mxschreibe(beta,1,n+1),
   //mxschreibe(theta,1,n+1);
//  cout << akzw << endl<<endl;

if (exp(akzw)>gsl_rng_uniform(r))
{
	for (int i=2; i<=n; i++)
	{
		beta[i]=theta[i];
	}
// 	mxschreibe(beta,1,n);
	acc_beta++;

}
// cout << "YY:"<<beta[n-1]<<" "<<beta[n]<<endl;
// cout << akzw <<" " << taubeta <<endl;

// if (doit>73)
//   {
//  double sum=0.0;
// 	for (int i=2; i<=n; i++)
// 	{
// 	  sum+=beta[i];
// 	}
// 	for (int i=2; i<=n; i++)
// 	{
// 	  beta[i]=beta[i]-(sum/(n-1));
// 	}
// 	gamma[0]=gamma[0]+(sum/(n-1));
//   }//cout << sum << " ";

return;
}

void update_beta_block( double* alpha, double* beta, double* gamma, double* delta, long** X, long n, long I, double taubeta, int rw, long& acc_beta, long sampleCounter, int n1, int n2, double* my, double* my2, double* z, double* theta, double* beta0, double* Q, double* Q2, double* L, double* L2, double** xcov, int ncov, int scov, double** omega)
{

 for(int t=0;t<=n2;t++){
   beta0[t] = beta[t+2];
 }

 erzeuge_b_Q_2(theta, Q, alpha, beta0, gamma, delta, X, n2, I, taubeta, rw, xcov, ncov, scov, omega); //erzeugt b und Q, theta ist b

 //  mxschreibe(Q,n2,bandw);

for (int i=0; i< ((rw+1)*n1); i++)
  {
    L[i]=Q[i]; //Q ist Q
  }

 cholesky(n1, L, rw); // erzeugt L, L ist L

//Lw=b -> w
 loese2(L, theta, n1, rw); // theta ist w

//L^T my=w -> my
 loese(L,theta, n1, rw); //theta ist my

//z
gausssample(temp, n1);

 for (int i=0; i<=(n2); i++)
   {
     z[i]=temp[i]; //z ist z
   }

 // cout << temp[n3-1] << endl;
 // cout << temp[n3] << endl;

//L^T v=z -> v
 loese(L,temp,n1, rw); //temp ist v

 // cout << temp[n3-1] << endl;
 // cout << temp[n3] << endl;

//beta=my + v
for (int i=0; i<=(n2); i++)
  {
    theta[i] += temp[i]; // theta = mu + v
  }

// mxschreibe(theta,n-1,1);

// cout << theta[n3]<< endl;
// cout << beta0[n3]<< endl;

// theta ist der vorgeschlagene Vektor

 erzeuge_b_Q_2(my2, Q2, alpha, theta, gamma, delta, X, n2, I, taubeta, rw, xcov, ncov, scov, omega); //erzeugt b^0 und Q^0, my2 ist b^0

 for (int i=0; i< ((rw+1)*(n-1)); i++)
   {
     L2[i]=Q2[i]; //Q2 ist Q^0
   }


 // mxschreibe(Q2,n-1,rw+1);
 // mxschreibe(L2,n-1,rw+1);

 cholesky(n1,L2,rw); //L2 ist L^0

 // mxschreibe(L2,n-1,rw+1);

 //L^0w=b^0 -> w
 loese2(L2, my2, n1, rw); //my2 ist jetzt w^0

//L^0^T my=w
 loese(L2,my2, n1, rw);//my2 ist jetzt mittelwertsvektor,  my2 ist jetzt mu^0

 for (int i=0; i<=(n2); i++)
   {
     my2[i]=beta0[i]-my2[i]; //my2 ist jetzt (beta^0 - mu^0)
   }



double akzw=0.0;  // loglik Proposal

//Prior
for (int i=2; i<=(n2); i++){
  akzw -= 0.5*taubeta*(theta[i]-2*theta[i-1]+theta[i-2])*(theta[i]-2*theta[i-1]+theta[i-2]); // - 0.5*taubeta*beta' R beta
  akzw += 0.5*taubeta*(beta0[i]-2*beta0[i-1]+beta0[i-2])*(beta0[i]-2*beta0[i-1]+beta0[i-2]); // 0.5*taubeta*beta^0' R beta^0
}
// cout << akzw << endl;
for (int i=1; i<=I; i++){
  for (int t=0; t<=(n2); t++){
    akzw += theta[t]*(X[i][t+2]); // beta c
    akzw -= beta0[t]*(X[i][t+2]); // - beta^0 c	     
    akzw -= omega[i][t+2]*delta[t+2]*exp(alpha[i]+theta[t]+sumg(ncov,xcov,gamma,t+2,scov)); // + h(beta)
    akzw += omega[i][t+2]*delta[t+2]*exp(alpha[i]+beta0[t]+sumg(ncov,xcov,gamma,t+2,scov)); // - h(beta^0)

  }
}
// cout << akzw << endl;
for (int i=0; i<=(n2); i++)  {
  akzw -= log(L[(rw+1)*i]); // - log(L_tt)
  akzw += log(L2[(rw+1)*i]); // log(L^0_tt)	
}
// cout << akzw << endl;
for (int i=0; i<=(n2); i++){
  akzw += 0.5*z[i]*z[i];
}
// cout << akzw << endl;
akzw -= 0.5*xMx2(Q2,my2,n2,rw+1); // -1/2*(beta^0 - mu^0)'Q^0(beta^0 - mu^0)

// cout << akzw << endl << endl;

// cout << taubeta << endl;

if (exp(akzw)>gsl_rng_uniform(r))
{
	for (int i=0; i<=(n2); i++)
	{
		beta[i+2]=theta[i];
	}
	acc_beta++;

}

return;
}


void update_beta_tau_block( double* alpha, double* beta, double* gamma, double* delta, double beta_a, double beta_b, long** X, long n, long I, double& taubeta, int rw, long& acc_beta, double taubetaRWSigma, double taubetaStar, long sampleCounter, int n1, int n2, double* my, double* my2, double* z, double* theta, double* beta0, double* Q, double* Q2, double* L, double* L2, double** xcov, int ncov, int scov, double** omega)
{


  //update log(taubeta) with change of variables
  taubetaStar = taubeta*exp(gsl_ran_gaussian(r,taubetaRWSigma));
  //taubetaStar = 720;

    // if(sampleCounter<10000){taubetaStar = 5000;}
 // cout << taubeta << "  " << taubetaStar << endl;
  // if(sampleCounter%500==1){cout << taubeta << endl << endl;}
 for(int t=0;t<=n2;t++){
   beta0[t] = beta[t+2];
 }


 // cout << taubetaStar << endl;
 erzeuge_b_Q_2(theta, Q, alpha, beta0, gamma, delta, X, n2, I, taubetaStar, rw, xcov, ncov, scov, omega); //erzeugt b und Q, theta ist b
 // cout << taubetaStar << endl;
for (int i=0; i< ((rw+1)*n1); i++)
  {
    L[i]=Q[i]; //Q ist Q
  }



 cholesky(n1, L, rw); // erzeugt L, L ist L

//Lw=b -> w
 loese2(L, theta, n1, rw); // theta ist w

//L^T my=w -> my
 loese(L,theta, n1, rw); //theta ist my

for (int i=0; i<=(n2); i++)
  {
    my[i] = theta[i]; // my ist my 
  }


//z
gausssample(temp, n1);

 for (int i=0; i<=(n2); i++)
   {
     z[i]=temp[i]; //z ist z
   }

//L^T v=z -> v
 loese(L,temp,n1, rw); //temp ist v

//beta=my + v
for (int i=0; i<=(n2); i++)
  {
    theta[i] += temp[i]; // theta = mu + v
  }

// theta ist der vorgeschlagene Vektor


for (int i=0; i<=(n2); i++)
  {
    my[i] = theta[i] - my[i]; // my ist (my - beta) 
  }


// cout << taubeta << endl;
 erzeuge_b_Q_2(my2, Q2, alpha, theta, gamma, delta, X, n2, I, taubeta, rw, xcov, ncov, scov, omega); //erzeugt b^0 und Q^0, my2 ist b^0
 // cout << taubeta << endl;
 for (int i=0; i< ((rw+1)*(n1)); i++)
   {
     L2[i]=Q2[i]; //Q2 ist Q^0
   }


 cholesky(n1,L2,rw); //L2 ist L^0

 //L^0w=b^0 -> w
 loese2(L2, my2, n1, rw); //my2 ist jetzt w^0

//L^0^T my=w
 loese(L2,my2, n1, rw);//my2 ist jetzt mittelwertsvektor,  my2 ist jetzt mu^0

 for (int i=0; i<=(n2); i++)
   {
     my2[i]=beta0[i]-my2[i]; //my2 ist jetzt (beta^0 - mu^0)
   }



double akzw=0.0;  // loglik Proposal

//Prior
for (int i=2; i<=(n2); i++){
      akzw -= 0.5*taubetaStar*(theta[i]-2*theta[i-1]+theta[i-2])*(theta[i]-2*theta[i-1]+theta[i-2]); // - 0.5*taubetaStar*beta' R beta
      akzw += 0.5*taubeta*(beta0[i]-2*beta0[i-1]+beta0[i-2])*(beta0[i]-2*beta0[i-1]+beta0[i-2]); // 0.5*taubeta*beta^0' R beta^0
      akzw += log(sqrt(taubetaStar)); //Normierungskonstante
      akzw -= log(sqrt(taubeta));
}


// cout << akzw << endl;
//likelihood
for (int i=1; i<=I; i++){
  for (int t=0; t<=(n2); t++){
    akzw += theta[t]*(X[i][t+2]); // beta c
    akzw -= beta0[t]*(X[i][t+2]); // - beta^0 c	     
    akzw -= omega[i][t+2]*delta[t+2]*exp(alpha[i]+theta[t]+sumg(ncov,xcov,gamma,t+2,scov)); // + h(beta)
    akzw += omega[i][t+2]*delta[t+2]*exp(alpha[i]+beta0[t]+sumg(ncov,xcov,gamma,t+2,scov)); // - h(beta^0)

  }
}
// cout << akzw << endl;
for (int i=0; i<=(n2); i++)  {
  akzw -= log(L[(rw+1)*i]); // - log(L_tt) ??
  akzw += log(L2[(rw+1)*i]); // + log(L^0_tt)	
}
// cout << akzw << endl;
//for (int i=0; i<=(n2); i++){
  //         akzw += 0.5*z[i]*z[i];
  //}
akzw += 0.5*xMx2(Q,my,n2,rw+1);
// cout << akzw << endl;
akzw -= 0.5*xMx2(Q2,my2,n2,rw+1); // -1/2*(beta^0 - mu^0)'Q^0(beta^0 - mu^0)


// cout << akzw << endl;



//Prior of log(taubeta)
// akzw += gsl_ran_gamma_log_pdf(taubetaStar,beta_a,1/beta_b) + log(taubetaStar);
// akzw -= gsl_ran_gamma_log_pdf(taubeta,beta_a,1/beta_b) + log(taubeta);

 akzw += beta_a*log(taubetaStar/taubeta);
 akzw -= beta_b*(taubetaStar - taubeta);


// double akzw2 = exp(akzw);
 //akzw2 *= gsl_ran_gamma_pdf(taubetaStar,beta_a,1/beta_b)*taubetaStar/(gsl_ran_gamma_pdf(taubeta,beta_a,1/beta_b)*taubeta);



 // berechneQ(Q, rw, taubetaStar, n2+1, 1, 0.0);
 // berechneQ(Q2, rw, taubeta, n2+1, 1, 0.0);
 // mxschreibe(Q2,n1,rw+1);
 //akzw -= 0.5*xMx2(Q,theta,n2,rw+1); 
//akzw += 0.5*xMx2(Q2,beta0,n2,rw+1); 


// cout << akzw << endl << endl;

// cout << taubeta << endl;

if (exp(akzw)>gsl_rng_uniform(r))
{
	for (int i=0; i<=(n2); i++)
	{
		beta[i+2]=theta[i];
	}
	taubeta = taubetaStar;
	acc_beta++;

}

return;
}



void machnu(double* mu, double* alpha, double* beta, double* delta, double** nu, long I, long n, int ncov, double** xcov, int scov)
{
	for (int i=1; i<=I; i++)
	  { //cout.flush();
		for (int t=2; t<=n; t++)
		{
			nu[i][t]=delta[t]*exp(sumg(ncov,xcov,mu,t,scov)+alpha[i]+beta[t]);
		}
	}
return;
}
/*
void update_gamma(double* alpha, double* beta, double* gamma, int ncov, double** xcov, long** X, long** Z, long** Y, long n, long I, double taugamma, double p, double** lambda, long& acc_gamma, double* P, double* P2, double* gammaalt, double*z, double* L, double* m, double** omega, double** omegaX, int scov, int mode)
{

  // erzeuge Pr�zisionsmatrix

 
  for (int i=0; i<ncov; i++)
    {
      for (int j=0; j<ncov; j++)
	{
	  P[i*ncov+j]=0.0;
	}
      P[ncov*i]=taugamma;
    }
  if (mode==1)
    {
    for (int k=0; k<ncov; k++)
    {
      for (int l=k; l<ncov; l++)
	{
	  for (int i=1; i<=I; i++)
	    {
	      for (int t=2; t<=n; t++)
		{
		  P[k*ncov+l-k]+= xcov[k][t]*xcov[l][t]*omegaX[i][t]*exp(alpha[i]+sumg(ncov,xcov,gamma,t,scov)+beta[t]);
		  
		  
		}
 	    }	      
	}
    }
    }
  // mxschreibe(P,ncov,ncov);
  if (mode==2)
    {
    for (int k=0; k<ncov; k++)
    {
      for (int l=k; l<ncov; l++)
	{
	  for (int i=1; i<=I; i++)
	    {
	      for (int t=2; t<=n; t++)
		{
		  P[k*ncov+l-k]+= xcov[k][t]*xcov[l][t]*omega[i][t]*Z[i][t-1]*exp(alpha[i]+sumg(ncov,xcov,gamma,t,scov)+beta[t]);
		  
		  
		}
 	    }	      
	}
    }
    }

   

//     //erzeuge m

    for (int j=0; j<ncov; j++)
      {
	m[j]=0.0;
      }
    if (mode==1)
      {
    for (int j=0; j<ncov; j++)
      {
	for (int i=1; i<=I; i++)
	  {
	    for (int t=2; t<=n; t++)
	      {

		m[j]+=xcov[j][t]*(X[i][t]);

		//1.Ableitung
		m[j] -= xcov[j][t]*omegaX[i][t]*exp(alpha[i]+sumg(ncov,xcov,gamma,t,scov)+beta[t]);
		//2.Ableitung
		for (int k=0; k<ncov; k++)
		  {
		    m[j]+=gamma[k]*xcov[k][t]*xcov[j][t]*omegaX[i][t]*exp(alpha[i]+sumg(ncov,xcov,gamma,t,scov)+beta[t]);
		  }
	      }
	  }
      }
      }
    if (mode==2)
      {
    for (int j=0; j<ncov; j++)
      {
	for (int i=1; i<=I; i++)
	  {
	    for (int t=2; t<=n; t++)
	      {

		m[j]+=xcov[j][t]*(Y[i][t]);

		//1.Ableitung
		m[j] -= xcov[j][t]*omega[i][t]*Z[i][t-1]*exp(alpha[i]+sumg(ncov,xcov,gamma,t,scov)+beta[t]);
		//2.Ableitung
		for (int k=0; k<ncov; k++)
		  {
		    m[j]+=gamma[k]*xcov[k][t]*xcov[j][t]*omega[i][t]*Z[i][t-1]*exp(alpha[i]+sumg(ncov,xcov,gamma,t,scov)+beta[t]);
		  }
	      }
	  }
      }
      }


    for (int i=0; i<ncov; i++)
      {
	gammaalt[i]=gamma[i];
	gamma[i]=m[i];
      }

    //Ziehe gammaproposal
    int bw=ncov-1;
   
    gausssample(z,ncov);    

    cholesky(ncov,P,bw); 


    
    double akzw=0.0;
    for (int i=0; i<ncov; i++)  
      {
	akzw += 0.5*z[i]*z[i];
	akzw -= log(P[i*ncov]);
      }

//     cout << akzw << endl;



    
    loese2(P, gamma, ncov, bw);


    loese(P, gamma, ncov, bw);


    loese(P, z, ncov, bw);
    for (int i=0; i<ncov; i++)
      {
	gamma[i]+=z[i];
      }



    //Berechen Akzeptanzwahrscheinlichkeit

    
      //erzeuge Pr�zisionsmatrix
    
    for (int i=0; i<ncov; i++)
      {
	for (int j=0; j<ncov; j++)
	{
	  P[i*ncov+j]=0.0;
	}
	P[ncov*i]=taugamma;
      }
    if (mode==1)
      {
    for (int k=0; k<ncov; k++)
      {
      for (int l=k; l<ncov; l++)
	{
	  for (int i=1; i<=I; i++)
	    {
	      for (int t=2; t<=n; t++)
		{
		  P[k*ncov+l-k]+= xcov[k][t]*xcov[l][t]*omegaX[i][t]*exp(alpha[i]+sumg(ncov,xcov,gamma,t,scov)+beta[t]);
		}
	    }	      
	}
      }
      }
    if (mode==2)
      {
    for (int k=0; k<ncov; k++)
      {
      for (int l=k; l<ncov; l++)
	{
	  for (int i=1; i<=I; i++)
	    {
	      for (int t=2; t<=n; t++)
		{
		  P[k*ncov+l-k]+= xcov[k][t]*xcov[l][t]*omega[i][t]*Z[i][t-1]*exp(alpha[i]+sumg(ncov,xcov,gamma,t,scov)+beta[t]);
		}
	    }	      
	}
      }
      }


//     mxschreibe(P,ncov,ncov);
    
    
    //erzeuge m
    
    for (int j=0; j<ncov; j++)
      {
	m[j]=0.0;
      }
    if (mode==1)
      {
    for (int j=0; j<ncov; j++)
      {
	for (int i=1; i<=I; i++)
	  {
	    for (int t=2; t<=n; t++)
	      {
		m[j]+=xcov[j][t]*(X[i][t]);
		
		//1.Ableitung
		m[j] -= xcov[j][t]*omegaX[i][t]*exp(alpha[i]+sumg(ncov,xcov,gamma,t,scov)+beta[t]);
		//2.Ableitung
		for (int k=0; k<ncov; k++)
		  {
		    m[j]+=gamma[k]*xcov[k][t]*xcov[j][t]*omegaX[i][t]*exp(alpha[i]+sumg(ncov,xcov,gamma,t,scov)+beta[t]);
		  }		
	      }
	  }
      }
      }
    if (mode==2)
      {
    for (int j=0; j<ncov; j++)
      {
	for (int i=1; i<=I; i++)
	  {
	    for (int t=2; t<=n; t++)
	      {
		m[j]+=xcov[j][t]*(Y[i][t]);
		
		//1.Ableitung
		m[j] -= xcov[j][t]*omega[i][t]*Z[i][t-1]*exp(alpha[i]+sumg(ncov,xcov,gamma,t,scov)+beta[t]);
		//2.Ableitung
		for (int k=0; k<ncov; k++)
		  {
		    m[j]+=gamma[k]*xcov[k][t]*xcov[j][t]*omega[i][t]*Z[i][t-1]*exp(alpha[i]+sumg(ncov,xcov,gamma,t,scov)+beta[t]);
		  }		
	      }
	  }
      }
      }

  
    for (int i=0; i<(ncov*ncov);i++)
      {
	P2[i]=P[i];
      }
 
   
    cholesky(ncov,P,bw);
    for (int i=0; i<ncov; i++)
      {
	akzw+=log(P[i*ncov]);
      }
//     cout << akzw << endl;

    loese2(P,m,ncov,bw);
    loese(P,m,ncov,bw);
    for (int i=0; i<ncov; i++)
      {
	m[i]=gammaalt[i]-m[i];
      }
//     mxschreibe(P2,ncov,ncov);

    //  akzw -= xMx(P2,m,ncov,bw+1);
     akzw -= 0.5*xMx(P2,m,ncov,bw+1);

//     cout << akzw << endl;


    for (int i=0; i<ncov; i++)
      {
	akzw -= 0.5*taugamma*gamma[i]*gamma[i];
      }
    if (mode==1)
      {
	
    for (int j=0; j<ncov; j++)
      {
	for (int i=1; i<=I; i++) //fehler i<I 20.07.05
	  {
	    for (int t=2;t<=n; t++) //fehler t<n 20.07.05
	      {
		akzw+=gamma[j]*xcov[j][t]*(X[i][t]);
		akzw-=omegaX[i][t]*exp(alpha[i]+sumg(ncov,xcov,gamma,t,scov)+beta[t]);
	      }
	  }
      }
	
      }
    if (mode==2)
      {
    for (int j=0; j<ncov; j++)
      {
	for (int i=1; i<I; i++)
	  {
	    for (int t=2;t<n; t++)
	      {
		akzw+=gamma[j]*xcov[j][t]*(Y[i][t]);
		akzw-=omega[i][t]*Z[i][t-1]*exp(alpha[i]+sumg(ncov,xcov,gamma,t,scov)+beta[t]);
	      }
	  }
      }
      }

//     cout << akzw << endl;

    for (int i=0; i<ncov; i++)
      {
	z[i]=gamma[i];
	gamma[i]=gammaalt[i];
 }

    for (int i=0; i<ncov; i++)
      {
	akzw += 0.5*taugamma*gamma[i]*gamma[i];
      }
 //    cout << akzw << endl;

    if (mode==1)
      {

    for (int j=0; j<ncov; j++)
      {
	for (int i=1; i<=I; i++) //fehler i<I 20.07.05
	  {
	    for (int t=2;t<=n; t++) //fehler t<n 20.07.05
	      {
		akzw-=gamma[j]*xcov[j][t]*(X[i][t]);
		akzw+=omegaX[i][t]*exp(alpha[i]+sumg(ncov,xcov,gamma,t,scov)+beta[t]);
	      }
	  }
      }

      }
    if (mode==2)
      {
    for (int j=0; j<ncov; j++)
      {
	for (int i=1; i<I; i++)
	  {
	    for (int t=2;t<n; t++)
	      {
		akzw-=gamma[j]*xcov[j][t]*(Y[i][t]);
		akzw+=omega[i][t]*Z[i][t-1]*exp(alpha[i]+sumg(ncov,xcov,gamma,t,scov)+beta[t]);
	      }
	  }
      }
      }

//     cout << akzw << endl;;
//     mxschreibe(gamma,1,ncov);
//     mxschreibe(z,1,ncov);

//     cout << akzw << endl << endl;;
    if (exp(akzw)>gsl_rng_uniform(r))
      {
	for (int i=0; i<ncov; i++)
	  {
	    gamma[i]=z[i];
	  }
	acc_gamma++;
      }
    return; 
}
*/
void update_gamma_j(int j, double* alpha, double* beta, double* gamma, double* delta, int ncov, double** xcov, long** X, long n, long I, double taugamma, double* gammaneu, long& acc_gamma, double** omega, int scov)
{
  double g = 0;
  double gd = 0;
  double gdd = 0;
  double c = 0;
  for(int i=1;i<=I;i++){
    for(int t=2;t<=n;t++){
      g -= omega[i][t]*delta[t]*exp(alpha[i] + beta[t] + sumg(ncov,xcov,gamma,t,scov)); // g ist g(gamma[j]^0)
      gd -= omega[i][t]*delta[t]*exp(alpha[i] + beta[t] + sumg(ncov,xcov,gamma,t,scov))*xcov[j][t];
      gdd -= omega[i][t]*delta[t]*exp(alpha[i] + beta[t] + sumg(ncov,xcov,gamma,t,scov))*xcov[j][t]*xcov[j][t];
      c += xcov[j][t]*X[i][t];
    }
  }

  double s = sqrt(1/(taugamma-gdd)); // s ist s
  double b = c + gd - gamma[j]*gdd;
  double m = b*s*s;

  double gammajStar = gsl_ran_gaussian(r,s) + m;

  // cout << g << endl;
  // cout << gd << endl;
  // cout << gdd << endl;
  // cout << c << endl;
  // cout << b << endl;
  // cout << s << endl;
  // cout << m << endl;
  // cout << gammajStar << endl;
  // cout << gamma[j] << endl << endl;

  // cout << gamma[j] << endl;

  for(int k=0;k<ncov;k++){
    gammaneu[k] = gamma[k];
  }

  // cout << gammaneu[j] << endl;

  gammaneu[j] = gammajStar;

  // cout << gamma[j] << endl;
  // cout << gammaneu[j] << endl << endl;

  double g2 = 0;
  double gd2 = 0;
  double gdd2 = 0;
  for(int i=1;i<=I;i++){
    for(int t=2;t<=n;t++){
      g2 -= omega[i][t]*delta[t]*exp(alpha[i] + beta[t] + sumg(ncov,xcov,gammaneu,t,scov)); // g2 ist g(gamma[j])
      gd2 -= omega[i][t]*delta[t]*exp(alpha[i] + beta[t] + sumg(ncov,xcov,gammaneu,t,scov))*xcov[j][t];
      gdd2 -= omega[i][t]*delta[t]*exp(alpha[i] + beta[t] + sumg(ncov,xcov,gammaneu,t,scov))*xcov[j][t]*xcov[j][t];

    }
  }

  double s2 = sqrt(1/(taugamma-gdd2)); //s2 ist s^0
  double b2 = c + gd2 - gammajStar*gdd2; 
  double m2 = b2*s2*s2;

  double a = 0;

  a += gammajStar*c;
  a -= gamma[j]*c;
  // cout << a << endl;
  a -= 0.5*taugamma*gammajStar*gammajStar;
  a += 0.5*taugamma*gamma[j]*gamma[j];
  // cout << a << endl;
  a += g2;
  a -= g;
  // cout << a << endl;
  a += log(s);
  a -= log(s2);
  // cout << a << endl;
  a += 0.5*((gammajStar-m)/s)*((gammajStar-m)/s);
  a -= 0.5*((gamma[j]-m2)/s2)*((gamma[j]-m2)/s2);
  //cout << a << endl << endl;

  if(exp(a)>gsl_rng_uniform (r)){
    gamma[j] = gammajStar;
    acc_gamma += 1;
  }

  return;
}



void update_beta_t(int t, double* alpha, double* beta, double* gamma, double* delta, int ncov, double** xcov, long** X, long n, long I, double taubeta, long& acc_beta, double** omega, int scov)
{
  double h = 0;
  double c = 0;
  double d = 0;
  for(int i=1;i<=I;i++){
    h -= omega[i][t]*delta[t]*exp(alpha[i] + beta[t] + sumg(ncov,xcov,gamma,t,scov)); // h ist h(beta[t]^0), beta ist \beta^0, betatStar ist \beta
    c += X[i][t];
    }
  if(t==2){
    c -= taubeta*(beta[t+2]-2*beta[t+1]);
    d = taubeta;
  }
  if(t==3){
    c -= taubeta*((beta[t+2]-2*beta[t+1]) + (-2*beta[t+1] - 2*beta[t-1]));
    d = 5*taubeta;
    }
  if((t>=4)&&(t<=(n-2))){
    c -= taubeta*((beta[t+2]-2*beta[t+1]) + (-2*beta[t+1] - 2*beta[t-1]) + (beta[t-2] - 2*beta[t-1]));
    d = 6*taubeta;
    }
  if(t==(n-1)){
    c -= taubeta*((-2*beta[t+1] - 2*beta[t-1]) + (beta[t-2] - 2*beta[t-1]));
    d = 5*taubeta;
  }
  if(t==n){
    c -= taubeta*(beta[t-2] - 2*beta[t-1]);
    d = taubeta;
  }



  double s = sqrt(1/(d - h)); // s ist s
  double b = c + (1 - beta[t])*h;
  double m = b*s*s;

  double betatStar = gsl_ran_gaussian(r,s) + m;

  // cout << s << endl;
  // cout << b << endl;
  // cout << m << endl;
  // cout << betatStar << endl;
  // cout << beta[t] << endl;


  double h2 = 0;
  for(int i=1;i<=I;i++){
    h2 -= omega[i][t]*delta[t]*exp(alpha[i] + betatStar + sumg(ncov,xcov,gamma,t,scov)); // h2 ist h(beta[t])
    }

  double s2 = sqrt(1/(d - h2)); // s2 ist s^0
  double b2 = c + (1 - betatStar)*h2;
  double m2 = b2*s2*s2;


  // cout << s2 << endl;
   // cout << b2 << endl;
   // cout << m2 << endl;



  double a = 0;

  a += betatStar*c;
  a -= beta[t]*c;
  // cout << a << endl;
  a -= 0.5*d*betatStar*betatStar;
  a += 0.5*d*beta[t]*beta[t];
  // cout << a << endl;
  a += h2;
  a -= h;
  // cout << a << endl;
  a += log(s);
  a -= log(s2);
  // cout << a << endl;
  a += 0.5*((betatStar-m)/s)*((betatStar-m)/s);
  a -= 0.5*((beta[t]-m2)/s2)*((beta[t]-m2)/s2);
  // cout << a << endl << endl;

  if(exp(a)>gsl_rng_uniform (r)){
    beta[t] = betatStar;
    acc_beta += 1;
  }

  return;
}






void update_lambda_br(double** lambda, double** lambda_br,double* xi_lambda, int** breakpoints, int** breakpointsStar, int* K, int* KStar, int* Km1, double alpha_lambda, double beta_lambda, long** Y, long** Z, long n, long I, double& acceptedbr, double** omega, int theta_pred_estim, int xi_estim, int K_geom, double p_K, double alpha_xi, double beta_xi)
{
    //update breakpoints of lambda using reversible jump MCMC


    int newbreakpoint;
    int removebreakpoint;
    int newbreakpointnumber;
    int u;
    double v=1;
    double a;
    double alpha_la;
    double beta_la;
    for(int i=1;i<=I;i++){
    if(!theta_pred_estim){
	a=gsl_rng_uniform(r);
	if(a<0.5){u=1;}else{u=2;}
 
	if(K[i]==1){u=2;v=.5;}  //K[i] is number of segments of lambda
	if(K[i]==(n-1)){u=1;v=.5;} //if(!theta_pred_estim) max of K[i] is n-1


	
	//decide if new brreakpoint or remove breakpoint
	if(u==1){//remove breakpoint
	  if(K[i]==2){v=2;} 
	  KStar[i]=K[i]-1;
	  a=gsl_rng_uniform(r);
          removebreakpoint=(int)floor(a*(double)(K[i]-1))+1; 
	  //generate breakpointsStar
	  for(int k=1;k<removebreakpoint;k++){
	    breakpointsStar[i][k]=breakpoints[i][k];
	  }
	  for(int k=(removebreakpoint+1);k<=K[i];k++){
	    breakpointsStar[i][k-1]=breakpoints[i][k];
	  }
	}//if(u==1)
	if(u==2){//new breakpoint
	  if(K[i]==(n-2)){v=2;} 
	  KStar[i]=K[i]+1;
	  //sample new breakpoint
	  int need=1;
	  while(need==1){
	    need=0;
	    a=gsl_rng_uniform(r);
            newbreakpoint=int(floor(a*double(n)))+1; 
	    if (newbreakpoint<=2){need=1;}
	    if (newbreakpoint>n){need=1;}
  	    for(int k=1;k<=K[i];k++){
  	      if(newbreakpoint==breakpoints[i][k]){
  	        need=1;
	      }
	    }
  	  }//while(need==1)
	  //generate breakpointsStar
  	  for(int k=1;k<=K[i];k++){
  	    if((newbreakpoint>breakpoints[i][k-1])&&(newbreakpoint<breakpoints[i][k])){
  	      newbreakpointnumber = k;
	    }
	  }
	  for(int k=1;k<newbreakpointnumber;k++){
	    breakpointsStar[i][k]=breakpoints[i][k];
	    need=0;

	  }
          breakpointsStar[i][newbreakpointnumber]=newbreakpoint;
	  for(int k=newbreakpointnumber;k<=K[i];k++){ //changed K[I]
	    breakpointsStar[i][k+1]=breakpoints[i][k];
	  }
	}//if(u==2)
    }//if(!theta_pred_estim)

    if(theta_pred_estim){

	a=gsl_rng_uniform(r);
	if(a<0.5){u=1;}else{u=2;}
 
	if(K[i]==1){u=2;v=.5;}
	if(K[i]==(n)){u=1;v=.5;} 


	
	//decide if new brreakpoint or remove breakpoint
	if(u==1){//remove breakpoint
	  if(K[i]==2){v=2;} 
	  KStar[i]=K[i]-1;
	  a=gsl_rng_uniform(r);
          removebreakpoint=(int)floor(a*(double)(K[i]-1))+1; 
	  //generate breakpointsStar
	  for(int k=1;k<removebreakpoint;k++){
	    breakpointsStar[i][k]=breakpoints[i][k];
	  }
	  for(int k=(removebreakpoint+1);k<=K[i];k++){ //changed K[I]
	    breakpointsStar[i][k-1]=breakpoints[i][k];
	  }
	}//if(u==1)
	if(u==2){//new breakpoint
	  if(K[i]==(n-1)){v=2;} 
	  KStar[i]=K[i]+1;
	  //sample new breakpoint
	  int need=1;
	  while(need==1){
	    need=0;
	    a=gsl_rng_uniform(r);
            newbreakpoint=int(floor(a*double(n+1)))+1; 
	    if (newbreakpoint<=2){need=1;}
	    if (newbreakpoint>(n+1)){need=1;}
  	    for(int k=1;k<=K[i];k++){
  	      if(newbreakpoint==breakpoints[i][k]){
  	        need=1;
	      }
	    }
  	  }//while(need==1)
	  //generate breakpointsStar
  	  for(int k=1;k<=K[i];k++){
  	    if((newbreakpoint>breakpoints[i][k-1])&&(newbreakpoint<breakpoints[i][k])){
  	      newbreakpointnumber = k;
	    }
	  }
	  for(int k=1;k<newbreakpointnumber;k++){
	    breakpointsStar[i][k]=breakpoints[i][k];
	    need=0;

	  }
          breakpointsStar[i][newbreakpointnumber]=newbreakpoint;
	  for(int k=newbreakpointnumber;k<=K[i];k++){ 
	    breakpointsStar[i][k+1]=breakpoints[i][k];
	  }
	}//if(u==2)
      }//if(theta_pred_estim)    
      

    double sumY1=0.0;
    double sumoZ1=0.0;
    double sumY2=0.0;
    double sumoZ2=0.0;
    double sumY3=0.0;
    double sumoZ3=0.0;
    double sumY4=0.0;
    double sumoZ4=0.0;
    if(newbreakpointnumber!=(KStar[i]-1)){
    if (u==2)
      {
	for (int t=breakpointsStar[i][newbreakpointnumber-1]; t<breakpointsStar[i][newbreakpointnumber];t++)
	  {
	    sumY1+=Y[i][t];
	    sumoZ1+=(omega[i][t]*Z[i][t-1]);
	  }
	for (int t=breakpointsStar[i][newbreakpointnumber]; t<breakpointsStar[i][newbreakpointnumber+1];t++)
	  {
	    sumY2+=Y[i][t];
	    sumoZ2+=(omega[i][t]*Z[i][t-1]);
	  }
	sumY3=sumY1+sumY2;
	sumoZ3=sumoZ1+sumoZ2;
      }
    }
    if(removebreakpoint!=(KStar[i])){
    if (u==1)
      {
	for (int t=breakpoints[i][removebreakpoint-1]; t<breakpoints[i][removebreakpoint];t++)
	  {
	    sumY3+=Y[i][t];
	    sumoZ3+=(omega[i][t]*Z[i][t-1]);
	  }
	for (int t=breakpoints[i][removebreakpoint]; t<breakpoints[i][removebreakpoint+1];t++)
	  {
	    sumY4+=Y[i][t];
	    sumoZ4+=(omega[i][t]*Z[i][t-1]);
	  }
	sumY1=sumY3+sumY4;
	sumoZ1=sumoZ3+sumoZ4;
      }
    }
    if(newbreakpointnumber==(KStar[i]-1)){
    if (u==2){
	for (int t=breakpointsStar[i][newbreakpointnumber-1]; t<breakpointsStar[i][newbreakpointnumber];t++)
	  {
	    sumY1+=Y[i][t];
	    sumoZ1+=(omega[i][t]*Z[i][t-1]);
	  }
        if(breakpoints[i][newbreakpointnumber]!=(n+1)){
	for (int t=breakpointsStar[i][newbreakpointnumber]; t<(breakpointsStar[i][newbreakpointnumber+1]-1);t++)
	  {
	    sumY2+=Y[i][t];
	    sumoZ2+=(omega[i][t]*Z[i][t-1]);
	  }
	}
	sumY3=sumY1+sumY2;
	sumoZ3=sumoZ1+sumoZ2;
      }
    }
    if(removebreakpoint==(KStar[i])){
      if (u==1){
	for (int t=breakpoints[i][removebreakpoint-1]; t<breakpoints[i][removebreakpoint];t++)
	  {
	    sumY3+=Y[i][t];
	    sumoZ3+=(omega[i][t]*Z[i][t-1]);
	  }
        if(breakpoints[i][removebreakpoint]!=(n+1)){
	for (int t=breakpoints[i][removebreakpoint]; t<(breakpoints[i][removebreakpoint+1]-1);t++)
	  {
	    sumY4+=Y[i][t];
	    sumoZ4+=(omega[i][t]*Z[i][t-1]);
	  }
        }
	sumY1=sumY3+sumY4;
	sumoZ1=sumoZ3+sumoZ4;

      }
    }

   alpha_la = alpha_lambda;
   beta_la = beta_lambda;
   if(xi_estim){
     beta_la = xi_lambda[i];
   }
    sumY1+=alpha_la;
    sumY2+=alpha_la;
    sumY3+=alpha_la;
    sumY4+=alpha_la;
    sumoZ1+=beta_la;
    sumoZ2+=beta_la;
    sumoZ3+=beta_la;
    sumoZ4+=beta_la;

	
    double accbr = alpha_la*log(beta_la)-gsl_sf_lngamma(alpha_la); // alpha_lambda beta_lambda
    if(K_geom){
      accbr += log(1-p_K);
    }
    accbr=accbr*pow(-1,u)+log(v);

    if (u==2)
      {
	accbr=accbr+gsl_sf_lngamma(sumY1)+gsl_sf_lngamma(sumY2)-gsl_sf_lngamma(sumY3);
        accbr=accbr-sumY1*log(sumoZ1)-sumY2*log(sumoZ2)+sumY3*log(sumoZ3);
     }
    if (u==1)
      {
	accbr=accbr+gsl_sf_lngamma(sumY1)-gsl_sf_lngamma(sumY3)-gsl_sf_lngamma(sumY4);
	accbr=accbr-sumY1*log(sumoZ1)+sumY3*log(sumoZ3)+sumY4*log(sumoZ4);
      }
    if (gsl_rng_uniform (r) <= exp(accbr)) {
      // for(int i=1;i<=I;i++){ changed
        K[i] = KStar[i]; 
        for(int k=0;k<=KStar[i];k++){
          breakpoints[i][k]=breakpointsStar[i][k];
        }
        acceptedbr++;
	//} changed
    }

    Km1[i]=K[i]-1;


    //update xi_lambda
    if(xi_estim){
	double a = alpha_xi + alpha_lambda*K[i];
	double b = beta_xi;
        for(int k=1;k<=K[i];k++){
	  b += lambda_br[i][k];
	}
	xi_lambda[i] = gsl_ran_gamma (r, a, 1/b);
    }else{
      xi_lambda[i] = beta_lambda;
    }
    } //for i

      //if theta_pred_estim
    //update timevariing lambda with breakpoints via reversible jump MCMC
    //breakpoints = 2,...,n+1, number of breakpoins in region i is K[i]+1, number of time regions is K[i] (breakpoints[i][0]=2, ..., breakpoints[i][K[i]]=n+1)
      //if !theta_pred_estim
    //update timevariing lambda with breakpoints via reversible jump MCMC
    //breakpoints = 2,...,n+2, number of breakpoins in region i is K[i]+1, number of time regions is K[i] (breakpoints[i][0]=2, ..., breakpoints[i][K[i]]=n+2)

      for(int i=1;i<=I;i++){
        for(int k=1;k<=(K[i]-1);k++){
	  double a = alpha_la;
	  double b = xi_lambda[i];
	  for(int t=breakpoints[i][k-1];t<breakpoints[i][k];t++){ //breakpoints = 2,...,n+2
	    a += Y[i][t];
	    b += omega[i][t]*Z[i][t-1];
	  }
          lambda_br[i][k] = gsl_ran_gamma (r, a, 1/b);
	  for(int t=breakpoints[i][k-1];t<breakpoints[i][k];t++){
	    lambda[i][t] = lambda_br[i][k]; //calculate the lambda[i][t] from the lambda_br[i][k]
	  }
        }


	double a = alpha_la;
	double b = xi_lambda[i];
	if(breakpoints[i][K[i]-1]<(n+1)){ 
	  for(int t=breakpoints[i][K[i]-1];t<(breakpoints[i][K[i]]-1);t++){ //breakpoints = 2,...,n+2
	    a += Y[i][t];
	    b += omega[i][t]*Z[i][t-1];
	  }
	}
          lambda_br[i][K[i]] = gsl_ran_gamma (r, a, 1/b);
	  for(int t=breakpoints[i][K[i]-1];t<breakpoints[i][K[i]];t++){
	    lambda[i][t] = lambda_br[i][K[i]]; //calculate the lambda[i][t] from the lambda_br[i][k]
	  }
      } //for i
  return;
}







void update_delta_br(double* delta, double* delta_br,double &xi_delta, int* breakpoints_delta, int* breakpointsStar_delta, int& K_delta, int& KStar_delta, int& Km1_delta, double delta_a, double delta_b, long** X, double** nu, long n, long I, double& acceptedbr_delta, double** omega, int xi_estim_delta, int K_geom, double p_K, double alpha_xi, double beta_xi)
{
    //update breakpoints of lambda using reversible jump MCMC


    int newbreakpoint;
    int removebreakpoint;
    int newbreakpointnumber;
    int u;
    double v=1;
    double a;
    double alpha_de;
    double beta_de;

	a=gsl_rng_uniform(r);
	if(a<0.5){u=1;}else{u=2;}
 
	if(K_delta==1){u=2;v=.5;}
	if(K_delta==(n-1)){u=1;v=.5;} 


	
	//decide if new brreakpoint or remove breakpoint
	if(u==1){//remove breakpoint
	  if(K_delta==2){v=2;} 
	  KStar_delta=K_delta-1;
	  a=gsl_rng_uniform(r);
          removebreakpoint=(int)floor(a*(double)(K_delta-1))+1; 
	  //generate breakpointsStar_delta
	  for(int k=1;k<removebreakpoint;k++){
	    breakpointsStar_delta[k]=breakpoints_delta[k];
	  }
	  for(int k=(removebreakpoint+1);k<=K_delta;k++){
	    breakpointsStar_delta[k-1]=breakpoints_delta[k];
	  }
	}//if(u==1)



	if(u==2){//new breakpoint
	  if(K_delta==(n-2)){v=2;} 
	  KStar_delta=K_delta+1;
	  //sample new breakpoint
	  int need=1;
	  while(need==1){
	    need=0;
	    a=gsl_rng_uniform(r);
            newbreakpoint=int(floor(a*double(n)))+1; 
	    if (newbreakpoint<=2){need=1;}
	    if (newbreakpoint>n){need=1;}
  	    for(int k=1;k<=K_delta;k++){
  	      if(newbreakpoint==breakpoints_delta[k]){
  	        need=1;
	      }
	    }
  	  }//while(need==1)
	  //generate breakpointsStar_delta
  	  for(int k=1;k<=K_delta;k++){
  	    if((newbreakpoint>breakpoints_delta[k-1])&&(newbreakpoint<breakpoints_delta[k])){
  	      newbreakpointnumber = k;
	    }
	  }

	  for(int k=1;k<newbreakpointnumber;k++){
	    breakpointsStar_delta[k]=breakpoints_delta[k];
	    need=0;

	  }
          breakpointsStar_delta[newbreakpointnumber]=newbreakpoint;

	  for(int k=newbreakpointnumber;k<=K_delta;k++){ //changed K
	    breakpointsStar_delta[k+1]=breakpoints_delta[k];
	  }
	}//if(u==2)


    double sumX1=0.0;
    double sumoZ1=0.0;
    double sumX2=0.0;
    double sumoZ2=0.0;
    double sumX3=0.0;
    double sumoZ3=0.0;
    double sumX4=0.0;
    double sumoZ4=0.0;
    if(newbreakpointnumber!=(KStar_delta-1)){
    if (u==2)
      {
	for(int i=1;i<=I;i++){
	for (int t=breakpointsStar_delta[newbreakpointnumber-1]; t<breakpointsStar_delta[newbreakpointnumber];t++)
	  {
	    sumX1+=X[i][t];
	    sumoZ1+=(omega[i][t]*nu[i][t]/delta[t]);
	  }
	for (int t=breakpointsStar_delta[newbreakpointnumber]; t<breakpointsStar_delta[newbreakpointnumber+1];t++)
	  {
	    sumX2+=X[i][t];
	    sumoZ2+=(omega[i][t]*nu[i][t]/delta[t]);
	  }
	}
	sumX3=sumX1+sumX2;
	sumoZ3=sumoZ1+sumoZ2;
      }
    }
    if(removebreakpoint!=(KStar_delta)){
    if (u==1)
      {
	for(int i=1;i<=I;i++){
	for (int t=breakpoints_delta[removebreakpoint-1]; t<breakpoints_delta[removebreakpoint];t++)
	  {
	    sumX3+=X[i][t];
	    sumoZ3+=(omega[i][t]*nu[i][t]/delta[t]);
	  }
	for (int t=breakpoints_delta[removebreakpoint]; t<breakpoints_delta[removebreakpoint+1];t++)
	  {
	    sumX4+=X[i][t];
	    sumoZ4+=(omega[i][t]*nu[i][t]/delta[t]);
	  }
	}
	sumX1=sumX3+sumX4;
	sumoZ1=sumoZ3+sumoZ4;
      }
    }
    if(newbreakpointnumber==(KStar_delta-1)){
    if (u==2){
	for(int i=1;i<=I;i++){
	for (int t=breakpointsStar_delta[newbreakpointnumber-1]; t<breakpointsStar_delta[newbreakpointnumber];t++)
	  {
	    sumX1+=X[i][t];
	    sumoZ1+=(omega[i][t]*nu[i][t]/delta[t]);
	  }
        if(breakpoints_delta[newbreakpointnumber]!=(n+1)){
	for (int t=breakpointsStar_delta[newbreakpointnumber]; t<(breakpointsStar_delta[newbreakpointnumber+1]-1);t++)
	  {
	    sumX2+=X[i][t];
	    sumoZ2+=(omega[i][t]*nu[i][t]/delta[t]);
	  }
	}
	}
	sumX3=sumX1+sumX2;
	sumoZ3=sumoZ1+sumoZ2;
      }
    }
    if(removebreakpoint==(KStar_delta)){
      if (u==1){
	for(int i=1;i<=I;i++){
	for (int t=breakpoints_delta[removebreakpoint-1]; t<breakpoints_delta[removebreakpoint];t++)
	  {
	    sumX3+=X[i][t];
	    sumoZ3+=(omega[i][t]*nu[i][t]/delta[t]);
	  }
        if(breakpoints_delta[removebreakpoint]!=(n+1)){
	for (int t=breakpoints_delta[removebreakpoint]; t<(breakpoints_delta[removebreakpoint+1]-1);t++)
	  {
	    sumX4+=X[i][t];
	    sumoZ4+=(omega[i][t]*nu[i][t]/delta[t]);
	  }
        }
	}
	sumX1=sumX3+sumX4;
	sumoZ1=sumoZ3+sumoZ4;

      }
    }

    // cout << sumX1 << sumX2 << sumX3 << sumX4 << sumoZ1 << sumoZ2 << sumoZ3 << sumoZ4 << endl;




   alpha_de = delta_a;
   beta_de = delta_b;
   if(xi_estim_delta){
   beta_de = xi_delta;

   }
    sumX1+=alpha_de;
    sumX2+=alpha_de;
    sumX3+=alpha_de;
    sumX4+=alpha_de;
    sumoZ1+=beta_de;
    sumoZ2+=beta_de;
    sumoZ3+=beta_de;
    sumoZ4+=beta_de;

	
    double accbr = alpha_de*log(beta_de)-gsl_sf_lngamma(alpha_de); // delta_a delta_b
    if(K_geom){
      accbr += log(1-p_K);
    }
    accbr=accbr*pow(-1,u)+log(v);

    if (u==2)
      {
	accbr=accbr+gsl_sf_lngamma(sumX1)+gsl_sf_lngamma(sumX2)-gsl_sf_lngamma(sumX3);
        accbr=accbr-sumX1*log(sumoZ1)-sumX2*log(sumoZ2)+sumX3*log(sumoZ3);
     }
    if (u==1)
      {
	accbr=accbr+gsl_sf_lngamma(sumX1)-gsl_sf_lngamma(sumX3)-gsl_sf_lngamma(sumX4);
	accbr=accbr-sumX1*log(sumoZ1)+sumX3*log(sumoZ3)+sumX4*log(sumoZ4);
      }
    if (gsl_rng_uniform (r) <= exp(accbr)) {
        K_delta = KStar_delta; 
        for(int k=0;k<=KStar_delta;k++){
          breakpoints_delta[k]=breakpointsStar_delta[k];
        }
        acceptedbr_delta++;

    }


    Km1_delta=K_delta-1;

    // cout << "accbr" << accbr << endl;




    //update xi_delta
    if(xi_estim_delta){
	double a = alpha_xi + delta_a*K_delta;
	double b = beta_xi;
        for(int k=1;k<=K_delta;k++){
	  b += delta_br[k];
	}
	xi_delta = gsl_ran_gamma (r, a, 1/b);
    }else{
      xi_delta = delta_b;
    }

    //  cout << "xi_delta  " << xi_delta << endl;
    // cout << "delta_br[1]  " << delta_br[1] << endl;






    //update timevariing delta with breakpoints_delta via reversible jump MCMC


        for(int k=1;k<=(K_delta-1);k++){
	  double a = alpha_de;
	  double b = xi_delta;
          for(int i=1;i<=I;i++){
	  for(int t=breakpoints_delta[k-1];t<breakpoints_delta[k];t++){ //breakpoints_delta = 2,...,n+2
	    a += X[i][t];
	    b += omega[i][t]*nu[i][t]/delta[t];
	  }
          }
          delta_br[k] = gsl_ran_gamma (r, a, 1/b);
	  for(int t=breakpoints_delta[k-1];t<breakpoints_delta[k];t++){
	    delta[t] = delta_br[k]; //calculate the delta[t] from the delta_br[k]
	  }

        }


	//cout << "nu[1][2]   " << nu[1][2] << endl;
	//cout << "delta[2]   " << delta[2] << endl;

	a = alpha_de;
	double b = xi_delta;
	//	cout << a << "           " << b << endl;
	if(breakpoints_delta[K_delta-1]<(n+1)){ 
          for(int i=1;i<=I;i++){
	  for(int t=breakpoints_delta[K_delta-1];t<(breakpoints_delta[K_delta]-1);t++){ //breakpoints_delta = 2,...,n+2
	    a += X[i][t];
	    b += omega[i][t]*nu[i][t]/delta[t];
	  }
	  }
	}
          delta_br[K_delta] = gsl_ran_gamma (r, a, 1/b);
	  for(int t=breakpoints_delta[K_delta-1];t<breakpoints_delta[K_delta];t++){
	    delta[t] = delta_br[K_delta]; //calculate the delta[t] from the delta_br[k]
	  }
	  //	cout << a << "           " << b << endl;

	  // 	  cout << delta_br[1] << endl;





  return;
}



void update_epsilon_br(double* epsilon, double* epsilon_br,double& xi_epsilon, int* breakpoints_epsilon, int* breakpointsStar_epsilon, int& K_epsilon, int& KStar_epsilon, int& Km1_epsilon, double epsilon_a, double epsilon_b, long** S, long n, long I, double& acceptedbr_epsilon, double** omega, int xi_estim_epsilon, int K_geom, double p_K, double alpha_xi, double beta_xi)
{
    //update breakpoints of lambda using reversible jump MCMC


    int newbreakpoint;
    int removebreakpoint;
    int newbreakpointnumber;
    int u;
    double v=1;
    double a;
    double alpha_ep;
    double beta_ep;

	a=gsl_rng_uniform(r);
	if(a<0.5){u=1;}else{u=2;}
 
	if(K_epsilon==1){u=2;v=.5;}
	if(K_epsilon==(n-1)){u=1;v=.5;} 


	
	//decide if new brreakpoint or remove breakpoint
	if(u==1){//remove breakpoint
	  if(K_epsilon==2){v=2;} 
	  KStar_epsilon=K_epsilon-1;
	  a=gsl_rng_uniform(r);
          removebreakpoint=(int)floor(a*(double)(K_epsilon-1))+1; 
	  //generate breakpointsStar_epsilon
	  for(int k=1;k<removebreakpoint;k++){
	    breakpointsStar_epsilon[k]=breakpoints_epsilon[k];
	  }
	  for(int k=(removebreakpoint+1);k<=K_epsilon;k++){
	    breakpointsStar_epsilon[k-1]=breakpoints_epsilon[k];
	  }
	}//if(u==1)



	if(u==2){//new breakpoint
	  if(K_epsilon==(n-2)){v=2;} 
	  KStar_epsilon=K_epsilon+1;
	  //sample new breakpoint
	  int need=1;
	  while(need==1){
	    need=0;
	    a=gsl_rng_uniform(r);
            newbreakpoint=int(floor(a*double(n)))+1; 
	    if (newbreakpoint<=2){need=1;}
	    if (newbreakpoint>n){need=1;}
  	    for(int k=1;k<=K_epsilon;k++){
  	      if(newbreakpoint==breakpoints_epsilon[k]){
  	        need=1;
	      }
	    }
  	  }//while(need==1)
	  //generate breakpointsStar_epsilon
  	  for(int k=1;k<=K_epsilon;k++){
  	    if((newbreakpoint>breakpoints_epsilon[k-1])&&(newbreakpoint<breakpoints_epsilon[k])){
  	      newbreakpointnumber = k;
	    }
	  }

	  for(int k=1;k<newbreakpointnumber;k++){
	    breakpointsStar_epsilon[k]=breakpoints_epsilon[k];
	    need=0;

	  }
          breakpointsStar_epsilon[newbreakpointnumber]=newbreakpoint;

	  for(int k=newbreakpointnumber;k<=K_epsilon;k++){ //changed K
	    breakpointsStar_epsilon[k+1]=breakpoints_epsilon[k];
	  }
	}//if(u==2)


    double sumX1=0.0;
    double sumoZ1=0.0;
    double sumX2=0.0;
    double sumoZ2=0.0;
    double sumX3=0.0;
    double sumoZ3=0.0;
    double sumX4=0.0;
    double sumoZ4=0.0;
    if(newbreakpointnumber!=(KStar_epsilon-1)){
    if (u==2)
      {
	for(int i=1;i<=I;i++){
	for (int t=breakpointsStar_epsilon[newbreakpointnumber-1]; t<breakpointsStar_epsilon[newbreakpointnumber];t++)
	  {
	    sumX1+=S[i][t];
	    sumoZ1+=(omega[i][t]);
	  }
	for (int t=breakpointsStar_epsilon[newbreakpointnumber]; t<breakpointsStar_epsilon[newbreakpointnumber+1];t++)
	  {
	    sumX2+=S[i][t];
	    sumoZ2+=(omega[i][t]);
	  }
	}
	sumX3=sumX1+sumX2;
	sumoZ3=sumoZ1+sumoZ2;
      }
    }
    if(removebreakpoint!=(KStar_epsilon)){
    if (u==1)
      {
	for(int i=1;i<=I;i++){
	for (int t=breakpoints_epsilon[removebreakpoint-1]; t<breakpoints_epsilon[removebreakpoint];t++)
	  {
	    sumX3+=S[i][t];
	    sumoZ3+=(omega[i][t]);
	  }
	for (int t=breakpoints_epsilon[removebreakpoint]; t<breakpoints_epsilon[removebreakpoint+1];t++)
	  {
	    sumX4+=S[i][t];
	    sumoZ4+=(omega[i][t]);
	  }
	}
	sumX1=sumX3+sumX4;
	sumoZ1=sumoZ3+sumoZ4;
      }
    }
    if(newbreakpointnumber==(KStar_epsilon-1)){
    if (u==2){
	for(int i=1;i<=I;i++){
	for (int t=breakpointsStar_epsilon[newbreakpointnumber-1]; t<breakpointsStar_epsilon[newbreakpointnumber];t++)
	  {
	    sumX1+=S[i][t];
	    sumoZ1+=(omega[i][t]);
	  }
        if(breakpoints_epsilon[newbreakpointnumber]!=(n+1)){
	for (int t=breakpointsStar_epsilon[newbreakpointnumber]; t<(breakpointsStar_epsilon[newbreakpointnumber+1]-1);t++)
	  {
	    sumX2+=S[i][t];
	    sumoZ2+=(omega[i][t]);
	  }
	}
	}
	sumX3=sumX1+sumX2;
	sumoZ3=sumoZ1+sumoZ2;
      }
    }
    if(removebreakpoint==(KStar_epsilon)){
      if (u==1){
	for(int i=1;i<=I;i++){
	for (int t=breakpoints_epsilon[removebreakpoint-1]; t<breakpoints_epsilon[removebreakpoint];t++)
	  {
	    sumX3+=S[i][t];
	    sumoZ3+=(omega[i][t]);
	  }
        if(breakpoints_epsilon[removebreakpoint]!=(n+1)){
	for (int t=breakpoints_epsilon[removebreakpoint]; t<(breakpoints_epsilon[removebreakpoint+1]-1);t++)
	  {
	    sumX4+=S[i][t];
	    sumoZ4+=(omega[i][t]);
	  }
        }
	}
	sumX1=sumX3+sumX4;
	sumoZ1=sumoZ3+sumoZ4;

      }
    }

    // cout << sumX1 << sumX2 << sumX3 << sumX4 << sumoZ1 << sumoZ2 << sumoZ3 << sumoZ4 << endl;




   alpha_ep = epsilon_a;
   beta_ep = epsilon_b;
   if(xi_estim_epsilon){
   beta_ep = xi_epsilon;

   }
    sumX1+=alpha_ep;
    sumX2+=alpha_ep;
    sumX3+=alpha_ep;
    sumX4+=alpha_ep;
    sumoZ1+=beta_ep;
    sumoZ2+=beta_ep;
    sumoZ3+=beta_ep;
    sumoZ4+=beta_ep;

    // cout << "xi_epsilon  " << xi_epsilon <<endl;
	
    double accbr = alpha_ep*log(beta_ep)-gsl_sf_lngamma(alpha_ep); // epsilon_a epsilon_b
    if(K_geom){
      accbr += log(1-p_K);
    }
    accbr=accbr*pow(-1,u)+log(v);

    if (u==2)
      {
	accbr=accbr+gsl_sf_lngamma(sumX1)+gsl_sf_lngamma(sumX2)-gsl_sf_lngamma(sumX3);
        accbr=accbr-sumX1*log(sumoZ1)-sumX2*log(sumoZ2)+sumX3*log(sumoZ3);
     }
    if (u==1)
      {
	accbr=accbr+gsl_sf_lngamma(sumX1)-gsl_sf_lngamma(sumX3)-gsl_sf_lngamma(sumX4);
	accbr=accbr-sumX1*log(sumoZ1)+sumX3*log(sumoZ3)+sumX4*log(sumoZ4);
      }
    if (gsl_rng_uniform (r) <= exp(accbr)) {
        K_epsilon = KStar_epsilon; 
        for(int k=0;k<=KStar_epsilon;k++){
          breakpoints_epsilon[k]=breakpointsStar_epsilon[k];
        }
        acceptedbr_epsilon++;

    }


    Km1_epsilon=K_epsilon-1;

    // cout << "accbr" << accbr << endl;




    //update xi_epsilon
    if(xi_estim_epsilon){
	double a = alpha_xi + epsilon_a*K_epsilon;
	double b = beta_xi;
        for(int k=1;k<=K_epsilon;k++){
	  b += epsilon_br[k];
	}
	xi_epsilon = gsl_ran_gamma (r, a, 1/b);
    }else{
      xi_epsilon = epsilon_b;
    }

    //  cout << "xi_epsilon  " << xi_epsilon << endl;
    // cout << "epsilon_br[1]  " << epsilon_br[1] << endl;






    //update timevariing epsilon with breakpoints_epsilon via reversible jump MCMC


        for(int k=1;k<=(K_epsilon-1);k++){
	  double a = alpha_ep;
	  double b = xi_epsilon;
          for(int i=1;i<=I;i++){
	  for(int t=breakpoints_epsilon[k-1];t<breakpoints_epsilon[k];t++){ //breakpoints_epsilon = 2,...,n+2
	    a += S[i][t];
	    b += omega[i][t];
	  }
          }
          epsilon_br[k] = gsl_ran_gamma (r, a, 1/b);
	  for(int t=breakpoints_epsilon[k-1];t<breakpoints_epsilon[k];t++){
	    epsilon[t] = epsilon_br[k]; //calculate the epsilon[t] from the epsilon_br[k]
	  }

        }


	//cout << "nu[1][2]   " << nu[1][2] << endl;
	//cout << "epsilon[2]   " << epsilon[2] << endl;

	a = alpha_ep;
	double b = xi_epsilon;
	//	cout << a << "           " << b << endl;
	if(breakpoints_epsilon[K_epsilon-1]<(n+1)){ 
          for(int i=1;i<=I;i++){
	  for(int t=breakpoints_epsilon[K_epsilon-1];t<(breakpoints_epsilon[K_epsilon]-1);t++){ //breakpoints_epsilon = 2,...,n+2
	    a += S[i][t];
	    b += omega[i][t];
	  }
	  }
	}
          epsilon_br[K_epsilon] = gsl_ran_gamma (r, a, 1/b);
	  for(int t=breakpoints_epsilon[K_epsilon-1];t<breakpoints_epsilon[K_epsilon];t++){
	    epsilon[t] = epsilon_br[K_epsilon]; //calculate the epsilon[t] from the epsilon_br[k]
	  }
	  //	cout << a << "           " << b << endl;

	  // 	  cout << epsilon_br[1] << endl;





  return;
}











/*********************************************************************
 * Read integer data from file. The format is: n Z_1 Z_2 ... Z_n
 *********************************************************************/

  long** readData(char* fileName, long *size, long *size2) {
  //Read Data from file and check if everything went allright

  //cout << "\"" << fileName  << "\"" <<  endl;

  ifstream fin(fileName);
  //ifstream fin("z:/PoissonModel/Artikel-varnu-varla/lambda-rev/article-program/data/hepatitisA.txt");
  
  if (!fin) {
    cerr << "Error: File \"" << fileName << "\" not found!" << endl;
    return(NULL);
  }
  
  // cout << fin << endl;
  //Count the number of data entries
  int n=0;
  fin >> n;
  cout << "n=" << n << endl;
  int I=1; 
  //fin >> I;
  //cout << "I=" << I << endl;

  long **Z = new long*[I+1];
  for (register long i=0; i<=I; i++){
	Z[i] = new long[n+1];
  }

	
  for (register long t=0; t<=n; t++){
	Z[0][t]=0;
  }
	
  for (register long i=0; i<=I; i++){
	Z[i][0]=0;
  }
	
  //Start @ index 1. (Z[0] is not defined)
  int t=1;
	while (!fin.eof() && (t<=n)) { 	
	  int i=1;
	  while (!fin.eof() && (i<=I)) { 
      fin >> Z[i][t];
      i++;
	  }
    t++;
	}

  fin.close();
  


  //for(long t=0; t<=10; t++){
  //cout <<  Z[1][t] << endl;
  //}
  //Return the result consisting of Z and n
  *size = n;
  *size2 = I;



  return(Z);
  
  }

 
/*********************************************************************
 * Read integer and xi data from file.
 *********************************************************************/
/*
  double* readData(char* fileName, long I) {
  //Read xi Data from file and check if everything went allright

  double *xi = new double[I+1];

  xi[0]=0;
 
  if(I==1){xi[1]=1;}
  else{
  ifstream fin(fileName);
  if (!fin) {
    cerr << "Error: File \"" << fileName << "\" not found!" << endl;
    return(NULL);
  }


  //Start @ index 1. (xi[0] is not defined)	
	  int i=1;
	  while (!fin.eof() && (i<=I)) { 
      fin >> xi[i];
      i++;
	  }

  fin.close();
  }//else

  //for (register int i=1; i<=I; i++) {
  //cout << xi[i] << endl;;
  //}


  return(xi);
  }
*/




/* Calculate the deviance of the data we use that the data, Z, is a
 *  sum of Poisson distributed variables, i.e. it is Poisson
 *  distributed.
 *
 *    Z_t = S_t + X_t + Y_t, i.e. 
 *    Z_t ~ Po(nu*p + nu*(1-p) + lambda*W_{t-1})
 *
 *    D = -2log p(Z|theta) + 2 log p(Z|\mu(theta)=Z)
 */
double satdevalt(long n, long I, long **X, long **Y, long **Z, 
	      double **omega, double** lambda, double **nu, double *xi, double** eta, double** eta2, double** varr, double psi, int overdispersion) {
  double res = 0;
  //Loop over all data
  for (register int i=1; i<=I; i++) {
    for (register int t=2; t<=n; t++) {
      //Use the equation derived for the saturated deviance in the paper
      //calculate the mean and variance of Z[i][t]
      eta[i][t] = (nu[i][t]*xi[i]+lambda[i][t]*Z[i][t-1]);
      eta2[i][t] = eta[i][t];
      if(overdispersion){
        varr[i][t] = eta2[i][t]*(1+eta2[i][t]/psi);
      }else{
	varr[i][t] = eta2[i][t];
      }
      //calculate the Deviance in the Poisson and NegBin case
      if(!overdispersion){
        if (Z[i][t] == 0) {
          res += 2 * eta[i][t];
        } else {
          res += 2 * ( Z[i][t] * log(Z[i][t]/eta[i][t]) - Z[i][t] + eta[i][t]);
        }
      }
      if(overdispersion){
        if (Z[i][t] == 0) {
          res += 2 * ( - (Z[i][t]+psi) * log((Z[i][t]+psi)/(eta[i][t]+psi)));
        } else {
          res += 2 * ( - (Z[i][t]+psi) * log((Z[i][t]+psi)/(eta[i][t]+psi)) + Z[i][t] * log(Z[i][t]/eta[i][t]));
        }
      }
    }
  }
  return(res);
}


/* Calculate the deviance of the data we use that the data, Z, is a
 *  sum of Poisson distributed variables, i.e. it is Poisson
 *  distributed.
 *
 *    Z_t = X_t + Y_t, i.e. 
 *    Z_t ~ Po(nu_t + lambda_t*Z_{t-1})
 *
 *    D = -2log p(Z|theta)
 */
double satdev(long n, long I, long **Z, 
	      double** lambda, double **nu, double *xi, double *epsilon, double** eta, double psi, int overdispersion) {
  double res = 0;
  //Loop over all data
  for (register int i=1; i<=I; i++) {
    for (register int t=2; t<=n; t++) {
      //Use the equation derived for the saturated deviance in the paper
      //calculate the mean and variance of Z[i][t]
      eta[i][t] = (epsilon[t] + nu[i][t]*xi[i]+lambda[i][t]*Z[i][t-1]);
      //calculate the Deviance in the Poisson and NegBin case
      if(!overdispersion){
        res -= 2 * ( Z[i][t] * log(eta[i][t]) - gsl_sf_lngamma(Z[i][t]+1) - eta[i][t]);
      }
      if(overdispersion){
          res -= 2 * ( gsl_sf_lngamma(Z[i][t]+psi) - gsl_sf_lngamma(Z[i][t]+1) - gsl_sf_lngamma(psi) - (Z[i][t]+psi)*log(eta[i][t]+psi) + psi*log(psi) + Z[i][t]*log(eta[i][t]));
      }
    }
  }
  return(res);
}



// Calculate chi square the sum of the qudratic pearson residuals (z-mean)/sd


double chisq(long n, long I, long **Z, 
	      double** lambda, double **nu, double *xi, double *epsilon, double** eta, double** varr, double** rpearson, double psi, int overdispersion) {
  double res = 0;
  //Loop over all data
  for (register int i=1; i<=I; i++) {
    for (register int t=2; t<=n; t++) {
      //calculate the mean and variance of Z[i][t]
      eta[i][t] = (epsilon[t] + nu[i][t]*xi[i]+lambda[i][t]*Z[i][t-1]);
      if(overdispersion){
        varr[i][t] = eta[i][t]*(1+eta[i][t]/psi);
      }else{
	varr[i][t] = eta[i][t];
      }
      rpearson[i][t] = (Z[i][t]-eta[i][t])/sqrt(varr[i][t]);
      //calculate chisq in the Poisson and NegBin case
      res += rpearson[i][t]*rpearson[i][t];
    }
  }
  return(res);
}
  







/**********************************************************************
 * Estimation in the basic epidemic model
 *
 */
void bplem_estimate(int verbose, ofstream &logfile, ofstream &logfile2, ofstream &acclog, long** Z, double* xi, long n, long I, long T, long nfreq, long burnin, long filter,
		    long samples, int rw) {
  //Model parameters - start values
  double nu_const = alpha_nu/beta_nu;
  double lambda_const = 0.5;
  double psi    = alpha_psi / beta_psi;
  double x      = logit(lambda_const);
  
  if(!verbose) {
    cout << "------------------------------------------------" << endl;
    if (!la_rev){
    cout  << "lambda:  Ga(" << alpha_lambda << "," << beta_lambda << ")-->\t" 
	 << lambda_const << endl;
    }
         if(!varnu){
    cout << "nu:      Ga(" << alpha_nu << "," << beta_nu << ")-->\t" 
 	 << nu_const << endl;
         }
	 if(overdispersion){
    cout << "psi:     Ga(" << alpha_psi << "," << beta_psi << ")-->\t" 
	 << psi << endl;
	 }
    cout << "------------------------------------------------" << endl;
  }
  

  //Allocate arrays for all latent variables and initialize them
  	
	long** X = new long*[I+1];
	long** Y = new long*[I+1];
	long** S = new long*[I+1];
	double **omega= new double*[I+1];
	double** sumX = new double*[I+1];
	double** sumY = new double*[I+1];
	double** sumS = new double*[I+1];
	double **sumomega= new double*[I+1];
        double **nu= new double*[I+1];
	double *alpha=new double[I+1];
	double* beta= new double[n+1];
	double **lambda=new double*[I+1];
	double **lambda_br=new double*[I+1];
	double **eta=new double*[I+1];
	double **eta2=new double*[I+1];
	double **varr=new double*[I+1];
	double **rpearson=new double*[I+1];
	double **Sumeta=new double*[I+1];
	double **Sumvarr=new double*[I+1];
	double **Sumrpearson=new double*[I+1];
	double *delta=new double[n+2];
	double *delta_br=new double[n+2];
        double xi_delta = 1;
	double *epsilon=new double[n+2];
	double *epsilon_br=new double[n+2];
        double xi_epsilon = 1;
	double xi_psi = 1;

	int **breakpoints=new int*[I+1];
	int **breakpointsStar=new int*[I+1];
        long **bp=new long*[I+1];
	int *K=new int[I+1];
        int *Km1=new int[I+1];
	int *KStar=new int[I+1];
        double* xi_lambda=new double[I+1];
	int *breakpoints_delta=new int[n+2];
	int *breakpointsStar_delta=new int[n+2];
        long *bp_delta=new long[n+2];
	int K_delta;
        int Km1_delta;
	int KStar_delta;
	int *breakpoints_epsilon=new int[n+2];
	int *breakpointsStar_epsilon=new int[n+2];
        long *bp_epsilon=new long[n+2];
	int K_epsilon;
        int Km1_epsilon;
	int KStar_epsilon;
	for (register long i=0; i<=I; i++){
	X[i]=new long[n+1];
	Y[i]=new long[n+1];
	S[i]=new long[n+1];
        omega[i]=new double[n+1];
	sumX[i]=new double[n+1];
	sumY[i]=new double[n+1];
	sumS[i]=new double[n+1];
        sumomega[i]=new double[n+1];
        nu[i]=new double[n+1];
	lambda[i]=new double[n+2];
	lambda_br[i]=new double[n+2];
	breakpoints[i]=new int[n+2];
	breakpointsStar[i]=new int[n+2];
        bp[i]=new long[n+2];
	eta[i]=new double[n+1];
	eta2[i]=new double[n+1];
	varr[i]=new double[n+1];
	rpearson[i]=new double[n+1];
	Sumeta[i]=new double[n+1];
	Sumvarr[i]=new double[n+1];
	Sumrpearson[i]=new double[n+1];
	}
        long* Xnp1 = new long[I+1];
        long* Snp1 = new long[I+1];
        long* Ynp1 = new long[I+1];
        long* Znp1 = new long[I+1];	
        double* omeganp1 = new double[I+1];
        double* nunp1 = new double[I+1];


  if(!varnu){
    for (register int i=0; i<=I; i++) {
      for (register int t=0; t<=n; t++) {
        nu[i][t] = alpha_nu/beta_nu;
      }
    }
  }

        for (register int i=0; i<=I; i++) {
      for (register int t=0; t<=n; t++) {
        lambda[i][t] = lambda_const;
      }
    }

  for (register int i=0; i<=I; i++) {
    for (register int t=0; t<=n; t++) {
      X[i][t] = 0; S[i][t] = 0; Y[i][t] = Z[i][t]; omega[i][t] = 1; eta[i][t] = 0; bp[i][t] = 0; bp_delta[t] = 0; bp_epsilon[t] = 0;
      sumX[i][t] = 0;  sumY[i][t] = 0;  sumS[i][t] = 0; sumomega[i][t] = 0; Sumeta[i][t] = 0; Sumrpearson[i][t] = 0;
    }
    bp[i][n+1] = 0; xi_lambda[i] = 1;
    bp_delta[n+1] = 0; bp_epsilon[n+1] = 0;
  }


	// F�r Saisonkomponenente
	int ncov;
	int scov = 0;
	if(delta_rev){
	  scov = 1;
	}

	double* gamma;
	double* gammaneu;
	double** xcov;
	if(!nu_trend){
	ncov=nfreq*2+1;
	gamma = new double[ncov];
	gammaneu = new double[ncov];
	xcov = new double*[ncov];
	for (int i=0; i<ncov; i++)
	  {
	    xcov[i]=new double[n+2];
	  }
  if(varnu){
	for (int t=2; t<=(n+1); t++)
	  {
	    xcov[0][t]=1.0;
	  }
	
	for (int i=1; i<=nfreq; i++)
	  {
	    for (int t=2; t<=(n+1); t++)
	      {
		xcov[i*2-1][t]=sin(2*PI*(t-1)*i/T); //schwingung um einen Zeitpunkt nach hinten verschoben. beginnt bei t=2
		xcov[i*2][t]=cos(2*PI*(t-1)*i/T);

	      } 
	    // cout << endl;
	  }
  } //if varnu
	} //if !nu_trend
	// Saisonkomponente mit linearem trend
else{
	ncov=nfreq*2+2;
	gamma = new double[ncov];
	xcov = new double*[ncov];
	for (int i=0; i<ncov; i++)
	  {
	    xcov[i]=new double[n+2];
	  }
  if(varnu){
	for (int t=2; t<=(n+1); t++)
	  {
	    xcov[0][t]=1.0;
	    xcov[ncov-1][t]=t-n/2;
	  }
	
	for (int i=1; i<=nfreq; i++)
	  {
	    for (int t=2; t<=(n+1); t++)
	      {
		xcov[i*2-1][t]=sin(2*PI*(t-1)*i/T); //schwingung um einen Zeitpunkt nach hinten verschoben. beginnt bei t=2
		xcov[i*2][t]=cos(2*PI*(t-1)*i/T);

	      } 
	    //cout << endl;
	  }
  } //if varnu
} // if nu_trend



	// Regionenanteil
	double* xreg=new double[I+1];
	if(varnu){
	for (int i=1; i<=I; i++)
	  {
	    xreg[i]=log(xi[i]);
	    xi[i]=1;
	    //	    cout << xreg[i]<<endl;
	  }
	}
	double taualpha=alpha_a/alpha_b;
	double taubeta=beta_a/beta_b;
	double taubetaStar;
        //double taubeta=beta_a/beta_b;
	double taugamma=gamma_a/gamma_b;
	

	
  

	//  if(la_rev){
    for(int i=0; i<=I; i++){
      K[i]=1;
      Km1[i]=0;
      KStar[i]=1; 
      breakpoints[i][0]=2;
      breakpoints[i][1]=n+2;
      breakpointsStar[i][0]=2; 
      breakpointsStar[i][1]=n+2; 
      for(int j=0; j<=(n+1); j++){
	lambda_br[i][j]= 0;
      }
      for(int t=2; t<=(n+1); t++){ 
	lambda[i][t]= 0; 
      } 
    }
  
    //  }

  //  if(delta_rev){
      K_delta=1;
      Km1_delta=0;
      KStar_delta=1; 
      breakpoints_delta[0]=2;
      breakpoints_delta[1]=n+2;
      breakpointsStar_delta[0]=2; 
      breakpointsStar_delta[1]=n+2; 
      for(int j=0; j<=(n+1); j++){
	delta_br[j]= 1;
      }
      for(int t=0; t<=(n+1); t++){ 
	delta[t]= 1; 
      } 
      //  }


      //  if(epsilon_rev){
      K_epsilon=1;
      Km1_epsilon=0;
      KStar_epsilon=1; 
      breakpoints_epsilon[0]=2;
      breakpoints_epsilon[1]=n+2;
      breakpointsStar_epsilon[0]=2; 
      breakpointsStar_epsilon[1]=n+2; 
      for(int j=0; j<=(n+1); j++){
	epsilon_br[j]= 0;
      }
      for(int t=0; t<=(n+1); t++){ 
	epsilon[t]= 0; 
      } 
      // }


  if(varnu){ 
    for (register int i=0; i<=I; i++) {
      alpha[i] = log(xreg[i]);
    }
 
    for (register int t=0; t<=n; t++) {
      beta[t] = 0.0;
    }
    for (register int j=0; j<ncov; j++) {
      gamma[j] = 0.0;
    }
    
    double Zmin = Z[1][2];
    for(int i=1;i<=I;i++){
      for(int t=2;t<=n;t++){
	if(Z[i][t]<Zmin){
	  Zmin = Z[i][t];
	}
      }
    }

    // gamma[0] =  log(alpha_nu/beta_nu);
    if(scov==0){
    gamma[0] = log(Zmin+1); //  //
    }
    machnu(gamma, alpha, beta,delta, nu, I, n, ncov, xcov,scov);

  }//if varnu  
  

    //Vectors for betaupdate
    int bandw=rw+1;


    my=new double[n+1];
    my2=new double[n+1];
    temp=new double[n+1];
    z=new double[n+1];
    theta=new double[n+1];
    beta0=new double[n+1];
    Q=new double[(n-1)*bandw];
    Q2=new double[(n-1)*bandw];
    L=new double[(n-1)*bandw];
    L2=new double[(n-1)*bandw];
    P=new double[ncov*ncov];	
    P2=new double[ncov*ncov];
    gammaalt=new double[ncov];
    z2=new double[ncov];
    n1 = n-1;
    n2 = n-2;


	

  for (register long i=1;i<=I; i++) {
    X[i][2] = (long)floor(nu[i][2]);
    Y[i][2] = (long)floor(lambda[i][2]*nu[i][2]/(1 - lambda[i][2]));
    omeganp1[i] = 1;
  }

	
	
  //Variables for statistics
  double acceptedPsi = 0;
  double acceptedlambda = 0;
  double acceptedbr = 0;
  double acceptedbr_delta = 0;
  double acceptedbr_epsilon = 0;
  long acc_beta=0;
  long acc_alpha=0;
  long acc_gamma=0;


  double accratelambda = 0;
  double accratebr = 0;
  double accratepsi = 0;
	
  /*hoehle: min/mac is deprecated
    double tuneSampleSize = 1000<?burnin; 
  */
  double tuneSampleSize = MIN(1000,burnin);

  double need = 0; 
  double tunex = 0; 
  double tunepsi = 0; 
  double tunetaubeta = 0; 

  
  //Write the header to the logfile
  logfile << "i\t";
  if (!la_rev&&la_estim)
    {
  logfile << "lambda\t";
    }
  logfile << "psi\t";
  logfile << "xipsi\t";

  if(!varnu){
    logfile << "nu\t";
  }

  if(varnu){
  for (register long j=0;j<ncov;j++) {
    logfile << "gamma[" << j << "]\t";
  }
    if(delta_rev){
      logfile << "Kdelta\t" << "xidelta\t";
      for (register long j=2; j<=n; j++) {
        logfile << "delta[" << j << "]\t";
      }
    }
  }//if varnu

    if(epsilon_rev){
      logfile << "Kepsilon\t" << "xiepsilon\t";
      for (register long j=2; j<=n; j++) {
        logfile << "epsilon[" << j << "]\t";
      }
    }


  if (la_rev&&la_estim)
    {
	  logfile << "K\t";
	  logfile << "xilambda\t";
      for (int t=2; t<=n; t++)
	{
	  logfile << "lambda[" << t <<"]\t";
	}
	}
   logfile << "Znp1\t";
  logfile << "D\t" << endl;


  //Write the header to the logfile2
    for (register long t=1;t<=n;t++) {
      logfile2 << "X[" << t << "]\tY[" << t << "]\tomega["
  	<< t << "]\tbp[" << t <<"]\t";
    }
  logfile2 << "bp[" << n+1 <<"]\t";
  logfile2 << endl;


  //Calculate the necessary number of samples.
  long sampleSize = filter*samples + burnin;
  if (!verbose) {
    cout << "Total number of samples = " << burnin << "+" << filter << "*" << samples << "= " << sampleSize << endl;
    //if (overdispersion) {
    //cout << "(overdispersion)" << endl;
    //}else {
    //cout << "(no overdispersion)" << endl;
    //}
  }


  //Loop over samples - start at 1
register long sampleCounter=1;
  while ( sampleCounter<=sampleSize) {
 
    if ((!verbose) && ((sampleCounter % 10) == 0)) {
      cout <<"."<<flush;
    }
    //Progress bar: 0,..,100% is shown.
    if (sampleCounter > tuneSampleSize && (!verbose) && (sampleCounter % (long)floor(sampleSize/100.0) == 0)) {
      cout << sampleCounter*100 / sampleSize << "% " << flush;
    }
    if(0){ 
    if(varnu){     
    if ((sampleCounter % 100 == 0)) {
          cout<< "alpha\t" << (double)acc_alpha/I<<"  "
      << "beta\t" <<" "<< beta[2] <<" "<< (double)acc_beta<<"  "
      << "gamma[0]\t" <<" "<< gamma[0] <<" "<< "gamma[1]\t" <<" "<< gamma[1] <<" "<< "gamma[2]\t" <<" "<< gamma[2] <<" "<< (double)acc_gamma<<"  "
	      << "lambda\t" << lambda[1][2] << endl;
    }
    } 
    if(la_rev){     
    if ((sampleCounter % 100 == 0)) {

      cout<< "K\t" << K[1] << endl;

    }
    }

    if(delta_rev){     
    if ((sampleCounter % 100 == 0)) {

      cout<< "K_delta\t" << K_delta << "  " << "delta[2]\t" << delta[2] << endl;


    }
    }
    if(epsilon_rev){     
    if ((sampleCounter % 100 == 0)) {
      cout<< "K_epsilon\t" << K_epsilon << "  " << "epsilon[2]\t" << epsilon[2] << endl;
    }
    }
    }

    //    cout << ":"<<flush;
    //Temporary variables
  
    double a,b,c,binp;
    
    //Calculate sums
    double XSum = sumIn2(X,I,n);
    double YSum = sumIn2(Y,I,n);
 
    double xiSum = 0;
    for (register long i=1;i<=I; i++) {
      xiSum = xiSum + xi[i];
    }

        if(!la_estim){
    for (register long i=1;i<=I; i++) {
      for (register long t=2;t<=n; t++) { 
        lambda[i][t] =  0;
      }
    }
    if(sampleCounter%3==0){
    acceptedlambda++;
    }
	}
    
    if(la_estim){
    if (!la_rev)
      {
    //Update x=logit(lambda) mit random walk proposal fur variables nu
    double xStar;
    xStar = gsl_ran_gaussian(r,xRWSigma) + x;
    double lambdaStar;
    lambdaStar = invlogit(xStar);
    double logFx = 0;
    logFx = log(gsl_ran_beta_pdf(lambda_const,alpha_lambda,beta_lambda)) + log(invlogitd(x));
    double logFxStar = 0;
    logFxStar = log(gsl_ran_beta_pdf(lambdaStar,alpha_lambda,beta_lambda)) + log(invlogitd(xStar));
    for (register long i=1;i<=I; i++) {
      for (register long t=2;t<=n; t++) {
        logFx = logFx + gsl_ran_poisson_log_pdf(Y[i][t], lambda_const*omega[i][t]*Z[i][t-1]);
        logFxStar = logFxStar + gsl_ran_poisson_log_pdf(Y[i][t], lambdaStar*omega[i][t]*Z[i][t-1]);
      }
    }
    double accx = exp(logFxStar - logFx);
   
    if (gsl_rng_uniform (r) <= accx) {x = xStar; lambda_const = invlogit(xStar); acceptedlambda++;}
    for (register long i=1;i<=I; i++) {
      for (register long t=2;t<=n; t++) { 
        lambda[i][t] =  lambda_const;
      }
    }
      }
    }

    if(!varnu){

      double omegaxiSum = 0;
    for (register long i=1;i<=I; i++) {
      for (register long t=2;t<=n; t++) { 
	omegaxiSum += omega[i][t]*xi[i];
      }
    }
    
    //Update nu
    a  = alpha_nu + XSum;
    b  = beta_nu + omegaxiSum;
    nu_const =  gsl_ran_gamma (r, a, 1/b);
    for (register long i=1;i<=I; i++) {
      for (register long t=2;t<=n; t++) { 
        nu[i][t] =  nu_const;
      }
    }
    }
    
  



    ///////////////////////////////////////////////////////////////////////////
    //In case of over-dispersion log(psi) has to be updated using a Random Walk
    //Metropolis-Hastings Step.
		
    if (overdispersion) {
      double a_psi = alpha_psi;
      double b_psi = beta_psi;
      if(xi_estim_psi){
	a_psi = 1;
	b_psi = xi_psi;
      }
      //loglik * prior of the old value
      double logFPsi = gsl_ran_gamma_log_pdf(psi,a_psi,1/b_psi) + log(psi);
      for (register long i=1; i<=I; i++) {
				for (register long t=2; t<=n; t++){
				logFPsi += gsl_ran_gamma_log_pdf(omega[i][t],psi,1/psi);
				}
      }
      //Generate the new value from Gaussian distrib N(psi, psiRWSigma^2)
      double logpsiStar = gsl_ran_gaussian(r,psiRWSigma) + log(psi);
      double psiStar = exp(logpsiStar);
      //loglik * prior of the new value
      double logFPsiStar = gsl_ran_gamma_log_pdf(psiStar,a_psi,1/b_psi) + logpsiStar;
      for (register long i=1; i<=I; i++) {
			 for (register long t=2; t<=n; t++) {
				 logFPsiStar += gsl_ran_gamma_log_pdf(omega[i][t],psiStar,1/psiStar);
				}
      }
      //Acceptance prob - fmin(1, <>) superflous.
      double accpsi = exp(logFPsiStar-logFPsi);
      
      //Do we accept?
      if ((psi>0) && (gsl_rng_uniform (r) <= accpsi)) {psi = psiStar; acceptedPsi++;}
    }

    //update xi_psi
    if(xi_estim_psi){
      double a = alpha_psi + 1;
      double b = beta_psi + psi;
      xi_psi = gsl_ran_gamma (r, a, 1/b);
    }
    //////////////////////////////////////////////////////////////////////////

    

    //State information to file if we are on an filter'th sample
   
    if ((sampleCounter>burnin) && ((sampleCounter-burnin) % filter == 0)) {
      logfile << sampleCounter << "\t";
      if (!la_rev){
	logfile << lambda_const  << "\t";
      }
      logfile << psi << "\t";
      logfile << xi_psi << "\t";
      if(!varnu){
	logfile << nu_const << "\t";
      }
    }

    if(varnu){
// Unterprogramme f�r den Update von alpha und beta
      if (I>=2)
	{
	  alphaupdate(gamma, alpha, beta, delta, lambda, 1, I, n, Y, X, acc_alpha, taualpha, ncov, xcov, xreg, omega, omega, scov,1);
    taualpha=update_tau_alpha(alpha, I, alpha_a, alpha_b, xreg);

        if (sampleCounter%3==0)
          {
	    if(scov==0){
   	    double asum=0;
	    for (int i=1; i<=I; i++)
	      {
	        asum+=(alpha[i]-xreg[i]);
	      }
	    for (int i=1; i<=I; i++)
	      {
	        alpha[i]-=(asum/I);
	      }
	    gamma[0]=gamma[0]+(asum/I);
	    }
          }
	}
      else 
	{
	  alpha[1]=0.0;
	}


//Update f�r zeitlichen effekt mit RW

     if (rw>0)
       {

	 //  update_beta_nurrw(gamma, alpha, beta, delta, X, Z, Y, n, I,  taubeta,  rw, 1, lambda, acc_beta, sampleCounter, my, my2, temp, z, theta, Q, Q2, L, L2, xcov, ncov, scov, omega, omega, 1);
	 //update_beta_block(alpha, beta, gamma, delta, X, n, I, taubeta, rw, acc_beta, sampleCounter, n1, n2, my, my2, z, theta, beta0, Q, Q2, L, L2, xcov, ncov, scov, omega);

	 update_beta_tau_block(alpha, beta, gamma, delta, beta_a, beta_b, X, n, I, taubeta, rw, acc_beta, taubetaRWSigma, taubetaStar, sampleCounter, n1, n2, my, my2, z, theta, beta0, Q, Q2, L, L2, xcov, ncov, scov, omega);

	  //taubeta=beta_a/beta_b;
	 // taubeta=hyper(rw, beta, beta_a, beta_b, n);
	   //taubeta=720;

	 //if(sampleCounter%500==1){cout << taubeta << endl << endl;}
	     //  for(int t=2;t<=n;t++){
	     //  update_beta_t(t, alpha, beta, gamma, delta, ncov, xcov, X, n, I, taubeta, acc_beta, omega, scov);
	     //   }
  
	 
       if(scov==0){
       //  if (sampleCounter%1==0)
       //   {
   	    double bsum=0;
	    for (int t=2; t<=n; t++)
	      {
	        bsum+=(beta[t]);
	      }
	    for (int t=2; t<=n; t++)
	      {
	        beta[t]-=(bsum/(n-1));
	      }
	    gamma[0]=gamma[0]+(bsum/(n-1));
	    //    }
       }
	 
       } //if (rw>0)


     //update saison

     //update_gamma( alpha,  beta, gamma,ncov,  xcov,  X,  Z, Y, n, I, taugamma, 1, lambda, acc_gamma, P, P2, gammaalt, z2, L, Q, omega, omega,1);
    taugamma=gamma_b;
    // cout << gamma[0]<<"  "  << gamma[1] << endl;
    for(int j=scov;j<ncov;j++){
      update_gamma_j(j, alpha, beta, gamma, delta, ncov, xcov, X, n, I, taugamma, gammaneu, acc_gamma, omega, scov);
      // gamma[j]=0;
      }
    //    cout << gamma[0]<<"  "  << gamma[1] << endl;
    //cout << "lambda [1][2]   " << lambda [1][2] << endl;
    //cout << "X [1][2]   " << X [1][2] << endl;
    //cout << "omega [1][2]   " << omega [1][2] << endl;
    // update delta_br
    if(delta_rev){
      update_delta_br(delta, delta_br, xi_delta, breakpoints_delta, breakpointsStar_delta, K_delta, KStar_delta, Km1_delta, delta_a, delta_b, X, nu, n, I, acceptedbr_delta, omega, xi_estim_delta, K_geom, p_K, alpha_xi, beta_xi);
  }
    //cout << delta[2]<<"  " << K_delta << endl;

// Berechnet nu_it=log(alpha_i+beta_t)
       machnu(gamma, alpha, beta,  delta, nu, I, n, ncov, xcov, scov);

       //cout << nu[1][2] << endl << endl;
       //cout << beta[2] << endl << endl;
       //       cout << "test" << endl;
       //       cout << "test2" << endl;


  if ((sampleCounter>burnin) && ((sampleCounter-burnin) % filter == 0)) {
//     for (register long i=1;i<=I; i++) {
//       for (register long t=1; t<=n; t++) {
// 	 logfile << nu[i][t] << "\t";
//       }
//     }
    //    logfile << mu << "\t";
    for (register long j=0; j<ncov; j++) {
	 logfile << gamma[j] << "\t";
    }
    if(delta_rev){
      if ((sampleCounter>burnin) && ((sampleCounter-burnin) % filter == 0)) {
        logfile << Km1_delta<<"\t"<< xi_delta<<"\t";
	    for (register long j=2; j<=n; j++) {
     	     logfile << delta[j] << "\t";
        }
      }

      if (sampleCounter>burnin) {
        for (register long k=1; k<=K_delta; k++) {
          for (register long j=2; j<=n; j++) {
            if (breakpoints_delta[k]==j){
  	      bp_delta[j]+=1;
            }
          }
        }
      }
    }//if(delta_rev)

  }//if

    }//if varnu

     if(epsilon_rev){
	 update_epsilon_br(epsilon, epsilon_br, xi_epsilon, breakpoints_epsilon, breakpointsStar_epsilon, K_epsilon, KStar_epsilon, Km1_epsilon, epsilon_a, epsilon_b, S, n, I, acceptedbr_epsilon, omega, xi_estim_epsilon, K_geom, p_K, alpha_xi, beta_xi);

      if ((sampleCounter>burnin) && ((sampleCounter-burnin) % filter == 0)) {
        logfile << Km1_epsilon<<"\t"<< xi_epsilon<<"\t";
	    for (register long j=2; j<=n; j++) {
     	     logfile << epsilon[j] << "\t";
        }
      }

      if (sampleCounter>burnin) {
        for (register long k=1; k<=K_epsilon; k++) {
          for (register long j=2; j<=n; j++) {
            if (breakpoints_epsilon[k]==j){
  	      bp_epsilon[j]+=1;
            }
          }
        }
      }
    }//if(epsilon_rev)


    if(la_estim){
   if (la_rev)
      {
	update_lambda_br(lambda, lambda_br, xi_lambda, breakpoints, breakpointsStar, K, KStar, Km1, alpha_lambda, beta_lambda, Y, Z, n, I, acceptedbr, omega, theta_pred_estim, xi_estim, K_geom, p_K, alpha_xi, beta_xi);



  if ((sampleCounter>burnin) && ((sampleCounter-burnin) % filter == 0)) {
    logfile << Km1[1]<<"\t"<< xi_lambda[1]<<"\t";
	    for (register long j=2; j<=n; j++) {
	 logfile << lambda[1][j] << "\t";
    }
  }
    for (register long i=1;i<=I; i++) {
  if (sampleCounter>burnin) {
    for (register long k=1; k<=K[i]; k++) {
      for (register long j=2; j<=n; j++) {
        if (breakpoints[i][k]==j){
  	  bp[i][j]+=1;
        }
      }
    }
  }
    }
      }//if(la_rev)
    } // if(la_estim)
    // cout << S[1][106] << endl;
    //   cout << "test" << endl;
    //Loop over the individual X[t], Y[t], S[t], and omega[t]
    for (register long i=1;i<=I; i++) {
      for (register long t=2; t<=n; t++) {
        //Update X
        double binp = nu[i][t]*xi[i] / (epsilon[t] + nu[i][t]*xi[i] + lambda[i][t] * Z[i][t-1]);
        X[i][t] =  gsl_ran_binomial(r, binp, Z[i][t]);


        //Update S
        binp =  epsilon[t] / (epsilon[t] + lambda[i][t] * Z[i][t-1]);
        S[i][t] =  gsl_ran_binomial(r, binp, (Z[i][t] - X[i][t]));

  
    
        //Update Y
        Y[i][t] = Z[i][t] - X[i][t] - S[i][t];
        
        //Update omega[t] in case of overdispersion
	if(overdispersion){
	       double a = psi + Z[i][t];
	       double b = psi + epsilon[t] + nu[i][t] + lambda[i][t]*Z[i][t-1];
	          omega[i][t] = gsl_ran_gamma(r,a,1/b);
	}
        //Write state to log-file.
        if (sampleCounter>burnin) {
                sumX[i][t] += X[i][t]; sumY[i][t] += Y[i][t]; sumS[i][t] += S[i][t]; sumomega[i][t] += omega[i][t]; Sumeta[i][t] += eta[i][t]; Sumvarr[i][t] += varr[i][t]; Sumrpearson[i][t] += rpearson[i][t];
	}
      }//for t
    }//for i
    //   cout << "test2" << endl;
    // cout << Z[1][2] << endl;
    // cout << X[1][2] << endl;
    // cout << Y[1][2] << endl;
    // cout << S[1][2] << endl;


    //Praediktive Verteilung f�r variables nu
    for (register long i=1;i<=I;i++) {
      if(!theta_pred_estim){
	double p_thetanp1 = ((double(K[i]))/double(n)); //(1+double(K[i]))
	if(K_geom){
	  p_thetanp1 = (double(K[i])*(1.0-p_K)*(1.0-pow(1.0-p_K,n-1)))/((double(n)-1.0)*(1.0-pow(1.0-p_K,n)));
	}
        if(gsl_rng_uniform (r)<=p_thetanp1){
        if (sampleCounter>burnin) {
	  bp[i][n+1] += 1;
	}
	double alpha_la = alpha_lambda;
	double beta_la = beta_lambda;
	if(xi_estim){
	  beta_la = xi_lambda[i];
	}
          lambda[i][n+1]=gsl_ran_gamma(r,alpha_la,1/beta_la);
        }
      }
       	if(overdispersion){
	omeganp1[i] =  gsl_ran_gamma(r,psi,1/psi);
       	}else{
	  omeganp1[i] = 1;
	}
	if(varnu){
	  a = 0;
	  for(int j=scov;j<ncov;j++){
	    a += gamma[j]*xcov[j][n+1];
          }
	  if(rw>0){
	    a += gsl_ran_gaussian(r,sqrt(1/taubeta)) + (2*beta[n-1]-beta[n]);
	  }
          if(delta_rev){
	    double p_thetanp1 = ((double(K[i]))/double(n)); //(1+double(K[i]))
	    if(K_geom){
	      p_thetanp1 = ((double(K[i]))*(1.0-p_K)*(1.0-pow(1.0-p_K,n-1)))/((double(n)-1.0)*(1.0-pow(1.0-p_K,n)));
	    }
            if(gsl_rng_uniform (r)<=p_thetanp1){
              if (sampleCounter>burnin) {
	        bp_delta[n+1] += 1;
   	      }
	      double alpha_de = delta_a;
	      double beta_de = delta_b;
	        if(xi_estim){
	          beta_de = xi_delta;
	        }
              delta[n+1]=gsl_ran_gamma(r,alpha_de,1/beta_de);
            }
            a += log(delta[n+1]);
          }	  
	  nunp1[i] = exp(a);
	}else{
          nunp1[i]=nu[i][n];
        }
        if(epsilon_rev){
	  double p_thetanp1 = ((double(K[i]))/double(n)); //(1+double(K[i]))
	  if(K_geom){
  	    p_thetanp1 = ((double(K[i]))*(1.0-p_K)*(1.0-pow(1.0-p_K,n-1)))/((double(n)-1.0)*(1.0-pow(1.0-p_K,n)));
	  }
          if(gsl_rng_uniform (r)<=p_thetanp1){
            if (sampleCounter>burnin) {
	      bp_epsilon[n+1] += 1;
   	    }
	    double alpha_ep = epsilon_a;
	    double beta_ep = epsilon_b;
	      if(xi_estim){
	        beta_ep = xi_epsilon;
	      }
            epsilon[n+1]=gsl_ran_gamma(r,alpha_ep,1/beta_ep);
          }
        }	  

	Xnp1[i] = gsl_ran_poisson(r,omeganp1[i]*nunp1[i]*xi[i]);
	Ynp1[i] = gsl_ran_poisson(r,lambda[i][n+1]*omeganp1[i]*(Z[i][n]));
	Snp1[i] = gsl_ran_poisson(r,omeganp1[i]*epsilon[n+1]);
        Znp1[i] = Xnp1[i] + Ynp1[i] + Snp1[i];
        if ((sampleCounter>burnin) && ((sampleCounter-burnin) % filter == 0)) {
	  logfile << Znp1[1] << "\t";
        }
    }
 
     

    if ((sampleCounter>burnin) && ((sampleCounter-burnin) % filter == 0)) {
      logfile << satdev(n,I,Z,lambda,nu,xi,epsilon,eta,psi,overdispersion) << endl;
    }

	
    logfile.flush();

    //Tuning	
    if(sampleCounter == tuneSampleSize){
      if (!la_rev)
	{
	cout << "Current xRWSigma=" << xRWSigma << " --> acc rate=" << acceptedlambda/tuneSampleSize << endl;
      tune(xRWSigma, acceptedlambda, tuneSampleSize,tunex);
        cout << "Corrected xRWSigma=" << xRWSigma << endl;
	}
      if(overdispersion){
	cout << endl;
        cout << "Current psiRWSigma=" << psiRWSigma << " --> acc rate=" << acceptedPsi/tuneSampleSize << endl;
        tune(psiRWSigma, acceptedPsi, tuneSampleSize,tunepsi);
        cout << "Corrected psiRWSigma=" << psiRWSigma << endl;
      }
          
        if(varnu&&(rw>0)){
        cout << "Current taubetaRWSigma=" << taubetaRWSigma << " --> acc rate=" << acc_beta/tuneSampleSize << endl;
        tune(taubetaRWSigma, acc_beta, tuneSampleSize,tunetaubeta,0.1,0.4);
        cout << "Corrected taubetaRWSigma=" << taubetaRWSigma << endl;
      }
        
	//tunetaubeta = 0;

      need=tunex + tunepsi + tunetaubeta;
      if(need > 0){
        acceptedlambda = 0;
        acceptedbr = 0;
        acceptedbr_delta = 0;
        acceptedbr_epsilon = 0;
        acceptedPsi = 0;
        sampleCounter = 0;
        if(varnu){
          acc_beta=0;
          acc_alpha=0;
          acc_gamma=0;
        }
        //Fix seed of generator to reproduce results.
	//        gsl_rng_set(r,seed);
      }//if
    }//if

    sampleCounter++;
  }//while counter
      

  //Write means to logfile2

    for (register long t=1;t<=n;t++) {
      logfile2 << (double)sumX[1][t]/((double)samples*(double)filter) << "\t" << (double)sumY[1][t]/((double)samples*(double)filter)<< "\t" << (double)sumomega[1][t]/((double)samples*(double)filter) << "\t"<< (double)bp[1][t]/((double)samples*(double)filter) << "\t";
    }
logfile2 << (double)bp[1][n+1]/((double)samples*(double)filter) << "\t";
  logfile2 << endl;


  //Write accepted status to file 
if(overdispersion){acclog << "psi\t" <<  psiRWSigma << "\t" << (double)acceptedPsi/(double)sampleSize << endl;}
  if (!la_rev){acclog << "lambda\t" << xRWSigma  << "\t" << (double)acceptedlambda/(double)sampleSize << endl;}
  if (la_rev){acclog << "br\t" << 0  << "\t" << (double)acceptedbr/(double)sampleSize << endl;}
  if(I>1){acclog << "alpha\t" << 0 <<"\t" <<(double)acc_alpha/((double)sampleSize*I)<<endl;}
  if(varnu&&(rw>0)){acclog <<"beta\t"<<0 <<"\t"<< (double)acc_beta/((double)sampleSize*(double)(n-1.0))<<endl;}
  if(varnu){acclog <<"gamma\t"<<0 <<"\t"<< (double)acc_gamma/((double)sampleSize*(double)ncov)<<endl;}
  if(delta_rev){acclog <<"brdelta\t"<<0 <<"\t"<< (double)acceptedbr_delta/((double)sampleSize)<<endl;}
  if(epsilon_rev){acclog <<"brepsilon\t"<<0 <<"\t"<< (double)acceptedbr_epsilon/((double)sampleSize)<<endl;}
  //cout << "done"<<endl;
  return;
    



  }//funktion

/* hoehle: interface for calling twins from R */
 
//Twins::Twins(char *iniFile,  long burnin, long filter,  long sampleSize, long seed, float arg_alpha_xi,  float arg_beta_xi, int T, int nfreq, float arg_psiRWSigma, float arg_alpha_psi, float arg_beta_psi) {


  //void Twins_main (char *iniFile,  long burnin, long filter,  long sampleSize, long seed, float arg_alpha_xi,  float arg_beta_xi, int T, int nfreq, float arg_psiRWSigma, float arg_alpha_psi, float arg_beta_psi) {
  //  void Twins_main (char *iniFile, long burnin) {
//Twins::Twins(long burnin) {

extern "C" { 
  void Twins_main (char **iniFile_ptr, long *burnin_ptr, long *filter_ptr,  long *sampleSize_ptr, long *seed_ptr, float *alpha_xi_ptr,  float *beta_xi_ptr, int *T_ptr, int *nfreq_ptr, float *psiRWSigma_ptr, float *alpha_psi_ptr, float *beta_psi_ptr) {


  //Splash screen
  cout << "MCMC Estimation in BPLE Model v1.0 (with R interface). " << endl;
  char *iniFile = new char[80];
  iniFile = *iniFile_ptr;


  cout << "The .ini file is: " << **iniFile_ptr << endl; 
  cout << "Burnin: " << *burnin_ptr << endl;

  char* dataFile;
  char* dataFile2;
  char* logFile;
  char* logFile2;
  


  overdispersion = 1;
  alpha_lambda = 1;
  beta_lambda = 1;
  alpha_nu = 1;
  beta_nu = 1;
  xRWSigma = 1;
  varnu = 1;
  alpha_a = 1;
  alpha_b = 1;
  beta_a = 1;
  beta_b = 1;
  int rw = 0;
  la_rev = 1;
  nu_trend = 0;
  theta_pred_estim = 0;
  xi_estim = 1;
  delta_rev = 0;
  xi_estim_delta = 0;
  delta_a = 1;
  delta_b = 1;
  epsilon_rev = 0;
  xi_estim_epsilon = 0;
  epsilon_a = 1;
  epsilon_b = 1;
  la_estim = 1;
  xi_estim_psi = 0;
  K_geom = 0;
  p_K = 0;
  gamma_a = 1;
  gamma_b = 0.000001;


  char line[200];
  ifstream iniIn(iniFile);

  if (!iniIn.is_open())
    { cout << "Error opening .ini file" << iniFile << endl; exit (1); }

  char line2[200];
  iniIn.getline(line,200,':');
  iniIn.getline(line2,200,'\r');
  dataFile = line2;

  dataFile2 = dataFile;

  char line3[200];
  iniIn.getline(line,200,':');
  iniIn.getline(line3,200,'\r');
  logFile = line3;

  char line4[200];
  iniIn.getline(line,200,':');
  iniIn.getline(line4,200,'\r');
  logFile2 = line4;


  /* hoehle -- new code  if called through interface */

  long burnin = *burnin_ptr;
  long filter = *filter_ptr;
  long sampleSize = *sampleSize_ptr;
  int T = *T_ptr;
  int nfreq = *nfreq_ptr;

  alpha_xi = *alpha_xi_ptr;
  beta_xi = *beta_xi_ptr;
  psiRWSigma = *psiRWSigma_ptr;
  alpha_psi = *alpha_psi_ptr;
  beta_psi = *beta_psi_ptr;
  
  /*

  iniIn.getline(line,200,':');
  iniIn.getline(line,200,'\r');
  burnin = atol(line);

  iniIn.getline(line,200,':');
  iniIn.getline(line,200,'\r');
  filter = atol(line);

  iniIn.getline(line,200,':');
  iniIn.getline(line,200,'\r');
  sampleSize = atol(line);

  iniIn.getline(line,200,':');
  iniIn.getline(line,200,'\r');
  seed = atol(line);

  iniIn.getline(line,200,':');
  iniIn.getline(line,200,'\r');
  alpha_xi = atof(line);

  iniIn.getline(line,200,':');
  iniIn.getline(line,200,'\r');
  beta_xi = atof(line);

  iniIn.getline(line,200,':');
  iniIn.getline(line,200,'\r');
  T = atoi(line);

  iniIn.getline(line,200,':');
  iniIn.getline(line,200,'\r');
  nfreq = atoi(line);


  iniIn.getline(line,200,':');
  iniIn.getline(line,200,'\r');
  psiRWSigma = atof(line);

  iniIn.getline(line,200,':');
  iniIn.getline(line,200,'\r');
  alpha_psi = atof(line);

  iniIn.getline(line,200,':');
  iniIn.getline(line,200,'\r');
  beta_psi = atof(line);

  cout << beta_psi << endl;

  iniIn.close();
  */


  //////////////////////////////////////////////////////////////////////
  //Log file
  ofstream logfile,logfile2,accfile;
  char *accFile = new char[200];
  sprintf(accFile,"%s.acc",logFile);
  logfile.open(logFile);
  logfile2.open(logFile2);
  accfile.open(accFile);
  if (!logfile) { cerr << "Error opening the log file." << endl;exit(-1);}
  if (!accfile) { cerr << "Error opening the acc file." << endl;exit(-1);}
  //////////////////////////////////////////////////////////////////////


  //Allocate a random number generator (we shall use a MT19937).
  /*hoehle: moving from GSL to R */
  /*r = gsl_rng_alloc(gsl_rng_mt19937);*/
  r = 0;
  
  //Fix seed of generator to reproduce results.
  /* gsl_rng_set(r,seed); */

  //Read Data
  long I;
  long n;
  long **Z;
  double *xi= new double[1];

	
  Z = readData(dataFile,&n,&I);

  //xi = readData(dataFile2,I);
  xi[1] = 1;  
  //cout << xi[1] << endl;

  //Do the MCMC estimation, results are written to log.txt
  bplem_estimate(0,logfile,logfile2,accfile,Z,xi,n,I,T,nfreq,burnin,filter,sampleSize, rw);
  //cout <<"done"<<endl;
  logfile.close();
  logfile2.close();
  accfile.close();
  cout << "\nDone." << endl;
  cout << "logfile is in \"" << logFile << "\"." << endl;
  cout << "logfile2 is in \"" << logFile2 << "\"." << endl;
}
/* end of twins */

}

int main(int argc, char *argv[]) {
  //Splash screen
  cout << "MCMC Estimation in BPLE Model v1.0. " << endl;
  if (argc != 2&&argc != 1) { 
    cerr << "Error: The syntax is:\n\n" 
	 << "\t estimate <inifile>" << endl;
    exit(-1);
  }

  char* iniFile;
  if (argc == 2) { 
  iniFile = argv[1];
  }
  if (argc == 1) { 
  iniFile = "twins.ini";
  }

  char* dataFile;
  char* dataFile2;
  char* logFile;
  char* logFile2;


  //cout << dataFile << endl;
  //cout << logFile << endl;
  //cout << iniFile << endl;


  overdispersion = 1;
  alpha_lambda = 1;
  beta_lambda = 1;
  alpha_nu = 1;
  beta_nu = 1;
  xRWSigma = 1;
  varnu = 1;
  alpha_a = 1;
  alpha_b = 1;
  beta_a = 1;
  beta_b = 1;
  int rw = 0;
  la_rev = 1;
  nu_trend = 0;
  theta_pred_estim = 0;
  xi_estim = 1;
  delta_rev = 0;
  xi_estim_delta = 0;
  delta_a = 1;
  delta_b = 1;
  epsilon_rev = 0;
  xi_estim_epsilon = 0;
  epsilon_a = 1;
  epsilon_b = 1;
  la_estim = 1;
  xi_estim_psi = 0;
  K_geom = 0;
  p_K = 0;
  gamma_a = 1;
  gamma_b = 0.000001;




  long burnin;
  long filter;
  long sampleSize;

  int T;
  int nfreq;






  char line[200];
  ifstream iniIn(iniFile);

  if (!iniIn.is_open())
  { cout << "Error opening .ini file"; exit (1); }

  char line2[200];
  iniIn.getline(line,200,':');
  iniIn.getline(line2,200,'\r');
  dataFile = line2;

  dataFile2 = dataFile;

  char line3[200];
  iniIn.getline(line,200,':');
  iniIn.getline(line3,200,'\r');
  logFile = line3;

  char line4[200];
  iniIn.getline(line,200,':');
  iniIn.getline(line4,200,'\r');
  logFile2 = line4;

  iniIn.getline(line,200,':');
  iniIn.getline(line,200,'\r');
  burnin = atol(line);

  iniIn.getline(line,200,':');
  iniIn.getline(line,200,'\r');
  filter = atol(line);

  iniIn.getline(line,200,':');
  iniIn.getline(line,200,'\r');
  sampleSize = atol(line);

  iniIn.getline(line,200,':');
  iniIn.getline(line,200,'\r');
  seed = atol(line);

  iniIn.getline(line,200,':');
  iniIn.getline(line,200,'\r');
  alpha_xi = atof(line);

  iniIn.getline(line,200,':');
  iniIn.getline(line,200,'\r');
  beta_xi = atof(line);

  iniIn.getline(line,200,':');
  iniIn.getline(line,200,'\r');
  T = atoi(line);

  iniIn.getline(line,200,':');
  iniIn.getline(line,200,'\r');
  nfreq = atoi(line);


  iniIn.getline(line,200,':');
  iniIn.getline(line,200,'\r');
  psiRWSigma = atof(line);

  iniIn.getline(line,200,':');
  iniIn.getline(line,200,'\r');
  alpha_psi = atof(line);

  iniIn.getline(line,200,':');
  iniIn.getline(line,200,'\r');
  beta_psi = atof(line);

  cout << beta_psi << endl;

  iniIn.close();




  //////////////////////////////////////////////////////////////////////
  //Log file
  ofstream logfile,logfile2,accfile;
  char *accFile = new char[200];
  sprintf(accFile,"%s.acc",logFile);
  logfile.open(logFile);
  logfile2.open(logFile2);
  accfile.open(accFile);
  if (!logfile) { cerr << "Error opening the log file." << endl;exit(-1);}
  if (!accfile) { cerr << "Error opening the acc file." << endl;exit(-1);}
  //////////////////////////////////////////////////////////////////////


  //Allocate a random number generator (we shall use a MT19937).
  /*hoehle: moving from GSL to R */
  /*r = gsl_rng_alloc(gsl_rng_mt19937);*/
  r = 0;
  
  //Fix seed of generator to reproduce results.
  /* gsl_rng_set(r,seed); */

  //Read Data
  long I;
  long n;
  long **Z;
  double *xi= new double[1];

	
  Z = readData(dataFile,&n,&I);

  //xi = readData(dataFile2,I);
  xi[1] = 1;  
  //cout << xi[1] << endl;

  //Do the MCMC estimation, results are written to log.txt
  bplem_estimate(0,logfile,logfile2,accfile,Z,xi,n,I,T,nfreq,burnin,filter,sampleSize, rw);
  //cout <<"done"<<endl;
  logfile.close();
  logfile2.close();
  accfile.close();
  cout << "\nDone." << endl;
  cout << "logfile is in \"" << logFile << "\"." << endl;
  cout << "logfile2 is in \"" << logFile2 << "\"." << endl;

  return(0);

}


