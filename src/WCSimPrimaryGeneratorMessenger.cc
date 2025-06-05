#include "WCSimPrimaryGeneratorMessenger.hh"
#include "WCSimPrimaryGeneratorAction.hh"
#include "G4UIdirectory.hh"
#include "G4UIcmdWithAString.hh"
#include "G4UIcmdWithAnInteger.hh"
#include "G4ios.hh"

WCSimPrimaryGeneratorMessenger::WCSimPrimaryGeneratorMessenger(WCSimPrimaryGeneratorAction* pointerToAction)
:myAction(pointerToAction)
{
  mydetDirectory = new G4UIdirectory("/mygen/");
  mydetDirectory->SetGuidance("WCSim detector control commands.");

  genCmd = new G4UIcmdWithAString("/mygen/generator",this);
  genCmd->SetGuidance("Select primary generator.");
  //T. Akiri: Addition of laser
  genCmd->SetGuidance(" Available generators : muline, gun, laser, gps, beam");
  genCmd->SetParameterName("generator",true);
  genCmd->SetDefaultValue("beam");	// previously muline
  //T. Akiri: Addition of laser
  genCmd->SetCandidates("muline gun laser gps beam");

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
  neutrinosfileDirectoryCmd->SetGuidance("Specify the directory containing nuisance neutrino root files. Set this before setting the primariesDirectory. Both should be set at the same time.");
  neutrinosfileDirectoryCmd->SetParameterName("directoryName",true);
  neutrinosfileDirectoryCmd->SetDefaultValue("");

  primariesStartEventCmd = new G4UIcmdWithAnInteger("/mygen/primariesoffset", this);
  primariesStartEventCmd->SetGuidance("The starting entry number for reading primaries");
  primariesStartEventCmd->SetParameterName("primariesoffset",true);
  primariesStartEventCmd->SetDefaultValue(0);
}

WCSimPrimaryGeneratorMessenger::~WCSimPrimaryGeneratorMessenger()
{
  delete genCmd;
  delete fileNameCmd;
  delete primariesfileDirectoryCmd;
  delete neutrinosfileDirectoryCmd;
  delete mydetDirectory;
  delete primariesStartEventCmd;
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

