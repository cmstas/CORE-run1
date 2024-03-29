// C++ Includes
#include <assert.h>
#include <iostream>
#include <algorithm>

// ROOT Includes
#include "Math/Point2Dfwd.h"
#include "Math/LorentzVector.h"
#include "Math/VectorUtil.h"
#include "TMath.h"
#include "TLorentzVector.h"
#include "TDatabasePDG.h"
#include "Math/Vector2D.h"
#include "TRandom3.h"

// CMS2 Includes
#include "electronSelections.h"
#include "electronSelectionsParameters.h"
#include "muonSelections.h"
#include "metSelections.h"
#include "ssSelections.h"
#include "jetSelections.h"
#include "trackSelections.h"
#include "MITConversionUtilities.h"
#include "triggerUtils.h"
#include "eventSelections.h"
#include "utilities.h"
#include "susySelections.h"
#include "jetcorr/FactorizedJetCorrector.h"
#include "jetcorr/JetCorrectionUncertainty.h"

using namespace wp2012;
using ROOT::Math::VectorUtil::DeltaR;

struct SortByPt 
{
    bool operator () (const LorentzVector& lhs, const LorentzVector& rhs) 
    {
        return lhs.pt() > rhs.pt();
    }

    bool operator () (const std::pair<LorentzVector, unsigned int>& lhs, const std::pair<LorentzVector, unsigned int>& rhs) 
    {
        return lhs.first.pt() > rhs.first.pt();
    }

    bool operator () (const std::pair<LorentzVector, int>& lhs, const std::pair<LorentzVector, int>& rhs) 
    {
        return lhs.first.pt() > rhs.first.pt();
    }

    bool operator () (const std::pair<LorentzVector, bool>& lhs, const std::pair<LorentzVector, bool>& rhs) 
    {
        return lhs.first.pt() > rhs.first.pt();
    }

    bool operator () (const std::pair<LorentzVector, float>& lhs, const std::pair<LorentzVector, float>& rhs) 
    {
        return lhs.first.pt() > rhs.first.pt();
    }

    bool operator () (const std::pair<int, int>& lhs, const std::pair<int, int>& rhs) 
    {
        const LorentzVector& lhs_p4 = (abs(lhs.first)==11 ? tas::els_p4().at(lhs.second) : tas::mus_p4().at(lhs.second));
        const LorentzVector& rhs_p4 = (abs(rhs.first)==11 ? tas::els_p4().at(rhs.second) : tas::mus_p4().at(rhs.second));
        return lhs_p4.pt() > rhs_p4.pt();
    }
};


