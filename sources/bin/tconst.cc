// *****************************************
// *                                       *
// *          ScintillatingFibers          *
// *              tconst.cc                *
// *          Katarzyna Rusiecka           *
// * katarzyna.rusiecka@doctoral.uj.edu.pl *
// *          Created in 2018              *
// *                                       *
// ***************************************** 

#include "SFTimeConst.hh"
#include "SFFitResults.hh"
#include "TCanvas.h"
#include "TLatex.h"
#include "TLegend.h"
#include "CmdLineConfig.hh"
#include "CmdLineOption.hh"
#include <sys/stat.h>
#include <sys/types.h>

int main(int argc, char **argv){
 
  if(argc<2 || argc>6){
    std::cout << "to run type: ./tconst seriesNo";
    std::cout << "-out path/to/output -db database" << std::endl;
    return 0;
  }
  
  int seriesNo = atoi(argv[1]);
  
  SFData *data;
  try{
    data = new SFData(seriesNo);
  }
  catch(const char* message){
    std::cerr << message << std::endl;
    std::cout << "##### Exception in tconst.cc!" << std::endl;
    return 0;
  }
  
  TString desc = data->GetDescription();
  if(!desc.Contains("Regular series")){
    std::cerr << "##### Error in tconst.cc! This is not regular series!" << std::endl;
    std::cerr << "Series number: " << seriesNo << std::endl;
    std::cerr << "Description: " << desc << std::endl;
    return 0;
  }
  
  int npoints = data->GetNpoints();
  std::vector <double> positions = data->GetPositions();
  data->Print();
  
  SFTimeConst *tconst;
  try{
    tconst = new SFTimeConst(seriesNo, 400, false); 
  }
  catch(const char *message){
    std::cerr << message << std::endl;
    return 0;
  }
  
  tconst->Print();
  tconst->FitAllSignals();
  std::vector <TProfile*>     signalsCh0 = tconst->GetSignals(0);
  std::vector <SFFitResults*> resultsCh0 = tconst->GetResults(0);
  std::vector <TF1*> compFunCh0;
  int statCh0 = -1;
  
  std::vector <TProfile*>     signalsCh1 = tconst->GetSignals(1);
  std::vector <SFFitResults*> resultsCh1 = tconst->GetResults(1);
  std::vector <TF1*> compFunCh1;
  int statCh1 = -1;
  
  std::vector <double> decConst = tconst->GetAverageDecayConst();
  std::vector <double> intens = tconst->GetAverageIntensities();
  
  TCanvas *canCh0 = new TCanvas("canCh0", "canCh0", 1500, 1200);
  canCh0->DivideSquare(npoints);
  
  TCanvas *canCh1 = new TCanvas("canCh1", "canCh1", 1500, 1200);
  canCh1->DivideSquare(npoints);
  
  double ymin, ymax;
  bool ch0_ones=false; 
  bool ch1_ones=false; 

  for(int i=0; i<npoints; i++){
    canCh0->cd(i+1);
    gPad->SetGrid(1,1);
    signalsCh0[i]->Draw();
    signalsCh0[i]->SetStats(false);
    signalsCh0[i]->SetTitle(Form("Averaged signal, source position %.2f mm", positions[i]));
    signalsCh0[i]->GetXaxis()->SetTitle("time [ns]");
    signalsCh0[i]->GetYaxis()->SetTitle("amplitude [mV]");
    ymax = signalsCh0[i]->GetBinContent(signalsCh0[i]->GetMaximumBin());
    ymax = ymax+0.2*ymax;
    ymin = signalsCh0[i]->GetBinContent(signalsCh0[i]->GetMinimumBin());
    ymin = ymin-2;
    signalsCh0[i]->GetYaxis()->SetRangeUser(ymin, ymax);
    if(resultsCh0[i]->GetStat()==0){
      ch0_ones=true;
      resultsCh0[i]->GetFunction()->Draw("same");
      resultsCh0[i]->GetResultsPave()->Draw();
      compFunCh0 = resultsCh0[i]->GetCompFunctions();
      for(int ii=0; ii<compFunCh0.size(); ii++){
        compFunCh0[ii]->Draw("same");
      }
    }
    
    canCh1->cd(i+1);
    gPad->SetGrid(1,1);
    signalsCh1[i]->Draw();
    signalsCh1[i]->SetStats(false);
    signalsCh1[i]->SetTitle(Form("Averaged signal, source position %.2f mm", positions[i]));
    signalsCh1[i]->GetXaxis()->SetTitle("time [ns]");
    signalsCh1[i]->GetYaxis()->SetTitle("amplitude [mV]");
    ymax = signalsCh1[i]->GetBinContent(signalsCh1[i]->GetMaximumBin());
    ymax = ymax+0.2*ymax;
    ymin = signalsCh1[i]->GetBinContent(signalsCh1[i]->GetMinimumBin());
    ymin = ymin-2;
    signalsCh1[i]->GetYaxis()->SetRangeUser(ymin,ymax);
    if(resultsCh1[i]->GetStat()==0){
      ch1_ones=true;
      resultsCh1[i]->GetFunction()->Draw("same");
      resultsCh1[i]->GetResultsPave()->Draw();
      compFunCh1 = resultsCh1[i]->GetCompFunctions();
      for(int ii=0; ii<compFunCh1.size(); ii++){
        compFunCh1[ii]->Draw("same");
      }
    }
  }
  
  int ncomp = resultsCh0[0]->GetNcomponents();
  TLegend *legCh0 = new TLegend(0.633, 0.278, 0.893, 0.451);
  TLegend *legCh1 = new TLegend(0.633, 0.278, 0.893, 0.451);
  
  if(ch0_ones){
    legCh0->AddEntry(compFunCh0[0], "Base line", "L");
    if(ncomp==1){
      legCh0->AddEntry(compFunCh0[1], "Decay", "L");
    }  
    else if(ncomp==2){
      legCh0->AddEntry(compFunCh0[1], "Fast component", "L");
      legCh0->AddEntry(compFunCh0[2], "Slow component", "L");
      legCh0->AddEntry(resultsCh0[0]->GetFunction(), "Double decay", "L");
    }
  }
  if(ch1_ones){
    legCh1->AddEntry(compFunCh1[0],"Base line","L");
    if(ncomp==1){
      legCh1->AddEntry(compFunCh1[1],"Decay","L");
    }
    else if(ncomp==2){
      legCh1->AddEntry(compFunCh1[1],"Fast component","L");
      legCh1->AddEntry(compFunCh1[2],"Slow component","L");
      legCh1->AddEntry(resultsCh1[0]->GetFunction(), "Double decay", "L");
    }
  }
  
  canCh0->cd(1);
  legCh0->Draw();
  canCh1->cd(1);
  legCh1->Draw();
  
  //----- saving
  TString path = std::string(getenv("SFPATH"));
  TString fname = Form("tconst_series%i.root", seriesNo);

  CmdLineOption cmd_outdir("Output directory", "-out", "Output directory (string), default: $SFPATH/results", path+"results");
  
  CmdLineOption cmd_dbase("Data base", "-db", "Data base name (string), default: ScintFibRes.db", "ScintFibRes.db");
  
  CmdLineConfig::instance()->ReadCmdLine(argc, argv);
  
TString outdir = CmdLineOption::GetStringValue("Output directory");
  TString dbase = CmdLineOption::GetStringValue("Data base");
  
  if(!gSystem->ChangeDirectory(outdir)){
    std::cout << "Creating new directory... " << std::endl;
    std::cout << outdir << std::endl;
    int stat = mkdir(outdir, 0777);
    if(stat==-1){
      std::cerr << "##### Error in tconst.cc! Unable to create new direcotry!" << std::endl;
      return 0;
    }
  }
  
  TString fname_full = outdir + "/" + fname;
  TString dbname_full = outdir + "/" + dbase;
  
  TFile *file = new TFile(fname_full, "RECREATE");
  
  if(!file->IsOpen()){
    std::cerr << "##### Error in tconst.cc!" << std::endl;
    std::cerr << "Couldn't open file: " << fname_full << std::endl;
    return 0;
  }
  
  canCh0->Write();
  canCh1->Write();
  file->Close();
  
  //----- writing results to the data base
  TString table = "TIME_CONSTANTS";
  TString query = Form("INSERT OR REPLACE INTO %s (SERIES_ID, RESULTS_FILE, FAST_DEC, FAST_DEC_ERR, SLOW_DEC, SLOW_DEC_ERR, IFAST, ISLOW) VALUES (%i, '%s', %f, %f, %f, %f, %f, %f)", table.Data(), seriesNo, fname_full.Data(), decConst[0], decConst[1], decConst[2], decConst[3], intens[0], intens[1]);
  SFTools::SaveResultsDB(dbname_full, table, query, seriesNo);
  
  delete tconst;
  delete data;
  
  return 1;
}