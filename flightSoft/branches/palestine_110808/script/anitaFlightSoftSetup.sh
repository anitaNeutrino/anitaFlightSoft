#!/bin/bash
## The ANITA Flight Software setup script. Only really needed when doing 
## development, although ANITA_CONFIG_DIR is used to find the various different
## config files.
##
## Ryan Nichol (rjn@mps.ohio-state.edu)
## August 2004

##CVSROOT should be :ext:<user>@auger.mps.ohio-state.edu:/work/anitaCVS
#export CVSROOT=:ext:rjn@auger1.mps.ohio-state.edu:/work1/anitaCVS
#export CVS_RSH=ssh



## Where is the main directory
export ANITA_FLIGHT_SOFT_DIR=/home/anita/flightSoft

## Shouldn't need to change this
export ANITA_CONFIG_DIR=${ANITA_FLIGHT_SOFT_DIR}/config
export PATH=${ANITA_FLIGHT_SOFT_DIR}/bin:${PATH}
export LD_LIBRARY_PATH=${ANITA_FLIGHT_SOFT_DIR}/lib:${LD_LIBRARY_PATH}