/////////////////////////////////////////////////////////////////
///                                                           ///
///                                                           ///
///                                                           ///
///          2012 Selections                                  ///
///                                                           ///
///                                                           ///
///                                                           ///
/////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////     
// 2012 good lepton (passes ID)
////////////////////////////////////////////////////////////////////////////////////////////     
bool samesign::isGoodLepton(int id, int idx, bool use_el_eta)
{
    // transverse IP
    const float d0 = leptonD0(id, idx);

    // electrons
    if (abs(id) == 11)
    {
        // tightened |d0| cut wrt standard ID cut
        if (fabs(d0) > 0.01) // 100 microns (units are cm)
        {
            return false;
        }

        if (use_el_eta)
        {
            cuts_t cuts_passed = electronSelection(idx, /*applyAlignmentCorrection=*/false, /*removedEtaCutInEndcap=*/false, /*useGsfTrack=*/true); 
            electronIdComponent_t answer_med_2012 = electronId_WP2012_noIso_useElEtaForIsEB(idx, MEDIUM);
            if ((answer_med_2012 & PassWP2012CutsNoIso) == PassWP2012CutsNoIso) 
            {
                cuts_passed |= (1ll<<ELEID_WP2012_MEDIUM_NOISO);
            }
            if ((cuts_passed & electronSelection_ssV7_noIso) != electronSelection_ssV7_noIso) 
            {
                return false;
            }
            if (fabs(cms2.els_p4().at(idx).eta()) < 1.4442)
            {
                return (cms2.els_hOverE().at(idx) < 0.10);
            }
            else
            {
                return (cms2.els_hOverE().at(idx) < 0.075);
            }
        }
        else 
        {
            if (!pass_electronSelection(idx, electronSelection_ssV7_noIso, false, false))
            {
                return false;
            }
            if ((cms2.els_fiduciality().at(idx) & (1<<ISEB)) == (1<<ISEB))
            {
                return (cms2.els_hOverE().at(idx) < 0.10);
            }
            else
            {
                return (cms2.els_hOverE().at(idx) < 0.075);
            }
        }

    }

    // muons
    if (abs(id) == 13)
    {
        // tightened |d0| cut wrt standard ID cut
        if (fabs(d0) > 0.005) // 50 microns (units are cm)
        {
            return false;
        }

        return (muonIdNotIsolated(idx, NominalSSv5));
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////     
// 2012 lepton impact parameters 
// uses CTF track for muons and GSF tracks for elections
// if no matching track found, return bogus value of -999999
// calc w.r.t first good vertex
////////////////////////////////////////////////////////////////////////////////////////////     
float samesign::leptonD0(const int id, const int idx)
{
    const int vtxidx = firstGoodVertex();
    if (vtxidx < 0)
    {
        std::cout << "[samesign::leptonD0] WARNING - first good vertex index < 0.  Returning bogus value 999999" << std::endl;
        return 999999.0;
    }
    if (abs(id)==13)
    {
        const int trkidx = cms2.mus_trkidx().at(idx);
        if (trkidx >= 0)
        {
            return trks_d0_pv(trkidx, vtxidx).first;
        }
    }
    else if (abs(id)==11)
    {
        const int gsfidx = cms2.els_gsftrkidx().at(idx);
        if (gsfidx >= 0) 
        {
            return gsftrks_d0_pv(gsfidx, vtxidx).first;
        }
    }

    // return bogus for non electon/muon
    return -999999.0;
}

float samesign::leptonDz(int id, int idx)
{
    const int vtxidx = firstGoodVertex();
    if (vtxidx < 0)
    {
        std::cout << "[samesign::leptonDz] WARNING - first good vertex index < 0.  Returning bogus value 999999" << std::endl;
        return 999999.0;
    }
    if (abs(id)==13)
    {
        const int trkidx = cms2.mus_trkidx().at(idx);
        if (trkidx >= 0)
        {
            return trks_dz_pv(trkidx, vtxidx).first;
        }
    }
    else if (abs(id)==11)
    {
        const int gsfidx = cms2.els_gsftrkidx().at(idx);
        if (gsfidx >= 0)
        {
            return gsftrks_dz_pv(gsfidx, vtxidx).first;
        }
    }

    // return bogus for non electon/muon
    return -999999.0;
}

////////////////////////////////////////////////////////////////////////////////////////////     
// 2012 isolated lepton
////////////////////////////////////////////////////////////////////////////////////////////     
bool samesign::isIsolatedLepton(int id, int idx)
{
    // electrons
    if (abs(id) == 11)
        return (samesign::electronIsolationPF2012(idx) < 0.09);

    // muons
    if (abs(id) == 13)
        return (muonIsoValuePF2012_deltaBeta(idx) < 0.10);

    return false;
}


////////////////////////////////////////////////////////////////////////////////////////////     
// 2012 lepton isolation value
////////////////////////////////////////////////////////////////////////////////////////////     
double samesign::leptonIsolation(int id, int idx)
{
    // electrons
    if (abs(id) == 11)
    {
        return samesign::electronIsolationPF2012(idx);
    }

    // muons
    if (abs(id) == 13)
    {
        return muonIsoValuePF2012_deltaBeta(idx);
    }

    return -999999.0;
}


////////////////////////////////////////////////////////////////////////////////////////////     
// 2012 effective area 
////////////////////////////////////////////////////////////////////////////////////////////     

float samesign::EffectiveArea03(int id, int idx)
{
    if (abs(id)!=11)
        return -999990.0;

    // use SC eta
    float eta = fabs(cms2.els_etaSC().at(idx));

    // get effective area from electronSelections.h
    //return fastJetEffArea03_v1(eta);  // used for HCP and ICHEP
    return fastJetEffArea03_v2(eta);    // 2013
}

float samesign::EffectiveArea04(int id, int idx)
{
    if (abs(id)!=11)
        return -999990.0;

    // use SC eta
    float eta = fabs(cms2.els_etaSC().at(idx));

    // get effective area from electronSelections.h
    //return fastJetEffArea04_v1(eta);  // used for HCP and ICHEP
    return fastJetEffArea04_v2(eta);    // 2013
}


////////////////////////////////////////////////////////////////////////////////////////////     
// 2012 numerator lepton (passes ID and isolation)
////////////////////////////////////////////////////////////////////////////////////////////     
bool samesign::isNumeratorLepton(int id, int idx, bool use_el_eta)
{
    return (samesign::isGoodLepton(id, idx, use_el_eta) && samesign::isIsolatedLepton(id, idx));
}


////////////////////////////////////////////////////////////////////////////////////////////     
// 2012 numerator hypothesis (passes ID and isolation)
////////////////////////////////////////////////////////////////////////////////////////////     
bool samesign::isNumeratorHypothesis(int idx, bool use_el_eta)
{
    if (!samesign::isNumeratorLepton(cms2.hyp_lt_id().at(idx), cms2.hyp_lt_index().at(idx), use_el_eta))
    {
        return false;
    }
    if (!samesign::isNumeratorLepton(cms2.hyp_ll_id().at(idx), cms2.hyp_ll_index().at(idx), use_el_eta))
    {
        return false;
    }

    return true;
}


////////////////////////////////////////////////////////////////////////////////////////////     
// 2012 denominator lepton (relaxed ID and Isolation)
////////////////////////////////////////////////////////////////////////////////////////////     
bool samesign::isDenominatorLepton(int id, int idx, bool use_el_eta)
{
    // electrons
    if (abs(id) == 11)
    {
        if (use_el_eta)
        {
            // check the id
            cuts_t cuts_passed = electronSelection(idx, /*applyAlignmentCorrection=*/false, /*removedEtaCutInEndcap=*/false, /*useGsfTrack=*/true); 
            electronIdComponent_t answer_med_2012 = electronId_WP2012_noIso_useElEtaForIsEB(idx, MEDIUM);
            if ((answer_med_2012 & PassWP2012CutsNoIso) == PassWP2012CutsNoIso) 
            {
                cuts_passed |= (1ll<<ELEID_WP2012_MEDIUM_NOISO_NOIP);
            }
            if ((cuts_passed & electronSelectionFOV7_v3) != electronSelectionFOV7_v3) 
            {
                return false;
            }

            // check the isolation
            if (samesign::electronIsolationPF2012(idx) > 0.60)
            {
                return false;
            } 

            // passes if we get here
            return true;
        }
        else
        {
            return (pass_electronSelection(idx, electronSelectionFOV7_v3) && samesign::electronIsolationPF2012(idx) < 0.60);
        }
    }

    // muons
    if (abs(id) == 13)
    {
        return (muonId(idx, muonSelectionFO_ssV5));
    }

    return false;
}


////////////////////////////////////////////////////////////////////////////////////////////     
// 2012 denominator hypothesis (relaxed ID and Isolation)
////////////////////////////////////////////////////////////////////////////////////////////     
bool samesign::isDenominatorHypothesis(int idx, bool use_el_eta)
{
    if (!samesign::isDenominatorLepton(cms2.hyp_lt_id().at(idx), cms2.hyp_lt_index().at(idx), use_el_eta))
        return false;
    if (!samesign::isDenominatorLepton(cms2.hyp_ll_id().at(idx), cms2.hyp_ll_index().at(idx), use_el_eta))
        return false;

    return true;
}


///////////////////////////////////////////////////////////////////////////////////////////
// require electron GSF, CTF and SC charges agree
///////////////////////////////////////////////////////////////////////////////////////////
bool samesign::passThreeChargeRequirement(int elIdx)
{
    int trk_idx = cms2.els_trkidx().at(elIdx);

    if (trk_idx >= 0)
    {
        if (cms2.els_sccharge().at(elIdx) == cms2.els_trk_charge().at(elIdx) && cms2.els_trk_charge().at(elIdx) == cms2.trks_charge().at(trk_idx))             
            return true;
    }

    return false;
}


///////////////////////////////////////////////////////////////////////////////////////////
// calculate PF-based isolation for electrons with rho*Aeff correction
///////////////////////////////////////////////////////////////////////////////////////////
float samesign::electronIsolationPF2012(int idx)
{
    return samesign::electronIsolationPF2012_cone03(idx);
}

float samesign::electronIsolationPF2012_cone03(int idx)
{
    // electron pT
    const float pt = cms2.els_p4().at(idx).pt();

    // get effective area
    const float AEff = EffectiveArea03(11, idx);

    // pf iso
    // calculate from the ntuple for now...
#ifdef SS_USE_OLD_ISO // for old 52X ntuples
    const float pfiso_ch = cms2.els_iso03_pf2012_ch().at(idx);
    const float pfiso_em = cms2.els_iso03_pf2012_em().at(idx);
    const float pfiso_nh = cms2.els_iso03_pf2012_nh().at(idx);
#else
    const float pfiso_ch = cms2.els_iso03_pf2012ext_ch().at(idx);
    const float pfiso_em = cms2.els_iso03_pf2012ext_em().at(idx);
    const float pfiso_nh = cms2.els_iso03_pf2012ext_nh().at(idx);
#endif

    // rho
    const float rhoPrime = std::max(cms2.evt_kt6pf_foregiso_rho(), 0.0f);
    const float pfiso_n = std::max(pfiso_em + pfiso_nh - rhoPrime * AEff, 0.0f);
    const float pfiso = (pfiso_ch + pfiso_n) / pt;

    return pfiso;
}

float samesign::electronIsolationPF2012_cone04(int idx)
{
    // electron pT
    const float pt = cms2.els_p4().at(idx).pt();

    // get effective area
    const float AEff = EffectiveArea04(11, idx);

    // pf iso
    // calculate from the ntuple for now...
#ifdef SS_USE_OLD_ISO // for 52X 
    const float pfiso_ch = cms2.els_iso04_pf2012_ch().at(idx);
    const float pfiso_em = cms2.els_iso04_pf2012_em().at(idx);
    const float pfiso_nh = cms2.els_iso04_pf2012_nh().at(idx);
#else
    const float pfiso_ch = cms2.els_iso04_pf2012ext_ch().at(idx);
    const float pfiso_em = cms2.els_iso04_pf2012ext_em().at(idx);
    const float pfiso_nh = cms2.els_iso04_pf2012ext_nh().at(idx);
#endif

    // rho
    const float rhoPrime = std::max(cms2.evt_kt6pf_foregiso_rho(), 0.0f);
    const float pfiso_n = std::max(pfiso_em + pfiso_nh - rhoPrime * AEff, 0.0f);
    const float pfiso = (pfiso_ch + pfiso_n) / pt;

    return pfiso;
}


///////////////////////////////////////////////////////////////////////////////////////////
// passes dilepton trigger
///////////////////////////////////////////////////////////////////////////////////////////

// analysis type:
//   0 --> use high pT analysis triggers
//   1 --> use low pT analysis triggers
//   2 --> use very low pT analysis triggers
//   anything else will return false

bool samesign::passesTrigger(int hyp_type, int analysis_type)
{
    //----------------------------------------
    // no trigger requirements applied to MC
    //----------------------------------------

    if (!cms2.evt_isRealData())
        return true; 

    switch(analysis_type)
    {
    case 0: return passesTriggerHighPt(hyp_type); break;
    case 1: return passesTriggerLowPt(hyp_type); break;
    case 2: return passesTriggerVeryLowPt(hyp_type); break;
    default: return false;
    }

    return false;
}

bool samesign::passesTriggerHighPt(int hyp_type)
{
    //----------------------------------------
    // no trigger requirements applied to MC
    //----------------------------------------

    if (!cms2.evt_isRealData())
        return true; 

    //---------------------------------
    // triggers for dilepton datasets
    //---------------------------------

    // mm
    if (hyp_type == 0) {
        if (passUnprescaledHLTTriggerPattern("HLT_Mu17_Mu8_v")) {return true;}
    }

    // em
    else if ((hyp_type == 1 || hyp_type == 2)) {
        if (passUnprescaledHLTTriggerPattern("HLT_Mu17_Ele8_CaloIdT_CaloIsoVL_TrkIdVL_TrkIsoVL_v")) {return true;}
        if (passUnprescaledHLTTriggerPattern("HLT_Mu8_Ele17_CaloIdT_CaloIsoVL_TrkIdVL_TrkIsoVL_v")) {return true;}
    }

    // ee
    else if (hyp_type == 3) {
        if (passUnprescaledHLTTriggerPattern("HLT_Ele17_CaloIdT_CaloIsoVL_TrkIdVL_TrkIsoVL_Ele8_CaloIdT_CaloIsoVL_TrkIdVL_TrkIsoVL_v")) {return true;}
    }

    return false;
}

bool samesign::passesTriggerLowPt(int hyp_type)
{
    //----------------------------------------
    // no trigger requirements applied to MC
    //----------------------------------------

    if (!cms2.evt_isRealData())
        return true; 

    //---------------------------------
    // triggers for dilepton datasets
    //---------------------------------

    // mm
    if (hyp_type == 0) {
        if (passUnprescaledHLTTriggerPattern("HLT_DoubleMu8_Mass8_PFNoPUHT175_v")) {return true;}
        if (passUnprescaledHLTTriggerPattern("HLT_DoubleMu8_Mass8_PFHT175_v"    )) {return true;}
    }

    // em
    else if ((hyp_type == 1 || hyp_type == 2)) {
        if (passUnprescaledHLTTriggerPattern("HLT_Mu8_Ele8_CaloIdT_TrkIdVL_Mass8_PFNoPUHT175_v")) {return true;}
        if (passUnprescaledHLTTriggerPattern("HLT_Mu8_Ele8_CaloIdT_TrkIdVL_Mass8_PFHT175_v"    )) {return true;}
    }

    // ee
    else if (hyp_type == 3) {
        if (passUnprescaledHLTTriggerPattern("HLT_DoubleEle8_CaloIdT_TrkIdVL_Mass8_PFNoPUHT175_v")) {return true;}
        if (passUnprescaledHLTTriggerPattern("HLT_DoubleEle8_CaloIdT_TrkIdVL_Mass8_PFHT175_v"    )) {return true;}
    }

    return false;
}

bool samesign::passesTriggerVeryLowPt(int hyp_type)
{
    //----------------------------------------
    // no trigger requirements applied to MC
    //----------------------------------------

    if (!cms2.evt_isRealData())
        return true; 

    //---------------------------------
    // triggers for dilepton datasets
    //---------------------------------

    // mm
    if (hyp_type == 0) {
        if (passUnprescaledHLTTriggerPattern("HLT_DoubleRelIso1p0Mu5_Mass8_PFNoPUHT175_v")) {return true;}
        if (passUnprescaledHLTTriggerPattern("HLT_DoubleRelIso1p0Mu5_Mass8_PFHT175_v"    )) {return true;}
    }

    // em
    else if ((hyp_type == 1 || hyp_type == 2)) {
        if (passUnprescaledHLTTriggerPattern("HLT_RelIso1p0Mu5_Ele8_CaloIdT_TrkIdVL_Mass8_PFNoPUHT175_v")) {return true;}
        if (passUnprescaledHLTTriggerPattern("HLT_RelIso1p0Mu5_Ele8_CaloIdT_TrkIdVL_Mass8_PFHT175_v"    )) {return true;}
    }

    // ee
    else if (hyp_type == 3) {
        if (passUnprescaledHLTTriggerPattern("HLT_DoubleEle8_CaloIdT_TrkIdVL_Mass8_PFNoPUHT175_v")) {return true;}
        if (passUnprescaledHLTTriggerPattern("HLT_DoubleEle8_CaloIdT_TrkIdVL_Mass8_PFHT175_v"    )) {return true;}
    }

    return false;
}


///////////////////////////////////////////////////////////////////////////////////////////
// extra Z veto for b-tagged same sign analysis
///////////////////////////////////////////////////////////////////////////////////////////
bool samesign::makesExtraZ(int idx, bool apply_id_iso) {

    std::vector<unsigned int> ele_idx;
    std::vector<unsigned int> mu_idx;

    int lt_id           = cms2.hyp_lt_id().at(idx);
    int ll_id           = cms2.hyp_ll_id().at(idx);
    unsigned int lt_idx = cms2.hyp_lt_index().at(idx);
    unsigned int ll_idx = cms2.hyp_ll_index().at(idx);

    (abs(lt_id) == 11) ? ele_idx.push_back(lt_idx) : mu_idx.push_back(lt_idx);
    (abs(ll_id) == 11) ? ele_idx.push_back(ll_idx) : mu_idx.push_back(ll_idx);

    if (ele_idx.size() + mu_idx.size() != 2) {
        std::cout << "ERROR: don't have 2 leptons in hypothesis!!!  Exiting" << std::endl;
        return false;
    }
        
    if (ele_idx.size() > 0) {
        for (unsigned int eidx = 0; eidx < cms2.els_p4().size(); eidx++) {

            bool is_hyp_lep = false;
            for (unsigned int vidx = 0; vidx < ele_idx.size(); vidx++) {
                if (eidx == ele_idx.at(vidx))
                    is_hyp_lep = true;                
            }
            if (is_hyp_lep)
                continue;

            if (fabs(cms2.els_p4().at(eidx).eta()) > 2.4)
            {
                continue;
            }

            if (cms2.els_p4().at(eidx).pt() < 10.)
            {
                continue;
            }


            if (apply_id_iso) {
                float iso_val = samesign::electronIsolationPF2012(eidx);
                if (iso_val > 0.2)
                {
                    continue;
                }
                
                electronIdComponent_t passAllVetoCuts = DETAIN | DPHIIN | SIGMAIETAIETA | HOE | D0VTX | DZVTX;
                electronIdComponent_t vetoid = electronId_WP2012(eidx, VETO);
                if ((passAllVetoCuts & vetoid) != passAllVetoCuts)
                {
                    continue;
                }
            }

            for (unsigned int vidx = 0; vidx < ele_idx.size(); vidx++) {

                if (cms2.els_charge().at(eidx) * cms2.els_charge().at(ele_idx.at(vidx)) > 0)
                {
                    continue;
                }

                LorentzVector zp4 = cms2.els_p4().at(eidx) + cms2.els_p4().at(ele_idx.at(vidx));
                float zcandmass = sqrt(fabs(zp4.mass2()));
                if (fabs(zcandmass-91.) < 15.)
                {
                    return true;
                }
            }
        }        
    }

    if (mu_idx.size() > 0) {
        for (unsigned int midx = 0; midx < cms2.mus_p4().size(); midx++) {

            bool is_hyp_lep = false;
            for (unsigned int vidx = 0; vidx < mu_idx.size(); vidx++) {
                if (midx == mu_idx.at(vidx))
                    is_hyp_lep = true;                
            }
            if (is_hyp_lep)
                continue;

            if (fabs(cms2.mus_p4().at(midx).eta()) > 2.4)
                continue;

            if (cms2.mus_p4().at(midx).pt() < 10.)
                continue;

            if (apply_id_iso) {
                float iso_val = muonIsoValuePF2012_deltaBeta(midx);
                if (iso_val > 0.2)
                    continue;
                
                if (!cms2.mus_pid_PFMuon().at(midx))
                    continue;

                bool is_global  = ((cms2.mus_type().at(midx) & (1<<1)) == (1<<1));
                bool is_tracker = ((cms2.mus_type().at(midx) & (1<<2)) == (1<<2));
                if (!is_global && !is_tracker)
                    continue;
            }

            for (unsigned int vidx = 0; vidx < mu_idx.size(); vidx++) {

                if (cms2.mus_charge().at(midx) * cms2.mus_charge().at(mu_idx.at(vidx)) > 0)
                    continue;

                LorentzVector zp4 = cms2.mus_p4().at(midx) + cms2.mus_p4().at(mu_idx.at(vidx));
                float zcandmass = sqrt(fabs(zp4.mass2()));
                if (fabs(zcandmass-91.) < 15.)
                    return true;
            }
        }
    }

    return false;
}


///////////////////////////////////////////////////////////////////////////////////////////
// Gamma* veto for b-tagged same sign analysis
///////////////////////////////////////////////////////////////////////////////////////////
bool samesign::makesExtraGammaStar(int idx, bool apply_id_iso) {

    std::vector<unsigned int> ele_idx;
    std::vector<unsigned int> mu_idx;

    int lt_id           = cms2.hyp_lt_id().at(idx);
    int ll_id           = cms2.hyp_ll_id().at(idx);
    unsigned int lt_idx = cms2.hyp_lt_index().at(idx);
    unsigned int ll_idx = cms2.hyp_ll_index().at(idx);

    (abs(lt_id) == 11) ? ele_idx.push_back(lt_idx) : mu_idx.push_back(lt_idx);
    (abs(ll_id) == 11) ? ele_idx.push_back(ll_idx) : mu_idx.push_back(ll_idx);

    if (ele_idx.size() + mu_idx.size() != 2) {
        std::cout << "ERROR: don't have 2 leptons in hypothesis!!!  Exiting" << std::endl;
        return false;
    }
        
    if (ele_idx.size() > 0) {
        for (unsigned int eidx = 0; eidx < cms2.els_p4().size(); eidx++) {

            bool is_hyp_lep = false;
            for (unsigned int vidx = 0; vidx < ele_idx.size(); vidx++) {
                if (eidx == ele_idx.at(vidx))
                    is_hyp_lep = true;                
            }
            if (is_hyp_lep)
                continue;

            if (fabs(cms2.els_p4().at(eidx).eta()) > 2.4)
            {
                continue;
            }

            if (cms2.els_p4().at(eidx).pt() < 5.0)
            {
                continue;
            }


            if (apply_id_iso) {
                float iso_val = samesign::electronIsolationPF2012(eidx);
                if (iso_val > 0.2)
                {
                    continue;
                }
                
                electronIdComponent_t passAllVetoCuts = DETAIN | DPHIIN | SIGMAIETAIETA | HOE | D0VTX | DZVTX;
                electronIdComponent_t vetoid = electronId_WP2012(eidx, VETO);
                if ((passAllVetoCuts & vetoid) != passAllVetoCuts)
                {
                    continue;
                }
            }

            for (unsigned int vidx = 0; vidx < ele_idx.size(); vidx++) {

                if (cms2.els_charge().at(eidx) * cms2.els_charge().at(ele_idx.at(vidx)) > 0)
                {
                    continue;
                }

                LorentzVector gamma_p4 = cms2.els_p4().at(eidx) + cms2.els_p4().at(ele_idx.at(vidx));
                float gammacandmass = sqrt(fabs(gamma_p4.mass2()));
                if (gammacandmass < 12.0)
                {
                    return true;
                }
            }
        }        
    }

    if (mu_idx.size() > 0) {
        for (unsigned int midx = 0; midx < cms2.mus_p4().size(); midx++) {

            bool is_hyp_lep = false;
            for (unsigned int vidx = 0; vidx < mu_idx.size(); vidx++) {
                if (midx == mu_idx.at(vidx))
                    is_hyp_lep = true;                
            }
            if (is_hyp_lep)
                continue;

            if (fabs(cms2.mus_p4().at(midx).eta()) > 2.4)
                continue;

            if (cms2.mus_p4().at(midx).pt() < 5.0)
                continue;

            if (apply_id_iso) {
                float iso_val = muonIsoValuePF2012_deltaBeta(midx);
                if (iso_val > 0.2)
                    continue;
                
                if (!cms2.mus_pid_PFMuon().at(midx))
                    continue;

                bool is_global  = ((cms2.mus_type().at(midx) & (1<<1)) == (1<<1));
                bool is_tracker = ((cms2.mus_type().at(midx) & (1<<2)) == (1<<2));
                if (!is_global && !is_tracker)
                    continue;
            }

            for (unsigned int vidx = 0; vidx < mu_idx.size(); vidx++) {

                if (cms2.mus_charge().at(midx) * cms2.mus_charge().at(mu_idx.at(vidx)) > 0)
                    continue;

                LorentzVector gamma_p4 = cms2.mus_p4().at(midx) + cms2.mus_p4().at(mu_idx.at(vidx));
                float gammacandmass = sqrt(fabs(gamma_p4.mass2()));
                if (gammacandmass < 12.0)
                {
                    return true;
                }
            }
        }
    }

    return false;
}

///////////////////////////////////////////////////////////////////////////////////////////
// additional leptons for same sign analysis
///////////////////////////////////////////////////////////////////////////////////////////

// does it pass the 3rd muon selection
// https://twiki.cern.ch/twiki/bin/view/CMSPublic/SWGuideMuonId#Tight_Muon_selection
bool passes3rdMuonSelection(const int mu_idx, const float min_lep_pt)
{
    using namespace tas;

    // return false if index is invalid
    if (mu_idx < 0)
    {
        throw std::domain_error("[samesign::passes3rdMuonSelection] ERROR - index is invalid!"); 
    }
    
    if (fabs(mus_p4().at(mu_idx).eta()) > 2.4)                   {return false;}
    if (fabs(mus_p4().at(mu_idx).pt()) < min_lep_pt)             {return false;}
    if (not passes_muid_wp2012(mu_idx, mu2012_tightness::TIGHT)) {return false;}
    if (samesign::leptonIsolation(13, mu_idx) > 0.15)            {return false;}
    if (fabs(samesign::leptonD0(13, mu_idx)) > 0.02)             {return false;} // tighted d0 cut w.r.t POG

    // if we're here, return true
    return true;
}

// does it pass the 3rd electron selection (no overlap removal)
// POG ID loose working point
// https://twiki.cern.ch/twiki/bin/view/CMS/EgammaCutBasedIdentification
bool passes3rdElectronSelectionNoOverlapRemoval(const int el_idx, const float min_lep_pt, const bool use_el_eta)
{
    using namespace tas;

    // return false if index is invalid
    if (el_idx < 0)
    {
        throw std::domain_error("[samesign::passes3rdElectronSelection] ERROR - index is invalid!"); 
    }

    // Electron ID 
    electronIdComponent_t wp2012_loose_bits = (use_el_eta ? electronId_WP2012_noIso_useElEtaForIsEB(el_idx, LOOSE) : electronId_WP2012_v3(el_idx, LOOSE));
    if ((wp2012_loose_bits & wp2012::PassWP2012CutsNoIso) != wp2012::PassWP2012CutsNoIso)
    {
        return false;
    }

    // electron selections
    if (fabs(els_p4().at(el_idx).eta()) > 2.4)                                         {return false;}
    if (fabs(els_p4().at(el_idx).pt()) < min_lep_pt)                                   {return false;}
    if (1.4442 < fabs(els_etaSC().at(el_idx)) && fabs(els_etaSC().at(el_idx)) < 1.566) {return false;}
    if (samesign::leptonIsolation(11, el_idx) > 0.15)                                  {return false;} 

    // if we're here, return true
    return true;
}

// check that the electron overlaps with a selection muon
bool electronOverlapsMuon(const LorentzVector& el_p4, const std::vector<unsigned int>& mu_indices)
{
    using namespace tas;

    for (size_t midx = 0; midx < mus_p4().size(); midx++)
    {
        // skip hyp muons
        if (std::find(mu_indices.begin(), mu_indices.end(), midx) != mu_indices.end()) {continue;}

        if (not passes3rdMuonSelection(midx, /*min_pt=*/10.0)) {continue;   }
        if (DeltaR(el_p4, mus_p4().at(midx)) < 0.1)            {return true;}
    }

    // if we got here, then none found
    return false;
}

// does it pass the 3rd lepton selection
bool passes3rdLeptonSelection(const int lep_id, const int lep_idx, const float min_lep_pt, const std::vector<unsigned int>& mu_indices)
{
    using namespace tas;

    // return false if index is invalid
    if (lep_idx < 0)
    {
        throw std::domain_error("[samesign::passes3rdLeptonSelection] ERROR - index is invalid!"); 
    }

    // electron selections
    // POG ID loose working point
    // https://twiki.cern.ch/twiki/bin/view/CMS/EgammaCutBasedIdentification
    if (abs(lep_id)==11)
    {
        if (not passes3rdElectronSelectionNoOverlapRemoval(lep_idx, min_lep_pt, /*use_el_eta=*/false)) {return false;}
        if (electronOverlapsMuon(els_p4().at(lep_idx), mu_indices))                                   {return false;} 
    }

    // muon selections
    if (abs(lep_id)==13)
    {
        if (not passes3rdMuonSelection(lep_idx, min_lep_pt)) {return false;}
    }

    // if we're here, return true
    return true;
}

// additional selected leptons 
std::vector<std::pair<int, int> > samesign::additionalLeptons(const int hyp_idx, const float min_lep_pt)
{
    using namespace tas;

    std::vector<unsigned int> el_indices;
    std::vector<unsigned int> mu_indices;

    const int lt_id           = hyp_lt_id().at(hyp_idx);
    const int ll_id           = hyp_ll_id().at(hyp_idx);
    const unsigned int lt_idx = hyp_lt_index().at(hyp_idx);
    const unsigned int ll_idx = hyp_ll_index().at(hyp_idx);

    (abs(lt_id) == 11) ? el_indices.push_back(lt_idx) : mu_indices.push_back(lt_idx);
    (abs(ll_id) == 11) ? el_indices.push_back(ll_idx) : mu_indices.push_back(ll_idx);
    if (el_indices.size() + mu_indices.size() != 2)
    {
        throw std::runtime_error("[samesign::highestPtAdditionalLepton] ERROR: don't have 2 leptons in hypothesis!!!");
    }

    // selected leptons 
    std::vector<std::pair<int, int> > selected_leps;

    // loop over electrons
    for (size_t eidx = 0; eidx != els_p4().size(); eidx++)
    {
        // skip hyp electrons
        if (std::find(el_indices.begin(), el_indices.end(), eidx) != el_indices.end()) {continue;}

        // return true if this electron passes the 3rd lepton selection
        if (passes3rdLeptonSelection(11, eidx, min_lep_pt, mu_indices))
        {
            selected_leps.push_back(std::make_pair(-11 * els_charge().at(eidx), eidx));
        }
    }
        
    // loop over muons
    for (size_t midx = 0; midx != mus_p4().size(); midx++)
    {
        // skip hyp muons
        if (std::find(mu_indices.begin(), mu_indices.end(), midx) != mu_indices.end()) {continue;}

        // return true if this muons passes the 3rd lepton selection
        if (passes3rdLeptonSelection(13, midx, min_lep_pt, mu_indices))
        {
            selected_leps.push_back(std::make_pair(-13 * mus_charge().at(midx), midx));
        }
    }

    // sort by pt
    std::sort(selected_leps.begin(), selected_leps.end(), SortByPt());
        
    // if we got here, then we didn't find one
    return selected_leps;
}


///////////////////////////////////////////////////////////////////////////////////////////
// highest pT additional lepton for same sign analysis
///////////////////////////////////////////////////////////////////////////////////////////
std::pair<int, int> samesign::highestPtAdditionalLepton(const int idx, const float min_lep_pt)
{
    std::vector<std::pair<int, int> > leps = samesign::additionalLeptons(idx, min_lep_pt);
    if (not leps.empty()) {return leps.front();}
    return std::make_pair(-999999, -999999);
}


///////////////////////////////////////////////////////////////////////////////////////////
// 3rd lepton veto for same sign analysis
///////////////////////////////////////////////////////////////////////////////////////////
bool samesign::has3rdLepton(const int idx, const float min_lep_pt)
{
    return (not additionalLeptons(idx, min_lep_pt).empty());
}


////////////////////////////////////////////////////////////////////////////////////////////////
// 2012 get jets and perform overlap removal with numerator e/mu with pt > x (defaults are 20/20 GeV)
////////////////////////////////////////////////////////////////////////////////////////////////
    
// JEC taken from ntuple
std::vector<LorentzVector> samesign::getJets(int idx, enum JetType type, float deltaR, float min_pt, float max_eta, float mu_minpt, float ele_minpt, float rescale, int systFlag, bool sort_by_pt) {

    std::vector<LorentzVector> tmp_jets = getJets(idx, /*sort=*/sort_by_pt, type, JETS_CLEAN_HYP_E_MU, deltaR, 0.0, max_eta, (double) rescale, systFlag);

    // ok, now perform the rest of the lepton overlap removal
    // and the impose the pt requirement after applying the
    // extra corrections
    std::vector<LorentzVector> final_jets;
    for (unsigned int jidx = 0; jidx < tmp_jets.size(); jidx++) {

        bool jetIsLep = false;

        LorentzVector vjet = tmp_jets.at(jidx);
        if (vjet.pt() < min_pt)
            continue;

        for (unsigned int eidx = 0; eidx < cms2.els_p4().size(); eidx++) {
            if (cms2.els_p4().at(eidx).pt() < ele_minpt)
                continue;
            if (!samesign::isNumeratorLepton(11, eidx))
                continue;

            if (ROOT::Math::VectorUtil::DeltaR(vjet, cms2.els_p4().at(eidx)) > deltaR)
                continue;

            jetIsLep = true;
            break;
        }

        if (jetIsLep) continue;

        for (unsigned int midx = 0; midx < cms2.mus_p4().size(); midx++) {
            if (cms2.mus_p4().at(midx).pt() < mu_minpt)
                continue;
            if (!samesign::isNumeratorLepton(13, midx))
                continue;

            if (ROOT::Math::VectorUtil::DeltaR(vjet, cms2.mus_p4().at(midx)) > deltaR)
                continue;                

            jetIsLep = true;
            break;
        }            

        if (jetIsLep) continue;

        final_jets.push_back(vjet);
    }

    if (sort_by_pt)
        sort(final_jets.begin(), final_jets.end(), SortByPt());
    return final_jets;    
}


// JEC applied otf
std::vector<LorentzVector> samesign::getJets(int idx, FactorizedJetCorrector* jet_corrector, enum JetType type, float deltaR, float min_pt, float max_eta, float mu_minpt, float ele_minpt, float rescale, int systFlag, bool sort_by_pt)
{
    std::vector<LorentzVector> final_jets;

    if (type != JETS_TYPE_PF_FAST_CORR_RESIDUAL && type != JETS_TYPE_PF_FAST_CORR && type != JETS_TYPE_PF_CORR && type != JETS_TYPE_PF_UNCORR)
    {
        std::cout << "Only particle flow jets are supported for use with a jet corrector!  Exiting." << std::endl;
        return final_jets;
    }

    std::vector<bool> tmp_jet_flags = samesign::getJetFlags(idx, type, deltaR, 0.0, max_eta, mu_minpt, ele_minpt, rescale, systFlag);

    // now impose the pt requirement after applying the extra corrections
    for (unsigned int jidx = 0; jidx < tmp_jet_flags.size(); jidx++) {

        if (!tmp_jet_flags.at(jidx))
            continue;

        LorentzVector vjet = cms2.pfjets_p4().at(jidx);
        jet_corrector->setRho(cms2.evt_ww_rho_vor());
        jet_corrector->setJetA(cms2.pfjets_area().at(jidx));
        jet_corrector->setJetPt(cms2.pfjets_p4().at(jidx).pt());
        jet_corrector->setJetEta(cms2.pfjets_p4().at(jidx).eta());        
        float jet_cor = jet_corrector->getCorrection();
        vjet *= jet_cor * rescale;
        if (systFlag != 0)
        {
            float c = getJetMetSyst(systFlag, vjet.pt(), vjet.eta());
            vjet *= c;
        }
        if (vjet.pt() < min_pt)
        {
            continue;
        }

        final_jets.push_back(vjet);
    }

    if (sort_by_pt)
        sort(final_jets.begin(), final_jets.end(), SortByPt());
    return final_jets;
}

// JEC uncertainty applied otf
std::vector<LorentzVector> samesign::getJets(int idx, JetCorrectionUncertainty *jet_unc, enum JetScaleType scale_type, enum JetType type, float deltaR, float min_pt, float max_eta, float mu_minpt, float ele_minpt, bool sort_by_pt)
{    
    std::vector<bool> tmp_jet_flags = samesign::getJetFlags(idx, type, deltaR, 0.0, max_eta, mu_minpt, ele_minpt);

    // now impose the pt requirement after applying the extra corrections
    std::vector<LorentzVector> final_jets;
    for (unsigned int jidx = 0; jidx < tmp_jet_flags.size(); jidx++) {

        if (!tmp_jet_flags.at(jidx))
            continue;

        const float jet_cor = (cms2.evt_isRealData() ? cms2.pfjets_corL1FastL2L3residual().at(jidx) :  cms2.pfjets_corL1FastL2L3().at(jidx));
        LorentzVector vjet  = cms2.pfjets_p4().at(jidx) * jet_cor; 
        jet_unc->setJetPt(vjet.pt());    
        jet_unc->setJetEta(vjet.eta());  
        const float jet_cor_unc = jet_unc->getUncertainty(true);     
        vjet *= (1.0 + jet_cor_unc * scale_type);    
        if (vjet.pt() < min_pt)
        {
            continue;
        }

        final_jets.push_back(vjet);
    }

    if (sort_by_pt)
        sort(final_jets.begin(), final_jets.end(), SortByPt());
    return final_jets;
}

// JEC AND JEC uncertainty applied otf
std::vector<LorentzVector> samesign::getJets(int idx, FactorizedJetCorrector* jet_corrector, JetCorrectionUncertainty *jet_unc, enum JetScaleType scale_type,  enum JetType type, float deltaR, float min_pt, float max_eta, float mu_minpt, float ele_minpt, bool sort_by_pt)
{    
    std::vector<bool> tmp_jet_flags = samesign::getJetFlags(idx, type, deltaR, 0.0, max_eta, mu_minpt, ele_minpt);

    // now impose the pt requirement after applying the extra corrections
    std::vector<LorentzVector> final_jets;
    for (unsigned int jidx = 0; jidx < tmp_jet_flags.size(); jidx++) {

        if (!tmp_jet_flags.at(jidx))
            continue;

        jet_corrector->setRho(cms2.evt_ww_rho_vor());
        jet_corrector->setJetA(cms2.pfjets_area().at(jidx));
        jet_corrector->setJetPt(cms2.pfjets_p4().at(jidx).pt());
        jet_corrector->setJetEta(cms2.pfjets_p4().at(jidx).eta());        
        const float jet_cor = jet_corrector->getCorrection();
        LorentzVector vjet = cms2.pfjets_p4().at(jidx) * jet_cor;
        jet_unc->setJetPt(vjet.pt());    
        jet_unc->setJetEta(vjet.eta());  
        const float jet_cor_unc = jet_unc->getUncertainty(true);     
        vjet *= (1.0 + jet_cor_unc * scale_type);    
        if (vjet.pt() < min_pt)
        {
            continue;
        }

        final_jets.push_back(vjet);
    }
     
    if (sort_by_pt)
        sort(final_jets.begin(), final_jets.end(), SortByPt());  
    return final_jets;   
}    
     
////////////////////////////////////////////////////////////////////////////////////////////////
// 2012 get all the jets with corrected energy 
////////////////////////////////////////////////////////////////////////////////////////////////
    
// JEC taken from ntuple
std::vector<LorentzVector> samesign::getAllCorrectedJets(enum JetType type, const int systFlag, bool sort_by_pt)
{
    // corrections 
    std::vector<LorentzVector> temp_vjets;
    for (unsigned int jidx = 0; jidx < cms2.pfjets_p4().size(); jidx++)
    {
        const float jet_cor = (cms2.evt_isRealData() ? cms2.pfjets_corL1FastL2L3residual().at(jidx) :  cms2.pfjets_corL1FastL2L3().at(jidx));
        LorentzVector vjet  = cms2.pfjets_p4().at(jidx) * jet_cor; 
        if (systFlag != 0) 
        {
            float c = getJetMetSyst(systFlag, vjet.pt(), vjet.eta());
            vjet *= c;
        }
        temp_vjets.push_back(vjet);
    }
    return temp_vjets;    
}


//JEC applied otf
std::vector<LorentzVector> samesign::getAllCorrectedJets(enum JetType type, FactorizedJetCorrector* jet_corrector, const int systFlag, bool sort_by_pt)
{
    // now impose the pt requirement after applying the extra corrections
    std::vector<LorentzVector> temp_vjets;
    for (unsigned int jidx = 0; jidx < cms2.pfjets_p4().size(); jidx++)
    {
        jet_corrector->setRho(cms2.evt_ww_rho_vor());
        jet_corrector->setJetA(cms2.pfjets_area().at(jidx));
        jet_corrector->setJetPt(cms2.pfjets_p4().at(jidx).pt());
        jet_corrector->setJetEta(cms2.pfjets_p4().at(jidx).eta());        
        float jet_cor = jet_corrector->getCorrection();
        LorentzVector vjet = cms2.pfjets_p4().at(jidx) * jet_cor;
        if (systFlag != 0) 
        {
            float c = getJetMetSyst(systFlag, vjet.pt(), vjet.eta());
            vjet *= c;
        }
        temp_vjets.push_back(vjet);
    }
    return temp_vjets;    
}


//JEC uncertainty applied otf
std::vector<LorentzVector> samesign::getAllCorrectedJets(enum JetType type, JetCorrectionUncertainty *jet_unc, enum JetScaleType scale_type, bool sort_by_pt)
{
    // now impose the pt requirement after applying the extra corrections
    std::vector<LorentzVector> temp_vjets;
    for (unsigned int jidx = 0; jidx < cms2.pfjets_p4().size(); jidx++)
    {
        const float jet_cor = (cms2.evt_isRealData() ? cms2.pfjets_corL1FastL2L3residual().at(jidx) :  cms2.pfjets_corL1FastL2L3().at(jidx));
        LorentzVector vjet  = cms2.pfjets_p4().at(jidx) * jet_cor; 
        jet_unc->setJetPt(vjet.pt());    
        jet_unc->setJetEta(vjet.eta() > 0.0f ? (std::min(vjet.eta(), 5.1999f)) : (std::max(vjet.eta(), -5.1999f)));  
        const float jet_cor_unc = jet_unc->getUncertainty(true);     
        vjet *= (1.0 + jet_cor_unc * scale_type);    
        temp_vjets.push_back(vjet);
    }
    return temp_vjets;    
}


//JEC AND JEC uncertainty applied otf
std::vector<LorentzVector> samesign::getAllCorrectedJets(enum JetType type, FactorizedJetCorrector* jet_corrector, JetCorrectionUncertainty *jet_unc, enum JetScaleType scale_type, bool sort_by_pt)
{
    // now impose the pt requirement after applying the extra corrections
    std::vector<LorentzVector> temp_vjets;
    for (unsigned int jidx = 0; jidx < cms2.pfjets_p4().size(); jidx++)
    {
        jet_corrector->setRho(cms2.evt_ww_rho_vor());
        jet_corrector->setJetA(cms2.pfjets_area().at(jidx));
        jet_corrector->setJetPt(cms2.pfjets_p4().at(jidx).pt());
        jet_corrector->setJetEta(cms2.pfjets_p4().at(jidx).eta());        
        float jet_cor = jet_corrector->getCorrection();
        LorentzVector vjet = cms2.pfjets_p4().at(jidx) * jet_cor;
        jet_unc->setJetPt(vjet.pt());    
        jet_unc->setJetEta(vjet.eta() > 0.0f ? (std::min(vjet.eta(), 5.1999f)) : (std::max(vjet.eta(), -5.1999f)));  
        const float jet_cor_unc = jet_unc->getUncertainty(true);     
        vjet *= (1.0 + jet_cor_unc * scale_type);    
        temp_vjets.push_back(vjet);
    }
    return temp_vjets;    
}


///////////////////////////////////////////////////////////////////////////////////////////
// 2012 get jet flags and perform overlap removal with numerator e/mu with pt > x (defaults are 20/20 GeV)
///////////////////////////////////////////////////////////////////////////////////////////

// JEC taken from ntuple
std::vector<bool> samesign::getJetFlags(int idx, enum JetType type, float deltaR, float min_pt, float max_eta, float mu_minpt, float ele_minpt, float rescale, int systFlag)
{
    std::vector<bool> tmp_jet_flags = getJetFlags((unsigned int)idx, type, JETS_CLEAN_HYP_E_MU, (double)deltaR, 0.0, (double)max_eta, (double)rescale, systFlag);

    // ok, now perform the rest of the lepton overlap removal
    // and the impose the pt requirement after applying the
    // extra corrections
    std::vector<bool> final_jets;
    for (unsigned int jidx = 0; jidx < tmp_jet_flags.size(); jidx++) {

        if (!tmp_jet_flags.at(jidx)) {
            final_jets.push_back(tmp_jet_flags.at(jidx));
            continue;
        }

        const float jet_cor = (cms2.evt_isRealData() ? cms2.pfjets_corL1FastL2L3residual().at(jidx) :  cms2.pfjets_corL1FastL2L3().at(jidx));
        LorentzVector vjet  = cms2.pfjets_p4().at(jidx) * jet_cor * rescale; 
        if (systFlag != 0) {
            float c = getJetMetSyst(systFlag, vjet.pt(), vjet.eta());
            vjet *= c;
        }
        if (vjet.pt() < min_pt) {
            final_jets.push_back(false);
            continue;
        }

        bool jetIsLep = false;

        for (unsigned int eidx = 0; eidx < cms2.els_p4().size(); eidx++) {
            if (cms2.els_p4().at(eidx).pt() < ele_minpt)
                continue;
            if (!samesign::isNumeratorLepton(11, eidx))
                continue;

            if (ROOT::Math::VectorUtil::DeltaR(vjet, cms2.els_p4().at(eidx)) > deltaR)
                continue;

            jetIsLep = true;
            break;
        }

        if (jetIsLep) {
            final_jets.push_back(false);
            continue;
        }

        for (unsigned int midx = 0; midx < cms2.mus_p4().size(); midx++) {
            if (cms2.mus_p4().at(midx).pt() < mu_minpt)
                continue;
            if (!samesign::isNumeratorLepton(13, midx))
                continue;

            if (ROOT::Math::VectorUtil::DeltaR(vjet, cms2.mus_p4().at(midx)) > deltaR)
                continue;                

            jetIsLep = true;
            break;
        }            

        if (jetIsLep) {
            final_jets.push_back(false);
            continue;   
        }
        final_jets.push_back(true);
    }

    return final_jets;    
}


// JEC applied otf
std::vector<bool> samesign::getJetFlags(int idx, FactorizedJetCorrector* jet_corrector, enum JetType type, float deltaR, float min_pt, float max_eta, float mu_minpt, float ele_minpt, float rescale, int systFlag)
{
    std::vector<bool> tmp_jet_flags = samesign::getJetFlags(idx, type, deltaR, 0.0, max_eta, mu_minpt, ele_minpt, rescale, systFlag);

    assert(tmp_jet_flags.size() == cms2.pfjets_p4().size());

    // now impose the pt requirement after applying the extra corrections
    std::vector<bool> final_jets;
    for (unsigned int jidx = 0; jidx < tmp_jet_flags.size(); jidx++) {

        if (!tmp_jet_flags.at(jidx)) {
            final_jets.push_back(tmp_jet_flags.at(jidx));
            continue;
        }

        jet_corrector->setRho(cms2.evt_ww_rho_vor());
        jet_corrector->setJetA(cms2.pfjets_area().at(jidx));
        jet_corrector->setJetPt(cms2.pfjets_p4().at(jidx).pt());
        jet_corrector->setJetEta(cms2.pfjets_p4().at(jidx).eta());        
        float jet_cor = jet_corrector->getCorrection();
        LorentzVector vjet = cms2.pfjets_p4().at(jidx) * jet_cor * rescale;
        if (systFlag != 0) 
        {
            float c = getJetMetSyst(systFlag, vjet.pt(), vjet.eta());
            vjet *= c;
        }
        if (vjet.pt() < min_pt) {
            final_jets.push_back(false);
            continue;   
        }

        final_jets.push_back(true);
    }

    return final_jets;    
}


// JEC uncertainty applied otf
std::vector<bool> samesign::getJetFlags(int idx, JetCorrectionUncertainty *jet_unc, enum JetScaleType scale_type, enum JetType type, float deltaR, float min_pt, float max_eta, float mu_minpt, float ele_minpt)     
{
    std::vector<bool> tmp_jet_flags = samesign::getJetFlags(idx, type, deltaR, 0.0, max_eta, mu_minpt, ele_minpt);

    assert(tmp_jet_flags.size() == cms2.pfjets_p4().size());

    // now impose the pt requirement after applying the extra corrections
    std::vector<bool> final_jets;
    for (unsigned int jidx = 0; jidx < tmp_jet_flags.size(); jidx++) {

        if (!tmp_jet_flags.at(jidx)) {
            final_jets.push_back(tmp_jet_flags.at(jidx));
            continue;
        }
        const float jet_cor = (cms2.evt_isRealData() ? cms2.pfjets_corL1FastL2L3residual().at(jidx) :  cms2.pfjets_corL1FastL2L3().at(jidx));
        LorentzVector vjet  = cms2.pfjets_p4().at(jidx) * jet_cor; 
        jet_unc->setJetPt(vjet.pt());    
        jet_unc->setJetEta(vjet.eta());  
        const float jet_cor_unc = jet_unc->getUncertainty(true);     
        vjet *= (1.0 + jet_cor_unc * scale_type);    
        if (vjet.pt() < min_pt) {
            final_jets.push_back(false);
            continue;   
        }

        final_jets.push_back(true);
    }

    return final_jets;    
}


// JEC AND JEC uncertainty applied otf
std::vector<bool> samesign::getJetFlags(int idx, FactorizedJetCorrector* jet_corrector, JetCorrectionUncertainty *jet_unc, enum JetScaleType scale_type,  enum JetType type, float deltaR, float min_pt, float max_eta, float mu_minpt, float ele_minpt)
{
    std::vector<bool> tmp_jet_flags = samesign::getJetFlags(idx, type, deltaR, 0.0, max_eta, mu_minpt, ele_minpt);

    assert(tmp_jet_flags.size() == cms2.pfjets_p4().size());

    // now impose the pt requirement after applying the extra corrections
    std::vector<bool> final_jets;
    for (unsigned int jidx = 0; jidx < tmp_jet_flags.size(); jidx++) {

        if (!tmp_jet_flags.at(jidx)) {
            final_jets.push_back(tmp_jet_flags.at(jidx));
            continue;
        }
        jet_corrector->setRho(cms2.evt_ww_rho_vor());
        jet_corrector->setJetA(cms2.pfjets_area().at(jidx));
        jet_corrector->setJetPt(cms2.pfjets_p4().at(jidx).pt());
        jet_corrector->setJetEta(cms2.pfjets_p4().at(jidx).eta());        
        float jet_cor = jet_corrector->getCorrection();
        LorentzVector vjet = cms2.pfjets_p4().at(jidx) * jet_cor;
        jet_unc->setJetPt(vjet.pt());    
        jet_unc->setJetEta(vjet.eta());  
        const float jet_cor_unc = jet_unc->getUncertainty(true);     
        vjet *= (1.0 + jet_cor_unc * scale_type);    
        if (vjet.pt() < min_pt) {
            final_jets.push_back(false);
            continue;   
        }

        final_jets.push_back(true);
    }

    return final_jets;    
}


///////////////////////////////////////////////////////////////////////////////////////////
// 2012 get sumpt, skip jets overlapping with numerator e/mu with pt>x (defaults are 20/20 GeV)
///////////////////////////////////////////////////////////////////////////////////////////

// JEC taken from ntuple
float samesign::sumJetPt(int idx_arg, enum JetType type, float deltaR, float min_pt, float max_eta, float mu_minpt, float ele_minpt, float rescale, int systFlag) {
    std::vector<LorentzVector> good_jets = samesign::getJets(idx_arg, type, deltaR, min_pt, max_eta, mu_minpt, ele_minpt, rescale, systFlag);
    unsigned int nJets = good_jets.size();
    if (nJets == 0) return 0.0;
    float sumpt = 0.0;
    for (unsigned int idx = 0; idx < nJets; idx++) {
        sumpt += good_jets.at(idx).pt();
    }
    return sumpt;
}


// JEC applied otf
float samesign::sumJetPt(int idx_arg, FactorizedJetCorrector* jet_corrector, enum JetType type, float deltaR, float min_pt, float max_eta, float mu_minpt, float ele_minpt, float rescale, int systFlag) {
    std::vector<LorentzVector> good_jets = samesign::getJets(idx_arg, jet_corrector, type, deltaR, min_pt, max_eta, mu_minpt, ele_minpt, rescale, systFlag);
    unsigned int nJets = good_jets.size();
    if (nJets == 0) return 0.0;
    float sumpt = 0.0;
    for (unsigned int idx = 0; idx < nJets; idx++) {
        sumpt += good_jets.at(idx).pt();
    }
    return sumpt;
}


// JEC uncertainty applied otf
float samesign::sumJetPt(int idx_arg, JetCorrectionUncertainty *jet_unc, enum JetScaleType scale_type, enum JetType type, float deltaR, float min_pt, float max_eta, float mu_minpt, float ele_minpt)    
{    
    std::vector<LorentzVector> good_jets = samesign::getJets(idx_arg, jet_unc, scale_type, type, deltaR, min_pt, max_eta, mu_minpt, ele_minpt);  
    unsigned int nJets = good_jets.size();   
    if (nJets == 0) return 0.0;  
    float sumpt = 0.0;   
    for (unsigned int idx = 0; idx < nJets; idx++) {     
        sumpt += good_jets.at(idx).pt();     
    }    
    return sumpt;    
}


// JEC AND JEC uncertainty applied otf
float samesign::sumJetPt(int idx_arg, FactorizedJetCorrector* jet_corrector, JetCorrectionUncertainty *jet_unc, enum JetScaleType scale_type, enum JetType type, float deltaR, float min_pt, float max_eta, float mu_minpt, float ele_minpt)     
{    
    std::vector<LorentzVector> good_jets = samesign::getJets(idx_arg, jet_corrector, jet_unc, scale_type, type, deltaR, min_pt, max_eta, mu_minpt, ele_minpt);   
    unsigned int nJets = good_jets.size();   
    if (nJets == 0) return 0.0;  
    float sumpt = 0.0;   
    for (unsigned int idx = 0; idx < nJets; idx++) {     
        sumpt += good_jets.at(idx).pt();     
    }    
    return sumpt;    
}


///////////////////////////////////////////////////////////////////////////////////////////
// 2012 get njets, skip jets overlapping with numerator e/mu with pt>x (defaults are 20/20 GeV)
///////////////////////////////////////////////////////////////////////////////////////////

// JEC taken from ntuple
int samesign::nJets(int idx, enum JetType type, float deltaR, float min_pt, float max_eta, float mu_minpt, float ele_minpt, float rescale, int systFlag)
{
    std::vector<LorentzVector> good_jets = samesign::getJets(idx, type, deltaR, min_pt, max_eta, mu_minpt, ele_minpt, rescale, systFlag);
    return good_jets.size();
}


// JEC applied otf
int samesign::nJets(int idx, FactorizedJetCorrector* jet_corrector, enum JetType type, float deltaR, float min_pt, float max_eta, float mu_minpt, float ele_minpt, float rescale, int systFlag)
{
    std::vector<LorentzVector> good_jets = samesign::getJets(idx, jet_corrector, type, deltaR, min_pt, max_eta, mu_minpt, ele_minpt, rescale, systFlag);
    return good_jets.size();
}


// JEC uncertainty applied otf
int samesign::nJets(int idx, JetCorrectionUncertainty *jet_unc, enum JetScaleType scale_type, enum JetType type, float deltaR, float min_pt, float max_eta, float mu_minpt, float ele_minpt)
{
    std::vector<LorentzVector> good_jets = samesign::getJets(idx, jet_unc, scale_type, type, deltaR, min_pt, max_eta, mu_minpt, ele_minpt);
    return good_jets.size();
}


// JEC AND JEC uncertainty applied otf
int samesign::nJets(int idx, FactorizedJetCorrector* jet_corrector, JetCorrectionUncertainty *jet_unc, enum JetScaleType scale_type, enum JetType type, float deltaR, float min_pt, float max_eta, float mu_minpt, float ele_minpt)
{
    std::vector<LorentzVector> good_jets = samesign::getJets(idx, jet_corrector, jet_unc, scale_type, type, deltaR, min_pt, max_eta, mu_minpt, ele_minpt);
    return good_jets.size();
}


///////////////////////////////////////////////////////////////////////////////////////////
// 2012 get b-tagged jets and perform overlap removal with numerator e/mu with pt > x (defaults are 20/20 GeV)
///////////////////////////////////////////////////////////////////////////////////////////

// JEC taken from ntuple
std::vector<LorentzVector> samesign::getBtaggedJets(int idx, enum JetType type, enum BtagType btag_type, float deltaR, float min_pt, float max_eta, float mu_minpt, float ele_minpt, float rescale, int systFlag, bool sort_by_pt)
{
    std::vector<LorentzVector> tmp_jets = getBtaggedJets(idx, true, type, JETS_CLEAN_HYP_E_MU, btag_type, deltaR, 0.0, max_eta, (double) rescale, systFlag);

    // ok, now perform the rest of the lepton overlap removal
    // and the impose the pt requirement after applying the
    // extra corrections
    std::vector<LorentzVector> final_jets;
    for (unsigned int jidx = 0; jidx < tmp_jets.size(); jidx++) {

        bool jetIsLep = false;

        LorentzVector vjet = tmp_jets.at(jidx);
        if (vjet.pt() < min_pt)
            continue;

        for (unsigned int eidx = 0; eidx < cms2.els_p4().size(); eidx++) {
            if (cms2.els_p4().at(eidx).pt() < ele_minpt)
                continue;
            if (!samesign::isNumeratorLepton(11, eidx))
                continue;

            if (ROOT::Math::VectorUtil::DeltaR(vjet, cms2.els_p4().at(eidx)) > deltaR)
                continue;

            jetIsLep = true;
            break;
        }

        if (jetIsLep) continue;

        for (unsigned int midx = 0; midx < cms2.mus_p4().size(); midx++) {
            if (cms2.mus_p4().at(midx).pt() < mu_minpt)
                continue;
            if (!samesign::isNumeratorLepton(13, midx))
                continue;

            if (ROOT::Math::VectorUtil::DeltaR(vjet, cms2.mus_p4().at(midx)) > deltaR)
                continue;                

            jetIsLep = true;
            break;
        }            

        if (jetIsLep) continue;

        final_jets.push_back(vjet);
    }


    if (sort_by_pt)
        sort(final_jets.begin(), final_jets.end(), SortByPt());
    return final_jets;        
}


// JEC applied otf
std::vector<LorentzVector> samesign::getBtaggedJets(int idx, FactorizedJetCorrector* jet_corrector, enum JetType type, enum BtagType btag_type, float deltaR, float min_pt, float max_eta, float mu_minpt, float ele_minpt, float rescale, int systFlag, bool sort_by_pt)
{
    std::vector<bool> tmp_jet_flags = samesign::getBtaggedJetFlags(idx, type, btag_type, deltaR, 0.0, max_eta, mu_minpt, ele_minpt, rescale, systFlag);

    // now impose the pt requirement after applying the extra corrections
    std::vector<LorentzVector> final_jets;
    for (unsigned int jidx = 0; jidx < tmp_jet_flags.size(); jidx++) {

        if (!tmp_jet_flags.at(jidx)) continue;

        jet_corrector->setRho(cms2.evt_ww_rho_vor());
        jet_corrector->setJetA(cms2.pfjets_area().at(jidx));
        jet_corrector->setJetPt(cms2.pfjets_p4().at(jidx).pt());
        jet_corrector->setJetEta(cms2.pfjets_p4().at(jidx).eta());        
        float jet_cor = jet_corrector->getCorrection();
        LorentzVector vjet = cms2.pfjets_p4().at(jidx) * jet_cor * rescale;
        if (systFlag != 0) {
            float c = getJetMetSyst(systFlag, vjet.pt(), vjet.eta());
            vjet *= c;
        }
        if (vjet.pt() < min_pt)
        {
            continue;
        }
        final_jets.push_back(vjet);
    }

    if (sort_by_pt)
        sort(final_jets.begin(), final_jets.end(), SortByPt());
    return final_jets;
}


// JEC uncertainty applied otf
std::vector<LorentzVector> samesign::getBtaggedJets(int idx, JetCorrectionUncertainty *jet_unc, enum JetScaleType scale_type, enum JetType type, enum BtagType btag_type, float deltaR, float min_pt, float max_eta, float mu_minpt, float ele_minpt, bool sort_by_pt)
{    
    std::vector<bool> tmp_jet_flags = samesign::getBtaggedJetFlags(idx, type, btag_type, deltaR, 0.0, max_eta, mu_minpt, ele_minpt);

    // now impose the pt requirement after applying the extra corrections
    std::vector<LorentzVector> final_jets;
    for (unsigned int jidx = 0; jidx < tmp_jet_flags.size(); jidx++) {

        if (!tmp_jet_flags.at(jidx)) continue;

        const float jet_cor = (cms2.evt_isRealData() ? cms2.pfjets_corL1FastL2L3residual().at(jidx) :  cms2.pfjets_corL1FastL2L3().at(jidx));
        LorentzVector vjet  = cms2.pfjets_p4().at(jidx) * jet_cor; 
        jet_unc->setJetPt(vjet.pt());    
        jet_unc->setJetEta(vjet.eta());  
        const float jet_cor_unc = jet_unc->getUncertainty(true);     
        vjet *= (1.0 + jet_cor_unc * scale_type);    
        if (vjet.pt() < min_pt)
        {
            continue;
        }
        final_jets.push_back(vjet);
    }

    if (sort_by_pt)
        sort(final_jets.begin(), final_jets.end(), SortByPt());
    return final_jets;
}    
     
 
// JEC AND JEC uncertainty applied otf
std::vector<LorentzVector> samesign::getBtaggedJets(int idx, FactorizedJetCorrector* jet_corrector, JetCorrectionUncertainty *jet_unc, enum JetScaleType scale_type, enum JetType type, enum BtagType btag_type, float deltaR, float min_pt, float max_eta, float mu_minpt, float ele_minpt, bool sort_by_pt)
{    
    std::vector<bool> tmp_jet_flags = samesign::getBtaggedJetFlags(idx, type, btag_type, deltaR, 0.0, max_eta, mu_minpt, ele_minpt);

    // now impose the pt requirement after applying the extra corrections
    std::vector<LorentzVector> final_jets;
    for (unsigned int jidx = 0; jidx < tmp_jet_flags.size(); jidx++) {

        if (!tmp_jet_flags.at(jidx)) continue;

        jet_corrector->setRho(cms2.evt_ww_rho_vor());
        jet_corrector->setJetA(cms2.pfjets_area().at(jidx));
        jet_corrector->setJetPt(cms2.pfjets_p4().at(jidx).pt());
        jet_corrector->setJetEta(cms2.pfjets_p4().at(jidx).eta());        
        float jet_cor = jet_corrector->getCorrection();
        LorentzVector vjet = cms2.pfjets_p4().at(jidx) * jet_cor;
        jet_unc->setJetPt(vjet.pt());    
        jet_unc->setJetEta(vjet.eta());  
        const float jet_cor_unc = jet_unc->getUncertainty(true);     
        vjet *= (1.0 + jet_cor_unc * scale_type);    
        if (vjet.pt() < min_pt)
        {
            continue;
        }
        final_jets.push_back(vjet);
    }

    if (sort_by_pt)
        sort(final_jets.begin(), final_jets.end(), SortByPt());
    return final_jets;
}    
     
     
///////////////////////////////////////////////////////////////////////////////////////////
// 2012 get b-tagged jet flags and perform overlap removal with numerator e/mu with pt > x (defaults are 20/20 GeV)
///////////////////////////////////////////////////////////////////////////////////////////

// JEC taken from ntuple
std::vector<bool> samesign::getBtaggedJetFlags(int idx, enum JetType type, enum BtagType btag_type, float deltaR, float min_pt, float max_eta, float mu_minpt, float ele_minpt, float rescale, int systFlag)
{
    std::vector<bool> tmp_jet_flags = getBtaggedJetFlags(idx, type, JETS_CLEAN_HYP_E_MU, btag_type, deltaR, 0.0, max_eta, (double) rescale, systFlag);

    // ok, now perform the rest of the lepton overlap removal
    // and the impose the pt requirement after applying the
    // extra corrections
    std::vector<bool> final_jets;
    for (unsigned int jidx = 0; jidx < tmp_jet_flags.size(); jidx++) {

        if (!tmp_jet_flags.at(jidx)) {
            final_jets.push_back(tmp_jet_flags.at(jidx));
            continue;
        }

        const float jet_cor = (cms2.evt_isRealData() ? cms2.pfjets_corL1FastL2L3residual().at(jidx) :  cms2.pfjets_corL1FastL2L3().at(jidx));
        LorentzVector vjet  = cms2.pfjets_p4().at(jidx) * jet_cor * rescale; 
        if (systFlag != 0) {
            float c = getJetMetSyst(systFlag, vjet.pt(), vjet.eta());
            vjet *= c;
        }
        if (vjet.pt() < min_pt) {
            final_jets.push_back(false);
            continue;
        }

        bool jetIsLep = false;

        for (unsigned int eidx = 0; eidx < cms2.els_p4().size(); eidx++) {
            if (cms2.els_p4().at(eidx).pt() < ele_minpt)
                continue;
            if (!samesign::isNumeratorLepton(11, eidx))
                continue;

            if (ROOT::Math::VectorUtil::DeltaR(vjet, cms2.els_p4().at(eidx)) > deltaR)
                continue;

            jetIsLep = true;
            break;
        }

        if (jetIsLep) {
            final_jets.push_back(false);
            continue;
        }

        for (unsigned int midx = 0; midx < cms2.mus_p4().size(); midx++) {
            if (cms2.mus_p4().at(midx).pt() < mu_minpt)
                continue;
            if (!samesign::isNumeratorLepton(13, midx))
                continue;

            if (ROOT::Math::VectorUtil::DeltaR(vjet, cms2.mus_p4().at(midx)) > deltaR)
                continue;                

            jetIsLep = true;
            break;
        }            

        if (jetIsLep) {
            final_jets.push_back(false);
            continue;   
        }

        final_jets.push_back(true);
    }

    return final_jets;    
}


// JEC applied otf
std::vector<bool> samesign::getBtaggedJetFlags(int idx, FactorizedJetCorrector* jet_corrector, enum JetType type, enum BtagType btag_type, float deltaR, float min_pt, float max_eta, float mu_minpt, float ele_minpt, float rescale, int systFlag)
{
    std::vector<bool> tmp_jet_flags = samesign::getBtaggedJetFlags(idx, type, btag_type, deltaR, 0.0, max_eta, mu_minpt, ele_minpt, rescale, systFlag);

    assert(tmp_jet_flags.size() == cms2.pfjets_p4().size());

    // now impose the pt requirement after applying the extra corrections
    std::vector<bool> final_jets;
    for (unsigned int jidx = 0; jidx < tmp_jet_flags.size(); jidx++) {

        if (!tmp_jet_flags.at(jidx)) {
            final_jets.push_back(tmp_jet_flags.at(jidx));
            continue;
        }

        LorentzVector vjet = cms2.pfjets_p4().at(jidx);
        jet_corrector->setRho(cms2.evt_ww_rho_vor());
        jet_corrector->setJetA(cms2.pfjets_area().at(jidx));
        jet_corrector->setJetPt(cms2.pfjets_p4().at(jidx).pt());
        jet_corrector->setJetEta(cms2.pfjets_p4().at(jidx).eta());        
        float jet_cor = jet_corrector->getCorrection();
        vjet *= jet_cor * rescale;
        if (systFlag != 0) {
            float c = getJetMetSyst(systFlag, vjet.pt(), vjet.eta());
            vjet *= c;
        }
        if (vjet.pt() < min_pt) {
            final_jets.push_back(false);
            continue;   
        }
        final_jets.push_back(true);
    }

    return final_jets;
}


// JEC uncertainty applied otf
std::vector<bool> samesign::getBtaggedJetFlags(int idx, JetCorrectionUncertainty *jet_unc, enum JetScaleType scale_type, enum JetType type, enum BtagType btag_type, float deltaR, float min_pt, float max_eta, float mu_minpt, float ele_minpt)
{
    std::vector<bool> tmp_jet_flags = samesign::getBtaggedJetFlags(idx, type, btag_type, deltaR, 0.0, max_eta, mu_minpt, ele_minpt);

    assert(tmp_jet_flags.size() == cms2.pfjets_p4().size());

    // now impose the pt requirement after applying the extra corrections
    std::vector<bool> final_jets;
    for (unsigned int jidx = 0; jidx < tmp_jet_flags.size(); jidx++) {

        if (!tmp_jet_flags.at(jidx)) {
            final_jets.push_back(tmp_jet_flags.at(jidx));
            continue;
        }
        const float jet_cor = (cms2.evt_isRealData() ? cms2.pfjets_corL1FastL2L3residual().at(jidx) :  cms2.pfjets_corL1FastL2L3().at(jidx));
        LorentzVector vjet  = cms2.pfjets_p4().at(jidx) * jet_cor; 
        jet_unc->setJetPt(vjet.pt());    
        jet_unc->setJetEta(vjet.eta());  
        const float jet_cor_unc = jet_unc->getUncertainty(true);     
        vjet *= (1.0 + jet_cor_unc * scale_type);    
        if (vjet.pt() < min_pt) {
            final_jets.push_back(false);
            continue;   
        }
        final_jets.push_back(true);
    }

    return final_jets;
}


// JEC AND JEC uncertainty applied otf
std::vector<bool> samesign::getBtaggedJetFlags(int idx, FactorizedJetCorrector* jet_corrector, JetCorrectionUncertainty *jet_unc, enum JetScaleType scale_type, enum JetType type, enum BtagType btag_type, float deltaR, float min_pt, float max_eta, float mu_minpt, float ele_minpt)
{
    std::vector<bool> tmp_jet_flags = samesign::getBtaggedJetFlags(idx, type, btag_type, deltaR, 0.0, max_eta, mu_minpt, ele_minpt);

    assert(tmp_jet_flags.size() == cms2.pfjets_p4().size());

    // now impose the pt requirement after applying the extra corrections
    std::vector<bool> final_jets;
    for (unsigned int jidx = 0; jidx < tmp_jet_flags.size(); jidx++) {

        if (!tmp_jet_flags.at(jidx)) {
            final_jets.push_back(tmp_jet_flags.at(jidx));
            continue;
        }
        jet_corrector->setRho(cms2.evt_ww_rho_vor());
        jet_corrector->setJetA(cms2.pfjets_area().at(jidx));
        jet_corrector->setJetPt(cms2.pfjets_p4().at(jidx).pt());
        jet_corrector->setJetEta(cms2.pfjets_p4().at(jidx).eta());        
        const float jet_cor = jet_corrector->getCorrection();
        LorentzVector vjet = cms2.pfjets_p4().at(jidx) * jet_cor;
        jet_unc->setJetPt(vjet.pt());    
        jet_unc->setJetEta(vjet.eta());  
        const float jet_cor_unc = jet_unc->getUncertainty(true);     
        vjet *= (1.0 + jet_cor_unc * scale_type);    
        if (vjet.pt() < min_pt) {
            final_jets.push_back(false);
            continue;   
        }
        final_jets.push_back(true);
    }

    return final_jets;
}


///////////////////////////////////////////////////////////////////////////////////////////
// 2012 get sumpt, skip jets overlapping with numerator e/mu with pt>x (defaults are 20/20 GeV)
///////////////////////////////////////////////////////////////////////////////////////////

// JEC taken from ntuple
int samesign::nBtaggedJets(int idx, enum JetType type, enum BtagType btag_type, float deltaR, float min_pt, float max_eta, float mu_minpt, float ele_minpt, float rescale, int systFlag)
{
    std::vector<LorentzVector> good_jets = samesign::getBtaggedJets(idx, type, btag_type, deltaR, min_pt, max_eta, mu_minpt, ele_minpt, rescale, systFlag);
    return good_jets.size();
}


// JEC applied otf
int samesign::nBtaggedJets(int idx, FactorizedJetCorrector* jet_corrector, enum JetType type, enum BtagType btag_type, float deltaR, float min_pt, float max_eta, float mu_minpt, float ele_minpt, float rescale, int systFlag)
{
    std::vector<LorentzVector> good_jets = samesign::getBtaggedJets(idx, jet_corrector, type, btag_type, deltaR, min_pt, max_eta, mu_minpt, ele_minpt, rescale, systFlag);
    return good_jets.size();    
}


// JEC uncertainty applied otf
int samesign::nBtaggedJets(int idx, JetCorrectionUncertainty *jet_unc, enum JetScaleType scale_type, enum JetType type, enum BtagType btag_type, float deltaR, float min_pt, float max_eta, float mu_minpt, float ele_minpt)     
{    
    std::vector<LorentzVector> good_btags = samesign::getBtaggedJets(idx, jet_unc, scale_type, type, btag_type, deltaR, min_pt, max_eta, mu_minpt, ele_minpt);   
    return good_btags.size();    
}    

 
// JEC AND JEC uncertainty applied otf
int samesign::nBtaggedJets(int idx, FactorizedJetCorrector* jet_corrector, JetCorrectionUncertainty *jet_unc, enum JetScaleType scale_type, enum JetType type, enum BtagType btag_type, float deltaR, float min_pt, float max_eta, float mu_minpt, float ele_minpt)  
{    
    std::vector<LorentzVector> good_btags = samesign::getBtaggedJets(idx, jet_corrector, jet_unc, scale_type, type, btag_type, deltaR, min_pt, max_eta, mu_minpt, ele_minpt);    
    return good_btags.size();    
}    


///////////////////////////////////////////////////////////////////////////////////////////
// 2012 get jet b-tag discriminators, skip jets overlapping with numerator e/mu with pt>x (defaults are 20/20 GeV)
///////////////////////////////////////////////////////////////////////////////////////////

// JEC taken from ntuple
std::vector<float> samesign::getJetBtagDiscriminators(int idx, enum JetType type, enum BtagType btag_type, float deltaR, float min_pt, float max_eta, float mu_minpt, float ele_minpt, float rescale, int systFlag, bool sort_by_pt)
{
    std::vector<LorentzVector> tmp_jet_p4s = samesign::getAllCorrectedJets(type, systFlag, /*sort_by_pt=*/false);
    std::vector<bool> tmp_jet_flags        = samesign::getJetFlags(idx, type, deltaR, min_pt, max_eta, mu_minpt, ele_minpt, rescale, systFlag);

    assert(tmp_jet_p4s.size() == tmp_jet_flags.size());

    std::vector<float> tmp_btag_disc = getJetBtagDiscriminators(btag_type, type);

    assert(tmp_jet_flags.size() == tmp_btag_disc.size());

    std::vector<float> ret;
    if (sort_by_pt)
    {
        std::vector<std::pair<LorentzVector, float> > tmp_jet_p4_disc;
        for (unsigned int jidx = 0; jidx < tmp_jet_flags.size(); jidx++)
        {
            if (!tmp_jet_flags.at(jidx)) continue;
            tmp_jet_p4_disc.push_back(std::make_pair(tmp_jet_p4s.at(jidx), tmp_btag_disc.at(jidx)));
        }
        
        sort(tmp_jet_p4_disc.begin(), tmp_jet_p4_disc.end(), SortByPt());
        
        for (unsigned int bidx = 0; bidx < tmp_jet_p4_disc.size(); bidx++)
        {
            ret.push_back(tmp_jet_p4_disc.at(bidx).second);
        }
    }
    else
    {
        for (unsigned int jidx = 0; jidx < tmp_jet_flags.size(); jidx++)
        {
            if (!tmp_jet_flags.at(jidx)) continue;
            ret.push_back(tmp_btag_disc.at(jidx));
        }
    }

    return ret;
}

// JEC applied otf
std::vector<float> samesign::getJetBtagDiscriminators(int idx, FactorizedJetCorrector* jet_corrector, enum JetType type, enum BtagType btag_type, float deltaR, float min_pt, float max_eta, float mu_minpt, float ele_minpt, float rescale, int systFlag, bool sort_by_pt)
{
    std::vector<LorentzVector> tmp_jet_p4s = samesign::getAllCorrectedJets(type, jet_corrector, systFlag, /*sort_by_pt=*/false);
    std::vector<bool> tmp_jet_flags        = samesign::getJetFlags(idx, jet_corrector, type, deltaR, min_pt, max_eta, mu_minpt, ele_minpt, rescale, systFlag);

    assert(tmp_jet_p4s.size() == tmp_jet_flags.size());

    std::vector<float> tmp_btag_disc = getJetBtagDiscriminators(btag_type, type);

    assert(tmp_jet_flags.size() == tmp_btag_disc.size());

    std::vector<float> ret;
    if (sort_by_pt)
    {
        std::vector<std::pair<LorentzVector, float> > tmp_jet_p4_disc;
        for (unsigned int jidx = 0; jidx < tmp_jet_flags.size(); jidx++)
        {
            if (!tmp_jet_flags.at(jidx)) continue;
            tmp_jet_p4_disc.push_back(std::make_pair(tmp_jet_p4s.at(jidx), tmp_btag_disc.at(jidx)));
        }
        
        sort(tmp_jet_p4_disc.begin(), tmp_jet_p4_disc.end(), SortByPt());
        
        for (unsigned int bidx = 0; bidx < tmp_jet_p4_disc.size(); bidx++)
        {
            ret.push_back(tmp_jet_p4_disc.at(bidx).second);
        }
    }
    else
    {
        for (unsigned int jidx = 0; jidx < tmp_jet_flags.size(); jidx++)
        {
            if (!tmp_jet_flags.at(jidx)) continue;
            ret.push_back(tmp_btag_disc.at(jidx));
        }
    }

    return ret;    
}

// JEC uncertainty applied otf
std::vector<float> samesign::getJetBtagDiscriminators(int idx, JetCorrectionUncertainty *jet_unc, enum JetScaleType scale_type, enum JetType type, enum BtagType btag_type, float deltaR, float min_pt, float max_eta, float mu_minpt, float ele_minpt, bool sort_by_pt)
{
    std::vector<LorentzVector> tmp_jet_p4s = samesign::getAllCorrectedJets(type, jet_unc, scale_type, /*sort_by_pt=*/false);
    std::vector<bool> tmp_jet_flags        = samesign::getJetFlags(idx, jet_unc, scale_type, type, deltaR, min_pt, max_eta, mu_minpt, ele_minpt);

    assert(tmp_jet_p4s.size() == tmp_jet_flags.size());

    std::vector<float> tmp_btag_disc = getJetBtagDiscriminators(btag_type, type);

    assert(tmp_jet_flags.size() == tmp_btag_disc.size());

    std::vector<float> ret;
    if (sort_by_pt)
    {
        std::vector<std::pair<LorentzVector, float> > tmp_jet_p4_disc;
        for (unsigned int jidx = 0; jidx < tmp_jet_flags.size(); jidx++)
        {
            if (!tmp_jet_flags.at(jidx)) continue;
            tmp_jet_p4_disc.push_back(std::make_pair(tmp_jet_p4s.at(jidx), tmp_btag_disc.at(jidx)));
        }
        
        sort(tmp_jet_p4_disc.begin(), tmp_jet_p4_disc.end(), SortByPt());
        
        for (unsigned int bidx = 0; bidx < tmp_jet_p4_disc.size(); bidx++)
        {
            ret.push_back(tmp_jet_p4_disc.at(bidx).second);
        }
    }
    else
    {
        for (unsigned int jidx = 0; jidx < tmp_jet_flags.size(); jidx++)
        {
            if (!tmp_jet_flags.at(jidx)) continue;
            ret.push_back(tmp_btag_disc.at(jidx));
        }
    }

    return ret;    
}

// JEC AND JEC uncertainty applied otf
std::vector<float> samesign::getJetBtagDiscriminators(int idx, FactorizedJetCorrector* jet_corrector, JetCorrectionUncertainty *jet_unc, enum JetScaleType scale_type, enum JetType type, enum BtagType btag_type, float deltaR, float min_pt, float max_eta, float mu_minpt, float ele_minpt, bool sort_by_pt)
{
    std::vector<LorentzVector> tmp_jet_p4s = samesign::getAllCorrectedJets(type, jet_corrector, jet_unc, scale_type, /*sort_by_pt=*/false);
    std::vector<bool> tmp_jet_flags        = samesign::getJetFlags(idx, jet_corrector, jet_unc, scale_type, type, deltaR, min_pt, max_eta, mu_minpt, ele_minpt);

    assert(tmp_jet_p4s.size() == tmp_jet_flags.size());

    std::vector<float> tmp_btag_disc = getJetBtagDiscriminators(btag_type, type);

    assert(tmp_jet_flags.size() == tmp_btag_disc.size());

    std::vector<float> ret;
    if (sort_by_pt)
    {
        std::vector<std::pair<LorentzVector, float> > tmp_jet_p4_disc;
        for (unsigned int jidx = 0; jidx < tmp_jet_flags.size(); jidx++)
        {
            if (!tmp_jet_flags.at(jidx)) continue;
            tmp_jet_p4_disc.push_back(std::make_pair(tmp_jet_p4s.at(jidx), tmp_btag_disc.at(jidx)));
        }
        
        sort(tmp_jet_p4_disc.begin(), tmp_jet_p4_disc.end(), SortByPt());
        
        for (unsigned int bidx = 0; bidx < tmp_jet_p4_disc.size(); bidx++)
        {
            ret.push_back(tmp_jet_p4_disc.at(bidx).second);
        }
    }
    else
    {
        for (unsigned int jidx = 0; jidx < tmp_jet_flags.size(); jidx++)
        {
            if (!tmp_jet_flags.at(jidx)) continue;
            ret.push_back(tmp_btag_disc.at(jidx));
        }
    }

    return ret;    
}


///////////////////////////////////////////////////////////////////////////////////////////
// 2012 get the mc flavor Algo matched to the jet (MC only -- data retusns bogus value) 
///////////////////////////////////////////////////////////////////////////////////////////

std::vector<int> getJetMcAlgoMatch(const std::vector<LorentzVector>& jets_p4, const std::vector<bool>& jets_flag, bool sort_by_pt)
{
    // test the jet flags 
    if (jets_flag.size() != cms2.pfjets_mcflavorAlgo().size())
    {
        throw std::runtime_error("[samesign::getJetMcAgloMatch]: ERROR -- jet_flags and pfjets_mcflavorAlgo not the same size!");
    }

    std::vector<std::pair<LorentzVector, int> > tmp_p4_match;
    unsigned int good_jidx = 0;
    for (unsigned int jidx = 0; jidx < jets_flag.size(); jidx++)
    {
        if (!jets_flag.at(jidx)) continue;
        tmp_p4_match.push_back(std::make_pair(jets_p4.at(good_jidx), cms2.pfjets_mcflavorAlgo().at(jidx)));
        good_jidx++;
    }

    if (sort_by_pt)
    {
        sort(tmp_p4_match.begin(), tmp_p4_match.end(), SortByPt());
    }

    std::vector<int> ret;
    for (unsigned int idx = 0; idx < tmp_p4_match.size(); idx++)
    {
        ret.push_back(tmp_p4_match.at(idx).second);
    }

    return ret;
}

std::vector<int> samesign::getJetMcAlgoMatch(int idx, enum JetType type, float deltaR, float min_pt, float max_eta, float mu_minpt, float ele_minpt, float rescale, int systFlag, bool sort_by_pt)
{
    // get appropriate jet flags
    std::vector<LorentzVector> tmp_jets_p4 = getJets    (idx, type, deltaR, min_pt, max_eta, mu_minpt, ele_minpt, rescale, systFlag, /*sort_by_pt=*/false);
    std::vector<bool> tmp_jets_flag        = getJetFlags(idx, type, deltaR, min_pt, max_eta, mu_minpt, ele_minpt, rescale, systFlag);

    return ::getJetMcAlgoMatch(tmp_jets_p4, tmp_jets_flag, sort_by_pt);
}

std::vector<int> samesign::getJetMcAlgoMatch(int idx, FactorizedJetCorrector* jet_corrector, enum JetType type, float deltaR, float min_pt, float max_eta, float mu_minpt, float ele_minpt, float rescale, int systFlag, bool sort_by_pt)
{
    // get appropriate jet flags
    std::vector<LorentzVector> tmp_jets_p4 = getJets    (idx, jet_corrector, type, deltaR, min_pt, max_eta, mu_minpt, ele_minpt, rescale, systFlag, /*sort_by_pt=*/false);
    std::vector<bool> tmp_jets_flag        = getJetFlags(idx, jet_corrector, type, deltaR, min_pt, max_eta, mu_minpt, ele_minpt, rescale, systFlag);

    return ::getJetMcAlgoMatch(tmp_jets_p4, tmp_jets_flag, sort_by_pt);
}

std::vector<int> samesign::getJetMcAlgoMatch(int idx, JetCorrectionUncertainty *jet_unc, enum JetScaleType scale_type, enum JetType type, float deltaR, float min_pt, float max_eta, float mu_minpt, float ele_minpt, bool sort_by_pt)	 
{
    // get appropriate jet flags
    std::vector<LorentzVector> tmp_jets_p4 = getJets    (idx, jet_unc, scale_type, type, deltaR, min_pt, max_eta, mu_minpt, ele_minpt, /*sort_by_pt=*/false);
    std::vector<bool> tmp_jets_flag        = getJetFlags(idx, jet_unc, scale_type, type, deltaR, min_pt, max_eta, mu_minpt, ele_minpt);

    return ::getJetMcAlgoMatch(tmp_jets_p4, tmp_jets_flag, sort_by_pt);
}

std::vector<int> samesign::getJetMcAlgoMatch(int idx, FactorizedJetCorrector* jet_corrector, JetCorrectionUncertainty *jet_unc, enum JetScaleType scale_type, enum JetType type, float deltaR, float min_pt, float max_eta, float mu_minpt, float ele_minpt, bool sort_by_pt)	 
{
    // get appropriate jet flags
    std::vector<LorentzVector> tmp_jets_p4 = getJets    (idx, jet_corrector, jet_unc, scale_type, type, deltaR, min_pt, max_eta, mu_minpt, ele_minpt, /*sort_by_pt=*/false);
    std::vector<bool> tmp_jets_flag        = getJetFlags(idx, jet_corrector, jet_unc, scale_type, type, deltaR, min_pt, max_eta, mu_minpt, ele_minpt);

    return ::getJetMcAlgoMatch(tmp_jets_p4, tmp_jets_flag, sort_by_pt);
}


///////////////////////////////////////////////////////////////////////////////////////////
// 2012 get the mc flavor Phys matched to the jet (MC only -- data retusns bogus value) 
///////////////////////////////////////////////////////////////////////////////////////////

std::vector<int> getJetMcPhysMatch(const std::vector<LorentzVector>& jets_p4, const std::vector<bool>& jets_flag, bool sort_by_pt)
{
    // test the jet flags 
    if (jets_flag.size() != cms2.pfjets_mcflavorPhys().size())
    {
        throw std::runtime_error("[samesign::getJetMcAgloMatch]: ERROR -- jet_flags and pfjets_mcflavorPhys not the same size!");
    }

    std::vector<std::pair<LorentzVector, int> > tmp_p4_match;
    unsigned int good_jidx = 0;
    for (unsigned int jidx = 0; jidx < jets_flag.size(); jidx++)
    {
        if (!jets_flag.at(jidx)) continue;
        tmp_p4_match.push_back(std::make_pair(jets_p4.at(good_jidx), cms2.pfjets_mcflavorPhys().at(jidx)));
        good_jidx++;
    }

    if (sort_by_pt)
    {
        sort(tmp_p4_match.begin(), tmp_p4_match.end(), SortByPt());
    }

    std::vector<int> ret;
    for (unsigned int idx = 0; idx < tmp_p4_match.size(); idx++)
    {
        ret.push_back(tmp_p4_match.at(idx).second);
    }

    return ret;
}

std::vector<int> samesign::getJetMcPhysMatch(int idx, enum JetType type, float deltaR, float min_pt, float max_eta, float mu_minpt, float ele_minpt, float rescale, int systFlag, bool sort_by_pt)
{
    // get appropriate jet flags
    std::vector<LorentzVector> tmp_jets_p4 = getJets    (idx, type, deltaR, min_pt, max_eta, mu_minpt, ele_minpt, rescale, systFlag, /*sort_by_pt=*/false);
    std::vector<bool> tmp_jets_flag        = getJetFlags(idx, type, deltaR, min_pt, max_eta, mu_minpt, ele_minpt, rescale, systFlag);

    return ::getJetMcPhysMatch(tmp_jets_p4, tmp_jets_flag, sort_by_pt);
}

std::vector<int> samesign::getJetMcPhysMatch(int idx, FactorizedJetCorrector* jet_corrector, enum JetType type, float deltaR, float min_pt, float max_eta, float mu_minpt, float ele_minpt, float rescale, int systFlag, bool sort_by_pt)
{
    // get appropriate jet flags
    std::vector<LorentzVector> tmp_jets_p4 = getJets    (idx, jet_corrector, type, deltaR, min_pt, max_eta, mu_minpt, ele_minpt, rescale, systFlag, /*sort_by_pt=*/false);
    std::vector<bool> tmp_jets_flag        = getJetFlags(idx, jet_corrector, type, deltaR, min_pt, max_eta, mu_minpt, ele_minpt, rescale, systFlag);

    return ::getJetMcPhysMatch(tmp_jets_p4, tmp_jets_flag, sort_by_pt);
}

std::vector<int> samesign::getJetMcPhysMatch(int idx, JetCorrectionUncertainty *jet_unc, enum JetScaleType scale_type, enum JetType type, float deltaR, float min_pt, float max_eta, float mu_minpt, float ele_minpt, bool sort_by_pt)	 
{
    // get appropriate jet flags
    std::vector<LorentzVector> tmp_jets_p4 = getJets    (idx, jet_unc, scale_type, type, deltaR, min_pt, max_eta, mu_minpt, ele_minpt, /*sort_by_pt=*/false);
    std::vector<bool> tmp_jets_flag        = getJetFlags(idx, jet_unc, scale_type, type, deltaR, min_pt, max_eta, mu_minpt, ele_minpt);

    return ::getJetMcPhysMatch(tmp_jets_p4, tmp_jets_flag, sort_by_pt);
}

std::vector<int> samesign::getJetMcPhysMatch(int idx, FactorizedJetCorrector* jet_corrector, JetCorrectionUncertainty *jet_unc, enum JetScaleType scale_type, enum JetType type, float deltaR, float min_pt, float max_eta, float mu_minpt, float ele_minpt, bool sort_by_pt)	 
{
    // get appropriate jet flags
    std::vector<LorentzVector> tmp_jets_p4 = getJets    (idx, jet_corrector, jet_unc, scale_type, type, deltaR, min_pt, max_eta, mu_minpt, ele_minpt, /*sort_by_pt=*/false);
    std::vector<bool> tmp_jets_flag        = getJetFlags(idx, jet_corrector, jet_unc, scale_type, type, deltaR, min_pt, max_eta, mu_minpt, ele_minpt);

    return ::getJetMcPhysMatch(tmp_jets_p4, tmp_jets_flag, sort_by_pt);
}


///////////////////////////////////////////////////////////////////////////////////////////
// 2012 get the pt sorted btagged jet flags 
///////////////////////////////////////////////////////////////////////////////////////////

std::vector<bool> sortBtaggedFlags(const std::vector<LorentzVector>& all_jet_p4s, const std::vector<bool>& all_jet_flags, const std::vector<bool>& all_bjet_flags)
{
    // some sanity checks
    assert(all_jet_p4s.size() == all_jet_flags.size());
    assert(all_jet_flags.size() == all_bjet_flags.size());

    std::vector<std::pair<LorentzVector, bool> > tmp_p4_flags;
    for (unsigned int idx = 0; idx < all_jet_flags.size(); idx++)
    {
        if (!all_jet_flags.at(idx)) continue;
        tmp_p4_flags.push_back(std::make_pair(all_jet_p4s.at(idx), all_bjet_flags.at(idx)));
    }

    sort(tmp_p4_flags.begin(), tmp_p4_flags.end(), SortByPt());

    std::vector<bool> ret;
    for(unsigned int idx = 0; idx < tmp_p4_flags.size(); idx++)
    {
        ret.push_back(tmp_p4_flags.at(idx).second);
    }

    return ret;
}

std::vector<bool> samesign::getSortedBtaggedFlags(int idx, enum JetType type, enum BtagType btag_type, float deltaR, float min_pt, float max_eta, float mu_minpt, float ele_minpt, float rescale, int systFlag)
{
    // get appropriate jet flags
    std::vector<LorentzVector> tmp_jets_p4 = getAllCorrectedJets(type, systFlag, /*sort_by_pt=*/false);
    std::vector<bool> tmp_jets_flag        = getJetFlags        (idx, type,            deltaR, min_pt, max_eta, mu_minpt, ele_minpt, rescale, systFlag);
    std::vector<bool> tmp_jets_btag_flag   = getBtaggedJetFlags (idx, type, btag_type, deltaR, min_pt, max_eta, mu_minpt, ele_minpt, rescale, systFlag);

    return sortBtaggedFlags(tmp_jets_p4, tmp_jets_flag, tmp_jets_btag_flag);
}

std::vector<bool> samesign::getSortedBtaggedFlags(int idx, FactorizedJetCorrector* jet_corrector, enum JetType type, enum BtagType btag_type, float deltaR, float min_pt, float max_eta, float mu_minpt, float ele_minpt, float rescale, int systFlag)
{
    // get appropriate jet flags
    std::vector<LorentzVector> tmp_jets_p4 = getAllCorrectedJets(type, jet_corrector, systFlag, /*sort_by_pt=*/false);
    std::vector<bool> tmp_jets_flag        = getJetFlags        (idx, jet_corrector, type,            deltaR, min_pt, max_eta, mu_minpt, ele_minpt, rescale, systFlag);
    std::vector<bool> tmp_jets_btag_flag   = getBtaggedJetFlags (idx, jet_corrector, type, btag_type, deltaR, min_pt, max_eta, mu_minpt, ele_minpt, rescale, systFlag);

    return sortBtaggedFlags(tmp_jets_p4, tmp_jets_flag, tmp_jets_btag_flag);
}

std::vector<bool> samesign::getSortedBtaggedFlags(int idx, JetCorrectionUncertainty *jet_unc, enum JetScaleType scale_type, enum JetType type, enum BtagType btag_type, float deltaR, float min_pt, float max_eta, float mu_minpt, float ele_minpt)	 
{
    // get appropriate jet flags
    std::vector<LorentzVector> tmp_jets_p4 = getAllCorrectedJets(type, jet_unc, scale_type, /*sort_by_pt=*/false);
    std::vector<bool> tmp_jets_flag        = getJetFlags        (idx, jet_unc, scale_type, type,            deltaR, min_pt, max_eta, mu_minpt, ele_minpt);
    std::vector<bool> tmp_jets_btag_flag   = getBtaggedJetFlags (idx, jet_unc, scale_type, type, btag_type, deltaR, min_pt, max_eta, mu_minpt, ele_minpt);

    return sortBtaggedFlags(tmp_jets_p4, tmp_jets_flag, tmp_jets_btag_flag);
}

std::vector<bool> samesign::getSortedBtaggedFlags(int idx, FactorizedJetCorrector* jet_corrector, JetCorrectionUncertainty *jet_unc, enum JetScaleType scale_type, enum JetType type, enum BtagType btag_type, float deltaR, float min_pt, float max_eta, float mu_minpt, float ele_minpt)	 
{
    // get appropriate jet flags
    std::vector<LorentzVector> tmp_jets_p4 = getAllCorrectedJets(type, jet_corrector, jet_unc, scale_type, /*sort_by_pt=*/false);
    std::vector<bool> tmp_jets_flag        = getJetFlags        (idx, jet_corrector, jet_unc, scale_type, type,            deltaR, min_pt, max_eta, mu_minpt, ele_minpt);
    std::vector<bool> tmp_jets_btag_flag   = getBtaggedJetFlags (idx, jet_corrector, jet_unc, scale_type, type, btag_type, deltaR, min_pt, max_eta, mu_minpt, ele_minpt);

    return sortBtaggedFlags(tmp_jets_p4, tmp_jets_flag, tmp_jets_btag_flag);
}

///////////////////////////////////////////////////////////////////////////////////////////
// 2012 rescale the jet energy resolution (JER) 
///////////////////////////////////////////////////////////////////////////////////////////

// function that returns the sigma(pT)*pT of the MC jets. those numbers are old,
// but at least they are in an understandable format.
// -----------------------------------------------------------------------------
float getErrPt(const float pt, const float eta)
{
    float InvPerr2;
    float N = 0.0;
    float S = 0.0;
    float C = 0.0;
    float m = 0.0;
    if(fabs(eta) < 0.5)
    {
        N = 3.96859;
        S = 0.18348;
        C = 0.;
        m = 0.62627;
    } 
    else if(fabs(eta) < 1.0)
    {
        N = 3.55226;
        S = 0.24026;
        C = 0.;
        m = 0.52571;
    } 
    else if(fabs(eta) < 1.5) 
    {
        N = 4.54826;
        S = 0.22652;
        C = 0.;
        m = 0.58963;
    }
    else if(fabs(eta) < 2.0)
    {
        N = 4.62622;
        S = 0.23664;
        C = 0.;
        m = 0.48738;
    }
    else if(fabs(eta) < 2.5)
    {
        N = 2.53324;
        S = 0.34306;
        C = 0.;
        m = 0.28662;
    }
    else if(fabs(eta) < 3.0)
    {
        N = -3.33814;
        S = 0.73360;
        C = 0.;
        m = 0.08264;
    }
    else if(fabs(eta) < 5.0)
    {
        N = 2.95397;
        S = 0.11619;
        C = 0.;
        m = 0.96086;
    }
    
    // this is the absolute resolution (squared), not sigma(pt)/pt
    // so have to multiply by pt^2, thats why m+1 instead of m-1
    InvPerr2 = (N * fabs(N) ) + (S * S) * pow(pt, m+1) + (C * C) * pt * pt;

    return sqrt(InvPerr2);
}

// function to get the jer scale factors. values taken from the twiki: 
// https://twiki.cern.ch/twiki/bin/view/CMS/JetResolution
// -------------------------------------------------------------------
float getJERScale(const float jet_eta)
{
    const float aeta = fabs(jet_eta);
    if      (aeta < 0.5) {return 1.052;}
    else if (aeta < 1.1) {return 1.057;}
    else if (aeta < 1.7) {return 1.096;}
    else if (aeta < 2.3) {return 1.134;}
    else                 {return 1.288;}

}

// rescaled the jet p4s, met, met_phi and ht scaling up the JER
void samesign::smearJETScaleJetsMetHt(std::vector<LorentzVector>& vjets_p4, float& met, float& met_phi, float& ht, const unsigned int seed)
{
    static TRandom3 random(seed);
    float new_ht = 0;

    // rescale the jets/met/ht
    ROOT::Math::XYVector cmet(met*cos(met_phi), met*sin(met_phi));
    std::vector<LorentzVector> new_vjets_p4;
    for (size_t jidx = 0; jidx != vjets_p4.size(); jidx++)
    {
        random.SetSeed(seed*(jidx+1));
    
        // rescale the jet pt
        const LorentzVector& jet_p4 = vjets_p4.at(jidx);
        const float jer_scale       = getJERScale(jet_p4.eta());
        const float sigma_mc        = getErrPt(jet_p4.pt(), jet_p4.eta())/jet_p4.pt();
        const float jet_rescaled    = random.Gaus(1.0, sqrt(jer_scale*jer_scale-1.0)*sigma_mc);
        LorentzVector new_jet_p4    = (jet_p4 * jet_rescaled);

        // propogate to the met
        ROOT::Math::XYVector old_jet(jet_p4.px(), jet_p4.py());
        ROOT::Math::XYVector new_jet(new_jet_p4.px(), new_jet_p4.py());
        cmet = cmet - new_jet + old_jet;

        // check that the new jets pass the min pt cut
        if (new_jet_p4.pt() < 40.0)
        {
            continue;
        }

        // add to the new ht
        new_ht += new_jet_p4.pt();

        // return the new jet
        new_vjets_p4.push_back(LorentzVector(new_jet_p4));
    }
    
    // set the new met
    met     = cmet.r();
    met_phi = cmet.phi();

    // set the new pt
    ht = new_ht;

    // set the new jets
    vjets_p4 = new_vjets_p4;

    // done
    return;
}       
            
void samesign::smearJETScaleJetsMetHt
(
    std::vector<LorentzVector>& vjets_p4, 
    float& met,
    float& met_phi,
    float& ht,
    int idx,
    enum JetType type,
    const unsigned int seed,
    float deltaR,
    float min_pt,
    float max_eta,
    float mu_minpt,
    float ele_minpt
    )
{
    static TRandom3 random(seed);
    float new_ht = 0;

    // rescale the jets/met/ht
    ROOT::Math::XYVector cmet(met*cos(met_phi), met*sin(met_phi));
    std::vector<LorentzVector> new_vjets_p4;
    std::vector<LorentzVector> tmp_vjets_p4 = samesign::getJets(idx, type, deltaR, /*min_pt=*/15, /*max_eta=*/2.4, mu_minpt, ele_minpt);
    for (size_t jidx = 0; jidx != tmp_vjets_p4.size(); jidx++)
    {
        random.SetSeed(seed*(jidx+1));

        // rescale the jet pt
        const LorentzVector& jet_p4 = tmp_vjets_p4.at(jidx);
        const float jer_scale       = getJERScale(jet_p4.eta());
        const float sigma_mc        = getErrPt(jet_p4.pt(), jet_p4.eta())/jet_p4.pt();
        const float arg             = sqrt(jer_scale*jer_scale-1.0)*sigma_mc;
        const float jet_rescaled    = random.Gaus(1.0, arg);
        LorentzVector new_jet_p4    = (jet_p4 * jet_rescaled);

        // propogate to the met
        ROOT::Math::XYVector old_jet(jet_p4.px(), jet_p4.py());
        ROOT::Math::XYVector new_jet(new_jet_p4.px(), new_jet_p4.py());
        cmet = cmet - new_jet + old_jet;

        // check that the new jets pass the min pt cut
        if (not (new_jet_p4.pt() > min_pt and fabs(new_jet_p4.eta()) < max_eta))
        {
            continue;
        }

        // add to the new ht
        new_ht += new_jet_p4.pt();

        // return the new jet
        new_vjets_p4.push_back(LorentzVector(new_jet_p4));
    }
    
    // set the new met
    met     = cmet.r();
    met_phi = cmet.phi();

    // set the new pt
    ht = new_ht;

    // set the new jets
    vjets_p4 = new_vjets_p4;

    // done
    return;
}


// semar JER for jets
void samesign::smearJETScaleJets(std::vector<LorentzVector>& vjets_p4, const unsigned int seed, float min_pt)
{
    static TRandom3 random(seed);

    // rescale the b-tagged jets
    std::vector<LorentzVector> new_vjets_p4;
    for (size_t jidx = 0; jidx != vjets_p4.size(); jidx++)
    {
        random.SetSeed(seed*(jidx+1));

        // rescale the jet pt
        const LorentzVector& jet_p4 = vjets_p4.at(jidx);
        const float jer_scale       = getJERScale(jet_p4.eta());
        const float sigma_mc        = getErrPt(jet_p4.pt(), jet_p4.eta())/jet_p4.pt();
        const float jet_rescaled    = random.Gaus(1.0, sqrt(jer_scale*jer_scale-1.0)*sigma_mc);
        LorentzVector new_jet_p4    = (jet_p4 * jet_rescaled);

        // check that the new jets pass the min pt cut
        if (new_jet_p4.pt() < min_pt)
        {
            continue;
        }

        // return the new jet
        new_vjets_p4.push_back(new_jet_p4);
    }
    vjets_p4 = new_vjets_p4;

    // done
    return;
}       


///////////////////////////////////////////////////////////////////////////////////////////
// 2012 rescale the MET by scaling up/down the unclustered erngy 
///////////////////////////////////////////////////////////////////////////////////////////
float samesign::scaleMET
(
    const float met,
    const float met_phi,
    int idx,
    enum JetType type,
    float deltaR,
    float min_pt,
    float max_eta,
    float mu_minpt,
    float ele_minpt,
    const int scale_type,
    const float scale
    )
{
    using namespace tas;
    typedef ROOT::Math::Polar2DVectorF Polar2D;

    // 2D vector to keep the sums
    Polar2D jets;
    Polar2D leps;

    // sum up the jets 
    std::vector<LorentzVector> vjets_p4 = samesign::getJets(idx, type, deltaR, min_pt, max_eta, mu_minpt, ele_minpt);
    for (size_t jidx = 0; jidx != vjets_p4.size(); jidx++)
    {
        jets += Polar2D(vjets_p4.at(jidx).pt(), vjets_p4.at(jidx).phi());
    }
    // sum up the electrons
    for (size_t eidx = 0; eidx < els_p4().size(); eidx++)
    {
        if (els_p4().at(eidx).pt() < ele_minpt)     {continue;}
        if (!samesign::isNumeratorLepton(11, eidx)) {continue;}
        leps += Polar2D(els_p4().at(eidx).pt(), els_p4().at(eidx).phi());
    }
    // sum up the muons
    for (size_t midx = 0; midx < mus_p4().size(); midx++)
    {
        if (mus_p4().at(midx).pt() < mu_minpt)      {continue;}
        if (!samesign::isNumeratorLepton(13, midx)) {continue;}
        leps += Polar2D(mus_p4().at(midx).pt(), mus_p4().at(midx).phi());
    }
    
    // subtract the leptons and jet contributions
    Polar2D umet(met, met_phi);
    umet = umet + leps + jets;

    // scale the unclustered energy by 10%
    umet.SetR(umet.r() * (1.0 + scale_type * scale));

    // resum the met
    Polar2D new_met = umet - jets - leps;
    return new_met.r();
}

     
///////////////////////////////////////////////////////////////////////////////////////////  
// 2012 get vector of good els p4s   
///////////////////////////////////////////////////////////////////////////////////////////  
std::vector<LorentzVector> samesign::getGoodElectrons(const float ptcut)     
{    
    std::vector<LorentzVector> good_els;     
    for (unsigned int idx = 0; idx < cms2.els_p4().size(); idx++) {  
     
        if (cms2.els_p4().at(idx).pt() < ptcut) continue;    
        if (fabs(cms2.els_p4().at(idx).eta()) > 2.4) continue;   
     
        if (!isNumeratorLepton(11, idx)) continue;   
         
        good_els.push_back(cms2.els_p4().at(idx));   
    }    
         
    sort(good_els.begin(), good_els.end(), SortByPt());  
    return good_els;     
}    


std::vector<std::pair<LorentzVector, unsigned int> > samesign::getNumeratorElectrons(const float ptcut)
{    
    std::vector<std::pair<LorentzVector, unsigned int> > good_els;   
    for (unsigned int idx = 0; idx < cms2.els_p4().size(); idx++)
    {    
        if (cms2.els_p4().at(idx).pt() < ptcut)      {continue;}
        if (fabs(cms2.els_p4().at(idx).eta()) > 2.4) {continue;}
        if (!samesign::isNumeratorLepton(11, idx))   {continue;}
        good_els.push_back(std::make_pair(cms2.els_p4().at(idx), idx));  
    }    
         
    sort(good_els.begin(), good_els.end(), SortByPt());  
    return good_els;     
}    

         
         
///////////////////////////////////////////////////////////////////////////////////////////  
// 2012 get vector of good mus p4s   
///////////////////////////////////////////////////////////////////////////////////////////  
std::vector<LorentzVector> samesign::getGoodMuons(const float ptcut)     
{    
    std::vector<LorentzVector> good_mus;     
    for (unsigned int idx = 0; idx < cms2.mus_p4().size(); idx++) {  
         
        if (cms2.mus_p4().at(idx).pt() < ptcut) continue;    
        if (fabs(cms2.mus_p4().at(idx).eta()) > 2.4) continue;   
        if (!isNumeratorLepton(13, idx)) continue;   
         
        good_mus.push_back(cms2.mus_p4().at(idx));   
    }    
         
    sort(good_mus.begin(), good_mus.end(), SortByPt());  
    return good_mus;     
}

std::vector<std::pair<LorentzVector, unsigned int> > samesign::getNumeratorMuons(const float ptcut)
{    
    std::vector<std::pair<LorentzVector, unsigned int> > good_mus;   
    for (unsigned int idx = 0; idx < cms2.mus_p4().size(); idx++)
    {    
        if (cms2.mus_p4().at(idx).pt() < ptcut)      {continue;}
        if (fabs(cms2.mus_p4().at(idx).eta()) > 2.4) {continue;}
        if (!samesign::isNumeratorLepton(13, idx))   {continue;}
        good_mus.push_back(std::make_pair(cms2.mus_p4().at(idx), idx));  
    }    
    sort(good_mus.begin(), good_mus.end(), SortByPt());  
    return good_mus;     
}    


/////////////////////////////////////////////////////////////////
///                                                           ///
///                                                           ///
///                                                           ///
///          2011 Selections                                  ///
///                                                           ///
///                                                           ///
///                                                           ///
/////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////     
// 2011 good lepton
////////////////////////////////////////////////////////////////////////////////////////////     
bool samesign2011::isGoodLepton(int id, int idx)
{
    // electrons
    if (abs(id) == 11)
        return (pass_electronSelection(idx, electronSelection_ssV6_noIso, false, false));

    // muons
    if (abs(id) == 13)
        return (muonIdNotIsolated(idx, NominalSSv4));

    return false;
}


////////////////////////////////////////////////////////////////////////////////////////////     
// 2011 isolated lepton
////////////////////////////////////////////////////////////////////////////////////////////     
bool samesign2011::isIsolatedLepton(int id, int idx, enum IsolationType iso_type)
{
    // electrons
    if (abs(id) == 11) {
        if (iso_type == DET_ISO)
            return (pass_electronSelection(idx, electronSelection_ssV6_iso));
        else if (iso_type == COR_DET_ISO)
            return (electronIsolation_cor_rel_v1(idx, true) < 0.10);
        else if (iso_type == TIGHT_DET_ISO)
            return (electronIsolation_rel_v1(idx, true) < 0.10);
    }

    // muons
    if (abs(id) == 13) {
        if (iso_type == DET_ISO)
            return (muonIsoValue(idx, false) < 0.15);
        else if (iso_type == COR_DET_ISO)
            return (muonCorIsoValue(idx, false) < 0.10);
        else if (iso_type == TIGHT_DET_ISO)
            return (muonIsoValue(idx, false) < 0.10);
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////     
// 2011 numerator lepton
////////////////////////////////////////////////////////////////////////////////////////////     
bool samesign2011::isNumeratorLepton(int id, int idx, enum IsolationType iso_type)
{
    return (samesign2011::isGoodLepton(id, idx) && samesign2011::isIsolatedLepton(id, idx, iso_type));
}


////////////////////////////////////////////////////////////////////////////////////////////     
// 2011 numerator hypothesis
////////////////////////////////////////////////////////////////////////////////////////////     
bool samesign2011::isNumeratorHypothesis(int idx, enum IsolationType iso_type)
{
    if (!samesign2011::isNumeratorLepton(cms2.hyp_lt_id().at(idx), cms2.hyp_lt_index().at(idx), iso_type))
        return false;
    if (!samesign2011::isNumeratorLepton(cms2.hyp_ll_id().at(idx), cms2.hyp_ll_index().at(idx), iso_type))
        return false;

    return true;
}


////////////////////////////////////////////////////////////////////////////////////////////     
// 2011 denominator lepton
////////////////////////////////////////////////////////////////////////////////////////////     
bool samesign2011::isDenominatorLepton(int id, int idx, enum IsolationType iso_type)
{
    // electrons
    if (abs(id) == 11) {
        if (iso_type == DET_ISO || iso_type == TIGHT_DET_ISO)
            return (pass_electronSelection(idx, electronSelectionFOV6_ssVBTF80_v3, false, false) && electronIsolation_rel_v1(idx, true) < 0.60);
        else if (iso_type == COR_DET_ISO)
            return (pass_electronSelection(idx, electronSelectionFOV6_ssVBTF80_v3, false, false) && electronIsolation_cor_rel_v1(idx, true) < 0.60);            
    }

    // muons
    if (abs(id) == 13) {
        if (iso_type == DET_ISO || iso_type == TIGHT_DET_ISO)
            return (muonId(idx, muonSelectionFO_ssV4));
        else if (iso_type == COR_DET_ISO)
            return (muonIdNotIsolated(idx, muonSelectionFO_ssV4) && muonCorIsoValue(idx, false) < 0.40);
    }

    return false;
}


///////////////////////////////////////////////////////////////////////////////////////////
// extra Z veto
///////////////////////////////////////////////////////////////////////////////////////////
bool samesign2011::overlapsOtherNNHypInZ(int idx, enum IsolationType iso_type){
    bool result = false;
    int nHyps = cms2.hyp_lt_p4().size();
    for (int iH = 0; iH< nHyps; ++iH){
        if (iH == idx || !hypsOverlap(idx,iH)
            || abs(cms2.hyp_lt_id()[iH])!= abs(cms2.hyp_ll_id()[iH]) || cms2.hyp_lt_id()[iH]*cms2.hyp_ll_id()[iH] > 0) 
            continue;
        if (! samesign2011::isNumeratorHypothesis(iH,iso_type)) continue;
        if (cms2.hyp_p4()[iH].mass2()>0 && fabs(cms2.hyp_p4()[iH].mass() - 91)< 15){ 
            result = true; break;
        }
    }
    return result;
}

///////////////////////////////////////////////////////////////////////////////////////////
// require electron GSF, CTF and SC charges agree
///////////////////////////////////////////////////////////////////////////////////////////
bool samesign2011::passThreeChargeRequirement(int elIdx)
{
    int trk_idx = cms2.els_trkidx()[elIdx];

    if (trk_idx >= 0)
    {
        if (cms2.els_sccharge()[elIdx] == cms2.els_trk_charge()[elIdx] && cms2.els_trk_charge()[elIdx] == cms2.trks_charge()[trk_idx])             
            return true;
    }

    return false;
}

///////////////////////////////////////////////////////////////////////////////////////////
// get jets and perform overlap removal with numerator e/mu with pt > x (defaults are 10/5 GeV)
///////////////////////////////////////////////////////////////////////////////////////////
std::vector<LorentzVector> samesign2011::getJets(int idx, enum JetType type, double deltaR, double min_pt, double max_eta, double mu_minpt, double ele_minpt, enum IsolationType iso_type, double rescale) {

    std::vector<LorentzVector> tmp_jets = getJets(idx, true, type, JETS_CLEAN_HYP_E_MU, deltaR, 0., max_eta, rescale);

    // ok, now perform the rest of the lepton overlap removal
    // and the impose the pt requirement after applying the
    // extra corrections
    std::vector<LorentzVector> final_jets;
    for (unsigned int jidx = 0; jidx < tmp_jets.size(); jidx++) {

        bool jetIsLep = false;

        LorentzVector vjet = tmp_jets.at(jidx);
        if (vjet.pt() < min_pt)
            continue;

        for (unsigned int eidx = 0; eidx < cms2.els_p4().size(); eidx++) {
            if (cms2.els_p4().at(eidx).pt() < ele_minpt)
                continue;
            if (!samesign2011::isNumeratorLepton(11, eidx, iso_type))
                continue;

            if (ROOT::Math::VectorUtil::DeltaR(vjet, cms2.els_p4().at(eidx)) > deltaR)
                continue;

            jetIsLep = true;
            break;
        }

        if (jetIsLep) continue;

        for (unsigned int midx = 0; midx < cms2.mus_p4().size(); midx++) {
            if (cms2.mus_p4().at(midx).pt() < mu_minpt)
                continue;
            if (!samesign2011::isNumeratorLepton(13, midx, iso_type))
                continue;

            if (ROOT::Math::VectorUtil::DeltaR(vjet, cms2.mus_p4().at(midx)) > deltaR)
                continue;                

            jetIsLep = true;
            break;
        }            

        if (jetIsLep) continue;

        final_jets.push_back(vjet);
    }

    sort(final_jets.begin(), final_jets.end(), SortByPt());
    return final_jets;    
}

///////////////////////////////////////////////////////////////////////////////////////////
// get jets and apply an on-the-fly JEC
///////////////////////////////////////////////////////////////////////////////////////////
std::vector<LorentzVector> samesign2011::getJets(int idx, FactorizedJetCorrector* jet_corrector, enum JetType type, double deltaR, double min_pt, double max_eta, double mu_minpt, double ele_minpt, enum IsolationType iso_type, double rescale) {

    std::vector<LorentzVector> tmp_jets = samesign2011::getJets(idx, type, deltaR, 0., max_eta, mu_minpt, ele_minpt, iso_type);

    // now impose the pt requirement after applying the extra corrections
    std::vector<LorentzVector> final_jets;
    for (unsigned int jidx = 0; jidx < tmp_jets.size(); jidx++) {

        LorentzVector vjet = tmp_jets.at(jidx);
        float jet_cor = jetCorrection(vjet, jet_corrector);
        vjet *= jet_cor * rescale;
        if (vjet.pt() < min_pt)
            continue;

        final_jets.push_back(vjet);
    }

    sort(final_jets.begin(), final_jets.end(), SortByPt());
    return final_jets;
}


///////////////////////////////////////////////////////////////////////////////////////////
// get jets and perform overlap removal with numerator e/mu with pt > x (defaults are 10/5 GeV)
///////////////////////////////////////////////////////////////////////////////////////////
std::vector<bool> samesign2011::getJetFlags(int idx, enum JetType type, double deltaR, double min_pt, double max_eta, double mu_minpt, double ele_minpt, enum IsolationType iso_type, double rescale) {

    std::vector<bool> tmp_jet_flags = getJetFlags((unsigned int)idx, type, JETS_CLEAN_HYP_E_MU, deltaR, 0., max_eta, rescale);

    // ok, now perform the rest of the lepton overlap removal
    // and the impose the pt requirement after applying the
    // extra corrections
    std::vector<bool> final_jets;
    for (unsigned int jidx = 0; jidx < tmp_jet_flags.size(); jidx++) {

        if (!tmp_jet_flags.at(jidx)) {
            final_jets.push_back(tmp_jet_flags.at(jidx));
            continue;
        }

        LorentzVector vjet = cms2.pfjets_p4().at(jidx) * cms2.pfjets_corL1FastL2L3().at(jidx) * rescale;
        if (vjet.pt() < min_pt) {
            final_jets.push_back(false);
            continue;
        }

        bool jetIsLep = false;

        for (unsigned int eidx = 0; eidx < cms2.els_p4().size(); eidx++) {
            if (cms2.els_p4().at(eidx).pt() < ele_minpt)
                continue;
            if (!samesign2011::isNumeratorLepton(11, eidx, iso_type))
                continue;

            if (ROOT::Math::VectorUtil::DeltaR(vjet, cms2.els_p4().at(eidx)) > deltaR)
                continue;

            jetIsLep = true;
            break;
        }

        if (jetIsLep) {
            final_jets.push_back(false);
            continue;
        }

        for (unsigned int midx = 0; midx < cms2.mus_p4().size(); midx++) {
            if (cms2.mus_p4().at(midx).pt() < mu_minpt)
                continue;
            if (!samesign2011::isNumeratorLepton(13, midx, iso_type))
                continue;

            if (ROOT::Math::VectorUtil::DeltaR(vjet, cms2.mus_p4().at(midx)) > deltaR)
                continue;                

            jetIsLep = true;
            break;
        }            

        if (jetIsLep) {
            final_jets.push_back(false);
            continue;   
        }

        final_jets.push_back(true);
    }

    return final_jets;    
}

///////////////////////////////////////////////////////////////////////////////////////////
// get jets and apply an on-the-fly JEC and perform overlap removal with numerator
// e/mu with pt > x (defaults are 10/5 GeV)
///////////////////////////////////////////////////////////////////////////////////////////
std::vector<bool> samesign2011::getJetFlags(int idx, FactorizedJetCorrector* jet_corrector, enum JetType type, double deltaR, double min_pt, double max_eta, double mu_minpt, double ele_minpt, enum IsolationType iso_type, double rescale) {

    std::vector<bool> tmp_jet_flags     = samesign2011::getJetFlags(idx, type, deltaR, 0., max_eta, mu_minpt, ele_minpt, iso_type);
    std::vector<LorentzVector> tmp_jets = samesign2011::getJets(idx, type, deltaR, 0., max_eta, mu_minpt, ele_minpt, iso_type);

    // now impose the pt requirement after applying the extra corrections
    std::vector<bool> final_jets;
    for (unsigned int jidx = 0; jidx < tmp_jet_flags.size(); jidx++) {

        if (!tmp_jet_flags.at(jidx)) {
            final_jets.push_back(tmp_jet_flags.at(jidx));
            continue;
        }

        LorentzVector vjet = tmp_jets.at(jidx);
        float jet_cor = jetCorrection(vjet, jet_corrector);
        vjet *= jet_cor * rescale;
        if (vjet.pt() < min_pt) {
            final_jets.push_back(false);
            continue;   
        }

        final_jets.push_back(true);
    }

    return final_jets;    
}


///////////////////////////////////////////////////////////////////////////////////////////
// get sumpt, skip jets overlapping with numerator e/mu with pt>x (defaults are 10/5 GeV)
///////////////////////////////////////////////////////////////////////////////////////////
float samesign2011::sumJetPt(int idx_arg, enum JetType type, double deltaR, double min_pt, double max_eta, double mu_minpt, double ele_minpt, enum IsolationType iso_type, double rescale) {
    std::vector<LorentzVector> good_jets = samesign2011::getJets(idx_arg, type, deltaR, min_pt, max_eta, mu_minpt, ele_minpt, iso_type, rescale);
    unsigned int nJets = good_jets.size();
    if (nJets == 0) return 0.0;
    float sumpt = 0.0;
    for (unsigned int idx = 0; idx < nJets; idx++) {
        sumpt += good_jets.at(idx).pt();
    }
    return sumpt;
}

///////////////////////////////////////////////////////////////////////////////////////////
// same as above, but allowing use of on-the-fly JEC corrections
///////////////////////////////////////////////////////////////////////////////////////////
float samesign2011::sumJetPt(int idx_arg, FactorizedJetCorrector* jet_corrector, enum JetType type, double deltaR, double min_pt, double max_eta, double mu_minpt, double ele_minpt, enum IsolationType iso_type, double rescale) {
    std::vector<LorentzVector> good_jets = samesign2011::getJets(idx_arg, jet_corrector, type, deltaR, min_pt, max_eta, mu_minpt, ele_minpt, iso_type, rescale);
    unsigned int nJets = good_jets.size();
    if (nJets == 0) return 0.0;
    float sumpt = 0.0;
    for (unsigned int idx = 0; idx < nJets; idx++) {
        sumpt += good_jets.at(idx).pt();
    }
    return sumpt;
}

///////////////////////////////////////////////////////////////////////////////////////////
// get sumpt, skip jets overlapping with numerator e/mu with pt>x (defaults are 10/5 GeV)
///////////////////////////////////////////////////////////////////////////////////////////
int samesign2011::nJets(int idx, enum JetType type, double deltaR, double min_pt, double max_eta, double mu_minpt, double ele_minpt, enum IsolationType iso_type, double rescale) {
    
    std::vector<LorentzVector> good_jets = samesign2011::getJets(idx, type, deltaR, min_pt, max_eta, mu_minpt, ele_minpt, iso_type, rescale);
    return good_jets.size();
}

///////////////////////////////////////////////////////////////////////////////////////////
// same as above, but allowing use of on-the-fly JEC corrections
///////////////////////////////////////////////////////////////////////////////////////////
int samesign2011::nJets(int idx, FactorizedJetCorrector* jet_corrector, enum JetType type, double deltaR, double min_pt, double max_eta, double mu_minpt, double ele_minpt, enum IsolationType iso_type, double rescale) {

    std::vector<LorentzVector> good_jets = samesign2011::getJets(idx, jet_corrector, type, deltaR, min_pt, max_eta, mu_minpt, ele_minpt, iso_type, rescale);
    return good_jets.size();
}


///////////////////////////////////////////////////////////////////////////////////////////
// passes dilepton trigger
///////////////////////////////////////////////////////////////////////////////////////////
bool samesign2011::passesTrigger(bool is_data, int hyp_type, bool is_high_pt) {
    
    //----------------------------------------
    // no trigger requirements applied to MC
    //----------------------------------------
  
    if (!is_data)
        return true; 
  
    //---------------------------------
    // triggers for lepton-HT datasets
    //---------------------------------
  
    if (!is_high_pt) {
  
        //mm
        if (hyp_type == 0) {
            if( passUnprescaledHLTTriggerPattern("HLT_DoubleMu3_HT150_v") )   return true;
            if( passUnprescaledHLTTriggerPattern("HLT_DoubleMu3_HT160_v") )   return true;
            if( passUnprescaledHLTTriggerPattern("HLT_DoubleMu3_HT200_v") )   return true;
            if( passUnprescaledHLTTriggerPattern("HLT_DoubleMu5_HT150_v") )   return true;
            if( passUnprescaledHLTTriggerPattern("HLT_DoubleMu5_Mass4_HT150_v") )   return true;
            if( passUnprescaledHLTTriggerPattern("HLT_DoubleMu5_Mass8_HT150_v") )   return true;
            if( passUnprescaledHLTTriggerPattern("HLT_DoubleMu8_Mass8_HT150_v") )   return true;
            if( passUnprescaledHLTTriggerPattern("HLT_DoubleMu8_Mass8_HT200_v") )   return true;
            if( passUnprescaledHLTTriggerPattern("HLT_DoubleTkIso10Mu5_Mass8_HT150_v") )   return true;
            if( passUnprescaledHLTTriggerPattern("HLT_DoubleTkIso10Mu5_Mass8_HT200_v") )   return true;
        }
    
        //em
        else if (hyp_type == 1 || hyp_type == 2) {
            if( passUnprescaledHLTTriggerPattern("HLT_Mu3_Ele8_CaloIdL_TrkIdVL_HT150_v") )     return true; 
            if( passUnprescaledHLTTriggerPattern("HLT_Mu3_Ele8_CaloIdT_TrkIdVL_HT150_v") )     return true;
            if( passUnprescaledHLTTriggerPattern("HLT_Mu3_Ele8_CaloIdL_TrkIdVL_HT160_v") )     return true; 
            if( passUnprescaledHLTTriggerPattern("HLT_Mu3_Ele8_CaloIdT_TrkIdVL_HT160_v") )     return true;
            if( passUnprescaledHLTTriggerPattern("HLT_Mu5_Ele8_CaloIdT_TrkIdVL_Mass4_HT150_v") )     return true;
            if( passUnprescaledHLTTriggerPattern("HLT_Mu5_Ele8_CaloIdT_TrkIdVL_Mass8_HT150_v") )     return true;
            if( passUnprescaledHLTTriggerPattern("HLT_Mu8_Ele8_CaloIdT_TrkIdVL_Mass8_HT150_v") )     return true;
            if( passUnprescaledHLTTriggerPattern("HLT_Mu8_Ele8_CaloIdT_TrkIdVL_Mass8_HT200_v") )     return true;
            if( passUnprescaledHLTTriggerPattern("HLT_TkIso10Mu5_Ele8_CaloIdT_CaloIsoVVL_TrkIdVL_Mass8_HT150_v") )     return true;
            if( passUnprescaledHLTTriggerPattern("HLT_TkIso10Mu5_Ele8_CaloIdT_CaloIsoVVL_TrkIdVL_Mass8_HT200_v") )     return true;
        }
    
        //ee
        else if (hyp_type == 3) {
            if( passUnprescaledHLTTriggerPattern("HLT_DoubleEle8_CaloIdL_TrkIdVL_HT150_v") )   return true;
            if( passUnprescaledHLTTriggerPattern("HLT_DoubleEle8_CaloIdT_TrkIdVL_HT150_v") )   return true;
            if( passUnprescaledHLTTriggerPattern("HLT_DoubleEle8_CaloIdL_TrkIdVL_HT160_v") )   return true;
            if( passUnprescaledHLTTriggerPattern("HLT_DoubleEle8_CaloIdT_TrkIdVL_HT160_v") )   return true;
            if( passUnprescaledHLTTriggerPattern("HLT_DoubleEle8_CaloIdT_TrkIdVL_Mass4_HT150_v") )   return true;
            if( passUnprescaledHLTTriggerPattern("HLT_DoubleEle8_CaloIdT_TrkIdVL_Mass8_HT150_v") )   return true;
            if( passUnprescaledHLTTriggerPattern("HLT_DoubleEle8_CaloIdT_TrkIdVL_Mass8_HT200_v") )   return true;
        }
    }
  
    //---------------------------------
    // triggers for dilepton datasets
    //---------------------------------
  
    else {
  
        //mm
        if (hyp_type == 0) {
            if( passUnprescaledHLTTriggerPattern("HLT_DoubleMu7_v") )   return true;
            if( passUnprescaledHLTTriggerPattern("HLT_Mu13_Mu7_v" ) )   return true;
            if( passUnprescaledHLTTriggerPattern("HLT_Mu13_Mu8_v" ) )   return true;
            if( passUnprescaledHLTTriggerPattern("HLT_Mu17_Mu8_v" ) )   return true;
        }
    
        //em
        else if (hyp_type == 1 || hyp_type == 2) {
            if( passUnprescaledHLTTriggerPattern("HLT_Mu17_Ele8_CaloIdL_v") )   return true;
            if( passUnprescaledHLTTriggerPattern("HLT_Mu8_Ele17_CaloIdL_v") )   return true;
            if( passUnprescaledHLTTriggerPattern("HLT_Mu17_Ele8_CaloIdT_CaloIsoVL_v") )   return true;
            if( passUnprescaledHLTTriggerPattern("HLT_Mu8_Ele17_CaloIdT_CaloIsoVL_v") )   return true;

        }
    
        //ee
        else if (hyp_type == 3) {
            if( passUnprescaledHLTTriggerPattern("HLT_Ele17_CaloIdL_CaloIsoVL_Ele8_CaloIdL_CaloIsoVL_v") )                                   return true;
            if( passUnprescaledHLTTriggerPattern("HLT_Ele17_CaloIdT_TrkIdVL_CaloIsoVL_TrkIsoVL_Ele8_CaloIdT_TrkIdVL_CaloIsoVL_TrkIsoVL_v") ) return true;
            if( passUnprescaledHLTTriggerPattern("HLT_Ele17_CaloIdT_CaloIsoVL_TrkIdVL_TrkIsoVL_Ele8_CaloIdT_CaloIsoVL_TrkIdVL_TrkIsoVL_v") ) return true;
        }                                     
    }        
  
    return false;
}

///////////////////////////////////////////////////////////////////////////////////////////
// get jets and perform overlap removal with numerator e/mu with pt > x (defaults are 10/5 GeV)
///////////////////////////////////////////////////////////////////////////////////////////
std::vector<LorentzVector> samesign2011::getBtaggedJets(int idx, enum JetType type, enum BtagType btag_type, double deltaR, double min_pt, double max_eta, double mu_minpt, double ele_minpt, enum IsolationType iso_type, double rescale) {

    std::vector<LorentzVector> tmp_jets = getBtaggedJets(idx, true, type, JETS_CLEAN_HYP_E_MU, btag_type, deltaR, 0., max_eta, rescale);

    // ok, now perform the rest of the lepton overlap removal
    // and the impose the pt requirement after applying the
    // extra corrections
    std::vector<LorentzVector> final_jets;
    for (unsigned int jidx = 0; jidx < tmp_jets.size(); jidx++) {

        bool jetIsLep = false;

        LorentzVector vjet = tmp_jets.at(jidx);
        if (vjet.pt() < min_pt)
            continue;

        for (unsigned int eidx = 0; eidx < cms2.els_p4().size(); eidx++) {
            if (cms2.els_p4().at(eidx).pt() < ele_minpt)
                continue;
            if (!samesign2011::isNumeratorLepton(11, eidx, iso_type))
                continue;

            if (ROOT::Math::VectorUtil::DeltaR(vjet, cms2.els_p4().at(eidx)) > deltaR)
                continue;

            jetIsLep = true;
            break;
        }

        if (jetIsLep) continue;

        for (unsigned int midx = 0; midx < cms2.mus_p4().size(); midx++) {
            if (cms2.mus_p4().at(midx).pt() < mu_minpt)
                continue;
            if (!samesign2011::isNumeratorLepton(13, midx, iso_type))
                continue;

            if (ROOT::Math::VectorUtil::DeltaR(vjet, cms2.mus_p4().at(midx)) > deltaR)
                continue;                

            jetIsLep = true;
            break;
        }            

        if (jetIsLep) continue;

        final_jets.push_back(vjet);
    }

    sort(final_jets.begin(), final_jets.end(), SortByPt());
    return final_jets;        
}

///////////////////////////////////////////////////////////////////////////////////////////
// get jets and apply an on-the-fly JEC and perform overlap removal with numerator
// e/mu with pt > x (defaults are 10/5 GeV)
///////////////////////////////////////////////////////////////////////////////////////////
std::vector<LorentzVector> samesign2011::getBtaggedJets(int idx, FactorizedJetCorrector* jet_corrector, enum JetType type, enum BtagType btag_type, double deltaR, double min_pt, double max_eta, double mu_minpt, double ele_minpt, enum IsolationType iso_type, double rescale) {

    std::vector<LorentzVector> tmp_jets = samesign2011::getBtaggedJets(idx, type, btag_type, deltaR, 0., max_eta, mu_minpt, ele_minpt, iso_type);

    // now impose the pt requirement after applying the extra corrections
    std::vector<LorentzVector> final_jets;
    for (unsigned int jidx = 0; jidx < tmp_jets.size(); jidx++) {

        LorentzVector vjet = tmp_jets.at(jidx);
        float jet_cor = jetCorrection(vjet, jet_corrector);
        vjet *= jet_cor * rescale;
        if (vjet.pt() < min_pt)
            continue;

        final_jets.push_back(vjet);
    }

    sort(final_jets.begin(), final_jets.end(), SortByPt());
    return final_jets;
    
}


///////////////////////////////////////////////////////////////////////////////////////////
// get sumpt, skip jets overlapping with numerator e/mu with pt>x (defaults are 10/5 GeV)
///////////////////////////////////////////////////////////////////////////////////////////
int samesign2011::nBtaggedJets(int idx, enum JetType type, enum BtagType btag_type, double deltaR, double min_pt, double max_eta, double mu_minpt, double ele_minpt, enum IsolationType iso_type, double rescale) {

    std::vector<LorentzVector> good_jets = samesign2011::getBtaggedJets(idx, type, btag_type, deltaR, min_pt, max_eta, mu_minpt, ele_minpt, iso_type, rescale);
    return good_jets.size();
}

///////////////////////////////////////////////////////////////////////////////////////////
// same as above, but allowing use of on-the-fly JEC corrections
///////////////////////////////////////////////////////////////////////////////////////////
int samesign2011::nBtaggedJets(int idx, FactorizedJetCorrector* jet_corrector, enum JetType type, enum BtagType btag_type, double deltaR, double min_pt, double max_eta, double mu_minpt, double ele_minpt, enum IsolationType iso_type, double rescale) {

    std::vector<LorentzVector> good_jets = samesign2011::getBtaggedJets(idx, jet_corrector, type, btag_type, deltaR, min_pt, max_eta, mu_minpt, ele_minpt, iso_type, rescale);
    return good_jets.size();    
}

///////////////////////////////////////////////////////////////////////////////////////////
// extra Z veto for b-tagged same sign analysis
///////////////////////////////////////////////////////////////////////////////////////////
bool samesign2011::makesExtraZ(int idx, enum IsolationType iso_type, bool apply_id_iso) {

    std::vector<unsigned int> ele_idx;
    std::vector<unsigned int> mu_idx;


    int lt_id   = cms2.hyp_lt_id().at(idx);
    int ll_id   = cms2.hyp_ll_id().at(idx);
    unsigned int lt_idx  = cms2.hyp_lt_index().at(idx);
    unsigned int ll_idx  = cms2.hyp_ll_index().at(idx);

    (abs(lt_id) == 11) ? ele_idx.push_back(lt_idx) : mu_idx.push_back(lt_idx);
    (abs(ll_id) == 11) ? ele_idx.push_back(ll_idx) : mu_idx.push_back(ll_idx);

    if (ele_idx.size() + mu_idx.size() != 2) {
        std::cout << "ERROR: don't have 2 leptons in hypothesis!!!  Exiting" << std::endl;
        return false;
    }
        
    if (ele_idx.size() > 0) {
        for (unsigned int eidx = 0; eidx < cms2.els_p4().size(); eidx++) {

            bool is_hyp_lep = false;
            for (unsigned int vidx = 0; vidx < ele_idx.size(); vidx++) {
                if (eidx == ele_idx.at(vidx))
                    is_hyp_lep = true;                
            }
            if (is_hyp_lep)
                continue;

            if (fabs(cms2.els_p4().at(eidx).eta()) > 2.5)
                continue;

            if (cms2.els_p4().at(eidx).pt() < 10.)
                continue;

            if (apply_id_iso) {
                float iso_val = (iso_type == DET_ISO || iso_type == TIGHT_DET_ISO) ? electronIsolation_rel_v1(eidx, true) : electronIsolation_cor_rel_v1(eidx, true);
                if (iso_val > 0.2)
                    continue;
                
                if (!electronId_VBTF(eidx, VBTF_95_NOHOEEND))
                    continue;                
            }

            for (unsigned int vidx = 0; vidx < ele_idx.size(); vidx++) {

                if (cms2.els_charge().at(eidx) * cms2.els_charge().at(ele_idx.at(vidx)) > 0)
                    continue;

                LorentzVector zp4 = cms2.els_p4().at(eidx) + cms2.els_p4().at(ele_idx.at(vidx));
                float zcandmass = sqrt(fabs(zp4.mass2()));
                if (fabs(zcandmass-91.) < 15.)
                    return true;
            }
        }        
    }

    if (mu_idx.size() > 0) {
        for (unsigned int midx = 0; midx < cms2.mus_p4().size(); midx++) {

            bool is_hyp_lep = false;
            for (unsigned int vidx = 0; vidx < mu_idx.size(); vidx++) {
                if (midx == mu_idx.at(vidx))
                    is_hyp_lep = true;                
            }
            if (is_hyp_lep)
                continue;

            if (fabs(cms2.mus_p4().at(midx).eta()) > 2.5)
                continue;

            if (cms2.mus_p4().at(midx).pt() < 10.)
                continue;

            if (apply_id_iso) {
                float iso_val = (iso_type == DET_ISO || iso_type == TIGHT_DET_ISO) ? muonIsoValue(midx, false) : muonCorIsoValue(midx, false);
                if (iso_val > 0.2)
                    continue;
                
                if (!muonIdNotIsolated(midx, OSGeneric_v4))
                    continue;                
            }

            for (unsigned int vidx = 0; vidx < mu_idx.size(); vidx++) {

                if (cms2.mus_charge().at(midx) * cms2.mus_charge().at(mu_idx.at(vidx)) > 0)
                    continue;

                LorentzVector zp4 = cms2.mus_p4().at(midx) + cms2.mus_p4().at(mu_idx.at(vidx));
                float zcandmass = sqrt(fabs(zp4.mass2()));
                if (fabs(zcandmass-91.) < 15.)
                    return true;
            }
        }
    }

    return false;
}
