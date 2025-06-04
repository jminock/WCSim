#include "WCSimRunAction.hh"
#include "WCSimRunActionMessenger.hh"

#include "G4Run.hh"
#include "G4UImanager.hh"
#include "G4VVisManager.hh"
#include "G4ios.hh"

#include "jhfNtuple.h"

#ifdef REFLEX_DICTIONARY
#include "Cintex/Cintex.h"
#endif
#include "TROOT.h"
#include "TFile.h"
#include "TTree.h"
#include "TBranch.h"
#include "TStreamerInfo.h"
#include "WCSimRootEvent.hh"
#include "WCSimRootGeom.hh"
#include "WCSimPmtInfo.hh"
#include "WCSimLAPPDInfo.hh"
#include "WCSimLAPPDObject.hh"

#include <vector>

int pawc_[500000];                // Declare the PAWC common
struct ntupleStruct jhfNtuple;

WCSimRunAction::WCSimRunAction(WCSimDetectorConstruction* test, WCSimRandomParameters* rand) : wcsimrootsuperevent_mrd(0), wcsimrootsuperevent_facc(0), wcsimrandomparameters(rand)
{
  ntuples = 1;

  // Messenger to allow IO options
  wcsimdetector = test;
  isANNIE=wcsimdetector->GetIsANNIE();
  messenger = new WCSimRunActionMessenger(this);
  OutputFileNum=-1;
  WCSimTree=0;

  wcsimrootoptions = new WCSimRootOptions();
// wcsimrootoptions->PopulateFileVersion();   // read from files and/or git the WCSimVersion and git commit hash
}

WCSimRunAction::~WCSimRunAction()
{

}

void WCSimRunAction::BeginOfRunAction(const G4Run* aRun)
{
  G4cout << "### Run " << aRun->GetRunID() << " start." << G4endl;
  numberOfEventsGenerated = 0;
  numberOfTimesWaterTubeHit = 0;
  numberOfTimesCatcherHit = 0;

#ifdef REFLEX_DICTIONARY
  ROOT::Cintex::Cintex::Enable();
#endif

  // Needed for Root 4.00/04
  WCSimRootEvent::Class()->GetStreamerInfo()->Optimize(kFALSE);
  // MF, aug 2006 ... you never know...
  WCSimRootTrigger::Class()->GetStreamerInfo()->Optimize(kFALSE);

  // Create the events to store in the output tree
  wcsimrootsuperevent = new WCSimRootEvent(); //empty list
  //  wcsimrootsuperevent->AddSubEvent(); // make at least one event
  wcsimrootsuperevent->Initialize(); // make at least one event
  if(isANNIE){
    wcsimrootsuperevent_mrd = new WCSimRootEvent();
    wcsimrootsuperevent_facc = new WCSimRootEvent();
    wcsimrootsuperevent_mrd->Initialize();
    wcsimrootsuperevent_facc->Initialize();
  }
  
  // Create the Root file and output tree
  CreateNewOutputFile();
}

void WCSimRunAction::CloseOutputFile(){
  if(WCSimTree){ 
    TFile* hfile = WCSimTree->GetCurrentFile(); 
    hfile->Close(); delete hfile; hfile=0; WCSimTree=0;
  }
}

