#include "WCSimPrimaryGeneratorMessenger.hh"
#include "WCSimPrimaryGeneratorAction.hh"
#include "G4UIdirectory.hh"
#include "G4UIcmdWithAString.hh"
#include "G4UIcmdWithAnInteger.hh"
#include "G4ios.hh"
#include "G4UIcmdWith3VectorAndUnit.hh"
#include "G4ThreeVector.hh"
#include "G4SystemOfUnits.hh"

WCSimPrimaryGeneratorMessenger::WCSimPrimaryGeneratorMessenger(WCSimPrimaryGeneratorAction* pointerToAction)
:myAction(pointerToAction)
{
  mydetDirectory = new G4UIdirectory("/mygen/");
  mydetDirectory->SetGuidance("WCSim detector control commands.");

  genCmd = new G4UIcmdWithAString("/mygen/generator",this);
  genCmd->SetGuidance("Select primary generator.");
  //T. Akiri: Addition of laser
  genCmd->SetGuidance("Select generator type: muline, gun, laser, gps, beam, AmBe");
  genCmd->SetParameterName("generator",true);
  genCmd->SetDefaultValue("beam");	// previously muline
  //T. Akiri: Addition of laser
  genCmd->SetCandidates("muline gun laser gps beam AmBe");

  fileNameCmd = new G4UIcmdWithAString("/mygen/vecfile",this);
  fileNameCmd->SetGuidance("Select the file of vectors.");
  fileNameCmd->SetGuidance(" Enter the file name of the vector file");
  fileNameCmd->SetParameterName("fileName",true);
  fileNameCmd->SetDefaultValue("inputvectorfile");
  
  primariesfileDirectoryCmd = new G4UIcmdWithAString("/mygen/primariesdirectory", this);
  primariesfileDirectoryCmd->SetGuidance("Specify the directory containing beam primary root files");
  primariesfileDirectoryCmd->SetParameterName("directoryName",true);
  primariesfileDirectoryCmd->SetDefaultValue("");
  
  neutrinosfileDirectoryCmd = new G4UIcmdWithAString("/mygen/neutrinosdirectory", this);
  neutrinosfileDirectoryCmd->SetGuidance("Specify the directory containing genie neutrino root files. Set this before setting the primariesDirectory. Both should be set at the same time.");
  neutrinosfileDirectoryCmd->SetParameterName("directoryName",true);
  neutrinosfileDirectoryCmd->SetDefaultValue("");

  primariesStartEventCmd = new G4UIcmdWithAnInteger("/mygen/primariesoffset", this);
  primariesStartEventCmd->SetGuidance("The starting entry number for reading primaries");
  primariesStartEventCmd->SetParameterName("primariesoffset",true);
  primariesStartEventCmd->SetDefaultValue(0);

  // For AmBe sim
  ambeFileCmd = new G4UIcmdWithAString("/mygen/AmBefile", this);
  ambeFileCmd->SetGuidance("Set the input ROOT file for the AmBe external primary generator.");
  ambeFileCmd->SetParameterName("AmBefile", false);
  ambeFileCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

  ambeOffsetCmd = new G4UIcmdWith3VectorAndUnit("/mygen/ambeoffset", this);
  ambeOffsetCmd->SetGuidance("Set the translation offset for AmBe input positions.");
  ambeOffsetCmd->SetGuidance("This shifts the ROOT-file particle vertices into detector coordinates.");
  ambeOffsetCmd->SetParameterName("X", "Y", "Z", false);
  ambeOffsetCmd->SetDefaultUnit("cm");
  ambeOffsetCmd->AvailableForStates(G4State_PreInit, G4State_Idle);
}

WCSimPrimaryGeneratorMessenger::~WCSimPrimaryGeneratorMessenger()
{
  delete genCmd;
  delete fileNameCmd;
  delete primariesfileDirectoryCmd;
  delete neutrinosfileDirectoryCmd;
  delete mydetDirectory;
  delete primariesStartEventCmd;
  delete ambeFileCmd;
  delete ambeOffsetCmd;
}

