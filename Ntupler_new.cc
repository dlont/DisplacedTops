/*|----------------------------------------------------------------------------------------------------------------------|
  |   Modified Ntuple.cc from Freya intended to be an example analysis macro for new users of the TopBrussels framework  |
  |----------------------------------------------------------------------------------------------------------------------|*/

// ROOT includes
#include "TStyle.h"
#include <cmath>
//Standard C++ library includes (both classes declared within the std namespace)
#include <fstream> //Stream class to both read and write from/to files.
#include <sstream> //Stream class to both read and write from/to strings.
#include <sys/stat.h>

// Include header files to make the interface (user code) available
#include "TopTreeProducer/interface/TRootRun.h"
#include "../TopTreeProducer/interface/TRootEvent.h"
#include "../TopTreeAnalysisBase/Selection/interface/SelectionTable.h"
#include "../TopTreeAnalysisBase/Selection/interface/DisplacedSelection.h"
#include "TopTreeProducer/interface/TRootPFJet.h"
#include "../TopTreeAnalysisBase/Tools/interface/PlottingTools.h"
#include "../TopTreeAnalysisBase/Tools/interface/MultiSamplePlot.h"
#include "../TopTreeAnalysisBase/Tools/interface/TTreeLoader.h"
#include "../TopTreeAnalysisBase/Tools/interface/AnalysisEnvironmentLoader.h"
#include "../TopTreeAnalysisBase/Content/interface/AnalysisEnvironment.h"
#include "../TopTreeAnalysisBase/Content/interface/Dataset.h"
#include "../TopTreeAnalysisBase/MCInformation/interface/MCWeighter.h"
#include "../TopTreeAnalysisBase/Selection/interface/ElectronPlotter.h"
#include "../TopTreeAnalysisBase/Selection/interface/MuonPlotter.h"
#include "../TopTreeAnalysisBase/Selection/interface/JetPlotter.h"
#include "../TopTreeAnalysisBase/Selection/interface/VertexPlotter.h"
#include "../TopTreeAnalysisBase/Tools/interface/JetTools.h"
#include "../TopTreeAnalysisBase/MCInformation/interface/ResolutionFit.h"
#include "../TopTreeAnalysisBase/MCInformation/interface/JetPartonMatching.h"
#include "../TopTreeAnalysisBase/Reconstruction/interface/JetCorrectorParameters.h"
#include "../TopTreeAnalysis/macros/Style.C"
#include "../TopTreeAnalysisBase/MCInformation/interface/LumiReWeighting.h"