void WCSimRunAction::CreateNewOutputFile(){
  CloseOutputFile();
  OutputFileNum++;
  RootFileName = GetRootFileNameBase() + "_" + std::to_string(OutputFileNum) + ".root";
  G4cout<<"Setting output file to "<<RootFileName<<G4endl;
  TFile* hfile = new TFile(RootFileName.c_str(),"RECREATE","WCSim ROOT file");
  hfile->SetCompressionLevel(2);

  // Event tree
  WCSimTree = new TTree("wcsimT","WCSim Tree");

  Int_t branchStyle = 1; //new style by default
  TTree::SetBranchStyle(branchStyle);
  Int_t bufsize = 64000;
  //  TBranch *branch = WCSimTree->Branch("wcsimrootsuperevent", "Jhf2kmrootsuperevent", &wcsimrootsuperevent, bufsize,0);
  wcsimrooteventbranch = WCSimTree->Branch("wcsimrootevent", "WCSimRootEvent", &wcsimrootsuperevent, bufsize,2);
  if(isANNIE){
    wcsimrooteventbranch_mrd = WCSimTree->Branch("wcsimrootevent_mrd", "WCSimRootEvent", &wcsimrootsuperevent_mrd, bufsize,2);
    wcsimrooteventbranch_facc = WCSimTree->Branch("wcsimrootevent_facc", "WCSimRootEvent", &wcsimrootsuperevent_facc, bufsize,2);
  }
  
  // Geometry tree - store a copy in each output file (a bit redundant but safer and doesn't take much space)
  geoTree = new TTree("wcsimGeoT","WCSim Geometry Tree");
  wcsimrootgeom = new WCSimRootGeom();
  TBranch *geoBranch = geoTree->Branch("wcsimrootgeom", "WCSimRootGeom", &wcsimrootgeom, bufsize,0);
  FillGeoTree();

  // Options tree
  optionsTree = new TTree("wcsimRootOptionsT","WCSim Options Tree");
  optionsTree->Branch("wcsimrootoptions", "WCSimRootOptions", &wcsimrootoptions, bufsize, 0);

  //set detector & random options
  wcsimdetector->SaveOptionsToOutput(wcsimrootoptions);
  wcsimrandomparameters->SaveOptionsToOutput(wcsimrootoptions);
}

void WCSimRunAction::EndOfRunAction(const G4Run* aRun)
{
  G4cout << "### Run " << aRun->GetRunID() << " end." << G4endl;
//G4cout << "Number of Events Generated: "<< numberOfEventsGenerated << G4endl;
//G4cout << "Number of times MRD hit: " << numberOfTimesMRDHit << G4endl;
//G4cout << "Number of times FGD hit: "    << numberOfTimesFGDHit << G4endl;
//G4cout << "Number of times lArD hit: "  << numberOfTimeslArDHit << G4endl;
//G4cout<<"Number of times waterTube hit: " << numberOfTimesWaterTubeHit<<G4endl;
//   G4cout << ((float(numberOfTimesMRDHit)+float(numberOfTimesFGDHit))/float(numberOfEventsGenerated))*100.
// 	 << "% hit FGD or MRD" << G4endl;
//   G4cout << "Number of times Catcher hit: " << numberOfTimesCatcherHit<<G4endl;
//   G4cout << "Number of times Rock hit: " << numberOfTimesRockHit<<G4endl;
//  G4cout << (float(numberOfTimesCatcherHit)/float(numberOfEventsGenerated))*100.
//        << "% through-going (hit Catcher)" << G4endl;

  //Write the options tree
  G4cout << "EndOfRunAction" << G4endl;
  optionsTree->Fill();
  optionsTree->Write();
  
  // Close the Root file at the end of the run
  CloseOutputFile();

  // Clean up stuff on the heap; I think deletion of hfile and trees
  // is taken care of by the file close

  delete wcsimrootsuperevent; wcsimrootsuperevent=0;
  if(wcsimrootsuperevent_mrd) delete wcsimrootsuperevent_mrd; wcsimrootsuperevent_mrd=0;
  if(wcsimrootsuperevent_facc) delete wcsimrootsuperevent_facc; wcsimrootsuperevent_facc=0;
  delete wcsimrootgeom; wcsimrootgeom=0;

}

