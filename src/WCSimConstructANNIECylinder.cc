//	-*- mode:c++; tab-width:4;	-*-
#include "WCSimDetectorConstruction.hh"

#include "G4Material.hh"
#include "G4Element.hh"
#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4UnionSolid.hh"
#include "G4Torus.hh"
#include "G4Sphere.hh"
#include "G4Trd.hh"
#include "G4IntersectionSolid.hh"
#include "G4Polyhedra.hh"
#include "G4LogicalVolume.hh"
#include "G4ThreeVector.hh"
#include "G4RotationMatrix.hh"
#include "G4PVReplica.hh"
#include "G4PVPlacement.hh"
#include "G4PVParameterised.hh"
#include "G4AssemblyVolume.hh"
#include "G4SubtractionSolid.hh"
#include "globals.hh"
#include "G4VisAttributes.hh"
#include "G4LogicalBorderSurface.hh"
#include "G4LogicalSkinSurface.hh"
#include "G4OpBoundaryProcess.hh"
#include "G4OpticalSurface.hh"
#include "G4UserLimits.hh"
#include "G4ReflectionFactory.hh"
#include "G4GeometryTolerance.hh"
#include "G4GeometryManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4PhysicalConstants.hh"

#include "WCSimTuningParameters.hh" //jl145

/***********************************************************
 *
 * This file containts the functions which construct a 
 * cylindrical WC detector.	It used by both the SK and 
 * LBNE WC detector modes.	It is called in the Construct()
 * method in WCSimDetectorConstruction.cc.
 *
 * Sourcefile for the WCSimDetectorConstruction class
 *
 ***********************************************************/

