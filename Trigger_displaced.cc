#include "Trigger_displaced.h"

Trigger::Trigger(bool isMuon, bool isElectron, bool trigSingleLep, bool trigDoubleLep):
muon(false), electron(false), singleLep(false), doubleLep(false), fullHadr(false), trigged(false), redotrigmap(false), triggerList(), currentRunTrig(0), previousRunTrig(-1), currentFilenameTrig(""), previousFilenameTrig(""), iFileTrig(-1), treeNumberTrig(-1), triggermap()
{
  if (isMuon)
  {
    muon = true;
  }
  if (isElectron)
  {
    electron = true;
  }
  
  if (trigSingleLep)
  {
    singleLep = true;
  }
  if (trigDoubleLep)
  {
    doubleLep = true;
  }
  if (! trigSingleLep && ! trigDoubleLep)
  {
    fullHadr = true;
    cout << "TRIGGER::TRIGGER - Do you really want no lepton triggers?" << endl;
  }
}

Trigger::~Trigger()
{
  
}

void Trigger::bookTriggers(bool isData)
{
  /// This function is called in the dataset loop
  //  Reset all quantities here
  triggerList.clear();
  currentRunTrig = 0;
  previousRunTrig = -1;
  currentFilenameTrig = previousFilenameTrig = "";
  iFileTrig = -1;
  treeNumberTrig = -1;
  triggermap.clear();
  
  /// Add relevant triggers to triggerlist
  //  Recommended triggers for TOP analyses
  //  Last updated: 16 Nov 2016. https://twiki.cern.ch/twiki/bin/viewauth/CMS/TopTrigger
  if (singleLep)
  {
    if (muon)   // Triggers for data & MC are the same
    {
      if(isData)triggerList.push_back("HLT_Iso(Tk)Mu24_v*");	//2016
      else triggerList.push_back("HLT_Iso(Tk)Mu24_v2");		//2016
      //triggerList.push_back("HLT_Mu28NoFiltersNoVtx_CentralCaloJet40_v*");	//2015
      //      triggerList.push_back("HLT_IsoMu20_v*");			//2015
      //      triggerList.push_back("HLT_IsoTkMu20_v*");		//2015
    }
    if (electron)
    {
      if (isData)
      {
	triggerList.push_back("HLT_Ele32_eta2p1_WPTight_Gsf_v*");	//2016
	//triggerList.push_back("HLT_Ele27_WPLoose_Gsf_v*");	//2015
      }
      else
      {
	triggerList.push_back("HLT_Ele32_eta2p1_WPTight_Gsf_v3");	//2016
	//triggerList.push_back("HLT_Ele27_WPLoose_Gsf_v*");	//2015
      }   	
    }
  }
  
  if (doubleLep)   // Triggers for data & MC are the same
  {
    if (muon && ! electron) // double muons
    {
      if (isData) triggerList.push_back("HLT_Mu17_TrkIsoVVL_Mu8_TrkIsoVVL_DZ_v*");     //2016
      else triggerList.push_back("HLT_Mu17_TrkIsoVVL_Mu8_TrkIsoVVL_DZ_v3");    //2016
      //triggerList.push_back("HLT_DoubleMu23NoFiltersNoVtxDisplaced_v*"); //dxz > 0.01 (2015)
      //triggerList.push_back("HLT_DoubleMu33NoFiltersNoVtx_v*");	   //2015
    }
    if (! muon && electron) // double electrons
    {
      if (isData) triggerList.push_back("HLT_Ele23_Ele12_CaloIdL_TrackIdL_IsoVL_DZ_v*"); //2016
      else triggerList.push_back("HLT_Ele23_Ele12_CaloIdL_TrackIdL_IsoVL_DZ_v4"); //2015
      //triggerList.push_back("HLT_Photon36_R9Id85_OR_CaloId24b40e_Iso50T80L_Photon22_AND_HE10_R9Id65_Eta2_Mass15_v*"); //2015
    }
    if (muon && electron) // muon and electron
    {
      if (isData) {
	triggerList.push_back("HLT_Mu23_TrkIsoVVL_Ele12_CaloIdL_TrackIdL_IsoVL_v*"); 	//2016
	triggerList.push_back("HLT_Mu12_TrkIsoVVL_Ele23_CaloIdL_TrackIdL_IsoVL_v*"); 	//2016
	triggerList.push_back("HLT_Mu23_TrkIsoVVL_Ele12_CaloIdL_TrackIdL_IsoVL_DZ_v*"); 	//2016
	triggerList.push_back("HLT_Mu12_TrkIsoVVL_Ele23_CaloIdL_TrackIdL_IsoVL_DZ_v*"); 	//2016
      }
      else {
	triggerList.push_back("HLT_Mu23_TrkIsoVVL_Ele12_CaloIdL_TrackIdL_IsoVL_v4"); 	//2016
	triggerList.push_back("HLT_Mu8_TrkIsoVVL_Ele23_CaloIdL_TrackIdL_IsoVL_v4"); 	//2016
      }
      //triggerList.push_back("HLT_Mu38NoFiltersNoVtx_Photon38_CaloIdL_v*");	//2015
    }
  }
  
  if (fullHadr)
  {
    if (isData)
    {
      triggerList.push_back("HLT_PFMET170_v*");
      triggerList.push_back("HLT_PFMET120_BTagCSV0p72_v*");
      triggerList.push_back("HLT_PFMET120_NoiseCleaned_BTagCSV0p72_v*");
      triggerList.push_back("HLT_CaloMHTNoPU90_PFMET90_NoiseCleaned_PFMHT90_IDTight_v*");
    }
    else
    {
      triggerList.push_back("HLT_PFMET170_v*");
      triggerList.push_back("HLT_PFMET120_BTagCSV0p72_v*");
      triggerList.push_back("HLT_PFMET120_NoiseCleaned_BTagCSV0p72_v*");
      triggerList.push_back("HLT_CaloMHTNoPU90_PFMET90_NoiseCleaned_PFMHT90_IDTight_v*");
    }
  }
  
  for(UInt_t iTrig = 0; iTrig < triggerList.size(); iTrig++)
  {
    triggermap[triggerList[iTrig]] = std::pair<int,bool> (-999,false);
  }
  
}

