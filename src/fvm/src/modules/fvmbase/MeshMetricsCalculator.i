%{
#include "MeshMetricsCalculator.h"
#include "atype.h"
%}

using namespace std;


template<class T>
class MeshMetricsCalculator : public Model
{
public:
  MeshMetricsCalculator(GeomFields& geomFields, const MeshList& meshes);
  virtual void init();
  void createNodeDisplacement();
  void calculateBoundaryNodeNormal();
  void computeIBInterpolationMatrices(const StorageSite& particles);
  void computeIBandSolidInterpolationMatrices(const StorageSite& particles);
  void computeGridInterpolationMatrices(const StorageSite& grids, const StorageSite& faces );	
#ifdef USING_ATYPE_TANGENT
  void setTangentCoords(int meshID, int faceZoneID, int dim);
#endif
};


%template(MeshMetricsCalculatorA) MeshMetricsCalculator<ATYPE_STR>;