G4LogicalVolume* WCSimDetectorConstruction::ConstructANNIECylinder()
{
	G4cout << "**** Building Cylindrical Detector ****" << G4endl;
	
	debugMode = false;
	
	//-----------------------------------------------------
	// Steel Barrel 
	//-----------------------------------------------------
	
	totalAngle = 2.0*pi*rad;
	dPhi = totalAngle/ WCBarrelRingNPhi;
	
	WCIDRadius = WCIDDiameter/2.;
	WCLength = WCIDHeight;
	
	G4double WChalfheightexcess, WCradialexcess;
	// tank is 7GA (7-gauge steel) = (0.1793"normal sheet steel or) 0.1875" stainless steel
	// ^ gauge represents the thickness of 1 ounce of copper rolled out to an area of 1 square foot
	// = 4.7625mm thick 'A36' steel (density 7,800 kg/m3 )
	WChalfheightexcess=4.7625*mm; 
	WCradialexcess=4.7625*mm;
	
	G4Tubs* solidWC = new G4Tubs("WC",
								 0.0*m,
								 WCRadius+WCradialexcess, 
								 0.5*WCLength+WChalfheightexcess,
								 0.*deg,
								 360.*deg);
	
	G4LogicalVolume* logicWC;
	logicWC = new G4LogicalVolume(solidWC,
								  G4Material::GetMaterial("StainlessSteel"),
								  "WC",
								  0,0,0);
	
	G4VisAttributes* showColor = new G4VisAttributes(G4Colour(0.0,1.0,0.0));
	logicWC->SetVisAttributes(showColor);
	
	//logicWC->SetVisAttributes(G4VisAttributes::Invisible); //amb79
	
	//-----------------------------------------------------
	// Water in the Barrel
	//-----------------------------------------------------
	
	//Decide if adding Gd
	water = (WCAddGd) ? "Doped Water" : "Water";
	
	G4Tubs* solidWCBarrel = new G4Tubs("WCBarrel",
										0.0*m,
										WCRadius,
										0.5*WCLength,
										0.*deg,
										360.*deg);
	
	logicWCBarrel = 
		new G4LogicalVolume(solidWCBarrel,
			G4Material::GetMaterial(water),
			"WCBarrel",
			0,0,0);
	
	physiWCBarrel = 
		new G4PVPlacement(0,
						  G4ThreeVector(0.,0.,0.),
						  logicWCBarrel,
						  "WCBarrel",
						  logicWC,
						  false,
						  0);
	
	// This volume needs to made invisible to view the blacksheet and PMTs with RayTracer
	if (Vis_Choice == "RayTracer"){
		logicWCBarrel->SetVisAttributes(G4VisAttributes::Invisible);
	} else {
		//{if(!debugMode)
		//logicWCBarrel->SetVisAttributes(G4VisAttributes::Invisible);} 
	}
	
	//-----------------------------------------------------------
	// The Blacksheet, a daughter of the cells containing PMTs,
	// and also some other volumes to make the edges light tight
	//-----------------------------------------------------------
	
	G4double annulusBlackSheetRmax[2] = {(WCIDRadius+WCBlackSheetThickness),
										  WCIDRadius+WCBlackSheetThickness};
	G4double annulusBlackSheetRmin[2] = {(WCIDRadius),
										  WCIDRadius};
	mainAnnulusHeight = WCIDHeight -2.*WCBarrelPMTOffset;
	G4double mainAnnulusZ[2] = {-mainAnnulusHeight/2., mainAnnulusHeight/2};
	
	G4Polyhedra* solidWCBarrelCellBlackSheet = new G4Polyhedra("WCBarrelCellBlackSheet",
																WCBarrelCellStartPhi, // phi start
																totalAngle,           //total phi
																WCBarrelRingNPhi,     //NPhi-gon
																2,
																mainAnnulusZ,
																annulusBlackSheetRmin,
																annulusBlackSheetRmax);
	
	logicWCBarrelCellBlackSheet =
		new G4LogicalVolume(solidWCBarrelCellBlackSheet,
							G4Material::GetMaterial("Blacksheet"),
							"WCBarrelCellBlackSheet",
							0,0,0);
	
	G4VPhysicalVolume* physiWCBarrelCellBlackSheet =
		new G4PVPlacement(0,
						  G4ThreeVector(0.,0.,InnerStructureCentreOffset),
						  logicWCBarrelCellBlackSheet,
						  "WCBarrelCellBlackSheet",
						  logicWCBarrel,
						  false,
						  0,true);
	G4cout<<"Constructed barrel cell blacksheet with radius "<<WCIDRadius<<" to "<<(WCIDRadius+WCBlackSheetThickness)<<G4endl;
	WCCylInfo[2]=WCIDRadius/10.;
	
	G4LogicalBorderSurface * WaterBSBarrelCellSurface 
		= new G4LogicalBorderSurface("WaterBSBarrelCellSurface",
									 physiWCBarrel,
									 physiWCBarrelCellBlackSheet,
									 OpWaterBSSurface);
	
	// Change made here to have the if statement contain the !debugmode to be consistent
	// This code gives the Blacksheet its color. 
	
	if(Vis_Choice == "RayTracer"){
		G4VisAttributes* WCBarrelBlackSheetCellVisAtt 
			= new G4VisAttributes(G4Colour(0.2,0.9,0.2)); // green color
		WCBarrelBlackSheetCellVisAtt->SetForceSolid(true); // force the object to be visualized with a surface
		WCBarrelBlackSheetCellVisAtt->SetForceAuxEdgeVisible(true); // force auxiliary edges to be shown
		if(!debugMode)
			logicWCBarrelCellBlackSheet->SetVisAttributes(WCBarrelBlackSheetCellVisAtt);
		else
			logicWCBarrelCellBlackSheet->SetVisAttributes(G4VisAttributes::Invisible);
	} else {
		G4VisAttributes* WCBarrelBlackSheetCellVisAtt 
			= new G4VisAttributes(G4Colour(0.2,0.9,0.2));
		if(!debugMode)
			logicWCBarrelCellBlackSheet->SetVisAttributes(G4VisAttributes::Invisible);
		else
			logicWCBarrelCellBlackSheet->SetVisAttributes(WCBarrelBlackSheetCellVisAtt);
	}
	
	//-----------------------------------------------------
	// The PMT
	//-----------------------------------------------------
	
	G4LogicalVolume* logicWCPMT;
	std::vector<G4LogicalVolume*> logicWCPMTs;
	for(auto atankcollection : WCTankCollectionNames){
		G4String thepmtname = WCPMTNameMap.at(atankcollection);
		logicWCPMT= ConstructPMT(thepmtname, atankcollection, "tank");
		logicWCPMTs.push_back(logicWCPMT);
	}
	G4LogicalVolume* logicWCLAPPD=0;
	logicWCLAPPD = ConstructLAPPD(WCLAPPDName, WCIDCollectionName2);
	
	G4VisAttributes* WClogic 
			= new G4VisAttributes(G4Colour(0.4,0.0,0.8));
	WClogic->SetForceSolid(true);
	WClogic->SetForceAuxEdgeVisible(true);
	
	//logicWCPMT->SetVisAttributes(WClogic);
	for(auto alogicWCPMT : logicWCPMTs) alogicWCPMT->SetVisAttributes(G4VisAttributes::Invisible);
	if(logicWCLAPPD) logicWCLAPPD->SetVisAttributes(G4VisAttributes::Invisible);
	
	//////----------- Barrel PMT placement --------------
	G4RotationMatrix* WCPMTRotation = new G4RotationMatrix;
	WCPMTRotation->rotateY(90.*deg);
	
	// azimuthal PMT spacing in rings
	G4double barrelCellWidth   = 2.*WCIDRadius*tan(dPhi/2.)*compressionfactor;
	G4double horizontalSpacing = barrelCellWidth/WCPMTperCellHorizontal;
	// vertical PMT spacing between rings
	G4double verticalSpacing   = (mainAnnulusHeight/WCBarrelNRings)*barrelcompressionfactor;
	
	// extra loop over barrel faces
	PMTcounter=0;
	for(int facei=0; facei<WCBarrelRingNPhi; facei++){
		// rotate the PMT. We need a new rotation matrix for each volume
		G4RotationMatrix* WCPMTRotationNext = new G4RotationMatrix(*WCPMTRotation);
		WCPMTRotationNext->rotateX((dPhi*facei)-67.5*deg);
//		G4double CellCentreX = WCIDRadius * sin(dPhi*(facei+0.5));
//		G4double CellCentreY = WCIDRadius * cos(dPhi*(facei+0.5));
		
		for(G4double i = 0; i < WCPMTperCellHorizontal; i++){
			for(G4double j = 0; j < WCBarrelNRings; j++){
				
				// we have one hole: skip PMT placement
				// if( j==5 && facei==0 && i==0 ) continue;
				// we have three holes: skip PMT placement
                                // if( j==0 && i==0 && facei%2==1) continue;	
			
				// Select the appropriate PMT logical volume
				// PMTs are placed in the vector in the order their collections are defined
				// - i.e. in the order they are declared in DetectorConfigs: 
				// {R7081 (10" WB/LUX), D784KFLB (11" LBNE), R5912HQE (8" HQE), R7081HQE (10" HQE WM)}
				
				// 0:  8",  8"
				// 1:  WB,  8" << 3 faces //  WB,  WB << 5 faces   (3 * 8" on downstream... is upstream better?)
				// 2:  WB,  WM << 2 faces //  WB,  8" << 6 faces
				// 3:  WM,  WB
				// 4:  WB,  WB
				// 5:  8",  8" (missing one)
				// N.B. facei==0 is x<0, upstream.
				// ring j==0 is top of tank.
				/*
				int PMTindex=-1;
				if( j==0 || j==5 || 
				   (j==1&&(facei>1&&facei<5)&&i==0) || 
				   (j==2&&(facei!=3&&facei!=4)&&i==1)
				  ){
					PMTindex = 2;  // 8" HQE new
				} else if( (j==3&&i==0) || (j==2&&(facei==3||facei==4)&&i==1) ){
					PMTindex = 3;  // WM 10" HQE
				} else {
					PMTindex = 0;  // WB 10"
				}
				*/

				//New mapping with actual PMT type positions:
				int PMTindex = -1;
				//Holes
				if ( j == 0 && i == 0 && (facei==1 || facei == 3 || facei == 5 || facei ==7)) continue; 	

				//PMTs
				if (j == 2 || j == 4 || (j == 1 && i == 1)) PMTindex = 2;	// 8" HQE new (Hamamatsu/ANNIE)
				else if (j ==3 && i == 0 && (facei==1 || facei ==3 || facei ==5 || facei == 7)) PMTindex = 3;	// WM 10" HQE
				else PMTindex = 0;	//WB 10"


				logicWCPMT = logicWCPMTs.at(PMTindex);
				G4String pmtCollectionName = WCTankCollectionNames.at(PMTindex);
				
				// account only for rotation of the cell and vertical (ring) translation
				G4ThreeVector PMTPosition =
					G4ThreeVector((-barrelCellWidth/2.+(i+0.5)*horizontalSpacing)*sin((dPhi*facei)+112.5*deg),
								  (-barrelCellWidth/2.+(i+0.5)*horizontalSpacing)*cos((dPhi*facei)+112.5*deg),
								   -mainAnnulusHeight/2.+(j+1)*verticalSpacing-InnerStructureCentreOffset/2.);
				

				// add translation of cell centre
				// this will depend on the PMT type, because of different mounting radii
				G4double MountingRadius = WCIDRadius - WCPMTExposeHeightMap.at(pmtCollectionName);
				G4double CellCentreX = MountingRadius * sin(dPhi*(facei+0.5));
				G4double CellCentreY = MountingRadius * cos(dPhi*(facei+0.5));
				PMTPosition.setX(PMTPosition.getX()+CellCentreX);
				PMTPosition.setY(PMTPosition.getY()+CellCentreY);

				//G4cout <<"PMTID: "<<PMTcounter<<", Position ("<<PMTPosition.getX()<<","<<PMTPosition.getY()<<","<<PMTPosition.getZ()<<")"<<G4endl;

				
				G4VPhysicalVolume* physiWCBarrelPMT =
						new G4PVPlacement(WCPMTRotationNext,    // its rotation
										  PMTPosition,          // its position
										  logicWCPMT,           // its logical volume
										  "WCPMT",              // its name
										  logicWCBarrel,        // its mother volume
										  false,                // no boolean operations
										  PMTcounter,           // a unique ID for this PMT
										  true);                // check overlaps
				PMTcounter++;
			}
		}
	}
	
	//////----------- Barrel LAPPD placement --------------
	G4RotationMatrix* WCLAPPDRotation = new G4RotationMatrix;
	WCLAPPDRotation->rotateY(90.*deg);
	
	for(int facei=0; facei<WCBarrelRingNPhi; facei++){
		// rotate the LAPPD to point to tank centre. We need a new rotation matrix for each G4VPhysicalVolume
		G4RotationMatrix* WCLAPPDRotationNext = new G4RotationMatrix(*WCLAPPDRotation);
		WCLAPPDRotationNext->rotateX((dPhi*facei)-90.*deg);
		
		// position LAPPD on corner of the inner strucutre
		// Note PMTs use double facei=0.5, using int facei=0 accounts for the relative rotation
		//G4double CellCentreX = WCIDRadius * sin(dPhi*facei);
		//G4double CellCentreY = WCIDRadius * cos(dPhi*facei);
		G4double CellCentreX = (WCIDRadius - WCLAPPDSliderThickness) * sin(dPhi*facei);
		G4double CellCentreY = (WCIDRadius - WCLAPPDSliderThickness) * cos(dPhi*facei);

		double verticalSpacingLAPPD	= mainAnnulusHeight/(WCLAPPDperCellVertical+1);
		
		for(G4double j = 0; j < WCLAPPDperCellVertical; j++){	// num LAPPD cols in the central ring
		
		G4ThreeVector LAPPDPosition = G4ThreeVector(CellCentreX,
													CellCentreY,
													-mainAnnulusHeight/2.+(j+1.)*verticalSpacingLAPPD);
		
		G4VPhysicalVolume* physiWCBarrelLAPPD =
		new G4PVPlacement(WCLAPPDRotationNext,                      // its rotation
							LAPPDPosition,                          // its position
							logicWCLAPPD,                           // its logical volume
							"WCLAPPD",                              // its name
							logicWCBarrel,                          // its mother volume
							false,                                  // no boolean operations
							(int)(j+facei*WCLAPPDperCellVertical),  // a unique copy number
							false);                                 // don't check overlaps
		}
	}
	
	//---------------------------- CAPS ----------------------------
	
	ConstructANNIECaps(-1);
	ConstructANNIECaps(1);
	
	//------------------------OUTER DETECTOR-------------------------
	// place 12 * 2" PMTs on the outside of the blacksheet, between the inner structure and the tank wall
	// two on each of the three upstream most octagon face 
	G4RotationMatrix* WCODPMTRotationMatrix = new G4RotationMatrix();
	WCODPMTRotationMatrix->rotateY(180.*deg);
	for(int facei=0; facei<WCBarrelRingNPhi; facei++){
		
		// facei==0 is x<0, upstream. inner structure is rotated with corners upstream/downstream
		//  if( facei!=0 && facei!=7 ) continue; // ALL faces have od pmts
		
//		for(G4double i = 0; i < 2; i++){                                    // 1 OD PMT per face
			for(G4double j = 0; j<WCBarrelNRings; j+=(WCBarrelNRings-1)){   // ODs at top and bottom of face
				
				// Select the appropriate PMT logical volume - from DetectorConfigs:
				// {R7081 (WB/LUX), D784KFLB (LBNE), R5912HQE (new HQE), R7081HQE (HQE WM), EMI9954KB (2" OD)}
				int PMTindex=4;
				logicWCPMT = logicWCPMTs.at(PMTindex);
				G4String pmtCollectionName = WCTankCollectionNames.at(PMTindex);
				
				// account only for rotation of the cell and vertical (ring) translation
				// ring j==0 is top of tank.
				G4double zflip = (j==0) ? -1 : 1;
				G4ThreeVector PMTPosition =
					G4ThreeVector((-barrelCellWidth/2.+horizontalSpacing)*sin((dPhi*facei)+112.5*deg),
								  (-barrelCellWidth/2.+horizontalSpacing)*cos((dPhi*facei)+112.5*deg),
								   -mainAnnulusHeight/2.-InnerStructureCentreOffset/2.
								   +(j+1)*verticalSpacing+zflip*verticalSpacing*1.);
				
				// add translation of cell centre
				G4double MountingRadius = WCIDRadius + WCPMTExposeHeightMap.at(pmtCollectionName);
				G4double CellCentreX = MountingRadius * sin(dPhi*(facei+0.5));
				G4double CellCentreY = MountingRadius * cos(dPhi*(facei+0.5));
				PMTPosition.setX(PMTPosition.getX()+CellCentreX);
				PMTPosition.setY(PMTPosition.getY()+CellCentreY);
				
				G4RotationMatrix* WCPMTRotationNext = (j!=0) ? WCODPMTRotationMatrix : nullptr;
				
				G4VPhysicalVolume* physiOuterDetectorPMT =
						new G4PVPlacement(WCPMTRotationNext,    // its rotation
										  PMTPosition,          // its position
										  logicWCPMT,          // its logical volume
										  "ODPMT",             // its name
										  logicWCBarrel,        // its mother volume
										  false,                // no boolean operations
										  PMTcounter,           // a unique ID for this PMT
										  true);                // check overlaps
				PMTcounter++;
			}
		//}
	}
	
	
	return logicWC;
	
}

