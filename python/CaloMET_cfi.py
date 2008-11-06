import FWCore.ParameterSet.Config as cms

# File: CaloMET.cff
# Original Author: R. Cavanaugh
# Date: 08.08.2006
#
# Form uncorrected Missing ET from Calorimeter Towers and store into event as a CaloMET
# product

# Modification by F. Ratnikov and R. Remington
# Date: 10/21/08
# Additional modules available for MET Reconstruction using towers w/wo HO included


met = cms.EDProducer("METProducer",
                         src = cms.InputTag("towerMaker"),
                         METType = cms.string('CaloMET'),
                         alias = cms.string('RawCaloMET'),
                         noHF = cms.bool(False),
                         geomCut = cms.bool(False), ## make geometry cuts on input objects
                         globalThreshold = cms.double(0.5),
                         InputType = cms.string('CandidateCollection')
                     )

metHO = met.clone()
metHO.src = "towerMakerWithHO"
metHO.alias = 'RawCaloMETHO'

metOpt = cms.EDProducer("METProducer",
                            src = cms.InputTag("calotoweroptmaker"),
                            METType = cms.string('CaloMET'),
                            alias = cms.string('RawCaloMETOpt'),
                            noHF = cms.bool(False),
                            geomCut = cms.bool(False), ## make geometry cuts on input objects
                            globalThreshold = cms.double(0.0),
                            InputType = cms.string('CandidateCollection')
                        )

metOptHO = metOpt.clone()
metOptHO.src = "calotoweroptmakerWithHO"
metOptHO.alias = 'RawCaloMETOptHO'

metNoHF = cms.EDProducer("METProducer",
                             src = cms.InputTag("towerMaker"),
                             METType = cms.string('CaloMET'),
                             alias = cms.string('RawCaloMETNoHF'),
                             noHF = cms.bool(True),
                             geomCut = cms.bool(False), ## make geometry cuts on input objects
                             globalThreshold = cms.double(0.5),
                             InputType = cms.string('CandidateCollection')
                         )

metNoHFHO = metNoHF.clone()
metNoHFHO.src = "towerMakerWithHO"
metNoHFHO.alias = 'RawCaloMETNoHFHO'

metOptNoHF = cms.EDProducer("METProducer",
                                src = cms.InputTag("calotoweroptmaker"),
                                METType = cms.string('CaloMET'),
                                alias = cms.string('RawCaloMETOptNoHF'),
                                noHF = cms.bool(True),
                                geomCut = cms.bool(False), ## make geometry cuts on input objects
                                globalThreshold = cms.double(0.0),
                                InputType = cms.string('CandidateCollection')
                            )

metOptNoHFHO = metOptNoHF.clone()
metOptNoHFHO.src = "calotoweroptmakerWithHO"
metOptNoHFHO.alias = 'RawCaloMETOptNoHFHO'


