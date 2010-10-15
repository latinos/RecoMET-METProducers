


// -*- C++ -*-
//
// Package:    MuonTCMETValueMapProducer
// Class:      MuonTCMETValueMapProducer
// 
/**\class MuonTCMETValueMapProducer MuonTCMETValueMapProducer.cc JetMETCorrections/Type1MET/src/MuonTCMETValueMapProducer.cc

Description: <one line class summary>

Implementation:
<Notes on implementation>
*/
//
// Original Author:  Frank Golf
//         Created:  Sun Mar 15 11:33:20 CDT 2009
// $Id: MuonTCMETValueMapProducer.cc,v 1.5 2010/09/14 21:20:16 benhoob Exp $
//
//

// system include files
#include <memory>

// user include files
#include "RecoMET/METProducers/interface/MuonTCMETValueMapProducer.h"

#include "RecoMET/METAlgorithms/interface/TCMETAlgo.h"

#include "DataFormats/MuonReco/interface/Muon.h"
#include "DataFormats/BeamSpot/interface/BeamSpot.h"
#include "DataFormats/TrackReco/interface/Track.h"
#include "DataFormats/TrackReco/interface/TrackBase.h"
#include "DataFormats/TrackReco/interface/TrackFwd.h"
#include "DataFormats/Common/interface/ValueMap.h" 
#include "DataFormats/MuonReco/interface/MuonMETCorrectionData.h"
#include "DataFormats/GeometrySurface/interface/Plane.h"
#include "DataFormats/GeometrySurface/interface/Cylinder.h"
#include "DataFormats/GeometryVector/interface/GlobalPoint.h"
#include "DataFormats/GeometryVector/interface/GlobalVector.h"
#include "DataFormats/Math/interface/Point3D.h"

#include "MagneticField/Engine/interface/MagneticField.h"
#include "MagneticField/Records/interface/IdealMagneticFieldRecord.h"

#include "TrackingTools/TrajectoryState/interface/TrajectoryStateOnSurface.h"
#include "TrackingTools/GeomPropagators/interface/AnalyticalPropagator.h"

#include "Geometry/Records/interface/IdealGeometryRecord.h"
#include "FWCore/Framework/interface/MakerMacros.h"

#include "TH2D.h"
#include "TVector3.h"
#include "TMath.h"

typedef math::XYZTLorentzVector LorentzVector;
typedef math::XYZPoint Point;

namespace cms {
  MuonTCMETValueMapProducer::MuonTCMETValueMapProducer(const edm::ParameterSet& iConfig) {
  
    produces<edm::ValueMap<reco::MuonMETCorrectionData> > ("muCorrData");

    // get input collections
    muonInputTag_     = iConfig.getParameter<edm::InputTag>("muonInputTag"    );
    beamSpotInputTag_ = iConfig.getParameter<edm::InputTag>("beamSpotInputTag");
    vertexInputTag_   = iConfig.getParameter<edm::InputTag>("vertexInputTag");

    rfType_     = iConfig.getParameter<int>("rf_type");

    nLayers_                = iConfig.getParameter<int>      ("nLayers");
    nLayersTight_           = iConfig.getParameter<int>      ("nLayersTight");
    vertexNdof_             = iConfig.getParameter<int>      ("vertexNdof");
    vertexZ_                = iConfig.getParameter<double>   ("vertexZ");
    vertexRho_              = iConfig.getParameter<double>   ("vertexRho");
    vertexMaxDZ_            = iConfig.getParameter<double>   ("vertexMaxDZ");
    maxpt_eta20_            = iConfig.getParameter<double>   ("maxpt_eta20");
    maxpt_eta25_            = iConfig.getParameter<double>   ("maxpt_eta25");

    // get configuration parameters
    maxTrackAlgo_    = iConfig.getParameter<int>("trackAlgo_max");
    maxd0cut_        = iConfig.getParameter<double>("d0_max"       );
    minpt_           = iConfig.getParameter<double>("pt_min"       );
    maxpt_           = iConfig.getParameter<double>("pt_max"       );
    maxeta_          = iConfig.getParameter<double>("eta_max"      );
    maxchi2_         = iConfig.getParameter<double>("chi2_max"     );
    minhits_         = iConfig.getParameter<double>("nhits_min"    );
    maxPtErr_        = iConfig.getParameter<double>("ptErr_max"    );

    trkQuality_      = iConfig.getParameter<std::vector<int> >("track_quality");
    trkAlgos_        = iConfig.getParameter<std::vector<int> >("track_algos"  );
    maxchi2_tight_   = iConfig.getParameter<double>("chi2_max_tight");
    minhits_tight_   = iConfig.getParameter<double>("nhits_min_tight");
    maxPtErr_tight_  = iConfig.getParameter<double>("ptErr_max_tight");
    usePvtxd0_       = iConfig.getParameter<bool>("usePvtxd0");
    d0cuta_          = iConfig.getParameter<double>("d0cuta");
    d0cutb_          = iConfig.getParameter<double>("d0cutb");

    muon_dptrel_  = iConfig.getParameter<double>("muon_dptrel");
    muond0_     = iConfig.getParameter<double>("d0_muon"    );
    muonpt_     = iConfig.getParameter<double>("pt_muon"    );
    muoneta_    = iConfig.getParameter<double>("eta_muon"   );
    muonchi2_   = iConfig.getParameter<double>("chi2_muon"  );
    muonhits_   = iConfig.getParameter<double>("nhits_muon" );
    muonGlobal_   = iConfig.getParameter<bool>("global_muon");
    muonTracker_  = iConfig.getParameter<bool>("tracker_muon");
    muonDeltaR_ = iConfig.getParameter<double>("deltaR_muon");
    useCaloMuons_ = iConfig.getParameter<bool>("useCaloMuons");
    muonMinValidStaHits_ = iConfig.getParameter<int>("muonMinValidStaHits");

    response_function = 0;
  }

  MuonTCMETValueMapProducer::~MuonTCMETValueMapProducer()
  {
 
    // do anything here that needs to be done at desctruction time
    // (e.g. close files, deallocate resources etc.)

  }

  //
  // member functions
  //

  // ------------ method called to produce the data  ------------
  void MuonTCMETValueMapProducer::produce(edm::Event& iEvent, const edm::EventSetup& iSetup) {
  
    //get input collections
    iEvent.getByLabel(muonInputTag_    , muon_h    );
    iEvent.getByLabel(beamSpotInputTag_, beamSpot_h);

    //get vertex collection
    hasValidVertex = false;
    if( usePvtxd0_ ){
      iEvent.getByLabel( vertexInputTag_  , VertexHandle  );
       
      if( VertexHandle.isValid() ) {
        vertexColl = VertexHandle.product();
        hasValidVertex = isValidVertex();
      } 
    }

    //get the Bfield
    edm::ESHandle<MagneticField> theMagField;
    bool haveBfield = true;
    if( !theMagField.isValid() ) haveBfield = false;
    iSetup.get<IdealMagneticFieldRecord>().get(theMagField);
    bField = theMagField.product();

    //make a ValueMap of ints => flags for 
    //met correction. The values and meanings of the flags are :
    // flag==0 --->    The muon is not used to correct the MET by default
    // flag==1 --->    The muon is used to correct the MET. The Global pt is used.
    // flag==2 --->    The muon is used to correct the MET. The tracker pt is used.
    // flag==3 --->    The muon is used to correct the MET. The standalone pt is used.
    // flag==4 --->    The muon is used to correct the MET as pion using the tcMET ZSP RF.
    // flag==5 --->    The muon is used to correct the MET.  The default fit is used; i.e. we get the pt from muon->pt().
    // In general, the flag should never be 3. You do not want to correct the MET using
    // the pt measurement from the standalone system (unless you really know what you're 
    // doing

    std::auto_ptr<edm::ValueMap<reco::MuonMETCorrectionData> > vm_muCorrData(new edm::ValueMap<reco::MuonMETCorrectionData>());

    std::vector<reco::MuonMETCorrectionData> v_muCorrData;

    unsigned int nMuons = muon_h->size();

    for (unsigned int iMu = 0; iMu < nMuons; iMu++) {

      const reco::Muon* mu = &(*muon_h)[iMu];
      double deltax = 0.0;
      double deltay = 0.0;

      reco::MuonMETCorrectionData muMETCorrData(reco::MuonMETCorrectionData::NotUsed, deltax, deltay);
    
      reco::TrackRef mu_track;
      if( mu->isGlobalMuon() || mu->isTrackerMuon() || mu->isCaloMuon() )
        mu_track = mu->innerTrack();
      else {
        v_muCorrData.push_back( muMETCorrData );
        continue;
      }

      // figure out depositions muons would make if they were treated as pions
      if( isGoodTrack( mu ) ) {

        if( mu_track->pt() < minpt_ ) 
          muMETCorrData = reco::MuonMETCorrectionData(reco::MuonMETCorrectionData::TreatedAsPion, deltax, deltay);

        else {
          int bin_index   = response_function->FindBin( mu_track->eta(), mu_track->pt() );
          double response = response_function->GetBinContent( bin_index );

          TVector3 outerTrkPosition = propagateTrack( mu );

          deltax = response * mu_track->p() * sin( outerTrkPosition.Theta() ) * cos( outerTrkPosition.Phi() );
          deltay = response * mu_track->p() * sin( outerTrkPosition.Theta() ) * sin( outerTrkPosition.Phi() );

          muMETCorrData = reco::MuonMETCorrectionData(reco::MuonMETCorrectionData::TreatedAsPion, deltax, deltay);
        }
      }

      // figure out muon flag
      if( isGoodMuon( mu ) )
        v_muCorrData.push_back( reco::MuonMETCorrectionData(reco::MuonMETCorrectionData::MuonCandidateValuesUsed, deltax, deltay) );

      else if( useCaloMuons_ && isGoodCaloMuon( mu, iMu ) )
        v_muCorrData.push_back( reco::MuonMETCorrectionData(reco::MuonMETCorrectionData::MuonCandidateValuesUsed, deltax, deltay) );

      else v_muCorrData.push_back( muMETCorrData );
    }
    
    edm::ValueMap<reco::MuonMETCorrectionData>::Filler dataFiller(*vm_muCorrData);

    dataFiller.insert( muon_h, v_muCorrData.begin(), v_muCorrData.end());
    dataFiller.fill();
    
    iEvent.put(vm_muCorrData, "muCorrData");    
  }
  
  // ------------ method called once each job just before starting event loop  ------------
  void MuonTCMETValueMapProducer::beginJob()
  {

    if( rfType_ == 1 )
      response_function = getResponseFunction_fit();
    else if( rfType_ == 2 )
      response_function = getResponseFunction_mode();
  }

  // ------------ method called once each job just after ending the event loop  ------------
  void MuonTCMETValueMapProducer::endJob() {
  }

  // ------------ check is muon is a good muon  ------------
  bool MuonTCMETValueMapProducer::isGoodMuon( const reco::Muon* muon ) {
    double d0    = -999;
    double nhits = 0;
    double chi2  = 999;  

    // get d0 corrected for beam spot
    bool haveBeamSpot = true;
    if( !beamSpot_h.isValid() ) haveBeamSpot = false;

    if( muonGlobal_  && !muon->isGlobalMuon()  ) return false;
    if( muonTracker_ && !muon->isTrackerMuon() ) return false;

    const reco::TrackRef siTrack     = muon->innerTrack();
    const reco::TrackRef globalTrack = muon->globalTrack();

    Point bspot = haveBeamSpot ? beamSpot_h->position() : Point(0,0,0);
    if( siTrack.isNonnull() ) nhits = siTrack->numberOfValidHits();
    if( globalTrack.isNonnull() ) {
      d0   = -1 * globalTrack->dxy( bspot );
      chi2 = globalTrack->normalizedChi2();
    }

    if( fabs( d0 ) > muond0_ )                          return false;
    if( muon->pt() < muonpt_ )                          return false;
    if( fabs( muon->eta() ) > muoneta_ )                return false;
    if( nhits < muonhits_ )                             return false;
    if( chi2 > muonchi2_ )                              return false;
    if( globalTrack->hitPattern().numberOfValidMuonHits() < muonMinValidStaHits_ ) return false;

    //reject muons with tracker dpt/pt > X
    if( !siTrack.isNonnull() )                                return false;
    if( siTrack->ptError() / siTrack->pt() > muon_dptrel_ )   return false;

    else return true;
  }

  // ------------ check if muon is a good calo muon  ------------
  bool MuonTCMETValueMapProducer::isGoodCaloMuon( const reco::Muon* muon, const unsigned int index ) {

    if( muon->pt() < 10 ) return false;

    if( !isGoodTrack( muon ) ) return false;

    const reco::TrackRef inputSiliconTrack = muon->innerTrack();
    if( !inputSiliconTrack.isNonnull() ) return false;

    //check if it is in the vicinity of a global or tracker muon
    unsigned int nMuons = muon_h->size();
    for (unsigned int iMu = 0; iMu < nMuons; iMu++) {

      if( iMu == index ) continue;

      const reco::Muon* mu = &(*muon_h)[iMu];

      const reco::TrackRef testSiliconTrack = mu->innerTrack();
      if( !testSiliconTrack.isNonnull() ) continue;

      double deltaEta = inputSiliconTrack.get()->eta() - testSiliconTrack.get()->eta();
      double deltaPhi = acos( cos( inputSiliconTrack.get()->phi() - testSiliconTrack.get()->phi() ) );
      double deltaR   = TMath::Sqrt( deltaEta * deltaEta + deltaPhi * deltaPhi );

      if( deltaR < muonDeltaR_ ) return false;
    }

    return true;
  }

  // ------------ check if track is good  ------------
  bool MuonTCMETValueMapProducer::isGoodTrack( const reco::Muon* muon ) {
    double d0    = -999;

    const reco::TrackRef siTrack = muon->innerTrack();
    if (!siTrack.isNonnull())
      return false;

    if( hasValidVertex ){
      //get d0 corrected for primary vertex
            
      const Point pvtx = Point(vertexColl->begin()->x(),
                               vertexColl->begin()->y(), 
                               vertexColl->begin()->z());
            
      d0 = -1 * siTrack->dxy( pvtx );
            
      double dz = siTrack->dz( pvtx );
            
      if( fabs( dz ) < vertexMaxDZ_ ){
              
        //get d0 corrected for pvtx
        d0 = -1 * siTrack->dxy( pvtx );
              
      }else{
              
        // get d0 corrected for beam spot
        bool haveBeamSpot = true;
        if( !beamSpot_h.isValid() ) haveBeamSpot = false;
              
        Point bspot = haveBeamSpot ? beamSpot_h->position() : Point(0,0,0);
        d0 = -1 * siTrack->dxy( bspot );
              
      }
    }else{
       
      // get d0 corrected for beam spot
      bool haveBeamSpot = true;
      if( !beamSpot_h.isValid() ) haveBeamSpot = false;
       
      Point bspot = haveBeamSpot ? beamSpot_h->position() : Point(0,0,0);
      d0 = -1 * siTrack->dxy( bspot );
    }

    if( siTrack->algo() < maxTrackAlgo_ ){
      //1st 4 tracking iterations (pT-dependent d0 cut)
       
      float d0cut = sqrt(std::pow(d0cuta_,2) + std::pow(d0cutb_/siTrack->pt(),2)); 
      if(d0cut > maxd0cut_) d0cut = maxd0cut_;
       
      if( fabs( d0 ) > d0cut )            return false;    
      if( nLayers( siTrack ) < nLayers_ ) return false;
    }
    else{
      //last 2 tracking iterations (tighten chi2, nhits, pt error cuts)
     
      if( siTrack->normalizedChi2() > maxchi2_tight_ )               return false;
      if( siTrack->numberOfValidHits() < minhits_tight_ )            return false;
      if( (siTrack->ptError() / siTrack->pt()) > maxPtErr_tight_ )   return false;
      if( nLayers( siTrack ) < nLayersTight_ )                       return false;
    }

    if( siTrack->numberOfValidHits() < minhits_ )                         return false;
    if( siTrack->normalizedChi2() > maxchi2_ )                            return false;
    if( fabs( siTrack->eta() ) > maxeta_ )                                return false;
    if( siTrack->pt() > maxpt_ )                                          return false;
    if( (siTrack->ptError() / siTrack->pt()) > maxPtErr_ )                return false;
    if( fabs( siTrack->eta() ) > 2.5 && siTrack->pt() > maxpt_eta25_ )    return false;
    if( fabs( siTrack->eta() ) > 2.0 && siTrack->pt() > maxpt_eta20_ )    return false;

    int cut = 0;	  
    for( unsigned int i = 0; i < trkQuality_.size(); i++ ) {

      cut |= (1 << trkQuality_.at(i));
    }

    if( !( (siTrack->qualityMask() & cut) == cut ) ) return false;
	  
    bool isGoodAlgo = false;    
    if( trkAlgos_.size() == 0 ) isGoodAlgo = true;
    for( unsigned int i = 0; i < trkAlgos_.size(); i++ ) {

      if( siTrack->algo() == trkAlgos_.at(i) ) isGoodAlgo = true;
    }

    if( !isGoodAlgo ) return false;
	  
    return true;
  }

  // ------------ propagate track to calorimeter face  ------------
  TVector3 MuonTCMETValueMapProducer::propagateTrack( const reco::Muon* muon) {

    TVector3 outerTrkPosition;

    outerTrkPosition.SetPtEtaPhi( 999., -10., 2 * TMath::Pi() );

    const reco::TrackRef track = muon->innerTrack();

    if( !track.isNonnull() ) {
      return outerTrkPosition;
    }

    GlobalPoint  tpVertex ( track->vx(), track->vy(), track->vz() );
    GlobalVector tpMomentum ( track.get()->px(), track.get()->py(), track.get()->pz() );
    int tpCharge ( track->charge() );

    FreeTrajectoryState fts ( tpVertex, tpMomentum, tpCharge, bField);

    const double zdist = 314.;

    const double radius = 130.;

    const double corner = 1.479;

    Plane::PlanePointer lendcap = Plane::build( Plane::PositionType (0, 0, -zdist), Plane::RotationType () );
    Plane::PlanePointer rendcap = Plane::build( Plane::PositionType (0, 0, zdist),  Plane::RotationType () );

    Cylinder::CylinderPointer barrel = Cylinder::build( Cylinder::PositionType (0, 0, 0), Cylinder::RotationType (), radius);

    AnalyticalPropagator myAP (bField, alongMomentum, 2*M_PI);

    TrajectoryStateOnSurface tsos;

    if( track.get()->eta() < -corner ) {
      tsos = myAP.propagate( fts, *lendcap);
    }
    else if( fabs(track.get()->eta()) < corner ) {
      tsos = myAP.propagate( fts, *barrel);
    }
    else if( track.get()->eta() > corner ) {
      tsos = myAP.propagate( fts, *rendcap);
    }

    if( tsos.isValid() )
      outerTrkPosition.SetXYZ( tsos.globalPosition().x(), tsos.globalPosition().y(), tsos.globalPosition().z() );

    else 
      outerTrkPosition.SetPtEtaPhi( 999., -10., 2 * TMath::Pi() );

    return outerTrkPosition;
  }

  // ------------ single pion response function from fit  ------------

  int MuonTCMETValueMapProducer::nLayers(const reco::TrackRef track){
    const reco::HitPattern& p = track->hitPattern();
    return p.trackerLayersWithMeasurement();
  }

  //--------------------------------------------------------------------

  bool MuonTCMETValueMapProducer::isValidVertex(){
    
    if( vertexColl->begin()->isFake()                ) return false;
    if( vertexColl->begin()->ndof() < vertexNdof_    ) return false;
    if( fabs( vertexColl->begin()->z() ) > vertexZ_  ) return false;
    if( sqrt( std::pow( vertexColl->begin()->x() , 2 ) + std::pow( vertexColl->begin()->y() , 2 ) ) > vertexRho_ ) return false;
    
    return true;
    
  }

