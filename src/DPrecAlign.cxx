// @(#)maf/dtools:$Name:  $:$Id: DPrecAlign.cxx,v.3 2005/10/02 18:03:46 sha Exp $
//  Author :  G.Orazi 1/2/2000
////////////////////////////////////////////////////////////////////////
// Class Description of DPrecAlign                                    //
//                                                                    //
// This class stores the precise alignement of a plane.
// 6 parameters are used: 3 for translations and 3 for rotations.
//
// Notations:
//  Telescope or tracker frame coordinates = XYZ
//  Plane frame coordinates = UVW
//
// Transformations from one from to the other are :
//  XYZ = InverseRotation of (UVW) - Translations
//  UVW = Rotation of ( XYZ - Translations)
//
// Parameters:
//  - translation vector is _fTr[3] (in micrometers)
//      these coordinates are given in the tracker frame.
//  - rotation angles vector _fTh[3] (in radian)
//      rotations around the following axis 0=z 1=y' 2=x".
//                                                                     //
// Standard usage:
// - First set the alignment parameters with                           //
//   SetTranslation( tx, ty, tz)                                       //
//   SetRotations( thz, thx, thy)                                      //
// - Transformation hit coordinates from plane frame                   //
//   to tracker frame with:                                            //
//   DR3 hit_x = TransformHitToTracker(DR3 hit_u);                     //
// - Transformation track coordinated from plane frame                 //
//   to tracker frame with:                                            //
//   DR3 track_u = TransformTrackToPlane( Double_t tx, Double_t ty, Double_t tz, Double_t tdx, Double_t tdy);                                             //
// - if the alignment parameters are relative to another alignment (refAlign),
//   the absolute alignment (wrt the tracker) can be obtained with
//   ConvoluteAlignment( DPRecAlign *refAlign)
//
// For alignment purpose when a minimization is required:
//  - Accumulate pairs of matching hit-track with
//   NewData(Double_t auh,Double_t avh,Double_t atx,Double_t aty,Double_t atz, Double_t atdx, Double_t atdy)
//                                                                     //
//                                                                     //
/////////////////////////////////////////////////////////////////////////
//*-- Modified :  LC 2015/01/.. New definition of matrices, Convolute Alignement, calculate planes, decompose rotation --> Method 1
//*-- Modified :  LC 2015/01/.. Previous matrices was the transpose matrices.
//*-- Modified :  LC 2015/01/.. New definition return good matrices elements for global alignment.
//*-- Modified :  LC 2014/12/05 DecomposeRotations() -> cuts removed (bad tilts with ConvoluteAlignment)
//*-- Modified :  JB 2014/02/17 ConvoluteAlignment, CopyAlignment
//*-- Modified :  JB 2012/09/05 add method to reset datapoints
//*-- Modified :  JB 2012/07/16 bug fix in CalculatePlane
//*-- Modified :  JB 2012/05/04 new method to compute plane equation
//*-- Modified :  JB 2011/10/27 names _A/B/C replaced by _A/B/Ccoeff
//*-- Modified :  JB 2011/07/25 Translation sign changed to match plane correct position
//*-- Modified :  JB 2011/04/04 add rotation decomposition & correct rotation convolution
//*-- Modified :  JB 2010/11/25 overloaded method Set and new rotation methods
//*-- Modified :  JB 2010/09/08 Compute the plane equation properly with the Z translation
//*-- Modified :  JB 2010/09/01 to include rotation matrix for Plane->Tracker
//*-- Author   :  G.Orazi 1/2/2000
#include "Riostream.h"
#include <math.h>
//*KEEP,DPrecAlign.
#include "DPrecAlign.h"
#include "DR3.h"
#include "TMath.h"
ClassImp(DataPoints)
ClassImp(DPrecAlign)
//______________________________________________________________________________
//
DataPoints::~DataPoints()
{
  // DataPoints default destructor
}
//______________________________________________________________________________
//
DataPoints::DataPoints(Double_t auh,Double_t avh,
		       Double_t aresU,Double_t aresV,
		       Double_t atx,Double_t aty,Double_t atz, Double_t atdx, Double_t atdy)
{
  _uh   = auh ;
  _vh   = avh ;
  _resU = aresU ;
  _resV = aresV ;
  _tx   = atx ;
  _ty   = aty ;
  _tz   = atz ;
  _tdx  = atdx ;
  _tdy  = atdy ;
}
//______________________________________________________________________________
//
void DataPoints::Set(Double_t auh,Double_t avh,
		     Double_t aresU,Double_t aresV,
		     Double_t atx,Double_t aty,Double_t atz, Double_t atdx, Double_t atdy)
{
  _uh   = auh ;
  _vh   = avh ;
  _resU = aresU ;
  _resV = aresV ;
  _tx   = atx ;
  _ty   = aty ;
  _tz   = atz ;
  _tdx  = atdx;
  _tdy  = atdy;
}
//______________________________________________________________________________
//
void DataPoints::PrintData()
{
  printf("DataPoint: hit: u=%f  v=%f\n           res: resU=%f  resV=%f\n           track: x=%f  y=%f  z=%f dx/dz=%f dy/dz=%f\n",
         _uh,_vh,
         _resU,_resV,
	 _tx,_ty,_tz,_tdx,_tdy);
}
//______________________________________________________________________________
//
DR3 DataPoints::GetHitPosition()
{
  DR3 tmp(_uh,_vh,0.0);
  return tmp ;
}
//______________________________________________________________________________
//
DR3 DataPoints::GetHitResolution()
{
  DR3 tmp(_resU,_resV,0.0);
  return tmp;
}
//______________________________________________________________________________
//
DR3 DataPoints::GetTrackOrigin()
{
  DR3 tmp(_tx,_ty,_tz);
  return tmp ;
}
//______________________________________________________________________________
//
DR3 DataPoints::GetTrackDirection()
{
  DR3 tmp(_tdx,_tdy,1.);
  return tmp ;
}
//______________________________________________________________________________
//
DPrecAlign::DPrecAlign(const DPrecAlign &aobj)
{

  cout << "WARNING: copy constructor of DPrecAlign does nothing !!!" << endl;

}
//______________________________________________________________________________
//
DPrecAlign::DPrecAlign( Int_t method )
{
  SetDebug(0);
  if(fDebugDPrecAlign)   cout << "DPrecAlign constructor" << endl;
  DPrecAlignMethod = method;
  _fIfDeformation = 0;

}

//______________________________________________________________________________
//
DPrecAlign::DPrecAlign()
{
  SetDebug(0);
  if(fDebugDPrecAlign)   cout << "DPrecAlign constructor : BEWARE Method 0 !!!" << endl;
  DPrecAlignMethod = 0;
  _fIfDeformation = 0;
}

