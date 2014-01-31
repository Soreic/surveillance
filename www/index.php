
<!-- This is the project specific website template -->
<!-- It can be changed as liked or replaced by other content -->

<?php

$domain=ereg_replace('[^\.]*\.(.*)$','\1',$_SERVER['HTTP_HOST']);
$group_name=ereg_replace('([^\.]*)\..*$','\1',$_SERVER['HTTP_HOST']);
$themeroot='http://r-forge.r-project.org/themes/rforge/';

echo '<?xml version="1.0" encoding="UTF-8"?>';
?>
<!DOCTYPE html
	PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"
	"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en" lang="en   ">

  <head>
	<meta http-equiv="Content-Type" content="text/html; charset=UTF-8" />
	<title><?php echo $group_name; ?></title>
	<link href="<?php echo $themeroot; ?>styles/estilo1.css" rel="stylesheet" type="text/css" />
  </head>

<body>

<! --- R-Forge Logo --- >
<table border="0" width="100%" cellspacing="0" cellpadding="0">
<tr><td>
<a href="/"><img src="<?php echo $themeroot; ?>/imagesrf/logo.png" border="0" alt="R-Forge Logo" /> </a> </td> </tr>
</table>


<!-- get project title  -->
<!-- own website starts here, the following may be changed as you like -->

<?php if ($handle=fopen('http://'.$domain.'/export/projtitl.php?group_name='.$group_name,'r')){
$contents = '';
while (!feof($handle)) {
	$contents .= fread($handle, 8192);
}
fclose($handle);
echo $contents; } ?>

<!-- end of project description -->

<hr><b>Description:</b>
<ul>
  <ul>
    <ul>
      <li> The intention of the R-package <tt>surveillance</tt> is to
provide open source software for the temporal and spatio-temporal visualization, modelling and
monitoring of epidemic phenomena. This includes count, binary and categorical data time series as well as continuous-time processes having discrete or continuous spatial resolution. <br>
      </li>
<li> Potential users of the package are biostatisticians, epidemiologists and others working in, e.g., applied infectious disease epidemiology. However, applications could just as
well originate from environmetrics, reliability engineering,
econometrics or social sciences. 
      <li> The main application is in the detection of aberrations in
routine collected public health data seen as univariate and
multivariate time series of counts. </li>
      <li> <tt>surveillance</tt> provides an S4 class data structure
and framework for methodological developments of change-point
algorithms for time series of counts. </li>
      <li>Prospective outbreak detection procedures for count data time
series:</li>
      <ul>
        <li><code>cdc</code> - Stroup et al. (1989)</li>
        <li><code>farrington</code> - Farrington et al. (1996)</li>
        <li><code>rki</code> - The system used at the Robert Koch
Institute, Germany </li>
        <li><code>bayes</code> - A Bayesian predictive posterior
approach, see
Höhle (2007)<br>
        </li>
        <li><code>hmm</code> - An online version of the Hidden Markov
Model approach by
Le Strat and Carrat (1999)
        </li>
        <li><code>rogerson</code> - Surveillance for time varying
Poisson means as documented in
Rogerson and Yamada (2004).<br>
This approach has been extended to
cover time varying proportions in a binomial setting.
        </li>
        <li><code>cusum</code> - An approximate CUSUM method for time
varying Poisson means
as documented in Rossi et al (1999)</li>
        <li><code>glrnb</code> - Likelihood and generalized likelihood
ratio detectors for time varying
Poisson and negative binomial distributed series documented in Höhle
and Paul (2008).<br>
        </li>
        <li><code>outbreakP</code> - Semiparametric surveillance of
outbreaks by Frisén and Andersson (2009)</li>
      </ul>
      <li>Online change-point detection in categorical time series:</li>
      <ul>
        <li><code>categoricalCUSUM</code> - includes change-point
detection based on regression models for binomial and beta-binomial
distributed response. Furthermore, multi-categorical models includes
the multinomial logistic model, proportional odds model and the
Bradley-Terry models, see <a
 href="http://www.stat.uni-muenchen.de/%7Ehoehle/pubs/hoehle2010-preprint.pdf">Höhle
(2010)</a>.</li>
        <li><code>pairedbinCUSUM</code> - paired-binary approach taken
in Steiner et al. (1999)<br>
        </li>
      </ul>
      <li>Retrospective modelling of univariate and multivariate count
data time series is also available as estimation and prediction
routines for the
models described in
        <ul>
          <li><code>algo.hhh </code>- Held et al. (2005) and Paul et
