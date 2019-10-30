// *****************************************
// *                                       *
// *          ScintillatingFibers          *
// *              SFTools.cc               *
// *          Katarzyna Rusiecka           *
// * katarzyna.rusiecka@doctoral.uj.edu.pl *
// *          Created in 2019              *
// *                                       *
// *****************************************

#include "SFTools.hh"

ClassImp(SFTools);

//------------------------------------------------------------------
/// Returns index in the fNames and fPositions arrays for the 
/// measurement of requested source position in mm. 
/// If measurements in analyzed series don't have unique positions 
/// a number of measurement should be passed. Measurements counting 
/// starts at 1.
/// \param positions - vector containing all source position in the 
/// measurement series
/// \param position - source position of the requested measurement
int SFTools::GetIndex(std::vector <double> positions, double position){
 
    
    int index = -1;
    int npoints = positions.size();
    
    if(fabs(positions[0]-positions[1])<1E-10){
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
      std::cerr << "##### Error in SFTools::GetIndex()! Incorrect position!" << std::endl;
      std::abort();
    }
    
    return index;
}
//------------------------------------------------------------------
int SFTools::GetSeriesNo(TString hname_tstr){

    std::string hname = std::string(hname_tstr);
    int nletters = hname.length();
    char letters[nletters];
    strcpy(letters,hname.c_str());
    
    int iposition = -1;
    for(int i=0; i<nletters; i++){
      if(letters[i]=='_'){
          iposition = i;
          break;
      }
    }
    
    if(iposition==-1){
      std::cerr << "##### Error in SFTools::GetSeriesNo()!" << std::endl;
      std::cerr << "Cannot interpret spectrum name!" << std::endl;
      std::abort();
    }
    
    TString seriesName = std::string(&letters[1], &letters[iposition]);
    int seriesNo = atoi(seriesName);
    
    return seriesNo;
} 
//------------------------------------------------------------------
int SFTools::GetChannel(TString hname_tstr){
    
    std::string hname = std::string(hname_tstr);
    int nletters = hname.length();
    char letters[nletters];
    strcpy(letters,hname.c_str());
    
    int iposition = -1;
    for(int i=0; i<nletters; i++){
      if(letters[i]=='h'){
          iposition = i;
          break;
      }
    }
    
    if(iposition==-1){
      std::cerr << "##### Error in SFTools::GetChannel()!" << std::endl;
      std::cerr << "Cannot interpret spectrum name!" << std::endl;
      std::abort();
    }
    
    TString channelName = std::string(&letters[iposition+1], &letters[iposition+2]);
    int channelNo = atoi(channelName);
    
    return channelNo;
}
//------------------------------------------------------------------
double SFTools::GetPosition(TString hname_tstr){
    
    std::string hname = std::string(hname_tstr);
    int nletters = hname.length();
    char letters[nletters];
    strcpy(letters,hname.c_str());
    
    int iposition = -1;
    for(int i=0; i<nletters; i++){
      if(letters[i]=='p'){
          iposition = i;
          break;
      }
    }
    
    if(iposition==-1){
      std::cerr << "##### Error in SFTools::GetPosition()!" << std::endl;
      std::cerr << "Cannot interpret spectrum name!" << std::endl;
      std::abort();
    }
    
    TString posName = std::string(&letters[iposition+4], &letters[iposition+7]);
    int pos = atof(posName);
    
    return pos;
}
//------------------------------------------------------------------
double SFTools::GetPosError(TString collimator, TString testBench){
 
  double err = -1;
  
  if(collimator.Contains("Lead"))
    err = 2.0;   // mm
  else if(collimator.Contains("Electronic") && testBench.Contains("DE"))
    err = 1.5;   // mm
  else if(collimator.Contains("Electronic") && testBench.Contains("PL"))
    err = 1.0;   // mm

  if(fabs(err+1)<1E-10){
    std::cerr << "##### Error in SFTools::GetPosError()!" << std::endl;
    std::cerr << "Incorrect position error value!" << std::endl;
    std::abort();
  }
    
  return err;
}
//------------------------------------------------------------------
void SFTools::CheckDBStatus(int status, sqlite3 *database){
  
  if(!((status==SQLITE_OK) || (status==SQLITE_DONE))){
    std::cerr << "##### SQL Error in SFTools::CheckDBStatus: " << sqlite3_errmsg(database) << std::endl;  
    std::abort();
  }
    
  return;
}
//------------------------------------------------------------------
bool SFTools::SaveResultsDB(TString database, TString table, 
                            TString query, int seriesNo){
 
  std::cout << "----- Saving results in the databse: " << database << std::endl;
  std::cout << "----- Accessing table: " << table << std::endl;
  
  sqlite3 *resultsDB;
  int status = -1;
  sqlite3_stmt *statement;
  time_t now = time(nullptr);
  ctime(&now);
  
  //--- opening data base
  status = sqlite3_open(database, &resultsDB);
  CheckDBStatus(status, resultsDB);
  
  //--- checking whether table exists
  TString table_query = Form("SELECT count(*) FROM sqlite_master WHERE type='table' AND name='%s'", table.Data());
  status = sqlite3_prepare_v2(resultsDB, table_query, -1, &statement, nullptr);
  CheckDBStatus(status, resultsDB);
 
  int table_stat = -1;
  
  while((sqlite3_step(statement) == SQLITE_ROW)){
    table_stat = sqlite3_column_int(statement, 0);
  }
  
  sqlite3_finalize(statement);
  
  if(table_stat==0)
    CreateTable(database, table);

  //--- executing given query
  status = sqlite3_prepare_v2(resultsDB, query, -1, &statement, nullptr);
  CheckDBStatus(status, resultsDB);
  status = sqlite3_step(statement);
  CheckDBStatus(status, resultsDB);
  sqlite3_finalize(statement);
  
  //--- adding time
  TString time_query = Form("UPDATE %s SET DATE = %lld WHERE SERIES_ID = %i", table.Data(), (long long) now, seriesNo);
  status = sqlite3_prepare_v2(resultsDB, time_query, -1, &statement, nullptr);
  CheckDBStatus(status, resultsDB);
  status = sqlite3_step(statement);
  CheckDBStatus(status, resultsDB);
  sqlite3_finalize(statement);
  
  //--- closing data base
  status = sqlite3_close_v2(resultsDB);
  CheckDBStatus(status, resultsDB);
  
  return true;
}
//------------------------------------------------------------------
bool SFTools::CreateTable(TString database, TString table){
    
  std::cout << "----- Creating new table: " << table << std::endl;
  std::cout << "----- Data base: " << database << std::endl;
   
  sqlite3 *resultsDB;
  int status = -1;
  sqlite3_stmt *statement;
  TString query;
  
  if(table == "DATA"){
    query = "CREATE TABLE 'DATA' ('SERIES_ID' INTEGER PLRIMARY_KEY, 'RESULTS_FILE' TEXT, 'DATE' INTIGER, PRIMARY KEY ('SERIES_ID'))";
  }
  else if(table == "ATTENUATION_LENGTH"){
    query = "CREATE TABLE 'ATTENUATION_LENGTH' ('SERIES_ID' INTEGER PRIMARY_KEY, 'RESULTS_FILE' TEXT, 'ATT_CH0' NUMERIC, 'ATT_CH0_ERR' NUMERIC, 'ATT_CH1' NUMERIC, 'ATT_CH1_ERR' NUMERIC, 'ATT_COMB' NUMERIC, 'ATT_COMB_ERR' NUMERIC, 'DATE' INTIGER, PRIMARY KEY ('SERIES_ID'))";
  }
  else if(table == "ENERGY_RESOLUTION"){
    query = "CREATE TABLE 'ENERGY_RESOLUTION' ('SERIES_ID' INTIGER PRIMARY_KEY, 'RESULTS_FILE' TEXT, 'ENRES_SUM' NUMERIC, 'ENRES_SUM_ERR' NUMERIC, 'ENRES_CH0' NUMERIC, 'ENRES_CH0_ERR' NUMERIC, 'ENRES_CH1' NUMERIC, 'ENRES_CH1_ERR' NUMERIC, 'DATE' INTEGER, PRIMARY KEY ('SERIES_ID'))";
  }
  else if(table == "LIGHT_OUTPUT"){
    query = "CREATE TABLE 'LIGHT_OUTPUT' ('SERIES_ID' INTEGER PRIMARY_KEY, 'RESULTS_FILE' TEXT, 'LOUT' NUMERIC, 'LOUT_ERR' NUMERIC, 'LOUT_CH0' NUMERIC, 'LOUT_CH0_ERR' NUMERIC, 'LOUT_CH1' NUMERIC, 'LOUT_CH1_ERR' NUMERIC, 'LCOL' NUMERIC, 'LCOL_ERR' NUMERIC, 'LCOL_CH0' NUMERIC, 'LCOL_CH0_ERR' NUMERIC, 'LCOL_CH1' NUMERIC, 'LCOL_CH1_ERR' NUMERIC, 'DATE' INTIGER, PRIMARY KEY ('SERIES_ID'))";
  }
  else if(table == "TIMING_RESOLUTION"){
    query = "CREATE TABLE 'TIMING_RESOLUTION' ('SERIES_ID' INTEGER PRIMARY_KEY, 'RESULTS_FILE' TEXT, 'TIMERES' NUMERIC, 'TIMERES_ERR' NUMERIC, 'TIMERES_ECUT' NUMERIC, 'TIMERES_ECUT_ERR' NUMERIC, 'DATE' INTIGER, PRIMARY KEY ('SERIES_ID'))";
  }
  else if(table == "TIME_CONSTANTS"){
    query = "CREATE TABLE 'TIME_CONSTANTS' ('SERIES_ID' INTEGER PRIMARY_KEY, 'RESULTS_FILE' TEXT, 'FAST_DEC' NUMERIC, 'FAST_DEC_ERR' NUMERIC, 'SLOW_DEC' NUMERIC, 'SLOW_DEC_ERR' NUMERIC, 'IFAST' NUMERIC, 'ISLOW' NUMERIC, 'DATE' INTIGER, PRIMARY KEY ('SERIES_ID'))";
  }
  else if(table == "TEMPERATURE"){
    query = "CREATE TABLE 'TEMPERATURE' ('SERIES_ID' INTEGER PRIMARY_KEY, 'RESULTS_FILE' TEXT, 'TEMP_446D45' NUMERIC, 'TEMP_ERR_446D45' NUMERIC, 'TEMP_044F45' NUMERIC, 'TEMP_ERR_044F45' NUMERIC, 'TEMP_2BAD44' NUMERIC, 'TEMP_ERR_2BAD44' NUMERIC, 'TEMP_8F1F46' NUMERIC, 'TEMP_ERR_8F1F46' NUMERIC, 'DATE' INTIGER, PRIMARY KEY ('SERIES_ID'))";
  }
  else if(table == "POSITION_RESOLUTION"){
    query = "CREATE TABLE 'POSITION_RESOLUTION' ('SERIES_ID' INTEGER PRIMARY_KEY, 'RESULTS_FILE' TEXT, 'POSITION_RES_MLR' NUMERIC, 'POSITION_RES_MLR_ERR' NUMERIC, 'POSITION_RES_Y' NUMERIC, 'POSITION_RES_Y_ERR' NUMERIC, 'DATE' INTIGER, PRIMARY KEY ('SERIES_ID'))"; 
  }
  else if(table == "PEAK_FINDER"){
    query = "CREATE TABLE 'PEAK_FINDER' ('SERIES_ID' INTEGER PRIMARY_KEY, 'RESULTS_FILE' TEXT, 'DATE' INTIGER, PRIMARY KEY ('SERIES_ID'))";
  }
  else{
    std::cerr << "##### Error in SFTools::CreateTable()!" << std::endl;
    std::cerr << "Unknown table!" << std::endl;
    std::abort();
  }
  
  //--- opening data base
  status = sqlite3_open(database, &resultsDB);
  CheckDBStatus(status, resultsDB);
  
  //--- creating table
  status = sqlite3_prepare_v2(resultsDB, query, -1, &statement, nullptr);
  CheckDBStatus(status, resultsDB);
  status = sqlite3_step(statement);
  CheckDBStatus(status, resultsDB);
  sqlite3_finalize(statement);
  
  //--- closing data base
  status = sqlite3_close_v2(resultsDB);
  CheckDBStatus(status, resultsDB);
  
  return true;
}
//------------------------------------------------------------------
double SFTools::GetMean(std::vector <double> vec){
  
  int size = vec.size();
  double sum = 0;
  
  for(int i=0; i<size; i++){
    sum += vec[i];   
  }
  
  double mean = sum/size;
  
  return mean;
}
//------------------------------------------------------------------
double SFTools::GetStandardErr(std::vector <double> vec){
  
  int size = vec.size();
  double mean = GetMean(vec);  
  
  double sumSquares = 0;
    
  for(int i=0; i<size; i++){
    sumSquares += pow((vec[i]-mean),2);  
  }
  
  double stdDev = sqrt(sumSquares/size);
  double stdErr = stdDev/sqrt(size);  
    
  return stdErr;
}
//------------------------------------------------------------------
TString SFTools::FindData(TString directory){

  TString path_1 = std::string(getenv("SFDATA")) + directory + "/results.root";
  TFile *file_1 = new TFile(path_1, "READ");
  
  if(file_1->IsOpen()){
    file_1->Close();
    return path_1;
  }
  
  TString path_2 = "/scratch/gccb/kasia/data/" + directory + "/results.root";
  TFile *file_2 = new TFile(path_2, "READ");
    
  if(file_2->IsOpen()){
    file_2->Close();
    return path_2;
  }
  
  std::cerr << "##### Error in SFTools::FindData()! Requested file doesn't exist!" << std::endl;
  std::abort();
}
//------------------------------------------------------------------