using namespace std;
using namespace reweight;
using namespace TopTree;
//Passing arguments to your program, for example from command line, when a program is invoked
//argc:Nof arguments given at command line (including executable name),argv:array of char*s of size argc
int main (int argc, char *argv[])
{
  //Returns the processor time consumed by the program.
  clock_t start = clock();
     
  //SetStyle if needed, this is defined in the ../TopTreeAnalysis/macros/Style.C
  setTDRStyle();
    
  /*|----------------|
    | Configuration  |
    |----------------|*/
  
  //xml file
  string xmlFileName ="myhiggsconfig.xml";
  //additional xmlfile argument 
    if (argc > 1)
        xmlFileName = (string)argv[1];
    
    const char *xmlfile = xmlFileName.c_str();
    
    std::cout << "1) Using the '" << xmlfile << "' file as input" << std::endl;

    /*|------------------------------|
      | Configuration output format  |
      |------------------------------|*/

    //Create a ROOT TTree with name, title
    TTree *configTree = new TTree("configTree","configuration Tree");
    //Create an array that can initially hold 1000 Dataset objects -> to retrieve the ith object of the array (Dataset*) tcdatasets->At(i)
    TClonesArray* tcdatasets = new TClonesArray("Dataset",1000);
    //Create a branch of the configTree(branchname, className=object type, &p_object=pointer to an object)
    //configTree->Branch("Datasets","TClonesArray",&tcdatasets);
    //Create an array that can initially hold 1000 AnalysisEnvironment objects
    TClonesArray* tcAnaEnv = new TClonesArray("AnalysisEnvironment",1000);
    //Create a branch(branchname, className=object type, &p_object=pointer to an object)
    //configTree->Branch("AnaEnv","TClonesArray",&tcAnaEnv);
    
    /*|-----------------------|
      |  AnalysisEnvironment  |
      |-----------------------|*/

    //More information in http://w3.iihe.ac.be/~echabert/Documentation/TopTreeAnalysisPackage/CMSSW_35X_2/html/classAnalysisEnvironment.html    
    AnalysisEnvironment anaEnv;
    std::cout << "2) Loading environment ";
    AnalysisEnvironmentLoader anaLoad(anaEnv,xmlfile);
    std::cout << "and creating AnalysisEnvironmentLoader" << std::endl;

    new ((*tcAnaEnv)[0]) AnalysisEnvironment(anaEnv);
    int verbose = anaEnv.Verbose;
    //old and new lumi?? question!
    float oldLuminosity = anaEnv.Luminosity;	// in 1/pb
    // std::cout << "analysis environment luminosity for rescaling "<< oldLuminosity << std::endl;
    
    /*|-----------------|
      |  Load Datasets  |
      |-----------------|*/

    //Loading the branches from the TopTrees: http://w3.iihe.ac.be/~echabert/Documentation/TopTreeAnalysisPackage/CMSSW_35X_2/html/classTTreeLoader.html
    std::cout << "3) Loading datasets ..." << std::endl;
    TTreeLoader treeLoader;
    vector < Dataset* > datasets;
    treeLoader.LoadDatasets (datasets, xmlfile);

    //Loop over the datasets
    /*    for(unsigned int i=0; i<datasets.size(); i++)
      {
	new ((*tcdatasets)[i]) Dataset(*datasets[i]);
	}*/
    float Luminosity = oldLuminosity;
    
    //Loop over datasets again and check for the equivalent luminosity
    for (unsigned int d=0; d<datasets.size(); d++) 
      {        
	new ((*tcdatasets)[d]) Dataset(*datasets[d]);
	
      if(Luminosity > datasets[d]->EquivalentLumi()) 
	{
	  Luminosity = datasets[d]->EquivalentLumi();
	}
      string dataSetName = datasets[d]->Name();
      std::cout << dataSetName << " with lumi of "<< Luminosity <<std::endl;

      }
    std::cout << "------------------------------------------------------------------------------------------------" <<  std::endl;
    
    
    //Global variable
    //TRootEvent* event = 0;
    string postfix = "_EventSelection_synclana_noNJets"; // to relabel the names of the output file 
    //nof selected events
    Double_t NEvtsData = 0;
    Double_t *nEvents = new Double_t[datasets.size()];
    //vector<string> cutFlowTableText;
    //SelectionTable cutFlowTable(cutFlowTableText, datasets);
    //cutFlowTable.SetLuminosity(Luminosity);
    //cutFlowTable.SetPrecision(1);

    //cutFlowTableText.push_back(string("Event cleaning and Trigger"));
    //cutFlowTableText.push_back(string("Exactly 1 isolated muon"));
    //cutFlowTableText.push_back(string("Exactly 1 isolated muonsssssss"));
    //cutFlowTableText.push_back(string("Exactly 1 isolated muonsssssss"));


    /*|---------------------|
      |  PileUp Reweighting |
      |---------------------|*/

    LumiReWeighting LumiWeights, LumiWeightsUp, LumiWeightsDown;
    
    LumiWeights = LumiReWeighting("../TopTreeAnalysisBase/Calibrations/PileUpReweighting/pileup_MC_Summer12_S10.root", "../TopTreeAnalysisBase/Calibrations/PileUpReweighting/pileup_2012Data53X_UpToRun208357/nominal.root", "pileup", "pileup");
    LumiWeightsUp = LumiReWeighting("../TopTreeAnalysisBase/Calibrations/PileUpReweighting/pileup_MC_Summer12_S10.root", "../TopTreeAnalysisBase/Calibrations/PileUpReweighting/pileup_2012Data53X_UpToRun208357/sys_up.root", "pileup", "pileup");
    LumiWeightsDown = LumiReWeighting("../TopTreeAnalysisBase/Calibrations/PileUpReweighting/pileup_MC_Summer12_S10.root", "../TopTreeAnalysisBase/Calibrations/PileUpReweighting/pileup_2012Data53X_UpToRun208357/sys_down.root", "pileup", "pileup");
    
    //std::cout << "*************** Initialized PileUpReWeighting ******************" << std::endl;
    
    /*|----------------------|
      |	 Loop over datasets  | 
      |----------------------|*/

    std::cout << "------------------------------------------------------------------------------------------------" <<  std::endl;
    std::cout << "                Looping over " << datasets.size () << " datasets in total" << std::endl;
    for (unsigned int d = 0; d < datasets.size (); d++) {
      string previousFilename = "";
      int iFile = -1;
      string dataSetName = datasets[d]->Name();
      std::cout << "------------------------------------------------------------------------------------------------" <<  std::endl;
      std::cout << "Dataset " << d << ": " << datasets[d]->Name () << " with title " << datasets[d]->Title () << 
	" which contains " << datasets[d]->NofEvtsToRunOver() << " events." << std::endl;
      std::cout << "------------------------------------------------------------------------------------------------" <<  std::endl;
      
      /*|---------------------|
        |  Make the ROOT Tree |
        |---------------------|*/

      string roottreename = datasets[d]->Name();
      roottreename+="_tree.root";
      std::cout << "creating tree in file " << roottreename << std::endl;
      
      TFile *fileout = new TFile (roottreename.c_str(), "RECREATE");
      fileout->cd();
      
      Int_t eventID, runID, lumiBlockID;
      Int_t isdata;

      Int_t nElectrons;
      //Define an array of doubles with length 10
      Double_t pX_electron[10];
      Double_t pY_electron[10];
      Double_t pZ_electron[10];
      Double_t E_electron[10];
      Double_t pfIso_electron[10];
      Int_t charge_electron[10];

      Int_t nMuons;
      Double_t pX_muon[10];
      Double_t pY_muon[10];
      Double_t pZ_muon[10];
      Double_t E_muon[10];
      Double_t pfIso_muon[10];
      Int_t charge_muon[10];

      Int_t nJets;
      Double_t pX_jet[10];
      Double_t pY_jet[10];
      Double_t pZ_jet[10];
      Double_t E_jet[10];

      Double_t missingEt;
      
      // Various weights and scale factors
      Double_t pu_weight;
      
      TTree* myTree = new TTree("tree","tree");
      
      myTree->Branch("eventID",&eventID);
      myTree->Branch("runID",&runID);
      myTree->Branch("lumiBlockID",&lumiBlockID);      
      myTree->Branch("isdata",&isdata,"isdata/I");
      
      myTree->Branch("nElectrons",&nElectrons, "nElectrons/I");
      myTree->Branch("pX_electron",pX_electron,"pX_electron[nElectrons]/D");
      myTree->Branch("pY_electron",pY_electron,"pY_electron[nElectrons]/D");
      myTree->Branch("pZ_electron",pZ_electron,"pZ_electron[nElectrons]/D");
      myTree->Branch("E_electron",E_electron,"E_electron[nElectrons]/D");
      myTree->Branch("pfIso_electron",pfIso_electron,"pfIso_electron[nElectrons]/D");
      myTree->Branch("charge_electron",charge_electron,"charge_electron[nElectrons]/I");
      
      myTree->Branch("nMuons",&nMuons, "nMuons/I");
      myTree->Branch("pX_muon",pX_muon,"pX_muon[nMuons]/D");
      myTree->Branch("pY_muon",pY_muon,"pY_muon[nMuons]/D");
      myTree->Branch("pZ_muon",pZ_muon,"pZ_muon[nMuons]/D");
      myTree->Branch("E_muon",E_muon,"E_muon[nMuons]/D");
      myTree->Branch("pfIso_muon",pfIso_muon,"pfIso_muon[nMuons]/D");
      myTree->Branch("charge_muon",charge_muon,"charge_muon[nMuons]/I");
      
      myTree->Branch("nJets",&nJets, "nJets/I");
      myTree->Branch("pX_jet",pX_jet,"pX_jet[nJets]/D");
      myTree->Branch("pY_jet",pY_jet,"pY_jet[nJets]/D");
      myTree->Branch("pZ_jet",pZ_jet,"pZ_jet[nJets]/D");
      myTree->Branch("E_jet",E_jet,"E_jet[nJets]/D");
      
      myTree->Branch("missingEt",&missingEt,"missingEt/D");
      myTree->Branch("pu_weight",&pu_weight,"pu_weight/D");
      
      //myTree->Print();
      
      //open files and load
      std::cout << "4) Loading Event" << std::endl;
      treeLoader.LoadDataset (datasets[d], anaEnv);
      
      /*|-------------------------|
	|  Initialize JEC factors |
	|-------------------------|*/
      
      vector<JetCorrectorParameters> vCorrParam;
      
      /*JetCorrectorParameters *L3JetPar  = new JetCorrectorParameters("../../TopTreeAnalysisBase/Calibrations/JECFiles/Summer12_V3_MC_L3Absolute_AK5PFchs.txt");
      JetCorrectorParameters *L2JetPar  = new JetCorrectorParameters("../../TopTreeAnalysisBase/Calibrations/JECFiles/Summer12_V3_MC_L2Relative_AK5PFchs.txt");
      JetCorrectorParameters *L1JetPar  = new JetCorrectorParameters("../../TopTreeAnalysisBase/Calibrations/JECFiles/Summer12_V3_MC_L1FastJet_AK5PFchs.txt");
      
      //Load the JetCorrectorParameter objects into a vector, IMPORTANT: THE ORDER MATTERS HERE !!!!
      vCorrParam.push_back(*L1JetPar);
      vCorrParam.push_back(*L2JetPar);
      vCorrParam.push_back(*L3JetPar);
      
      if(dataSetName.find("Data") == 0 || dataSetName.find("data") == 0 || dataSetName.find("DATA") == 0) { // DATA!
	JetCorrectorParameters *ResJetCorPar = new JetCorrectorParameters("../../TopTreeAnalysisBase/Calibrations/JECFiles/Summer12_V3_DATA_L2L3Residual_AK5PFchs.txt");
	vCorrParam.push_back(*ResJetCorPar);
	}*/
      
      JetCorrectionUncertainty *jecUnc = new JetCorrectionUncertainty(*(new JetCorrectorParameters("../TopTreeAnalysisBase/Calibrations/JECFiles/Fall12_V6_DATA_UncertaintySources_AK5PFchs.txt", "Total")));
      
      // true means redo also the L1
      JetTools *jetTools = new JetTools(vCorrParam, jecUnc, true);
        
      bool tightIDdisplacedMuons(TRootMuon* mu);
      bool selectedDisplacedMuons(TRootMuon* mu);
      bool mvaNonTrig_HtoZZto4l(TRootElectron* el);
      double electronRelPFrhoIso(TRootElectron* el, TRootEvent* ev);
      bool jetIDLoose();
      bool isScrapingVeto(TRootEvent* ev);
      nEvents[d] = 0;
      int itriggerSemiMu = -1,itriggerSemiEl = -1, previousRun = -1;

      std::vector<TRootVertex*> vertex;
      // these loops over the objects are anw done in the Selection.cc and are not needed but 
      //for now it's better to be able to use them here for our analysis and our selections
      std::vector<TRootMuon*> muons_;
      std::vector <TRootElectron*> electrons_;
      std::vector <TRootJet*> init_jets_corrected;
      std::vector <TRootJet*> jets_;
      std::vector <TRootPFJet*> PFJet;
      std::vector <TRootMET* > mets;
      std::vector <TRootGenJet*> genjets;

      /*|------------------|                                                                                                                                                
        |  Loop on events  |                                                                                                                   
	|------------------|*/

      for (unsigned int ievt = 0; ievt < datasets[d]->NofEvtsToRunOver(); ievt++) // event loop
        {	  
	  nEvents[d]++;
          if(ievt%1000 == 0)
            {
	      std::cout<<"Processing the "<<ievt<<"th event (" << ((double)ievt/(double)datasets[d]->NofEvtsToRunOver())*100  << "%)" << " +> # selected" << flush<<"\r";
            }
	  
	  /*|-------------|
	    |  Load Event |
	    |-------------|*/
	  
	  TRootEvent* event = treeLoader.LoadEvent (ievt, vertex, muons_, electrons_, init_jets_corrected, mets);
	  isdata=0;
	  if(! (dataSetName.find("Data") == 0 || dataSetName.find("data") == 0 || dataSetName.find("DATA") == 0 ) ) {
	    genjets = treeLoader.LoadGenJet(ievt,false);
	    sort(genjets.begin(),genjets.end(),HighestPt()); // HighestPt() is included from the Selection class
	  }
	  else{
	    isdata=1;
	  }
	  
	  /*|--------------------------------|    
	    |  Determine Event Scale Factor  |
	    |--------------------------------|*/
          
	  // scale factor for the event
	  float scaleFactor = 1.;
	  Double_t lumiWeight = LumiWeights.ITweight( (int)event->nTruePU() );          

	  //A simple filter to remove events which arise from the interaction LHC beam with the beampipe known as "beam scraping events".     
	  if(dataSetName.find("Data") != 0 || dataSetName.find("data") != 0 || dataSetName.find("DATA") != 0) {  
	    if(isScrapingVeto(event)==false)continue;
	  }
	  else {
	    double lumiWeight = LumiWeights.ITweight( (int)event->nTruePU() );
	    double lumiWeightOLD=lumiWeight;
	   
	    // PU REWEIGHTING
	    if(dataSetName.find("Data") == 0 || dataSetName.find("data") == 0 || dataSetName.find("DATA") == 0)
	      lumiWeight=1;
	    scaleFactor = scaleFactor*lumiWeight;

	  // up syst -> lumiWeight = LumiWeightsUp.ITweight( (int)event->nTruePU() );
	  // down syst -> lumiWeight = LumiWeightsDown.ITweight( (int)event->nTruePU() );
          
	  pu_weight=lumiWeight;          
	  scaleFactor = scaleFactor*lumiWeight;
	  }
          
	  /*|-----------------------|
            |  Trigger Requirement  |
	    |-----------------------|*/
          
	  string currentFilename = datasets[d]->eventTree()->GetFile()->GetName();
	  if(previousFilename != currentFilename){
	    previousFilename = currentFilename;
	    iFile++;
	    std::cout<<"File changed!!! => iFile = "<<iFile << " new file is " << datasets[d]->eventTree()->GetFile()->GetName() << " in sample " << dataSetName << std::endl;

	    //HLT_Mu22_Photon22_CaloIdL_v

	  }
          
	  int currentRun = event->runId();
          
	  if(previousRun != currentRun)
	    previousRun = currentRun;
	  cout << "currentRun " << currentRun << " iFile " << iFile << endl;	  
          int trigger = treeLoader.iTrigger("HLT_Mu17_Mu8_v1",currentRun,iFile); 
	  cout << "trigger: " << trigger <<endl; 

	  /////////////////////////////////////////////////////////////////////////////
	  // JES SYSTEMATICS && SMEAR JET RESOLUTION TO MIMIC THE RESOLUTION IN DATA //
	  /////////////////////////////////////////////////////////////////////////////
          
	  //if( ! (dataSetName.find("Data") == 0 || dataSetName.find("data") == 0 || dataSetName.find("DATA") == 0 ) )	    
	  //jetTools->correctJetJER(init_jets_corrected, genjets, mets[0], "nominal",false);
	  
	  /*|-----------------|
	    | Event Selection |
	    |-----------------|*/
	  
	  std::vector<TRootElectron*> displacedelectrons;
	  //	  std::cout << "THIS IS THE ELECTRON COLLECTION SIZE: "<< electrons_.size() << endl;
          for(unsigned int i=0;i<electrons_.size();i++) {
            if (fabs(electrons_[i]->Eta())<2.5) {
	      if(fabs(electrons_[i]->superClusterEta()) < 1.4442 || fabs(electrons_[i]->superClusterEta()) > 1.5660){
	        if(electrons_[i]->Pt()>25){
		  if(mvaNonTrig_HtoZZto4l(electrons_[i])) {
		    if(electrons_[i]->passConversion() && electrons_[i]->missingHits()==0){
		      if ((electronRelPFrhoIso(electrons_[i], event))<0.1){
		      cout << "electronRelPFrhoIso in the code : " << (electronRelPFrhoIso(electrons_[i], event)) << endl;
		      displacedelectrons.push_back(electrons_[i]);
		      }
		    }
		  }
		}
	      }
	    }
	  }
	      //  cout << "I am the electron and I have eta value of " << electrons_[i]->superClusterEta() << endl; }
	    //            if (electrons_[i]->Eta()<2.5 && electrons_[i]->Pt()>25){}
	    // if (tightIDdisplacedElectrons(electrons_[i])){}
            //if(selectedDisplacedElectrons(electrons_[i])){displacedelectrons.push_back(electrons_[i]);}
	    //}
	    //std::cout << " THIS IS THE DISPLACED ELECTRON VECTOR SIZE: " << displacedelectrons.size() << std::endl;
	    //for(unsigned int i=0;i<displacedelectrons.size();i++){}

	
	  //leave this here in case we want to see the cutflow
	  std::vector<TRootMuon*> displacedmuons;
          for(unsigned int i=0;i<muons_.size();i++) {
            if (muons_[i]->Eta()<2.5 && muons_[i]->Pt()>25){
	      if (tightIDdisplacedMuons(muons_[i])){
		if(selectedDisplacedMuons(muons_[i])){
		  displacedmuons.push_back(muons_[i]);}
	      }
	    }
	  }
	  //	  std::cout << " THIS IS THE DISPLACED MUON VECTOR SIZE: " << displacedmuons.size() << std::endl;
	  //          for(unsigned int i=0;i<displacedmuons.size();i++){}

	  //if (displacedmuons.size()>0){
	    //cutFlowTable.Fill(d,0,scaleFactor);
	    //cout << "first row " << scaleFactor << endl;
	  //cutFlowTable.Fill(d,1,scaleFactor);
	  //cout << "second row " << scaleFactor << endl;
		//  }
	  //if(displacedmuons.size() =0 ){
	  //  cutFlowTable.Fill(d,2,scaleFactor);
	  //  cout << "fourth row " << scaleFactor << endl;	  
	  //  }
	  // if(displacedmuons.size() >0 ){
	  //    cutFlowTable.Fill(d,3,scaleFactor);
	  //   cout << "third row " << scaleFactor << endl;
	  // }
	   
	  eventID = event->eventId();
          runID = event->runId();
          lumiBlockID = event->lumiBlockId();

	  //Declare selection instance
	  //See definitions in ../TopTreeAnalysisBase/Selection/src/Selection.cc
	  Selection selection(init_jets_corrected, displacedmuons, electrons_, mets, event->kt6PFJets_rho());
	  DisplacedSelection displacedselection(init_jets_corrected, displacedmuons, electrons_, event->kt6PFJets_rho());
	  // Pt, eta, relIso, d0, NmatchedStations, dz, NtrackerLayers, NpixelHits
	  //displacedselection.setDisplacedMuonCuts(25,2.5,0.4,0.3,1,5,0); // standard mu selection but with looser iso
	  //displacedselection.setDisplacedElectronCuts(10,2.5,0.4,0.5,0.3,0); // standard ele selection but with looser iso
	  //displacedselection.setJetCuts(20,2.5,0.01,1.,0.98,0.3,0.1); // standard TOP jet selection
	  for(unsigned int i=0;i<muons_.size();i++) {
	    //cout << "I am a displaced muon with pt: " << muons_[i]->Pt() << endl;
	  }
	  for(unsigned int i=0;i<electrons_.size();i++) {
	    //	    cout << "electronRelPFrhoIso---------------> "<< displacedselection.electronRelPFrhoIso(electrons_[i]) <<endl;
	  }
	  
	  for(unsigned int i=0;i<init_jets_corrected.size();i++) {
	    TRootJet* jets_ = (TRootJet*) init_jets_corrected[i];
	    if(jets_->jetType() == 2){
	      const TRootPFJet* PFJet = static_cast<const TRootPFJet*>(jets_);
	      if(jetIDLoose() == true){
		if (PFJet->Pt() >25 && fabs(PFJet->Eta()) < 2.4 ){    
		  //		  cout << "IN THE LOOP PF THE pfjets---------------> "<< PFJet->Pt() <<endl;   
		}
	      }
	    }
	  }

	  if(isScrapingVeto(event)==false){cout << "scraping veto is true" << isScrapingVeto << endl;}

	  
	  bool isGoodPV = selection.isPVSelected(vertex, 4, 24, 2.);
	  if(!isGoodPV)
	    continue;
	  
	  missingEt=mets[0]->Pt();
          

	  vector<TRootJet*> selectedJets= displacedselection.GetSelectedJets(25, 2.4, true);
	  vector<TRootMuon*> selectedMuons = displacedselection.GetSelectedMuons(25, 2.5, 0.12); //gives same answer as in loop and as in manually adding the cuts values in .cc
	  vector<TRootElectron*> selectedElectrons = displacedselection.GetSelectedElectrons(25, 2.5, 0.1, 0); //gives also the same results - checked!
	  //vector<TRootMuon*> selectedMuons = selection.GetSelectedMuons(vertex[0],selectedJets);          
	  //vector<TRootElectron*> selectedElectrons = selection.GetSelectedElectrons(selectedJets);
	  
	  nElectrons=0;
	  for(int iele=0; iele<selectedElectrons.size() && nElectrons<10; iele++){
	    pX_electron[nElectrons]=selectedElectrons[iele]->Px();
	    pY_electron[nElectrons]=selectedElectrons[iele]->Py();
	    pZ_electron[nElectrons]=selectedElectrons[iele]->Pz();
	    E_electron[nElectrons]=selectedElectrons[iele]->E();
	    Double_t isocorr=0;
            
	    //WHY ADD THIS HERE??
	    // get isolation out, start by getting pu corrections
	    //if(selectedElectrons[iele]->puChargedHadronIso()>0){
	    //isocorr = selectedElectrons[iele]->puChargedHadronIso(); 
	    //}
	    //else if{
	      // go through loads of pain to get rho correction, no function available. code below taken from TRootElectron selector in TopTreeAnalysisBase/*/Selector.cc
	      Double_t EffectiveArea = 0.;

	      if (fabs(selectedElectrons[iele]->superClusterEta()) >= 0.0 && fabs(selectedElectrons[iele]->superClusterEta()) < 1.0 ) EffectiveArea = 0.130;
	      if (fabs(selectedElectrons[iele]->superClusterEta()) >= 1.0 && fabs(selectedElectrons[iele]->superClusterEta()) < 1.479 ) EffectiveArea = 0.137;
	      if (fabs(selectedElectrons[iele]->superClusterEta()) >= 1.479 && fabs(selectedElectrons[iele]->superClusterEta()) < 2.0 ) EffectiveArea = 0.067;
	      if (fabs(selectedElectrons[iele]->superClusterEta()) >= 2.0 && fabs(selectedElectrons[iele]->superClusterEta()) < 2.2 ) EffectiveArea = 0.089;
	      if (fabs(selectedElectrons[iele]->superClusterEta()) >= 2.2 && fabs(selectedElectrons[iele]->superClusterEta()) < 2.3 ) EffectiveArea = 0.107;
	      if (fabs(selectedElectrons[iele]->superClusterEta()) >= 2.3 && fabs(selectedElectrons[iele]->superClusterEta()) < 2.4 ) EffectiveArea = 0.110;
	      if (fabs(selectedElectrons[iele]->superClusterEta()) >= 2.4) EffectiveArea = 0.138;
	      isocorr = event->kt6PFJets_rho()*EffectiveArea;
	      //cout << "FREYAisocorr " << isocorr << endl;
	      //}
	    
	    pfIso_electron[nElectrons]=(selectedElectrons[iele]->chargedHadronIso() + max( selectedElectrons[iele]->neutralHadronIso() + selectedElectrons[iele]->photonIso()  - isocorr, 0.) )/ selectedElectrons[iele]->Pt();
	    charge_electron[nElectrons]=selectedElectrons[iele]->charge();
	    nElectrons++;
	  }
	  nMuons=0;
	  for(int imuo=0; imuo<selectedMuons.size() && nMuons<10; imuo++){
	    pX_muon[nMuons]=selectedMuons[imuo]->Px();
	    pY_muon[nMuons]=selectedMuons[imuo]->Py();
	    pZ_muon[nMuons]=selectedMuons[imuo]->Pz();
	    E_muon[nMuons]=selectedMuons[imuo]->E();
	    pfIso_muon[nMuons]=(selectedMuons[imuo]->chargedHadronIso() + max( 0.0, selectedMuons[imuo]->neutralHadronIso() + selectedMuons[imuo]->photonIso() - 0.5*selectedMuons[imuo]->puChargedHadronIso() ) ) / selectedMuons[imuo]->Pt(); // dBeta corrected

	    
	    charge_muon[nMuons]=selectedMuons[imuo]->charge();
	    nMuons++;
	  }
	  nJets=0;
	  for(int ijet=0; ijet<selectedJets.size() && nJets<10; ijet++){
	    //cout << "check the selectedJets function: " << selectedJets[ijet]->Pt() << endl;
	    pX_jet[nJets]=selectedJets[ijet]->Px();
	    pY_jet[nJets]=selectedJets[ijet]->Py();
	    pZ_jet[nJets]=selectedJets[ijet]->Pz();
	    E_jet[nJets]=selectedJets[ijet]->E();
	    nJets++;
	  }
	 
          if(nElectrons+nMuons>0) {
	    myTree->Fill();
	    //std::cout << "found " << nMuons << " muons and " << nElectrons << " electrons!" << std::endl;
	  }
          
        }			//loop on events
      
      std::cout<<std::endl;
              
      /*|--------------|
	| Jet Cleaning |
	|--------------|*/
      
      if (jecUnc) delete jecUnc;
      if (jetTools) delete jetTools;
      
      myTree->Write();
      fileout->Write();
      fileout->Close();
      //        delete myTree;
      delete fileout;
      
      //important: free memory
      treeLoader.UnLoadDataset();
      
    }				//loop on datasets



    //(bool mergeTT, bool mergeQCD, bool mergeW, bool mergeZ, bool mergeST)                                                                                            
    //cutFlowTable.TableCalculator(true, true, true, true, true);                                                                                                        
    //cutFlowTable.TableCalculator(false, false, false, false, false);
    //WithError(false), writeMerged (true), useBookTabs(false), addRawsyNumbers(false), addEfficiencies(false), addTotalEfficiencies(false), writeLandscape(false)     
    //cutFlowTable.Write("Displaced"+postfix+"Table_Mu.tex",false,true,false,false,false,false,false);

    if (verbose > 0)    
    //Once everything is filled ...
      std::cout << " We ran over all the data ;-)" << std::endl;
    
    // Do some special things with certain plots (normalize, BayesDivide, ... )
    //    if (verbose > 0)
    //        std::cout << "Treating the special plots." << std::endl;
    
    delete tcdatasets;
    delete tcAnaEnv;
    delete configTree;
    
    std::cout << "It took us " << ((double)clock() - start) / CLOCKS_PER_SEC << " to run the program" << std::endl;
    
    std::cout << "********************************************" << std::endl;
    std::cout << "           End of the program !!            " << std::endl;
    std::cout << "********************************************" << std::endl;
    
    return 0;
}

