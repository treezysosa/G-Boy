#include "PluginProcessor.h"
#include <iostream>
#include <stdexcept>
void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
void set(GBoyProcessor& p,const std::string& id,float value){auto* param=p.state.getParameter(id);require(param!=nullptr,"parameter exists");param->setValueNotifyingHost(param->convertTo0to1(value));}
void settle(GBoyProcessor& p){
 juce::AudioBuffer<float> buffer(2,128);juce::MidiBuffer midi;
 for(int n=0;n<500;++n){p.processBlock(buffer,midi);bool loading=false;for(int i=0;i<4;++i)loading|=p.status(i)=="LOADING...";if(!loading)return;std::this_thread::sleep_for(std::chrono::milliseconds(10));}
 throw std::runtime_error("sample loader timeout");
}
int main(){
 juce::ScopedJuceInitialiser_GUI gui;
 auto p=std::make_unique<GBoyProcessor>();p->prepareToPlay(48000,256);
 for(int layer=0;layer<4;++layer){set(*p,gboy::parameterId(layer,"enabled"),1);set(*p,gboy::parameterId(layer,"generator"),float(layer+1));set(*p,gboy::parameterId(layer,"tune"),float(layer*3));}
 set(*p,gboy::routeId(2,0,"source"),4);set(*p,gboy::routeId(2,0,"target"),0);set(*p,gboy::routeId(2,0,"amount"),.7f);
 juce::MemoryBlock saved;p->getStateInformation(saved);set(*p,gboy::parameterId(2,"tune"),-12);p->setStateInformation(saved.getData(),int(saved.getSize()));settle(*p);
 require(p->state.getRawParameterValue(gboy::parameterId(2,"tune"))->load()==6,"layer state roundtrip");require(p->state.getRawParameterValue(gboy::routeId(2,0,"amount"))->load()==.7f,"matrix state roundtrip");
 juce::AudioBuffer<float> buffer(2,256);juce::MidiBuffer midi;midi.addEvent(juce::MidiMessage::noteOn(1,60,.8f),64);p->processBlock(buffer,midi);
 require(buffer.getMagnitude(0,64)==0,"sample offset preceding note silent");require(buffer.getMagnitude(64,192)>0,"sample offset following note audible");
 midi.addEvent(juce::MidiMessage::pitchWheel(1,16383),0);midi.addEvent(juce::MidiMessage::controllerEvent(1,1,127),0);p->processBlock(buffer,midi);require(std::isfinite(buffer.getMagnitude(0,256)),"MIDI modulation finite");
 juce::XmlElement phase1("GBOY");auto* attack=phase1.createNewChildElement("PARAM");attack->setAttribute("id","attack");attack->setAttribute("value",.123);phase1.setAttribute("sampleSource","factory:1");juce::MemoryBlock legacy;juce::AudioProcessor::copyXmlToBinary(phase1,legacy);p->setStateInformation(legacy.getData(),int(legacy.getSize()));settle(*p);
 require(std::abs(p->state.getRawParameterValue("attack")->load()-.123f)<1e-5f,"Phase 1 attack migrated");
 for(int layer=1;layer<4;++layer)require(p->state.getRawParameterValue(gboy::parameterId(layer,"enabled"))->load()==0,"Phase 1 extra layers disabled");
 require(p->state.getRawParameterValue(gboy::routeId(2,0,"amount"))->load()==0,"old matrix does not leak into legacy state");
 p->requestSample("/a-missing-gboy-file.wav",2);settle(*p);require(p->status(2).startsWith("Cannot load"),"missing sample reported");
 p->setStateInformation("invalid",7);p->setStateInformation(nullptr,0);
 std::cout<<"PASS processor: layer/matrix recall, sample-offset MIDI, wheel/bend, Phase 1 migration, missing sample\n";
}