//______________________________________________________________________________
//
DPrecAlign::~DPrecAlign()
{
  // DPrecAlign default destructor
}
//______________________________________________________________________________
//
void DPrecAlign::NewData(Double_t auh,Double_t avh,
			 Double_t aresU,Double_t aresV,
			 Double_t atx,Double_t aty,Double_t atz, Double_t atdx, Double_t atdy)
{
  _data.Add( (new DataPoints(auh,avh,
			     aresU,aresV,
			     atx,aty,atz,atdx,atdy)) ) ;
}
//______________________________________________________________________________
//
void DPrecAlign::NewData1(DataPoints* myDataPoint)
{
  _data1.Add( myDataPoint ) ;
}
//______________________________________________________________________________
//
void DPrecAlign::NewData(DataPoints* myDataPoint)
{
  _data.Add( myDataPoint ) ;
}
//______________________________________________________________________________
//
void DPrecAlign::RemoveData(DataPoints* myDataPoint)
{
  _data.Remove(myDataPoint);
}
//______________________________________________________________________________
//
void DPrecAlign::RemoveData1(DataPoints* myDataPoint)
{
  _data1.Remove(myDataPoint);
}
/*
//______________________________________________________________________________
//
void DPrecAlign::NewMiniVector(DTrack* myMV)
{
  _dataMV.Add( myMV ) ;
}
//______________________________________________________________________________
//
void DPrecAlign::NewMiniVector1(DTrack* myMV1)
{
  _dataMV1.Add( myMV1 ) ;
}
//______________________________________________________________________________
//
void DPrecAlign::RemoveMiniVector(DTrack* myMV)
{
  _dataMV.Remove(myMV);
}
//______________________________________________________________________________
//
void DPrecAlign::RemoveMiniVector1(DTrack* myMV1)
{
  _dataMV1.Remove(myMV1);
}
*/
//______________________________________________________________________________
//
void DPrecAlign::ListDataPoints()
{
  cout<< "Listing data..." << endl;
  DataPoints *current ;
  current = (DataPoints*) _data.First();

  //DTrack* miniVector;
  //miniVector = (DTrack*) _dataMV1.First();

  while(current) {
   current->PrintData() ;
   //miniVector->GetHit(1)->GetPositionCG()->Print();
   current=(DataPoints*) _data.After(current) ;
   //miniVector = (DTrack*) _dataMV1.After(miniVector);
  }
}
//______________________________________________________________________________
//
Int_t DPrecAlign::DataSize()
{
   return _data.GetSize();
}
//______________________________________________________________________________
//
Int_t DPrecAlign::DataSize1()
{
   return _data1.GetSize();
}
/*
//______________________________________________________________________________
//
Int_t DPrecAlign::DataSizeMV()
{
   return _dataMV.GetSize();
}
//______________________________________________________________________________
//
Int_t DPrecAlign::DataSizeMV1()
{
   return _dataMV1.GetSize();
}
*/
//______________________________________________________________________________
//
void DPrecAlign::SetInitialRotations()
{
 //initialRotations = new Double_t[3];
 for(Int_t i=0; i<3; ++i) _initialRotations[i] = _fTh[i];
}
//______________________________________________________________________________
//
void DPrecAlign::SetInitialPosition()
{
 //initialPosition = new Double_t[3];
 for(Int_t i=0; i<3; ++i) _initialPosition[i] = _fTr[i];
}
//______________________________________________________________________________
//
void DPrecAlign::SetRotations(Double_t th0,Double_t th1,Double_t th2)
{
  //th0, th1, th2 : Rotation angles around z, y' and x" axis

  //    printf("Setting Rotations... %11f %11f  %11f\n",th0,th1,th2);
  //  cout << "th0= " << th0/3.1415*180. <<  endl;
  //  cout << "th1= " << th1/3.1415*180. <<  endl;
  //  cout << "th2= " << th2/3.1415*180. <<  endl;

  _fTh[0]= th0  ;
  _fTh[1]= th1  ;
  _fTh[2]= th2  ;
  ComputeRotationMatrix()  ;
  CalculatePlane();
}
//______________________________________________________________________________
//
void DPrecAlign::SetRotations(DR3 tilt)
{
  //tilt[0], tilt[1], tilt[2] should be the rotation angles around the z, y' and x" axis
 // JB 2010/11/25

  SetRotations( tilt(0), tilt(1), tilt(2));
}
//______________________________________________________________________________
//
void DPrecAlign::SetTranslation(Double_t t0,Double_t t1,Double_t t2 )
{
  // Sets a translation

  //printf("translation: \n") ;
  //printf("translation: %f %f %f \n",t0, t1, t2) ;
  _fTr[0]=t0 ;
  _fTr[1]=t1 ;
  _fTr[2]=t2 ;
  //printf("DPrec::SetTranslation to %f %f %f \n",_fTr[0], _fTr[1], _fTr[2]) ;
}
//______________________________________________________________________________
//
void DPrecAlign::SetTranslation(DR3 tr)
{
  //tr[0], tr[1], tr[2] should be the translation along the x y z axis
 // JB 2010/11/25

  SetTranslation( tr(0), tr(1), tr(2));
}
//______________________________________________________________________________
//
void DPrecAlign::CopyAlignment( DPrecAlign *anAlign)
{
  // Copy the alignment parameters of anAlign,
  // and recompute everything.
  //
  // JB 2014/02/17
  DPrecAlignMethod = anAlign->GetDPrecAlignMethod();
  SetTranslation( anAlign->GetTranslation()[0], anAlign->GetTranslation()[1], anAlign->GetTranslation()[2]);
  SetRotations( anAlign->GetRotations()[0], anAlign->GetRotations()[1], anAlign->GetRotations()[2]);
}

//______________________________________________________________________________
//
void DPrecAlign::SetDeformation( Double_t *coeffU, Double_t *coeffV)
{
  // Define deformation parameters
  // JB 2015/10/31

  if( coeffU!=NULL) {
    _fIfDeformation = 1;
    for ( int i=0; i<8; i++) {
      _fUDeformationCoef[i] = coeffU[i];
    }
  }

  if( coeffV!=NULL) {
    _fIfDeformation = 1;
    for ( int i=0; i<8; i++) {
      _fVDeformationCoef[i] = coeffV[i];
    }
  }

  PrintDeformations();

}

