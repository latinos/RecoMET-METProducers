import FWCore.ParameterSet.Config as cms

muonTCMETValueMapProducer = cms.EDProducer("MuonTCMETValueMapProducer",
     muonInputTag        = cms.InputTag("muons"),
     beamSpotInputTag    = cms.InputTag("offlineBeamSpot"),
     vertexInputTag      = cms.InputTag("offlinePrimaryVertices"),
     rf_type             = cms.int32(1),                                           
     trackAlgo_max       = cms.int32(8),
     d0_max              = cms.double(0.3),                                           
     pt_min              = cms.double(1.),
     pt_max              = cms.double(100.),
     eta_max             = cms.double(2.65),
     chi2_max            = cms.double(5),
     nhits_min           = cms.double(6),
     ptErr_max           = cms.double(0.2),
     track_quality       = cms.vint32(2),
     track_algos         = cms.vint32(),
     chi2_max_tight      = cms.double(5.),
     nhits_min_tight      = cms.double(9),
     ptErr_max_tight     = cms.double(0.2),
     usePvtxd0           = cms.bool(False),
     d0cuta              = cms.double(0.015),
     d0cutb              = cms.double(0.5),                                           
     d0_muon             = cms.double(0.2),
     pt_muon             = cms.double(10),
     eta_muon            = cms.double(2.4),
     chi2_muon           = cms.double(10),
     nhits_muon          = cms.double(11),
     global_muon         = cms.bool(True),
     tracker_muon        = cms.bool(True),
     deltaR_muon         = cms.double(0.05),
     useCaloMuons        = cms.bool(False),
     muonMinValidStaHits = cms.int32(1)
)
