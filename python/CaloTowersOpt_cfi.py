import FWCore.ParameterSet.Config as cms

# File: CaloTowerOpt.cfi
# Author: B. Scurlock
# Date: 03.06.2008
#
# Form uncorrected Missing ET from Calorimeter Towers and store into event as a CaloMET
# product
# Creates new calotowers with optimized Energy thresholds for MET. 
calotoweroptmaker = cms.EDFilter("CaloTowersCreator",
    # Depth, fraction of the respective calorimeter [0,1]
    MomEmDepth = cms.double(0.0),
    # Energy threshold for EB 5x5 crystal inclusion [GeV]
    EBSumThreshold = cms.double(0.2),
    # Weighting factor for HF short-fiber readouts
    HF2Weight = cms.double(1.0),
    # Weighting factor for EB   
    EBWeight = cms.double(1.0),
    # Label of HFRecHitCollection to use
    hfInput = cms.InputTag("hfreco"),
    # Energy threshold for EE crystals-in-tower inclusion [GeV]
    EESumThreshold = cms.double(0.45),
    # Energy threshold for HO cell inclusion [GeV]
    HOThreshold = cms.double(0.5),
    HBGrid = cms.untracked.vdouble(-1.0, 1.0, 10.0, 100.0, 1000.0),
    # Energy threshold for HB cell inclusion [GeV]
    HBThreshold = cms.double(0.5),
    EEWeights = cms.untracked.vdouble(1.0, 1.0, 1.0, 1.0, 1.0),
    # Energy threshold for long-fiber HF readout inclusion [GeV]
    HF1Threshold = cms.double(1.2),
    HF2Weights = cms.untracked.vdouble(1.0, 1.0, 1.0, 1.0, 1.0),
    HOWeights = cms.untracked.vdouble(1.0, 1.0, 1.0, 1.0, 1.0),
    EEGrid = cms.untracked.vdouble(-1.0, 1.0, 10.0, 100.0, 1000.0),
    # Weighting factor for HE 10-degree cells   
    HEDWeight = cms.double(1.0),
    # Method for momentum reconstruction
    MomConstrMethod = cms.int32(0),
    # Weighting factor for EE   
    EEWeight = cms.double(1.0),
    # HO on/off flag for tower energy reconstruction
    UseHO = cms.bool(True),
    HBWeights = cms.untracked.vdouble(1.0, 1.0, 1.0, 1.0, 1.0),
    # Weighting factor for HE 5-degree cells   
    HESWeight = cms.double(1.0),
    # Weighting factor for HF long-fiber readouts 
    HF1Weight = cms.double(1.0),
    HF2Grid = cms.untracked.vdouble(-1.0, 1.0, 10.0, 100.0, 1000.0),
    HEDWeights = cms.untracked.vdouble(1.0, 1.0, 1.0, 1.0, 1.0),
    HF1Grid = cms.untracked.vdouble(-1.0, 1.0, 10.0, 100.0, 1000.0),
    EBWeights = cms.untracked.vdouble(1.0, 1.0, 1.0, 1.0, 1.0),
    # Weighting factor for HO 
    HOWeight = cms.double(1.0),
    # Energy threshold for EB crystal inclusion [GeV]
    EBThreshold = cms.double(0.09),
    # Label of HBHERecHitCollection to use
    hbheInput = cms.InputTag("hbhereco"),
    # Global energy threshold on Hcal [GeV]
    HcalThreshold = cms.double(-1000.0),
    # Energy threshold for short-fiber HF readout inclusion [GeV]
    HF2Threshold = cms.double(1.8),
    # Energy threshold for EE crystal inclusion [GeV]
    EEThreshold = cms.double(0.45),
    # Energy threshold for 5-degree (phi) HE cell inclusion [GeV]
    HESThreshold = cms.double(0.7),
    HF1Weights = cms.untracked.vdouble(1.0, 1.0, 1.0, 1.0, 1.0),
    # Label of HORecHitCollection to use
    hoInput = cms.InputTag("horeco"),
    HESGrid = cms.untracked.vdouble(-1.0, 1.0, 10.0, 100.0, 1000.0),
    #
    MomTotDepth = cms.double(0.0),
    HESWeights = cms.untracked.vdouble(1.0, 1.0, 1.0, 1.0, 1.0),
    # Energy threshold for 10-degree (phi) HE cel inclusion [GeV]
    HEDThreshold = cms.double(0.5),
    #
    MomHadDepth = cms.double(0.0),
    # Global energy threshold on tower [GeV]
    EcutTower = cms.double(-1000.0),
    HEDGrid = cms.untracked.vdouble(-1.0, 1.0, 10.0, 100.0, 1000.0),
    # Label of EcalRecHitCollections to use
    ecalInputs = cms.VInputTag(cms.InputTag("ecalRecHit","EcalRecHitsEB"), cms.InputTag("ecalRecHit","EcalRecHitsEE")),
    # Weighting factor for HB   
    HBWeight = cms.double(1.0),
    HOGrid = cms.untracked.vdouble(-1.0, 1.0, 10.0, 100.0, 1000.0),
    # Energy dependent weights and energy scale to be used
    EBGrid = cms.untracked.vdouble(-1.0, 1.0, 10.0, 100.0, 1000.0)
)