al. (2008)</li>
    <li><code>hhh4</code>- Paul and Held (2011)
          <li><code>algo.twins</code> - Held et al. (2006)</li>
        </ul>
      </li>
      <li>For evaluation purposes, the package contains example
datasets drawn
from the SurvStat@RKI Database maintained the RKI, Germany. More
comprehensive comparisons using simulation studies are possible by
methods for simulating point source outbreak data using a hidden Markov
model. To compare the algorithms, benchmark numbers like sensitivity,
specificity and detection delay can be computed for entire sets of
surveillance time series. Furthermore, a Markov Chain approximation for
computing the run-length distribution of the proposed likelihood ratio
CUSUMs is available as function <code>LRCUSUM.runlength.</code></li>
<li> Retrospective continuous time modelling and simulation of epidemic phenomena (experimental from version 1.3-0 on):  
<ul>
<li><code>twinSIR</code> - continuous-time and discrete-space modelling as described in H&ouml;hle (2009). The <code>epidata</code> class provides the appropriate data structure for such data.
<li><code>twinstim</code> - continuous-time and continuous-space modelling as desribed in
<a href="http://dx.doi.org/10.1111/j.1541-0420.2011.01684.x">Meyer et al. (2012)</a>.
The <code>epidataCS</code> data provides a novel data class for point-referenced space-time data.
</ul>
      <li>Prospective space-time cluster detection:</li>
      <ul>
        <li><code>stcd</code> - (experimental) Point process based
approach by Assuncao &amp; Correa (2009)<br>
        </li>
      </ul>
    </ul>
  </ul>
</ul>

The package comes with ABSOLUTELY NO WARRANTY; for details see <a
href="http://www.gnu.org/copyleft/gpl.html">http://www.gnu.org/copyleft/gpl.html</a>
(GPL). This is free software, and and you are welcome to redistribute
it under the GPL conditions.

<hr>
<b>Download:</b>
<blockquote>
  <blockquote>
    <blockquote> The <tt>surveillance</tt> package is
available for download from <a
 href="http://cran.r-project.org/web/packages/surveillance/">CRAN</a>.<br>
Current package development, help-forum and bugtracking is hosted
through
R-Forge:<br>
      <br>
      <div style="text-align: center;"><a
 href="https://r-forge.r-project.org/projects/surveillance/">https://r-forge.r-project.org/projects/surveillance/</a><br>
      </div>
      <br>
From this page snapshots of the current development version are available.
For example -- if you are running a recent R version -- you can obtain a
binary snapshot of the development version as<br>

<div style="text-align: center;"> <tt>install.packages("surveillance",repos="http://r-forge.r-project.org")</tt>.</div>

You can also manually download the
<a href="http://r-forge.r-project.org/bin">binary snapshot</a>
or the <a href="http://r-forge.r-project.org/src">source tarball</a>.
      <br><p>
New features:<br>
      <ul>
        <li>See <a
 href="https://r-forge.r-project.org/scm/viewvc.php/pkg/inst/NEWS.Rd?view=markup&root=surveillance">NEWS</a>
file in the current distribution</li>
      </ul>
      <ul>
      </ul>
    </blockquote>
  </blockquote>
</blockquote>

<hr><b>News:</b>
<ul>
 <ul>
  <ul>
   <li>2013/04/23 <a href="StockholmR-Hoehle_4.pdf">Talk</a> at the  Stockholm R useR group (StockholmR) on <a href="http://www.meetup.com/StockholmR/events/105738342/">Making R packages (and) Shiny</a>.</li>
  </ul>
 </ul>
</ul>
  
<hr><b>Documentation:</b>
<blockquote>
  <blockquote>
    <ul>
      <li>A good (but slightly outdated) introduction to the outbreak detection part of the package is
provided in the paper <a
 href="http://dx.doi.org/10.1007/s00180-007-0074-8"><span
 style="font-family: monospace;">surveillance</span>: An R package for
the surveillance of infectious diseases</a>, Computational Statistics
(2007), 22(4), pp. 571-582. <a
 href="http://www.stat.uni-muenchen.de/%7Ehoehle/pubs/hoehle-CoSt2008-preprint.pdf">
[preprint]</a></li>
      <li>A more recent description can be found in the book chapter <i>Aberration
detection in R illustrated by Danish mortality monitoring</i> (2010),
M. Höhle and A. Mazick, To appear in T. Kass-Hout and X. Zhang (Eds.)
Biosurveillance: A Health Protection Priority, CRC Press. [<a
							   href="http://www.stat.uni-muenchen.de/%7Ehoehle/pubs/hoehle_mazick2009-preprint.pdf">preprint</a>]. Note: As ISO 8601 handling is not fully implemented in R on Windows the demo("biosurvbook") will only run with package version >= 1.2, where a workaround was implemented.</li>
      <li>An overview of statistical methods and implementational usage