  // ------------ single pion response function from fit  ------------
  TH2D* MuonTCMETValueMapProducer::getResponseFunction_fit() {

    Double_t xAxis1[53] = {-2.5, -2.322, -2.172, -2.043, -1.93, -1.83, 
                           -1.74, -1.653, -1.566, -1.479, -1.392, -1.305, 
                           -1.218, -1.131, -1.044, -0.957, -0.879, -0.783, 
                           -0.696, -0.609, -0.522, -0.435, -0.348, -0.261, 
                           -0.174, -0.087, 0, 0.087, 0.174, 0.261, 0.348, 
                           0.435, 0.522, 0.609, 0.696, 0.783, 0.879, 0.957, 
                           1.044, 1.131, 1.218, 1.305, 1.392, 1.479, 1.566, 
                           1.653, 1.74, 1.83, 1.93, 2.043, 2.172, 2.322, 2.5}; 
  
    Double_t yAxis1[29] = {0, 0.5, 1, 1.5, 2, 3, 4, 5, 6, 7, 8, 9, 10, 12, 14,
                           16, 18, 20, 25, 30, 35, 40, 45, 50, 60, 70, 80, 90, 100}; 
   
    TH2D *hrf = new TH2D("hrf","RF  (fit)",52, xAxis1,28, yAxis1);

    hrf->SetBinContent(163,0.596559);
    hrf->SetBinContent(164,0.628848);
    hrf->SetBinContent(165,0.6198216);
    hrf->SetBinContent(166,0.6057171);
    hrf->SetBinContent(167,0.5523221);
    hrf->SetBinContent(168,0.4973629);
    hrf->SetBinContent(169,0.4271086);
    hrf->SetBinContent(170,0.377397);
    hrf->SetBinContent(171,0.2579508);
    hrf->SetBinContent(172,0.1808747);
    hrf->SetBinContent(173,0.1696191);
    hrf->SetBinContent(174,0.199506);
    hrf->SetBinContent(175,0.2166552);
    hrf->SetBinContent(176,3.630126e-15);
    hrf->SetBinContent(177,2.176102e-15);
    hrf->SetBinContent(178,2.31394e-13);
    hrf->SetBinContent(179,6.127694e-15);
    hrf->SetBinContent(180,5.981923e-14);
    hrf->SetBinContent(181,8.263721e-12);
    hrf->SetBinContent(182,0.246672);
    hrf->SetBinContent(183,0.3038504);
    hrf->SetBinContent(184,8.375397e-14);
    hrf->SetBinContent(185,6.349614e-14);
    hrf->SetBinContent(186,1.084202e-19);
    hrf->SetBinContent(187,5.239299e-15);
    hrf->SetBinContent(188,1.82249e-15);
    hrf->SetBinContent(189,7.130256e-16);
    hrf->SetBinContent(190,6.670581e-13);
    hrf->SetBinContent(191,3.85114e-15);
    hrf->SetBinContent(192,9.043607e-14);
    hrf->SetBinContent(193,1.904228e-14);
    hrf->SetBinContent(194,0.3022267);
    hrf->SetBinContent(195,9.776685e-15);
    hrf->SetBinContent(196,1.607238e-11);
    hrf->SetBinContent(197,1.438433e-12);
    hrf->SetBinContent(198,1.891458e-13);
    hrf->SetBinContent(199,2.278017e-15);
    hrf->SetBinContent(200,1.474515e-17);
    hrf->SetBinContent(201,0.1953482);
    hrf->SetBinContent(202,0.1696679);
    hrf->SetBinContent(203,1.013956e-12);
    hrf->SetBinContent(204,1.528058e-14);
    hrf->SetBinContent(205,1.523467e-15);
    hrf->SetBinContent(206,2.530109e-11);
    hrf->SetBinContent(207,0.1999144);
    hrf->SetBinContent(208,0.2090482);
    hrf->SetBinContent(209,0.2830529);
    hrf->SetBinContent(210,0.3812176);
    hrf->SetBinContent(211,0.463355);
    hrf->SetBinContent(212,0.4664196);
    hrf->SetBinContent(213,0.4673808);
    hrf->SetBinContent(214,0.4517687);
    hrf->SetBinContent(217,0.5753061);
    hrf->SetBinContent(218,0.5892629);
    hrf->SetBinContent(219,0.60405);
    hrf->SetBinContent(220,0.5768098);
    hrf->SetBinContent(221,0.5439);
    hrf->SetBinContent(222,0.5066264);
    hrf->SetBinContent(223,0.4571394);
    hrf->SetBinContent(224,0.3468399);
    hrf->SetBinContent(225,0.1857008);
    hrf->SetBinContent(226,0.1019076);
    hrf->SetBinContent(227,4.94788e-11);
    hrf->SetBinContent(228,4.364363e-12);
    hrf->SetBinContent(229,1.692513e-11);
    hrf->SetBinContent(230,6.919022e-13);
    hrf->SetBinContent(231,0.005085689);
    hrf->SetBinContent(232,0.2074284);
    hrf->SetBinContent(233,0.2625127);
    hrf->SetBinContent(234,0.1147665);
    hrf->SetBinContent(235,4.82676e-14);
    hrf->SetBinContent(236,1.297871e-14);
    hrf->SetBinContent(237,2.458336e-13);
    hrf->SetBinContent(238,1.165398e-14);
    hrf->SetBinContent(239,1.565169e-13);
    hrf->SetBinContent(240,6.175073e-16);
    hrf->SetBinContent(241,5.472115e-14);
    hrf->SetBinContent(242,5.028639e-13);
    hrf->SetBinContent(243,1.97841e-12);
    hrf->SetBinContent(244,3.762162e-13);
    hrf->SetBinContent(245,1.957538e-13);
    hrf->SetBinContent(246,1.604375e-14);
    hrf->SetBinContent(247,1.154167e-12);
    hrf->SetBinContent(248,2.839862e-14);
    hrf->SetBinContent(249,3.039627e-13);
    hrf->SetBinContent(250,2.935477e-16);
    hrf->SetBinContent(251,0.3008533);
    hrf->SetBinContent(252,0.2601693);
    hrf->SetBinContent(253,0.187013);
    hrf->SetBinContent(254,7.883153e-14);
    hrf->SetBinContent(255,1.058615e-15);
    hrf->SetBinContent(256,3.865831e-15);
    hrf->SetBinContent(257,7.144892e-16);
    hrf->SetBinContent(258,4.432561e-13);
    hrf->SetBinContent(259,4.892572e-13);
    hrf->SetBinContent(260,4.394992e-14);
    hrf->SetBinContent(261,0.09488474);
    hrf->SetBinContent(262,0.25234);
    hrf->SetBinContent(263,0.3080661);
    hrf->SetBinContent(264,0.3395023);
    hrf->SetBinContent(265,0.3849501);
    hrf->SetBinContent(266,0.4352584);
    hrf->SetBinContent(267,0.4827173);
    hrf->SetBinContent(268,0.4638615);
    hrf->SetBinContent(271,0.5776743);
    hrf->SetBinContent(272,0.5831007);
    hrf->SetBinContent(273,0.5801519);
    hrf->SetBinContent(274,0.5517887);
    hrf->SetBinContent(275,0.5113201);
    hrf->SetBinContent(276,0.488913);
    hrf->SetBinContent(277,0.4362271);
    hrf->SetBinContent(278,0.4085703);
    hrf->SetBinContent(279,0.2786153);
    hrf->SetBinContent(280,0.2983199);
    hrf->SetBinContent(281,0.2869726);
    hrf->SetBinContent(282,0.2443394);
    hrf->SetBinContent(283,0.2043574);
    hrf->SetBinContent(284,0.1864919);
    hrf->SetBinContent(285,0.1770072);
    hrf->SetBinContent(286,0.1737026);
    hrf->SetBinContent(287,0.1990662);
    hrf->SetBinContent(288,0.2191123);
    hrf->SetBinContent(289,0.2065748);
    hrf->SetBinContent(290,0.2551676);
    hrf->SetBinContent(291,0.2659435);
    hrf->SetBinContent(292,0.2860025);
    hrf->SetBinContent(293,0.3015017);
    hrf->SetBinContent(294,0.348653);
    hrf->SetBinContent(295,0.3362122);
    hrf->SetBinContent(296,0.3548431);
    hrf->SetBinContent(297,0.3565721);
    hrf->SetBinContent(298,0.3369229);
    hrf->SetBinContent(299,0.3344429);
    hrf->SetBinContent(300,0.3190944);
    hrf->SetBinContent(301,0.297819);
    hrf->SetBinContent(302,0.2696725);
    hrf->SetBinContent(303,0.2296587);
    hrf->SetBinContent(304,0.1975535);
    hrf->SetBinContent(305,0.1589602);
    hrf->SetBinContent(306,0.2075599);
    hrf->SetBinContent(307,0.1812693);
    hrf->SetBinContent(308,0.1612744);
    hrf->SetBinContent(309,0.1307955);
    hrf->SetBinContent(310,0.1913683);
    hrf->SetBinContent(311,0.203024);
    hrf->SetBinContent(312,0.08631216);
    hrf->SetBinContent(313,0.1296129);
    hrf->SetBinContent(314,3.001588e-12);
    hrf->SetBinContent(315,0.2013968);
    hrf->SetBinContent(316,0.2526931);
    hrf->SetBinContent(317,0.3269316);
    hrf->SetBinContent(318,0.3695775);
    hrf->SetBinContent(319,0.3865211);
    hrf->SetBinContent(320,0.4577455);
    hrf->SetBinContent(321,0.4823737);
    hrf->SetBinContent(322,0.5029419);
    hrf->SetBinContent(325,0.6388038);
    hrf->SetBinContent(326,0.6276655);
    hrf->SetBinContent(327,0.599334);
    hrf->SetBinContent(328,0.5681248);
    hrf->SetBinContent(329,0.5278199);
    hrf->SetBinContent(330,0.506333);
    hrf->SetBinContent(331,0.4560981);
    hrf->SetBinContent(332,0.4411896);
    hrf->SetBinContent(333,0.4125218);
    hrf->SetBinContent(334,0.4209384);
    hrf->SetBinContent(335,0.4049862);
    hrf->SetBinContent(336,0.4061612);
    hrf->SetBinContent(337,0.3860883);
    hrf->SetBinContent(338,0.3815906);
    hrf->SetBinContent(339,0.3796453);
    hrf->SetBinContent(340,0.3839429);
    hrf->SetBinContent(341,0.3979055);
    hrf->SetBinContent(342,0.4124853);
    hrf->SetBinContent(343,0.4046432);
    hrf->SetBinContent(344,0.396976);
    hrf->SetBinContent(345,0.4078462);
    hrf->SetBinContent(346,0.4173306);
    hrf->SetBinContent(347,0.4017106);
    hrf->SetBinContent(348,0.3876909);
    hrf->SetBinContent(349,0.3987448);
    hrf->SetBinContent(350,0.4120063);
    hrf->SetBinContent(351,0.4029436);
    hrf->SetBinContent(352,0.4105899);
    hrf->SetBinContent(353,0.4055827);
    hrf->SetBinContent(354,0.3988593);
    hrf->SetBinContent(355,0.4066432);
    hrf->SetBinContent(356,0.3944102);
    hrf->SetBinContent(357,0.3908547);
    hrf->SetBinContent(358,0.403081);
    hrf->SetBinContent(359,0.3909493);
    hrf->SetBinContent(360,0.3859306);
    hrf->SetBinContent(361,0.3799499);
    hrf->SetBinContent(362,0.3736477);
    hrf->SetBinContent(363,0.3643707);
    hrf->SetBinContent(364,0.3423016);
    hrf->SetBinContent(365,0.3583263);
    hrf->SetBinContent(366,0.3529241);
    hrf->SetBinContent(367,0.3697914);
    hrf->SetBinContent(368,0.2970605);
    hrf->SetBinContent(369,0.3629799);
    hrf->SetBinContent(370,0.3638369);
    hrf->SetBinContent(371,0.3926952);
    hrf->SetBinContent(372,0.4360398);
    hrf->SetBinContent(373,0.4791366);
    hrf->SetBinContent(374,0.5103499);
    hrf->SetBinContent(375,0.556242);
    hrf->SetBinContent(376,0.5863192);
    hrf->SetBinContent(379,0.7023737);
    hrf->SetBinContent(380,0.6884339);
    hrf->SetBinContent(381,0.6494117);
    hrf->SetBinContent(382,0.6140636);
    hrf->SetBinContent(383,0.5626363);
    hrf->SetBinContent(384,0.5252846);
    hrf->SetBinContent(385,0.500723);
    hrf->SetBinContent(386,0.4829722);
    hrf->SetBinContent(387,0.471149);
    hrf->SetBinContent(388,0.4711686);
    hrf->SetBinContent(389,0.4411855);
    hrf->SetBinContent(390,0.4388084);
    hrf->SetBinContent(391,0.4303384);
    hrf->SetBinContent(392,0.433742);
    hrf->SetBinContent(393,0.4412655);
    hrf->SetBinContent(394,0.4560982);
    hrf->SetBinContent(395,0.4680078);
    hrf->SetBinContent(396,0.4600742);
    hrf->SetBinContent(397,0.4707758);
    hrf->SetBinContent(398,0.4787438);
    hrf->SetBinContent(399,0.4812429);
    hrf->SetBinContent(400,0.4899426);
    hrf->SetBinContent(401,0.4856245);
    hrf->SetBinContent(402,0.48637);
    hrf->SetBinContent(403,0.4968673);
    hrf->SetBinContent(404,0.5040057);
    hrf->SetBinContent(405,0.5007334);
    hrf->SetBinContent(406,0.4998913);
    hrf->SetBinContent(407,0.5019802);
    hrf->SetBinContent(408,0.4701506);
    hrf->SetBinContent(409,0.4977818);
    hrf->SetBinContent(410,0.4621918);
    hrf->SetBinContent(411,0.4730811);
    hrf->SetBinContent(412,0.4708458);
    hrf->SetBinContent(413,0.4649467);
    hrf->SetBinContent(414,0.4572159);
    hrf->SetBinContent(415,0.4481314);
    hrf->SetBinContent(416,0.4430795);
    hrf->SetBinContent(417,0.4387775);
    hrf->SetBinContent(418,0.4281495);
    hrf->SetBinContent(419,0.4233392);
    hrf->SetBinContent(420,0.4181026);
    hrf->SetBinContent(421,0.4348903);
    hrf->SetBinContent(422,0.401774);
    hrf->SetBinContent(423,0.4088957);
    hrf->SetBinContent(424,0.4102485);
    hrf->SetBinContent(425,0.467757);
    hrf->SetBinContent(426,0.4979419);
    hrf->SetBinContent(427,0.5392405);
    hrf->SetBinContent(428,0.5779461);
    hrf->SetBinContent(429,0.621405);
    hrf->SetBinContent(430,0.6427583);
    hrf->SetBinContent(433,0.7175694);
    hrf->SetBinContent(434,0.7227534);
    hrf->SetBinContent(435,0.6872768);
    hrf->SetBinContent(436,0.6629565);
    hrf->SetBinContent(437,0.6285023);
    hrf->SetBinContent(438,0.5852711);
    hrf->SetBinContent(439,0.5342276);
    hrf->SetBinContent(440,0.5179378);
    hrf->SetBinContent(441,0.5124368);
    hrf->SetBinContent(442,0.4788256);
    hrf->SetBinContent(443,0.4678588);
    hrf->SetBinContent(444,0.463864);
    hrf->SetBinContent(445,0.46812);
    hrf->SetBinContent(446,0.4789559);
    hrf->SetBinContent(447,0.4950132);
    hrf->SetBinContent(448,0.4863113);
    hrf->SetBinContent(449,0.502516);
    hrf->SetBinContent(450,0.511229);
    hrf->SetBinContent(451,0.5147772);
    hrf->SetBinContent(452,0.520461);
    hrf->SetBinContent(453,0.518261);
    hrf->SetBinContent(454,0.5295278);
    hrf->SetBinContent(455,0.5262392);
    hrf->SetBinContent(456,0.5340487);
    hrf->SetBinContent(457,0.5421284);
    hrf->SetBinContent(458,0.5449393);
    hrf->SetBinContent(459,0.5376937);
    hrf->SetBinContent(460,0.5293429);
    hrf->SetBinContent(461,0.5300031);
    hrf->SetBinContent(462,0.5206918);
    hrf->SetBinContent(463,0.5228946);
    hrf->SetBinContent(464,0.5131121);
    hrf->SetBinContent(465,0.5059139);
    hrf->SetBinContent(466,0.5147421);
    hrf->SetBinContent(467,0.4951201);
    hrf->SetBinContent(468,0.4995864);
    hrf->SetBinContent(469,0.4922295);
    hrf->SetBinContent(470,0.4734143);
    hrf->SetBinContent(471,0.4630154);
    hrf->SetBinContent(472,0.4671657);
    hrf->SetBinContent(473,0.4452614);
    hrf->SetBinContent(474,0.4531828);
    hrf->SetBinContent(475,0.4544982);
    hrf->SetBinContent(476,0.4618195);
    hrf->SetBinContent(477,0.4821447);
    hrf->SetBinContent(478,0.4902962);
    hrf->SetBinContent(479,0.5183212);
    hrf->SetBinContent(480,0.5634815);
    hrf->SetBinContent(481,0.5912207);
    hrf->SetBinContent(482,0.642562);
    hrf->SetBinContent(483,0.6754044);
    hrf->SetBinContent(484,0.678292);
    hrf->SetBinContent(487,0.7278041);
    hrf->SetBinContent(488,0.7437301);
    hrf->SetBinContent(489,0.7145185);
    hrf->SetBinContent(490,0.6930009);
    hrf->SetBinContent(491,0.6531578);
    hrf->SetBinContent(492,0.611073);
    hrf->SetBinContent(493,0.5911223);
    hrf->SetBinContent(494,0.5705427);
    hrf->SetBinContent(495,0.5650659);
    hrf->SetBinContent(496,0.508364);
    hrf->SetBinContent(497,0.4973736);
    hrf->SetBinContent(498,0.4960206);
    hrf->SetBinContent(499,0.483606);
    hrf->SetBinContent(500,0.4864967);
    hrf->SetBinContent(501,0.4971298);
    hrf->SetBinContent(502,0.5198098);
    hrf->SetBinContent(503,0.5295321);
    hrf->SetBinContent(504,0.5414425);
    hrf->SetBinContent(505,0.5408665);
    hrf->SetBinContent(506,0.5426844);
    hrf->SetBinContent(507,0.5461603);
    hrf->SetBinContent(508,0.5596908);
    hrf->SetBinContent(509,0.5642127);
    hrf->SetBinContent(510,0.5630645);
    hrf->SetBinContent(511,0.5650176);
    hrf->SetBinContent(512,0.5717707);
    hrf->SetBinContent(513,0.5560891);
    hrf->SetBinContent(514,0.5630159);
    hrf->SetBinContent(515,0.538871);
    hrf->SetBinContent(516,0.5478573);
    hrf->SetBinContent(517,0.5369281);
    hrf->SetBinContent(518,0.5404783);
    hrf->SetBinContent(519,0.5359845);
    hrf->SetBinContent(520,0.5441793);
    hrf->SetBinContent(521,0.5433333);
    hrf->SetBinContent(522,0.5172439);
    hrf->SetBinContent(523,0.5205659);
    hrf->SetBinContent(524,0.4980981);
    hrf->SetBinContent(525,0.4814596);
    hrf->SetBinContent(526,0.4766175);
    hrf->SetBinContent(527,0.4670495);
    hrf->SetBinContent(528,0.4746933);
    hrf->SetBinContent(529,0.4934972);
    hrf->SetBinContent(530,0.5154036);
    hrf->SetBinContent(531,0.5006227);
    hrf->SetBinContent(532,0.5336388);
    hrf->SetBinContent(533,0.5572283);
    hrf->SetBinContent(534,0.594081);
    hrf->SetBinContent(535,0.6412556);
    hrf->SetBinContent(536,0.6608669);
    hrf->SetBinContent(537,0.681874);
    hrf->SetBinContent(538,0.695639);
    hrf->SetBinContent(541,0.7421356);
    hrf->SetBinContent(542,0.7425929);
    hrf->SetBinContent(543,0.7394532);
    hrf->SetBinContent(544,0.710999);
    hrf->SetBinContent(545,0.6838782);
    hrf->SetBinContent(546,0.6516311);
    hrf->SetBinContent(547,0.6309382);
    hrf->SetBinContent(548,0.6008532);
    hrf->SetBinContent(549,0.600331);
    hrf->SetBinContent(550,0.5480969);
    hrf->SetBinContent(551,0.5260786);
    hrf->SetBinContent(552,0.5120159);
    hrf->SetBinContent(553,0.518993);
    hrf->SetBinContent(554,0.5121258);
    hrf->SetBinContent(555,0.5135887);
    hrf->SetBinContent(556,0.5316808);
    hrf->SetBinContent(557,0.5284951);
    hrf->SetBinContent(558,0.5566773);
    hrf->SetBinContent(559,0.574351);
    hrf->SetBinContent(560,0.5693278);
    hrf->SetBinContent(561,0.5681988);
    hrf->SetBinContent(562,0.5867079);
    hrf->SetBinContent(563,0.5799457);
    hrf->SetBinContent(564,0.5790245);
    hrf->SetBinContent(565,0.5834141);
    hrf->SetBinContent(566,0.592364);
    hrf->SetBinContent(567,0.5863733);
    hrf->SetBinContent(568,0.5843332);
    hrf->SetBinContent(569,0.5693204);
    hrf->SetBinContent(570,0.5657271);
    hrf->SetBinContent(571,0.5642989);
    hrf->SetBinContent(572,0.5573784);
    hrf->SetBinContent(573,0.5357993);
    hrf->SetBinContent(574,0.5544055);
    hrf->SetBinContent(575,0.5601413);
    hrf->SetBinContent(576,0.5246989);
    hrf->SetBinContent(577,0.5251947);
    hrf->SetBinContent(578,0.5230677);
    hrf->SetBinContent(579,0.5145684);
    hrf->SetBinContent(580,0.5039691);
    hrf->SetBinContent(581,0.5049056);
    hrf->SetBinContent(582,0.5080774);
    hrf->SetBinContent(583,0.5330774);
    hrf->SetBinContent(584,0.5738508);
    hrf->SetBinContent(585,0.5696554);
    hrf->SetBinContent(586,0.5803724);
    hrf->SetBinContent(587,0.6177924);
    hrf->SetBinContent(588,0.644672);
    hrf->SetBinContent(589,0.6626406);
    hrf->SetBinContent(590,0.6861277);
    hrf->SetBinContent(591,0.700965);
    hrf->SetBinContent(592,0.7060913);
    hrf->SetBinContent(595,0.7475226);
    hrf->SetBinContent(596,0.7557024);
    hrf->SetBinContent(597,0.7394427);
    hrf->SetBinContent(598,0.7337875);
    hrf->SetBinContent(599,0.7168513);
    hrf->SetBinContent(600,0.6858083);
    hrf->SetBinContent(601,0.6527843);
    hrf->SetBinContent(602,0.6517891);
    hrf->SetBinContent(603,0.6449288);
    hrf->SetBinContent(604,0.5725983);
    hrf->SetBinContent(605,0.5533513);
    hrf->SetBinContent(606,0.5520771);
    hrf->SetBinContent(607,0.5480133);
    hrf->SetBinContent(608,0.5436477);
    hrf->SetBinContent(609,0.5393097);
    hrf->SetBinContent(610,0.5496147);
    hrf->SetBinContent(611,0.549866);
    hrf->SetBinContent(612,0.5623665);
    hrf->SetBinContent(613,0.5628304);
    hrf->SetBinContent(614,0.57687);
    hrf->SetBinContent(615,0.5864559);
    hrf->SetBinContent(616,0.583317);
    hrf->SetBinContent(617,0.5850238);
    hrf->SetBinContent(618,0.5893172);
    hrf->SetBinContent(619,0.6012055);
    hrf->SetBinContent(620,0.600987);
    hrf->SetBinContent(621,0.5934208);
    hrf->SetBinContent(622,0.592157);
    hrf->SetBinContent(623,0.5723094);
    hrf->SetBinContent(624,0.588411);
    hrf->SetBinContent(625,0.5967565);
    hrf->SetBinContent(626,0.5784929);
    hrf->SetBinContent(627,0.559729);
    hrf->SetBinContent(628,0.5586101);
    hrf->SetBinContent(629,0.5574296);
    hrf->SetBinContent(630,0.5429283);
    hrf->SetBinContent(631,0.5427474);
    hrf->SetBinContent(632,0.5420908);
    hrf->SetBinContent(633,0.5373055);
    hrf->SetBinContent(634,0.5361665);
    hrf->SetBinContent(635,0.5450932);
    hrf->SetBinContent(636,0.5287489);
    hrf->SetBinContent(637,0.5583689);
    hrf->SetBinContent(638,0.6126305);
    hrf->SetBinContent(639,0.6033527);
    hrf->SetBinContent(640,0.6136293);
    hrf->SetBinContent(641,0.6365719);
    hrf->SetBinContent(642,0.663968);
    hrf->SetBinContent(643,0.6785302);
    hrf->SetBinContent(644,0.698177);
    hrf->SetBinContent(645,0.7140514);
    hrf->SetBinContent(646,0.7132909);
    hrf->SetBinContent(649,0.7509353);
    hrf->SetBinContent(650,0.7481239);
    hrf->SetBinContent(651,0.7528548);
    hrf->SetBinContent(652,0.7305032);
    hrf->SetBinContent(653,0.7161828);
    hrf->SetBinContent(654,0.6881066);
    hrf->SetBinContent(655,0.6674716);
    hrf->SetBinContent(656,0.6646308);
    hrf->SetBinContent(657,0.6692024);
    hrf->SetBinContent(658,0.5923947);
    hrf->SetBinContent(659,0.5790197);
    hrf->SetBinContent(660,0.5716576);
    hrf->SetBinContent(661,0.5632296);
    hrf->SetBinContent(662,0.5653917);
    hrf->SetBinContent(663,0.5652825);
    hrf->SetBinContent(664,0.5722731);
    hrf->SetBinContent(665,0.5687287);
    hrf->SetBinContent(666,0.5840636);
    hrf->SetBinContent(667,0.5832837);
    hrf->SetBinContent(668,0.5911939);
    hrf->SetBinContent(669,0.5879493);
    hrf->SetBinContent(670,0.6012655);
    hrf->SetBinContent(671,0.604796);
    hrf->SetBinContent(672,0.6154519);
    hrf->SetBinContent(673,0.6178173);
    hrf->SetBinContent(674,0.6027289);
    hrf->SetBinContent(675,0.6204012);
    hrf->SetBinContent(676,0.5990086);
    hrf->SetBinContent(677,0.602294);
    hrf->SetBinContent(678,0.5972872);
    hrf->SetBinContent(679,0.5872762);
    hrf->SetBinContent(680,0.5722558);
    hrf->SetBinContent(681,0.5680713);
    hrf->SetBinContent(682,0.5747243);
    hrf->SetBinContent(683,0.5765499);
    hrf->SetBinContent(684,0.5756451);
    hrf->SetBinContent(685,0.5752973);
    hrf->SetBinContent(686,0.5648109);
    hrf->SetBinContent(687,0.5616939);
    hrf->SetBinContent(688,0.5521079);
    hrf->SetBinContent(689,0.5710905);
    hrf->SetBinContent(690,0.568258);
    hrf->SetBinContent(691,0.5772039);
    hrf->SetBinContent(692,0.649491);
    hrf->SetBinContent(693,0.6336948);
    hrf->SetBinContent(694,0.6392729);
    hrf->SetBinContent(695,0.6520424);
    hrf->SetBinContent(696,0.6744816);
    hrf->SetBinContent(697,0.6857698);
    hrf->SetBinContent(698,0.706839);
    hrf->SetBinContent(699,0.7229526);
    hrf->SetBinContent(700,0.7224955);
    hrf->SetBinContent(703,0.743013);
    hrf->SetBinContent(704,0.757265);
    hrf->SetBinContent(705,0.7547972);
    hrf->SetBinContent(706,0.7373625);
    hrf->SetBinContent(707,0.7245529);
    hrf->SetBinContent(708,0.7119363);
    hrf->SetBinContent(709,0.6820973);
    hrf->SetBinContent(710,0.6710863);
    hrf->SetBinContent(711,0.6743201);
    hrf->SetBinContent(712,0.6166688);
    hrf->SetBinContent(713,0.5898132);
    hrf->SetBinContent(714,0.5747908);
    hrf->SetBinContent(715,0.5955827);
    hrf->SetBinContent(716,0.5996143);
    hrf->SetBinContent(717,0.6031038);
    hrf->SetBinContent(718,0.5980663);
    hrf->SetBinContent(719,0.6094644);
    hrf->SetBinContent(720,0.6048343);
    hrf->SetBinContent(721,0.621331);
    hrf->SetBinContent(722,0.6116534);
    hrf->SetBinContent(723,0.6016229);
    hrf->SetBinContent(724,0.6126882);
    hrf->SetBinContent(725,0.6017101);
    hrf->SetBinContent(726,0.6136634);
    hrf->SetBinContent(727,0.6025576);
    hrf->SetBinContent(728,0.6084566);
    hrf->SetBinContent(729,0.612228);
    hrf->SetBinContent(730,0.590066);
    hrf->SetBinContent(731,0.5872589);
    hrf->SetBinContent(732,0.6029574);
    hrf->SetBinContent(733,0.6015266);
    hrf->SetBinContent(734,0.5876325);
    hrf->SetBinContent(735,0.5952729);
    hrf->SetBinContent(736,0.602466);
    hrf->SetBinContent(737,0.6048284);
    hrf->SetBinContent(738,0.6180722);
    hrf->SetBinContent(739,0.603563);
    hrf->SetBinContent(740,0.5877262);
    hrf->SetBinContent(741,0.5879717);
    hrf->SetBinContent(742,0.5736278);
    hrf->SetBinContent(743,0.5741115);
    hrf->SetBinContent(744,0.579726);
    hrf->SetBinContent(745,0.5919455);
    hrf->SetBinContent(746,0.653911);
    hrf->SetBinContent(747,0.656211);
    hrf->SetBinContent(748,0.64449);
    hrf->SetBinContent(749,0.6795421);
    hrf->SetBinContent(750,0.688488);
    hrf->SetBinContent(751,0.6899459);
    hrf->SetBinContent(752,0.707727);
    hrf->SetBinContent(753,0.7373027);
    hrf->SetBinContent(754,0.7173992);
    hrf->SetBinContent(757,0.7762201);
    hrf->SetBinContent(758,0.7922555);
    hrf->SetBinContent(759,0.7618926);
    hrf->SetBinContent(760,0.7614654);
    hrf->SetBinContent(761,0.7398314);
    hrf->SetBinContent(762,0.7139991);
    hrf->SetBinContent(763,0.7087259);
    hrf->SetBinContent(764,0.6919274);
    hrf->SetBinContent(765,0.7118896);
    hrf->SetBinContent(766,0.6417386);
    hrf->SetBinContent(767,0.6284056);
    hrf->SetBinContent(768,0.6312464);
    hrf->SetBinContent(769,0.6285428);
    hrf->SetBinContent(770,0.6145315);
    hrf->SetBinContent(771,0.6398894);
    hrf->SetBinContent(772,0.6432972);
    hrf->SetBinContent(773,0.6234546);
    hrf->SetBinContent(774,0.6535459);
    hrf->SetBinContent(775,0.6370906);
    hrf->SetBinContent(776,0.6308004);
    hrf->SetBinContent(777,0.6275457);
    hrf->SetBinContent(778,0.6472575);
    hrf->SetBinContent(779,0.6540804);
    hrf->SetBinContent(780,0.6503102);
    hrf->SetBinContent(781,0.6364889);
    hrf->SetBinContent(782,0.6454083);
    hrf->SetBinContent(783,0.6431745);
    hrf->SetBinContent(784,0.6442333);
    hrf->SetBinContent(785,0.6234152);
    hrf->SetBinContent(786,0.6150312);
    hrf->SetBinContent(787,0.6318147);
    hrf->SetBinContent(788,0.660322);
    hrf->SetBinContent(789,0.6467797);
    hrf->SetBinContent(790,0.6305873);
    hrf->SetBinContent(791,0.6333795);
    hrf->SetBinContent(792,0.6365413);
    hrf->SetBinContent(793,0.6455333);
    hrf->SetBinContent(794,0.6199582);
    hrf->SetBinContent(795,0.6234086);
    hrf->SetBinContent(796,0.6432598);
    hrf->SetBinContent(797,0.6275219);
    hrf->SetBinContent(798,0.6084053);
    hrf->SetBinContent(799,0.6306369);
    hrf->SetBinContent(800,0.7352997);
    hrf->SetBinContent(801,0.689982);
    hrf->SetBinContent(802,0.6839678);
    hrf->SetBinContent(803,0.7097731);
    hrf->SetBinContent(804,0.7078945);
    hrf->SetBinContent(805,0.7525824);
    hrf->SetBinContent(806,0.7356946);
    hrf->SetBinContent(807,0.7591921);
    hrf->SetBinContent(808,0.7577095);
    hrf->SetBinContent(811,0.7851269);
    hrf->SetBinContent(812,0.7979873);
    hrf->SetBinContent(813,0.7796074);
    hrf->SetBinContent(814,0.7897721);
    hrf->SetBinContent(815,0.7673025);
    hrf->SetBinContent(816,0.7463681);
    hrf->SetBinContent(817,0.7407829);
    hrf->SetBinContent(818,0.7254772);
    hrf->SetBinContent(819,0.7501467);
    hrf->SetBinContent(820,0.682878);
    hrf->SetBinContent(821,0.6359177);
    hrf->SetBinContent(822,0.6559011);
    hrf->SetBinContent(823,0.6632125);
    hrf->SetBinContent(824,0.6639908);
    hrf->SetBinContent(825,0.6668174);
    hrf->SetBinContent(826,0.6646313);
    hrf->SetBinContent(827,0.6761202);
    hrf->SetBinContent(828,0.6506999);
    hrf->SetBinContent(829,0.6783583);
    hrf->SetBinContent(830,0.6675706);
    hrf->SetBinContent(831,0.6832684);
    hrf->SetBinContent(832,0.6736984);
    hrf->SetBinContent(833,0.6757175);
    hrf->SetBinContent(834,0.6569413);
    hrf->SetBinContent(835,0.6644614);
    hrf->SetBinContent(836,0.687782);
    hrf->SetBinContent(837,0.6644785);
    hrf->SetBinContent(838,0.6726776);
    hrf->SetBinContent(839,0.6685038);
    hrf->SetBinContent(840,0.6792084);
    hrf->SetBinContent(841,0.6823671);
    hrf->SetBinContent(842,0.6675);
    hrf->SetBinContent(843,0.6689284);
    hrf->SetBinContent(844,0.6775497);
    hrf->SetBinContent(845,0.6766756);
    hrf->SetBinContent(846,0.6684731);
    hrf->SetBinContent(847,0.6626517);
    hrf->SetBinContent(848,0.6884043);
    hrf->SetBinContent(849,0.6722244);
    hrf->SetBinContent(850,0.667429);
    hrf->SetBinContent(851,0.6427224);
    hrf->SetBinContent(852,0.6519591);
    hrf->SetBinContent(853,0.6615716);
    hrf->SetBinContent(854,0.7207934);
    hrf->SetBinContent(855,0.6980299);
    hrf->SetBinContent(856,0.705512);
    hrf->SetBinContent(857,0.7217741);
    hrf->SetBinContent(858,0.741257);
    hrf->SetBinContent(859,0.7309523);
    hrf->SetBinContent(860,0.7510293);
    hrf->SetBinContent(861,0.7698293);
    hrf->SetBinContent(862,0.7687299);
    hrf->SetBinContent(865,0.7998108);
    hrf->SetBinContent(866,0.7911402);
    hrf->SetBinContent(867,0.7859766);
    hrf->SetBinContent(868,0.7901751);
    hrf->SetBinContent(869,0.772135);
    hrf->SetBinContent(870,0.7605265);
    hrf->SetBinContent(871,0.7407048);
    hrf->SetBinContent(872,0.7427229);
    hrf->SetBinContent(873,0.7884253);
    hrf->SetBinContent(874,0.6778198);
    hrf->SetBinContent(875,0.6660694);
    hrf->SetBinContent(876,0.6575113);
    hrf->SetBinContent(877,0.6620342);
    hrf->SetBinContent(878,0.6777028);
    hrf->SetBinContent(879,0.6724306);
    hrf->SetBinContent(880,0.7063372);
    hrf->SetBinContent(881,0.6923195);
    hrf->SetBinContent(882,0.7100313);
    hrf->SetBinContent(883,0.6991185);
    hrf->SetBinContent(884,0.6900581);
    hrf->SetBinContent(885,0.6947417);
    hrf->SetBinContent(886,0.6919176);
    hrf->SetBinContent(887,0.6974078);
    hrf->SetBinContent(888,0.6919622);
    hrf->SetBinContent(889,0.7094784);
    hrf->SetBinContent(890,0.7120138);
    hrf->SetBinContent(891,0.6911546);
    hrf->SetBinContent(892,0.7008265);
    hrf->SetBinContent(893,0.6886013);
    hrf->SetBinContent(894,0.6983498);
    hrf->SetBinContent(895,0.6992657);
    hrf->SetBinContent(896,0.7070684);
    hrf->SetBinContent(897,0.7042978);
    hrf->SetBinContent(898,0.6966478);
    hrf->SetBinContent(899,0.7000308);
    hrf->SetBinContent(900,0.6895164);
    hrf->SetBinContent(901,0.6916679);
    hrf->SetBinContent(902,0.6816393);
    hrf->SetBinContent(903,0.6747249);
    hrf->SetBinContent(904,0.6695186);
    hrf->SetBinContent(905,0.6644837);
    hrf->SetBinContent(906,0.6556708);
    hrf->SetBinContent(907,0.6861832);
    hrf->SetBinContent(908,0.7365648);
    hrf->SetBinContent(909,0.7339019);
    hrf->SetBinContent(910,0.7479324);
    hrf->SetBinContent(911,0.7200305);
    hrf->SetBinContent(912,0.7402619);
    hrf->SetBinContent(913,0.7689364);
    hrf->SetBinContent(914,0.757987);
    hrf->SetBinContent(915,0.7774559);
    hrf->SetBinContent(916,0.7744513);
    hrf->SetBinContent(919,0.805436);
    hrf->SetBinContent(920,0.8116394);
    hrf->SetBinContent(921,0.7901397);
    hrf->SetBinContent(922,0.7889259);
    hrf->SetBinContent(923,0.7869421);
    hrf->SetBinContent(924,0.7832718);
    hrf->SetBinContent(925,0.7495821);
    hrf->SetBinContent(926,0.7460531);
    hrf->SetBinContent(927,0.7983673);
    hrf->SetBinContent(928,0.6756758);
    hrf->SetBinContent(929,0.6745369);
    hrf->SetBinContent(930,0.6523583);
    hrf->SetBinContent(931,0.6857114);
    hrf->SetBinContent(932,0.7022838);
    hrf->SetBinContent(933,0.6993511);
    hrf->SetBinContent(934,0.7240101);
    hrf->SetBinContent(935,0.7355589);
    hrf->SetBinContent(936,0.7311175);
    hrf->SetBinContent(937,0.7316082);
    hrf->SetBinContent(938,0.7291019);
    hrf->SetBinContent(939,0.7318584);
    hrf->SetBinContent(940,0.7403764);
    hrf->SetBinContent(941,0.7226663);
    hrf->SetBinContent(942,0.7255294);
    hrf->SetBinContent(943,0.7199827);
    hrf->SetBinContent(944,0.7234808);
    hrf->SetBinContent(945,0.7225159);
    hrf->SetBinContent(946,0.7176975);
    hrf->SetBinContent(947,0.7105052);
    hrf->SetBinContent(948,0.7047699);
    hrf->SetBinContent(949,0.7213685);
    hrf->SetBinContent(950,0.704915);
    hrf->SetBinContent(951,0.7251048);
    hrf->SetBinContent(952,0.7165749);
    hrf->SetBinContent(953,0.7134793);
    hrf->SetBinContent(954,0.7175746);
    hrf->SetBinContent(955,0.7005467);
    hrf->SetBinContent(956,0.6930621);
    hrf->SetBinContent(957,0.6914936);
    hrf->SetBinContent(958,0.6764546);
    hrf->SetBinContent(959,0.6782277);
    hrf->SetBinContent(960,0.6637185);
    hrf->SetBinContent(961,0.6757742);
    hrf->SetBinContent(962,0.765221);
    hrf->SetBinContent(963,0.7309626);
    hrf->SetBinContent(964,0.7232758);
    hrf->SetBinContent(965,0.7510916);
    hrf->SetBinContent(966,0.7605473);
    hrf->SetBinContent(967,0.7600837);
    hrf->SetBinContent(968,0.7676377);
    hrf->SetBinContent(969,0.7824863);
    hrf->SetBinContent(970,0.7901561);
    hrf->SetBinContent(973,0.8148504);
    hrf->SetBinContent(974,0.8187529);
    hrf->SetBinContent(975,0.8045154);
    hrf->SetBinContent(976,0.8017915);
    hrf->SetBinContent(977,0.7754249);
    hrf->SetBinContent(978,0.7795503);
    hrf->SetBinContent(979,0.7763172);
    hrf->SetBinContent(980,0.7717363);
    hrf->SetBinContent(981,0.7916771);
    hrf->SetBinContent(982,0.7032699);
    hrf->SetBinContent(983,0.6799935);
    hrf->SetBinContent(984,0.7084384);
    hrf->SetBinContent(985,0.695959);
    hrf->SetBinContent(986,0.7196264);
    hrf->SetBinContent(987,0.7225077);
    hrf->SetBinContent(988,0.7321133);
    hrf->SetBinContent(989,0.730117);
    hrf->SetBinContent(990,0.731978);
    hrf->SetBinContent(991,0.7549209);
    hrf->SetBinContent(992,0.7554434);
    hrf->SetBinContent(993,0.7625932);
    hrf->SetBinContent(994,0.7600612);
    hrf->SetBinContent(995,0.7579741);
    hrf->SetBinContent(996,0.7629159);
    hrf->SetBinContent(997,0.7488126);
    hrf->SetBinContent(998,0.7531744);
    hrf->SetBinContent(999,0.7593042);
    hrf->SetBinContent(1000,0.7629349);
    hrf->SetBinContent(1001,0.7525982);
    hrf->SetBinContent(1002,0.7644352);
    hrf->SetBinContent(1003,0.7675645);
    hrf->SetBinContent(1004,0.7502424);
    hrf->SetBinContent(1005,0.7487847);
    hrf->SetBinContent(1006,0.7451458);
    hrf->SetBinContent(1007,0.7391483);
    hrf->SetBinContent(1008,0.7237684);
    hrf->SetBinContent(1009,0.7352841);
    hrf->SetBinContent(1010,0.7106415);
    hrf->SetBinContent(1011,0.7013185);
    hrf->SetBinContent(1012,0.6973479);
    hrf->SetBinContent(1013,0.6976935);
    hrf->SetBinContent(1014,0.6800641);
    hrf->SetBinContent(1015,0.7096396);
    hrf->SetBinContent(1016,0.786708);
    hrf->SetBinContent(1017,0.7733635);
    hrf->SetBinContent(1018,0.752633);
    hrf->SetBinContent(1019,0.7620634);
    hrf->SetBinContent(1020,0.7723323);
    hrf->SetBinContent(1021,0.7844983);
    hrf->SetBinContent(1022,0.7910036);
    hrf->SetBinContent(1023,0.8134696);
    hrf->SetBinContent(1024,0.8024259);
    hrf->SetBinContent(1027,0.8266649);
    hrf->SetBinContent(1028,0.8246576);
    hrf->SetBinContent(1029,0.8192465);
    hrf->SetBinContent(1030,0.8153209);
    hrf->SetBinContent(1031,0.8153977);
    hrf->SetBinContent(1032,0.8028545);
    hrf->SetBinContent(1033,0.7942671);
    hrf->SetBinContent(1034,0.7753556);
    hrf->SetBinContent(1035,0.8128864);
    hrf->SetBinContent(1036,0.7450962);
    hrf->SetBinContent(1037,0.7099326);
    hrf->SetBinContent(1038,0.7087278);
    hrf->SetBinContent(1039,0.7140728);
    hrf->SetBinContent(1040,0.7095634);
    hrf->SetBinContent(1041,0.7344745);
    hrf->SetBinContent(1042,0.7486882);
    hrf->SetBinContent(1043,0.7349047);
    hrf->SetBinContent(1044,0.7710617);
    hrf->SetBinContent(1045,0.7576075);
    hrf->SetBinContent(1046,0.7625866);
    hrf->SetBinContent(1047,0.785255);
    hrf->SetBinContent(1048,0.7892328);
    hrf->SetBinContent(1049,0.7938899);
    hrf->SetBinContent(1050,0.7853175);
    hrf->SetBinContent(1051,0.7859523);
    hrf->SetBinContent(1052,0.7865126);
    hrf->SetBinContent(1053,0.7795312);
    hrf->SetBinContent(1054,0.7910514);
    hrf->SetBinContent(1055,0.7582257);
    hrf->SetBinContent(1056,0.7829325);
    hrf->SetBinContent(1057,0.7687827);
    hrf->SetBinContent(1058,0.7571276);
    hrf->SetBinContent(1059,0.7665426);
    hrf->SetBinContent(1060,0.7769426);
    hrf->SetBinContent(1061,0.7579499);
    hrf->SetBinContent(1062,0.7525055);
    hrf->SetBinContent(1063,0.7469941);
    hrf->SetBinContent(1064,0.724868);
    hrf->SetBinContent(1065,0.7379481);
    hrf->SetBinContent(1066,0.6938654);
    hrf->SetBinContent(1067,0.7067453);
    hrf->SetBinContent(1068,0.7023482);
    hrf->SetBinContent(1069,0.7144358);
    hrf->SetBinContent(1070,0.7970693);
    hrf->SetBinContent(1071,0.7756627);
    hrf->SetBinContent(1072,0.7860057);
    hrf->SetBinContent(1073,0.7892865);
    hrf->SetBinContent(1074,0.8006026);
    hrf->SetBinContent(1075,0.8051771);
    hrf->SetBinContent(1076,0.8028495);
    hrf->SetBinContent(1077,0.8149759);
    hrf->SetBinContent(1078,0.8127387);
    hrf->SetBinContent(1081,0.8472241);
    hrf->SetBinContent(1082,0.8462484);
    hrf->SetBinContent(1083,0.8294211);
    hrf->SetBinContent(1084,0.8331454);
    hrf->SetBinContent(1085,0.8233687);
    hrf->SetBinContent(1086,0.8127255);
    hrf->SetBinContent(1087,0.8096145);
    hrf->SetBinContent(1088,0.7999486);
    hrf->SetBinContent(1089,0.8344988);
    hrf->SetBinContent(1090,0.7483042);
    hrf->SetBinContent(1091,0.7213368);
    hrf->SetBinContent(1092,0.735413);
    hrf->SetBinContent(1093,0.7238604);
    hrf->SetBinContent(1094,0.7414742);
    hrf->SetBinContent(1095,0.7500997);
    hrf->SetBinContent(1096,0.7497545);
    hrf->SetBinContent(1097,0.7674037);
    hrf->SetBinContent(1098,0.7763353);
    hrf->SetBinContent(1099,0.7844313);
    hrf->SetBinContent(1100,0.7886686);
    hrf->SetBinContent(1101,0.7890237);
    hrf->SetBinContent(1102,0.7935224);
    hrf->SetBinContent(1103,0.8195034);
    hrf->SetBinContent(1104,0.7976466);
    hrf->SetBinContent(1105,0.7929903);
    hrf->SetBinContent(1106,0.7951132);
    hrf->SetBinContent(1107,0.7885212);
    hrf->SetBinContent(1108,0.7950817);
    hrf->SetBinContent(1109,0.7823561);
    hrf->SetBinContent(1110,0.8002901);
    hrf->SetBinContent(1111,0.7923795);
    hrf->SetBinContent(1112,0.7795157);
    hrf->SetBinContent(1113,0.7915238);
    hrf->SetBinContent(1114,0.7844104);
    hrf->SetBinContent(1115,0.7701443);
    hrf->SetBinContent(1116,0.7567722);
    hrf->SetBinContent(1117,0.7749527);
    hrf->SetBinContent(1118,0.742634);
    hrf->SetBinContent(1119,0.7402747);
    hrf->SetBinContent(1120,0.7289827);
    hrf->SetBinContent(1121,0.7194493);
    hrf->SetBinContent(1122,0.719686);
    hrf->SetBinContent(1123,0.7508826);
    hrf->SetBinContent(1124,0.8226015);
    hrf->SetBinContent(1125,0.7943595);
    hrf->SetBinContent(1126,0.7803666);
    hrf->SetBinContent(1127,0.7861452);
    hrf->SetBinContent(1128,0.8111635);
    hrf->SetBinContent(1129,0.8178813);
    hrf->SetBinContent(1130,0.8294749);
    hrf->SetBinContent(1131,0.8419492);
    hrf->SetBinContent(1132,0.8335012);
    hrf->SetBinContent(1135,0.8509099);
    hrf->SetBinContent(1136,0.8614552);
    hrf->SetBinContent(1137,0.8490606);
    hrf->SetBinContent(1138,0.8361901);
    hrf->SetBinContent(1139,0.827865);
    hrf->SetBinContent(1140,0.8235855);
    hrf->SetBinContent(1141,0.8103971);
    hrf->SetBinContent(1142,0.8119806);
    hrf->SetBinContent(1143,0.8442774);
    hrf->SetBinContent(1144,0.7516465);
    hrf->SetBinContent(1145,0.7318999);
    hrf->SetBinContent(1146,0.7272938);
    hrf->SetBinContent(1147,0.7503273);
    hrf->SetBinContent(1148,0.7630686);
    hrf->SetBinContent(1149,0.7665451);
    hrf->SetBinContent(1150,0.7636703);
    hrf->SetBinContent(1151,0.8004253);
    hrf->SetBinContent(1152,0.7917111);
    hrf->SetBinContent(1153,0.7930679);
    hrf->SetBinContent(1154,0.7844377);
    hrf->SetBinContent(1155,0.7881631);
    hrf->SetBinContent(1156,0.8106493);
    hrf->SetBinContent(1157,0.812787);
    hrf->SetBinContent(1158,0.7958186);
    hrf->SetBinContent(1159,0.8061783);
    hrf->SetBinContent(1160,0.8078495);
    hrf->SetBinContent(1161,0.811165);
    hrf->SetBinContent(1162,0.7972608);
    hrf->SetBinContent(1163,0.7959145);
    hrf->SetBinContent(1164,0.8054785);
    hrf->SetBinContent(1165,0.8060117);
    hrf->SetBinContent(1166,0.8036563);
    hrf->SetBinContent(1167,0.7856187);
    hrf->SetBinContent(1168,0.8039296);
    hrf->SetBinContent(1169,0.78768);
    hrf->SetBinContent(1170,0.7756561);
    hrf->SetBinContent(1171,0.7519668);
    hrf->SetBinContent(1172,0.7740593);
    hrf->SetBinContent(1173,0.7555604);
    hrf->SetBinContent(1174,0.748863);
    hrf->SetBinContent(1175,0.7474926);
    hrf->SetBinContent(1176,0.7390568);
    hrf->SetBinContent(1177,0.7548794);
    hrf->SetBinContent(1178,0.8286866);
    hrf->SetBinContent(1179,0.8024026);
    hrf->SetBinContent(1180,0.8119431);
    hrf->SetBinContent(1181,0.8227212);
    hrf->SetBinContent(1182,0.8178754);
    hrf->SetBinContent(1183,0.8294048);
    hrf->SetBinContent(1184,0.8381754);
    hrf->SetBinContent(1185,0.8470465);
    hrf->SetBinContent(1186,0.8474937);
    hrf->SetBinContent(1189,0.8508152);
    hrf->SetBinContent(1190,0.8581914);
    hrf->SetBinContent(1191,0.8611621);
    hrf->SetBinContent(1192,0.8429426);
    hrf->SetBinContent(1193,0.8337341);
    hrf->SetBinContent(1194,0.8249326);
    hrf->SetBinContent(1195,0.832922);
    hrf->SetBinContent(1196,0.8329102);
    hrf->SetBinContent(1197,0.8513843);
    hrf->SetBinContent(1198,0.7611291);
    hrf->SetBinContent(1199,0.7421638);
    hrf->SetBinContent(1200,0.7457579);
    hrf->SetBinContent(1201,0.7504473);
    hrf->SetBinContent(1202,0.7652636);
    hrf->SetBinContent(1203,0.7717913);
    hrf->SetBinContent(1204,0.7823215);
    hrf->SetBinContent(1205,0.7836031);
    hrf->SetBinContent(1206,0.8022784);
    hrf->SetBinContent(1207,0.8044854);
    hrf->SetBinContent(1208,0.7928731);
    hrf->SetBinContent(1209,0.8126316);
    hrf->SetBinContent(1210,0.8246453);
    hrf->SetBinContent(1211,0.8105548);
    hrf->SetBinContent(1212,0.828289);
    hrf->SetBinContent(1213,0.8192238);
    hrf->SetBinContent(1214,0.8152702);
    hrf->SetBinContent(1215,0.8159017);
    hrf->SetBinContent(1216,0.822512);
    hrf->SetBinContent(1217,0.8189492);
    hrf->SetBinContent(1218,0.8206261);
    hrf->SetBinContent(1219,0.8188085);
    hrf->SetBinContent(1220,0.8103344);
    hrf->SetBinContent(1221,0.796335);
    hrf->SetBinContent(1222,0.7867166);
    hrf->SetBinContent(1223,0.798481);
    hrf->SetBinContent(1224,0.7919205);
    hrf->SetBinContent(1225,0.7957431);
    hrf->SetBinContent(1226,0.7780355);
    hrf->SetBinContent(1227,0.7633665);
    hrf->SetBinContent(1228,0.7664109);
    hrf->SetBinContent(1229,0.7620563);
    hrf->SetBinContent(1230,0.7419543);
    hrf->SetBinContent(1231,0.7526901);
    hrf->SetBinContent(1232,0.8415922);
    hrf->SetBinContent(1233,0.8180589);
    hrf->SetBinContent(1234,0.8152712);
    hrf->SetBinContent(1235,0.8263748);
    hrf->SetBinContent(1236,0.8321477);
    hrf->SetBinContent(1237,0.847544);
    hrf->SetBinContent(1238,0.8432384);
    hrf->SetBinContent(1239,0.8535107);
    hrf->SetBinContent(1240,0.8540379);
    hrf->SetBinContent(1243,0.8553762);
    hrf->SetBinContent(1244,0.8623774);
    hrf->SetBinContent(1245,0.8597532);
    hrf->SetBinContent(1246,0.8520666);
    hrf->SetBinContent(1247,0.8493451);
    hrf->SetBinContent(1248,0.8332564);
    hrf->SetBinContent(1249,0.843973);
    hrf->SetBinContent(1250,0.8376496);
    hrf->SetBinContent(1251,0.8624837);
    hrf->SetBinContent(1252,0.7732126);
    hrf->SetBinContent(1253,0.7609761);
    hrf->SetBinContent(1254,0.7617697);
    hrf->SetBinContent(1255,0.7664544);
    hrf->SetBinContent(1256,0.7856858);
    hrf->SetBinContent(1257,0.777308);
    hrf->SetBinContent(1258,0.7918889);
    hrf->SetBinContent(1259,0.8046429);
    hrf->SetBinContent(1260,0.8081163);
    hrf->SetBinContent(1261,0.8186274);
    hrf->SetBinContent(1262,0.8140515);
    hrf->SetBinContent(1263,0.826897);
    hrf->SetBinContent(1264,0.824986);
    hrf->SetBinContent(1265,0.8280492);
    hrf->SetBinContent(1266,0.8122628);
    hrf->SetBinContent(1267,0.8181385);
    hrf->SetBinContent(1268,0.8257712);
    hrf->SetBinContent(1269,0.818505);
    hrf->SetBinContent(1270,0.8129805);
    hrf->SetBinContent(1271,0.8177031);
    hrf->SetBinContent(1272,0.8128901);
    hrf->SetBinContent(1273,0.8148227);
    hrf->SetBinContent(1274,0.809682);
    hrf->SetBinContent(1275,0.816658);
    hrf->SetBinContent(1276,0.8005593);
    hrf->SetBinContent(1277,0.798992);
    hrf->SetBinContent(1278,0.7870884);
    hrf->SetBinContent(1279,0.790388);
    hrf->SetBinContent(1280,0.7694438);
    hrf->SetBinContent(1281,0.7739675);
    hrf->SetBinContent(1282,0.7509179);
    hrf->SetBinContent(1283,0.7555026);
    hrf->SetBinContent(1284,0.761837);
    hrf->SetBinContent(1285,0.7733455);
    hrf->SetBinContent(1286,0.8462643);
    hrf->SetBinContent(1287,0.8282765);
    hrf->SetBinContent(1288,0.8263845);
    hrf->SetBinContent(1289,0.8340995);
    hrf->SetBinContent(1290,0.8408461);
    hrf->SetBinContent(1291,0.8472092);
    hrf->SetBinContent(1292,0.8550544);
    hrf->SetBinContent(1293,0.8541167);
    hrf->SetBinContent(1294,0.8496448);
    hrf->SetBinContent(1297,0.8611135);
    hrf->SetBinContent(1298,0.8703642);
    hrf->SetBinContent(1299,0.8727389);
    hrf->SetBinContent(1300,0.8656142);
    hrf->SetBinContent(1301,0.8595632);
    hrf->SetBinContent(1302,0.852265);
    hrf->SetBinContent(1303,0.8406764);
    hrf->SetBinContent(1304,0.8430285);
    hrf->SetBinContent(1305,0.8683907);
    hrf->SetBinContent(1306,0.8034278);
    hrf->SetBinContent(1307,0.7780163);
    hrf->SetBinContent(1308,0.7640301);
    hrf->SetBinContent(1309,0.7647865);
    hrf->SetBinContent(1310,0.7951781);
    hrf->SetBinContent(1311,0.7949511);
    hrf->SetBinContent(1312,0.801971);
    hrf->SetBinContent(1313,0.8150673);
    hrf->SetBinContent(1314,0.8136585);
    hrf->SetBinContent(1315,0.8342834);
    hrf->SetBinContent(1316,0.825223);
    hrf->SetBinContent(1317,0.8254063);
    hrf->SetBinContent(1318,0.8396173);
    hrf->SetBinContent(1319,0.8193269);
    hrf->SetBinContent(1320,0.8262784);
    hrf->SetBinContent(1321,0.8338575);
    hrf->SetBinContent(1322,0.8282815);
    hrf->SetBinContent(1323,0.832306);
    hrf->SetBinContent(1324,0.8440025);
    hrf->SetBinContent(1325,0.8351529);
    hrf->SetBinContent(1326,0.8234144);
    hrf->SetBinContent(1327,0.8336404);
    hrf->SetBinContent(1328,0.8351884);
    hrf->SetBinContent(1329,0.8200249);
    hrf->SetBinContent(1330,0.8113003);
    hrf->SetBinContent(1331,0.8198286);
    hrf->SetBinContent(1332,0.8049809);
    hrf->SetBinContent(1333,0.8141916);
    hrf->SetBinContent(1334,0.8112144);
    hrf->SetBinContent(1335,0.7844836);
    hrf->SetBinContent(1336,0.7827156);
    hrf->SetBinContent(1337,0.7744574);
    hrf->SetBinContent(1338,0.7615972);
    hrf->SetBinContent(1339,0.7685483);
    hrf->SetBinContent(1340,0.8586424);
    hrf->SetBinContent(1341,0.8540938);
    hrf->SetBinContent(1342,0.836296);
    hrf->SetBinContent(1343,0.834484);
    hrf->SetBinContent(1344,0.8497908);
    hrf->SetBinContent(1345,0.863311);
    hrf->SetBinContent(1346,0.8614742);
    hrf->SetBinContent(1347,0.8622196);
    hrf->SetBinContent(1348,0.8572411);
    hrf->SetBinContent(1351,0.8859822);
    hrf->SetBinContent(1352,0.8813127);
    hrf->SetBinContent(1353,0.877708);
    hrf->SetBinContent(1354,0.8811167);
    hrf->SetBinContent(1355,0.8709751);
    hrf->SetBinContent(1356,0.856346);
    hrf->SetBinContent(1357,0.8536229);
    hrf->SetBinContent(1358,0.8636259);
    hrf->SetBinContent(1359,0.8924866);
    hrf->SetBinContent(1360,0.7996697);
    hrf->SetBinContent(1361,0.7842888);
    hrf->SetBinContent(1362,0.7688008);
    hrf->SetBinContent(1363,0.7989625);
    hrf->SetBinContent(1364,0.8062073);
    hrf->SetBinContent(1365,0.7940506);
    hrf->SetBinContent(1366,0.8175641);
    hrf->SetBinContent(1367,0.8259819);
    hrf->SetBinContent(1368,0.8398951);
    hrf->SetBinContent(1369,0.8307853);
    hrf->SetBinContent(1370,0.8286684);
    hrf->SetBinContent(1371,0.8267256);
    hrf->SetBinContent(1372,0.8541481);
    hrf->SetBinContent(1373,0.8410212);
    hrf->SetBinContent(1374,0.8552721);
    hrf->SetBinContent(1375,0.8460447);
    hrf->SetBinContent(1376,0.851162);
    hrf->SetBinContent(1377,0.8552656);
    hrf->SetBinContent(1378,0.8495465);
    hrf->SetBinContent(1379,0.8454371);
    hrf->SetBinContent(1380,0.8294467);
    hrf->SetBinContent(1381,0.8445339);
    hrf->SetBinContent(1382,0.8327602);
    hrf->SetBinContent(1383,0.8308812);
    hrf->SetBinContent(1384,0.8329239);
    hrf->SetBinContent(1385,0.8236286);
    hrf->SetBinContent(1386,0.8077038);
    hrf->SetBinContent(1387,0.8395121);
    hrf->SetBinContent(1388,0.8175387);
    hrf->SetBinContent(1389,0.7976534);
    hrf->SetBinContent(1390,0.8014926);
    hrf->SetBinContent(1391,0.7732332);
    hrf->SetBinContent(1392,0.7764672);
    hrf->SetBinContent(1393,0.7958435);
    hrf->SetBinContent(1394,0.8715696);
    hrf->SetBinContent(1395,0.8588074);
    hrf->SetBinContent(1396,0.8559551);
    hrf->SetBinContent(1397,0.859411);
    hrf->SetBinContent(1398,0.8659895);
    hrf->SetBinContent(1399,0.8712149);
    hrf->SetBinContent(1400,0.8672059);
    hrf->SetBinContent(1401,0.881777);
    hrf->SetBinContent(1402,0.8758276);
    hrf->SetBinContent(1405,0.8958947);
    hrf->SetBinContent(1406,0.8905468);
    hrf->SetBinContent(1407,0.8933099);
    hrf->SetBinContent(1408,0.8891889);
    hrf->SetBinContent(1409,0.8783801);
    hrf->SetBinContent(1410,0.8562557);
    hrf->SetBinContent(1411,0.871002);
    hrf->SetBinContent(1412,0.865341);
    hrf->SetBinContent(1413,0.8685762);
    hrf->SetBinContent(1414,0.8075956);
    hrf->SetBinContent(1415,0.7889336);
    hrf->SetBinContent(1416,0.7952157);
    hrf->SetBinContent(1417,0.7951327);
    hrf->SetBinContent(1418,0.8162889);
    hrf->SetBinContent(1419,0.8219948);
    hrf->SetBinContent(1420,0.8119217);
    hrf->SetBinContent(1421,0.8383381);
    hrf->SetBinContent(1422,0.8395214);
    hrf->SetBinContent(1423,0.841587);
    hrf->SetBinContent(1424,0.8357058);
    hrf->SetBinContent(1425,0.8520364);
    hrf->SetBinContent(1426,0.8735436);
    hrf->SetBinContent(1427,0.8683173);
    hrf->SetBinContent(1428,0.8567585);
    hrf->SetBinContent(1429,0.8503947);
    hrf->SetBinContent(1430,0.8429065);
    hrf->SetBinContent(1431,0.8534837);
    hrf->SetBinContent(1432,0.8536079);
    hrf->SetBinContent(1433,0.8533946);
    hrf->SetBinContent(1434,0.8551936);
    hrf->SetBinContent(1435,0.8466191);
    hrf->SetBinContent(1436,0.8480928);
    hrf->SetBinContent(1437,0.8509755);
    hrf->SetBinContent(1438,0.8416375);
    hrf->SetBinContent(1439,0.8435714);
    hrf->SetBinContent(1440,0.8286923);
    hrf->SetBinContent(1441,0.8335129);
    hrf->SetBinContent(1442,0.8144135);
    hrf->SetBinContent(1443,0.8146556);
    hrf->SetBinContent(1444,0.7943402);
    hrf->SetBinContent(1445,0.7955962);
    hrf->SetBinContent(1446,0.7923511);
    hrf->SetBinContent(1447,0.8153083);
    hrf->SetBinContent(1448,0.8766764);
    hrf->SetBinContent(1449,0.8663556);
    hrf->SetBinContent(1450,0.8658811);
    hrf->SetBinContent(1451,0.8591501);
    hrf->SetBinContent(1452,0.8616685);
    hrf->SetBinContent(1453,0.8760469);
    hrf->SetBinContent(1454,0.8782374);
    hrf->SetBinContent(1455,0.8900301);
    hrf->SetBinContent(1456,0.8846641);
    hrf->SetBinContent(1459,0.8899569);
    hrf->SetBinContent(1460,0.8937837);
    hrf->SetBinContent(1461,0.8894906);
    hrf->SetBinContent(1462,0.8830953);
    hrf->SetBinContent(1463,0.8919073);
    hrf->SetBinContent(1464,0.8855677);
    hrf->SetBinContent(1465,0.8732993);
    hrf->SetBinContent(1466,0.8894289);
    hrf->SetBinContent(1467,0.8791205);
    hrf->SetBinContent(1468,0.8194761);
    hrf->SetBinContent(1469,0.8044042);
    hrf->SetBinContent(1470,0.7958142);
    hrf->SetBinContent(1471,0.8175958);
    hrf->SetBinContent(1472,0.8150435);
    hrf->SetBinContent(1473,0.8275356);
    hrf->SetBinContent(1474,0.8274845);
    hrf->SetBinContent(1475,0.851217);
    hrf->SetBinContent(1476,0.8429838);
    hrf->SetBinContent(1477,0.8491835);
    hrf->SetBinContent(1478,0.8595371);
    hrf->SetBinContent(1479,0.8560484);
    hrf->SetBinContent(1480,0.8527646);
    hrf->SetBinContent(1481,0.8674493);
    hrf->SetBinContent(1482,0.8636762);
    hrf->SetBinContent(1483,0.860177);
    hrf->SetBinContent(1484,0.8537801);
    hrf->SetBinContent(1485,0.8562581);
    hrf->SetBinContent(1486,0.8573899);
    hrf->SetBinContent(1487,0.8537657);
    hrf->SetBinContent(1488,0.8625689);
    hrf->SetBinContent(1489,0.8668813);
    hrf->SetBinContent(1490,0.8618633);
    hrf->SetBinContent(1491,0.8445396);
    hrf->SetBinContent(1492,0.8451921);
    hrf->SetBinContent(1493,0.8524957);
    hrf->SetBinContent(1494,0.839224);
    hrf->SetBinContent(1495,0.8353939);
    hrf->SetBinContent(1496,0.8320506);
    hrf->SetBinContent(1497,0.8131844);
    hrf->SetBinContent(1498,0.815483);
    hrf->SetBinContent(1499,0.7837349);
    hrf->SetBinContent(1500,0.8101255);
    hrf->SetBinContent(1501,0.8168123);
    hrf->SetBinContent(1502,0.8915868);
    hrf->SetBinContent(1503,0.8676065);
    hrf->SetBinContent(1504,0.8605552);
    hrf->SetBinContent(1505,0.8756496);
    hrf->SetBinContent(1506,0.8782847);
    hrf->SetBinContent(1507,0.8874688);
    hrf->SetBinContent(1508,0.8869709);
    hrf->SetBinContent(1509,0.8894036);
    hrf->SetBinContent(1510,0.8837066);
    hrf->SetBinContent(1513,0.8632733);
    hrf->SetBinContent(1514,0.8904168);
    hrf->SetBinContent(1515,0.8904095);
    hrf->SetBinContent(1516,0.8860579);
    hrf->SetBinContent(1517,0.8803535);
    hrf->SetBinContent(1518,0.8749995);
    hrf->SetBinContent(1519,0.8786336);
    hrf->SetBinContent(1520,0.8782337);
    hrf->SetBinContent(1521,0.9000699);
    hrf->SetBinContent(1522,0.8236216);
    hrf->SetBinContent(1523,0.8062729);
    hrf->SetBinContent(1524,0.806299);
    hrf->SetBinContent(1525,0.8139138);
    hrf->SetBinContent(1526,0.8153047);
    hrf->SetBinContent(1527,0.83806);
    hrf->SetBinContent(1528,0.8379446);
    hrf->SetBinContent(1529,0.8511885);
    hrf->SetBinContent(1530,0.8498399);
    hrf->SetBinContent(1531,0.8478335);
    hrf->SetBinContent(1532,0.8583114);
    hrf->SetBinContent(1533,0.8610271);
    hrf->SetBinContent(1534,0.8568704);
    hrf->SetBinContent(1535,0.8748058);
    hrf->SetBinContent(1536,0.8568359);
    hrf->SetBinContent(1537,0.8538641);
    hrf->SetBinContent(1538,0.8676881);
    hrf->SetBinContent(1539,0.8624924);
    hrf->SetBinContent(1540,0.8572467);
    hrf->SetBinContent(1541,0.8720655);
    hrf->SetBinContent(1542,0.8567328);
    hrf->SetBinContent(1543,0.8648477);
    hrf->SetBinContent(1544,0.8636746);
    hrf->SetBinContent(1545,0.8551086);
    hrf->SetBinContent(1546,0.8583532);
    hrf->SetBinContent(1547,0.8387756);
    hrf->SetBinContent(1548,0.8410261);
    hrf->SetBinContent(1549,0.8315368);
    hrf->SetBinContent(1550,0.8209644);
    hrf->SetBinContent(1551,0.8305156);
    hrf->SetBinContent(1552,0.813111);
    hrf->SetBinContent(1553,0.7837745);
    hrf->SetBinContent(1554,0.809455);
    hrf->SetBinContent(1555,0.821058);
    hrf->SetBinContent(1556,0.8854335);
    hrf->SetBinContent(1557,0.877328);
    hrf->SetBinContent(1558,0.877007);
    hrf->SetBinContent(1559,0.8668342);
    hrf->SetBinContent(1560,0.8815846);
    hrf->SetBinContent(1561,0.8797744);
    hrf->SetBinContent(1562,0.8901795);
    hrf->SetBinContent(1563,0.8872392);
    hrf->SetBinContent(1564,0.8585756);
 
    return hrf;
  }

