void run_DiReOne( TString mcEngine="TGeant3" )
{
  FairLogger *logger = FairLogger::GetLogger();
 // logger->SetLogFileName("MyLog.log");
 // logger->SetLogToScreen(kTRUE);
//  logger->SetLogToFile(kTRUE);
  logger->SetLogVerbosityLevel("LOW");
//  logger->SetLogFileLevel("DEBUG4");
//  logger->SetLogScreenLevel("DEBUG");
  
  TString workDir = gSystem->WorkingDirectory();

  // Verbosity level (0=quiet, 1=event level, 2=track level, 3=debug)
  Int_t iVerbose = 0; // just forget about it, for the moment
  
  // Input file (MC events)
  //  TString inFile = "data/testrun_";
  //  inFile = inFile + mcEngine + ".root";
  
  // Parameter file
  //  TString parFile = "data/testparams_"; 
  //  parFile = parFile + mcEngine + ".root";

  // Output file
  TString outFile = Form("data/testDiReOne_");
  outFile = outFile + mcEngine + ".root";
  
  // -----   Timer   --------------------------------------------------------
  TStopwatch timer;
  
  // -----   Reconstruction run   -------------------------------------------
  FairRunAna *fRun= new FairRunAna();
  fRun->SetInputFile(Form("file://%s/data/testrun_%s_f%d.root",workDir.Data(),mcEngine.Data(),0));

  fRun->SetOutputFile(outFile);
  
  FairRuntimeDb* rtdb = fRun->GetRuntimeDb();
  FairParRootFileIo* parInput1 = new FairParRootFileIo();

  TList* fnamelist = new TList();

  Int_t ifile = 0;
  TString parFile = Form("%s/data/testpar_%s_f%d.root",workDir.Data(),mcEngine.Data(),ifile);

  cout << "PAR LIST CREATED" << endl;
  //  parInput1->open(fnamelist);       
  parInput1->open(parFile.Data());

  rtdb->setFirstInput(parInput1);
  
  FairTestDetectorDigiTask* digiTask = new FairTestDetectorDigiTask();
  fRun->AddTask(digiTask);
  
  FairTestDetectorRecoTask* hitProducer = new FairTestDetectorRecoTask();
  fRun->AddTask(hitProducer);

  fRun->Init();

  timer.Start();
  fRun->Run();

  // -----   Finish   -------------------------------------------------------

  cout << endl << endl;

  // Extract the maximal used memory an add is as Dart measurement
  // This line is filtered by CTest and the value send to CDash
  FairSystemInfo sysInfo;
  Float_t maxMemory=sysInfo.GetMaxMemory();
  cout << "<DartMeasurement name=\"MaxMemory\" type=\"numeric/double\">";
  cout << maxMemory;
  cout << "</DartMeasurement>" << endl;

  timer.Stop();
  Double_t rtime = timer.RealTime();
  Double_t ctime = timer.CpuTime();

  Float_t cpuUsage=ctime/rtime;
  cout << "<DartMeasurement name=\"CpuLoad\" type=\"numeric/double\">";
  cout << cpuUsage;
  cout << "</DartMeasurement>" << endl;

  cout << endl << endl;
  cout << "Output file is "    << outFile << endl;
  cout << "Parameter file is " << parFile << endl;
  cout << "Real time " << rtime << " s, CPU time " << ctime
       << "s" << endl << endl;
  cout << "Macro finished successfully." << endl;

  // ------------------------------------------------------------------------
}
