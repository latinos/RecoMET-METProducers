import FWCore.ParameterSet.Config as cms

# File: PFMET.cff
# Author: R. Cavanaugh
# Date: 28.10.2008
#
# Form uncorrected Missing ET from Particle Flow and store into event as a MET
# product

pfMet = cms.EDProducer("METProducer",
                           src = cms.InputTag("particleFlow"),
                           METType = cms.string('PFMET'),
                           alias = cms.string('PFMET'),
                           noHF = cms.bool(False),
                           globalThreshold = cms.double(0.0),
                           InputType = cms.string('PFCandidateCollection')
                       )