//Scraping veto                                                                                                                                                        
bool isScrapingVeto(TRootEvent* ev){
  bool Yes = false;
  Yes = (ev->nTracks() > 10 && (((float) ev->nHighPurityTracks()) / ((float) ev->nTracks() ) > 0.25));
  return Yes;
}




bool tightIDdisplacedMuons(TRootMuon* mu){
  bool Yes=false;
  Yes=(
       mu->isGlobalMuon() 
       && mu->isPFMuon() 
       && mu->chi2()<10 
       && mu->nofValidPixelHits()>0 
       && mu->nofMatchedStations()>1 
       && mu->nofValidPixelHits()>0 
       && mu->nofTrackerLayersWithMeasurement()>5
       );
  return Yes;
}

bool selectedDisplacedMuons(TRootMuon* mu){
  bool Yes=false;
  float muonRelPFdBetaIso=(mu->chargedHadronIso(4)+max(0.0,mu->neutralHadronIso(4)+mu->photonIso(4)-0.5*mu->puChargedHadronIso(4)))/mu->Pt(); //dBeta corrected
  Yes=(
       tightIDdisplacedMuons(mu) && muonRelPFdBetaIso<0.12
       );
  return Yes;
}

// https://twiki.cern.ch/twiki/bin/view/CMS/MultivariateElectronIdentification#Non_triggering_MVA
bool mvaNonTrig_HtoZZto4l(TRootElectron* el){
  bool Yes=false;
  if(fabs(el->superClusterEta())<0.8 && el->mvaNonTrigId()>-0.34)
    Yes = true;
  else if(fabs(el->superClusterEta())<1.479 && el->mvaNonTrigId()>-0.65)
    Yes = true;
  else if(fabs(el->superClusterEta())<2.5 && el->mvaNonTrigId()>0.60)
    Yes = true;
    return Yes;
}

