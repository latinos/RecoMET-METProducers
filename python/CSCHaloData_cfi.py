import FWCore.ParameterSet.Config as cms
# File: CSCHaloData_cfi.py
# Original Author: R. Remington, The University of Florida
# Description: Module to build CSCHaloData and put into the event
# Date: Oct. 15, 2009

CSCHaloData = cms.EDProducer("CSCHaloDataProducer",
                          
                             # Digi Level
                             L1MuGMTReadoutLabel = cms.InputTag("gtDigis"),

                             # HLT
                             HLTResultLabel = cms.InputTag("TriggerResults::HLT"),
                             HLTBitLabel = cms.VInputTag(
                                    cms.InputTag("HLT_CSCBeamHalo"),
                                    cms.InputTag("HLT_CSCBeamHaloOverlapRing1"),
                                    cms.InputTag("HLT_CSCBeamHaloOverlapRing2"),
                                    cms.InputTag("HLT_CSCBeamHaloRing2or3")
                                    ),
    
                             # RecHit Level
                             CSCRecHitLabel = cms.InputTag("csc2DRecHits"),
                             
                             # Higher Level Reco
                             CSCSegmentLabel= cms.InputTag("cscSegments"),
                             CosmicMuonLabel= cms.InputTag("cosmicMuons"),
                             MuonLabel = cms.InputTag("muons"),
                             SALabel  =  cms.InputTag("standAloneMuons"),

                             DetaParam = cms.double(0.0),
                             DphiParam = cms.double(999.),
                             NormChi2Param = cms.double(999.),
                             InnerRMinParam = cms.double(0.),
                             OuterRMinParam = cms.double(0.),
                             InnerRMaxParam = cms.double(9999.),
                             OuterRMaxParam = cms.double(9999.)
                             
                             )



