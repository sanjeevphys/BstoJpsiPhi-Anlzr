// -*- C++ -*-
//
// Package:    JPsiphiPAT
// Class:      JPsiphiPAT
// 
//=================================================
// original author:  Jhovanny Andres Mejia        |
//         created:  Fryday Anuary 20 2017        |
//         <jhovanny.andres.mejia.guisao@cern.ch> | 
//=================================================

// system include files
#include <memory>

// user include files
#include "myAnalyzers/JPsiKsPAT/src/JPsiphiPAT.h"

#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/EDAnalyzer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "CommonTools/Statistics/interface/ChiSquaredProbability.h"

#include "FWCore/ServiceRegistry/interface/Service.h"
#include "CommonTools/UtilAlgos/interface/TFileService.h"
#include "DataFormats/Common/interface/Handle.h"
#include "DataFormats/Common/interface/TriggerResults.h"
#include "FWCore/Common/interface/TriggerNames.h"
#include "DataFormats/HLTReco/interface/TriggerEvent.h"
#include "DataFormats/FWLite/interface/EventBase.h"

#include "DataFormats/Candidate/interface/VertexCompositeCandidate.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "SimDataFormats/GeneratorProducts/interface/HepMCProduct.h"

#include "RecoVertex/KinematicFitPrimitives/interface/MultiTrackKinematicConstraint.h"
#include "RecoVertex/KinematicFit/interface/KinematicConstrainedVertexFitter.h"
#include "RecoVertex/KinematicFit/interface/TwoTrackMassKinematicConstraint.h"
#include "DataFormats/BeamSpot/interface/BeamSpot.h"

#include "TrackingTools/TransientTrack/interface/TransientTrackBuilder.h"
#include "TrackingTools/Records/interface/TransientTrackRecord.h"
#include "TrackingTools/TransientTrack/interface/TransientTrack.h"
#include "TrackingTools/PatternTools/interface/ClosestApproachInRPhi.h"

#include "MagneticField/Engine/interface/MagneticField.h"
#include "DataFormats/MuonReco/interface/MuonChamberMatch.h"

#include "DataFormats/Common/interface/RefToBase.h"
#include "DataFormats/Candidate/interface/ShallowCloneCandidate.h"
#include "DataFormats/HepMCCandidate/interface/GenParticle.h"
#include "DataFormats/Candidate/interface/CandMatchMap.h"
#include "DataFormats/Math/interface/Error.h"
#include "RecoVertex/VertexTools/interface/VertexDistance.h"
#include "RecoVertex/VertexTools/interface/VertexDistance3D.h"
#include "RecoVertex/VertexTools/interface/VertexDistanceXY.h"

#include "DataFormats/PatCandidates/interface/Muon.h"
#include "DataFormats/PatCandidates/interface/GenericParticle.h"

#include "SimDataFormats/GeneratorProducts/interface/GenEventInfoProduct.h"
#include "DataFormats/CLHEP/interface/Migration.h"

#include "RecoVertex/VertexPrimitives/interface/BasicSingleVertexState.h"
#include "RecoVertex/VertexPrimitives/interface/VertexState.h"

#include "TFile.h"
#include "TTree.h"

#include <vector>
#include "TLorentzVector.h"
#include <utility>
#include <string>

#include "DataFormats/PatCandidates/interface/TriggerObjectStandAlone.h"

//
// constants, enums and typedefs
//

  typedef math::Error<3>::type CovarianceMatrix;

//
// static data member definitions
//

//
// constructors and destructor
//
JPsiphiPAT::JPsiphiPAT(const edm::ParameterSet& iConfig)
  :
  dimuon_Label(consumes<edm::View<pat::Muon>>(iConfig.getParameter<edm::InputTag>("dimuons"))),
  trakCollection_label(consumes<edm::View<pat::PackedCandidate>>(iConfig.getParameter<edm::InputTag>("Trak"))),
  //trakCollection_label_lowpt(consumes<edm::View<pat::PackedCandidate>>(iConfig.getParameter<edm::InputTag>("Trak_lowpt"))),
  primaryVertices_Label(consumes<reco::VertexCollection>(iConfig.getParameter<edm::InputTag>("primaryVertices"))),
  triggerResults_Label(consumes<edm::TriggerResults>(iConfig.getParameter<edm::InputTag>("TriggerResults"))),
  BSLabel_(consumes<reco::BeamSpot>(iConfig.getParameter<edm::InputTag>("bslabel"))),

  OnlyBest_(iConfig.getParameter<bool>("OnlyBest")),
  isMC_(iConfig.getParameter<bool>("isMC")),
  OnlyGen_(iConfig.getParameter<bool>("OnlyGen")),

  //genParticles_ ( iConfig.getUntrackedParameter<std::string>("GenParticles",std::string("genParticles")) ),
  //doMC_( iConfig.getUntrackedParameter<bool>("doMC",false) ),
  tree_(0), 

  priRfVtxX(0), priRfVtxY(0), priRfVtxZ(0), priRfVtxXE(0), priRfVtxYE(0), priRfVtxZE(0), priRfVtxCL(0),
  priRfVtxXYE(0), priRfVtxXZE(0), priRfVtxYZE(0),
  
  priRfNTrkDif(0), 
  //Rfvtx_key(0),
  //trackrefPVpt(0), trackrefPVdxy(0), trackrefPVdxy_e(0), trackrefPVdz(0), trackrefPVdz_e(0),

  mumC2(0), mumCat(0), mumAngT(0), mumNHits(0), mumNPHits(0),
  mupC2(0), mupCat(0), mupAngT(0), mupNHits(0), mupNPHits(0),
  mumdxy(0), mupdxy(0), mumdz(0), mupdz(0),
  muon_dca(0),

  tri_Dim25(0), tri_Dim20(0), tri_JpsiTk(0), tri_DoubleMu43Jpsi(0),

  mu1soft(0), mu2soft(0), mu1tight(0), mu2tight(0), 
  mu1PF(0), mu2PF(0), mu1loose(0), mu2loose(0),

  // *******************************************************
  
  nB(0), nMu(0),
  B_mass(0), B_px(0), B_py(0), B_pz(0),

  B_phi_mass(0), 
  //B_phi_px(0), B_phi_py(0), B_phi_pz(0),
  B_phi_px1(0), B_phi_py1(0), B_phi_pz1(0), 
  B_phi_px2(0), B_phi_py2(0), B_phi_pz2(0),

  B_phi_px1_track(0), B_phi_py1_track(0), B_phi_pz1_track(0), 
  B_phi_px2_track(0), B_phi_py2_track(0), B_phi_pz2_track(0),

  B_phi_charge1(0), B_phi_charge2(0),
  k1dxy(0), k2dxy(0), k1dz(0), k2dz(0),
  k1dxy_e(0), k2dxy_e(0), k1dz_e(0), k2dz_e(0),

  B_J_mass(0), B_J_px(0), B_J_py(0), B_J_pz(0),
  //B_J_pt1(0),
  B_J_px1(0), B_J_py1(0), B_J_pz1(0), 
  //B_J_pt2(0), 
  B_J_px2(0), B_J_py2(0), B_J_pz2(0), 
  B_J_charge1(0), B_J_charge2(0),

  B_phi_parentId1(0), B_phi_parentId2(0),
  B_phi_pId1(0), B_phi_pId2(0),


  nVtx(0), vertex_id(0),
  priVtxX(0), priVtxY(0), priVtxZ(0), priVtxXE(0), priVtxYE(0), priVtxZE(0), priVtxCL(0),
  priVtxXYE(0), priVtxXZE(0), priVtxYZE(0),
  
    // vertice primario CON mejor pointin-angle
  pVtxIPX(0),   pVtxIPY(0),   pVtxIPZ(0), pVtxIPXE(0),   pVtxIPYE(0),   pVtxIPZE(0), pVtxIPCL(0),
  pVtxIPXYE(0),   pVtxIPXZE(0),   pVtxIPYZE(0),

   
  // ************ esta es la informacion concerniente al beamspot *************
  PVXBS(0), PVYBS(0), PVZBS(0), PVXBSE(0), PVYBSE(0), PVZBSE(0),
  PVXYBSE(0), PVXZBSE(0), PVYZBSE(0),
  
  // ************************ ****************************************************


  B_chi2(0), B_J_chi2(0), 
  B_Prob(0), B_J_Prob(0), 
  //B_phi_chi2(0), B_phi_Prob(0),

  B_DecayVtxX(0),     B_DecayVtxY(0),     B_DecayVtxZ(0),
  B_DecayVtxXE(0),    B_DecayVtxYE(0),    B_DecayVtxZE(0),
  B_DecayVtxXYE(0),   B_DecayVtxXZE(0),   B_DecayVtxYZE(0),

  /* B_phi_DecayVtxX(0),  B_phi_DecayVtxY(0),  B_phi_DecayVtxZ(0),
  B_phi_DecayVtxXE(0), B_phi_DecayVtxYE(0), B_phi_DecayVtxZE(0),
  B_phi_DecayVtxXYE(0), B_phi_DecayVtxXZE(0), B_phi_DecayVtxYZE(0),*/

  B_J_DecayVtxX(0),   B_J_DecayVtxY(0),   B_J_DecayVtxZ(0),
  B_J_DecayVtxXE(0),  B_J_DecayVtxYE(0),  B_J_DecayVtxZE(0),
  B_J_DecayVtxXYE(0), B_J_DecayVtxXZE(0), B_J_DecayVtxYZE(0),

 //triggersL(0), nTrgL(0),
  run(0), event(0),
lumiblock(0)

{
   //now do what ever initialization is needed
}


JPsiphiPAT::~JPsiphiPAT()
{

}


//
// member functions
//

void JPsiphiPAT::CheckHLTTriggers(const std::vector<std::string>& TrigList){

    using namespace std;
    using namespace edm;
    using namespace reco;
    using namespace helper;
    
    
    string AllTrg="";
    string tmptrig;

    int ntrigs=TrigList.size();
    if (ntrigs==0)
        std::cout << "No trigger name given in TriggerResults of the input " << endl;
    
    for (int itrig=0; itrig< ntrigs; itrig++) {
        //TString trigName = triggerNames_.triggerName(itrig);
        string trigName = TrigList.at(itrig);
    
	     tmptrig = (string) trigName; tmptrig +=" ";
	     AllTrg += tmptrig;
    }

     int m = sprintf(triggersL,"%s","");
    m = sprintf(triggersL,"%s",AllTrg.c_str());
    //cout<<" INFO: Triggers :  "<<m<<"  "<<n<<"  "<<triggersL<<endl;

    //nTrgL = AllTrg.size();
   nTrgL = m;
   //cout<<" INFO: Triggers :  "<<m<<"  "<<nTrgL<<"  "<<triggersL<<endl;

   return;
}



