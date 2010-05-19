#ifndef _STRUCTURESOURCEDISCRETIZATION_H_
#define _STRUCTURESOURCEDISCRETIZATION_H_

#include "Field.h"
#include "MultiField.h"
#include "MultiFieldMatrix.h"
#include "Mesh.h"
#include "Discretization.h"
#include "StorageSite.h"
#include "GeomFields.h"
#include "CRConnectivity.h"
#include "CRMatrixRect.h"
#include "Vector.h"
#include "GradientModel.h"

template<class T, class Diag, class OffDiag>
class StructureSourceDiscretization : public Discretization
{
public:

  typedef Array<T> TArray;
  typedef Vector<T,3> VectorT3;
  typedef Array<VectorT3> VectorT3Array;
  typedef Gradient<VectorT3> VGradType;
  typedef Array<Gradient<VectorT3> > VGradArray;

  typedef CRMatrix<Diag,OffDiag,VectorT3> CCMatrix;
  typedef typename CCMatrix::DiagArray DiagArray;
  typedef typename CCMatrix::PairWiseAssembler CCAssembler;

  typedef GradientModel<VectorT3> VGradModelType;
  typedef typename VGradModelType::GradMatrixType VGradMatrix;
  
  StructureSourceDiscretization(const MeshList& meshes,
				const GeomFields& geomFields,
				Field& varField,
				const Field& muField,
				const Field& lambdaField,
				const Field& varGradientField,
                                bool fullLinearization=true)  :
    Discretization(meshes),
    _geomFields(geomFields),
    _varField(varField),
    _muField(muField),
    _lambdaField(lambdaField),
    _varGradientField(varGradientField),
    _fullLinearization(fullLinearization)
   {}