void WCSimRunAction::FillGeoTree(){
  // Fill the geometry tree
  G4int geo_type;
  G4double cylinfo[3];
  G4double pmtradius;
  G4int numpmt;
  G4double lappdradius;
  G4int numlappd;
  G4double mrdpmtradius;
  G4int nummrdpmts;
  G4double faccpmtradius;
  G4int numfaccpmts;
  G4int orientation;
  Float_t offset[3];
  
  Int_t tubeNo;
  Int_t lappdNo;
  Float_t pos[3];
  Float_t rot[3];
  Int_t cylLoc;
  Float_t zeroes[3]={0.,0.,0.};

  if (wcsimdetector->GetIsEggShapedHyperK()) {
      geo_type = 2;
  }
  else {
      geo_type = 0;
  }
  // geo_type 1 is for defunct mailbox design

  wcsimrootgeom-> SetGeo_Type(geo_type);

  if (geo_type == 0) {
      //cylinder
      if(not isANNIE){
        cylinfo[1] = wcsimdetector->GetGeo_Dm(3);
        cylinfo[2] = wcsimdetector->GetGeo_Dm(2);
      } else {
        cylinfo[1] = wcsimdetector->GetGeo_Dm(2);
        cylinfo[2] = abs(wcsimdetector->GetGeo_Dm(0)- wcsimdetector->GetGeo_Dm(1));
      }
      wcsimrootgeom-> SetWCCylRadius(cylinfo[1]);
      wcsimrootgeom-> SetWCCylLength(cylinfo[2]);
  }

  std::vector<G4String> tankcollectionnames = wcsimdetector->GetIDCollectionNames();
  if(tankcollectionnames.size()!=0){
    numpmt=0;
    for(auto acollectionname : tankcollectionnames){
      WCSimPMTObject* PMT = wcsimdetector->GetPMTPointer(acollectionname);
      Float_t pmtSize = PMT->GetRadius()/CLHEP::cm;
      wcsimrootgeom->AddPmtRadius(pmtSize);
      G4String thepmtname = PMT->GetPMTName();
      wcsimrootgeom->AddPmtName(thepmtname);
      Int_t pmtcount = wcsimdetector->GetTotalNumPmts(acollectionname);
      wcsimrootgeom->AddPmtTypeCount(pmtcount);
      numpmt+=pmtcount;
    }
    if(wcsimdetector->GetTotalNumPmts()!=numpmt){
      std::cerr<<"RunAction PMT count "<<numpmt<<" does not match DetectorConstruction TotalNumPmts "
               <<wcsimdetector->GetTotalNumPmts()<<std::endl;
      assert(false);
    }
  } else {
    numpmt = wcsimdetector->GetTotalNumPmts();
  }
  pmtradius = wcsimdetector->GetPMTSize1();
  
  orientation = 0;
  
  wcsimrootgeom-> SetWCPMTRadius(pmtradius);
  wcsimrootgeom-> SetOrientation(orientation);
  
  G4ThreeVector offset1= wcsimdetector->GetWCOffset();
  offset[0] = offset1[0];
  offset[1] = offset1[1];
  offset[2] = offset1[2];
  wcsimrootgeom-> SetWCOffset(offset[0],offset[1],offset[2]);
  
  std::vector<WCSimPmtInfo*> *fpmts = wcsimdetector->Get_Pmts();
  if (fpmts->size() != (unsigned int)numpmt) {
    G4cout << "Mismatch between number of pmts and pmt list in geofile.txt!!"<<G4endl;
    G4cout << fpmts->size() <<" vs. "<< numpmt <<G4endl;
    assert(false);
  }
  
  WCSimPmtInfo *pmt;
  std::string tankpmtname="unset";
  G4String WCCollectionName = wcsimdetector->GetIDCollectionName();
  if(WCCollectionName!="WCIDCollectionNameIsUnused"){
    WCSimPMTObject * WCPMT = wcsimdetector->GetPMTPointer(WCCollectionName);
    tankpmtname = WCPMT->GetPMTName();
  }
  for (unsigned int i=0;i!=fpmts->size();i++){
    pmt = ((WCSimPmtInfo*)fpmts->at(i));
    pos[0] = (Float_t)pmt->Get_transx();
    pos[1] = (Float_t)pmt->Get_transy();
    pos[2] = (Float_t)pmt->Get_transz();
    rot[0] = (Float_t)pmt->Get_orienx();
    rot[1] = (Float_t)pmt->Get_orieny();
    rot[2] = (Float_t)pmt->Get_orienz();
    tubeNo = pmt->Get_tubeid();
    cylLoc = pmt->Get_cylocation();
    if(tankcollectionnames.size()!=0){
      wcsimrootgeom-> SetPMT(i,tubeNo,cylLoc,rot,pos,(std::string)wcsimdetector->GetTubeCollection(tubeNo));
      G4String thistubescollectionname = wcsimdetector->GetTubeCollection(tubeNo);
      auto it = std::find(tankcollectionnames.begin(), tankcollectionnames.end(), thistubescollectionname);
      int thecollectionindex=-1;
      if(it!=tankcollectionnames.end()){
        thecollectionindex = std::distance(tankcollectionnames.begin(), it);
      }
      wcsimrootgeom->SetTubeIdType(tubeNo, thecollectionindex);
    } else {
      if(tankpmtname=="unset"){
        std::cerr<<"RunAction error filling wcsimrootgeom!"
                   " tankcollectionnames not filled, but WCIDCollectionName not set"<<std::endl;
        assert(false);
      }
      wcsimrootgeom-> SetPMT(i,tubeNo,cylLoc,rot,pos,tankpmtname);
    }
  }
  
  wcsimrootgeom-> SetWCNumPMT(numpmt);
  
  if(isANNIE){
    lappdradius = wcsimdetector->GetLAPPDSize1();
    numlappd = wcsimdetector->GetTotalNumLAPPDs();
    mrdpmtradius = wcsimdetector->GetMRDPMTRadius();
    nummrdpmts = wcsimdetector->GetTotalNumMrdPmts();
    faccpmtradius = wcsimdetector->GetFACCPMTRadius();
    numfaccpmts = wcsimdetector->GetTotalNumFaccPmts();
    
    wcsimrootgeom-> SetWCLAPPDRadius(lappdradius);
    wcsimrootgeom-> SetMRDPMTRadius(mrdpmtradius);
    wcsimrootgeom-> SetFACCPMTRadius(faccpmtradius);
  
    std::vector<WCSimLAPPDInfo*> *flappds = wcsimdetector->Get_LAPPDs();
    WCSimLAPPDInfo *lappd;
    // Get LAPPD name
    G4String WCIDCollectionName = wcsimdetector->GetIDCollectionName2();
    WCSimLAPPDObject * LAPPD = wcsimdetector->GetLAPPDPointer(WCIDCollectionName);
    std::string lappdname = LAPPD->GetLAPPDName();
    for (unsigned int i=0;i!=flappds->size();i++){
      lappd = ((WCSimLAPPDInfo*)flappds->at(i));
      pos[0] = (Float_t)lappd->Get_transx();
      pos[1] = (Float_t)lappd->Get_transy();
      pos[2] = (Float_t)lappd->Get_transz();
      rot[0] = (Float_t)lappd->Get_orienx();
      rot[1] = (Float_t)lappd->Get_orieny();
      rot[2] = (Float_t)lappd->Get_orienz();
      lappdNo = lappd->Get_lappdid();
      cylLoc = lappd->Get_cylocation();
      wcsimrootgeom-> SetLAPPD(i,lappdNo,cylLoc,rot,pos,lappdname);
      //G4cout<<"lappd= "<<lappdNo<<" at position: "<<pos[0]<<","<<pos[1]<<","<<pos[2]<<G4endl;
    }
    if (flappds->size() != (unsigned int)numlappd) {
      G4cout << "Mismatch between number of lappds and lappd list in geofile.txt!!"<<G4endl;
      G4cout << flappds->size() <<" vs. "<< numlappd <<G4endl;
    }
    
    // mrd pmts
    G4String WCMRDCollectionName = wcsimdetector->GetMRDCollectionName();
    WCSimPMTObject * MRDPMT = wcsimdetector->GetPMTPointer(WCMRDCollectionName);
    std::string mrdpmtname = MRDPMT->GetPMTName();
    fpmts = wcsimdetector->Get_MrdPmts();
    for (unsigned int i=0;i!=fpmts->size();i++){
      pmt = ((WCSimPmtInfo*)fpmts->at(i));
      pos[0] = (Float_t)pmt->Get_transx();
      pos[1] = (Float_t)pmt->Get_transy();
      pos[2] = (Float_t)pmt->Get_transz();
      rot[0] = (Float_t)pmt->Get_orienx();
      rot[1] = (Float_t)pmt->Get_orieny();
      rot[2] = (Float_t)pmt->Get_orienz();
      tubeNo = pmt->Get_tubeid();
      cylLoc = pmt->Get_cylocation();
      wcsimrootgeom-> SetPMT(i,tubeNo,cylLoc,rot,pos,mrdpmtname);
    }
    if (fpmts->size() != (unsigned int)nummrdpmts) {
      G4cout << "Mismatch between number of mrd pmts and pmt list in geofile.txt!!"<<G4endl;
      G4cout << fpmts->size() <<" vs. "<< nummrdpmts <<G4endl;
    }
    
    //facc pmts
    G4String WCFACCCollectionName = wcsimdetector->GetFACCCollectionName();
    WCSimPMTObject * FACCPMT = wcsimdetector->GetPMTPointer(WCFACCCollectionName);
    std::string faccpmtname = FACCPMT->GetPMTName();
    fpmts = wcsimdetector->Get_FaccPmts();
    for (unsigned int i=0;i!=fpmts->size();i++){
      pmt = ((WCSimPmtInfo*)fpmts->at(i));
      pos[0] = (Float_t)pmt->Get_transx();
      pos[1] = (Float_t)pmt->Get_transy();
      pos[2] = (Float_t)pmt->Get_transz();
      rot[0] = (Float_t)pmt->Get_orienx();
      rot[1] = (Float_t)pmt->Get_orieny();
      rot[2] = (Float_t)pmt->Get_orienz();
      tubeNo = pmt->Get_tubeid();
      cylLoc = pmt->Get_cylocation();
      wcsimrootgeom-> SetPMT(i,tubeNo,cylLoc,rot,pos,faccpmtname);
    }
    if (fpmts->size() != (unsigned int)numfaccpmts) {
      G4cout << "Mismatch between number of facc pmts and pmt list in geofile.txt!!"<<G4endl;
      G4cout << fpmts->size() <<" vs. "<< numfaccpmts <<G4endl;
    }

    // G4cout <<"#lappds: "<<flappds->size() <<" vs. "<< numlappd <<G4endl;
  
    wcsimrootgeom-> SetWCNumLAPPD(numlappd);
    wcsimrootgeom-> SetWCNumMrdPMT(nummrdpmts);
    wcsimrootgeom-> SetWCNumFaccPMT(numfaccpmts);
  } else { // ! isANNIE
    wcsimrootgeom-> SetWCLAPPDRadius(0.);
    wcsimrootgeom-> SetMRDPMTRadius(0.);
    wcsimrootgeom-> SetFACCPMTRadius(0.);
    wcsimrootgeom-> SetWCNumLAPPD(0);
    wcsimrootgeom-> SetWCNumMrdPMT(0);
    wcsimrootgeom-> SetWCNumFaccPMT(0);
  }
  
  // debugging
//  Float_t thecylrad = wcsimrootgeom->GetWCCylRadius();
//  G4cout<<"thecylrad="<<thecylrad<<G4endl;
//  Float_t thecyllength = wcsimrootgeom->GetWCCylLength();
//  G4cout<<"thecyllength="<<thecyllength<<G4endl;
//  Int_t thegeotype = wcsimrootgeom->GetGeo_Type();
//  G4cout<<"thegeotype="<<thegeotype<<G4endl;
//  Int_t thenumpmt = wcsimrootgeom->GetWCNumPMT();
//  G4cout<<"thenumpmt="<<thenumpmt<<G4endl;
//  Float_t thepmtrad = wcsimrootgeom->GetWCPMTRadius();
//  G4cout<<"thepmtrad="<<thepmtrad<<G4endl;
//  Int_t thenumlappd = wcsimrootgeom->GetWCNumLAPPD();
//  G4cout<<"thenumlappd="<<thenumlappd<<G4endl;
//  Float_t thelappdrad = wcsimrootgeom->GetWCLAPPDRadius();
//  G4cout<<"thelappdrad="<<thelappdrad<<G4endl;
//  for(int indx=0;indx<3;indx++){
//    Float_t thewcoffset = wcsimrootgeom->GetWCOffset(indx);
//    G4cout<<"thewcoffset["<<indx<<"]="<<thewcoffset<<G4endl;
//  }
//  Int_t theorient = wcsimrootgeom->GetOrientation();
//  G4cout<<"theorient="<<theorient<<G4endl;
//  for(int indx=0;indx<thenumpmt;indx++){
//    WCSimRootPMT therootpmt = wcsimrootgeom->GetPMT(indx);
//    //G4cout<<"therootpmt["<<indx<<"]="<<therootpmt<<G4endl;
//    Int_t thetubeno = therootpmt.GetTubeNo();
//    G4cout<<"thetubeno["<<indx<<"]="<<thetubeno<<G4endl;
////    Int_t thelappdno = therootpmt.GetLAPPDNo();
////    G4cout<<"thelappdno["<<indx<<"]="<<thelappdno<<G4endl;
//    Int_t thecylloc = therootpmt.GetCylLoc();
//    G4cout<<"thecylloc["<<indx<<"]="<<thecylloc<<G4endl;
//    for(int indxx=0;indxx<3;indxx++){
//      Float_t theorient = therootpmt.GetOrientation(indxx);
//      G4cout<<"theorient["<<indx<<","<<indxx<<"]="<<theorient<<G4endl;
//      Float_t thepos = therootpmt.GetPosition(indxx);
//      G4cout<<"thepos["<<indx<<","<<indxx<<"]="<<thepos<<G4endl;
//    }
//  }
//  for(int indx=0;indx<thenumlappd;indx++){
//    WCSimRootPMT therootpmt = wcsimrootgeom->GetLAPPD(indx);
//    //G4cout<<"therootpmt["<<indx<<"]="<<therootpmt<<G4endl;
//    Int_t thetubeno = therootpmt.GetTubeNo();
//    G4cout<<"thetubeno["<<indx<<"]="<<thetubeno<<G4endl;
////    Int_t thelappdno = therootpmt.GetLAPPDNo();
////    G4cout<<"thelappdno["<<indx<<"]="<<thelappdno<<G4endl;
//    Int_t thecylloc = therootpmt.GetCylLoc();
//    G4cout<<"thecylloc["<<indx<<"]="<<thecylloc<<G4endl;
//    for(int indxx=0;indxx<3;indxx++){
//      Float_t theorient = therootpmt.GetOrientation(indxx);
//      G4cout<<"theorient["<<indx<<","<<indxx<<"]="<<theorient<<G4endl;
//      Float_t thepos = therootpmt.GetPosition(indxx);
//      G4cout<<"thepos["<<indx<<","<<indxx<<"]="<<thepos<<G4endl;
//    }
//  }
//  G4cout<<"wcsimrootgeom is at "<<wcsimrootgeom<<G4endl;
  // end debugging
  
  geoTree->Fill();
  geoTree->Write();
}