void Trigger::checkAvail(int currentRunTrig, vector < Dataset* > datasets, unsigned int d, TTreeLoader *treeLoader, TRootEvent* event, bool verbose)
{
  redotrigmap = false;
  treeNumberTrig = datasets[d]->eventTree()->GetTreeNumber();
  currentFilenameTrig = datasets[d]->eventTree()->GetFile()->GetName();
  if (previousFilenameTrig != currentFilenameTrig)
  {
    previousFilenameTrig = currentFilenameTrig;
    iFileTrig++;
    redotrigmap = true;
    cout << endl << "*****File changed!!! => iFile = " << iFileTrig << " new file is " << currentFilenameTrig << " in sample " << datasets[d]->Name() << " *****" << endl;
  }
  if (previousRunTrig != currentRunTrig)
  {
    previousRunTrig = currentRunTrig;
    cout << "*****Run changed!!! => new run = " << previousRunTrig << " *****" << endl;
    redotrigmap=true;
  }
  
  if (verbose && redotrigmap)
  {
    treeLoader->ListTriggers(currentRunTrig, treeNumberTrig);
  }
  
  
  // get trigger info:
  for(std::map<std::string,std::pair<int,bool> >::iterator iter = triggermap.begin(); iter != triggermap.end(); iter++)
  {
    if (redotrigmap)
    {
      Int_t loc = treeLoader->iTrigger(iter->first, currentRunTrig, treeNumberTrig);
      string trigname = iter->first;
      cout << "trigname: " << trigname << "  location: " << loc << /*"  " << currentFilenameTrig << "  " << currentRunTrig <<*/ endl;
      
      iter->second.first = loc;
    }
    // and check if it exists and if it fired:
    if (iter->second.first >= 0 && iter->second.first != 9999) // trigger exists
      iter->second.second = event->trigHLT(iter->second.first);
    else
      iter->second.second = false;
    #ifdef NOTRIGMC
     #warning "WARNING: Code for MC without triggers switched on!!!"
     if (iter->second.first >= 0 && iter->second.first == 9999) // For MC samples without triggers
         iter->second.second = true;                            // Force trigger to be fired 
    #endif
     
  }   
}


int Trigger::checkIfFired()
{
  // now check if the appropriate triggers fired for each analysis:
  trigged = 0;
  
  for(UInt_t itrig = 0; itrig < triggerList.size() && trigged == 0; itrig++)
  {
    if (triggermap[triggerList[itrig]].second)
      trigged = 1;
  }
  
  return trigged;
}