//______________________________________________________________________________
//
void DPrecAlign::ComputeRotationMatrix()
{
  //Compute the rotation matrix from Tracker frame to Plane Frame
  // alpha is the rotation around z
  // beta is the rotation around y'
  // gamma is the rotation around x"
  // final matrix is gamma*beta*alpha
  //
  // This method is called when the rotation angles are set
  //  with SetRotations( thz, thy, thx).
  //
  // Matrix element expression assuming:
  // ca = cos(alpha), sb=sin(beta), cg/sg=cos/sin(gamma)
  //
  // ca.cb            cb.sa             -sb
  // ca.sb.sg-sa.sg   sa.sb.sg+ca.cg    cb.sg
  // ca.sb.cg-sa.sg   sa.sb.cg-ca.sg    cb.cg

  if(DPrecAlignMethod==0) {

    //cout << "computing rotation matrix..." << endl;
    Double_t c[3] ;
    Double_t s[3] ;
    for (Int_t i=0 ; i<3 ; i++){
      c[i]=cos(_fTh[i]);
      s[i]=sin(_fTh[i]);
    }
    // Three rotation matrices:
    _alpha[0][0] = c[0];
    _alpha[0][1] = s[0];
    _alpha[0][2] = 0.;
    _alpha[1][0] = -s[0];
    _alpha[1][1] = c[0];
    _alpha[1][2] = 0.;
    _alpha[2][0] = 0.;
    _alpha[2][1] = 0.;
    _alpha[2][2] = 1.;
    //
    _beta[0][0] = c[1];
    _beta[0][1] = 0.;
    _beta[0][2] = -s[1];
    _beta[1][0] = 0.;
    _beta[1][1] = 1.;
    _beta[1][2] = 0.;
    _beta[2][0] = s[1];
    _beta[2][1] = 0.;
    _beta[2][2] = c[1];
    //
    _gamma[0][0] = 1.;
    _gamma[0][1] = 0.;
    _gamma[0][2] = 0.;
    _gamma[1][0] = 0.;
    _gamma[1][1] = c[2];
    _gamma[1][2] = s[2];
    _gamma[2][0] = 0.;
    _gamma[2][1] = -s[2];
    _gamma[2][2] = c[2];
    //
    for (Int_t i=0; i<3; i++) {
      for (Int_t j=0; j<3; j++) {
        _rotmat[i][j] = 0.;
        for (Int_t l=0; l<3; l++) {
          for (Int_t k=0; k<3; k++) _rotmat[i][j]+=_gamma[i][l]*_beta[l][k]*_alpha[k][j];
        }
      }
    }
  } // end if Method 0

  if(DPrecAlignMethod==1) {

    Double_t angle_x = +_fTh[2];  // Horaire (-) Anti-Horaire (+)
    Double_t angle_y = +_fTh[1];
    Double_t angle_z = +_fTh[0];

    Double_t A = cos(angle_x);
    Double_t B = sin(angle_x);
    Double_t C = cos(angle_y);
    Double_t D = sin(angle_y);
    Double_t E = cos(angle_z);
    Double_t F = sin(angle_z);

    Double_t AD = A*D;
    Double_t BD = B*D;

    // Rotation X,Y,Z :
    _rotmat[0][0] = C*E;
    _rotmat[0][1] = BD*E - A*F;
    _rotmat[0][2] = AD*E + B*F;
    _rotmat[1][0] = C*F;
    _rotmat[1][1] = BD*F + A*E;
    _rotmat[1][2] = AD*F - B*E;
    _rotmat[2][0] = -D;
    _rotmat[2][1] = B*C;
    _rotmat[2][2] = A*C;

    /*
    // Rotation Z,Y,X
    _rotmat[0][0] = C*E;
    _rotmat[0][1] = -C*F;
    _rotmat[0][2] = D;
    _rotmat[1][0] = BD*E + A*F;
    _rotmat[1][1] = -BD*F + A*E;
    _rotmat[1][2] = -B*C;
    _rotmat[2][0] = -AD*E + B*F;
    _rotmat[2][1] = AD *F + B*E;
    _rotmat[2][2] = A*C;
    */
  }

  ComputeTorationMatrix(); // JB 2010/09/08
  //PrintRotationMatrix();
}
//______________________________________________________________________________
//
void DPrecAlign::ComputeTorationMatrix()
{
  //Compute the rotation matrix from Plane frame to Tracker Frame
  // ahpla is the inverse rotation around z
  // ateb is the inverse rotation around y
  // ammag is the inverse rotation around x
  // final matrix is ammag*ateb*ahpla
  //
  // This method is called when the rotation matrix is computed
  //  with ComputeRotationMatrix().
  //
  // Created: JB 2010/09/03
  if(DPrecAlignMethod==0) {

    // cout << "computing rotation matrix..." << endl;
    Double_t c[3] ;
    Double_t s[3] ;
    for (Int_t i=0 ; i<3 ; i++){
      c[i]=cos(_fTh[i]);
      s[i]=sin(_fTh[i]);
    }
    // Three rotation matrices:
    _ahpla[0][0] = c[0];
    _ahpla[0][1] = -s[0];
    _ahpla[0][2] = 0.;
    _ahpla[1][0] = s[0];
    _ahpla[1][1] = c[0];
    _ahpla[1][2] = 0.;
    _ahpla[2][0] = 0.;
    _ahpla[2][1] = 0.;
    _ahpla[2][2] = 1.;
    //
    _ateb[0][0] = c[1];
    _ateb[0][1] = 0.;
    _ateb[0][2] = s[1];
    _ateb[1][0] = 0.;
    _ateb[1][1] = 1.;
    _ateb[1][2] = 0.;
    _ateb[2][0] = -s[1];
    _ateb[2][1] = 0.;
    _ateb[2][2] = c[1];
    //
    _ammag[0][0] = 1.;
    _ammag[0][1] = 0.;
    _ammag[0][2] = 0.;
    _ammag[1][0] = 0.;
    _ammag[1][1] = c[2];
    _ammag[1][2] = -s[2];
    _ammag[2][0] = 0.;
    _ammag[2][1] = s[2];
    _ammag[2][2] = c[2];
    //
    for (Int_t i=0; i<3; i++) {
      for (Int_t j=0; j<3; j++) {
        _tormat[i][j] = 0.;
        for (Int_t l=0; l<3; l++) {
          for (Int_t k=0; k<3; k++) _tormat[i][j]+=_ahpla[i][l]*_ateb[l][k]*_ammag[k][j];
        }
      }
    }
  } // end if Method 0

  if(DPrecAlignMethod==1) {

    Double_t angle_x = -_fTh[2];  // (+) Anti-Horaire (-) Horaire
    Double_t angle_y = -_fTh[1];
    Double_t angle_z = -_fTh[0];

    Double_t A = cos(angle_x);
    Double_t B = sin(angle_x);
    Double_t C = cos(angle_y);
    Double_t D = sin(angle_y);
    Double_t E = cos(angle_z);
    Double_t F = sin(angle_z);

    Double_t AD = A*D;
    Double_t BD = B*D;

    // Rotation Z,Y,X

    _tormat[0][0] = C*E;
    _tormat[0][1] = -C*F;
    _tormat[0][2] = D;
    _tormat[1][0] = BD*E + A*F;
    _tormat[1][1] = -BD*F + A*E;
    _tormat[1][2] = -B*C;
    _tormat[2][0] = -AD*E + B*F;
    _tormat[2][1] = AD *F + B*E;
    _tormat[2][2] = A*C;

    /*
    // Rotation X,Y,Z
    _tormat[0][0] = C*E;
    _tormat[0][1] = BD*E - A*F;
    _tormat[0][2] = AD*E + B*F;
    _tormat[1][0] = C*F;
    _tormat[1][1] = BD*F + A*E;
    _tormat[1][2] = AD*F - B*E;
    _tormat[2][0] = -D;
    _tormat[2][1] = B*C;
    _tormat[2][2] = A*C;
    */
  }

}
//______________________________________________________________________________
//
void DPrecAlign::CalculatePlane()
{
  // A,B and C are parameters of the plane: z+Ax+By+C=0
  //
  // This method is called when the rotation angles are set
  //  with SetRotations( thz, thy, thx), just after the rotation matrix
  //  has been calculated.
  //
  // The original method crashed in certain cases and was replaced by a
  //  more robust computation.
/*
  // Original method by G.Orazi
  // e0,e1 and e2 are points that determine the plane
  // we start with the original plane in the UVW frame,
  // then rotate it into the XYZ frame and find the A,B and C params
  // of the rotated plane
  DR3 e00(0.,0.,0.);
  DR3 e10(1.,0.,0.);
  DR3 e20(0.,1.,0.);
  DR3 e0 = TransformHitToTracker(e00);
  DR3 e1 = TransformHitToTracker(e10);
  DR3 e2 = TransformHitToTracker(e20);
  _Bcoeff=((e2(2)-e1(2))*e0(0)+(e0(2)-e2(2))*e1(0)+(e1(2)-e0(2))*e2(0))/
    ((e0(0)-e1(0))*(e1(1)-e2(1))+(e0(1)-e1(1))*(e2(0)-e1(0)));
  _Ccoeff=(_Bcoeff*(e0(1)*e1(0)-e0(0)*e1(1))-e0(0)*e1(2)+e1(0)*e1(2)+e0(2)*e1(0)-e1(2)*e1(0))/(e0(0)-e1(0));
  _Acoeff=(-e0(2)+e1(2)-_Bcoeff*(e0(1)-e1(1)))/(e0(0)-e1(0));

  printf("DPrec::CalculatePlane e00: %f %f %f\n",e00(0),e00(1),e00(2));
  printf("DPrec::CalculatePlane e0: %f %f %f\n",e0(0),e0(1),e0(2));
  printf("DPrec::CalculatePlane e01: %f %f %f\n",e10(0),e10(1),e10(2));
  printf("DPrec::CalculatePlane e1: %f %f %f\n",e1(0),e1(1),e1(2));
  printf("DPrec::CalculatePlane e02: %f %f %f\n",e20(0),e20(1),e20(2));
  printf("DPrec::CalculatePlane e2: %f %f %f\n",e2(0),e2(1),e2(2));
  printf("DPrec::CalculatePlane: old way plane: z+ %f x + %f y + %f =0\n", _Acoeff,_Bcoeff,_Ccoeff);
*/
  // New method by JB 2012/05/04
  // !!! Correction on 2012/07/16 R[2][x] changed to R[x][2] !!!
  // A plane equation in the telescope frame writes like V.X = c
  //  where V is a row-vector (a, b, 1) and X the column vector (x, y, z).
  // The same plane equation in the plane frame writes
  //  W.U = 0 or w=0 with W=row(0,0,1) and U=column(u,v,w).
  // Taking into account that U = R (X-T) where:
  //   o R is a rotation matrix,
  //   o T is a shift vector.
  // We simply get : W.R(X-T)=0 wich translates into (WR)X=WRT.
  // So we can identify the vector WR=V and the scalar WRT=c.
  // Finally, replacing the elements, we have :
  //  c = (RT)[2]/R[2][2]
  //  a = R[0][2]/R[2][2]
  //  b = R[1][2]/R[2][2]
  // Of course, when R[2][2]=0 the plane cannot be // z.

  if(DPrecAlignMethod==0) {

    DR3 aVect;
    DR3 trans(_fTr[0],_fTr[1],_fTr[2]);
    aVect = RotateToPlane( trans);
    if( _rotmat[2][2] != 0. ) {
      _Ccoeff = -aVect(2)/_rotmat[2][2];
      _Acoeff = _rotmat[0][2]/_rotmat[2][2];
      _Bcoeff = _rotmat[1][2]/_rotmat[2][2];
     //std::cout<<"Calculate coefficients"<<_Acoeff<<"   "<<_Bcoeff<<"   "<<_Ccoeff<<std::endl;
    }
    else {
      printf( "\n*** WARNING *** can't compute plane equation !\n   --> parameters set to 0.\n\n");
      _Ccoeff = 0.;
      _Acoeff = 0.;
      _Bcoeff = 0.;
    }
    //  printf("DPrec::CalculatePlane: plane: z+ %f x + %f y + %f =0\n", _Acoeff,_Bcoeff,_Ccoeff);
  } // end if Method 0

  if(DPrecAlignMethod==1) {

    // New version :
    // Rotate (x,y,z) to (u,v,w) : _rotmat
    // Last Modified : LC 2015/01/30 Now we use the real definition of the rotation matrix.

    Double_t mat22 = _rotmat[2][2];

    if( mat22 != 0 ) {

      _Acoeff = _rotmat[2][0] / mat22;
      _Bcoeff = _rotmat[2][1] / mat22;
      _Ccoeff = -(_rotmat[2][0]/mat22*_fTr[0] + _rotmat[2][1]/mat22*_fTr[1] + _fTr[2]);
    }

    else {

      printf( "\n*** WARNING *** can't compute plane equation !\n   --> parameters set to 0.\n\n");
      _Ccoeff = 0.;
      _Acoeff = 0.;
      _Bcoeff = 0.;
    }
  } // end if Method 1

}
//______________________________________________________________________________
//
void DPrecAlign::CalculateIntersection(DataPoints* aDataPoint)
{
  //If a plane is determined as Ax+By+z+C=0
  // and a line - as x=x0+t*a
  //                 y=y0+t*b
  //                 z=z0+t*c
  // then a crossection point is at -t=Ax0+By0+z0+C/(Aa+Bb+c)
  // and one can calculate x,y,z of that point
  // All computation in telescope frame
  //
  // Modified JB 2015/10/31 to store track slope

  DR3 TrackOrigin = aDataPoint->GetTrackOrigin();
  DR3 TrackDirection = aDataPoint->GetTrackDirection();
  Double_t tx=TrackOrigin(0);
  Double_t ty=TrackOrigin(1);
  Double_t tz=TrackOrigin(2);
  Double_t tdx=TrackDirection(0);
  Double_t tdy=TrackDirection(1);
  Double_t t=-(_Acoeff*tx+_Bcoeff*ty+tz+_Ccoeff)/(_Acoeff*tdx+_Bcoeff*tdy+1.);

  //std::cout<<"A= "<<_Acoeff<<"   B= "<<_Bcoeff<<"   C= "<<_Ccoeff<<"    t = "<<t<<std::endl;
  //printf("DPrec::CalculateIntersection: tx=%.2f, ty=%.2f, tz=%.2f, tdx=%e, tdy=%e, t=%e\n", tx, ty, tz, tdx, tdy, t);

  _xh = tx+t*tdx;
  _yh = ty+t*tdy;
  _zh = tz+t*1.;
  _xtd = tdx;
  _ytd = tdy;
  _ztd = 1.;
  //printf("DPrec::CalculateIntersection: track pos -xh, -yh, -zh %f %f %f \n", _xh, _yh, _zh);
}

