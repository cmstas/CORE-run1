// $Id: jetSelections.cc,v 1.12 2010/10/16 18:33:04 dmytro Exp $

#include <algorithm>
#include <utility>
#include "Math/VectorUtil.h"
#include "jetSelections.h"
#include "jetcorr/JetCorrectorParameters.icc"
#include "jetcorr/FactorizedJetCorrector.icc"
#include "jetcorr/SimpleJetCorrector.icc"

using std::vector;
using std::pair;

// define this type for speed: allows us to get a vector of selected
// jets, potentially with a correction factor, without having to make
// copies
typedef vector<pair<const LorentzVector *, double> > jets_with_corr_t;

// function to give us the indices of jets passing kinematic and cleaning cuts
static jets_with_corr_t getJets_fast (unsigned int i_hyp, enum JetType type, enum CleaningType cleaning,
				      double deltaR, double min_pt, double max_eta)
{
     // JPT, PF or calo jets?  Introduce this variable so we only have to decide once
     const vector<LorentzVector> *jets = 0;
     switch (type) {
     case JETS_TYPE_JPT:
	  jets = &cms2.jpts_p4();
	  break;
     case JETS_TYPE_CALO_CORR: case JETS_TYPE_CALO_UNCORR:
	  jets = &cms2.jets_p4();
	  break;
     case JETS_TYPE_PF_CORR: case JETS_TYPE_PF_UNCORR:
	  jets = &cms2.pfjets_p4();
	  break;
#if haveGEN	  
     case JETS_TYPE_GEN:
	  jets = &cms2.genjets_p4();
	  break;
#endif
     }
     jets_with_corr_t ret;
     ret.reserve(jets->size()); // reserve so we don't have to realloc later, which is slow
     for (unsigned int i = 0; i < jets->size(); ++i) {
	  //------------------------------------------------------------
	  // min pt cut
	  //------------------------------------------------------------
	  double corr = 1;
	  // CALO_CORR and PF_CORR need to be pt-corrected
	  switch (type) {
	  case JETS_TYPE_CALO_CORR:
	       corr = cms2.jets_cor().at(i);
	       break;
	  case JETS_TYPE_PF_CORR:
	       corr = cms2.pfjets_cor().at(i);
	       break;
	  case JETS_TYPE_JPT: 
	       corr = cms2.jpts_cor().at(i);
	       break;
	  case JETS_TYPE_CALO_UNCORR: 
	  case JETS_TYPE_PF_UNCORR:
#if haveGEN	  
	  case JETS_TYPE_GEN:
#endif
	       break;
	  }
	  const double pt = jets->at(i).pt() * corr;
	  if (pt < min_pt) 
	       goto conti;
	  //------------------------------------------------------------
	  // max |eta| cut
	  //------------------------------------------------------------
	  if (fabs(jets->at(i).eta()) > max_eta)
	       goto conti;
	  //------------------------------------------------------------
	  // lepton cleaning
	  //------------------------------------------------------------
	  switch (cleaning) {
	  case JETS_CLEAN_NONE:
	       break;
	  case JETS_CLEAN_HYP_E_MU:
	  {
	       const LorentzVector &lt = cms2.hyp_lt_p4().at(i_hyp);
	       if (ROOT::Math::VectorUtil::DeltaR(jets->at(i), lt) < deltaR)
		    goto conti;
	       const LorentzVector &ll = cms2.hyp_ll_p4().at(i_hyp);
	       if (ROOT::Math::VectorUtil::DeltaR(jets->at(i), ll) < deltaR)
		    goto conti;
	       break;
	  }
	  case JETS_CLEAN_HYP_E:
	  {
	       const LorentzVector &lt = cms2.hyp_lt_p4().at(i_hyp);
	       const int lt_id = cms2.hyp_lt_id().at(i_hyp);
	       if (abs(lt_id) == 11 && ROOT::Math::VectorUtil::DeltaR(jets->at(i), lt) < deltaR)
		    goto conti;
	       const LorentzVector &ll = cms2.hyp_ll_p4().at(i_hyp);
	       const int ll_id = cms2.hyp_ll_id().at(i_hyp);
	       if (abs(ll_id) == 11 && ROOT::Math::VectorUtil::DeltaR(jets->at(i), ll) < deltaR)
		    goto conti;
	       break;
	  }
	  case JETS_CLEAN_SINGLE_E:
	  {
	       const LorentzVector &e = cms2.els_p4().at(i_hyp);
	       if (ROOT::Math::VectorUtil::DeltaR(jets->at(i), e) < deltaR)
		    goto conti;
	       break;
	  }
	  default:
	       assert(false);
	  }
	  //------------------------------------------------------------
	  // jet ID cuts
	  //------------------------------------------------------------
	  if (not passesCaloJetID(jets->at(i)))
	       goto conti;
	  //------------------------------------------------------------
	  // jet passed all cuts
	  //------------------------------------------------------------
	  ret.push_back(pair<const LorentzVector *, double>(&jets->at(i), corr));
	  continue;
	  //------------------------------------------------------------
	  // jet failed
	  //------------------------------------------------------------
     conti:
	  ret.push_back(pair<const LorentzVector *, double>(0, corr));
     }
     return ret;
}

