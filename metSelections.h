#ifndef METSELECTIONS_H
#define METSELECTIONS_H

typedef std::vector<ROOT::Math::LorentzVector<ROOT::Math::PxPyPzE4D<float> > > VofP4;
//
// met selections
//

struct metStruct{
  metStruct() : met(-999.), metphi(-999.), metx(-999.), mety(-999.), sumet(-999.)  {}
  float met;
  float metphi;
  float metx;
  float mety;
  float sumet;
};

enum whichMetType {
  usingTcMet = 1,
  usingCaloMet = 2,
  usingTcMet35X = 3,
  usingTcMet_looper = 4
};

//---------------------------------------------------
// Function that checks whether met (or tcmet) was 
// corrected for a given muon.  This uses the value maps
//---------------------------------------------------
bool wasMetCorrectedForThisMuon(int imu, whichMetType type);

//-----------------------------------------------------------
// Function that corrects the met (or tcmet) for a given
// muon in case this was not done in reco.  Uses value maps
//-------------------------------------------------------------
void fixMetForThisMuon(int imu, float& metX, float& metY, whichMetType type);

//-----------------------------------------------------------
// Function that corrects the met (or tcmet) for a given
// muon in case this was not done in reco.  Uses value maps
// same things as above but also corrects sumET
//-------------------------------------------------------------
void fixMetForThisMuon(int imu, float& metX, float& metY, float& sumET, whichMetType type);


//---------------------------------------------
// function to calculate projected MET.
// takes three parameters as input:
//
// 1. met
// 2. met phi
// 3. hypothesis index
//---------------------------------------------
float projectedMET( float met, float metPhi, int hyp_index );

//---------------------------------------------
// as above but simpler for single lepton events
//---------------------------------------------
float projectedMETW( float met, float metPhi, float leptonPhi);

//---------------------------------------------
// utility function find deltaPhi between met
// and nearest hypothesis lepton
//---------------------------------------------
float nearestHypLeptonPhi( float metPhi, int hyp_index);

//---------------------------------------------
// correct tcMET for any hypothesis muons
// that have not been corrected for
//---------------------------------------------
metStruct correctTCMETforHypMuons (int hyp_index, float met_x, float met_y, float sumet);

//---------------------------------------------
// function to calculate latest tcMET
//---------------------------------------------
metStruct correctedTCMET(bool printout = false, ostream& ostr = std::cout);

//---------------------------------------------
//function to perform custom Type1 correction
//---------------------------------------------
metStruct customType1Met( float metx , float mety , float sumet  , VofP4 jets , std::vector<float> cors );

#endif