//______________________________________________________________________________
//
DR3 DPrecAlign::DeformedLocalPoint(DR3& aPlaneCoordinate){

  // Use the model for the plane deformation
  //  to change the w coordinate of the given point in the local plane frame.
  // Note that only w is changed, (u,v) are unchanged.
  //
  // JB 2014/04/21

  DR3 aDeformedPoint = aPlaneCoordinate;
  double unorm = aPlaneCoordinate(0);
  aDeformedPoint += DR3( 0., 0., fTool.CLof7LegendrePol( &unorm, _fUDeformationCoef) );
  double vnorm = aPlaneCoordinate(1);
  aDeformedPoint += DR3( 0., 0., fTool.CLof7LegendrePol( &vnorm, _fVDeformationCoef) );
  if(fDebugDPrecAlign) {
    printf("DPrecAlign::DeformedLocalPoint deformation applied.\n");
    printf("  from: %.1f, %.1f, %1.f to %.1f, %.1f, %1.f\n", aPlaneCoordinate(0), aPlaneCoordinate(1), aPlaneCoordinate(2), aDeformedPoint(0), aDeformedPoint(1), aDeformedPoint(2));
  }

  return aDeformedPoint;
}

//______________________________________________________________________________
//
void DPrecAlign::PrintRotationMatrix()
{
  printf("  Rotation Matrix with angles around z = %.3f, y = %.3f, x = %.3f deg\n", _fTh[0]*TMath::RadToDeg(), _fTh[1]*TMath::RadToDeg(), _fTh[2]*TMath::RadToDeg());
  for (Int_t i=0 ; i<3 ; i++){
    for (Int_t j=0 ; j<3 ; j++){
      printf("%f ",_rotmat[i][j]);
    }
    printf("\n");
  }
}

//______________________________________________________________________________
//
void DPrecAlign::PrintTorMatrix()
{
  printf(" Inverse Rotation Matrix with angles around z = %.3f, y = %.3f, x = %.3f deg\n", _fTh[0]*TMath::RadToDeg(), _fTh[1]*TMath::RadToDeg(), _fTh[2]*TMath::RadToDeg());
  for (Int_t i=0 ; i<3 ; i++){
    for (Int_t j=0 ; j<3 ; j++){
      printf("%f ",_tormat[i][j]);
    }
    printf("\n");
  }
}

//______________________________________________________________________________
//
void DPrecAlign::PrintRotTimeInv()
{

 Double_t rotmat_times_tormat[3][3];
  for (Int_t i=0; i<3; i++) {
    for (Int_t j=0; j<3; j++) {
      rotmat_times_tormat[i][j] = 0.;
      for (Int_t l=0; l<3; l++) rotmat_times_tormat[i][j] += _rotmat[i][l]*_tormat[l][j];
    }
  }


  printf("Rotation Matrix Times Inverse Rotation Matrixwith angles around z = %.3f, y = %.3f, x = %.3f deg\n", _fTh[0]*TMath::RadToDeg(), _fTh[1]*TMath::RadToDeg(), _fTh[2]*TMath::RadToDeg());
  for (Int_t i=0 ; i<3 ; i++){
    for (Int_t j=0 ; j<3 ; j++){
      printf("%f ",rotmat_times_tormat[i][j]);
    }
    printf("\n");
  }
}

//______________________________________________________________________________
//
void DPrecAlign::PrintTranslations()
{
  printf("  Translation vector: ");
  for (Int_t i=0 ; i<3 ; i++){
    printf("%f ",_fTr[i]);
  }
  printf(" um\n");
}

//______________________________________________________________________________
//
void DPrecAlign::PrintDeformations()
{
  // JB 2015/10/31

  printf("  Deformation coefficients: \n");

  if ( _fUDeformationCoef!=NULL ) {
    printf("    U: ");
    for (Int_t i=0 ; i<8 ; i++){
      printf("%.2f ",_fUDeformationCoef[i]);
    }
    printf("\n");
  }
  else {
    printf("    -> no deformation in U!\n");
  }

  if ( _fVDeformationCoef!=NULL ) {
    printf("    V: ");
    for (Int_t i=0 ; i<8 ; i++){
      printf("%.2f ",_fVDeformationCoef[i]);
    }
    printf("\n");
  }
  else {
    printf("    -> no deformation in V!\n");
  }

}