is given the course notes of several courses on the package, e.g.
the course notes
of the lecture
<a href="http://www.statistik.lmu.de/~hoehle/teaching/moid2011/moid.pdf">
Temporal and spatio-temporal modelling of infectious
diseases
</a>
at the Department of Statistics, University of Munich, Oct 10-13, 2011 or
the shortcourse  <a
 href="http://www.stat.uni-muenchen.de/%7Ehoehle/surv-short/index.html">Statistical
surveillance of infectious diseases</a> held at the Department of
Statistics, Universidade Federal de Minas Gerais (UFMG), Belo
Horizonte, Brazil, Nov 27-28, 2008. 
</li>
      <li><a href="hoehle-surveillance.pdf">Invited talk</a> held at
the 2008 ESCAIDE satellite workshop on <ii>Computer supported outbreak
detection and signal management</ii> (<a href="hoehle-surveillance.R">R-File</a>,
        <a href="ha.csv">Data</a> from SurvStat@RKI)
      </li>
      <li>Application of the package in veterinary public health
surveillance is described in <a
 href="http://dx.doi.org/10.1016/j.prevetmed.2009.05.017">Statistical
approaches to the surveillance of infectious diseases for veterinary
public health</a> [<a href="http://epub.ub.uni-muenchen.de/2093/">preprint</a>].
      </li>
      <li>Read the package vignette or look <a
 href="http://www.stat.uni-muenchen.de/%7Ehoehle/pubs/">here</a> for
further preprints.<br>
      </li>
      <li>Sometimes pictures says more than 1000 words:</li>
      <code>algo.farrington</code> + <code>algo.glrnb</code> + <code>nowcast</code><p>
      <img src="detectandnowcast.png" align="middle"><p>
      <code>backprojNP</code><p>
      <img src="backproj.png" align="middle"><p>
      <code>twinSIR</code><p>
<!--  convert -density 100x100 mpbb-intensity.pdf mpbb-intensity.png -->
      <img src="mpbb-intensity.png" align="middle"><p>
    </ul>
  </blockquote>
</blockquote>

<hr><b>Developers:</b>
<blockquote>
  <blockquote>
    <ul>
      <li><a href="http://www.math.su.se/%7Ehoehle">Michael
Höhle</a>, Department of Mathematics, Stockholm University, Sweden
(Project Admin)</li>
      <li><a href="http://www.biostat.uzh.ch/aboutus/people/smeyer.html">Sebastian
Meyer</a>, Institute of Social and Preventive Medicine, University of 
Zurich, Switzerland</li> 
      <li><a href="http://www.biostat.uzh.ch/aboutus/people/mpaul.html">Michaela
Paul</a>, Institute of Social and Preventive Medicine, University of
Zurich, Switzerland</li>
      <li><a href="http://www.normalesup.org/~masalmon/">Ma&eumllle Salmon</a>, Department for Infectious Disease Epidemiology, Robert Koch Institute, Germany</li>
      <li>Former student programmers: C. Lang, A. Riebler, V. Wimmer</li>
      <li>Contributions by: T. Correa, L. Held, M. Hofmann, D. Sabanés Bové, S. Steiner,
M. Virtanen</li>
    </ul>
  </blockquote>
</blockquote>

<hr><b>Financial support:</b>
<blockquote>
  <blockquote> The development of models and algorithms implemented in
surveillance was financially supported by :
    <ul>
      <li>German Science Foundation (DFG, 2003-2006)</li>
      <li><a href="http://www.en.mc-health.uni-muenchen.de/index.html">Munich
Center of Health Sciences</a> (MC-Health, 2007-2010)</li>
      <li>Swiss National Science Foundation (SNF, 2007--2010, projects <a href="http://p3.snf.ch/Project-116776">#116776</a> and <a href="http://p3.snf.ch/Project-124429">#124429</a>)</li>
      <li>Swiss National Science Foundation (SNF, 2012--2015, project <a href="http://p3.snf.ch/Project-137919">#137919</a>)</li>
    </ul>
  </blockquote>
</blockquote>
<!-- 
<p> The <strong>project summary page</strong> you can find <a href="http://<?php echo $domain; ?>/projects/<?php echo $group_name; ?>/"><strong>here</strong></a>. </p>
-->
</body>
</html>
