#ifndef QuarksGluonTagger_h
#define QuarksGluonTagger_h

// C++
#include <stdint.h>
#include <vector>

// ROOT
#include "TMath.h"
#include "Math/LorentzVector.h"
#include "Math/VectorUtil.h"
#include "TROOT.h"
#include "TFormula.h"

// CMS2

#ifdef CMS2_USE_CMSSW
#include "CMS2/NtupleMacrosHeader/interface/CMS2.h"
#include "CMS2/NtupleMacrosCore/interface/trackSelections.h"
#include "CMS2/NtupleMacrosCore/interface/utilities.h"
#include "CMS2/NtupleMacrosCore/interface/QuarkGluonTagger/QGLikelihoodCalculator.h"
#else                                     
#include "../CMS2.h"
#include "../trackSelections.h"
#include "../utilities.h"
#include "QGLikelihoodCalculator.h"
#endif

using namespace std;
using namespace tas;

//struct indP4_{
//  LorentzVector p4obj;
//  int p4ind;
//};

float getLRM (int ijet , int power);

float constituentPtDistribution(int ijet);

float QGtagger(LorentzVector p4 ,int ijet, QGLikelihoodCalculator * );
 

#endif