//______________________________________________________________________________
//
void DPrecAlign::PrintAll()
{
 // JB 2010/09/08
 // Modified: JB, 2015/10/31 print deformation added

  printf("\n  Plane equation (XYZ frame, um): z + %.2f x + %.2f y + %.2f = 0\n", _Acoeff, _Bcoeff, _Ccoeff);
  PrintRotationMatrix();
  PrintTranslations();
  if( _fIfDeformation ) PrintDeformations();
  printf("\n");
}

//______________________________________________________________________________
//
DR3 DPrecAlign::RotateToPlane(DR3 xxx)
{
  // Rotate a vector from the telescope frame to the plane frame
  // Mathematical operation is: uuu = rotMatrix * xxx
  //
  // Created: JB 2010/11/25
  DR3 uuu(0.0,0.0,0.0) ;
  //printf("DPrec::RotateToPlane: xxx= %f %f %f\n",xxx(0),xxx(1),xxx(2));

  if(DPrecAlignMethod==0) {

    for (Int_t i=0 ; i< 3 ; i++){
      for (Int_t j=0 ; j< 3 ; j++){
        uuu(i)+=_rotmat[j][i]*xxx(j);
      }
    }
  } // end if Method 0

  if(DPrecAlignMethod==1) {

    for (Int_t i=0 ; i< 3 ; i++){
      for (Int_t j=0 ; j< 3 ; j++){
        uuu(i)+=_rotmat[i][j]*xxx(j);
      }
    }
  } // end if Method 1

  //printf("DPrec::RotateToPlane: rotated vector= %f %f %f\n",uuu(0),uuu(1),uuu(2));
  return uuu;
}
//______________________________________________________________________________
//
DR3 DPrecAlign::RotateToTracker( Double_t u, Double_t v, Double_t w)
{
  // Rotate a (U,V,W) vector from the plane frame to the telescope frame
  //
  // Created: JB 2010/11/25
  DR3 uuu( u, v, w);
  DR3 xxx = RotateToTracker( uuu);
  return xxx;
}
//______________________________________________________________________________
//
DR3 DPrecAlign::RotateToTracker(DR3 uuu)
{
  // Rotate a vector from the plane frame to the telescope frame
  //
  // Created: JB 2010/11/25

  //printf("DPrec::RotateToTracker: uuu= %f %f %f\n",uuu(0),uuu(1),uuu(2));
  DR3 xxx(0.0,0.0,0.0) ;

  if(DPrecAlignMethod==0) {

    for (Int_t i=0 ; i< 3 ; i++){
      for (Int_t j=0 ; j< 3 ; j++){
        xxx(i)+=_tormat[j][i]*uuu(j);
      }
    }
  } // end if Method 0

  if(DPrecAlignMethod==1) {

    for (Int_t i=0 ; i< 3 ; i++){
      for (Int_t j=0 ; j< 3 ; j++){
        xxx(i)+=_tormat[i][j]*uuu(j);
      }
    }
  } // End if Method 1

  //printf("DPrec::RotateToTracker: rotated vector= %f %f %f\n",xxx(0),xxx(1),xxx(2));
  return xxx;
}
//______________________________________________________________________________
//
DR3 DPrecAlign::TransformHitToTracker(DR3 uuu)
{
  // Boost the given position from the plane frame to the telescope frame
  //
  // Created: JB 2010/09/03
  // Modified JB 2015/10/31 take deformation into account

  DR3 aPoint = uuu;

  // Take into account deformation if required
  if( _fIfDeformation ) {
    aPoint = DeformedLocalPoint( uuu);
    if (fDebugDPrecAlign) {
      printf("DPrecAlign::PlaneToTracker deformation applied.\n");
      printf("  from: %.1f, %.1f, %1.f to %.1f, %.1f, %1.f\n", uuu(0), uuu(1), uuu(2), aPoint(0), aPoint(1), aPoint(2));
    }
  }

  DR3 res(0.0,0.0,0.0) ;
  DR3 trans(_fTr[0],_fTr[1],_fTr[2]);
  res = RotateToTracker( aPoint) + trans; // changed to +, JB 2011/07/25
  //printf("DPrec::Hit2Tracker transformed vector xyz = RotToTracker( uvw[%.1f, %.1f, %.1f] ) + trans[%.1f, %.1f, %.1f] = %f %f %f\n",aPoint(0), aPoint(1), aPoint(2), trans(0), trans(1), trans(2), res(0),res(1),res(2));
  return res;
}

//______________________________________________________________________________
//
DR3 DPrecAlign::TransformHitToPlane(DR3 xxx)
{
  // Boost the given position from the tracker frame to the plane frame
  // Use the mechanism built for track, though there is no need
  // to find the track intersection with the plane
  //
  // Created: JB 2010/11/25
  //printf("DPrec::Hit2Plane xyz= %f %f %f\n",xxx(0),xxx(1),xxx(2));
  SetTrackPosition( xxx);
  DR3 res = TransformTrackToPlane();
  //printf("DPrec::Hit2Plane transformed vector uvw= %f %f %f\n",res(0),res(1),res(2));
  return res;
}

//______________________________________________________________________________
//
DR3 DPrecAlign::TransformTrackToPlane()
{
  // Returns the position where the track crosses the plane
  // in the plane frame
  // Assumes the position of the track at the plane but in the telescop frame is known
  //
  // Note the plane is considered as flat with no deformation,
  //  to take into account deformation, call DPlane::Intersection

  DR3 tmp = GetTrackPosition() ;
  DR3 trans(_fTr[0],_fTr[1],_fTr[2]);
  DR3 aFlatPoint(0.0,0.0,0.0) ;
  aFlatPoint = RotateToPlane( tmp - trans ); // changed to -, JB 2011/07/25

  //printf("DPrec::Track2PLane transformed vector in uvw = RotToPlane( xyz[%.1f, %.1f, %.1f] - trans[%.1f, %.1f, %.1f] ) = %f %f %f\n",tmp(0), tmp(1), tmp(2), trans(0), trans(1), trans(2), aFlatPoint(0),aFlatPoint(1),aFlatPoint(2));
  //return aFlatPoint;

  // Deformation
  if( _fIfDeformation ) { // if deformation
    DR3 aDeformedPoint = DeformedLocalPoint( aFlatPoint);
    DR3 trackSlope = GetTrackSlope();
    DR3 trackSlopeInPlane = RotateToPlane( trackSlope );
    if (fDebugDPrecAlign) {
      printf("DPrecAlign::Intersection deformation applied.\n");
      printf("  from: %.1f, %.1f, %1.f to %.1f, %.1f, %1.f\n", aFlatPoint(0), aFlatPoint(1), aFlatPoint(2), aDeformedPoint(0), aDeformedPoint(1), aDeformedPoint(2));
    }
    Double_t deltaZU = aDeformedPoint(2)/tan(_fTh[1]) - aFlatPoint(2);
    Double_t deltaZV = aDeformedPoint(2) - aFlatPoint(2);
    if((_fTh[2]*180./M_PI) < 180. && abs(_fTh[2]*180./M_PI) > 175. ) {
      aDeformedPoint -= DR3( trackSlopeInPlane(0)*deltaZU, trackSlopeInPlane(1)*deltaZV, 0.);
    }
    else{
      aDeformedPoint += DR3( trackSlopeInPlane(0)*deltaZU, trackSlopeInPlane(1)*deltaZV, 0.);
    }
    // Double_t deltaZ = aDeformedPoint(2)-aFlatPoint(2);
    //aDeformedPoint += DR3( (trackSlopeInPlane(0))*deltaZ, trackSlopeInPlane(1)*deltaZ, 0.);
    if (fDebugDPrecAlign) {
      printf("  then with deltaZ=(%.1f,%.1f) and thetaPlane=%.1f to %.1f, %.1f, %1.f\n", deltaZU, deltaZV, _fTh[2]*180./M_PI, aDeformedPoint(0), aDeformedPoint(1), aDeformedPoint(2));
    }
    return aDeformedPoint;
  } // end if deformation

  else { // if no deformation
    return aFlatPoint;
  }

}

