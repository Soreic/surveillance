################################################################################
## Authors: Sebastian Meyer, Michael Hoehle
## $Date$
## Project: Preparing package build
## Note: Watch the list in .Rbuildignore
##
## History:
##    8 Jan 2008 (MH): initial version
##   29 Mar 2012 (SM): Added making of R/sysdata.rda from sysdata/sysdata.R
##    4 Apr 2012 (SM): R execution command as a variable
##   11 Jun 2012 (SM): Restored and cleaned up Makefile
##   18 Jun 2012 (SM): fixed make manual bug (must first remove old manual.pdf)
##   19 Jun 2012 (SM): run check on built package instead of source directory
##   27 Jun 2012 (SM): added checkUsage recipe (R package codetools)
################################################################################

## Define variable for R executable which enables the use of alternatives,
## e.g., 'make check R=R-devel'
R = R

## sysdata file
SYSDATA := pkg/R/sysdata.rda

## package version
VERSION := $(strip $(shell grep "Version:" pkg/DESCRIPTION | cut -f 2 -d ":"))

## phony targets
.PHONY: clean build check install manual



build: clean ${SYSDATA}
	$R CMD build --no-resave-data --compact-vignettes pkg 
#--resave-data also compresses data/*.txt -> readData("k1",week53to52=TRUE) would no longer work

clean:
	cd pkg/src; rm -f *.o *.so *.dll symbols.rds
	rm -f pkg/*/.Rhistory

## Save internal datasets from pkg/sysdata/ into pkg/R/sysdata.rda
${SYSDATA}: pkg/sysdata/sysdata.R
	cd pkg/sysdata; $R CMD BATCH --vanilla --no-timing sysdata.R
	mv pkg/sysdata/sysdata.rda $@

check: build
	$R CMD check --as-cran surveillance_${VERSION}.tar.gz
## further option: --use-gct (for better detection of memory bugs/segfaults)

install: ${SYSDATA}
	$R CMD INSTALL pkg

checkUsage: install
	echo "library('surveillance'); library('codetools'); \
	checkUsagePackage('surveillance', suppressFundefMismatch=FALSE, \
	    suppressLocalUnused=TRUE, suppressNoLocalFun=TRUE, skipWith=TRUE, \
	    suppressUndefined=FALSE, suppressPartialMatchArgs=FALSE)" \
	| R --slave --no-save --no-restore

manual:	
	$R CMD Rd2pdf --batch --force --output=manual.pdf pkg

# macsrc:
# 	cd src ; gcc-4.0 -arch i386 -isysroot /Developer/SDKs/MacOSX10.4u.sdk -no-cpp-precomp -I/Library/Frameworks/R.framework/Resources/include -I/Library/Frameworks/R.framework/Resources/include/i386  -msse3  -D__NO_MATH_INLINES  -fPIC  -g -O2 -Wall -pedantic -c surveillance.c -o surveillance.o