struct jets_pt_gt {
     bool operator () (const LorentzVector &v1, const LorentzVector &v2) 
	  {
	       return v1.pt() > v2.pt();
	  }
};

// functions that we let other people use
vector<LorentzVector> getJets (unsigned int i_hyp, bool sort_, 
			       enum JetType type, enum CleaningType cleaning,
			       double deltaR, double min_pt, double max_eta)
{
     jets_with_corr_t jets = getJets_fast(i_hyp, type, cleaning, deltaR, min_pt, max_eta);
     vector<LorentzVector> ret;
     ret.reserve(jets.size());
     for (unsigned int i = 0; i < jets.size(); ++i) {
	  // correct the jet momentum if a corrected jet type was requested
	  if (jets[i].first != 0)
	       ret.push_back(*jets[i].first * jets[i].second);
     }
     if (sort_)
	  sort(ret.begin(), ret.end(), jets_pt_gt());
     return ret;
}

std::vector<bool> getJetFlags (unsigned int i_hyp, enum JetType type, enum CleaningType cleaning,
	   double deltaR, double min_pt, double max_eta)
{
     jets_with_corr_t jets = getJets_fast(i_hyp, type, cleaning, deltaR, min_pt, max_eta);
     vector<bool> ret;
     ret.reserve(jets.size());
     for (unsigned int i = 0; i < jets.size(); ++i) {
	  // correct the jet momentum if a corrected jet type was requested
	  ret.push_back(jets[i].first != 0);
     }
     return ret;
}


int nJets (unsigned int i_hyp, enum JetType type, enum CleaningType cleaning,
	   double deltaR, double min_pt, double max_eta)
{
     jets_with_corr_t jets = getJets_fast(i_hyp, type, cleaning, deltaR, min_pt, max_eta);
     int ret = 0;
     for (unsigned int i = 0; i < jets.size(); ++i) {
	  // correct the jet momentum if a corrected jet type was requested
	  if (jets[i].first != 0)
	       ret++;
     }
     return ret;
}

double sumPt (unsigned int i_hyp, enum JetType type, enum CleaningType cleaning,
	      double deltaR, double min_pt, double max_eta)
{
     jets_with_corr_t jets = getJets_fast(i_hyp, type, cleaning, deltaR, min_pt, max_eta);
     double ret = 0;
     for (unsigned int i = 0; i < jets.size(); ++i) {
	  // correct the jet momentum if a corrected jet type was requested
	  if (jets[i].first != 0)
	       ret += jets[i].first->pt() * jets[i].second;
     }
     return ret;
}

static FactorizedJetCorrector *jetCorrector = 0;

void setJetCorrector (FactorizedJetCorrector *jc) 
{
     if (jetCorrector != 0)
	  delete jetCorrector;
     jetCorrector = jc;
}

