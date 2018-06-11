// *****************************************
// *                                       *
// *          ScintillatingFibers          *
// *            SFTimingRes.cc             *
// *          Katarzyna Rusiecka           *
// * katarzyna.rusiecka@doctoral.uj.edu.pl *
// *          Created in 2018              *
// *                                       *
// *****************************************

#include "SFTimingRes.hh"

ClassImp(SFTimingRes);

//------------------------------------------------------------------
/// Default constructor.
SFTimingRes::SFTimingRes(){
  cout << "##### Warning in SFTimingRes constructor!" << endl;
  cout << "You are using default constructor!" <<endl;
  fSeriesNo  = -1;
  fThreshold = "dummy";
  fMethod    = "dummy";
  fData      = NULL;
}
//------------------------------------------------------------------
/// Standard constructor (recommended)
/// \param seriesNo is number of experimental series to be analyzed.
/// \param threshold identifies which tree should be opened. Possible
/// options are: ft - fixed threshold and cf - constant fraction.
/// \param method is a flag to choose type of analysis. Possible options are:
/// with cut - for analysis with energy cut on 511 keV, no cut - cutting only
/// scattered events.
SFTimingRes::SFTimingRes(int seriesNo, TString threshold, TString method){
  if(!(threshold=="ft" || threshold=="cf")){
    throw "##### Error in SFTimingRes constructor! Incorrect threshold type!\nPossible options are: ft, cf";
  }
  if(!(method=="with cut" || method=="no cut")){
    throw "##### Error in SFTimingRes constructor! Incorrect method!\nPossible options are: with cut, no cut";
  }
  fSeriesNo = seriesNo;
  fThreshold = threshold;
  fMethod = method;
  fData = new SFData(fSeriesNo,threshold);
  if(fMethod=="no cut") AnalyzeNoECut();
  else if(fMethod=="with cut") AnalyzeWithECut();
}
//------------------------------------------------------------------
/// Default destructor.
SFTimingRes::~SFTimingRes(){
 if(fData!=NULL) delete fData; 
}
//------------------------------------------------------------------
///Private method to get index of requested measurement based on source position.
int SFTimingRes::GetIndex(double position){
  int index = -1;
  double *positions = fData->GetPositions();
  int npoints = fData->GetNpoints();
  if(fSeriesNo>5){
    index = position-1;
    return index;
  }
  for(int i=0; i<npoints; i++){
    if(fabs(positions[i]-position)<1){
      index = i;
      break;
    }
  }
  if(index==-1){
   cout << "##### Error in SFTimingRes::GetIndex()! Incorrecct position!" << endl;
   return index;
  }
  return index;
}
//------------------------------------------------------------------
///Private method to get ratio histograms necesarry to impose cuts.
bool SFTimingRes::LoadRatios(void){
  
  int npoints = fData->GetNpoints();
  
  TString selection = "log(sqrt(ch_1.fPE/ch_0.fPE))";
  TString cut = "ch_0.fT0<590 && ch_0.fPE>0 && ch_0.fT0>0 && ch_1.fT0<590 && ch_1.fT0>0 && ch_1.fPE>0"; 
  fRatios = fData->GetCustomHistograms(selection,cut);
  
  TF1 *gaus = new TF1("gaus","gaus",-100,100);
  double min, max;
  
  for(int i=0; i<npoints; i++){
    min = fRatios[i]->GetMean() - fRatios[i]->GetRMS();
    max = fRatios[i]->GetMean() + fRatios[i]->GetRMS();
    fRatios[i]->Fit(gaus,"Q","",min,max);
  }
 
  return true;
}
//------------------------------------------------------------------
///Formula for Lorentzian function
double LorentzianFun(double *x, double *par){
 //par0 - integral
 //par1 - width
 //par2 - center  
 return (0.5*par[0]*par[1]/TMath::Pi())/
        ((x[0]-par[2])*(x[0]-par[2])+.25*par[1]*par[1]);
}
//------------------------------------------------------------------
///Method for simple timing resolution determination. Timing resolution spectrum
///is build as difference between channel 0 and 1 signals with additional cut
///on scattered events. Timing resolution and its uncertainty is determined as 
///histogram's mean and RMS. 
bool SFTimingRes::AnalyzeNoECut(void){
  
  int npoints = fData->GetNpoints();
  double *positions = fData->GetPositions();
  
  TString selection = "ch_0.fT0-ch_1.fT0";
  TString cut;
  double mean, sigma;
  double min_fit, max_fit;
  
  if(fRatios.empty()) LoadRatios();
  TF1 *lorentz = new TF1("lorentz",LorentzianFun,-100,100,3);
  
  fT0Graph = new TGraphErrors(npoints);
  fT0Graph->GetXaxis()->SetTitle("source position [mm]");
  fT0Graph->GetYaxis()->SetTitle("mean of time difference distribution [ns]");
  fT0Graph->SetTitle("");
  fT0Graph->SetMarkerStyle(8);
  
  for(int i=0; i<npoints; i++){
    mean = fRatios[i]->GetFunction("gaus")->GetParameter(1);
    sigma = fRatios[i]->GetFunction("gaus")->GetParameter(2);
    cut = Form("ch_0.fT0>0 && ch_1.fT0>0 && log(sqrt(ch_1.fPE/ch_0.fPE))>%f && log(sqrt(ch_1.fPE/ch_0.fPE))<%f",mean-sigma, mean+sigma);
    fT0Diff.push_back(fData->GetCustomHistogram(selection,cut,positions[i]));
    lorentz->SetParameter(0,fT0Diff[i]->Integral());
    lorentz->SetParameter(1,fT0Diff[i]->GetRMS());
    lorentz->SetParameter(2,fT0Diff[i]->GetMean());
    min_fit = fT0Diff[i]->GetMean()-3*fT0Diff[i]->GetRMS();
    max_fit = fT0Diff[i]->GetMean()+3*fT0Diff[i]->GetRMS();
    fT0Diff[i]->Fit(lorentz,"Q","",min_fit,max_fit);
    fTimeRes.push_back(lorentz->GetParameter(1));
    fTimeResErr.push_back(lorentz->GetParError(1));
    fT0Graph->SetPoint(i,positions[i],lorentz->GetParameter(2));
    fT0Graph->SetPointError(i,0,lorentz->GetParameter(1));
  }
  
  return true;
}
//------------------------------------------------------------------
///Method for timing resolution determination with an energy cut on 511 keV peak.
bool SFTimingRes::AnalyzeWithECut(void){
  
  int npoints = fData->GetNpoints();
  double *positions = fData->GetPositions();
  
  if(fRatios.empty()) LoadRatios();
  
  fPEch0 = fData->GetSpectra(0,"fPE","ch_0.fT0>0 && ch_0.fT0<590 && ch_0.fPE>0");
  fPEch1 = fData->GetSpectra(1,"fPE","ch_1.fT0>0 && ch_1.fT0<590 && ch_1.fPE>0");
  vector <SFPeakFinder*> peakFin_ch0;
  vector <SFPeakFinder*> peakFin_ch1;
  TF1* fun = new TF1("fun","gaus",-200,200);
  double xmin_ch0, xmax_ch0;
  double xmin_ch1, xmax_ch1;
  double mean_ratio, sigma_ratio;
  double mean, sigma;
  TString selection = "ch_0.fT0-ch_1.fT0";
  TString cut;
  
  double min0, max0;
  double min1, max1;
  
  fT0Graph = new TGraphErrors(npoints);
  fT0Graph->GetXaxis()->SetTitle("source position [mm]");
  fT0Graph->GetYaxis()->SetTitle("mean of time difference distribution [ns]");
  fT0Graph->SetTitle("");
  fT0Graph->SetMarkerStyle(8);
  
  for(int i=0; i<npoints; i++){
    peakFin_ch0.push_back(new SFPeakFinder(fPEch0[i],"511",false));
    peakFin_ch1.push_back(new SFPeakFinder(fPEch1[i],"511",false));
    peakFin_ch0[i]->FindPeakRange(xmin_ch0,xmax_ch0);
    peakFin_ch1[i]->FindPeakRange(xmin_ch1,xmax_ch1);
    mean_ratio = fRatios[i]->GetFunction("gaus")->GetParameter(1);
    sigma_ratio = fRatios[i]->GetFunction("gaus")->GetParameter(2);
    cut = Form("ch_0.fT0>0 && ch_1.fT0>0 && ch_0.fPE>%f && ch_0.fPE<%f && ch_1.fPE>%f && ch_1.fPE<%f && log(sqrt(ch_1.fPE/ch_0.fPE))>%f && log(sqrt(ch_1.fPE/ch_0.fPE))<%f", xmin_ch0,xmax_ch0,xmin_ch1,xmax_ch1,mean_ratio-sigma_ratio, mean_ratio+sigma_ratio);
    fT0Diff.push_back(fData->GetCustomHistogram(selection,cut,positions[i]));
    mean = fT0Diff[i]->GetMean();
    sigma = fT0Diff[i]->GetRMS();
    fT0Diff[i]->Fit(fun,"Q","",mean-3*sigma,mean+3*sigma);
    fTimeRes.push_back(fun->GetParameter(2));
    fTimeResErr.push_back(fun->GetParError(2));
    fT0Graph->SetPoint(i,positions[i],fT0Diff[i]->GetMean());
    fT0Graph->SetPointError(i,0,fT0Diff[i]->GetRMS());
  }
  
  return true;
}
//------------------------------------------------------------------
///Returns vector of histograms, which contains timing resolution spectra
///for whole series.
vector <TH1D*> SFTimingRes::GetT0Diff(void){
  if(fT0Diff.empty()){
   cout << "##### Error in SFTimingRes::GetT0Diff()! Empty vector!" << endl; 
  }
  return fT0Diff;
}
//------------------------------------------------------------------
///Returns single timing resolution spectrum for requested measurement.
///Measurement is identified by source position. If source position is not unique
///number of measurement in the series should be given.
TH1D* SFTimingRes::GetT0Diff(double position){
  int index = GetIndex(position);
  if(index>fT0Diff.size()){
    cout << "##### Error in SFTimingRes::GetT0Diff(position)! Incorrect position!" << endl;
    return NULL;
  }
  return fT0Diff[index];
}
//------------------------------------------------------------------
///Returns graph ch_0.T0-ch_1.T0 vs. source position.
TGraphErrors* SFTimingRes::GetT0Graph(void){
  if(fT0Graph==NULL){
    cout << "##### Error in SFTimingRes::GetT0Graph()! Graph was not created!" << endl;
    return NULL;
  }
  return fT0Graph;
}
//------------------------------------------------------------------
///Returns timing resolution value and its uncertainty for requested measurement.
///Measurement is identified like in GetT0Diff() function. Order in the returned vector:
///timing resolution, uncertainty.
vector <double> SFTimingRes::GetTimingResolution(double position){
  
  vector <double> temp;
  int index = GetIndex(position);
  
  if(index<fTimeRes.size()){
    cout << "##### Error in SFTimingRes::GetTimingResolution(position)! Incorrect position!" << endl;
  }
  
  temp.push_back(fTimeRes[index]);
  temp.push_back(fTimeResErr[index]);
  
  return temp;
}
//------------------------------------------------------------------
///Returns vector containing timing resolutions for all measurements 
///in the series
vector <double> SFTimingRes::GetTimingResolutions(void){
  if(fTimeRes.empty()){
    cout << "##### Error in SFTimingRes::GetTimingResolutions()! Empty vector!" << endl;
  }
  return fTimeRes;
}
//------------------------------------------------------------------
///Returns vector containing uncertainties of timing resolutions for 
///all measurements in the series.
vector <double> SFTimingRes::GetTimingResErrors(void){
  if(fTimeResErr.empty()){
    cout << "##### Error in SFTimingRes::GetTimingResErrors()! Empty vector!" << endl;
  }
  return fTimeResErr;
}
//------------------------------------------------------------------
/// Returns vector of ratio histograms used for cut on scattered events.
vector <TH1D*> SFTimingRes::GetRatios(void){
 if(fRatios.empty()){
   cout << "##### Error in SFTimingRes::GetRatios()! Empty vector!" << endl;
 }
 return fRatios; 
}
//------------------------------------------------------------------
/// Returns vector of PE spectra used for 511 keV energy cut. 
/// \param ch - channel number (0 or 1).
vector <TH1D*> SFTimingRes::GetSpectra(int ch){
 if(fMethod=="no cut"){
   cout << "##### Error in SFTimingRes::GetSpectra()! Incorrect method!" << endl;
 }
 if((ch==0 && fPEch0.empty()) || (ch==1 && fPEch1.empty())){
   cout << "##### Error in SFTimingRes::GetSpectra()! Empty vector!" << endl;
 }
 if(ch==0)       return fPEch0; 
 else if (ch==1) return fPEch1;
}
//------------------------------------------------------------------
/// Prints details of SFTimingRes class object.
void SFTimingRes::Print(void){
  cout << "\n--------------------------------------------" << endl;
  cout << "This is print out of SFTimingRes class object" << endl;
  cout << "Experimental series number " << fSeriesNo << endl;
  cout << "Threshold type: " << fThreshold << endl;
  cout << "Analysis method: " << fMethod << endl;
  cout << "--------------------------------------------\n" << endl;
}
//------------------------------------------------------------------