/*
int JPsiphiPAT::PdgIDatTruthLevel(reco::TrackRef Track, edm::Handle<reco::GenParticleCollection> genParticles, int &ParentID)
{
  double pXTrack = Track->momentum().x();
  double pYTrack = Track->momentum().y();
  double pZTrack = Track->momentum().z();

  //std::cout<<" *** "<<pXTrack<<" -- "<<pYTrack<<" --  "<<pZTrack<<std::endl;

  std::vector<double> chi2v;
  std::vector<int> idPDG;
  std::vector<int> idPDGP;

  for( size_t k = 0; k < genParticles->size(); k++ ) 
    {

      reco::GenParticle PCand =  (*genParticles)[k];
      double truePx = PCand.px();
      double truePy = PCand.py();
      double truePz = PCand.pz();
      //if(PCand.pdgId()==fabs(211)) std::cout<<truePx<<"  "<<truePy<<"  "<<truePz<<std::endl;
      
      double dpx = pXTrack - truePx;
      double dpy = pYTrack - truePy;
      double dpz = pZTrack - truePz;
      double chi2 = dpx*dpx + dpy*dpy + dpz*dpz;
      //if(chi2<1.) std::cout<<chi2<<"  id: "<<PCand.pdgId()<<std::endl;
      //if(chi2<1.) std::cout<<truePx<<"  "<<truePy<<"  "<<truePz<<" id: "<<PCand.pdgId()<<std::endl;

       if(chi2>10.0)continue;

      if(PCand.numberOfMothers()!=1) continue;
      const reco::Candidate * PCandM = PCand.mother();
      //std::cout<<PCandM->pdgId()<<std::endl;

      //B_phi_chi2->push_back(chi2);
      chi2v.push_back(chi2);
      idPDG.push_back(PCand.pdgId());
      idPDGP.push_back(PCandM->pdgId());

    }

  double chi2min_temp=100000;
  //double chi2min=-1;
  int idtemp = -1;
  int idptemp = -1;
  for(int i=0;i<(int)chi2v.size();i++)
    {
      if(chi2min_temp>chi2v.at(i))
	{
	  chi2min_temp = chi2v.at(i);
	  idtemp = idPDG.at(i);
	  idptemp = idPDGP.at(i);
	}
    }
 
  //std::cout<<chi2min_temp<<"  id: "<<idtemp<<std::endl;
  ParentID = idptemp;
  return idtemp;
}
*/





