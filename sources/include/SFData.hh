// *****************************************
// *                                       *
// *          ScintillatingFibers          *
// *              SFData.hh                *
// *          Katarzyna Rusiecka           *
// * katarzyna.rusiecka@doctoral.uj.edu.pl *
// *          Created in 2018              *
// *                                       *
// *****************************************

#ifndef __SFData_H_
#define __SFData_H_ 1
#include "TObject.h"
#include "TString.h"
#include "TFile.h"
#include "TTree.h"
#include "TH1D.h"
#include "TROOT.h"
#include "TRandom.h"
#include "TProfile.h"
#include "DDSignal.hh"
#include <iostream>
#include <fstream>
#include <string>
#include <stdlib.h>
#include <vector>

using namespace std;

class SFData : public TObject{
  
private:
  int            fSeriesNo;		///< Series number
  int            fNpoints;		///< Number of measurements in the series
  TString        fFiber;		///< Scintillating fiber type e.g. LuAG (1)
  TString        fDesc;			///< Description of the measurement series
  TString        *fNames;		///< Array with names of measurements
  double         *fPositions;		///< Array with positions of radioactive source in mm
  TH1D           *fSpectrum;		///< Single requested spectrum
  TProfile       *fSignalProfile;	///< Average of n requested signals
  TH1D           *fSignal;		///< Histogram of single chosen signal
  vector <TH1D*> fSpectra;		///< Vector with all spectra from this series (requested type e.g. fPE)
  
  static TString fNames_1[9];
  static TString fNames_2[9];
  static TString fNames_3[9];
  static TString fNames_4[9];
  static TString fNames_5[9];
  static TString fNames_6[5];
  static TString fNames_7[1];
  static TString fNames_8[1];
  static TString fNames_9[2];
  
  static double fPositions_1[9];
  static double fPositions_2[9];
  static double fPositions_3[9];
  static double fPositions_4[9];
  static double fPositions_5[9];
  static double fPositions_6[5];
  static double fPositions_7[1];
  static double fPositions_8[1];
  static double fPositions_9[2];
  
  TString GetSelection(int ch, TString type);
  int     GetIndex(double position);
  bool    InterpretCut(DDSignal *sig, TString cut);
  
public:
  SFData();
  SFData(int seriesNo);
  ~SFData();
  
  bool           SetDetails(int seriesNo);
  TH1D*          GetSpectrum(int ch, TString type, TString cut, double position);
  vector <TH1D*> GetSpectra(int ch, TString type, TString cut);
  TProfile*      GetSignalAverage(int ch, double position, TString cut, int number, bool bl);
  TH1D*          GetSignal(int ch, double position, TString cut, int number, bool bl);  
  void           Reset(void);
  void           Print(void);
  
  int      GetNpoints(void){ return fNpoints; };
  TString  GetFiber(void){ return fFiber; };
  TString  GetDescription(void){ return fDesc; };
  TString* GetNames(void) { return fNames; };
  double*  GetPositions(void) { return fPositions; };
  
  ClassDef(SFData,1)
  
};

#endif