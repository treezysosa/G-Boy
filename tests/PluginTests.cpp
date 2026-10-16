#include "PluginProcessor.h"
#include <iostream>
#include <iomanip>
#include <stdexcept>
void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
void set(GBoyProcessor& p,const std::string& id,float value){auto* param=p.state.getParameter(id);require(param!=nullptr,"parameter exists");param->setValueNotifyingHost(param->convertTo0to1(value));}
void settle(GBoyProcessor& p){
 juce::AudioBuffer<float> buffer(2,128);juce::MidiBuffer midi;
 for(int n=0;n<3000;++n){p.processBlock(buffer,midi);bool loading=false;for(int i=0;i<4;++i)loading|=p.isBusy(i);if(!loading)return;std::this_thread::sleep_for(std::chrono::milliseconds(10));}
 throw std::runtime_error("sample loader timeout");
}
int runTests(){
 juce::ScopedJuceInitialiser_GUI gui;
 auto p=std::make_unique<GBoyProcessor>();p->prepareToPlay(48000,256);
 for(int layer=0;layer<4;++layer){set(*p,gboy::parameterId(layer,"enabled"),1);set(*p,gboy::parameterId(layer,"generator"),float(layer+1));set(*p,gboy::parameterId(layer,"tune"),float(layer*3));}
 set(*p,gboy::routeId(2,0,"source"),4);set(*p,gboy::routeId(2,0,"target"),0);set(*p,gboy::routeId(2,0,"amount"),.7f);
 const float savedTune=p->state.getRawParameterValue(gboy::parameterId(2,"tune"))->load();
 const float savedAmount=p->state.getRawParameterValue(gboy::routeId(2,0,"amount"))->load();
 juce::MemoryBlock saved;p->getStateInformation(saved);set(*p,gboy::parameterId(2,"tune"),-12);p->setStateInformation(saved.getData(),int(saved.getSize()));settle(*p);
  const float restoredTune=p->state.getRawParameterValue(gboy::parameterId(2,"tune"))->load();
 const float restoredAmount=p->state.getRawParameterValue(gboy::routeId(2,0,"amount"))->load();
 std::cerr<<std::setprecision(9)<<"Tune saved/restored: "<<savedTune<<" / "<<restoredTune<<"; route: "<<savedAmount<<" / "<<restoredAmount<<"\n";
 require(std::abs(restoredTune-savedTune)<.0002f&&std::abs(restoredTune-6)<.0002f,"layer state roundtrip");require(std::abs(restoredAmount-savedAmount)<.0001f&&std::abs(restoredAmount-.7f)<.0001f,"matrix state roundtrip");
 juce::AudioBuffer<float> buffer(2,256);juce::MidiBuffer midi;midi.addEvent(juce::MidiMessage::noteOn(1,60,.8f),64);p->processBlock(buffer,midi);
 require(buffer.getMagnitude(0,64)==0,"sample offset preceding note silent");require(buffer.getMagnitude(64,192)>0,"sample offset following note audible");
 midi.addEvent(juce::MidiMessage::pitchWheel(1,16383),0);midi.addEvent(juce::MidiMessage::controllerEvent(1,1,127),0);p->processBlock(buffer,midi);require(std::isfinite(buffer.getMagnitude(0,256)),"MIDI modulation finite");
 juce::XmlElement phase1("GBOY");auto* attack=phase1.createNewChildElement("PARAM");attack->setAttribute("id","attack");attack->setAttribute("value",.123);phase1.setAttribute("sampleSource","factory:1");juce::MemoryBlock legacy;juce::AudioProcessor::copyXmlToBinary(phase1,legacy);p->setStateInformation(legacy.getData(),int(legacy.getSize()));settle(*p);
 require(std::abs(p->state.getRawParameterValue("attack")->load()-.123f)<1e-5f,"Phase 1 attack migrated");
 for(int layer=1;layer<4;++layer)require(p->state.getRawParameterValue(gboy::parameterId(layer,"enabled"))->load()==0,"Phase 1 extra layers disabled");
 const float legacyAmount=p->state.getRawParameterValue(gboy::routeId(2,0,"amount"))->load();std::cerr<<"Legacy route amount: "<<legacyAmount<<"\n";
 require(std::abs(legacyAmount)<.00001f,"old matrix does not leak into legacy state");
 p->requestSample("/a-missing-gboy-file.wav",2);settle(*p);require(p->status(2).startsWith("Cannot load"),"missing sample reported");
 // Decode a real mono WAV through the worker, into only layer 4.
 juce::TemporaryFile temporary(".wav");juce::WavAudioFormat format;auto stream=temporary.getFile().createOutputStream();require(stream!=nullptr,"temporary WAV stream");
 std::unique_ptr<juce::AudioFormatWriter> writer(format.createWriterFor(stream.get(),44100,1,16,{},0));require(writer!=nullptr,"WAV writer");stream.release();
 juce::AudioBuffer<float> tone(1,4410);for(int i=0;i<tone.getNumSamples();++i)tone.setSample(0,i,float(std::sin(gboy::tau*220*i/44100))*.5f);require(writer->writeFromAudioSampleBuffer(tone,0,tone.getNumSamples()),"write WAV");writer.reset();
 const auto layer1Status=p->status(0);p->requestSample(temporary.getFile().getFullPathName(),3);settle(*p);require(p->status(0)==layer1Status,"import isolated from layer 1");require(p->status(3)==temporary.getFile().getFileName(),"real WAV decoded");
 set(*p,gboy::parameterId(0,"enabled"),0);set(*p,gboy::parameterId(3,"enabled"),1);midi.addEvent(juce::MidiMessage::noteOn(1,60,.8f),0);p->processBlock(buffer,midi);require(buffer.getMagnitude(0,256)>0,"imported layer sample plays");
 for(int preset=0;preset<4;++preset){p->loadHybridPreset(preset);settle(*p);midi.addEvent(juce::MidiMessage::noteOn(1,60,.8f),0);p->processBlock(buffer,midi);require(buffer.getMagnitude(0,256)>0,"hybrid factory preset audible");}
 // Vocal render controls, immutable publication, slice export, and recall.
 p->loadVocalPreset(2);
 // A stopped host may not call processBlock. Export must still work once the worker is done.
 for(int n=0;n<3000&&p->isBusy(0,false);++n)std::this_thread::sleep_for(std::chrono::milliseconds(10));
 juce::TemporaryFile stoppedMidi(".mid");require(p->exportSlices(stoppedMidi.getFile(),0),"MIDI export while audio processing is stopped");
 settle(*p);require(p->sliceInfo(0).count==8,"vocal preset has eight slice keys");
 require(p->status(0)=="SYNTHETIC VOWEL PHRASE","synthetic source is accurately labelled");
 set(*p,gboy::parameterId(0,"vPitch"),5);set(*p,gboy::parameterId(0,"vStretch"),1.5f);set(*p,gboy::parameterId(0,"vFormant"),-3);settle(*p);
 require(std::abs(p->sliceInfo(0).seconds-4.8)<.002,"worker renders duration independently");
 juce::TemporaryFile midiFile(".mid");require(p->exportSlices(midiFile.getFile(),0),"export slice MIDI");auto midiInput=midiFile.getFile().createInputStream();juce::MidiFile parsed;require(midiInput&&parsed.readFrom(*midiInput),"parse exported MIDI");require(parsed.getNumTracks()==1&&parsed.getTimeFormat()==480,"standard MIDI timing");int noteOns=0;for(int i=0;i<parsed.getTrack(0)->getNumEvents();++i)noteOns+=parsed.getTrack(0)->getEventPointer(i)->message.isNoteOn();require(noteOns==8,"exported MIDI notes correspond to slices");
 juce::MemoryBlock vocalState;p->getStateInformation(vocalState);p->loadHybridPreset(0);settle(*p);p->setStateInformation(vocalState.getData(),int(vocalState.getSize()));settle(*p);require(p->sliceInfo(0).count==8&&std::abs(p->sliceInfo(0).seconds-4.8)<.002,"vocal rendering reconstructs after recall");
 midi.addEvent(juce::MidiMessage::noteOn(1,61,.8f),0);p->processBlock(buffer,midi);require(buffer.getMagnitude(0,256)>0,"rendered vocal slice responds to MIDI");
 for(int preset:{0,1,3}){p->loadVocalPreset(preset);settle(*p);midi.addEvent(juce::MidiMessage::noteOn(1,60,.8f),0);p->processBlock(buffer,midi);double peak=buffer.getMagnitude(0,256);for(int n=0;n<100&&peak==0;++n){p->processBlock(buffer,midi);peak=std::max(peak,double(buffer.getMagnitude(0,256)));}require(peak>0,"vocal preset audible");}
 // Phase 4 rack persistence, ordering and sequence playback/export.
 p->loadHybridPreset(0);settle(*p);p->loadRackPreset(4,4);
 juce::TemporaryFile rackFile(".gbfx");require(p->saveRack(rackFile.getFile(),4),"rack preset save");p->moveEffect(4,0,1);require(int(p->state.getRawParameterValue(gboy::fx::parameterId(4,0,"type"))->load())==int(gboy::fx::Type::Duck),"rack slot order changes");p->loadRackPreset(4,0);require(p->loadRack(rackFile.getFile(),4),"rack preset load");require(int(p->state.getRawParameterValue(gboy::fx::parameterId(4,0,"type"))->load())==int(gboy::fx::Type::PingPong),"rack restores effect choice");
 set(*p,gboy::routeId(0,0,"fxTarget"),3);set(*p,gboy::routeId(0,0,"source"),6);set(*p,gboy::routeId(0,0,"amount"),.2f);set(*p,gboy::parameterId(0,"macro"),.5f);
 p->loadSequencePreset(0);set(*p,"seq_sync",0);set(*p,"seq_enabled",1);set(*p,"seq_run",1);double seqEnergy=0;for(int block=0;block<100;++block){p->processBlock(buffer,midi);seqEnergy+=buffer.getMagnitude(0,256);}require(seqEnergy>.01&&p->playingStep.load()>=0,"internal sequencer produces audio without incoming MIDI");
 juce::TemporaryFile patternFile(".mid");require(p->exportSequence(patternFile.getFile()),"pattern MIDI export");auto patternInput=patternFile.getFile().createInputStream();juce::MidiFile patternMidi;require(patternInput&&patternMidi.readFrom(*patternInput),"parse pattern MIDI");require(patternMidi.getTimeFormat()==960,"pattern MIDI PPQN");int patternOns=0,patternOffs=0;for(int i=0;i<patternMidi.getTrack(0)->getNumEvents();++i){const auto& event=patternMidi.getTrack(0)->getEventPointer(i)->message;patternOns+=event.isNoteOn();patternOffs+=event.isNoteOff();}require(patternOns==12&&patternOns==patternOffs,"export has expected notes and matching releases");
 juce::MemoryBlock phase4;p->getStateInformation(phase4);p->stopSequence();p->loadRackPreset(4,0);set(*p,gboy::seq::stepId(0,0,"pitch"),-12);p->setStateInformation(phase4.getData(),int(phase4.getSize()));settle(*p);require(p->state.getRawParameterValue("seq_enabled")->load()>.5f&&int(p->state.getRawParameterValue(gboy::seq::stepId(0,0,"pitch"))->load())==0,"sequencer state recall");require(int(p->state.getRawParameterValue(gboy::fx::parameterId(4,0,"type"))->load())==int(gboy::fx::Type::PingPong),"master rack recall");
 struct Playhead final:juce::AudioPlayHead {PositionInfo info;juce::Optional<PositionInfo> getPosition() const override{return info;}} playhead;
 set(*p,"seq_sync",1);playhead.info.setBpm(90);playhead.info.setPpqPosition(2.0);playhead.info.setIsPlaying(true);p->setPlayHead(&playhead);p->processBlock(buffer,midi);require(p->hostSyncActive.load()&&p->playingStep.load()==8,"host PPQ selects synced step");playhead.info.setIsPlaying(false);p->processBlock(buffer,midi);require(p->playingStep.load()==-1,"host stop releases sequence");p->setPlayHead(nullptr);p->stopSequence();
 p->setStateInformation(legacy.getData(),int(legacy.getSize()));settle(*p);require(p->state.getRawParameterValue("seq_enabled")->load()==0&&p->state.getRawParameterValue(gboy::fx::parameterId(4,0,"type"))->load()==0,"legacy state disables sequencer and rack");
 p->setStateInformation("invalid",7);p->setStateInformation(nullptr,0);
 std::cout<<"PASS processor: layer/matrix recall, sample-offset MIDI, wheel/bend, Phase 1 migration, missing sample, vocal render/recall, MIDI export, vocal presets, rack persistence, sequencer audio/export/recall/host sync\n";
 return 0;
}
int main(){try{return runTests();}catch(const std::exception& error){std::cerr<<"FAIL processor: "<<error.what()<<std::endl;return 1;}}