// ------------ method called to for each event  ------------
void JPsiphiPAT::analyze(const edm::Event& iEvent, const edm::EventSetup& iSetup)
{
  using std::vector;
  using namespace edm;
  using namespace reco;
  using namespace std;


  
  //*********************************
  // Get event content information
  //*********************************  

  //ESHandle<MagneticField> bFieldHandle;
  //iSetup.get<IdealMagneticFieldRecord>().get(bFieldHandle);

 // Kinematic fit
  edm::ESHandle<TransientTrackBuilder> theB; 
  iSetup.get<TransientTrackRecord>().get("TransientTrackBuilder",theB); 

  edm::Handle< View<pat::PackedCandidate> > thePATTrackHandle;
  iEvent.getByToken(trakCollection_label,thePATTrackHandle);
  //edm::Handle< View<pat::PackedCandidate> > thePATTrackHandle_Ori;
  //iEvent.getByToken(trakCollection_label,thePATTrackHandle_Ori);

  //edm::Handle< View<pat::PackedCandidate> > thePATTrackHandle_lowpt;
  //iEvent.getByToken(trakCollection_label_lowpt,thePATTrackHandle_lowpt);

  edm::Handle< View<pat::Muon> > thePATMuonHandle; 
  iEvent.getByToken(dimuon_Label,thePATMuonHandle);

  // ************* VERY IMPORTANT **********************************
  // add "lostTracks" to "packedPFCandidates"
  //
  /*std::vector<const pat::PackedCandidate *> thePATTrackHandle ;
  for (const pat::PackedCandidate &p1 : *thePATTrackHandle_Ori ) thePATTrackHandle.push_back(&p1);
  for (const pat::PackedCandidate &p2 : *thePATTrackHandle_lowpt ) thePATTrackHandle.push_back(&p2);*/
  //
  //
  // ******************** *******************************************
 

  //get genParticles 
  // Handle<GenParticleCollection> genParticles;
//   if (doMC_) 
//     {
  // iEvent.getByLabel(genParticles_, genParticles);
      //    }
    ////////////////////////////////////


// Get HLT results
 edm::Handle<edm::TriggerResults> triggerResults_handle;
 //iEvent.getByLabel(triggerResults_Label, triggerResults_handle);
 iEvent.getByToken(triggerResults_Label, triggerResults_handle);

 /*
  edm::Handle<edm::TriggerResults> triggerResults_handle;
  try {
    std::string const &trig = std::string("TriggerResults::")+triggerResults_Label;
    iEvent.getByLabel(edm::InputTag(trig),triggerResults_handle);
  }
  catch ( ... ) 
    {
      std::cout << "Couldn't get handle on HLT Trigger!" << endl;
    }
    
    //HLTConfigProvider hltConfig_;
    */

    std::vector<std::string> TrigTable; TrigTable.clear();
    // Get hold of trigger names - based on TriggerResults object
    const edm::TriggerNames& triggerNames1_ = iEvent.triggerNames(*triggerResults_handle);
    
    for (unsigned int itrig = 0; itrig < triggerResults_handle->size(); ++itrig){
        if ((*triggerResults_handle)[itrig].accept() == 1){
            std::string trigName1 = triggerNames1_.triggerName(itrig);
            //int trigPrescale = hltConfig_.prescaleValue(itrig, trigName1);
            TrigTable.push_back(trigName1);
        }
    }

    /*
    cout<< "Trigger table: ";
    for( unsigned int i = 0; i<TrigTable.size(); ++i)
        cout<<TrigTable.at(i) << " ";
    
    cout<< endl;
    */
    

    
    CheckHLTTriggers(TrigTable);

    /*//here condiction for HLT   
    string alltnames = triggersL;
    //Bool_t checkflag = false;
   
   //cout<<"*Trigger List "<< triggersL << endl;     

   string::size_type trigger1 = alltnames.find("HLT_Dimuon25_Jpsi",0);
   string::size_type trigger2 = alltnames.find("HLT_DoubleMu4_JpsiTrk_Displaced",0);
   string::size_type trigger3 = alltnames.find("HLT_DoubleMu4_3_Jpsi_Displaced",0);
   string::size_type trigger4 = alltnames.find("HLT_Dimuon20_Jpsi_Barrel_Seagulls",0);

   if(trigger4==string::npos)
     {
       //checkflag=true;return;
       cout<<"*Trigger HLT_Dimuon20_Jpsi_Barrel_Seagulls "<<endl;        
       return;
     } 
    */
    



  //*********************************
  //Now we get the primary vertex 
  //*********************************


 

  // get primary vertex
  reco::Vertex bestVtx;
  edm::Handle<std::vector<reco::Vertex> > recVtxs;
  iEvent.getByToken(primaryVertices_Label, recVtxs);

  bestVtx = *(recVtxs->begin());
  
  priVtxX = bestVtx.x();
  priVtxY = bestVtx.y();
  priVtxZ = bestVtx.z();
  //priVtxXE = bestVtx.xError();
  //priVtxYE = bestVtx.yError();
  //priVtxZE = bestVtx.zError();
  priVtxXE = bestVtx.covariance(0, 0);
  priVtxYE = bestVtx.covariance(1, 1);
  priVtxZE = bestVtx.covariance(2, 2);
  priVtxXYE = bestVtx.covariance(0, 1);
  priVtxXZE = bestVtx.covariance(0, 2);
  priVtxYZE = bestVtx.covariance(1, 2);
  priVtxCL = ChiSquaredProbability((double)(bestVtx.chi2()),(double)(bestVtx.ndof())); 
  
  nVtx = recVtxs->size();  
    
   
  //*********************************
  //Now we get the Beam Spot  
  //*********************************

  reco::BeamSpot beamSpot;
  edm::Handle<reco::BeamSpot> beamSpotHandle;
  iEvent.getByToken(BSLabel_, beamSpotHandle);
  //iEvent.getByLabel(BSLabel_, beamSpotHandle);
  if ( beamSpotHandle.isValid() ) beamSpot = *beamSpotHandle; 
  else std::cout << "No beam spot available from EventSetup" << endl;

  PVXBS = beamSpot.x0();
  PVYBS = beamSpot.y0();
  PVZBS = beamSpot.z0();
  PVXBSE = beamSpot.covariance()(0, 0);
  PVYBSE = beamSpot.covariance()(1, 1);
  PVZBSE = beamSpot.covariance()(2, 2);
  PVXYBSE = beamSpot.covariance()(0,1);
  PVXZBSE = beamSpot.covariance()(0,2);
  PVYZBSE = beamSpot.covariance()(1,2);

 
 lumiblock = iEvent.id().luminosityBlock();
 run = iEvent.id().run();
  event = iEvent.id().event();
  //int run1   =  iEvent.id().run();
  //int event1 =  iEvent.id().event();

 //*****************************************
  //Let's begin by looking for J/psi->mu+mu- bestVtx

  //std::cout<< " number of vertex:   " << recVtxs->size() << endl;
  //std::cout<< " tracks total number :   " << thePATTrackHandle->size() << endl;
  //std::cout<< " tracks number in the first vertex:   " << bestVtx.tracksSize() << endl;

  unsigned int nMu_tmp = thePATMuonHandle->size();
  for(View<pat::Muon>::const_iterator iMuon1 = thePATMuonHandle->begin(); iMuon1 != thePATMuonHandle->end(); ++iMuon1) 
    {
      
      for(View<pat::Muon>::const_iterator iMuon2 = iMuon1+1; iMuon2 != thePATMuonHandle->end(); ++iMuon2) 
	{
	  if(iMuon1==iMuon2) continue;
	  
	  //opposite charge 
	  if( (iMuon1->charge())*(iMuon2->charge()) == 1) continue;

	  const pat::Muon *patMuonP = 0;
	  const pat::Muon *patMuonM = 0;
	  TrackRef glbTrackP;	  
	  TrackRef glbTrackM;	  
	  
	  if(iMuon1->charge() == 1){ patMuonP = &(*iMuon1); glbTrackP = iMuon1->track();}
	  if(iMuon1->charge() == -1){patMuonM = &(*iMuon1); glbTrackM = iMuon1->track();}
	  
	  if(iMuon2->charge() == 1) {patMuonP = &(*iMuon2); glbTrackP = iMuon2->track();}
	  if(iMuon2->charge() == -1){patMuonM = &(*iMuon2); glbTrackM = iMuon2->track();}
	  
	  if( glbTrackP.isNull() || glbTrackM.isNull() ) 
	    {
	      //std::cout << "continue due to no track ref" << endl;
	      continue;
	    }

	  if(iMuon1->track()->pt()<4.0) continue;
	  if(iMuon2->track()->pt()<4.0) continue;

	  if(!(glbTrackM->quality(reco::TrackBase::highPurity))) continue;
	  if(!(glbTrackP->quality(reco::TrackBase::highPurity))) continue;
        
	  //if(fabs(iMuon1->eta())>2.2 || fabs(iMuon2->eta())>2.2) continue;
	 

	  //Let's check the vertex and mass

	  //TransientTrack muon1TT(glbTrackP, &(*bFieldHandle) );
	  //TransientTrack muon2TT(glbTrackM, &(*bFieldHandle) );
	  reco::TransientTrack muon1TT((*theB).build(glbTrackP));
	  reco::TransientTrack muon2TT((*theB).build(glbTrackM));

	  // *****  Trajectory states to calculate DCA for the 2 muons *********************
	  FreeTrajectoryState mu1State = muon1TT.impactPointTSCP().theState();
	  FreeTrajectoryState mu2State = muon2TT.impactPointTSCP().theState();

	  if( !muon1TT.impactPointTSCP().isValid() || !muon2TT.impactPointTSCP().isValid() ) continue;

	  // Measure distance between tracks at their closest approach
	  ClosestApproachInRPhi cApp;
	  cApp.calculate(mu1State, mu2State);
	  if( !cApp.status() ) continue;
	  float dca = fabs( cApp.distance() );	  
	  //if (dca < 0. || dca > 0.5) continue;
	  //cout<<" closest approach  "<<dca<<endl;

	  
	  //The mass of a muon and the insignificant mass sigma 
	  //to avoid singularities in the covariance matrix.
	  ParticleMass muon_mass = 0.10565837; //pdg mass
	  ParticleMass psi_mass = 3.096916;
	  float muon_sigma = muon_mass*1.e-6;
	  //float psi_sigma = psi_mass*1.e-6;

	  //Creating a KinematicParticleFactory
	  KinematicParticleFactoryFromTransientTrack pFactory;
	  
	  //initial chi2 and ndf before kinematic fits.
	  float chi = 0.;
	  float ndf = 0.;
	  vector<RefCountedKinematicParticle> muonParticles;
	  try {
	    muonParticles.push_back(pFactory.particle(muon1TT,muon_mass,chi,ndf,muon_sigma));
	    muonParticles.push_back(pFactory.particle(muon2TT,muon_mass,chi,ndf,muon_sigma));
	  }
	  catch(...) { 
	    std::cout<<" Exception caught ... continuing 1 "<<std::endl; 
	    continue;
	  }

	  KinematicParticleVertexFitter fitter;   

	  RefCountedKinematicTree psiVertexFitTree;
	  try {
	    psiVertexFitTree = fitter.fit(muonParticles); 
	  }
	  catch (...) { 
	    std::cout<<" Exception caught ... continuing 2 "<<std::endl; 
	    continue;
	  }

	  if (!psiVertexFitTree->isValid()) 
	    {
	      //std::cout << "caught an exception in the psi vertex fit" << std::endl;
	      continue; 
	    }

	  psiVertexFitTree->movePointerToTheTop();
	  
	  RefCountedKinematicParticle psi_vFit_noMC = psiVertexFitTree->currentParticle();
	  RefCountedKinematicVertex psi_vFit_vertex_noMC = psiVertexFitTree->currentDecayVertex();
	  
	  if( psi_vFit_vertex_noMC->chiSquared() < 0 )
	    {
	      //std::cout << "negative chisq from psi fit" << endl;
	      continue;
	    }

	   //some loose cuts go here

	   if(psi_vFit_vertex_noMC->chiSquared()>26.) continue;
	   if(psi_vFit_noMC->currentState().mass()<2.9 || psi_vFit_noMC->currentState().mass()>3.3) continue;
	   
	   //More quality cuts?


	   //Now that we have a J/psi candidate, we look for phi candidates

	   int PId1=0; int PId2=0;

	   for(View<pat::PackedCandidate>::const_iterator iTrack1 = thePATTrackHandle->begin();
	   iTrack1 != thePATTrackHandle->end(); ++iTrack1 )
	     {
	   /*for (unsigned int i = 0, n = thePATTrackHandle.size(); i<n ; ++i )
	     {
	     const pat::PackedCandidate *iTrack1 =  (thePATTrackHandle)[i];*/


               if(iTrack1->charge()==0) continue;
	       if(fabs(iTrack1->pdgId())!=211) continue;
	       if(iTrack1->pt()<0.5) continue;
	       
	       //pat::GenericParticle patTrack1 = *iTrack1;

	       //if(!(iTrack1->quality(reco::TrackBase::highPurity))) continue;
	       if(!(iTrack1->trackHighPurity())) continue;

	       int ngenT1 = 0;//PdgIDatTruthLevel(iTrack1, genParticles, PId1);


	       for(View<pat::PackedCandidate>::const_iterator iTrack2 = iTrack1+1;
	       iTrack2 != thePATTrackHandle->end(); ++iTrack2 ) 
		 {
	       //for(const pat::PackedCandidate *iTrack2: thePATTrackHandle )	       
	       /*for (unsigned int ii = i+1, nn = thePATTrackHandle.size(); ii<nn ; ++ii )
		 {
		 const pat::PackedCandidate *iTrack2 =  (thePATTrackHandle)[ii];*/
		 
		   if(iTrack1==iTrack2) continue;
		   if(iTrack2->charge()==0) continue;
		   if(fabs(iTrack2->pdgId())!=211) continue;
		   //if(iTrack1->charge() == iTrack2->charge()) continue;

		   if(iTrack2->pt()<0.5) continue;

		   //pat::GenericParticle patTrack2 = *iTrack2;

		   //if(!(iTrack2->quality(reco::TrackBase::highPurity))) continue;
		   if(!(iTrack2->trackHighPurity())) continue;

		   //Let's open a parentesis here to check about the truth mathced particles

		   //std::cout<< " por aca ando:   " << endl;


		   int ngenT2 = 0;//PdgIDatTruthLevel(iTrack2, genParticles, PId2);


		   //Now let's checks if our muons do not use the same tracks as we are using now
		   if ( IsTheSame(*iTrack1,*iMuon1) || IsTheSame(*iTrack1,*iMuon2) ) continue;
		   if ( IsTheSame(*iTrack2,*iMuon1) || IsTheSame(*iTrack2,*iMuon2) ) continue;

		   //Now let's see if these two tracks make a vertex

		   //TransientTrack pion1TT(iTrack1->pseudoTrack(), &(*bFieldHandle) );
		   //TransientTrack pion2TT(iTrack2->pseudoTrack(), &(*bFieldHandle) );
		   reco::TransientTrack pion1TT((*theB).build(iTrack1->pseudoTrack()));
		   reco::TransientTrack pion2TT((*theB).build(iTrack2->pseudoTrack()));

		   //ParticleMass pion_mass = 0.13957018;
		   //float pion_sigma = pion_mass*1.e-6;
		   ParticleMass kaon_mass = 0.493677;
		   float kaon_sigma = kaon_mass*1.e-6;
		   //float kaon_sigma = 0.000013;
		   //ParticleMass phi(1020)_mass = 1.019461;
		   //float phi_sigma = 0.000019; //


		   // ***************************
		   // pipi invariant mass (before kinematic vertex fit)
		   // ***************************
		   TLorentzVector pion14V,pion24V,pipi4V, Jpsi4V; 
		   //pion14V.SetXYZM(iTrack1->px(),iTrack1->py(),iTrack1->pz(),pion_mass);
		   pion14V.SetXYZM(iTrack1->px(),iTrack1->py(),iTrack1->pz(),kaon_mass);
		   pion24V.SetXYZM(iTrack2->px(),iTrack2->py(),iTrack2->pz(),kaon_mass);

		   pipi4V=pion14V+pion24V;
		   if(pipi4V.M()<0.970 || pipi4V.M()>1.070) continue;
                   //if(pipi4V.M()<0.750 || pipi4V.M()>1.050) continue;
	       
		   //initial chi2 and ndf before kinematic fits.
		   float chi = 0.;
		   float ndf = 0.;
		   /*
		   vector<RefCountedKinematicParticle> pionParticles;
		   // vector<RefCountedKinematicParticle> muonParticles;
		   try {
		     pionParticles.push_back(pFactory.particle(pion1TT,kaon_mass,chi,ndf,kaon_sigma));
		     pionParticles.push_back(pFactory.particle(pion2TT,kaon_mass,chi,ndf,kaon_sigma));
		   }
		   catch(...) {
		     std::cout<<" Exception caught ... continuing 3 "<<std::endl;
		     continue;
		   }

		   RefCountedKinematicTree phiVertexFitTree;
		   try{
		     phiVertexFitTree = fitter.fit(pionParticles); 
		   }
		   catch(...) {
		     std::cout<<" Exception caught ... continuing 4 "<<std::endl;                   
		     continue;
		   }
		   if (!phiVertexFitTree->isValid()) 
		     {
		       //std::cout << "invalid vertex from the phi vertex fit" << std::endl;
		       continue; 
		     }
		   phiVertexFitTree->movePointerToTheTop();

		   RefCountedKinematicParticle phi_vFit_noMC = phiVertexFitTree->currentParticle();
		   RefCountedKinematicVertex phi_vFit_vertex_noMC = phiVertexFitTree->currentDecayVertex();

		   if( phi_vFit_vertex_noMC->chiSquared() < 0 )
		     { 
		       //std::cout << "negative chisq from ks fit" << endl;
		       continue;
		     }

		   //some loose cuts go here
		   
		   //cout<<"Mass 1 : "<<phi_vFit_noMC->currentState().mass()<<endl;

		   if(phi_vFit_vertex_noMC->chiSquared()>26) continue;
		   //if(phi_vFit_noMC->currentState().mass()<0.850 || phi_vFit_noMC->currentState().mass()>1.090) continue;
		   if(phi_vFit_noMC->currentState().mass()<0.40 || phi_vFit_noMC->currentState().mass()>2.8) continue;
		   */

		   //Now that we have the J/psi and the phi, let's combine them

		   //First we do a constrained mass fit for the J/psi
		   /*
		   KinematicParticleFitter csFitter;
		   KinematicConstraint * Jpsi_c = new MassKinematicConstraint(psi_mass,psi_sigma);
	       
		   // Let's add this mass constraint to the J/psi fit to then do a vertex constrained fit:  
 
		   psiVertexFitTree = csFitter.fit(Jpsi_c,psiVertexFitTree);
		   
		   if (!psiVertexFitTree->isValid()){
		     //std::cout << "caught an exception in the J/psi mass + vertex constraint fit" << std::endl;
		     continue; 
		   }
	       
		   psiVertexFitTree->movePointerToTheTop();
		   RefCountedKinematicParticle psi_vFit_withMC = psiVertexFitTree->currentParticle();

		   if( psi_vFit_withMC->chiSquared() < 0 )
		     { 
		       //std::cout << "negative chisq from J/psi vertex+mass constrain fit" << endl;
		       continue;
		     }
		   */



		   // ***************************
		   // Bs invariant mass (before kinematic vertex fit)
		   // ***************************

		   Jpsi4V.SetXYZM(psi_vFit_noMC->currentState().globalMomentum().x(),psi_vFit_noMC->currentState().globalMomentum().y(),psi_vFit_noMC->currentState().globalMomentum().z(),psi_vFit_noMC->currentState().mass());

		   //if (fabs((phi4V + Jpsi4V).M() - bs_mass) > .600) continue;
		   if ( (pipi4V + Jpsi4V).M()<4.4 || (pipi4V + Jpsi4V).M()>6.3 ) continue;


		   //std::cout<< " por aca ando 2:   " << endl;


		   vector<RefCountedKinematicParticle> vFitMCParticles;
		   vFitMCParticles.push_back(pFactory.particle(muon1TT,muon_mass,chi,ndf,muon_sigma));
		   vFitMCParticles.push_back(pFactory.particle(muon2TT,muon_mass,chi,ndf,muon_sigma));
		   //vFitMCParticles.push_back(pFactory.particle(pion1TT,pion_mass,chi,ndf,pion_sigma));
		   vFitMCParticles.push_back(pFactory.particle(pion1TT,kaon_mass,chi,ndf,kaon_sigma));
		   vFitMCParticles.push_back(pFactory.particle(pion2TT,kaon_mass,chi,ndf,kaon_sigma));


                   // JPsi mass constraint is applied in the final Bs fit,                                                               
                   MultiTrackKinematicConstraint *  j_psi_c = new  TwoTrackMassKinematicConstraint(psi_mass);
                   KinematicConstrainedVertexFitter kcvFitter;
                   RefCountedKinematicTree vertexFitTree = kcvFitter.fit(vFitMCParticles, j_psi_c);
                   if (!vertexFitTree->isValid()) {
                     //std::cout << "caught an exception in the B vertex fit with MC" << std::endl;                                        
                     continue;
                   }


		   //std::cout<< " por aca ando 3:   " << endl;


		   vertexFitTree->movePointerToTheTop();
		   RefCountedKinematicParticle bCandMC = vertexFitTree->currentParticle();
		   RefCountedKinematicVertex bDecayVertexMC = vertexFitTree->currentDecayVertex();
		   if (!bDecayVertexMC->vertexIsValid()){
		     //std::cout << "B MC fit vertex is not valid" << endl;
		     continue;
		   }
		 
		   //std::cout<< " por aca ando 4:   " << endl;
	       

		   if(bCandMC->currentState().mass()<5.0 || bCandMC->currentState().mass()>5.7) continue;
		   //std::cout<<" Mass:  "<<bCandMC->currentState().mass()<<std::endl;

		   //std::cout<< " por aca ando 5:   " << endl;

		   
		   if(bDecayVertexMC->chiSquared()<0 || bDecayVertexMC->chiSquared()>26 ) 
		     {
		       //if ( bDecayVertexMC->chiSquared()<0 ) 
		       //std::cout << " continue from negative chi2 = " << bDecayVertexMC->chiSquared() << endl;
		       //else std::cout << " continue from bad chi2 = " << bDecayVertexMC->chiSquared() << endl;
		       continue;
		     }
		   

		   //std::cout<< " por aca ando 6:   " << endl;


		   // get children from final B fit

		   vertexFitTree->movePointerToTheFirstChild();
                   RefCountedKinematicParticle mu1CandMC = vertexFitTree->currentParticle();

                   vertexFitTree->movePointerToTheNextChild();
                   RefCountedKinematicParticle mu2CandMC = vertexFitTree->currentParticle();


                   vertexFitTree->movePointerToTheNextChild();
                   RefCountedKinematicParticle T1CandMC = vertexFitTree->currentParticle();

                   vertexFitTree->movePointerToTheNextChild();
                   RefCountedKinematicParticle T2CandMC = vertexFitTree->currentParticle();


		   KinematicParameters psiMu1KP = mu1CandMC->currentState().kinematicParameters();
		   KinematicParameters psiMu2KP = mu2CandMC->currentState().kinematicParameters();
		   

		   KinematicParameters phiPi1KP = T1CandMC->currentState().kinematicParameters();
		   KinematicParameters phiPi2KP = T2CandMC->currentState().kinematicParameters();
		   
		

		   if(iMuon1->charge() == 1) mupCategory = getMuCat( *iMuon1 );
		   if(iMuon1->charge() == -1) mumCategory = getMuCat( *iMuon1 );
		   if(iMuon2->charge() == 1) mupCategory = getMuCat( *iMuon2 );
		   if(iMuon2->charge() == -1) mumCategory = getMuCat( *iMuon2 );

		   const reco::Muon *recoMuonM = patMuonM;
		   const reco::Muon *recoMuonP = patMuonP;
		  
		   
		   //KinematicParameters VCandKP = phiCandMC->currentState().kinematicParameters();
		   
		   //std::cout << "filling new candidate" << endl;
	       
		   // fill candidate variables now

		   // ************
		   
		  // Only save the first time
		   if(nB==0){


		     //CheckHLTTriggers(TrigTable);
		     // cout<<"*Trigger List "<< triggersL << endl;
		     // Save number of Muons

		    
		     nMu  = nMu_tmp;
		     // cout<< "*Number of Muons : " << nMu_tmp << endl;
		     
		   // Get number of run and event
		     //run   =  run1;
		     //event =  event1;
		     
		     // cout<< "* Run: "<< run << "  event: " << event << endl;

		   } // end nB==0

 

 // ********************* todos los vertices primarios y escogemos el de mejor pointing angle **************** 
		   reco::Vertex bestVtxIP;

		   int vertex_id_temp = -1000;

		   Double_t pVtxIPX_temp = -10000.0;
		   Double_t pVtxIPY_temp = -10000.0;
		   Double_t pVtxIPZ_temp = -10000.0;
		   Double_t pVtxIPXE_temp = -10000.0;
		   Double_t pVtxIPYE_temp = -10000.0;
		   Double_t pVtxIPZE_temp = -10000.0;
		   Double_t pVtxIPXYE_temp = -10000.0;
		   Double_t pVtxIPXZE_temp = -10000.0;
		   Double_t pVtxIPYZE_temp = -10000.0;
		   Double_t pVtxIPCL_temp = -10000.0;	
		   Double_t lip1 = -1000000.0;
		     for(size_t i = 0; i < recVtxs->size(); ++i) {
		       const Vertex &vtx = (*recVtxs)[i];
		       
		       Double_t dx1 = (*bDecayVertexMC).position().x() - vtx.x(); 
		       Double_t dy1 = (*bDecayVertexMC).position().y() - vtx.y();
		       Double_t dz1 = (*bDecayVertexMC).position().z() - vtx.z();
		       float cosAlphaXYb1 = ( bCandMC->currentState().globalMomentum().x() * dx1 + bCandMC->currentState().globalMomentum().y()*dy1 + bCandMC->currentState().globalMomentum().z()*dz1  )/( sqrt(dx1*dx1+dy1*dy1+dz1*dz1)* bCandMC->currentState().globalMomentum().mag() );

		       if(cosAlphaXYb1>lip1)
			 {
			   lip1 = cosAlphaXYb1 ;
			   pVtxIPX_temp = vtx.x();
			   pVtxIPY_temp = vtx.y();
			   pVtxIPZ_temp = vtx.z();
			   pVtxIPXE_temp = vtx.covariance(0, 0);
			   pVtxIPYE_temp = vtx.covariance(1, 1);
			   pVtxIPZE_temp = vtx.covariance(2, 2);
			   pVtxIPXYE_temp = vtx.covariance(0, 1);
			   pVtxIPXZE_temp = vtx.covariance(0, 2);
			   pVtxIPYZE_temp = vtx.covariance(1, 2);
			   pVtxIPCL_temp = (TMath::Prob(vtx.chi2(),(int)vtx.ndof()) );

			   bestVtxIP = vtx;
			   vertex_id_temp = i;
			 
			 }
                 
		     }
		   pVtxIPX->push_back( pVtxIPX_temp);
		   pVtxIPY->push_back(  pVtxIPY_temp);	    
		   pVtxIPZ->push_back(  pVtxIPZ_temp);
		   pVtxIPXE->push_back( pVtxIPXE_temp);
		   pVtxIPYE->push_back( pVtxIPYE_temp);	    
		   pVtxIPZE->push_back( pVtxIPZE_temp);
		   pVtxIPXYE->push_back( pVtxIPXYE_temp);
		   pVtxIPXZE->push_back( pVtxIPXZE_temp);	    
		   pVtxIPYZE->push_back( pVtxIPYZE_temp);
		   pVtxIPCL->push_back(  pVtxIPCL_temp);

		   //std::cout<<"vertex id   "<<vertex_id_temp<<endl;
		   
		   vertex_id->push_back(  vertex_id_temp);
  

		   B_mass->push_back(bCandMC->currentState().mass());
		   B_px->push_back(bCandMC->currentState().globalMomentum().x());
		   B_py->push_back(bCandMC->currentState().globalMomentum().y());
		   B_pz->push_back(bCandMC->currentState().globalMomentum().z());

		   B_phi_mass->push_back( pipi4V.M() );
		   /*B_phi_mass->push_back( phi_vFit_noMC->currentState().mass() );
		   B_phi_px->push_back( phi_vFit_noMC->currentState().globalMomentum().x() );
		   B_phi_py->push_back( phi_vFit_noMC->currentState().globalMomentum().y() );
		   B_phi_pz->push_back( phi_vFit_noMC->currentState().globalMomentum().z() );*/
		   B_phi_parentId1->push_back(PId1);
		   B_phi_parentId2->push_back(PId2);
		   //B_phi_chi2->push_back(ngenT1);

		   B_J_mass->push_back( psi_vFit_noMC->currentState().mass() );
		   B_J_px->push_back( psi_vFit_noMC->currentState().globalMomentum().x() );
		   B_J_py->push_back( psi_vFit_noMC->currentState().globalMomentum().y() );
		   B_J_pz->push_back( psi_vFit_noMC->currentState().globalMomentum().z() );

		   B_phi_pId1->push_back(ngenT1);
		   //B_phi_pt1->push_back(phip1vec.perp());
		   B_phi_px1->push_back(phiPi1KP.momentum().x());
		   B_phi_py1->push_back(phiPi1KP.momentum().y());
		   B_phi_pz1->push_back(phiPi1KP.momentum().z());
		   B_phi_px1_track->push_back(iTrack1->px());
		   B_phi_py1_track->push_back(iTrack1->py());
		   B_phi_pz1_track->push_back(iTrack1->pz());
		   B_phi_charge1->push_back(T1CandMC->currentState().particleCharge());

		   B_phi_pId2->push_back(ngenT2);
		   //B_phi_pt2->push_back(phip2vec.perp());
		   B_phi_px2->push_back(phiPi2KP.momentum().x());
		   B_phi_py2->push_back(phiPi2KP.momentum().y());
		   B_phi_pz2->push_back(phiPi2KP.momentum().z());
		   B_phi_px2_track->push_back(iTrack2->px());
		   B_phi_py2_track->push_back(iTrack2->py());
		   B_phi_pz2_track->push_back(iTrack2->pz());
		   B_phi_charge2->push_back(T2CandMC->currentState().particleCharge());

		   //B_J_pt1->push_back(Jp1vec.perp());
		   B_J_px1->push_back(psiMu1KP.momentum().x());
		   B_J_py1->push_back(psiMu1KP.momentum().y());
		   B_J_pz1->push_back(psiMu1KP.momentum().z());
		   B_J_charge1->push_back(mu1CandMC->currentState().particleCharge());

		   //B_J_pt2->push_back(Jp2vec.perp());
		   B_J_px2->push_back(psiMu2KP.momentum().x());
		   B_J_py2->push_back(psiMu2KP.momentum().y());
		   B_J_pz2->push_back(psiMu2KP.momentum().z());
		   B_J_charge2->push_back(mu2CandMC->currentState().particleCharge());

		   //B_phi_chi2->push_back(phi_vFit_vertex_noMC->chiSquared());
		   B_J_chi2->push_back(psi_vFit_vertex_noMC->chiSquared());
		   B_chi2->push_back(bDecayVertexMC->chiSquared());
             
		   double B_Prob_tmp       = TMath::Prob(bDecayVertexMC->chiSquared(),(int)bDecayVertexMC->degreesOfFreedom());
		   double Omb_J_Prob_tmp   = TMath::Prob(psi_vFit_vertex_noMC->chiSquared(),(int)psi_vFit_vertex_noMC->degreesOfFreedom());
		   //double Omb_phi_Prob_tmp  = TMath::Prob(phi_vFit_vertex_noMC->chiSquared(),(int)phi_vFit_vertex_noMC->degreesOfFreedom());
		   B_Prob    ->push_back(B_Prob_tmp);
		   B_J_Prob  ->push_back(Omb_J_Prob_tmp);
		   //B_phi_Prob ->push_back(Omb_phi_Prob_tmp);


		   B_DecayVtxX ->push_back((*bDecayVertexMC).position().x());    
		   B_DecayVtxY ->push_back((*bDecayVertexMC).position().y());
		   B_DecayVtxZ ->push_back((*bDecayVertexMC).position().z());
		   B_DecayVtxXE ->push_back(bDecayVertexMC->error().cxx());   
		   B_DecayVtxYE ->push_back(bDecayVertexMC->error().cyy());   
		   B_DecayVtxZE ->push_back(bDecayVertexMC->error().czz());
		   B_DecayVtxXYE ->push_back(bDecayVertexMC->error().cyx());
		   B_DecayVtxXZE ->push_back(bDecayVertexMC->error().czx());
		   B_DecayVtxYZE ->push_back(bDecayVertexMC->error().czy());

		   /*B_phi_DecayVtxX ->push_back( phi_vFit_vertex_noMC->position().x() ); 
		   B_phi_DecayVtxY ->push_back( phi_vFit_vertex_noMC->position().y() );
		   B_phi_DecayVtxZ ->push_back( phi_vFit_vertex_noMC->position().z() );
		   B_phi_DecayVtxXE ->push_back( phi_vFit_vertex_noMC->error().cxx() );
		   B_phi_DecayVtxYE ->push_back( phi_vFit_vertex_noMC->error().cyy() );
		   B_phi_DecayVtxZE ->push_back( phi_vFit_vertex_noMC->error().czz() );
		   B_phi_DecayVtxXYE ->push_back( phi_vFit_vertex_noMC->error().cyx() );
		   B_phi_DecayVtxXZE ->push_back( phi_vFit_vertex_noMC->error().czx() );
		   B_phi_DecayVtxYZE ->push_back( phi_vFit_vertex_noMC->error().czy() );*/

		   B_J_DecayVtxX ->push_back( psi_vFit_vertex_noMC->position().x() );
	           B_J_DecayVtxY ->push_back( psi_vFit_vertex_noMC->position().y() );
	           B_J_DecayVtxZ ->push_back( psi_vFit_vertex_noMC->position().z() );
		   B_J_DecayVtxXE ->push_back( psi_vFit_vertex_noMC->error().cxx() );
	           B_J_DecayVtxYE ->push_back( psi_vFit_vertex_noMC->error().cyy() );
	           B_J_DecayVtxZE ->push_back( psi_vFit_vertex_noMC->error().czz() );
	           B_J_DecayVtxXYE ->push_back( psi_vFit_vertex_noMC->error().cyx() );
	           B_J_DecayVtxXZE ->push_back( psi_vFit_vertex_noMC->error().czx() );
	           B_J_DecayVtxYZE ->push_back( psi_vFit_vertex_noMC->error().czy() );	 

   // ********************* muon-trigger-machint **************** 
		   

		   // here we will check for muon-trigger-machint  (iMuon1 and iMuon2)

		   //const pat::TriggerObjectStandAloneCollection muHLTMatches1 = (dynamic_cast<const pat::Muon*>(iMuon1))->triggerObjectMatchesByFilter("hltDisplacedmumuFilterDimuon10JpsiBarrel");
		   //const pat::TriggerObjectStandAloneCollection muHLTMatches2 = (dynamic_cast<const pat::Muon*>(iMuon2))->triggerObjectMatchesByFilter("hltDisplacedmumuFilterDimuon16Jpsi");
		   const pat::TriggerObjectStandAloneCollection muHLTMatches1_t1 = iMuon1->triggerObjectMatchesByFilter("hltDisplacedmumuFilterDimuon25Jpsis");
		   const pat::TriggerObjectStandAloneCollection muHLTMatches2_t1 = iMuon2->triggerObjectMatchesByFilter("hltDisplacedmumuFilterDimuon25Jpsis");
		   
		   const pat::TriggerObjectStandAloneCollection muHLTMatches1_t2 = iMuon1->triggerObjectMatchesByFilter("hltJpsiTkVertexFilter");
		   const pat::TriggerObjectStandAloneCollection muHLTMatches2_t2 = iMuon2->triggerObjectMatchesByFilter("hltJpsiTkVertexFilter");

		   const pat::TriggerObjectStandAloneCollection muHLTMatches1_t3 = iMuon1->triggerObjectMatchesByFilter("hltDisplacedmumuFilterDoubleMu43Jpsi");
		   const pat::TriggerObjectStandAloneCollection muHLTMatches2_t3 = iMuon2->triggerObjectMatchesByFilter("hltDisplacedmumuFilterDoubleMu43Jpsi");

		    const pat::TriggerObjectStandAloneCollection muHLTMatches1_t4 = iMuon1->triggerObjectMatchesByFilter("hltDisplacedmumuFilterDimuon20JpsiBarrelnoCow");
		   const pat::TriggerObjectStandAloneCollection muHLTMatches2_t4 = iMuon2->triggerObjectMatchesByFilter("hltDisplacedmumuFilterDimuon20JpsiBarrelnoCow");
		   
		   int tri_Dim25_tmp = 10, tri_JpsiTk_tmp = 10,  tri_DoubleMu43Jpsi_tmp = 10, tri_Dim20_tmp = 10;

		   if (muHLTMatches1_t1.size() > 0 && muHLTMatches2_t1.size() > 0)
		     {
		       //std::cout<< " it is trigger HLT_Dimuon16_Jpsi " << endl;
		      tri_Dim25_tmp = 1;
		     }
		   else
		     {
		      tri_Dim25_tmp = 0;
		     }

		   if (muHLTMatches1_t2.size() > 0 && muHLTMatches2_t2.size() > 0)
		     {
		       //std::cout<< " it is trigger HLT_DoubleMu4_JpsiTrk_Displaced " << endl;
		       tri_JpsiTk_tmp = 1;
		     }
		   else
		     {
		      tri_JpsiTk_tmp = 0;
		     }

		   if (muHLTMatches1_t3.size() > 0 && muHLTMatches2_t3.size() > 0)
		     {
		       //std::cout<< " it is trigger  HLT_DoubleMu4_3_Jpsi_Displaced" << endl;
		       tri_DoubleMu43Jpsi_tmp = 1;
		     }
		   else
		     {
		      tri_DoubleMu43Jpsi_tmp = 0;
		     }

		   if (muHLTMatches1_t4.size() > 0 && muHLTMatches2_t4.size() > 0)
		     {
		       //std::cout<< " it is trigger HLT_Dimuon20_Jpsi " << endl;
		       tri_Dim20_tmp = 1;
		     }
		   else
		     {
		      tri_Dim20_tmp = 0;
		     }

		   tri_Dim25->push_back( tri_Dim25_tmp );	       
		   tri_JpsiTk->push_back( tri_JpsiTk_tmp );
		   tri_DoubleMu43Jpsi->push_back( tri_DoubleMu43Jpsi_tmp );
		   tri_Dim20->push_back( tri_Dim20_tmp );

 
            

	   // ************

		   mu1soft->push_back(iMuon1->isSoftMuon(bestVtxIP) );
		   mu2soft->push_back(iMuon2->isSoftMuon(bestVtxIP) );
		   mu1tight->push_back(iMuon1->isTightMuon(bestVtxIP) );
		   mu2tight->push_back(iMuon2->isTightMuon(bestVtxIP) );
		   mu1PF->push_back(iMuon1->isPFMuon());
		   mu2PF->push_back(iMuon2->isPFMuon());
		   mu1loose->push_back(muon::isLooseMuon(*iMuon1));
		   mu2loose->push_back(muon::isLooseMuon(*iMuon2));

		   mumC2->push_back( glbTrackM->normalizedChi2() );
		   mumCat->push_back( mumCategory );
		   //mumAngT->push_back( muon::isGoodMuon(*recoMuonM,muon::TMLastStationAngTight) ); // este no se  que es
		   mumAngT->push_back( muon::isGoodMuon(*recoMuonM,muon::TMOneStationTight) ); // este es para poner la condicion si es o no softmuon
		   mumNHits->push_back( glbTrackM->numberOfValidHits() );
		   mumNPHits->push_back( glbTrackM->hitPattern().numberOfValidPixelHits() );	       
		   mupC2->push_back( glbTrackP->normalizedChi2() );
		   mupCat->push_back( mupCategory );
		   //mupAngT->push_back( muon::isGoodMuon(*recoMuonP,muon::TMLastStationAngTight) );  // este no se  que es
		   mupAngT->push_back( muon::isGoodMuon(*recoMuonP,muon::TMOneStationTight) ); // este es para poner la condicion si es o no softmuon
		   mupNHits->push_back( glbTrackP->numberOfValidHits() );
		   mupNPHits->push_back( glbTrackP->hitPattern().numberOfValidPixelHits() );
                   mumdxy->push_back(glbTrackM->dxy(bestVtxIP.position()) );// el dxy del Muon negatico respetcto del PV con BSc (el de mayor pt)
		   mupdxy->push_back(glbTrackP->dxy(bestVtxIP.position()) );// el dxy del Muon positivo respetcto del PV con BSc (el de mayor pt)
		   mumdz->push_back(glbTrackM->dz(bestVtxIP.position()) );
		   mupdz->push_back(glbTrackP->dz(bestVtxIP.position()) );
		   muon_dca->push_back(dca);
		   //cout<<" closest approach  "<<dca<<endl;

		   k1dxy->push_back(iTrack1->dxy());
		   k2dxy->push_back(iTrack2->dxy());
		   k1dz->push_back(iTrack1->dz());
		   k2dz->push_back(iTrack2->dz());

		   k1dxy_e->push_back(iTrack1->dxyError());
		   k2dxy_e->push_back(iTrack2->dxyError());
		   k1dz_e->push_back(iTrack1->dzError());
		   k2dz_e->push_back(iTrack2->dzError());


		   /////////////////////////////////////////////////////
		   //patMuonP
		   /*reco::VertexRef::key_type mu1key = -1;
		  reco::VertexRef::key_type mu2key = -1;

		  //mu1key = iMuon1->originalObjectRef()->vertexRef().key();
		  //mu2key = iMuon2->originalObjectRef()->vertexRef().key();

		  //pat::PackedCandidate pfmu1 = (dynamic_cast<const pat::PackedCandidate *>(iMuon1->embedPFCandidate()) );
		  //pat::PackedCandidate pfmu2 = (dynamic_cast<const pat::PackedCandidate *>(iMuon2->embedPFCandidate()) );
		  //pat::PackedCandidate pfmu1 = (dynamic_cast<const pat::PackedCandidate *>(iMuon1->originalObjectRef()));
		  //pat::PackedCandidate pfmu2 = (dynamic_cast<const pat::PackedCandidate *>(iMuon2->originalObjectRef()));
		  //mu1key = pfmu1.vertexRef().key();
		  //mu2key = pfmu2.vertexRef().key();
		  //mu1key = iMuon1->embedPFCandidate()->vertexRef().key();
		  //mu2key = iMuon2->embedPFCandidate()->vertexRef().key();

		  
		  for(View<pat::PackedCandidate>::const_iterator jtrackmu = thePATTrackHandle->begin();
		       jtrackmu != thePATTrackHandle->end(); ++jtrackmu ) 
		     {
		       //if(jtrackmu->charge()==0) continue;
		       //if(fabs(jtrackmu->pdgId())!=211) continue;
		       if(fabs(jtrackmu->pdgId())!=13) continue;

		       if ( IsTheSame(*jtrackmu,*iMuon1) )
			 {
			   mu1key = jtrackmu->vertexRef().key();
			   //matchmu1= true;
			 }
		       
		       if ( IsTheSame(*jtrackmu,*iMuon2) )
			 {
			   mu2key = jtrackmu->vertexRef().key();
			   //matchmu1= true;
			 }

		    }
		  
		  if (mu1key != mu2key) continue;

		  Rfvtx_key->push_back( mu1key );
		  std::cout<<"vertexRf key   "<<mu1key<<endl;
		  */
		   
		  vector<reco::TransientTrack> vertexTracks;
		    
		  for(View<pat::PackedCandidate>::const_iterator jtrack =thePATTrackHandle->begin();
		  jtrack != thePATTrackHandle->end(); ++jtrack )
		    {
		      /*for (unsigned int j = 0, m = thePATTrackHandle.size(); j<m ; ++j )
		       {
		       const pat::PackedCandidate *jtrack  =  (thePATTrackHandle)[j];*/
		     

		       if(iTrack1==jtrack || iTrack2==jtrack ) continue;		       
		       if ( IsTheSame(*jtrack,*iMuon1) || IsTheSame(*jtrack,*iMuon2) )continue;
		       if(jtrack->charge()==0) continue;
		       if(fabs(jtrack->pdgId())!=211) continue;

		       if(jtrack->pt()<0.5) continue;
		       //if(!(jtrack->trackHighPurity())) continue;

		       //if( jtrack->fromPV(mu1key)!=3 ) continue;
		       
		       reco::VertexRef pvRef = jtrack->vertexRef();
		       
		       //if(jtrack->pvAssociationQuality()==UsedInFitTight and pvRef.key()==0)
		       //if(jtrack->pvAssociationQuality()!=UsedInFitTight || pvRef.key()!=0)continue;
		       if(jtrack->pvAssociationQuality()!=pat::PackedCandidate::UsedInFitTight || pvRef.key()!=0)continue;
		       //if(jtrack->pvAssociationQuality()!=7)continue;
		       //if(jtrack->pvAssociationQuality()!=7 || pvRef.key()!=0)continue;
		       //if( !(pat::jtrack::UsedInFitTight) || pvRef.key()!=0)continue;
		       //std::cout<<jtrack->pvAssociationQuality()<<endl;

		       reco::TransientTrack tt((*theB).build(jtrack->pseudoTrack()));
		       //reco::TransientTrack tt((*theB).build(jtrack->pseudoTrack(), beamSpot));
		       vertexTracks.push_back(tt);
		       /*trackrefPVpt->push_back(jtrack->pt());
		       trackrefPVdxy->push_back(jtrack->dxy());
		       trackrefPVdxy_e->push_back(jtrack->dxyError());
		       trackrefPVdz->push_back(jtrack->dz());
		       trackrefPVdz_e->push_back(jtrack->dzError());
		       */
			
		       
		     }
		   //std::cout << "tracks vector size:  "<< vertexTracks.size() << std::endl;
		   priRfNTrkDif->push_back(vertexTracks.size());

		   
		   //reco::Vertex refitvertex = bestVtxIP;bestVtx
		   reco::Vertex refitvertex = bestVtx;

		   //reco::Vertex refitvertex;

		   if (  vertexTracks.size()>1  ) 
		     {
		       AdaptiveVertexFitter theFitter;
		       //TransientVertex v = theFitter.vertex(vertexTracks, beamSpot);  // if you want the beam-spot constraint
		       TransientVertex v = theFitter.vertex(vertexTracks);
		       if ( v.isValid() ) {
			 //reco::Vertex recoV = (reco::Vertex)v;		       
			 refitvertex = reco::Vertex(v);	
		       } 
		     }
		   	   
		   //std::cout << "vertex x position:  "<<refitvertex.x() << "    " << bestVtxIP.x()<< std::endl;
		   //std::cout << "vertex y position:  "<<refitvertex.y() << "    " << bestVtxIP.y()<< std::endl;
		   //std::cout << "vertex z position:  "<<refitvertex.z() << "    " << bestVtxIP.z()<< std::endl;
		   

		   priRfVtxX->push_back( refitvertex.x() );
                   priRfVtxY->push_back( refitvertex.y() );
                   priRfVtxZ->push_back( refitvertex.z() );
                   priRfVtxXE->push_back( refitvertex.covariance(0, 0) );
                   priRfVtxYE->push_back( refitvertex.covariance(1, 1) );
                   priRfVtxZE->push_back( refitvertex.covariance(2, 2) );
                   priRfVtxXYE->push_back( refitvertex.covariance(0, 1) );
                   priRfVtxXZE->push_back( refitvertex.covariance(0, 2) );
                   priRfVtxYZE->push_back( refitvertex.covariance(1, 2) );

                   priRfVtxCL->push_back( ChiSquaredProbability((double)(refitvertex.chi2()),(double)(refitvertex.ndof())) );
		   

		    
		   
		   
		   nB++;	       


		   
		   /////////////////////////////////////////////////
		   
		   //pionParticles.clear();
		   muonParticles.clear();
		   vFitMCParticles.clear();

		 }
	     }
	}
    }
 
   
   //fill the tree and clear the vectors
   if (nB > 0 ) 
     {

       //std::cout << "filling tree" << endl;
       tree_->Fill();
     }
   // *********

   nB = 0; nMu = 0;

   //triggersL = 0; 


   B_mass->clear();    B_px->clear();    B_py->clear();    B_pz->clear();
   B_phi_mass->clear(); 
   //B_phi_px->clear(); B_phi_py->clear(); B_phi_pz->clear();
   B_phi_parentId1->clear(); B_phi_parentId2->clear();

   B_J_mass->clear();  B_J_px->clear();  B_J_py->clear();  B_J_pz->clear();

   B_phi_px1->clear(); B_phi_py1->clear(); B_phi_pz1->clear(); B_phi_charge1->clear(); B_phi_pId1->clear();
   B_phi_px2->clear(); B_phi_py2->clear(); B_phi_pz2->clear(); B_phi_charge2->clear(); B_phi_pId2->clear();

   B_phi_px1_track->clear(); B_phi_py1_track->clear(); B_phi_pz1_track->clear();
   B_phi_px2_track->clear(); B_phi_py2_track->clear(); B_phi_pz2_track->clear();

   //B_J_pt1->clear();
   B_J_px1->clear();  B_J_py1->clear();  B_J_pz1->clear(), B_J_charge1->clear();
   //B_J_pt2->clear();  
   B_J_px2->clear();  B_J_py2->clear();  B_J_pz2->clear(), B_J_charge2->clear();

   B_chi2->clear(); B_J_chi2->clear();
   B_Prob->clear(); B_J_Prob->clear(); 
   //B_phi_chi2->clear(); B_phi_Prob->clear();

   B_DecayVtxX->clear();     B_DecayVtxY->clear();     B_DecayVtxZ->clear();
   B_DecayVtxXE->clear();    B_DecayVtxYE->clear();    B_DecayVtxZE->clear();
   B_DecayVtxXYE->clear();   B_DecayVtxXZE->clear();   B_DecayVtxYZE->clear();

   /*B_phi_DecayVtxX->clear();  B_phi_DecayVtxY->clear();  B_phi_DecayVtxZ->clear();
   B_phi_DecayVtxXE->clear(); B_phi_DecayVtxYE->clear(); B_phi_DecayVtxZE->clear();
   B_phi_DecayVtxXYE->clear(); B_phi_DecayVtxXZE->clear(); B_phi_DecayVtxYZE->clear();*/

   B_J_DecayVtxX->clear();   B_J_DecayVtxY->clear();   B_J_DecayVtxZ->clear();
   B_J_DecayVtxXE->clear();  B_J_DecayVtxYE->clear();  B_J_DecayVtxZE->clear();
   B_J_DecayVtxXYE->clear(); B_J_DecayVtxXZE->clear(); B_J_DecayVtxYZE->clear();


   nVtx = 0; 
   vertex_id ->clear();
   priVtxX = 0;     priVtxY = 0;     priVtxZ = 0; 
   priVtxXE = 0;    priVtxYE = 0;    priVtxZE = 0; priVtxCL = 0;
   priVtxXYE = 0;   priVtxXZE = 0;   priVtxYZE = 0; 
   
  

   // vertice primario CON mejor pointin-angle
   pVtxIPX->clear();  pVtxIPY->clear();  pVtxIPZ->clear();
   pVtxIPXE->clear();  pVtxIPYE->clear();  pVtxIPZE->clear();  pVtxIPCL->clear();
   pVtxIPXYE->clear();  pVtxIPXZE->clear();  pVtxIPYZE->clear(); 

  
   
   PVXBS = 0;     PVYBS = 0;     PVZBS = 0; 
   PVXBSE = 0;    PVYBSE = 0;    PVZBSE = 0; 
   PVXYBSE = 0;   PVXZBSE = 0;   PVYZBSE = 0; 

   // *********                                                                                                                                                                                                    

   priRfVtxX->clear(); priRfVtxY->clear(); priRfVtxZ->clear(); priRfVtxXE->clear(); priRfVtxYE->clear();
   priRfVtxZE->clear(); priRfVtxXYE->clear(); priRfVtxXZE->clear(); priRfVtxYZE->clear(); priRfVtxCL->clear();
   
   priRfNTrkDif->clear(); 
   //Rfvtx_key->clear();
   //trackrefPVpt->clear(); trackrefPVdxy->clear(); trackrefPVdxy_e->clear(); trackrefPVdz->clear(); trackrefPVdz_e->clear();

   // *********

   k1dxy->clear(); k2dxy->clear(); k1dz->clear(); k2dz->clear();
   k1dxy_e->clear(); k2dxy_e->clear(); k1dz_e->clear(); k2dz_e->clear();


   mumC2->clear();
   mumCat->clear(); mumAngT->clear(); mumNHits->clear(); mumNPHits->clear();
   mupC2->clear();
   mupCat->clear(); mupAngT->clear(); mupNHits->clear(); mupNPHits->clear();
   mumdxy->clear(); mupdxy->clear(); mumdz->clear(); mupdz->clear(); muon_dca->clear();

   tri_Dim25->clear(); tri_Dim20->clear(); tri_JpsiTk->clear(); tri_DoubleMu43Jpsi->clear();

   mu1soft->clear(); mu2soft->clear(); mu1tight->clear(); mu2tight->clear();
   mu1PF->clear(); mu2PF->clear(); mu1loose->clear(); mu2loose->clear(); 
 
}

