#!/usr/bin/env python
import sys

import fvm
import fvm.fvmbaseExt as fvmbaseExt
import fvm.importers as importers

import time
from numpy import *
import tecplotESBGK
import tecplot
import string

fvm.set_atype('double')

from FluentCase import FluentCase

#fvmbaseExt.enableDebug("cdtor")

fileBase = None

fileBase = "/home/ba01/u116/schigull/memosa/ESBGK-tests/testWallBC/testKineticFlowModel"
def usage():
    print "Usage: %s filebase [outfilename]" % sys.argv[0]
    print "Where filebase.cas is a Fluent case file."
    print "Output will be in filebase-prism.dat if it is not specified."
    sys.exit(1)


# change as needed

outfile = None
if __name__ == '__main__' and fileBase is None:
    if len(sys.argv) < 2:
        usage()
    fileBase = sys.argv[1]
    if len(sys.argv) == 3:
        outfile = sys.argv[2]

if outfile == None:
    outfile = fileBase+"-tecplt.dat"

#import debug

   
reader = FluentCase(fileBase+".cas")

reader.read();
import sys

#fluent_meshes = reader.getMeshList()

import time
t0 = time.time()


meshes = reader.getMeshList()

geomFields =  fvm.models.GeomFields('geom')
metricsCalculator = fvm.models.MeshMetricsCalculatorA(geomFields,meshes)

metricsCalculator.init()
#!!!!!!!!!!!!!!!!source code (metricsMeshCalcultaro)::init(), volumeField.synLocal, coordField.syncLocal() turned off

flowFields =  fvm.models.FlowFields('flow')

fmodel = fvm.models.FlowModelA(geomFields,flowFields,meshes)

## set bc for top to be a wall with x velocity
bcMap = fmodel.getBCMap()
if 3 in bcMap:
   bc3 = fmodel.getBCMap()[3]
   bc3.bcType = 'NoSlipWall'
   bc3.setVar('specifiedXVelocity',1)
				

## set viscosity and density, this is done per mesh since each mesh has its own VC object
vcMap = fmodel.getVCMap()
for vc in vcMap.values():
    vc.setVar('density',1.0)
    vc.setVar('viscosity',1.0)


momSolver = fvmbaseExt.AMG()
contSolver = fvmbaseExt.AMG()

# to test quadrature


import fvm.esbgk_atyped_double as esbgk

#import fvm.MacroParameters as macropr
#import fvm.DistFunctFields as f

#foptions = fmodel.getOptions()
#foptions['timeStep'] = 1E-6
#foptions.transient = True
#fmodel.init()


#kineticmodel=fvm.models.KineticModelD(meshes,flowFields,macroFields,quad)
#kineticmodel.init()

#cartesian
#import ddd
quad0=esbgk.QuadratureD(20,20,20,10.5,1.0) #cartesian
#quad1=esbgk.QuadratureD(2,2,0,12,0,5) #spherical
#quad2=esbgk.QuadratureD(16,16,1,8,1,4) #gauss-hermit quadrature and 3/8th rule

macroFields=esbgk.MacroFields('flow')

esbgk1=esbgk.KineticModelD(meshes,geomFields,macroFields,quad0)
#esbgk1options = esbgk1.getOptions()
#print esbgk1options['timeStep']
#print options

# initialize macroparameters to different values
esbgk1.InitializeMacroparameters()
#import ddd
esbgk1.weightedMaxwellian(0.5,0.02,0.02) #initial distribution
esbgk1.ComputeMacroparameters()

cellSite = meshes[0].getCells()
densityField =  macroFields.density[cellSite].asNumPyArray()
collisionFrequencyField = macroFields.collisionFrequency[cellSite].asNumPyArray()
#print 'len = ',len(densityField)
#print densityField[0],densityField[1],densityField[2]
#print collisionFrequencyField[0],collisionFrequencyField[1],collisionFrequencyField[2]

#esbgk1.initializeMaxwellianEq() #equilibrium dist function
"""
#boundary conditions
bcMap = esbgk1.getBCMap()
bcTop = bcMap[3]
bcTop.bcType = 'CopyBC'
bcBot = bcMap[3]
bcBot.bcType = 'WallBC'
bcRight = bcMap[3]
bcRight.bcType = 'WallBC'
"""
print ''
   
numTsteps = 5
numIter=10
output_interval = 1
def advance(ntstep):
    for i in range(0,ntstep):
        print 'timestep = ',i+1
        esbgk1.advance(numIter)
        esbgk1.updateTime()
        
        if ((i+1)%output_interval == 0) :
            filename = "output_"+string.zfill(str(i+1),3)+".plt"
            esbgk1.OutputDsfBLOCK(filename)
            filename = "macro_"+string.zfill(str(i+1),3)+".plt"
            tecplotESBGK.esbgkTecplotFile(meshes,macroFields,filename)


esbgk1.OutputDsfBLOCK("output_000.plt")
tecplotESBGK.esbgkTecplotFile(meshes,macroFields,"macro_000.plt")       
advance(numTsteps)        
esbgk1.OutputDsfBLOCK("final.plt")

t1=time.time()
print 'time taken for',numTsteps,'tsteps',numIter,'iters =', t1-t0

"""
cellSite = meshes[0].getCells()
velField = macroFields.velocity[ cellSite ].asNumPyArray() 
print  macroFields.velocity[ cellSite ].asNumPyArray()
rhoField = macroFields.density[ cellSite ].asNumPyArray() 
print  macroFields.density[ cellSite ].asNumPyArray()
#import debug
#fmodel.advance(100)
filename="macro1.plt"
tecplotESBGK.esbgkTecplotFile(meshes,macroFields,filename)
#tecplot.dumpTecplotFile(1,meshes,macroFields)
"""