  void discretize(const Mesh& mesh, MultiFieldMatrix& mfmatrix,
                  MultiField& xField, MultiField& rField)
  {
    const StorageSite& iFaces = mesh.getInteriorFaceGroup().site;
    
    discretizeFaces(mesh, iFaces, mfmatrix, xField, rField, false, false);

    foreach(const FaceGroupPtr fgPtr, mesh.getInterfaceGroups())
    {
        const FaceGroup& fg = *fgPtr;
        const StorageSite& faces = fg.site;
        discretizeFaces(mesh, faces, mfmatrix, xField, rField, false, false);
    }
        
    // boundaries
    foreach(const FaceGroupPtr fgPtr, mesh.getBoundaryFaceGroups())
    {
        const FaceGroup& fg = *fgPtr;
        const StorageSite& faces = fg.site;
        discretizeFaces(mesh, faces, mfmatrix, xField, rField,
                        true,
                        fg.groupType=="symmetry");
    }
  }

                          
  void discretizeFaces(const Mesh& mesh, const StorageSite& faces,
                       MultiFieldMatrix& mfmatrix,
                       MultiField& xField, MultiField& rField,
                       const bool isBoundary, const bool isSymmetry)
  {
    const StorageSite& cells = mesh.getCells();

    const MultiField::ArrayIndex cVarIndex(&_varField,&cells);

    const VectorT3Array& faceArea =
      dynamic_cast<const VectorT3Array&>(_geomFields.area[faces]);

    const TArray& faceAreaMag =
      dynamic_cast<const TArray&>(_geomFields.areaMag[faces]);

    const VectorT3Array& cellCentroid =
      dynamic_cast<const VectorT3Array&>(_geomFields.coordinate[cells]);

    const VectorT3Array& faceCentroid =
      dynamic_cast<const VectorT3Array&>(_geomFields.coordinate[faces]);


    const TArray& cellVolume =
      dynamic_cast<const TArray&>(_geomFields.volume[cells]);

    const CRConnectivity& faceCells = mesh.getFaceCells(faces);

    VectorT3Array& rCell = dynamic_cast<VectorT3Array&>(rField[cVarIndex]);
    const VectorT3Array& xCell = dynamic_cast<const VectorT3Array&>(xField[cVarIndex]);

    const VGradArray& vGradCell =
      dynamic_cast<const VGradArray&>(_varGradientField[cells]);

    const TArray& muCell =
      dynamic_cast<const TArray&>(_muField[cells]);

    const TArray& lambdaCell =
      dynamic_cast<const TArray&>(_lambdaField[cells]);

    CCMatrix& matrix = dynamic_cast<CCMatrix&>(mfmatrix.getMatrix(cVarIndex,
                                                             cVarIndex));
    CCAssembler& assembler = matrix.getPairWiseAssembler(faceCells);
    DiagArray& diag = matrix.getDiag();


    const int nFaces = faces.getCount();

    const VGradMatrix& vgMatrix = VGradModelType::getGradientMatrix(mesh,_geomFields);
    const CRConnectivity& vgConn = vgMatrix.getConnectivity();
    const Array<int>& vgmRow = vgConn.getRow();
    const Array<int>& vgmCol = vgConn.getCol();
    const VectorT3Array& vgmCoeffs = vgMatrix.getCoeffs();

    const int nInteriorCells = cells.getSelfCount();
    
    for(int f=0; f<nFaces; f++)
    {
        const int c0 = faceCells(f,0);
        const int c1 = faceCells(f,1);

	const VectorT3& Af = faceArea[f];
        const VectorT3 en = Af/faceAreaMag[f];
        
        VectorT3 ds=cellCentroid[c1]-cellCentroid[c0];

        T vol0 = cellVolume[c0];
        T vol1 = cellVolume[c1];

        T wt0 = vol0/(vol0+vol1);
        T wt1 = vol1/(vol0+vol1);

        if (isBoundary && !isSymmetry)
        {  
            wt0 = T(1.0);
            wt1 = T(0.);
        }
        
        T faceMu(1.0);
	T faceLambda(1.0);

        Diag& a00 = diag[c0];
        Diag& a11 = diag[c1];
        OffDiag& a01 = assembler.getCoeff01(f);
        OffDiag& a10 = assembler.getCoeff10(f);

        if (vol0 == 0.)
       	{
            faceMu = muCell[c1];
	    faceLambda = lambdaCell[c1];
	}
        else if (vol1 == 0.)
	{
            faceMu = muCell[c0];
	    faceLambda = lambdaCell[c0];
	}
        else
	{
            faceMu = harmonicAverage(muCell[c0],muCell[c1]);
	    faceLambda = harmonicAverage(lambdaCell[c0],lambdaCell[c1]);
	}

        faceMu = muCell[c0]*wt0 + muCell[c1]*wt1;
        faceLambda = lambdaCell[c0]*wt0 + lambdaCell[c1]*wt1;
        
	const VGradType gradF = (vGradCell[c0]*wt0 + vGradCell[c1]*wt1);

	VectorT3 source(NumTypeTraits<VectorT3>::getZero());
        const T divU = (gradF[0][0] + gradF[1][1] + gradF[2][2]);

        const T diffMetric = faceAreaMag[f]*faceAreaMag[f]/dot(faceArea[f],ds);
        const VectorT3 secondaryCoeff = faceMu*(faceArea[f]-ds*diffMetric);

        // mu*grad U ^ T + lambda * div U I
	source[0] = faceMu*(gradF[0][0]*Af[0] + gradF[0][1]*Af[1] + gradF[0][2]*Af[2])
          + faceLambda*divU*Af[0];
        
	source[1] = faceMu*(gradF[1][0]*Af[0] + gradF[1][1]*Af[1] + gradF[1][2]*Af[2])
          + faceLambda*divU*Af[1];
        
	source[2] = faceMu*(gradF[2][0]*Af[0] + gradF[2][1]*Af[1] + gradF[2][2]*Af[2])
          + faceLambda*divU*Af[2];

        
        VectorT3 s0(NumTypeTraits<VectorT3>::getZero());
        VectorT3 s1(NumTypeTraits<VectorT3>::getZero());
        if (_fullLinearization)
        {
            
            for(int nnb = vgmRow[c0]; nnb<vgmRow[c0+1]; nnb++)
            {
                const int nb = vgmCol[nnb];
                
                VectorT3 g_nb = vgmCoeffs[nnb];
#if 1
                if (isSymmetry)
                {
                    const T gnb_dot_nx2 = T(2.0)*dot(en,g_nb);
                    g_nb = gnb_dot_nx2*en;
                }
#endif
                Diag coeff;

                for(int i=0; i<3; i++)
                {
                    for(int j=0; j<3; j++)
                    {
                        coeff(i,j) = wt0*(faceMu*Af[j]*g_nb[i] 
                                          + faceLambda*Af[i]*g_nb[j]
                                          );
                    }

                    for(int k=0; k<3; k++)
                      coeff(i,i) += wt0*secondaryCoeff[k]*g_nb[k];
                }
#if 0
                if (isSymmetry)
                {
                    SquareTensor<T,3> R;
                    
                    for(int i=0;i<3;i++)
                      for(int j=0; j<3; j++)
                      {
                          if (i==j)
                            R(i,j) = 1.0 - 2*en[i]*en[j];
                          else
                            R(i,j) = - 2*en[i]*en[j];
                      }

                    Diag coeff1(R*coeff*R);
                    coeff += coeff1;
                      
                }
#endif
                OffDiag& a0_nb = matrix.getCoeff(c0,nb);

                a0_nb += coeff;
                a00 -= coeff;
                a10 += coeff;
                
                if (c1 != nb)
                {
                    OffDiag& a1_nb = matrix.getCoeff(c1,nb);
                    a1_nb -= coeff;
                }
                else
                  a11 -= coeff;
            }


            if (!isBoundary)
            {
                for(int nnb = vgmRow[c1]; nnb<vgmRow[c1+1]; nnb++)
                {
                    const int nb = vgmCol[nnb];
                    const VectorT3& g_nb = vgmCoeffs[nnb];
                    
                    Diag coeff;
                    
                    for(int i=0; i<3; i++)
                    {
                        for(int j=0; j<3; j++)
                        {
                            coeff(i,j) = wt1*(faceMu*Af[j]*g_nb[i] 
                                              + faceLambda*Af[i]*g_nb[j]
                                              );
                        }
                        
                        for(int k=0; k<3; k++)
                          coeff(i,i) += wt1*secondaryCoeff[k]*g_nb[k];
                    }
                    
                    
                    OffDiag& a1_nb = matrix.getCoeff(c1,nb);
                    a1_nb -= coeff;
                    a11 += coeff;
                    a01 -= coeff;
                    
                    if (c0 != nb)
                    {
                        OffDiag& a0_nb = matrix.getCoeff(c0,nb);
                        
                        a0_nb += coeff;
                    }
                    else
                      a00 += coeff;
                    
                }
            }
        }
        
        // mu*gradU, primary part
        

        source += faceMu*diffMetric*(xCell[c1]-xCell[c0]);

        // mu*gradU, secondart part


        source += gradF*secondaryCoeff;

        // add flux to the residual of c0 and c1
        rCell[c0] += source;
	rCell[c1] -= source;

        // for Jacobian, use 2*mu + lambda as the diffusivity
        const T faceDiffusivity = faceMu;
        const T diffCoeff = faceDiffusivity*diffMetric;

        
        a01 +=diffCoeff;
        a10 +=diffCoeff;

        
        a00 -= diffCoeff;
        a11 -= diffCoeff;

    }
  }
    

private:
  const GeomFields& _geomFields;
  Field& _varField;
  const Field& _muField;
  const Field& _lambdaField;
  const Field& _varGradientField;
  const bool _fullLinearization;
};

#endif