//______________________________________________________________________________
//
DR3 DPrecAlign::TransformTrackToPlane( Double_t tx, Double_t ty, Double_t tz, Double_t tdx, Double_t tdy)
{
  // Returns the position where the track crosses the plane
  // in the plane frame
  // (tx, ty, tz=0) are the coordinate of the track origin in the telescope frame
  // (tdx, tdy, tdz=1) are the slope of the track so that, in the telescope frame,
  // the track equation is: x=(z-tz)*tdx+tx, y=(z-tz)*tdy+ty, z=(z-tz)*tdz+tz
  //
  // Created: JB 2010/09/10
  DataPoints aPosition(0,0,0,0,tx,ty,tz,tdx,tdy); // track at origin in the telescope frame
  CalculateIntersection(&aPosition); // track at the plane surface in the telescope frame
  return TransformTrackToPlane(); // track at the plane surface in the plane frame
}
//______________________________________________________________________________
//
void DPrecAlign::SetTrackPosition( DR3 xxx)
{
  // Set a "track" position in the XYZ frame
  //
  // JB 2010/11/25
  _xh = xxx(0);
  _yh = xxx(1);
  _zh = xxx(2);
}
//______________________________________________________________________________
//
DR3 DPrecAlign::GetTrackPosition()
{
  // Returns the position where the track crosses the plane
  // in the telescope frame
  DR3 tmp(_xh,_yh,_zh);
  return tmp ;
}
//______________________________________________________________________________
//
DR3 DPrecAlign::GetTrackSlope()
{
  // Returns the slope of the track
  // in the telescope frame
  DR3 tmp(_xtd,_ytd,_ztd);
  return tmp ;
}
//______________________________________________________________________________
//
void DPrecAlign::ConvoluteRotation( Double_t tiltW)
{
  // Recompute the rotation angles around z,y',x"
  // taking into account an additional rotation around w
  //
  // First compute the new rotation matrix : R(z,y,x)*R-1(w,0,0)
  // Then compute the new toration matrix : R(w,0,0)*R-1(z,y,x)
  // Finaly deconvolute the rotation matrix to get the new angles
  //
  // JB 2010/11/25
  // Modified JB 2011/04/05 matrix product swapped
  //printf("DPrecAlign: convoluting rotation with angle (around W) %.5f degrees\n", tiltW*TMath::RadToDeg());
  Double_t temp[3][3];
  // the R-1(w,0,0) matrix
  Double_t torw[3][3];
  torw[0][0] = cos( tiltW);
  torw[0][1] = -sin( tiltW); // sign changed on 2011/04/04
  torw[0][2] = 0.;
  torw[1][0] = sin( tiltW); // sign changed on 2011/04/04
  torw[1][1] = cos( tiltW);
  torw[1][2] = 0.;
  torw[2][0] = 0.;
  torw[2][1] = 0.;
  torw[2][2] = 1.;

  // new rotation matrix R-1(w,0,0)*R(z,y,x)
  for (Int_t i=0; i<3; i++) {
    for (Int_t j=0; j<3; j++) {
      temp[i][j] = 0.;
      for (Int_t l=0; l<3; l++) temp[i][j] += _rotmat[i][l]*torw[l][j];
    }
  }
  for (Int_t i=0; i<3; i++) {
    for (Int_t j=0; j<3; j++) {
      _rotmat[i][j] = temp[i][j];
    }
  }
  // the R(w,0,0) matrix
  Double_t rotw[3][3];
  rotw[0][0] = cos( tiltW);
  rotw[0][1] = sin( tiltW); // sign changed on 2011/04/04
  rotw[0][2] = 0.;
  rotw[1][0] = -sin( tiltW); // sign changed on 2011/04/04
  rotw[1][1] = cos( tiltW);
  rotw[1][2] = 0.;
  rotw[2][0] = 0.;
  rotw[2][1] = 0.;
  rotw[2][2] = 1.;

  // new toration matrix R-1(z,y,x)*R(w,0,0)
  for (Int_t i=0; i<3; i++) {
    for (Int_t j=0; j<3; j++) {
      temp[i][j] = 0.;
      for (Int_t l=0; l<3; l++) temp[i][j] += rotw[i][l]*_tormat[l][j];
    }
  }
  for (Int_t i=0; i<3; i++) {
    for (Int_t j=0; j<3; j++) {
      _tormat[i][j] = temp[i][j];
    }
  }
  // deconvolute the new rotation matrix to get the angles
  DecomposeRotations();
}
//______________________________________________________________________________
//
void DPrecAlign::ConvoluteRotation( DR3 aTilt)
{
  // Recompute the rotation matrix R and angles around z,y',x"
  //  taking into account additional rotation angles from aTilt.
  //
  // Notation:
  //  o Rin is the initial rotation matrix,
  //  o Rtilt is the rotation matrix Rtilt from the agnles aTilt,
  //  o Rout is the final rotation matrix with
  //     Rout = Rin x Rtilt
  //
  // JB 2014/02/16

}

