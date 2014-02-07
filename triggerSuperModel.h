#ifndef TRIGGERSUPERMODEL_H
#define TRIGGERSUPERMODEL_H

#include "Math/LorentzVector.h"
#include "Math/VectorUtil.h"
#include "TMath.h"
#ifdef CMS2_USE_CMSSW
#include "CMS2/NtupleMacrosHeader/interface/CMS2.h"
#else
#include "CMS2.h"
#endif

//----------------------------------------
// To be used on Monte Carlo events.
// Input is hypothesis index.
// Output is the 2010 trigger efficiency for the 
// given hypothesis.
//----------------------------------------
float triggerSuperModelEffic(int hyp);

#endif
