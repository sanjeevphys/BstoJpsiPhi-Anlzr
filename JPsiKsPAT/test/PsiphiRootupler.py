import FWCore.ParameterSet.Config as cms
process = cms.Process("Rootuple")

process.load("TrackingTools.TransientTrack.TransientTrackBuilder_cfi")
process.load('Configuration.StandardSequences.Services_cff')
process.load('SimGeneral.HepPDTESSource.pythiapdt_cfi')
process.load('FWCore.MessageService.MessageLogger_cfi')
process.load('Configuration.EventContent.EventContent_cff')
process.load('Configuration.StandardSequences.GeometryRecoDB_cff')
process.load('Configuration.StandardSequences.MagneticField_AutoFromDBCurrent_cff')
process.load('Configuration.StandardSequences.EndOfProcess_cff')
process.load('Configuration.StandardSequences.FrontierConditions_GlobalTag_cff')
from Configuration.AlCa.GlobalTag import GlobalTag
#process.GlobalTag = GlobalTag(process.GlobalTag, 'auto:run2_data', '')
process.GlobalTag = GlobalTag(process.GlobalTag, '92X_dataRun2_Prompt_v4', '')

process.MessageLogger.cerr.FwkReport.reportEvery = 100
process.options = cms.untracked.PSet( wantSummary = cms.untracked.bool(True) )
process.options.allowUnscheduled = cms.untracked.bool(True)

#process.maxEvents = cms.untracked.PSet(input = cms.untracked.int32(-1))
process.maxEvents = cms.untracked.PSet(input = cms.untracked.int32(100),SkipEvent = cms.untracked.vstring('ProductNotFound'))

process.source = cms.Source("PoolSource",
    fileNames = cms.untracked.vstring(


##lxplus Asanchez (testing)
#'/store/group/phys_bphys/asanchez/MINIAOD/A4B4AC67-B996-E611-9ECD-008CFAFBE8CE.root',

#MiniAOD 2017
#'/store/data/Run2017B/Charmonium/MINIAOD/PromptReco-v1/000/297/031/00000/E629DFF7-0B56-E711-8B6A-02163E0141F0.root',
'/store/data/Run2017B/Charmonium/MINIAOD/PromptReco-v1/000/297/050/00000/183B4680-3356-E711-B33A-02163E014487.root',


 )
)

#process.load("myAnalyzers.JPsiKsPAT.miniaodTriggerMatcher_cfi")
process.load("myAnalyzers.JPsiKsPAT.slimmedMuonsTriggerMatcher_cfi")  

process.load("myAnalyzers.JPsiKsPAT.PsiphiRootupler_cfi")
process.rootuple.dimuons = cms.InputTag('slimmedMuonsWithTrigger') 
#process.rootuple.dimuons = cms.InputTag('miniaodPATMuonsWithTrigger')                                                                                

process.TFileService = cms.Service("TFileService",
       fileName = cms.string('Rootuple_BstoJpsiphi_2017_MiniAOD.root'),                                                                            
)


process.p = cms.Path(process.slimmedMuonsWithTriggerSequence*process.rootuple)
#process.p = cms.Path(process.miniaodPATMuonsWithTriggerSequence*process.rootuple)                                                                    
#process.p = cms.Path(process.rootuple)