//______________________________________________________________________________
//
void DPrecAlign::ConvoluteUVWAlignment( Double_t shiftU, Double_t shiftV, Double_t tiltW)
{
  // Recompute all the alignment parameters in the XYZ frame
  // from an alignment performed in the UVW frame
  //  (only U,V shifts and W anlge)
  //
  // First convolute the W alignment angle with the initial rotation matrix
  // so far, the new rotation angles are not updated, just the matrices (2010/11/25)
  //
  // Then convolute the alignment (shift and rotation) with the initial position and rotations
  // first rotate the u-shift vector to tracker coordinates
  // which then has to be added to the plane center position.
  //
  // Also update the plane definition
  //
  // JB 2010/11/25
  DR3 shifts = RotateToTracker( shiftU, shiftV, 0.); // for translations
  //printf("DPrecAlign: convoluting translations with shift (U,V) = ( %.2f, %.2f)\n", shiftU, shiftV);
  _fTr[0] -= shifts(0);
  _fTr[1] -= shifts(1);
  _fTr[2] -= shifts(2);
  ConvoluteRotation( tiltW ); // for rotation
  CalculatePlane();
}
//______________________________________________________________________________
//
void DPrecAlign::ConvoluteAlignment( DPrecAlign *refAlignment)
{
  // Consider the current alignment is relative to the given one (refAlignment).
  // So change this alignment to be absolute wrt Telescope frame.
  //
  // Notation:
  //  o current alignment = shift vector Srel, rotation matrix Rrel,
  //  o reference alignment = shift vector Sref, rotation matrix Rref,
  //  o final aboslute alignemnt = shift vector Sabs, rotation matrix Rabs.
  //
  // Formula are:
  //  Rabs = Rref x Rrel
  //  Sabs = invRrel x Srel + Sref
  //
  // JB 2014/02/16

  // Get the shift of the reference alignment Rref
  DR3 shiftRef( refAlignment->GetTranslation()[0], refAlignment->GetTranslation()[1], refAlignment->GetTranslation()[2] );
  DR3 shiftAbs = refAlignment->RotateToTracker( DR3(_fTr[0], _fTr[1], _fTr[2]) ) + shiftRef; // RotateToTracker(DR3(shiftRef(0), shiftRef(1), 0));

  SetTranslation( shiftAbs);

  // Get the rotation matrix of the reference alignment Rref
  Double_t rotmatRef[3][3];

  Double_t *temp = refAlignment->GetRotationMatrix();
  for (Int_t i=0; i<3; i++) {
    for (Int_t j=0; j<3; j++) {
      rotmatRef[i][j] = temp[i*3+j];
    }
  }


  if(DPrecAlignMethod==0) {

    // Compute the new rotation matrix Rabs
    Double_t rotmatAbs[3][3];
    for (Int_t i=0; i<3; i++) {
      for (Int_t j=0; j<3; j++) {
        rotmatAbs[i][j] = 0.;
        for (Int_t l=0; l<3; l++) rotmatAbs[i][j] += rotmatRef[i][l]*_rotmat[l][j];
      }
    }

    // Store the new rotation matrix
    for (Int_t i=0; i<3; i++) {
      for (Int_t j=0; j<3; j++) {
        _rotmat[i][j] = rotmatAbs[i][j];
      }
    }

  }
  else if(DPrecAlignMethod==1) {

    // Compute the new rotation matrix Rabs
    Double_t rotmatAbs[3][3];
    for (Int_t i=0; i<3; i++) {
      for (Int_t j=0; j<3; j++) {
        rotmatAbs[i][j] = 0.;
        for (Int_t l=0; l<3; l++) rotmatAbs[i][j] += _rotmat[i][l]*rotmatRef[l][j];
      }
    }

    // Store the new rotation matrix
    for (Int_t i=0; i<3; i++) {
      for (Int_t j=0; j<3; j++) {
        _rotmat[i][j] = rotmatAbs[i][j];
      }
    }

  }

  // Compute the inverse of the rotation matrix
  //  using the angles obtained by decomposition of the rotation matrix
  DecomposeRotations();
  ComputeTorationMatrix();

  // Recompute plane equation
  CalculatePlane();

  if(fDebugDPrecAlign) {
    cout<<"Convolute Alignment"<<endl;
    PrintAll();
  }
}
//______________________________________________________________________________
//
void DPrecAlign::DeconvoluteAlignment( DPrecAlign *refAlignment)
{
  // Consider the current alignment is absolute wrt Telescope frame,
  // So change this alignment to be relative to the given one (refAlignment).
  //
  // Notation:
  //  o current alignment = shift vector Sabs, rotation matrix Rabs,
  //  o reference alignment = shift vector Sref, rotation matrix Rref,
  //  o final relative alignemnt = shift vector Srel, rotation matrix Rrel.
  //
  // Formula are:
  //  Rrel = invRref x Rabs
  //  Srel = Rrel x (Sabs - Sref)
  //
  // JB 2014/11/24

  // Get the toration matrix of the reference alignment Rref
  Double_t tormatRef[3][3];
  Double_t *temp = refAlignment->GetTorationMatrix();
  for (Int_t i=0; i<3; i++) {
    for (Int_t j=0; j<3; j++) {
      tormatRef[i][j] = temp[i*3+j];
    }
  }

  Double_t rotmatRel[3][3];

  if(DPrecAlignMethod==0) {
  // Compute the new rotation matrix Rrel
  for (Int_t i=0; i<3; i++) {
    for (Int_t j=0; j<3; j++) {
      rotmatRel[i][j] = 0.;
      for (Int_t l=0; l<3; l++) rotmatRel[i][j] += tormatRef[i][l]*_rotmat[l][j];
    }
  }
 }
 else if(DPrecAlignMethod==1) {
  // Compute the new rotation matrix Rrel
  for (Int_t i=0; i<3; i++) {
    for (Int_t j=0; j<3; j++) {
      rotmatRel[i][j] = 0.;
      for (Int_t l=0; l<3; l++) rotmatRel[i][j] += _rotmat[i][l]*tormatRef[l][j];
    }
  }
 }

  // Store the new rotation matrix
  for (Int_t i=0; i<3; i++) {
    for (Int_t j=0; j<3; j++) {
      _rotmat[i][j] = rotmatRel[i][j];
    }
  }

  // Compute the inverse of the rotation matrix
  //  using the angles obtained by decomposition of the rotation matrix
  DecomposeRotations();
  ComputeTorationMatrix();


  // Get the shift of the reference alignment Rref
  DR3 shiftRef( refAlignment->GetTranslation()[0], refAlignment->GetTranslation()[1], refAlignment->GetTranslation()[2] );
  // Compute the relative shift
  DR3 shiftDiff( _fTr[0]-shiftRef(0), _fTr[1]-shiftRef(1), _fTr[2]-shiftRef(2) );
  DR3 shiftRel = refAlignment->RotateToPlane( shiftDiff);
  SetTranslation( shiftRel);

  // Recompute plane equation
  CalculatePlane();

  //PrintAll();

}
//______________________________________________________________________________
//
void DPrecAlign::DecomposeRotations()
{
  // Decomposes the rotation matrix, that is:
  // compute the values of the rotation angle around z, x', y"
  // from the values of the rotation matrix
  //
  // To solve ambiguities, angle values nearest to the known ones are chosen.
  //
  // Updates the angle values in the _fTh vector,
  // [0]=z, [1]=y', [2]=x"
  //
  // JB 2011/04/04
  // LC : Caution : cuts removed.

  Double_t theta[3]; //0=z, 1=y, 2=x

  // first angle is obtained from
  // rotmat[0,2] = - sin(theta[1])
  // but we need to test if this +/- 1

// OLD CODE :: (LC 2014/12/05)
  Int_t NoGimbalLock = 1;

  if(DPrecAlignMethod==0 && NoGimbalLock==1)
  {
    // Case where theta[1] = +/- pi/2 means the beam is perpendicular to the sensor!!!
    if( TMath::Abs(_rotmat[0][2]) > (1-1e-4) ) {
      cout << " Rotation around y is pi/2 and indicates the beam is perpendicular to the sensor!!!" << endl;
      for( Short_t i=0; i<3; i++) {
        theta[i] = _fTh[i];
      }
    }
    // Case where theta[1] != +/- pi/2
    else {
      if( _rotmat[0][2] < 1.e-4 ) { // theta[1] is 0 or Pi, keep as before
        theta[1] = _fTh[1];
      }
      else {
        theta[1] = asin( - _rotmat[0][2]);
      }
      //cout << " th(y) = asin(" << _rotmat[0][2] << ") = " << theta[1] << endl;
      theta[2] = atan2( _rotmat[1][2]*cos(theta[1]), _rotmat[2][2]*cos(theta[1]) );
      //cout << " th(x) = atan(" << _rotmat[1][2]*cos(theta[1]) << " / " << _rotmat[2][2]*cos(theta[1]) << ") = " << theta[2] << endl;
      theta[0] = atan2( _rotmat[0][1]*cos(theta[1]), _rotmat[0][0]*cos(theta[1]) );
      //cout << " th(z) = atan(" << _rotmat[0][1]*cos(theta[1]) << " / " << _rotmat[0][0]*cos(theta[1]) << ") = " << theta[0] << endl;
    }

    for( Short_t i=0; i<3; i++) {
      _fTh[i] = theta[i];
    }

  } // end if test

  if(DPrecAlignMethod==0 && NoGimbalLock==0) {
    //NEW CODE :: (LC 2014/12/05) BEWARE !!!!!!!!!!!!! GIMBAL LOCK !!!!!!!!!!!!!!!!
    theta[1] = asin( -_rotmat[0][2]);
    theta[2] = atan2( _rotmat[1][2]*cos(theta[1]), _rotmat[2][2]*cos(theta[1]) );
    theta[0] = atan2( _rotmat[0][1]*cos(theta[1]), _rotmat[0][0]*cos(theta[1]) );

    for( Short_t i=0; i<3; i++) {
      _fTh[i] = theta[i];
    }
  }

  if(NoGimbalLock==0 && DPrecAlignMethod==1) {

    if( fabs(_rotmat[2][0]) < 1.e-4 ) {   // if theta[1] is 0, Pi or -Pi, keep as before
        theta[1] = _fTh[1];  // 0., PI, -PI
    }
    else {
        theta[1] = asin( -_rotmat[2][0] );
    }

    Double_t cosY = cos(theta[1]);

    Double_t cosX = _rotmat[2][2] / cosY;
    Double_t sinX = +_rotmat[2][1] / cosY;
    theta[2] = atan2(sinX, cosX); // Rad

    Double_t cosZ = _rotmat[0][0] / cosY;
    Double_t sinZ = +_rotmat[1][0] / cosY;
    theta[0] = atan2(sinZ, cosZ); // Rad

    for( Short_t i=0; i<3; i++) {
      _fTh[i] = fmod( theta[i], 2.*TMath::Pi() );
    }

  }

  if(NoGimbalLock==1 && DPrecAlignMethod==1) {

    // With rotation in this order : X, Y, Z.
    //std::cout<<"_fTh[1] = "<<_fTh[1]<<std::endl;
    //std::cout<<"_rotmat[2][0] = -sin(angleY) = "<<_rotmat[2][0]<<" asin(-_rotmat[2][0]) = "<<asin(-_rotmat[2][0] )<<std::endl;

    if(_rotmat[2][0] < 1.-1e-4 && _rotmat[2][0] > -1.+1e-4 ) {

     if( fabs(/*_rotmat[0][2])*/_rotmat[2][0]) < 1e-4 ) {   // if theta[1] is 0, Pi or -Pi, keep as before
//      theta[1] = _fTh[1];
//      Int_t moduloPi = _fTh[1]/TMath::Pi();  // 0., PI, -PI
//      theta[1] = fmod( asin( -_rotmat[0][2] ) + moduloPi*TMath::Pi() , 2.*TMath::Pi() );
      theta[1] = asin( -_rotmat[2][0] );
     }
     else{
//      Int_t moduloPi = _fTh[1]/TMath::Pi();  // 0., PI, -PI
//      theta[1] = fmod( asin( -_rotmat[0][2] ) + moduloPi*TMath::Pi() , 2.*TMath::Pi() );
       theta[1] = asin( -_rotmat[2][0] );
     }

      Double_t cosY=cos(theta[1]);  // The sign depends on the theta[1] value (0 or pi)
      //std::cout<<" theta[1] = "<<theta[1]<<" cos(theta[1]) = "<<cosY<<std::endl;
      //theta[1] = asin( -_rotmat[2][0] );
      theta[0] = atan2( _rotmat[1][0]/cos(theta[1]) , _rotmat[0][0]/cos(theta[1]) );  // Valid if cosY != 0 --> angleY != PI/2, -PI/2 Or sinY +/- 1
      theta[2] = atan2( _rotmat[2][1]/cos(theta[1]) , _rotmat[2][2]/cos(theta[1]) );  // Valid if cosY != 0 --> angleY != PI/2, -PI/2 Or sinY +/- 1
    }
    else // _rotmat[2][0] = +/- 1 -/+ Epsilon --> GIMBAL LOCK
    {

      theta[0] = .0; // We choose a arbitrary phase. Here angleZ = 0.;

      if(_rotmat[2][0]>1.-1e-4) { // _rotmat[2][0] = -sin(theta[1]

        // Not a unique solution --> theta[2]+theta[0] =  atan2( +_rotmat[1][2], +_rotmat[1][1] );
        theta[1] = -TMath::Pi()/2.;
        theta[2] = atan2( -_rotmat[0][1], -_rotmat[0][2] ) - theta[0];
      }
      else if(_rotmat[2][0]<-1.+1e-4) {

        // Not a unique solution --> theta[2]-theta[0] =  atan2( +_rotmat[1][2], +_rotmat[1][1] );
        theta[1] = +TMath::Pi()/2.;
        theta[2] = atan2( +_rotmat[0][1], +_rotmat[0][2] ) + theta[0];
      }
    }

   for( Short_t i=0; i<3; i++) {
     _fTh[i] = fmod( theta[i], 2.*TMath::Pi() );
   }


/*
    theta[1] = asin(-_rotmat[2][0]);  // (+) --> sens anti-horaire
    Double_t cosY = cos(theta[1]);

    if( fabs(cosY) > 1e-4 ) {    // No GIMBAL LOCK

      Double_t cosX = _rotmat[2][2] / cosY;
      Double_t sinX = +_rotmat[2][1] / cosY; // (-) --> sens anti-horaire.
      theta[2] = atan2(sinX, cosX); // Rad

      Double_t cosZ = _rotmat[0][0] / cosY;
      Double_t sinZ = +_rotmat[1][0] / cosY;  // (-) --> sens anti-horaire.
      theta[0] = atan2(sinZ, cosZ); // Rad

    }
    else {                               // GIMBAL LOCK !

      theta[0] = 0;

      Double_t cosX = _rotmat[1][1];
      Double_t sinX = -_rotmat[1][2];  // (+) --> sens anti-horaire.
      theta[2] = atan2(sinX, cosX);
    }

    for( Short_t i=0; i<3; i++) {
      _fTh[i] = fmod(theta[i], 2*TMath::Pi() );
    }
*/

  // Last Modified : 2015/01/30 : Now we compute the sensor tilts with the real rotation matrix.
  // With rotation in this order : X, Y, Z.
  /*
    theta[1] = TMath::ASin(+_rotmat[0][2]);         // (-) --> sens anti-horaire.
    Double_t cosY = cos(theta[1]);

    if( fabs(cosY) > 1e-4 ) {    // No GIMBAL LOCK

      Double_t cosX = _rotmat[2][2] / cosY;
      Double_t sinX = -_rotmat[1][2] / cosY;  // (+) --> sens anti-horaire.
      theta[2] = atan2(sinX, cosX); // Rad

      Double_t cosZ = _rotmat[0][0] / cosY;
      Double_t sinZ = -_rotmat[0][1] / cosY;  // (+) --> sens anti-horaire.
      theta[0] = atan2(sinZ, cosZ); // Rad

    }
    else {                               // To compose with GIMBAL LOCK :)

      theta[2] = 0;

      Double_t cosZ = _rotmat[1][1];
      Double_t sinZ = -_rotmat[1][0];      // (-) --> sens anti-horaire.
      theta[0] = atan2(sinZ, cosZ); //rad
    }

    for( Short_t i=0; i<3; i++) {
      _fTh[i] = fmod(theta[i], 2*TMath::Pi() );
    }
  */
  } // enf if Method 1

}
//______________________________________________________________________________
//
// JB, for inline compilation
void DPrecAlign::Streamer(TBuffer &R__b)
{
  // Modified: JB 2010/09/03
  if(fDebugDPrecAlign)  cout << "In DPrecAlign::Streamer "  << endl;
  if ( R__b.IsReading() ) {
    if(fDebugDPrecAlign)  cout << "Buffer is reading " << endl;
    UInt_t R__s, R__c;
    Version_t R__v = R__b.ReadVersion(&R__s,&R__c);
    if(fDebugDPrecAlign)  cout << "Version "<< R__v << endl;
    if ( R__v > 1 ) {
      DPrecAlign::Class()->ReadBuffer(R__b,this,R__v,R__s,R__c);
      return;
    }
    //====process old versions before automatic schema evolution=v1
    if(fDebugDPrecAlign)   cout << "Processing old version " << endl;
      TObject::Streamer(R__b);
      R__b.ReadStaticArray((double*)_ahpla);
      R__b.ReadStaticArray((double*)_ateb);
      R__b.ReadStaticArray((double*)_ammag);
      R__b.ReadStaticArray((double*)_tormat);
      R__b.ReadStaticArray((double*)_alpha);
      R__b.ReadStaticArray((double*)_beta);
      R__b.ReadStaticArray((double*)_gamma);
      R__b.ReadStaticArray((double*)_rotmat);
      R__b >> _Acoeff;
      R__b >> _Bcoeff;
      R__b >> _Ccoeff;
      R__b.ReadStaticArray((double*)_fTh);
      R__b.ReadStaticArray((double*)_fTr);
      R__b.ReadInt((int&)_fIfDeformation);
      R__b.ReadStaticArray((double*)_fUDeformationCoef);
      R__b.ReadStaticArray((double*)_fVDeformationCoef);
      _data.Streamer(R__b);
      _data1.Streamer(R__b);
      //__miniVectors.Streamer(R__b);
      //_dataMV.Streamer(R__b);
      //_dataMV1.Streamer(R__b);
      R__b >> _xh;
      R__b >> _yh;
      R__b >> _zh;
      R__b.ReadStaticArray((double*)_initialRotations);
      R__b.ReadStaticArray((double*)_initialPosition);
      R__b.CheckByteCount(R__s, R__c, DPrecAlign::IsA());
      //====end of old version
  } else {
  if(fDebugDPrecAlign)  cout << "Buffer is writing " << endl;
    DPrecAlign::Class()->WriteBuffer(R__b,this);
  }
}
