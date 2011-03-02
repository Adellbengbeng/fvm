#include "StructureFields.h"

StructureFields::StructureFields(const string baseName) :
  deformation(baseName + ".deformation"),
  deformationGradient(baseName + ".deformationGradient"),
  deformationFlux(baseName + ".deformationFlux"),
  eta(baseName + ".eta"),
  eta1(baseName + ".eta1"),
  alpha(baseName + ".alpha"),
  density(baseName + ".density"),
  deformationN1(baseName + ".deformationN1"),
  deformationN2(baseName + ".deformationN2"),
  deformationN3(baseName + ".deformationN3"),
  tractionX(baseName + ".tractionX"),
  tractionY(baseName + ".tractionY"),
  tractionZ(baseName + ".tractionZ"),
  temperature(baseName + ".temperature"),
  bodyForce(baseName + ".bodyForce"),
  volume0(baseName + ".volume0")
{}

