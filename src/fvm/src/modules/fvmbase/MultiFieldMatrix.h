#ifndef _MULTIFIELDMATRIX_H_
#define _MULTIFIELDMATRIX_H_

#include "Matrix.h"
#include "MultiField.h"
#include <set>

//UnorderedSet doesn't duplicate and  sort elements
class UnorderedSet
{
   public : 
       UnorderedSet() {};
      ~UnorderedSet() {};

       void insert( int x ) {

	  if ( _dataSet.count(x) == 0 ){
	     _data.push_back(x);	  
             _dataSet.insert(x);
          }	
       };

       const vector<int>& getData() const { return _data;};
       int size()  const { return  _data.size(); };

   private :
      vector<int> _data;
      set<int>    _dataSet;


};

class Connectivity;
class IContainer;
class OneToOneIndexMap;

class LinearSystem;

class  MultiFieldMatrix
{
public:

  friend class LinearSystem;
  
  typedef MultiField::ArrayIndex Index;

  typedef pair<Index,Index> EntryIndex;
  typedef map<EntryIndex,shared_ptr<Matrix> > MatrixMap;
  typedef map<Index,int> MatrixSizeMap;
  typedef map<const StorageSite*,shared_ptr<Array<int> > > MatrixMappersMap;
  typedef map<Index,shared_ptr<StorageSite> > StorageSiteMap;
  typedef map<const StorageSite*,shared_ptr<StorageSite> > GhostStorageSiteMap;
  
  typedef map<Index,shared_ptr<CRConnectivity> > CoarseToFineMappingMap;
  typedef map<EntryIndex,shared_ptr<CRConnectivity> > CoarseConnectivitiesMap;
  
  MultiFieldMatrix();
  virtual ~MultiFieldMatrix();
  
  DEFINE_TYPENAME("MultiFieldMatrix");

  void initAssembly();
  
  virtual void multiply(IContainer& yB, const IContainer& xB) const;
  
  virtual void multiplyAndAdd(IContainer& yB, const IContainer& xB) const;
  
  virtual void forwardGS(IContainer& xB, const IContainer& bB, IContainer& temp) const;
  
  virtual void reverseGS(IContainer& xB, const IContainer& bB, IContainer& temp) const;

  virtual void solveBoundary(IContainer& xB, const IContainer& bB, IContainer& temp) const;

  virtual void
  computeResidual(const IContainer& xB, const IContainer& bB,
                  IContainer& rB) const;

  bool hasMatrix(const Index& rowIndex,
                 const Index& colIndex) const;
  Matrix& getMatrix(const Index& rowIndex,const Index& colIndex);
  const Matrix& getMatrix(const Index& rowIndex,const Index& colIndex) const;
  
  void removeMatrix(const Index& rowIndex, const Index& colIndex);

  void createCoarsening(MultiField& coarseIndex,
                        const int groupSize,
                        const double weightRatioThreshold);
  
  void syncGhostCoarsening(MultiField& coarseIndexField);

  void createCoarseToFineMapping(const MultiField& coarseIndexField);

  void createCoarseConnectivity(MultiField& coarseIndex);

  void createCoarseMatrices(MultiField& coarseIndex);

  void
  injectResidual(const MultiField& coarseIndex,
                 const MultiField& fineResidualField,
                 MultiField& coarseBField);

  void
  correctSolution(const MultiField& coarseIndex,
                  MultiField& fineSolutionField,
                  const MultiField& coarseSolutionField);
  
  int getSize() const;

  void addMatrix(const Index& rowI, const Index& colI, shared_ptr<Matrix> m)
  {
    EntryIndex e(rowI,colI);
    _matrices[e] = m;
  }
  
private:
  MatrixMap _matrices;
  MatrixSizeMap _coarseSizes;
  MatrixSizeMap _coarseGhostSizes;
  MatrixMappersMap _coarseScatterMaps;
  MatrixMappersMap _coarseGatherMaps;
  
  StorageSiteMap _coarseSites;
  GhostStorageSiteMap _coarseGhostSites;
  CoarseToFineMappingMap _coarseToFineMappings;
  CoarseConnectivitiesMap _coarseConnectivities;
  MatrixMap _coarseMatrices;
};


#endif