double electronRelPFrhoIso(TRootElectron* el, TRootEvent* ev) {
   double EffectiveArea=0.;
   double isoCorr=0;
   double electronRelPFrhoIso=0;
   //HCP2012 for el conesize = 0.3 http://cmssw.cvs.cern.ch/cgi-bin/cmssw.cgi/UserCode/EGamma/EGammaAnalysisTools/interface/ElectronEffectiveArea.h?revision=1.4&view=markup
   if(fabs(el->superClusterEta()) >= 0.0   && fabs(el->superClusterEta()) < 1.0   ) EffectiveArea = 0.130;
   if(fabs(el->superClusterEta()) >= 1.0   && fabs(el->superClusterEta()) < 1.479 ) EffectiveArea = 0.137;
   if(fabs(el->superClusterEta()) >= 1.479 && fabs(el->superClusterEta()) < 2.0   ) EffectiveArea = 0.067;
   if(fabs(el->superClusterEta()) >= 2.0   && fabs(el->superClusterEta()) < 2.2   ) EffectiveArea = 0.089;
   if(fabs(el->superClusterEta()) >= 2.2   && fabs(el->superClusterEta()) < 2.3   ) EffectiveArea = 0.107;
   if(fabs(el->superClusterEta()) >= 2.3   && fabs(el->superClusterEta()) < 2.4   ) EffectiveArea = 0.110;
   if(fabs(el->superClusterEta()) >= 2.4) EffectiveArea = 0.138;
   isoCorr = ev->kt6PFJets_rho()*EffectiveArea;
   electronRelPFrhoIso=(el->chargedHadronIso() + max(0.0, el->neutralHadronIso() + el->photonIso()-isoCorr))/ el->Pt();
   return electronRelPFrhoIso;
}

bool jetIDLoose(){
  bool Yes=false;
  TRootPFJet* jet;
  Yes=(
       (jet->nConstituents() > 1)
       && (jet->neutralHadronEnergyFraction() < 0.99)
       && (jet->neutralEmEnergyFraction() < 0.99)
       && (fabs(jet->Eta()) >= 2.4 || jet->chargedEmEnergyFraction() < 0.99)
       && (fabs(jet->Eta()) >= 2.4 || jet->chargedHadronEnergyFraction() > 0)
       && (fabs(jet->Eta()) >= 2.4 || jet->chargedMultiplicity() > 0)
       );
  return Yes;
}