int const JPsiphiPAT::getMuCat(reco::Muon const& muon) const{
  int muCat = 0;
  if (muon.isGlobalMuon()) {
    if (muon.isTrackerMuon()) muCat = 1;
    else muCat = 10;
  }
  else if (muon.isTrackerMuon()) {
    if (muon::isGoodMuon(muon, muon::TrackerMuonArbitrated)) {
      if (muon::isGoodMuon(muon, muon::TMLastStationAngTight)) muCat = 6;
      else if (muon::isGoodMuon(muon, muon::TMOneStationTight)) muCat = 5;
      else if (muon::isGoodMuon(muon, muon::TMOneStationLoose)) muCat = 4;
      else muCat = 3;
    }
    else muCat = 2;
  }
  else if (muon.isStandAloneMuon()) muCat = 7;
  else if (muon.isCaloMuon()) muCat = 8;
  else muCat = 9;
  
  if ( !(muon::isGoodMuon(muon, muon::TMOneStationLoose)) && muon::isGoodMuon(muon, muon::TMOneStationTight) )
    std::cout << "inconsistent muon cat 1" << std::endl;
  if ( !(muon::isGoodMuon(muon, muon::TMOneStationTight)) && muon::isGoodMuon(muon, muon::TMLastStationAngTight) )
    std::cout << "inconsistent muon cat 2" << std::endl;

  return muCat;
}