void WCSimPrimaryGeneratorMessenger::SetNewValue(G4UIcommand * command,G4String newValue)
{
  if( command==genCmd )
  {
    if (newValue == "muline")
    {
      G4cout<<"Setting generator source to muline"<<G4endl;
      myAction->SetMulineEvtGenerator(true);
      myAction->SetGunEvtGenerator(false);
      myAction->SetLaserEvtGenerator(false);
      myAction->SetBeamEvtGenerator(false);
      myAction->SetGPSEvtGenerator(false);
    }
    else if ( newValue == "gun")
    {
      G4cout<<"Setting generator source to gun"<<G4endl;
      myAction->SetMulineEvtGenerator(false);
      myAction->SetGunEvtGenerator(true);
      myAction->SetLaserEvtGenerator(false);
      myAction->SetBeamEvtGenerator(false);
      myAction->SetGPSEvtGenerator(false);
    }
    else if ( newValue == "laser")   //T. Akiri: Addition of laser
    {
      G4cout<<"Setting generator source to laser"<<G4endl;
      myAction->SetMulineEvtGenerator(false);
      myAction->SetGunEvtGenerator(false);
      myAction->SetLaserEvtGenerator(true);
      myAction->SetBeamEvtGenerator(false);
      myAction->SetGPSEvtGenerator(false);
    }
    else if ( newValue == "beam")
    {
      G4cout<<"Setting generator source to beam"<<G4endl;
      myAction->SetMulineEvtGenerator(false);
      myAction->SetGunEvtGenerator(false);
      myAction->SetLaserEvtGenerator(false);
      myAction->SetBeamEvtGenerator(true);
      myAction->SetGPSEvtGenerator(false);
    }
    else if ( newValue == "gps")
    {
      G4cout<<"Setting generator source to gps"<<G4endl;
      myAction->SetMulineEvtGenerator(false);
      myAction->SetGunEvtGenerator(false);
      myAction->SetLaserEvtGenerator(false);
      myAction->SetBeamEvtGenerator(false);
      myAction->SetGPSEvtGenerator(true);
    }

    else if (newValue == "AmBe") {
      myAction->SetAmBeRootGenerator(true);

      // Existing messenger likely already turns the others off here.
      // Keep that same pattern, for example:
      myAction->SetMulineEvtGenerator(false);
      myAction->SetGunEvtGenerator(false);
      myAction->SetLaserEvtGenerator(false);
      myAction->SetGPSEvtGenerator(false);
      myAction->SetBeamEvtGenerator(false);

      G4cout << "Primary generator set to external AmBe ROOT input." << G4endl;
   }
  }

  if( command == fileNameCmd )
  {
    myAction->OpenVectorFile(newValue);
    G4cout << "Input vector file set to " << newValue << G4endl;
  }
  
  if( command == primariesfileDirectoryCmd )
  {
    myAction->SetPrimaryFilesDirectory(newValue);
    myAction->SetNewPrimariesFlag(true);
    G4cout << "Input directory set to " << newValue << G4endl;
  }
  
  if( command == neutrinosfileDirectoryCmd )
  {
    myAction->SetNeutrinoFilesDirectory(newValue);
    G4cout << "Input directory set to " << newValue << G4endl;
  }
  
  if( command == primariesStartEventCmd )
  {
    myAction->SetPrimariesOffset(primariesStartEventCmd->GetNewIntValue(newValue));
    G4cout << "Primary files will be read starting from entry "<<newValue << G4endl;
  }

  if (command == ambeFileCmd) {
    myAction->SetAmBeInputFileName(newValue);

    if (!myAction->OpenAmBePrimaryFile(newValue)) {
      G4cerr << "Failed to open AmBe input ROOT file: " << newValue << G4endl;
    }
    else {
      G4cout << "Configured AmBe input ROOT file: " << newValue << G4endl;
    }
  }

  if (command == ambeOffsetCmd) {
    G4ThreeVector offset = ambeOffsetCmd->GetNew3VectorValue(newValue);
    myAction->SetAmBePositionOffset(offset);
    G4cout << "Set AmBe position offset to "
       << offset.x()/cm << " "
       << offset.y()/cm << " "
       << offset.z()/cm << " cm"
       << G4endl;
  }
}

G4String WCSimPrimaryGeneratorMessenger::GetCurrentValue(G4UIcommand* command)
{
  G4String cv;
  
  if( command==genCmd )
  {
    if(myAction->IsUsingMulineEvtGenerator())
      { cv = "muline"; }
    else if(myAction->IsUsingGunEvtGenerator())
      { cv = "gun"; }
    else if(myAction->IsUsingLaserEvtGenerator())
      { cv = "laser"; }   //T. Akiri: Addition of laser
    else if(myAction->IsUsingBeamEvtGenerator())
      { cv = "beam"; }
    else if(myAction->IsUsingGPSEvtGenerator())
      { cv = "gps"; }
  }
  
  return cv;
  G4cout<<"generator is currently "<<cv<<G4endl;
}