//------------------------- Construct Caps ---------------------------
void WCSimDetectorConstruction::ConstructANNIECaps(G4int zflip)
{
	//---------------------------------------------------------------------
	// add cap blacksheet
	// -------------------------------------------------------------------
	
	G4double capBlackSheetZ[2] = {-WCBlackSheetThickness*zflip, 0.};
	G4double capBlackSheetRmin[2] = {0., 0.};
	G4double capBlackSheetRmax[2] = {WCIDRadius+WCBlackSheetThickness, 
									 WCIDRadius+WCBlackSheetThickness};
	G4VSolid* solidWCCapBlackSheet;
	if(WCBarrelRingNPhi*WCPMTperCellHorizontal == WCBarrelNumPMTHorizontal){
		solidWCCapBlackSheet
			= new G4Polyhedra("WCCapBlackSheet",  // name
							  0.*deg,             // phi start
							  totalAngle,         // total phi
							  WCBarrelRingNPhi,   // NPhi-gon
							  2,                  // z-planes
							  capBlackSheetZ,     // position of the Z planes
							  capBlackSheetRmin,  // min radius at the z planes
							  capBlackSheetRmax   // max radius at the Z planes
							  );
		// G4cout << *solidWCCapBlackSheet << G4endl;
	}
	
	G4LogicalVolume* logicWCCapBlackSheet =
		new G4LogicalVolume(solidWCCapBlackSheet,
							G4Material::GetMaterial("Blacksheet"),
							"WCCapBlackSheet",
							0,0,0);
	
	capAssemblyHeight = mainAnnulusHeight/2+1*mm+WCBlackSheetThickness;
	
	G4VPhysicalVolume* physiWCCapBlackSheet =
		new G4PVPlacement(0,
						  G4ThreeVector(0.,0.,capAssemblyHeight*zflip+InnerStructureCentreOffset),
						  logicWCCapBlackSheet,
						  "WCCapBlackSheet",
						  logicWCBarrel,
						  false,
						  0,
						  true);
	G4cout<<"constructed cap blacksheet at height "<<capAssemblyHeight*zflip+InnerStructureCentreOffset<<G4endl;
	WCCylInfo[(zflip>0)]=(capAssemblyHeight*zflip+InnerStructureCentreOffset)/10.;
	
	G4LogicalBorderSurface * WaterBSBottomCapSurface 
			= new G4LogicalBorderSurface("WaterBSCapPolySurface",
										 physiWCBarrel,physiWCCapBlackSheet,
										 OpWaterBSSurface);
	
	G4VisAttributes* WCCapBlackSheetVisAtt 
			= new G4VisAttributes(G4Colour(0.9,0.2,0.2));
	
	// used for OGLSX
	if (Vis_Choice == "OGLSX"){
		G4VisAttributes* WCCapBlackSheetVisAtt 
			= new G4VisAttributes(G4Colour(0.9,0.2,0.2));
		if(!debugMode)
			logicWCCapBlackSheet->SetVisAttributes(G4VisAttributes::Invisible);
		else
			logicWCCapBlackSheet->SetVisAttributes(WCCapBlackSheetVisAtt);
	}
	// used for RayTracer (makes the caps blacksheet yellow)
	if (Vis_Choice == "RayTracer"){
		
		G4VisAttributes* WCCapBlackSheetVisAtt 
		= new G4VisAttributes(G4Colour(1.0,1.0,0.0));
		
		if(!debugMode)
				//logicWCCapBlackSheet->SetVisAttributes(G4VisAttributes::Invisible);
				logicWCCapBlackSheet->SetVisAttributes(WCCapBlackSheetVisAtt);
				// Use first line if you want to make the blacksheet on the caps invisible to view through
			else
				logicWCCapBlackSheet->SetVisAttributes(WCCapBlackSheetVisAtt);
	}
	
	//---------------------------------------------------------
	// Add top and bottom PMTs
	// -----------------------------------------------------
	G4String thecollectionName;
	//G4cout<<"Adding cap PMTs to "<<WCDetectorName<<" detetor"<<G4endl;
	if(zflip>0){
		thecollectionName = WCTankCollectionNames.at(0);
		WCPMTName = WCPMTNameMap.at(thecollectionName); // bottom is R7081
	} else {
		thecollectionName = WCTankCollectionNames.at(1);
		WCPMTName = WCPMTNameMap.at(thecollectionName); // top is D784KFLB
	}
	//G4cout<<"Placing "<<WCPMTName<<" in " << ((zflip>0) ? "bottom" : "top") << " caps"<<G4endl;
	G4LogicalVolume* logicWCPMT = ConstructPMT(WCPMTName, thecollectionName, "tank");
	
	G4double xoffset;
	G4double yoffset;
	G4double zoffset;
	
	G4RotationMatrix* WCCapPMTRotation = new G4RotationMatrix;
	if(zflip==-1){
		WCCapPMTRotation->rotateY(180.*deg);
	}
	
	// loop over the cap and place PMTs
	//G4cout<<"Constructing caps for "<<WCDetectorName<<G4endl;
	
	if(zflip>0){ // bottom cap, PMTs placed in regular grid, but with different X:Y spacing
		G4int CapNCellX = WCCapEdgeLimit/WCCapPMTSpacing + 2;
		G4int CapNCellY = WCCapEdgeLimit/WCCapPMTSpacing*capcompressionratio + 2;
		for ( int i = -CapNCellX ; i < CapNCellX; i++) {
			for (int j = -CapNCellY ; j < CapNCellY; j++) {
				
				xoffset = i*WCCapPMTSpacing + WCCapPMTSpacing*0.5;
				int yside = (j<0) ? -1 : 1;
				yoffset = (j*WCCapPMTSpacing + WCCapPMTSpacing*0.5)*capcompressionratio+(capcentrebarwidth*yside);
				zoffset = -capAssemblyHeight*zflip+InnerStructureCentreOffset+WCCapBottomPMTOffset;
				
				//G4ThreeVector cellpos = G4ThreeVector(xoffset, yoffset, zoffset);
				//if (((sqrt(xoffset*xoffset + yoffset*yoffset) + WCPMTRadius) < WCCapEdgeLimit) ) {
				
				G4double xoffset_rot45 = 1./sqrt(2)*(yoffset+xoffset);
				G4double yoffset_rot45 = 1./sqrt(2)*(yoffset-xoffset);

				G4ThreeVector cellpos = G4ThreeVector(xoffset_rot45, yoffset_rot45, zoffset);
				if (((sqrt(xoffset_rot45*xoffset_rot45 + yoffset_rot45*yoffset_rot45) + WCPMTRadius) < WCCapEdgeLimit) ) {
					//G4cout<<"Constructing bottom cap PMT "<<i<<","<<j<<" at ("
					//		<<xoffset<<", "<<yoffset<<")"<<G4endl;
					G4VPhysicalVolume* physiCapPMT =
								new G4PVPlacement(WCCapPMTRotation,
												  cellpos,        // its position
												  logicWCPMT,     // its logical volume
												  "WCPMT",        // its name 
												  logicWCBarrel,  // its mother volume
												  false,          // no boolean os
												  PMTcounter);    // every PMT need a unique id.
					PMTcounter++;
				} //else { G4cout<<"skipping bottom cap PMT "<<i<<","<<j<<")"<<G4endl; }
			}
		}
	} else { // top cap
		
		// first position the ring of PMTs
		G4double PMTangle = dPhi/WCPMTperCellHorizontal;
		for ( int i = 0 ; i < WCBarrelNumPMTHorizontal; i++) {
			// 2 PMTs per cell, as per the barrel, but with smaller radius 
			xoffset = WCCapPMTPosRadius*cos((i+0.5)*PMTangle);
			yoffset = WCCapPMTPosRadius*sin((i+0.5)*PMTangle);
			zoffset = -capAssemblyHeight*zflip+InnerStructureCentreOffset+WCBlackSheetThickness*zflip+WCCapTopPMTOffset;
			//G4cout<<"Constructing top cap ring PMT "<<i<<" at ("<<xoffset<<", "<<yoffset<<")"<<G4endl;
			
			G4ThreeVector cellpos = G4ThreeVector(xoffset, yoffset, zoffset);
			G4VPhysicalVolume* physiCapPMT =
						new G4PVPlacement(WCCapPMTRotation,  // its rotation
										  cellpos,           // its position
										  logicWCPMT,        // its logical volume
										  "WCPMT",           // its name 
										  logicWCBarrel,     // its mother volume
										  false,             // no boolean os
										  PMTcounter);       // every PMT need a unique id.
			PMTcounter++;
		}
		
		// then position the odd PMTs on the hatch
		PMTangle = 2*pi/numhatchpmts;
		for ( int i = 0 ; i < numhatchpmts; i++) {
			xoffset = WCCapPMTPosRadius2*cos(i*PMTangle);
			yoffset = WCCapPMTPosRadius2*sin(i*PMTangle);
			zoffset = -capAssemblyHeight*zflip+InnerStructureCentreOffset+WCBlackSheetThickness*zflip+WCCapTopPMTOffset;
			//G4cout<<"Constructing top cap hatch PMT "<<i<<" at ("<<xoffset<<", "<<yoffset<<")"<<G4endl;
			
			G4ThreeVector cellpos = G4ThreeVector(xoffset, yoffset, zoffset);
			G4VPhysicalVolume* physiCapPMT =
						new G4PVPlacement(WCCapPMTRotation,   // its rotation
										  cellpos,            // its position
										  logicWCPMT,         // its logical volume
										  "WCPMT",            // its name 
										  logicWCBarrel,      // its mother volume
										  false,              // no boolean os
										  PMTcounter);        // every PMT need a unique id.
			PMTcounter++;
		}
		
	}
	
}


