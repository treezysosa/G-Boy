#include "PluginProcessor.h"
#include "PluginEditor.h"
juce::AudioProcessorValueTreeState::ParameterLayout GBoyProcessor::layout(){
 juce::AudioProcessorValueTreeState::ParameterLayout l;
 auto add=[&](const char* id,const char* name,float lo,float hi,float value){l.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id,1},name,juce::NormalisableRange<float>(lo,hi,.0001f,lo>0?.4f:1.f),value));};
 add("attack","Attack",.001f,5,.01f);add("decay","Decay",.001f,5,.15f);add("sustain","Sustain",0,1,.7f);add("release","Release",.001f,10,.25f);add("volume","Volume",0,1,.65f);return l;
}
GBoyProcessor::GBoyProcessor():AudioProcessor(BusesProperties().withOutput("Output",juce::AudioChannelSet::stereo(),true)),state(*this,nullptr,"GBOY",layout()){
 keyboard.addListener(this);
 active=new gboy::Sample(gboy::factory(0));engine.setSample(active);
 for(size_t i=0;i<waveform.size();++i)waveform[i].store(active->left[i*40]);
 worker=std::thread([this]{loadLoop();});
}
GBoyProcessor::~GBoyProcessor(){keyboard.removeListener(this);quit=true;worker.join();delete pending.exchange(nullptr);delete retired.exchange(nullptr);delete active;}
void GBoyProcessor::prepareToPlay(double r,int){engine.prepare(r);engine.setSample(active);keyboard.reset();}
void GBoyProcessor::requestSample(juce::String s){std::lock_guard lock(mutex);requested=std::move(s);hasRequest=true;message="LOADING...";}
juce::String GBoyProcessor::status(){std::lock_guard lock(mutex);return message;}
void GBoyProcessor::loadLoop(){
 juce::AudioFormatManager formats;formats.registerBasicFormats();
 while(!quit.load()){
  delete retired.exchange(nullptr);
  juce::String next;bool load=false;
  {std::lock_guard lock(mutex);if(hasRequest){next=requested;hasRequest=false;load=true;}}
  if(load){
   std::unique_ptr<gboy::Sample> s;juce::String error;
   if(next.startsWith("factory:")){int k=next.substring(8).getIntValue();if(k>=0&&k<4)s=std::make_unique<gboy::Sample>(gboy::factory(k));}
   else{
    std::unique_ptr<juce::AudioFormatReader> reader(formats.createReaderFor(juce::File(next)));
    if(reader && reader->lengthInSamples>1 && reader->lengthInSamples<=192000*30 && reader->numChannels>0){
     juce::AudioBuffer<float> b(2,int(reader->lengthInSamples));
     if(reader->read(&b,0,b.getNumSamples(),0,true,true)){
      s=std::make_unique<gboy::Sample>();s->rate=reader->sampleRate;
      s->left.assign(b.getReadPointer(0),b.getReadPointer(0)+b.getNumSamples());
      const auto* right=b.getReadPointer(reader->numChannels>1?1:0);s->right.assign(right,right+b.getNumSamples());
     }
    }
    error="Cannot load (missing, invalid, or over 30s). Previous sound retained.";
   }
   if(s){
    for(size_t i=0;i<waveform.size();++i)waveform[i].store(s->left[i*std::min(size_t(40),(s->left.size()-1)/128)]);
    delete pending.exchange(s.release());
    std::lock_guard lock(mutex);source=next;message=next.startsWith("factory:")?juce::StringArray{"CHIP PULSE","SOFT KEYS","FM BELL","ARCADE DRUM"}[next.substring(8).getIntValue()]:juce::File(next).getFileName();
   }else{std::lock_guard lock(mutex);message=error.isEmpty()?"Invalid factory patch":error;}
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
 }
}
void GBoyProcessor::processBlock(juce::AudioBuffer<float>& b,juce::MidiBuffer& midi){
 juce::ScopedNoDenormals noDenormals;
 if(retired.load()==nullptr){if(auto* s=pending.exchange(nullptr)){auto* old=active;active=s;engine.setSample(active);retired.store(old);}}
 engine.params={state.getRawParameterValue("attack")->load(),state.getRawParameterValue("decay")->load(),state.getRawParameterValue("sustain")->load(),state.getRawParameterValue("release")->load(),state.getRawParameterValue("volume")->load()};
 int start1,size1,start2,size2;keyFifo.prepareToRead(256,start1,size1,start2,size2);
 auto apply=[this](int index){const auto& e=keyEvents[size_t(index)];if(e.velocity>0)engine.noteOn(e.note,e.velocity,e.channel);else engine.noteOff(e.note,e.channel);};
 for(int i=0;i<size1;++i)apply(start1+i);for(int i=0;i<size2;++i)apply(start2+i);keyFifo.finishedRead(size1+size2);
 if(panicRequested.exchange(false))engine.allOff();
 b.clear();if(b.getNumChannels()<2)return;
 int pos=0;
 for(const auto meta:midi){int at=juce::jlimit(pos,b.getNumSamples(),meta.samplePosition);engine.render(b.getWritePointer(0)+pos,b.getWritePointer(1)+pos,at-pos);pos=at;
  auto m=meta.getMessage();if(m.isNoteOn())engine.noteOn(m.getNoteNumber(),m.getFloatVelocity(),m.getChannel());else if(m.isNoteOff())engine.noteOff(m.getNoteNumber(),m.getChannel());else if(m.isAllNotesOff()||m.isAllSoundOff())engine.allOff(m.getChannel());
 }
 engine.render(b.getWritePointer(0)+pos,b.getWritePointer(1)+pos,b.getNumSamples()-pos);
 meter.store(b.getMagnitude(0,b.getNumSamples()));midi.clear();
}
void GBoyProcessor::getStateInformation(juce::MemoryBlock& block){auto tree=state.copyState();{std::lock_guard lock(mutex);tree.setProperty("sampleSource",source,nullptr);}auto xml=tree.createXml();copyXmlToBinary(*xml,block);}
void GBoyProcessor::setStateInformation(const void* data,int size){if(auto xml=getXmlFromBinary(data,size)){auto tree=juce::ValueTree::fromXml(*xml);if(tree.hasType("GBOY")){state.replaceState(tree);requestSample(tree.getProperty("sampleSource","factory:0").toString());}}}
juce::AudioProcessorEditor* GBoyProcessor::createEditor(){return new GBoyEditor(*this);}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new GBoyProcessor();}

void GBoyProcessor::handleNoteOn(juce::MidiKeyboardState*,int channel,int note,float velocity){int a,n,b,m;keyFifo.prepareToWrite(1,a,n,b,m);if(n){keyEvents[size_t(a)]={note,channel,velocity};keyFifo.finishedWrite(1);}else panicRequested.store(true);}
void GBoyProcessor::handleNoteOff(juce::MidiKeyboardState* k,int channel,int note,float){handleNoteOn(k,channel,note,0);}