  // ------------ single pion response function from mode ------------
  TH2D* MuonTCMETValueMapProducer::getResponseFunction_mode() {
  
    Double_t xAxis2[53] = {-2.5, -2.322, -2.172, -2.043, 
                           -1.93, -1.83, -1.74, -1.653, 
                           -1.566, -1.479, -1.392, -1.305, 
                           -1.218, -1.131, -1.044, -0.957, 
                           -0.879, -0.783, -0.696, -0.609, 
                           -0.522, -0.435, -0.348, -0.261, 
                           -0.174, -0.087, 0, 0.087, 0.174, 
                           0.261, 0.348, 0.435, 0.522, 0.609, 
                           0.696, 0.783, 0.879, 0.957, 1.044, 
                           1.131, 1.218, 1.305, 1.392, 1.479, 
                           1.566, 1.653, 1.74, 1.83, 1.93, 
                           2.043, 2.172, 2.322, 2.5}; 
  
    Double_t yAxis2[29] = {0, 0.5, 1, 1.5, 2, 3, 4, 5, 6, 7, 
                           8, 9, 10, 12, 14, 16, 18, 20, 25, 
                           30, 35, 40, 45, 50, 60, 70, 80, 90, 100}; 
   
    TH2D *hrf = new TH2D("hrf","RF  (mode)",52, xAxis2,28, yAxis2);

    hrf->SetBinContent(163,0.5);
    hrf->SetBinContent(164,0.58);
    hrf->SetBinContent(165,0.62);
    hrf->SetBinContent(166,0.54);
    hrf->SetBinContent(167,0.46);
    hrf->SetBinContent(168,0.14);
    hrf->SetBinContent(169,0.14);
    hrf->SetBinContent(170,0.02);
    hrf->SetBinContent(171,0.02);
    hrf->SetBinContent(172,0.02);
    hrf->SetBinContent(173,0.02);
    hrf->SetBinContent(174,0.02);
    hrf->SetBinContent(175,0.02);
    hrf->SetBinContent(176,0.02);
    hrf->SetBinContent(177,0.02);
    hrf->SetBinContent(178,0.02);
    hrf->SetBinContent(179,0.02);
    hrf->SetBinContent(180,0.02);
    hrf->SetBinContent(181,0.02);
    hrf->SetBinContent(182,0.02);
    hrf->SetBinContent(183,0.02);
    hrf->SetBinContent(184,0.02);
    hrf->SetBinContent(185,0.02);
    hrf->SetBinContent(186,0.02);
    hrf->SetBinContent(187,0.02);
    hrf->SetBinContent(188,0.02);
    hrf->SetBinContent(189,0.02);
    hrf->SetBinContent(190,0.02);
    hrf->SetBinContent(191,0.02);
    hrf->SetBinContent(192,0.02);
    hrf->SetBinContent(193,0.02);
    hrf->SetBinContent(194,0.02);
    hrf->SetBinContent(195,0.02);
    hrf->SetBinContent(196,0.02);
    hrf->SetBinContent(197,0.02);
    hrf->SetBinContent(198,0.02);
    hrf->SetBinContent(199,0.02);
    hrf->SetBinContent(200,0.02);
    hrf->SetBinContent(201,0.02);
    hrf->SetBinContent(202,0.02);
    hrf->SetBinContent(203,0.02);
    hrf->SetBinContent(204,0.02);
    hrf->SetBinContent(205,0.02);
    hrf->SetBinContent(206,0.02);
    hrf->SetBinContent(207,0.02);
    hrf->SetBinContent(208,0.02);
    hrf->SetBinContent(209,0.02);
    hrf->SetBinContent(210,0.14);
    hrf->SetBinContent(211,0.1);
    hrf->SetBinContent(212,0.34);
    hrf->SetBinContent(213,0.34);
    hrf->SetBinContent(214,0.38);
    hrf->SetBinContent(217,0.5);
    hrf->SetBinContent(218,0.54);
    hrf->SetBinContent(219,0.5);
    hrf->SetBinContent(220,0.58);
    hrf->SetBinContent(221,0.46);
    hrf->SetBinContent(222,0.5);
    hrf->SetBinContent(223,0.1);
    hrf->SetBinContent(224,0.22);
    hrf->SetBinContent(225,0.02);
    hrf->SetBinContent(226,0.02);
    hrf->SetBinContent(227,0.02);
    hrf->SetBinContent(228,0.02);
    hrf->SetBinContent(229,0.02);
    hrf->SetBinContent(230,0.02);
    hrf->SetBinContent(231,0.02);
    hrf->SetBinContent(232,0.02);
    hrf->SetBinContent(233,0.02);
    hrf->SetBinContent(234,0.02);
    hrf->SetBinContent(235,0.02);
    hrf->SetBinContent(236,0.02);
    hrf->SetBinContent(237,0.02);
    hrf->SetBinContent(238,0.02);
    hrf->SetBinContent(239,0.02);
    hrf->SetBinContent(240,0.02);
    hrf->SetBinContent(241,0.02);
    hrf->SetBinContent(242,0.02);
    hrf->SetBinContent(243,0.02);
    hrf->SetBinContent(244,0.02);
    hrf->SetBinContent(245,0.02);
    hrf->SetBinContent(246,0.02);
    hrf->SetBinContent(247,0.02);
    hrf->SetBinContent(248,0.02);
    hrf->SetBinContent(249,0.02);
    hrf->SetBinContent(250,0.02);
    hrf->SetBinContent(251,0.02);
    hrf->SetBinContent(252,0.02);
    hrf->SetBinContent(253,0.02);
    hrf->SetBinContent(254,0.02);
    hrf->SetBinContent(255,0.02);
    hrf->SetBinContent(256,0.02);
    hrf->SetBinContent(257,0.02);
    hrf->SetBinContent(258,0.02);
    hrf->SetBinContent(259,0.02);
    hrf->SetBinContent(260,0.02);
    hrf->SetBinContent(261,0.02);
    hrf->SetBinContent(262,0.1);
    hrf->SetBinContent(263,0.1);
    hrf->SetBinContent(264,0.18);
    hrf->SetBinContent(265,0.38);
    hrf->SetBinContent(266,0.38);
    hrf->SetBinContent(267,0.42);
    hrf->SetBinContent(268,0.38);
    hrf->SetBinContent(271,0.5);
    hrf->SetBinContent(272,0.58);
    hrf->SetBinContent(273,0.58);
    hrf->SetBinContent(274,0.58);
    hrf->SetBinContent(275,0.42);
    hrf->SetBinContent(276,0.42);
    hrf->SetBinContent(277,0.38);
    hrf->SetBinContent(278,0.38);
    hrf->SetBinContent(279,0.18);
    hrf->SetBinContent(280,0.38);
    hrf->SetBinContent(281,0.38);
    hrf->SetBinContent(282,0.3);
    hrf->SetBinContent(283,0.3);
    hrf->SetBinContent(284,0.06);
    hrf->SetBinContent(285,0.06);
    hrf->SetBinContent(286,0.02);
    hrf->SetBinContent(287,0.02);
    hrf->SetBinContent(288,0.3);
    hrf->SetBinContent(289,0.02);
    hrf->SetBinContent(290,0.02);
    hrf->SetBinContent(291,0.02);
    hrf->SetBinContent(292,0.02);
    hrf->SetBinContent(293,0.02);
    hrf->SetBinContent(294,0.02);
    hrf->SetBinContent(295,0.02);
    hrf->SetBinContent(296,0.02);
    hrf->SetBinContent(297,0.02);
    hrf->SetBinContent(298,0.02);
    hrf->SetBinContent(299,0.02);
    hrf->SetBinContent(300,0.02);
    hrf->SetBinContent(301,0.02);
    hrf->SetBinContent(302,0.02);
    hrf->SetBinContent(303,0.02);
    hrf->SetBinContent(304,0.02);
    hrf->SetBinContent(305,0.02);
    hrf->SetBinContent(306,0.02);
    hrf->SetBinContent(307,0.02);
    hrf->SetBinContent(308,0.02);
    hrf->SetBinContent(309,0.02);
    hrf->SetBinContent(310,0.02);
    hrf->SetBinContent(311,0.3);
    hrf->SetBinContent(312,0.06);
    hrf->SetBinContent(313,0.02);
    hrf->SetBinContent(314,0.02);
    hrf->SetBinContent(315,0.3);
    hrf->SetBinContent(316,0.18);
    hrf->SetBinContent(317,0.26);
    hrf->SetBinContent(318,0.22);
    hrf->SetBinContent(319,0.34);
    hrf->SetBinContent(320,0.38);
    hrf->SetBinContent(321,0.5);
    hrf->SetBinContent(322,0.38);
    hrf->SetBinContent(325,0.66);
    hrf->SetBinContent(326,0.54);
    hrf->SetBinContent(327,0.58);
    hrf->SetBinContent(328,0.5);
    hrf->SetBinContent(329,0.38);
    hrf->SetBinContent(330,0.58);
    hrf->SetBinContent(331,0.42);
    hrf->SetBinContent(332,0.38);
    hrf->SetBinContent(333,0.38);
    hrf->SetBinContent(334,0.3);
    hrf->SetBinContent(335,0.3);
    hrf->SetBinContent(336,0.38);
    hrf->SetBinContent(337,0.3);
    hrf->SetBinContent(338,0.3);
    hrf->SetBinContent(339,0.34);
    hrf->SetBinContent(340,0.3);
    hrf->SetBinContent(341,0.26);
    hrf->SetBinContent(342,0.3);
    hrf->SetBinContent(343,0.34);
    hrf->SetBinContent(344,0.34);
    hrf->SetBinContent(345,0.34);
    hrf->SetBinContent(346,0.3);
    hrf->SetBinContent(347,0.46);
    hrf->SetBinContent(348,0.34);
    hrf->SetBinContent(349,0.38);
    hrf->SetBinContent(350,0.38);
    hrf->SetBinContent(351,0.3);
    hrf->SetBinContent(352,0.34);
    hrf->SetBinContent(353,0.34);
    hrf->SetBinContent(354,0.34);
    hrf->SetBinContent(355,0.38);
    hrf->SetBinContent(356,0.3);
    hrf->SetBinContent(357,0.3);
    hrf->SetBinContent(358,0.34);
    hrf->SetBinContent(359,0.34);
    hrf->SetBinContent(360,0.34);
    hrf->SetBinContent(361,0.34);
    hrf->SetBinContent(362,0.26);
    hrf->SetBinContent(363,0.38);
    hrf->SetBinContent(364,0.3);
    hrf->SetBinContent(365,0.3);
    hrf->SetBinContent(366,0.26);
    hrf->SetBinContent(367,0.3);
    hrf->SetBinContent(368,0.26);
    hrf->SetBinContent(369,0.3);
    hrf->SetBinContent(370,0.34);
    hrf->SetBinContent(371,0.5);
    hrf->SetBinContent(372,0.46);
    hrf->SetBinContent(373,0.46);
    hrf->SetBinContent(374,0.42);
    hrf->SetBinContent(375,0.54);
    hrf->SetBinContent(376,0.5);
    hrf->SetBinContent(379,0.74);
    hrf->SetBinContent(380,0.66);
    hrf->SetBinContent(381,0.74);
    hrf->SetBinContent(382,0.62);
    hrf->SetBinContent(383,0.54);
    hrf->SetBinContent(384,0.46);
    hrf->SetBinContent(385,0.46);
    hrf->SetBinContent(386,0.38);
    hrf->SetBinContent(387,0.34);
    hrf->SetBinContent(388,0.42);
    hrf->SetBinContent(389,0.46);
    hrf->SetBinContent(390,0.38);
    hrf->SetBinContent(391,0.42);
    hrf->SetBinContent(392,0.34);
    hrf->SetBinContent(393,0.42);
    hrf->SetBinContent(394,0.46);
    hrf->SetBinContent(395,0.38);
    hrf->SetBinContent(396,0.38);
    hrf->SetBinContent(397,0.5);
    hrf->SetBinContent(398,0.38);
    hrf->SetBinContent(399,0.42);
    hrf->SetBinContent(400,0.5);
    hrf->SetBinContent(401,0.38);
    hrf->SetBinContent(402,0.46);
    hrf->SetBinContent(403,0.42);
    hrf->SetBinContent(404,0.46);
    hrf->SetBinContent(405,0.38);
    hrf->SetBinContent(406,0.42);
    hrf->SetBinContent(407,0.46);
    hrf->SetBinContent(408,0.46);
    hrf->SetBinContent(409,0.46);
    hrf->SetBinContent(410,0.34);
    hrf->SetBinContent(411,0.46);
    hrf->SetBinContent(412,0.46);
    hrf->SetBinContent(413,0.46);
    hrf->SetBinContent(414,0.46);
    hrf->SetBinContent(415,0.34);
    hrf->SetBinContent(416,0.46);
    hrf->SetBinContent(417,0.42);
    hrf->SetBinContent(418,0.3);
    hrf->SetBinContent(419,0.46);
    hrf->SetBinContent(420,0.42);
    hrf->SetBinContent(421,0.38);
    hrf->SetBinContent(422,0.38);
    hrf->SetBinContent(423,0.5);
    hrf->SetBinContent(424,0.38);
    hrf->SetBinContent(425,0.46);
    hrf->SetBinContent(426,0.5);
    hrf->SetBinContent(427,0.58);
    hrf->SetBinContent(428,0.5);
    hrf->SetBinContent(429,0.54);
    hrf->SetBinContent(430,0.62);
    hrf->SetBinContent(433,0.74);
    hrf->SetBinContent(434,0.74);
    hrf->SetBinContent(435,0.74);
    hrf->SetBinContent(436,0.66);
    hrf->SetBinContent(437,0.58);
    hrf->SetBinContent(438,0.62);
    hrf->SetBinContent(439,0.54);
    hrf->SetBinContent(440,0.54);
    hrf->SetBinContent(441,0.46);
    hrf->SetBinContent(442,0.42);
    hrf->SetBinContent(443,0.46);
    hrf->SetBinContent(444,0.46);
    hrf->SetBinContent(445,0.34);
    hrf->SetBinContent(446,0.42);
    hrf->SetBinContent(447,0.38);
    hrf->SetBinContent(448,0.46);
    hrf->SetBinContent(449,0.38);
    hrf->SetBinContent(450,0.38);
    hrf->SetBinContent(451,0.5);
    hrf->SetBinContent(452,0.42);
    hrf->SetBinContent(453,0.46);
    hrf->SetBinContent(454,0.54);
    hrf->SetBinContent(455,0.5);
    hrf->SetBinContent(456,0.54);
    hrf->SetBinContent(457,0.46);
    hrf->SetBinContent(458,0.5);
    hrf->SetBinContent(459,0.46);
    hrf->SetBinContent(460,0.5);
    hrf->SetBinContent(461,0.42);
    hrf->SetBinContent(462,0.46);
    hrf->SetBinContent(463,0.5);
    hrf->SetBinContent(464,0.42);
    hrf->SetBinContent(465,0.42);
    hrf->SetBinContent(466,0.42);
    hrf->SetBinContent(467,0.46);
    hrf->SetBinContent(468,0.38);
    hrf->SetBinContent(469,0.5);
    hrf->SetBinContent(470,0.38);
    hrf->SetBinContent(471,0.46);
    hrf->SetBinContent(472,0.42);
    hrf->SetBinContent(473,0.46);
    hrf->SetBinContent(474,0.46);
    hrf->SetBinContent(475,0.34);
    hrf->SetBinContent(476,0.34);
    hrf->SetBinContent(477,0.5);
    hrf->SetBinContent(478,0.42);
    hrf->SetBinContent(479,0.5);
    hrf->SetBinContent(480,0.54);
    hrf->SetBinContent(481,0.62);
    hrf->SetBinContent(482,0.58);
    hrf->SetBinContent(483,0.66);
    hrf->SetBinContent(484,0.74);
    hrf->SetBinContent(487,0.82);
    hrf->SetBinContent(488,0.7);
    hrf->SetBinContent(489,0.66);
    hrf->SetBinContent(490,0.74);
    hrf->SetBinContent(491,0.7);
    hrf->SetBinContent(492,0.62);
    hrf->SetBinContent(493,0.62);
    hrf->SetBinContent(494,0.5);
    hrf->SetBinContent(495,0.46);
    hrf->SetBinContent(496,0.5);
    hrf->SetBinContent(497,0.5);
    hrf->SetBinContent(498,0.46);
    hrf->SetBinContent(499,0.5);
    hrf->SetBinContent(500,0.5);
    hrf->SetBinContent(501,0.42);
    hrf->SetBinContent(502,0.38);
    hrf->SetBinContent(503,0.42);
    hrf->SetBinContent(504,0.5);
    hrf->SetBinContent(505,0.5);
    hrf->SetBinContent(506,0.42);
    hrf->SetBinContent(507,0.5);
    hrf->SetBinContent(508,0.38);
    hrf->SetBinContent(509,0.54);
    hrf->SetBinContent(510,0.46);
    hrf->SetBinContent(511,0.54);
    hrf->SetBinContent(512,0.42);
    hrf->SetBinContent(513,0.5);
    hrf->SetBinContent(514,0.46);
    hrf->SetBinContent(515,0.42);
    hrf->SetBinContent(516,0.5);
    hrf->SetBinContent(517,0.46);
    hrf->SetBinContent(518,0.42);
    hrf->SetBinContent(519,0.42);
    hrf->SetBinContent(520,0.54);
    hrf->SetBinContent(521,0.42);
    hrf->SetBinContent(522,0.5);
    hrf->SetBinContent(523,0.46);
    hrf->SetBinContent(524,0.46);
    hrf->SetBinContent(525,0.38);
    hrf->SetBinContent(526,0.5);
    hrf->SetBinContent(527,0.5);
    hrf->SetBinContent(528,0.38);
    hrf->SetBinContent(529,0.46);
    hrf->SetBinContent(530,0.5);
    hrf->SetBinContent(531,0.42);
    hrf->SetBinContent(532,0.5);
    hrf->SetBinContent(533,0.58);
    hrf->SetBinContent(534,0.58);
    hrf->SetBinContent(535,0.66);
    hrf->SetBinContent(536,0.7);
    hrf->SetBinContent(537,0.66);
    hrf->SetBinContent(538,0.7);
    hrf->SetBinContent(541,0.74);
    hrf->SetBinContent(542,0.74);
    hrf->SetBinContent(543,0.74);
    hrf->SetBinContent(544,0.66);
    hrf->SetBinContent(545,0.74);
    hrf->SetBinContent(546,0.66);
    hrf->SetBinContent(547,0.58);
    hrf->SetBinContent(548,0.58);
    hrf->SetBinContent(549,0.54);
    hrf->SetBinContent(550,0.5);
    hrf->SetBinContent(551,0.54);
    hrf->SetBinContent(552,0.54);
    hrf->SetBinContent(553,0.46);
    hrf->SetBinContent(554,0.42);
    hrf->SetBinContent(555,0.46);
    hrf->SetBinContent(556,0.54);
    hrf->SetBinContent(557,0.5);
    hrf->SetBinContent(558,0.54);
    hrf->SetBinContent(559,0.46);
    hrf->SetBinContent(560,0.42);
    hrf->SetBinContent(561,0.5);
    hrf->SetBinContent(562,0.5);
    hrf->SetBinContent(563,0.62);
    hrf->SetBinContent(564,0.5);
    hrf->SetBinContent(565,0.62);
    hrf->SetBinContent(566,0.42);
    hrf->SetBinContent(567,0.46);
    hrf->SetBinContent(568,0.5);
    hrf->SetBinContent(569,0.5);
    hrf->SetBinContent(570,0.5);
    hrf->SetBinContent(571,0.5);
    hrf->SetBinContent(572,0.58);
    hrf->SetBinContent(573,0.42);
    hrf->SetBinContent(574,0.46);
    hrf->SetBinContent(575,0.58);
    hrf->SetBinContent(576,0.42);
    hrf->SetBinContent(577,0.46);
    hrf->SetBinContent(578,0.5);
    hrf->SetBinContent(579,0.5);
    hrf->SetBinContent(580,0.46);
    hrf->SetBinContent(581,0.42);
    hrf->SetBinContent(582,0.5);
    hrf->SetBinContent(583,0.46);
    hrf->SetBinContent(584,0.5);
    hrf->SetBinContent(585,0.58);
    hrf->SetBinContent(586,0.58);
    hrf->SetBinContent(587,0.62);
    hrf->SetBinContent(588,0.7);
    hrf->SetBinContent(589,0.66);
    hrf->SetBinContent(590,0.7);
    hrf->SetBinContent(591,0.7);
    hrf->SetBinContent(592,0.7);
    hrf->SetBinContent(595,0.74);
    hrf->SetBinContent(596,0.74);
    hrf->SetBinContent(597,0.7);
    hrf->SetBinContent(598,0.78);
    hrf->SetBinContent(599,0.7);
    hrf->SetBinContent(600,0.7);
    hrf->SetBinContent(601,0.66);
    hrf->SetBinContent(602,0.54);
    hrf->SetBinContent(603,0.7);
    hrf->SetBinContent(604,0.54);
    hrf->SetBinContent(605,0.58);
    hrf->SetBinContent(606,0.54);
    hrf->SetBinContent(607,0.46);
    hrf->SetBinContent(608,0.58);
    hrf->SetBinContent(609,0.58);
    hrf->SetBinContent(610,0.5);
    hrf->SetBinContent(611,0.5);
    hrf->SetBinContent(612,0.5);
    hrf->SetBinContent(613,0.58);
    hrf->SetBinContent(614,0.46);
    hrf->SetBinContent(615,0.54);
    hrf->SetBinContent(616,0.58);
    hrf->SetBinContent(617,0.5);
    hrf->SetBinContent(618,0.5);
    hrf->SetBinContent(619,0.5);
    hrf->SetBinContent(620,0.5);
    hrf->SetBinContent(621,0.5);
    hrf->SetBinContent(622,0.54);
    hrf->SetBinContent(623,0.46);
    hrf->SetBinContent(624,0.54);
    hrf->SetBinContent(625,0.54);
    hrf->SetBinContent(626,0.54);
    hrf->SetBinContent(627,0.54);
    hrf->SetBinContent(628,0.54);
    hrf->SetBinContent(629,0.5);
    hrf->SetBinContent(630,0.5);
    hrf->SetBinContent(631,0.46);
    hrf->SetBinContent(632,0.46);
    hrf->SetBinContent(633,0.46);
    hrf->SetBinContent(634,0.5);
    hrf->SetBinContent(635,0.46);
    hrf->SetBinContent(636,0.46);
    hrf->SetBinContent(637,0.5);
    hrf->SetBinContent(638,0.66);
    hrf->SetBinContent(639,0.62);
    hrf->SetBinContent(640,0.62);
    hrf->SetBinContent(641,0.66);
    hrf->SetBinContent(642,0.66);
    hrf->SetBinContent(643,0.58);
    hrf->SetBinContent(644,0.66);
    hrf->SetBinContent(645,0.66);
    hrf->SetBinContent(646,0.74);
    hrf->SetBinContent(649,0.7);
    hrf->SetBinContent(650,0.74);
    hrf->SetBinContent(651,0.74);
    hrf->SetBinContent(652,0.78);
    hrf->SetBinContent(653,0.7);
    hrf->SetBinContent(654,0.7);
    hrf->SetBinContent(655,0.62);
    hrf->SetBinContent(656,0.66);
    hrf->SetBinContent(657,0.74);
    hrf->SetBinContent(658,0.46);
    hrf->SetBinContent(659,0.54);
    hrf->SetBinContent(660,0.5);
    hrf->SetBinContent(661,0.54);
    hrf->SetBinContent(662,0.58);
    hrf->SetBinContent(663,0.42);
    hrf->SetBinContent(664,0.54);
    hrf->SetBinContent(665,0.58);
    hrf->SetBinContent(666,0.62);
    hrf->SetBinContent(667,0.58);
    hrf->SetBinContent(668,0.58);
    hrf->SetBinContent(669,0.58);
    hrf->SetBinContent(670,0.62);
    hrf->SetBinContent(671,0.46);
    hrf->SetBinContent(672,0.5);
    hrf->SetBinContent(673,0.62);
    hrf->SetBinContent(674,0.62);
    hrf->SetBinContent(675,0.66);
    hrf->SetBinContent(676,0.5);
    hrf->SetBinContent(677,0.66);
    hrf->SetBinContent(678,0.5);
    hrf->SetBinContent(679,0.5);
    hrf->SetBinContent(680,0.58);
    hrf->SetBinContent(681,0.54);
    hrf->SetBinContent(682,0.54);
    hrf->SetBinContent(683,0.58);
    hrf->SetBinContent(684,0.62);
    hrf->SetBinContent(685,0.5);
    hrf->SetBinContent(686,0.5);
    hrf->SetBinContent(687,0.54);
    hrf->SetBinContent(688,0.54);
    hrf->SetBinContent(689,0.66);
    hrf->SetBinContent(690,0.58);
    hrf->SetBinContent(691,0.54);
    hrf->SetBinContent(692,0.7);
    hrf->SetBinContent(693,0.7);
    hrf->SetBinContent(694,0.62);
    hrf->SetBinContent(695,0.62);
    hrf->SetBinContent(696,0.66);
    hrf->SetBinContent(697,0.74);
    hrf->SetBinContent(698,0.7);
    hrf->SetBinContent(699,0.74);
    hrf->SetBinContent(700,0.74);
    hrf->SetBinContent(703,0.7);
    hrf->SetBinContent(704,0.78);
    hrf->SetBinContent(705,0.66);
    hrf->SetBinContent(706,0.66);
    hrf->SetBinContent(707,0.66);
    hrf->SetBinContent(708,0.74);
    hrf->SetBinContent(709,0.7);
    hrf->SetBinContent(710,0.66);
    hrf->SetBinContent(711,0.58);
    hrf->SetBinContent(712,0.54);
    hrf->SetBinContent(713,0.58);
    hrf->SetBinContent(714,0.5);
    hrf->SetBinContent(715,0.54);
    hrf->SetBinContent(716,0.66);
    hrf->SetBinContent(717,0.54);
    hrf->SetBinContent(718,0.58);
    hrf->SetBinContent(719,0.66);
    hrf->SetBinContent(720,0.46);
    hrf->SetBinContent(721,0.58);
    hrf->SetBinContent(722,0.5);
    hrf->SetBinContent(723,0.58);
    hrf->SetBinContent(724,0.54);
    hrf->SetBinContent(725,0.58);
    hrf->SetBinContent(726,0.66);
    hrf->SetBinContent(727,0.58);
    hrf->SetBinContent(728,0.62);
    hrf->SetBinContent(729,0.58);
    hrf->SetBinContent(730,0.54);
    hrf->SetBinContent(731,0.54);
    hrf->SetBinContent(732,0.66);
    hrf->SetBinContent(733,0.5);
    hrf->SetBinContent(734,0.62);
    hrf->SetBinContent(735,0.62);
    hrf->SetBinContent(736,0.5);
    hrf->SetBinContent(737,0.54);
    hrf->SetBinContent(738,0.62);
    hrf->SetBinContent(739,0.62);
    hrf->SetBinContent(740,0.58);
    hrf->SetBinContent(741,0.58);
    hrf->SetBinContent(742,0.46);
    hrf->SetBinContent(743,0.46);
    hrf->SetBinContent(744,0.58);
    hrf->SetBinContent(745,0.5);
    hrf->SetBinContent(746,0.66);
    hrf->SetBinContent(747,0.66);
    hrf->SetBinContent(748,0.54);
    hrf->SetBinContent(749,0.78);
    hrf->SetBinContent(750,0.62);
    hrf->SetBinContent(751,0.7);
    hrf->SetBinContent(752,0.66);
    hrf->SetBinContent(753,0.66);
    hrf->SetBinContent(754,0.78);
    hrf->SetBinContent(757,0.82);
    hrf->SetBinContent(758,0.74);
    hrf->SetBinContent(759,0.7);
    hrf->SetBinContent(760,0.78);
    hrf->SetBinContent(761,0.7);
    hrf->SetBinContent(762,0.62);
    hrf->SetBinContent(763,0.78);
    hrf->SetBinContent(764,0.78);
    hrf->SetBinContent(765,0.7);
    hrf->SetBinContent(766,0.62);
    hrf->SetBinContent(767,0.58);
    hrf->SetBinContent(768,0.62);
    hrf->SetBinContent(769,0.66);
    hrf->SetBinContent(770,0.58);
    hrf->SetBinContent(771,0.66);
    hrf->SetBinContent(772,0.54);
    hrf->SetBinContent(773,0.7);
    hrf->SetBinContent(774,0.58);
    hrf->SetBinContent(775,0.58);
    hrf->SetBinContent(776,0.62);
    hrf->SetBinContent(777,0.7);
    hrf->SetBinContent(778,0.62);
    hrf->SetBinContent(779,0.66);
    hrf->SetBinContent(780,0.74);
    hrf->SetBinContent(781,0.54);
    hrf->SetBinContent(782,0.58);
    hrf->SetBinContent(783,0.54);
    hrf->SetBinContent(784,0.62);
    hrf->SetBinContent(785,0.62);
    hrf->SetBinContent(786,0.54);
    hrf->SetBinContent(787,0.54);
    hrf->SetBinContent(788,0.62);
    hrf->SetBinContent(789,0.5);
    hrf->SetBinContent(790,0.58);
    hrf->SetBinContent(791,0.74);
    hrf->SetBinContent(792,0.54);
    hrf->SetBinContent(793,0.58);
    hrf->SetBinContent(794,0.58);
    hrf->SetBinContent(795,0.54);
    hrf->SetBinContent(796,0.62);
    hrf->SetBinContent(797,0.58);
    hrf->SetBinContent(798,0.58);
    hrf->SetBinContent(799,0.7);
    hrf->SetBinContent(800,0.62);
    hrf->SetBinContent(801,0.7);
    hrf->SetBinContent(802,0.7);
    hrf->SetBinContent(803,0.82);
    hrf->SetBinContent(804,0.74);
    hrf->SetBinContent(805,0.74);
    hrf->SetBinContent(806,0.74);
    hrf->SetBinContent(807,0.74);
    hrf->SetBinContent(808,0.74);
    hrf->SetBinContent(811,0.78);
    hrf->SetBinContent(812,0.78);
    hrf->SetBinContent(813,0.86);
    hrf->SetBinContent(814,0.78);
    hrf->SetBinContent(815,0.78);
    hrf->SetBinContent(816,0.82);
    hrf->SetBinContent(817,0.82);
    hrf->SetBinContent(818,0.7);
    hrf->SetBinContent(819,0.7);
    hrf->SetBinContent(820,0.58);
    hrf->SetBinContent(821,0.58);
    hrf->SetBinContent(822,0.66);
    hrf->SetBinContent(823,0.62);
    hrf->SetBinContent(824,0.7);
    hrf->SetBinContent(825,0.58);
    hrf->SetBinContent(826,0.66);
    hrf->SetBinContent(827,0.54);
    hrf->SetBinContent(828,0.62);
    hrf->SetBinContent(829,0.62);
    hrf->SetBinContent(830,0.7);
    hrf->SetBinContent(831,0.78);
    hrf->SetBinContent(832,0.66);
    hrf->SetBinContent(833,0.62);
    hrf->SetBinContent(834,0.66);
    hrf->SetBinContent(835,0.66);
    hrf->SetBinContent(836,0.62);
    hrf->SetBinContent(837,0.66);
    hrf->SetBinContent(838,0.66);
    hrf->SetBinContent(839,0.66);
    hrf->SetBinContent(840,0.78);
    hrf->SetBinContent(841,0.66);
    hrf->SetBinContent(842,0.62);
    hrf->SetBinContent(843,0.66);
    hrf->SetBinContent(844,0.66);
    hrf->SetBinContent(845,0.7);
    hrf->SetBinContent(846,0.7);
    hrf->SetBinContent(847,0.58);
    hrf->SetBinContent(848,0.66);
    hrf->SetBinContent(849,0.78);
    hrf->SetBinContent(850,0.7);
    hrf->SetBinContent(851,0.62);
    hrf->SetBinContent(852,0.62);
    hrf->SetBinContent(853,0.7);
    hrf->SetBinContent(854,0.82);
    hrf->SetBinContent(855,0.66);
    hrf->SetBinContent(856,0.7);
    hrf->SetBinContent(857,0.74);
    hrf->SetBinContent(858,0.66);
    hrf->SetBinContent(859,0.74);
    hrf->SetBinContent(860,0.7);
    hrf->SetBinContent(861,0.78);
    hrf->SetBinContent(862,0.82);
    hrf->SetBinContent(865,0.78);
    hrf->SetBinContent(866,0.74);
    hrf->SetBinContent(867,0.78);
    hrf->SetBinContent(868,0.82);
    hrf->SetBinContent(869,0.78);
    hrf->SetBinContent(870,0.66);
    hrf->SetBinContent(871,0.74);
    hrf->SetBinContent(872,0.7);
    hrf->SetBinContent(873,0.86);
    hrf->SetBinContent(874,0.58);
    hrf->SetBinContent(875,0.62);
    hrf->SetBinContent(876,0.62);
    hrf->SetBinContent(877,0.74);
    hrf->SetBinContent(878,0.66);
    hrf->SetBinContent(879,0.66);
    hrf->SetBinContent(880,0.58);
    hrf->SetBinContent(881,0.66);
    hrf->SetBinContent(882,0.62);
    hrf->SetBinContent(883,0.7);
    hrf->SetBinContent(884,0.66);
    hrf->SetBinContent(885,0.74);
    hrf->SetBinContent(886,0.7);
    hrf->SetBinContent(887,0.62);
    hrf->SetBinContent(888,0.58);
    hrf->SetBinContent(889,0.66);
    hrf->SetBinContent(890,0.62);
    hrf->SetBinContent(891,0.66);
    hrf->SetBinContent(892,0.66);
    hrf->SetBinContent(893,0.74);
    hrf->SetBinContent(894,0.7);
    hrf->SetBinContent(895,0.58);
    hrf->SetBinContent(896,0.7);
    hrf->SetBinContent(897,0.66);
    hrf->SetBinContent(898,0.7);
    hrf->SetBinContent(899,0.66);
    hrf->SetBinContent(900,0.7);
    hrf->SetBinContent(901,0.62);
    hrf->SetBinContent(902,0.58);
    hrf->SetBinContent(903,0.7);
    hrf->SetBinContent(904,0.7);
    hrf->SetBinContent(905,0.7);
    hrf->SetBinContent(906,0.66);
    hrf->SetBinContent(907,0.7);
    hrf->SetBinContent(908,0.74);
    hrf->SetBinContent(909,0.7);
    hrf->SetBinContent(910,0.74);
    hrf->SetBinContent(911,0.74);
    hrf->SetBinContent(912,0.66);
    hrf->SetBinContent(913,0.82);
    hrf->SetBinContent(914,0.78);
    hrf->SetBinContent(915,0.78);
    hrf->SetBinContent(916,0.82);
    hrf->SetBinContent(919,0.86);
    hrf->SetBinContent(920,0.78);
    hrf->SetBinContent(921,0.78);
    hrf->SetBinContent(922,0.78);
    hrf->SetBinContent(923,0.86);
    hrf->SetBinContent(924,0.78);
    hrf->SetBinContent(925,0.7);
    hrf->SetBinContent(926,0.78);
    hrf->SetBinContent(927,0.74);
    hrf->SetBinContent(928,0.62);
    hrf->SetBinContent(929,0.62);
    hrf->SetBinContent(930,0.7);
    hrf->SetBinContent(931,0.78);
    hrf->SetBinContent(932,0.66);
    hrf->SetBinContent(933,0.7);
    hrf->SetBinContent(934,0.7);
    hrf->SetBinContent(935,0.66);
    hrf->SetBinContent(936,0.62);
    hrf->SetBinContent(937,0.7);
    hrf->SetBinContent(938,0.7);
    hrf->SetBinContent(939,0.62);
    hrf->SetBinContent(940,0.74);
    hrf->SetBinContent(941,0.66);
    hrf->SetBinContent(942,0.62);
    hrf->SetBinContent(943,0.66);
    hrf->SetBinContent(944,0.74);
    hrf->SetBinContent(945,0.66);
    hrf->SetBinContent(946,0.66);
    hrf->SetBinContent(947,0.74);
    hrf->SetBinContent(948,0.66);
    hrf->SetBinContent(949,0.66);
    hrf->SetBinContent(950,0.62);
    hrf->SetBinContent(951,0.74);
    hrf->SetBinContent(952,0.74);
    hrf->SetBinContent(953,0.66);
    hrf->SetBinContent(954,0.66);
    hrf->SetBinContent(955,0.7);
    hrf->SetBinContent(956,0.58);
    hrf->SetBinContent(957,0.7);
    hrf->SetBinContent(958,0.66);
    hrf->SetBinContent(959,0.7);
    hrf->SetBinContent(960,0.62);
    hrf->SetBinContent(961,0.7);
    hrf->SetBinContent(962,0.82);
    hrf->SetBinContent(963,0.66);
    hrf->SetBinContent(964,0.74);
    hrf->SetBinContent(965,0.7);
    hrf->SetBinContent(966,0.74);
    hrf->SetBinContent(967,0.78);
    hrf->SetBinContent(968,0.86);
    hrf->SetBinContent(969,0.78);
    hrf->SetBinContent(970,0.78);
    hrf->SetBinContent(973,0.82);
    hrf->SetBinContent(974,0.82);
    hrf->SetBinContent(975,0.74);
    hrf->SetBinContent(976,0.7);
    hrf->SetBinContent(977,0.78);
    hrf->SetBinContent(978,0.74);
    hrf->SetBinContent(979,0.82);
    hrf->SetBinContent(980,0.7);
    hrf->SetBinContent(981,0.74);
    hrf->SetBinContent(982,0.62);
    hrf->SetBinContent(983,0.7);
    hrf->SetBinContent(984,0.7);
    hrf->SetBinContent(985,0.66);
    hrf->SetBinContent(986,0.78);
    hrf->SetBinContent(987,0.62);
    hrf->SetBinContent(988,0.74);
    hrf->SetBinContent(989,0.7);
    hrf->SetBinContent(990,0.7);
    hrf->SetBinContent(991,0.7);
    hrf->SetBinContent(992,0.7);
    hrf->SetBinContent(993,0.82);
    hrf->SetBinContent(994,0.86);
    hrf->SetBinContent(995,0.7);
    hrf->SetBinContent(996,0.7);
    hrf->SetBinContent(997,0.78);
    hrf->SetBinContent(998,0.66);
    hrf->SetBinContent(999,0.74);
    hrf->SetBinContent(1000,0.86);
    hrf->SetBinContent(1001,0.66);
    hrf->SetBinContent(1002,0.66);
    hrf->SetBinContent(1003,0.74);
    hrf->SetBinContent(1004,0.7);
    hrf->SetBinContent(1005,0.62);
    hrf->SetBinContent(1006,0.66);
    hrf->SetBinContent(1007,0.78);
    hrf->SetBinContent(1008,0.7);
    hrf->SetBinContent(1009,0.74);
    hrf->SetBinContent(1010,0.66);
    hrf->SetBinContent(1011,0.78);
    hrf->SetBinContent(1012,0.62);
    hrf->SetBinContent(1013,0.7);
    hrf->SetBinContent(1014,0.58);
    hrf->SetBinContent(1015,0.74);
    hrf->SetBinContent(1016,0.74);
    hrf->SetBinContent(1017,0.7);
    hrf->SetBinContent(1018,0.7);
    hrf->SetBinContent(1019,0.74);
    hrf->SetBinContent(1020,0.78);
    hrf->SetBinContent(1021,0.74);
    hrf->SetBinContent(1022,0.86);
    hrf->SetBinContent(1023,0.86);
    hrf->SetBinContent(1024,0.82);
    hrf->SetBinContent(1027,0.82);
    hrf->SetBinContent(1028,0.82);
    hrf->SetBinContent(1029,0.78);
    hrf->SetBinContent(1030,0.78);
    hrf->SetBinContent(1031,0.78);
    hrf->SetBinContent(1032,0.78);
    hrf->SetBinContent(1033,0.78);
    hrf->SetBinContent(1034,0.78);
    hrf->SetBinContent(1035,0.9);
    hrf->SetBinContent(1036,0.74);
    hrf->SetBinContent(1037,0.62);
    hrf->SetBinContent(1038,0.7);
    hrf->SetBinContent(1039,0.66);
    hrf->SetBinContent(1040,0.7);
    hrf->SetBinContent(1041,0.74);
    hrf->SetBinContent(1042,0.74);
    hrf->SetBinContent(1043,0.74);
    hrf->SetBinContent(1044,0.7);
    hrf->SetBinContent(1045,0.62);
    hrf->SetBinContent(1046,0.74);
    hrf->SetBinContent(1047,0.78);
    hrf->SetBinContent(1048,0.74);
    hrf->SetBinContent(1049,0.78);
    hrf->SetBinContent(1050,0.78);
    hrf->SetBinContent(1051,0.78);
    hrf->SetBinContent(1052,0.74);
    hrf->SetBinContent(1053,0.74);
    hrf->SetBinContent(1054,0.78);
    hrf->SetBinContent(1055,0.74);
    hrf->SetBinContent(1056,0.78);
    hrf->SetBinContent(1057,0.74);
    hrf->SetBinContent(1058,0.74);
    hrf->SetBinContent(1059,0.74);
    hrf->SetBinContent(1060,0.74);
    hrf->SetBinContent(1061,0.7);
    hrf->SetBinContent(1062,0.7);
    hrf->SetBinContent(1063,0.66);
    hrf->SetBinContent(1064,0.74);
    hrf->SetBinContent(1065,0.74);
    hrf->SetBinContent(1066,0.7);
    hrf->SetBinContent(1067,0.7);
    hrf->SetBinContent(1068,0.62);
    hrf->SetBinContent(1069,0.74);
    hrf->SetBinContent(1070,0.7);
    hrf->SetBinContent(1071,0.74);
    hrf->SetBinContent(1072,0.74);
    hrf->SetBinContent(1073,0.82);
    hrf->SetBinContent(1074,0.82);
    hrf->SetBinContent(1075,0.86);
    hrf->SetBinContent(1076,0.78);
    hrf->SetBinContent(1077,0.82);
    hrf->SetBinContent(1078,0.86);
    hrf->SetBinContent(1081,0.86);
    hrf->SetBinContent(1082,0.82);
    hrf->SetBinContent(1083,0.78);
    hrf->SetBinContent(1084,0.78);
    hrf->SetBinContent(1085,0.82);
    hrf->SetBinContent(1086,0.82);
    hrf->SetBinContent(1087,0.82);
    hrf->SetBinContent(1088,0.78);
    hrf->SetBinContent(1089,0.86);
    hrf->SetBinContent(1090,0.74);
    hrf->SetBinContent(1091,0.78);
    hrf->SetBinContent(1092,0.74);
    hrf->SetBinContent(1093,0.7);
    hrf->SetBinContent(1094,0.66);
    hrf->SetBinContent(1095,0.78);
    hrf->SetBinContent(1096,0.74);
    hrf->SetBinContent(1097,0.78);
    hrf->SetBinContent(1098,0.78);
    hrf->SetBinContent(1099,0.78);
    hrf->SetBinContent(1100,0.74);
    hrf->SetBinContent(1101,0.7);
    hrf->SetBinContent(1102,0.82);
    hrf->SetBinContent(1103,0.78);
    hrf->SetBinContent(1104,0.86);
    hrf->SetBinContent(1105,0.78);
    hrf->SetBinContent(1106,0.74);
    hrf->SetBinContent(1107,0.78);
    hrf->SetBinContent(1108,0.74);
    hrf->SetBinContent(1109,0.82);
    hrf->SetBinContent(1110,0.74);
    hrf->SetBinContent(1111,0.82);
    hrf->SetBinContent(1112,0.74);
    hrf->SetBinContent(1113,0.86);
    hrf->SetBinContent(1114,0.82);
    hrf->SetBinContent(1115,0.74);
    hrf->SetBinContent(1116,0.78);
    hrf->SetBinContent(1117,0.74);
    hrf->SetBinContent(1118,0.7);
    hrf->SetBinContent(1119,0.66);
    hrf->SetBinContent(1120,0.66);
    hrf->SetBinContent(1121,0.74);
    hrf->SetBinContent(1122,0.66);
    hrf->SetBinContent(1123,0.7);
    hrf->SetBinContent(1124,0.82);
    hrf->SetBinContent(1125,0.86);
    hrf->SetBinContent(1126,0.74);
    hrf->SetBinContent(1127,0.78);
    hrf->SetBinContent(1128,0.82);
    hrf->SetBinContent(1129,0.78);
    hrf->SetBinContent(1130,0.82);
    hrf->SetBinContent(1131,0.86);
    hrf->SetBinContent(1132,0.82);
    hrf->SetBinContent(1135,0.86);
    hrf->SetBinContent(1136,0.86);
    hrf->SetBinContent(1137,0.9);
    hrf->SetBinContent(1138,0.78);
    hrf->SetBinContent(1139,0.9);
    hrf->SetBinContent(1140,0.86);
    hrf->SetBinContent(1141,0.82);
    hrf->SetBinContent(1142,0.78);
    hrf->SetBinContent(1143,0.86);
    hrf->SetBinContent(1144,0.7);
    hrf->SetBinContent(1145,0.7);
    hrf->SetBinContent(1146,0.74);
    hrf->SetBinContent(1147,0.62);
    hrf->SetBinContent(1148,0.74);
    hrf->SetBinContent(1149,0.74);
    hrf->SetBinContent(1150,0.78);
    hrf->SetBinContent(1151,0.74);
    hrf->SetBinContent(1152,0.74);
    hrf->SetBinContent(1153,0.78);
    hrf->SetBinContent(1154,0.78);
    hrf->SetBinContent(1155,0.78);
    hrf->SetBinContent(1156,0.82);
    hrf->SetBinContent(1157,0.74);
    hrf->SetBinContent(1158,0.78);
    hrf->SetBinContent(1159,0.78);
    hrf->SetBinContent(1160,0.74);
    hrf->SetBinContent(1161,0.78);
    hrf->SetBinContent(1162,0.82);
    hrf->SetBinContent(1163,0.78);
    hrf->SetBinContent(1164,0.78);
    hrf->SetBinContent(1165,0.82);
    hrf->SetBinContent(1166,0.7);
    hrf->SetBinContent(1167,0.78);
    hrf->SetBinContent(1168,0.7);
    hrf->SetBinContent(1169,0.82);
    hrf->SetBinContent(1170,0.74);
    hrf->SetBinContent(1171,0.74);
    hrf->SetBinContent(1172,0.78);
    hrf->SetBinContent(1173,0.74);
    hrf->SetBinContent(1174,0.66);
    hrf->SetBinContent(1175,0.7);
    hrf->SetBinContent(1176,0.7);
    hrf->SetBinContent(1177,0.74);
    hrf->SetBinContent(1178,0.86);
    hrf->SetBinContent(1179,0.86);
    hrf->SetBinContent(1180,0.78);
    hrf->SetBinContent(1181,0.78);
    hrf->SetBinContent(1182,0.82);
    hrf->SetBinContent(1183,0.82);
    hrf->SetBinContent(1184,0.86);
    hrf->SetBinContent(1185,0.86);
    hrf->SetBinContent(1186,0.82);
    hrf->SetBinContent(1189,0.86);
    hrf->SetBinContent(1190,0.82);
    hrf->SetBinContent(1191,0.86);
    hrf->SetBinContent(1192,0.82);
    hrf->SetBinContent(1193,0.86);
    hrf->SetBinContent(1194,0.86);
    hrf->SetBinContent(1195,0.78);
    hrf->SetBinContent(1196,0.86);
    hrf->SetBinContent(1197,0.82);
    hrf->SetBinContent(1198,0.74);
    hrf->SetBinContent(1199,0.74);
    hrf->SetBinContent(1200,0.74);
    hrf->SetBinContent(1201,0.74);
    hrf->SetBinContent(1202,0.7);
    hrf->SetBinContent(1203,0.7);
    hrf->SetBinContent(1204,0.82);
    hrf->SetBinContent(1205,0.74);
    hrf->SetBinContent(1206,0.82);
    hrf->SetBinContent(1207,0.82);
    hrf->SetBinContent(1208,0.78);
    hrf->SetBinContent(1209,0.82);
    hrf->SetBinContent(1210,0.82);
    hrf->SetBinContent(1211,0.82);
    hrf->SetBinContent(1212,0.78);
    hrf->SetBinContent(1213,0.82);
    hrf->SetBinContent(1214,0.78);
    hrf->SetBinContent(1215,0.82);
    hrf->SetBinContent(1216,0.86);
    hrf->SetBinContent(1217,0.74);
    hrf->SetBinContent(1218,0.82);
    hrf->SetBinContent(1219,0.82);
    hrf->SetBinContent(1220,0.78);
    hrf->SetBinContent(1221,0.78);
    hrf->SetBinContent(1222,0.7);
    hrf->SetBinContent(1223,0.74);
    hrf->SetBinContent(1224,0.82);
    hrf->SetBinContent(1225,0.7);
    hrf->SetBinContent(1226,0.78);
    hrf->SetBinContent(1227,0.74);
    hrf->SetBinContent(1228,0.7);
    hrf->SetBinContent(1229,0.7);
    hrf->SetBinContent(1230,0.66);
    hrf->SetBinContent(1231,0.74);
    hrf->SetBinContent(1232,0.78);
    hrf->SetBinContent(1233,0.82);
    hrf->SetBinContent(1234,0.82);
    hrf->SetBinContent(1235,0.78);
    hrf->SetBinContent(1236,0.82);
    hrf->SetBinContent(1237,0.82);
    hrf->SetBinContent(1238,0.82);
    hrf->SetBinContent(1239,0.82);
    hrf->SetBinContent(1240,0.86);
    hrf->SetBinContent(1243,0.86);
    hrf->SetBinContent(1244,0.86);
    hrf->SetBinContent(1245,0.82);
    hrf->SetBinContent(1246,0.86);
    hrf->SetBinContent(1247,0.9);
    hrf->SetBinContent(1248,0.82);
    hrf->SetBinContent(1249,0.86);
    hrf->SetBinContent(1250,0.82);
    hrf->SetBinContent(1251,0.82);
    hrf->SetBinContent(1252,0.78);
    hrf->SetBinContent(1253,0.78);
    hrf->SetBinContent(1254,0.66);
    hrf->SetBinContent(1255,0.7);
    hrf->SetBinContent(1256,0.74);
    hrf->SetBinContent(1257,0.78);
    hrf->SetBinContent(1258,0.78);
    hrf->SetBinContent(1259,0.82);
    hrf->SetBinContent(1260,0.86);
    hrf->SetBinContent(1261,0.82);
    hrf->SetBinContent(1262,0.82);
    hrf->SetBinContent(1263,0.82);
    hrf->SetBinContent(1264,0.78);
    hrf->SetBinContent(1265,0.9);
    hrf->SetBinContent(1266,0.78);
    hrf->SetBinContent(1267,0.82);
    hrf->SetBinContent(1268,0.78);
    hrf->SetBinContent(1269,0.78);
    hrf->SetBinContent(1270,0.78);
    hrf->SetBinContent(1271,0.78);
    hrf->SetBinContent(1272,0.78);
    hrf->SetBinContent(1273,0.86);
    hrf->SetBinContent(1274,0.78);
    hrf->SetBinContent(1275,0.78);
    hrf->SetBinContent(1276,0.74);
    hrf->SetBinContent(1277,0.78);
    hrf->SetBinContent(1278,0.7);
    hrf->SetBinContent(1279,0.7);
    hrf->SetBinContent(1280,0.74);
    hrf->SetBinContent(1281,0.74);
    hrf->SetBinContent(1282,0.74);
    hrf->SetBinContent(1283,0.78);
    hrf->SetBinContent(1284,0.82);
    hrf->SetBinContent(1285,0.74);
    hrf->SetBinContent(1286,0.86);
    hrf->SetBinContent(1287,0.78);
    hrf->SetBinContent(1288,0.86);
    hrf->SetBinContent(1289,0.78);
    hrf->SetBinContent(1290,0.82);
    hrf->SetBinContent(1291,0.86);
    hrf->SetBinContent(1292,0.82);
    hrf->SetBinContent(1293,0.86);
    hrf->SetBinContent(1294,0.82);
    hrf->SetBinContent(1297,0.86);
    hrf->SetBinContent(1298,0.86);
    hrf->SetBinContent(1299,0.82);
    hrf->SetBinContent(1300,0.82);
    hrf->SetBinContent(1301,0.86);
    hrf->SetBinContent(1302,0.82);
    hrf->SetBinContent(1303,0.82);
    hrf->SetBinContent(1304,0.86);
    hrf->SetBinContent(1305,0.86);
    hrf->SetBinContent(1306,0.78);
    hrf->SetBinContent(1307,0.74);
    hrf->SetBinContent(1308,0.7);
    hrf->SetBinContent(1309,0.66);
    hrf->SetBinContent(1310,0.74);
    hrf->SetBinContent(1311,0.74);
    hrf->SetBinContent(1312,0.78);
    hrf->SetBinContent(1313,0.78);
    hrf->SetBinContent(1314,0.78);
    hrf->SetBinContent(1315,0.82);
    hrf->SetBinContent(1316,0.78);
    hrf->SetBinContent(1317,0.82);
    hrf->SetBinContent(1318,0.78);
    hrf->SetBinContent(1319,0.78);
    hrf->SetBinContent(1320,0.78);
    hrf->SetBinContent(1321,0.82);
    hrf->SetBinContent(1322,0.86);
    hrf->SetBinContent(1323,0.82);
    hrf->SetBinContent(1324,0.82);
    hrf->SetBinContent(1325,0.82);
    hrf->SetBinContent(1326,0.74);
    hrf->SetBinContent(1327,0.78);
    hrf->SetBinContent(1328,0.82);
    hrf->SetBinContent(1329,0.78);
    hrf->SetBinContent(1330,0.74);
    hrf->SetBinContent(1331,0.82);
    hrf->SetBinContent(1332,0.78);
    hrf->SetBinContent(1333,0.78);
    hrf->SetBinContent(1334,0.78);
    hrf->SetBinContent(1335,0.74);
    hrf->SetBinContent(1336,0.78);
    hrf->SetBinContent(1337,0.78);
    hrf->SetBinContent(1338,0.7);
    hrf->SetBinContent(1339,0.7);
    hrf->SetBinContent(1340,0.86);
    hrf->SetBinContent(1341,0.82);
    hrf->SetBinContent(1342,0.82);
    hrf->SetBinContent(1343,0.82);
    hrf->SetBinContent(1344,0.86);
    hrf->SetBinContent(1345,0.86);
    hrf->SetBinContent(1346,0.86);
    hrf->SetBinContent(1347,0.9);
    hrf->SetBinContent(1348,0.86);
    hrf->SetBinContent(1351,0.86);
    hrf->SetBinContent(1352,0.82);
    hrf->SetBinContent(1353,0.9);
    hrf->SetBinContent(1354,0.86);
    hrf->SetBinContent(1355,0.86);
    hrf->SetBinContent(1356,0.9);
    hrf->SetBinContent(1357,0.82);
    hrf->SetBinContent(1358,0.86);
    hrf->SetBinContent(1359,0.9);
    hrf->SetBinContent(1360,0.74);
    hrf->SetBinContent(1361,0.78);
    hrf->SetBinContent(1362,0.78);
    hrf->SetBinContent(1363,0.74);
    hrf->SetBinContent(1364,0.78);
    hrf->SetBinContent(1365,0.74);
    hrf->SetBinContent(1366,0.82);
    hrf->SetBinContent(1367,0.82);
    hrf->SetBinContent(1368,0.86);
    hrf->SetBinContent(1369,0.82);
    hrf->SetBinContent(1370,0.82);
    hrf->SetBinContent(1371,0.78);
    hrf->SetBinContent(1372,0.82);
    hrf->SetBinContent(1373,0.78);
    hrf->SetBinContent(1374,0.78);
    hrf->SetBinContent(1375,0.82);
    hrf->SetBinContent(1376,0.78);
    hrf->SetBinContent(1377,0.78);
    hrf->SetBinContent(1378,0.82);
    hrf->SetBinContent(1379,0.78);
    hrf->SetBinContent(1380,0.82);
    hrf->SetBinContent(1381,0.86);
    hrf->SetBinContent(1382,0.82);
    hrf->SetBinContent(1383,0.82);
    hrf->SetBinContent(1384,0.78);
    hrf->SetBinContent(1385,0.82);
    hrf->SetBinContent(1386,0.82);
    hrf->SetBinContent(1387,0.86);
    hrf->SetBinContent(1388,0.74);
    hrf->SetBinContent(1389,0.82);
    hrf->SetBinContent(1390,0.7);
    hrf->SetBinContent(1391,0.7);
    hrf->SetBinContent(1392,0.78);
    hrf->SetBinContent(1393,0.78);
    hrf->SetBinContent(1394,0.9);
    hrf->SetBinContent(1395,0.86);
    hrf->SetBinContent(1396,0.86);
    hrf->SetBinContent(1397,0.9);
    hrf->SetBinContent(1398,0.86);
    hrf->SetBinContent(1399,0.9);
    hrf->SetBinContent(1400,0.82);
    hrf->SetBinContent(1401,0.9);
    hrf->SetBinContent(1402,0.9);
    hrf->SetBinContent(1405,0.9);
    hrf->SetBinContent(1406,0.86);
    hrf->SetBinContent(1407,0.86);
    hrf->SetBinContent(1408,0.94);
    hrf->SetBinContent(1409,0.86);
    hrf->SetBinContent(1410,0.86);
    hrf->SetBinContent(1411,0.86);
    hrf->SetBinContent(1412,0.86);
    hrf->SetBinContent(1413,0.86);
    hrf->SetBinContent(1414,0.82);
    hrf->SetBinContent(1415,0.74);
    hrf->SetBinContent(1416,0.7);
    hrf->SetBinContent(1417,0.78);
    hrf->SetBinContent(1418,0.74);
    hrf->SetBinContent(1419,0.78);
    hrf->SetBinContent(1420,0.78);
    hrf->SetBinContent(1421,0.78);
    hrf->SetBinContent(1422,0.86);
    hrf->SetBinContent(1423,0.78);
    hrf->SetBinContent(1424,0.82);
    hrf->SetBinContent(1425,0.82);
    hrf->SetBinContent(1426,0.82);
    hrf->SetBinContent(1427,0.78);
    hrf->SetBinContent(1428,0.78);
    hrf->SetBinContent(1429,0.86);
    hrf->SetBinContent(1430,0.82);
    hrf->SetBinContent(1431,0.86);
    hrf->SetBinContent(1432,0.86);
    hrf->SetBinContent(1433,0.82);
    hrf->SetBinContent(1434,0.9);
    hrf->SetBinContent(1435,0.82);
    hrf->SetBinContent(1436,0.86);
    hrf->SetBinContent(1437,0.82);
    hrf->SetBinContent(1438,0.9);
    hrf->SetBinContent(1439,0.82);
    hrf->SetBinContent(1440,0.78);
    hrf->SetBinContent(1441,0.78);
    hrf->SetBinContent(1442,0.82);
    hrf->SetBinContent(1443,0.74);
    hrf->SetBinContent(1444,0.82);
    hrf->SetBinContent(1445,0.78);
    hrf->SetBinContent(1446,0.82);
    hrf->SetBinContent(1447,0.78);
    hrf->SetBinContent(1448,0.9);
    hrf->SetBinContent(1449,0.82);
    hrf->SetBinContent(1450,0.86);
    hrf->SetBinContent(1451,0.86);
    hrf->SetBinContent(1452,0.86);
    hrf->SetBinContent(1453,0.86);
    hrf->SetBinContent(1454,0.86);
    hrf->SetBinContent(1455,0.86);
    hrf->SetBinContent(1456,0.86);
    hrf->SetBinContent(1459,0.9);
    hrf->SetBinContent(1460,0.86);
    hrf->SetBinContent(1461,0.9);
    hrf->SetBinContent(1462,0.9);
    hrf->SetBinContent(1463,0.9);
    hrf->SetBinContent(1464,0.9);
    hrf->SetBinContent(1465,0.86);
    hrf->SetBinContent(1466,0.86);
    hrf->SetBinContent(1467,0.9);
    hrf->SetBinContent(1468,0.78);
    hrf->SetBinContent(1469,0.82);
    hrf->SetBinContent(1470,0.7);
    hrf->SetBinContent(1471,0.78);
    hrf->SetBinContent(1472,0.78);
    hrf->SetBinContent(1473,0.78);
    hrf->SetBinContent(1474,0.82);
    hrf->SetBinContent(1475,0.82);
    hrf->SetBinContent(1476,0.86);
    hrf->SetBinContent(1477,0.82);
    hrf->SetBinContent(1478,0.86);
    hrf->SetBinContent(1479,0.82);
    hrf->SetBinContent(1480,0.82);
    hrf->SetBinContent(1481,0.9);
    hrf->SetBinContent(1482,0.9);
    hrf->SetBinContent(1483,0.82);
    hrf->SetBinContent(1484,0.86);
    hrf->SetBinContent(1485,0.9);
    hrf->SetBinContent(1486,0.86);
    hrf->SetBinContent(1487,0.78);
    hrf->SetBinContent(1488,0.9);
    hrf->SetBinContent(1489,0.86);
    hrf->SetBinContent(1490,0.86);
    hrf->SetBinContent(1491,0.82);
    hrf->SetBinContent(1492,0.78);
    hrf->SetBinContent(1493,0.86);
    hrf->SetBinContent(1494,0.82);
    hrf->SetBinContent(1495,0.78);
    hrf->SetBinContent(1496,0.82);
    hrf->SetBinContent(1497,0.78);
    hrf->SetBinContent(1498,0.74);
    hrf->SetBinContent(1499,0.74);
    hrf->SetBinContent(1500,0.78);
    hrf->SetBinContent(1501,0.78);
    hrf->SetBinContent(1502,0.86);
    hrf->SetBinContent(1503,0.86);
    hrf->SetBinContent(1504,0.82);
    hrf->SetBinContent(1505,0.86);
    hrf->SetBinContent(1506,0.86);
    hrf->SetBinContent(1507,0.86);
    hrf->SetBinContent(1508,0.86);
    hrf->SetBinContent(1509,0.9);
    hrf->SetBinContent(1510,0.9);
    hrf->SetBinContent(1513,0.86);
    hrf->SetBinContent(1514,0.9);
    hrf->SetBinContent(1515,0.9);
    hrf->SetBinContent(1516,0.9);
    hrf->SetBinContent(1517,0.9);
    hrf->SetBinContent(1518,0.86);
    hrf->SetBinContent(1519,0.86);
    hrf->SetBinContent(1520,0.9);
    hrf->SetBinContent(1521,0.9);
    hrf->SetBinContent(1522,0.78);
    hrf->SetBinContent(1523,0.78);
    hrf->SetBinContent(1524,0.78);
    hrf->SetBinContent(1525,0.74);
    hrf->SetBinContent(1526,0.78);
    hrf->SetBinContent(1527,0.82);
    hrf->SetBinContent(1528,0.78);
    hrf->SetBinContent(1529,0.82);
    hrf->SetBinContent(1530,0.86);
    hrf->SetBinContent(1531,0.82);
    hrf->SetBinContent(1532,0.86);
    hrf->SetBinContent(1533,0.86);
    hrf->SetBinContent(1534,0.78);
    hrf->SetBinContent(1535,0.86);
    hrf->SetBinContent(1536,0.82);
    hrf->SetBinContent(1537,0.82);
    hrf->SetBinContent(1538,0.82);
    hrf->SetBinContent(1539,0.9);
    hrf->SetBinContent(1540,0.82);
    hrf->SetBinContent(1541,0.82);
    hrf->SetBinContent(1542,0.82);
    hrf->SetBinContent(1543,0.9);
    hrf->SetBinContent(1544,0.9);
    hrf->SetBinContent(1545,0.82);
    hrf->SetBinContent(1546,0.82);
    hrf->SetBinContent(1547,0.82);
    hrf->SetBinContent(1548,0.82);
    hrf->SetBinContent(1549,0.82);
    hrf->SetBinContent(1550,0.82);
    hrf->SetBinContent(1551,0.82);
    hrf->SetBinContent(1552,0.78);
    hrf->SetBinContent(1553,0.82);
    hrf->SetBinContent(1554,0.82);
    hrf->SetBinContent(1555,0.82);
    hrf->SetBinContent(1556,0.86);
    hrf->SetBinContent(1557,0.86);
    hrf->SetBinContent(1558,0.86);
    hrf->SetBinContent(1559,0.9);
    hrf->SetBinContent(1560,0.86);
    hrf->SetBinContent(1561,0.86);
    hrf->SetBinContent(1562,0.86);
    hrf->SetBinContent(1563,0.86);
    hrf->SetBinContent(1564,0.82);
 
    return hrf;
  }
}