bool JPsiphiPAT::IsTheSame(const pat::GenericParticle& tk, const pat::Muon& mu){
  double DeltaEta = fabs(mu.eta()-tk.eta());
  double DeltaP   = fabs(mu.p()-tk.p());
  if (DeltaEta < 0.02 && DeltaP < 0.02) return true;
  return false;
}




// ------------ method called once each job just before starting event loop  ------------

void 
JPsiphiPAT::beginJob()
{

  std::cout << "Beginning analyzer job with value of isMC_ = " << isMC_ << std::endl;

  edm::Service<TFileService> fs;
  tree_ = fs->make<TTree>("ntuple","Bs->J/psi phi ntuple");

  tree_->Branch("nB",&nB,"nB/i");
  tree_->Branch("nMu",&nMu,"nMu/i");


  tree_->Branch("B_mass", &B_mass);
  tree_->Branch("B_px", &B_px);
  tree_->Branch("B_py", &B_py);
  tree_->Branch("B_pz", &B_pz);

  tree_->Branch("B_phi_mass", &B_phi_mass);
  /*tree_->Branch("B_phi_px", &B_phi_px);
  tree_->Branch("B_phi_py", &B_phi_py);
  tree_->Branch("B_phi_pz", &B_phi_pz);*/
  tree_->Branch("B_phi_parentId1", &B_phi_parentId1);
  tree_->Branch("B_phi_parentId2", &B_phi_parentId2);

  tree_->Branch("B_J_mass", &B_J_mass);
  tree_->Branch("B_J_px", &B_J_px);
  tree_->Branch("B_J_py", &B_J_py);
  tree_->Branch("B_J_pz", &B_J_pz);

  //tree_->Branch("B_phi_pt1", &B_phi_pt1);
  tree_->Branch("B_phi_px1", &B_phi_px1);
  tree_->Branch("B_phi_py1", &B_phi_py1);
  tree_->Branch("B_phi_pz1", &B_phi_pz1);
  tree_->Branch("B_phi_px1_track", &B_phi_px1_track);
  tree_->Branch("B_phi_py1_track", &B_phi_py1_track);
  tree_->Branch("B_phi_pz1_track", &B_phi_pz1_track);
  tree_->Branch("B_phi_charge1", &B_phi_charge1); 
  tree_->Branch("B_phi_pId1", &B_phi_pId1);
 
  //tree_->Branch("B_phi_pt2", &B_phi_pt2);
  tree_->Branch("B_phi_px2", &B_phi_px2);
  tree_->Branch("B_phi_py2", &B_phi_py2);
  tree_->Branch("B_phi_pz2", &B_phi_pz2);
  tree_->Branch("B_phi_px2_track", &B_phi_px2_track);
  tree_->Branch("B_phi_py2_track", &B_phi_py2_track);
  tree_->Branch("B_phi_pz2_track", &B_phi_pz2_track);
  tree_->Branch("B_phi_charge2", &B_phi_charge2);
  tree_->Branch("B_phi_pId2", &B_phi_pId2);

  //tree_->Branch("B_J_pt1", &B_J_pt1);
  tree_->Branch("B_J_px1", &B_J_px1);
  tree_->Branch("B_J_py1", &B_J_py1);
  tree_->Branch("B_J_pz1", &B_J_pz1);
  tree_->Branch("B_J_charge1", &B_J_charge1);


  //tree_->Branch("B_J_pt2", &B_J_pt2);
  tree_->Branch("B_J_px2", &B_J_px2);
  tree_->Branch("B_J_py2", &B_J_py2);
  tree_->Branch("B_J_pz2", &B_J_pz2);
  tree_->Branch("B_J_charge2", &B_J_charge2);

  tree_->Branch("B_chi2",    &B_chi2);
  tree_->Branch("B_J_chi2",  &B_J_chi2);
  //tree_->Branch("B_phi_chi2", &B_phi_chi2);

  tree_->Branch("B_Prob",    &B_Prob);
  tree_->Branch("B_J_Prob",  &B_J_Prob);
  //tree_->Branch("B_phi_Prob", &B_phi_Prob);
     
  tree_->Branch("B_DecayVtxX",     &B_DecayVtxX);
  tree_->Branch("B_DecayVtxY",     &B_DecayVtxY);
  tree_->Branch("B_DecayVtxZ",     &B_DecayVtxZ);
  tree_->Branch("B_DecayVtxXE",    &B_DecayVtxXE);
  tree_->Branch("B_DecayVtxYE",    &B_DecayVtxYE);
  tree_->Branch("B_DecayVtxZE",    &B_DecayVtxZE);
  tree_->Branch("B_DecayVtxXYE",    &B_DecayVtxXYE);
  tree_->Branch("B_DecayVtxXZE",    &B_DecayVtxXZE);
  tree_->Branch("B_DecayVtxYZE",    &B_DecayVtxYZE);

  /*tree_->Branch("B_phi_DecayVtxX",  &B_phi_DecayVtxX);
  tree_->Branch("B_phi_DecayVtxY",  &B_phi_DecayVtxY);
  tree_->Branch("B_phi_DecayVtxZ",  &B_phi_DecayVtxZ);
  tree_->Branch("B_phi_DecayVtxXE", &B_phi_DecayVtxXE);
  tree_->Branch("B_phi_DecayVtxYE", &B_phi_DecayVtxYE);
  tree_->Branch("B_phi_DecayVtxZE", &B_phi_DecayVtxZE);
  tree_->Branch("B_phi_DecayVtxXYE", &B_phi_DecayVtxXYE);
  tree_->Branch("B_phi_DecayVtxXZE", &B_phi_DecayVtxXZE);
  tree_->Branch("B_phi_DecayVtxYZE", &B_phi_DecayVtxYZE);*/


  tree_->Branch("B_J_DecayVtxX",   &B_J_DecayVtxX);
  tree_->Branch("B_J_DecayVtxY",   &B_J_DecayVtxY);
  tree_->Branch("B_J_DecayVtxZ",   &B_J_DecayVtxZ);
  tree_->Branch("B_J_DecayVtxXE",  &B_J_DecayVtxXE);
  tree_->Branch("B_J_DecayVtxYE",  &B_J_DecayVtxYE);
  tree_->Branch("B_J_DecayVtxZE",  &B_J_DecayVtxZE);
  tree_->Branch("B_J_DecayVtxXYE",  &B_J_DecayVtxXYE);
  tree_->Branch("B_J_DecayVtxXZE",  &B_J_DecayVtxXZE);
  tree_->Branch("B_J_DecayVtxYZE",  &B_J_DecayVtxYZE);

  tree_->Branch("priVtxX",&priVtxX, "priVtxX/f");
  tree_->Branch("priVtxY",&priVtxY, "priVtxY/f");
  tree_->Branch("priVtxZ",&priVtxZ, "priVtxZ/f");
  tree_->Branch("priVtxXE",&priVtxXE, "priVtxXE/f");
  tree_->Branch("priVtxYE",&priVtxYE, "priVtxYE/f");
  tree_->Branch("priVtxZE",&priVtxZE, "priVtxZE/f");
  tree_->Branch("priVtxXYE",&priVtxXYE, "priVtxXYE/f");
  tree_->Branch("priVtxXZE",&priVtxXZE, "priVtxXZE/f");
  tree_->Branch("priVtxYZE",&priVtxYZE, "priVtxYZE/f");
  tree_->Branch("priVtxCL",&priVtxCL, "priVtxCL/f");

   tree_->Branch("pVtxIPX",     &pVtxIPX);
  tree_->Branch("pVtxIPY",     &pVtxIPY);
  tree_->Branch("pVtxIPZ",     &pVtxIPZ);
  tree_->Branch("pVtxIPXE",     &pVtxIPXE);
  tree_->Branch("pVtxIPYE",     &pVtxIPYE);
  tree_->Branch("pVtxIPZE",     &pVtxIPZE);
  tree_->Branch("pVtxIPXYE",     &pVtxIPXYE);
  tree_->Branch("pVtxIPXZE",     &pVtxIPXZE);
  tree_->Branch("pVtxIPYZE",     &pVtxIPYZE);
  tree_->Branch("pVtxIPCL",     &pVtxIPCL);

  tree_->Branch("PVXBS",&PVXBS, "PVXBS/D");
  tree_->Branch("PVYBS",&PVYBS, "PVYBS/D");
  tree_->Branch("PVZBS",&PVZBS, "PVZBS/D");
  tree_->Branch("PVXBSE",&PVXBSE, "PVXBSE/D");
  tree_->Branch("PVYBSE",&PVYBSE, "PVYBSE/D");
  tree_->Branch("PVZBSE",&PVZBSE, "PVZBSE/D");
  tree_->Branch("PVXYBSE",&PVXYBSE, "PVXYBSE/D");
  tree_->Branch("PVXZBSE",&PVXZBSE, "PVXZBSE/D");
  tree_->Branch("PVYZBSE",&PVYZBSE, "PVYZBSE/D");


  tree_->Branch("priRfVtxX",&priRfVtxX);
  tree_->Branch("priRfVtxY",&priRfVtxY);
  tree_->Branch("priRfVtxZ",&priRfVtxZ);
  tree_->Branch("priRfVtxXE",&priRfVtxXE);
  tree_->Branch("priRfVtxYE",&priRfVtxYE);
  tree_->Branch("priRfVtxZE",&priRfVtxZE);
  tree_->Branch("priRfVtxXYE",&priRfVtxXYE);
  tree_->Branch("priRfVtxXZE",&priRfVtxXZE);
  tree_->Branch("priRfVtxYZE",&priRfVtxYZE);
  tree_->Branch("priRfVtxCL",&priRfVtxCL);
  


  //tree_->Branch("Rfvtx_key",&Rfvtx_key);
  tree_->Branch("priRfNTrkDif",&priRfNTrkDif);

  /*tree_->Branch("trackrefPVpt",&trackrefPVpt);
  tree_->Branch("trackrefPVdxy",&trackrefPVdxy);
  tree_->Branch("trackrefPVdxy_e",&trackrefPVdxy_e);
  tree_->Branch("trackrefPVdz",&trackrefPVdz);
  tree_->Branch("trackrefPVdz_e",&trackrefPVdz_e);*/
  
  tree_->Branch("vertex_id", &vertex_id);
  tree_->Branch("nVtx",       &nVtx);
  tree_->Branch("run",        &run,       "run/I");
  tree_->Branch("event",        &event,     "event/I");
  tree_->Branch("lumiblock",&lumiblock,"lumiblock/I");
 
  tree_->Branch("nTrgL",      &nTrgL,    "nTrgL/I");  
  tree_->Branch("triggersL",         &triggersL,  "triggersL[nTrgL]/C");
    
  // *************************
  
  tree_->Branch("k1dxy",&k1dxy);
  tree_->Branch("k2dxy",&k2dxy);
  tree_->Branch("k1dz",&k1dz);
  tree_->Branch("k2dz",&k2dz);

  tree_->Branch("k1dxy_e",&k1dxy_e);
  tree_->Branch("k2dxy_e",&k2dxy_e);
  tree_->Branch("k1dz_e",&k1dz_e);
  tree_->Branch("k2dz_e",&k2dz_e);


  tree_->Branch("mumC2",&mumC2);  
  tree_->Branch("mumCat",&mumCat);
  tree_->Branch("mumAngT",&mumAngT);
  tree_->Branch("mumNHits",&mumNHits);
  tree_->Branch("mumNPHits",&mumNPHits);
  tree_->Branch("mupC2",&mupC2);  
  tree_->Branch("mupCat",&mupCat);
  tree_->Branch("mupAngT",&mupAngT);
  tree_->Branch("mupNHits",&mupNHits);
  tree_->Branch("mupNPHits",&mupNPHits);
  tree_->Branch("mumdxy",&mumdxy);
  tree_->Branch("mupdxy",&mupdxy);
  tree_->Branch("mumdz",&mumdz);
  tree_->Branch("mupdz",&mupdz);
  tree_->Branch("muon_dca",&muon_dca);

  tree_->Branch("tri_Dim25",&tri_Dim25);
  tree_->Branch("tri_Dim20",&tri_Dim20);
  tree_->Branch("tri_JpsiTk",&tri_JpsiTk);
  tree_->Branch("tri_DoubleMu43Jpsi",&tri_DoubleMu43Jpsi);

  tree_->Branch("mu1soft",&mu1soft);
  tree_->Branch("mu2soft",&mu2soft);
  tree_->Branch("mu1tight",&mu1tight);
  tree_->Branch("mu2tight",&mu2tight);
  tree_->Branch("mu1PF",&mu1PF);
  tree_->Branch("mu2PF",&mu2PF);
  tree_->Branch("mu1loose",&mu1loose);
  tree_->Branch("mu2loose",&mu2loose);


}


// ------------ method called once each job just after ending the event loop  ------------
void JPsiphiPAT::endJob() {
  tree_->GetDirectory()->cd();
  tree_->Write();
}

//define this as a plug-in
DEFINE_FWK_MODULE(JPsiphiPAT);