class FactorizedJetCorrector *makeJetCorrector (const char *l2corr, 
						const char *l3corr, 
						const char *l2l3_residual_corr)
{
     std::vector<std::string> corrs;
     corrs.reserve(3);
     corrs.push_back(l2corr);
     corrs.push_back(l3corr);
     corrs.push_back(l2l3_residual_corr);
     return makeJetCorrector(corrs);
}

class FactorizedJetCorrector *makeJetCorrector (const std::vector<std::string> &corrs)
{
     vector<JetCorrectorParameters> vParam;
     for (std::vector<std::string>::const_iterator i = corrs.begin(), i_end = corrs.end();
	  i != i_end; ++i) {
	  // do some rigmarole to evaluate env variables in the strings
	  const std::string cmd = "echo ";
	  FILE *f = popen((cmd + *i).c_str(), "r");
	  if (!f) {
	       perror((std::string("Opening pipe to execute ") + cmd + *i).c_str());
	       return 0;
	  }
	  char corr_name[1024];
	  int s = fscanf(f, " %1024s\n", corr_name);
	  if (s != 1) {
	       perror("reading file list");
	  }
	  assert(s == 1);
	  JetCorrectorParameters JetCorPar(corr_name);
	  // printf("%s\n", corr_name);
	  vParam.push_back(JetCorrectorParameters(corr_name));
     }
     return new FactorizedJetCorrector(vParam);
}

double jetCorrection (const LorentzVector &jet, FactorizedJetCorrector *jetCorrector)
{
     jetCorrector->setJetPt(jet.pt());
     jetCorrector->setJetEta(jet.eta());
     return jetCorrector->getCorrection();
}

double jetCorrection (const LorentzVector &jet)
{
     assert(jetCorrector != 0);
     jetCorrector->setJetPt(jet.pt());
     jetCorrector->setJetEta(jet.eta());
     return jetCorrector->getCorrection();
}

double jetCorrection (int ijet) 
{ 
     return jetCorrection(cms2.jets_p4()[ijet]); 
}

bool jetPassesLooseJetID(int ijet)
{
     if (fabs(cms2.jets_p4()[ijet].eta()) < 3.)
     {
	  if (cms2.jets_fHPD()[ijet] > 0.98)
	       return false;
	  if (cms2.jets_emFrac()[ijet] < 0.01)
	       return false;
	  if (cms2.jets_n90Hits()[ijet] < 2)
	       return false;
     }

     return true;
}

bool passesCaloJetID (const LorentzVector &jetp4)
{
     int jet_idx = -1;
     double minDR = 999;

     for (unsigned int i = 0; i < cms2.jets_p4().size(); i++)
     {
	  double deltaR = ROOT::Math::VectorUtil::DeltaR(jetp4, cms2.jets_p4()[i]);

	  if (deltaR < minDR)
	  {
	       minDR = deltaR;
	       jet_idx = i;
	  }
     }

     if (jet_idx < 0)
	  return false;

     if (cms2.jets_emFrac()[jet_idx] < 0.01 || cms2.jets_fHPD()[jet_idx] > 0.98 || cms2.jets_n90Hits()[jet_idx] < 2)
	  return false;

     return true;
}

bool passesPFJetID(unsigned int pfJetIdx) {

  float pfjet_chf_  = cms2.pfjets_chargedHadronE()[pfJetIdx] / cms2.pfjets_p4()[pfJetIdx].energy();
  float pfjet_nhf_  = cms2.pfjets_neutralHadronE()[pfJetIdx] / cms2.pfjets_p4()[pfJetIdx].energy();
  float pfjet_cef_  = cms2.pfjets_chargedEmE()[pfJetIdx] / cms2.pfjets_p4()[pfJetIdx].energy();
  float pfjet_nef_  = cms2.pfjets_neutralEmE()[pfJetIdx] / cms2.pfjets_p4()[pfJetIdx].energy();

  if (pfjet_nhf_ < 1. && pfjet_cef_ < 1. && pfjet_nef_ < 1.)    {
    if (fabs(cms2.pfjets_p4()[pfJetIdx].eta()) > 2.4)
      return true;
    else if (pfjet_chf_ > 0.)
      return true;
    }

  return false;
}  