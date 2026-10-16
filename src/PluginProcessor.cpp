#include "PluginProcessor.h"
#include "PluginEditor.h"
juce::AudioProcessorValueTreeState::ParameterLayout GBoyProcessor::layout(){
 juce::AudioProcessorValueTreeState::ParameterLayout layout;
 layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"volume",1},"Master volume",juce::NormalisableRange<float>(0.f,1.f,.0001f),.65f));
 for(int layer=0;layer<4;++layer){
  const auto title="Layer "+juce::String(layer+1)+" ";
  for(const auto& spec:gboy::layerSpecs){
   layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{gboy::parameterId(layer,spec.id),1},title+spec.label,juce::NormalisableRange<float>(spec.low,spec.high,.0001f,spec.skew),spec.initial));
  }
  layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{gboy::parameterId(layer,"enabled"),1},title+"Enabled",layer==0));
  layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{gboy::parameterId(layer,"generator"),1},title+"Engine",juce::StringArray{"Sampler","Virtual analog","Wavetable","FM","Granular","Vocal"},0));
  layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{gboy::parameterId(layer,"filter"),1},title+"Filter",juce::StringArray{"Bypass","Low pass","Band pass","High pass"},0));
  for(int route=0;route<4;++route){
   layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{gboy::routeId(layer,route,"source"),1},title+"Route "+juce::String(route+1)+" source",juce::StringArray{"Off","LFO","Envelope","Velocity","Mod wheel","Note","Macro"},0));
   layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{gboy::routeId(layer,route,"target"),1},title+"Route "+juce::String(route+1)+" target",juce::StringArray{"Pitch","Cutoff","Gain","Pan","Grain position","Shape / FM index"},0));
   layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{gboy::routeId(layer,route,"amount"),1},title+"Route "+juce::String(route+1)+" amount",juce::NormalisableRange<float>(-1.f,1.f),0.f));
  }
 }
 for(int layer=0;layer<4;++layer){
  const auto title="Layer "+juce::String(layer+1)+" Vocal ";
  for(const auto& spec:gboy::vocalSpecs)layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{gboy::parameterId(layer,spec.id),1},title+spec.label,juce::NormalisableRange<float>(spec.low,spec.high),spec.initial));
  layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{gboy::parameterId(layer,"vPreserve"),1},title+"Preserve formants",true));
  layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{gboy::parameterId(layer,"vReverse"),1},title+"Reverse",false));
  layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{gboy::parameterId(layer,"vSlices"),1},title+"Slice mapping",juce::StringArray{"Whole phrase","8 grid slices","16 grid slices","Detect onsets"},0));
  layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{gboy::parameterId(layer,"vEffect"),1},title+"Effect",juce::StringArray{"Clean","Telephone","Lo-fi","Stutter (120 BPM)"},0));
 }
 return layout;
}
GBoyProcessor::GBoyProcessor():AudioProcessor(BusesProperties().withOutput("Output",juce::AudioChannelSet::stereo(),true)),state(*this,nullptr,"GBOY",layout()){
 defaults=state.copyState();master=state.getRawParameterValue("volume");keyboard.addListener(this);
 for(int layer=0;layer<4;++layer){
  auto& h=handles[size_t(layer)];
  for(size_t i=0;i<h.values.size();++i)h.values[i]=state.getRawParameterValue(gboy::parameterId(layer,gboy::layerSpecs[i].id));
  for(size_t i=0;i<3;++i)h.vocalValues[i]=state.getRawParameterValue(gboy::parameterId(layer,gboy::vocalSpecs[i].id));
  h.preserve=state.getRawParameterValue(gboy::parameterId(layer,"vPreserve"));h.reverse=state.getRawParameterValue(gboy::parameterId(layer,"vReverse"));h.slicing=state.getRawParameterValue(gboy::parameterId(layer,"vSlices"));h.effect=state.getRawParameterValue(gboy::parameterId(layer,"vEffect"));
  h.enabled=state.getRawParameterValue(gboy::parameterId(layer,"enabled"));h.generator=state.getRawParameterValue(gboy::parameterId(layer,"generator"));h.filter=state.getRawParameterValue(gboy::parameterId(layer,"filter"));
  for(int route=0;route<4;++route){auto& row=h.routes[size_t(route)];row[0]=state.getRawParameterValue(gboy::routeId(layer,route,"source"));row[1]=state.getRawParameterValue(gboy::routeId(layer,route,"target"));row[2]=state.getRawParameterValue(gboy::routeId(layer,route,"amount"));}
  auto& slot=slots[size_t(layer)];slot.source="factory:"+juce::String(layer);slot.originalSource=slot.source;slot.original=std::make_unique<gboy::Sample>(gboy::factory(layer));slot.active=new gboy::Sample(*slot.original);slot.info.seconds=double(slot.active->left.size())/slot.active->rate;engine.layers[size_t(layer)].setSample(slot.active);
  slot.message=juce::StringArray{"CHIP PULSE","SOFT KEYS","FM BELL","ARCADE DRUM"}[layer];
  for(size_t i=0;i<128;++i)waveform[size_t(layer)][i].store(slot.active->left[i*40]);
 }
 worker=std::thread([this]{loadLoop();});
}
GBoyProcessor::~GBoyProcessor(){keyboard.removeListener(this);quit=true;worker.join();for(auto& slot:slots){delete slot.pending.exchange(nullptr);delete slot.retired.exchange(nullptr);delete slot.active;}}
void GBoyProcessor::prepareToPlay(double rate,int){engine.prepare(rate);for(size_t i=0;i<4;++i)engine.layers[i].setSample(slots[i].active);keyboard.reset();}
void GBoyProcessor::requestSample(juce::String source,int layer){if(layer<0||layer>3)return;std::lock_guard lock(mutex);auto& slot=slots[size_t(layer)];slot.requested=std::move(source);slot.hasRequest=true;slot.message="LOADING...";}
juce::String GBoyProcessor::status(int layer){std::lock_guard lock(mutex);return slots[size_t(juce::jlimit(0,3,layer))].message;}
void GBoyProcessor::readParameters(){
 engine.volume=master->load();
 for(size_t layer=0;layer<4;++layer){auto& p=engine.layers[layer].params;const auto& h=handles[layer];auto get=[&](size_t i){return h.values[i]->load();};
  p.enabled=h.enabled->load()>.5f;p.generator=gboy::Generator(juce::jlimit(0,5,int(h.generator->load())));p.filter=juce::jlimit(0,3,int(h.filter->load()));
  p.attack=get(0);p.decay=get(1);p.sustain=get(2);p.release=get(3);p.level=get(4);p.pan=get(5);p.tune=get(6);p.cutoff=get(7);p.resonance=get(8);p.shape=get(9);p.ratio=get(10);p.position=get(11);p.grainSize=get(12);p.lfoRate=get(13);p.macro=get(14);
  for(size_t route=0;route<4;++route){const auto& row=h.routes[route];p.routes[route]={gboy::ModSource(juce::jlimit(0,6,int(row[0]->load()))),gboy::ModTarget(juce::jlimit(0,5,int(row[1]->load()))),row[2]->load()};}
 }
}
gboy::vocal::Settings GBoyProcessor::vocalSettings(size_t layer) const {
 const auto& h=handles[layer];gboy::vocal::Settings settings;
 settings.pitch=h.vocalValues[0]->load();settings.stretch=h.vocalValues[1]->load();settings.formant=h.vocalValues[2]->load();
 if(std::abs(settings.pitch)<.0001f)settings.pitch=0;if(std::abs(settings.stretch-1)<.0001f)settings.stretch=1;if(std::abs(settings.formant)<.0001f)settings.formant=0;
 settings.preserve=h.preserve->load()>.5f;settings.reverse=h.reverse->load()>.5f;settings.slicing=juce::jlimit(0,3,int(h.slicing->load()));settings.effect=juce::jlimit(0,3,int(h.effect->load()));return settings;
}
bool GBoyProcessor::isBusy(int layer) const {
 if(layer<0||layer>3)return false;const auto index=size_t(layer);const auto desired=vocalSettings(index);std::lock_guard lock(mutex);const auto& slot=slots[index];
 return slot.hasRequest||slot.rendering||slot.sourceDirty||slot.pending.load()!=nullptr||!(desired==slot.applied);
}
GBoyProcessor::SliceInfo GBoyProcessor::sliceInfo(int layer) const {std::lock_guard lock(mutex);return slots[size_t(juce::jlimit(0,3,layer))].info;}
void GBoyProcessor::loadLoop(){
 juce::AudioFormatManager formats;formats.registerBasicFormats();
 std::array<gboy::vocal::Settings,4> observed{};
 std::array<std::chrono::steady_clock::time_point,4> changed{};
 while(!quit.load()){
  for(size_t layer=0;layer<slots.size()&&!quit.load();++layer){
   auto& slot=slots[layer];delete slot.retired.exchange(nullptr);
   const auto desired=vocalSettings(layer);const auto now=std::chrono::steady_clock::now();
   if(!(desired==observed[layer])){observed[layer]=desired;changed[layer]=now;}
   juce::String next;bool load=false,render=false;
   {std::lock_guard lock(mutex);if(slot.hasRequest){next=slot.requested;slot.hasRequest=false;load=true;}render=load||((slot.sourceDirty||!(desired==slot.applied))&&now-changed[layer]>=std::chrono::milliseconds(150));if(render)slot.rendering=true;}
   if(!render)continue;
   try{
    if(load){
     std::unique_ptr<gboy::Sample> source;
     if(next=="vocal:0")source=std::make_unique<gboy::Sample>(gboy::vocal::syntheticPhrase());
     else if(next.startsWith("factory:")){int kind=next.substring(8).getIntValue();if(kind>=0&&kind<4)source=std::make_unique<gboy::Sample>(gboy::factory(kind));}
     else{
      std::unique_ptr<juce::AudioFormatReader> reader(formats.createReaderFor(juce::File(next)));
      if(reader&&std::isfinite(reader->sampleRate)&&reader->sampleRate>=8000&&reader->sampleRate<=192000&&reader->lengthInSamples>1&&double(reader->lengthInSamples)<=reader->sampleRate*30&&reader->numChannels>0){
       juce::AudioBuffer<float> buffer(2,int(reader->lengthInSamples));
       if(reader->read(&buffer,0,buffer.getNumSamples(),0,true,true)){
        source=std::make_unique<gboy::Sample>();source->rate=reader->sampleRate;
        source->left.assign(buffer.getReadPointer(0),buffer.getReadPointer(0)+buffer.getNumSamples());const auto* right=buffer.getReadPointer(reader->numChannels>1?1:0);source->right.assign(right,right+buffer.getNumSamples());
        for(auto* data:{&source->left,&source->right})for(auto& value:*data)value=std::isfinite(value)?juce::jlimit(-1.f,1.f,value):0.f;
       }
      }
     }
     if(!source)throw std::runtime_error("Cannot load (missing, invalid, or over 30s). Previous sound retained.");
     slot.original=std::move(source);slot.originalSource=next;{std::lock_guard lock(mutex);slot.sourceDirty=true;}
    }
    if(!slot.original)throw std::runtime_error("No original source for vocal rendering.");
    auto sample=std::make_unique<gboy::Sample>(gboy::vocal::render(*slot.original,desired,&quit));
    if(!(desired==vocalSettings(layer))){std::lock_guard lock(mutex);slot.rendering=false;continue;}
    SliceInfo info;info.count=sample->sliceCount;info.seconds=double(sample->left.size())/sample->rate;
    for(int i=0;i<=info.count;++i)info.edges[size_t(i)]=double(sample->sliceEdges[size_t(i)])/double(sample->left.size());
    std::array<float,128> peaks{};
    for(size_t i=0;i<128;++i){const size_t begin=sample->left.size()*i/128,end=std::max(begin+1,sample->left.size()*(i+1)/128);float peak=0;for(size_t j=begin;j<std::min(end,sample->left.size());++j)if(std::abs(sample->left[j])>std::abs(peak))peak=sample->left[j];peaks[i]=peak;}
    std::lock_guard lock(mutex);
    if(!slot.hasRequest){
     for(size_t i=0;i<128;++i)waveform[layer][i].store(peaks[i]);
     delete slot.pending.exchange(sample.release());slot.source=slot.originalSource;slot.applied=desired;slot.sourceDirty=false;slot.info=info;
     slot.message=slot.source=="vocal:0"?"SYNTHETIC VOWEL PHRASE":slot.source.startsWith("factory:")?juce::StringArray{"CHIP PULSE","SOFT KEYS","FM BELL","ARCADE DRUM"}[slot.source.substring(8).getIntValue()]:juce::File(slot.source).getFileName();
    }
    slot.rendering=false;
   }catch(const std::exception& error){std::lock_guard lock(mutex);slot.applied=desired;slot.sourceDirty=false;slot.rendering=false;slot.message=error.what();}
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
 }
}
void GBoyProcessor::processBlock(juce::AudioBuffer<float>& b,juce::MidiBuffer& midi){
 juce::ScopedNoDenormals noDenormals;
 for(size_t layer=0;layer<4;++layer){auto& slot=slots[layer];if(slot.retired.load()==nullptr){if(auto* sample=slot.pending.exchange(nullptr)){auto* old=slot.active;slot.active=sample;engine.layers[layer].setSample(sample);slot.retired.store(old);}}}
 readParameters();
 int start1,size1,start2,size2;keyFifo.prepareToRead(256,start1,size1,start2,size2);
 auto apply=[this](int index){const auto& e=keyEvents[size_t(index)];if(e.velocity>0)engine.noteOn(e.note,e.velocity,e.channel);else engine.noteOff(e.note,e.channel);};
 for(int i=0;i<size1;++i)apply(start1+i);for(int i=0;i<size2;++i)apply(start2+i);keyFifo.finishedRead(size1+size2);
 if(panicRequested.exchange(false))engine.allOff();
 b.clear();if(b.getNumChannels()<2)return;
 int pos=0;
 for(const auto meta:midi){if(meta.numBytes>3)continue;int at=juce::jlimit(pos,b.getNumSamples(),meta.samplePosition);engine.render(b.getWritePointer(0)+pos,b.getWritePointer(1)+pos,at-pos);pos=at;
  auto m=meta.getMessage();if(m.isNoteOn())engine.noteOn(m.getNoteNumber(),m.getFloatVelocity(),m.getChannel());else if(m.isNoteOff())engine.noteOff(m.getNoteNumber(),m.getChannel());else if(m.isPitchWheel())engine.bend(m.getChannel(),float(m.getPitchWheelValue()-8192)/8192.f);else if(m.isController()&&m.getControllerNumber()==1)engine.wheel(m.getChannel(),float(m.getControllerValue())/127.f);else if(m.isAllNotesOff()||m.isAllSoundOff())engine.allOff(m.getChannel());
 }
 engine.render(b.getWritePointer(0)+pos,b.getWritePointer(1)+pos,b.getNumSamples()-pos);
 meter.store(b.getMagnitude(0,b.getNumSamples()));midi.clear();
}
void GBoyProcessor::getStateInformation(juce::MemoryBlock& block){
 auto tree=state.copyState();tree.setProperty("version",3,nullptr);
 {std::lock_guard lock(mutex);for(int i=0;i<4;++i)tree.setProperty("sampleSource"+juce::String(i+1),slots[size_t(i)].source,nullptr);}
 auto xml=tree.createXml();copyXmlToBinary(*xml,block);
}
void GBoyProcessor::setStateInformation(const void* data,int size){
 if(size<=0||size>1024*1024)return;
 if(auto xml=getXmlFromBinary(data,size)){auto input=juce::ValueTree::fromXml(*xml);if(!input.hasType("GBOY"))return;
  // Start from defaults, so Phase 1 state cannot inherit stale Phase 2 settings.
  auto migrated=defaults.createCopy();
  for(int i=0;i<input.getNumChildren();++i){auto child=input.getChild(i);auto target=migrated.getChildWithProperty("id",child.getProperty("id"));if(target.isValid()&&child.hasProperty("value"))target.setProperty("value",child.getProperty("value"),nullptr);}
  state.replaceState(migrated);
  for(int i=0;i<4;++i){auto fallback=i==0?input.getProperty("sampleSource","factory:0").toString():"factory:"+juce::String(i);requestSample(input.getProperty("sampleSource"+juce::String(i+1),fallback).toString(),i);}
  panicRequested.store(true);
 }
}
juce::AudioProcessorEditor* GBoyProcessor::createEditor(){return new GBoyEditor(*this);}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new GBoyProcessor();}

void GBoyProcessor::handleNoteOn(juce::MidiKeyboardState*,int channel,int note,float velocity){int a,n,b,m;keyFifo.prepareToWrite(1,a,n,b,m);if(n){keyEvents[size_t(a)]={note,channel,velocity};keyFifo.finishedWrite(1);}else panicRequested.store(true);}
void GBoyProcessor::handleNoteOff(juce::MidiKeyboardState* k,int channel,int note,float){handleNoteOn(k,channel,note,0);}

void GBoyProcessor::loadHybridPreset(int preset){
 auto tree=defaults.createCopy();
 auto set=[&](int layer,const std::string& suffix,float value){auto node=tree.getChildWithProperty("id",juce::String(gboy::parameterId(layer,suffix)));if(node.isValid())node.setProperty("value",value,nullptr);};
 const int count=preset==3?4:3;
 for(int layer=0;layer<count;++layer){
  set(layer,"enabled",1);set(layer,"level",layer==0?.7f:.4f);set(layer,"pan",layer==1?-.35f:layer==2?.35f:0.f);
  if(preset==0){set(layer,"generator",float(layer+1));set(layer,"tune",layer==1?12.f:0.f);set(layer,"filter",1);set(layer,"cutoff",layer==0?2800.f:7000.f);set(layer,"shape",.3f);}
  else if(preset==1){set(layer,"generator",layer==0?4.f:layer==1?2.f:3.f);set(layer,"attack",.6f);set(layer,"release",1.5f);set(layer,"filter",1);set(layer,"cutoff",5000);set(layer,"lfoRate",.3f);set(layer,"r1_source",1);set(layer,"r1_target",layer==0?4.f:1.f);set(layer,"r1_amount",.3f);}
  else if(preset==2){set(layer,"generator",layer==1?3.f:1.f);set(layer,"tune",-12);set(layer,"filter",1);set(layer,"cutoff",700);set(layer,"resonance",.35f);set(layer,"shape",.65f);set(layer,"r1_source",2);set(layer,"r1_target",1);set(layer,"r1_amount",.7f);}
  else{set(layer,"generator",3);set(layer,"ratio",float(1+layer*2));set(layer,"shape",.25f);set(layer,"decay",1.2f);set(layer,"sustain",.2f);set(layer,"release",1);set(layer,"tune",layer==3?12.f:0.f);}
 }
 state.replaceState(tree);for(int layer=0;layer<4;++layer)requestSample("factory:"+juce::String(layer),layer);panicRequested.store(true);
}

bool GBoyProcessor::exportSlices(const juce::File& file,int layer) const {
 if(isBusy(layer))return false;const auto info=sliceInfo(layer);if(info.count<1)return false;
 juce::MidiMessageSequence sequence;auto tempo=juce::MidiMessage::tempoMetaEvent(500000);tempo.setTimeStamp(0);sequence.addEvent(tempo);
 for(int i=0;i<info.count;++i){auto on=juce::MidiMessage::noteOn(1,60+i,juce::uint8(100));auto off=juce::MidiMessage::noteOff(1,60+i);const double begin=info.edges[size_t(i)]*info.seconds*960,end=info.edges[size_t(i+1)]*info.seconds*960;on.setTimeStamp(begin);off.setTimeStamp(begin+(end-begin)*.9);sequence.addEvent(on);sequence.addEvent(off);}
 sequence.updateMatchedPairs();juce::MidiFile midi;midi.setTicksPerQuarterNote(480);midi.addTrack(sequence);auto output=file.createOutputStream();if(!output||!output->setPosition(0)||output->truncate().failed())return false;return midi.writeTo(*output);
}
void GBoyProcessor::loadVocalPreset(int preset){
 auto tree=defaults.createCopy();auto set=[&](int layer,const std::string& suffix,float value){auto node=tree.getChildWithProperty("id",juce::String(gboy::parameterId(layer,suffix)));if(node.isValid())node.setProperty("value",value,nullptr);};
 const int count=preset==3?3:1;
 for(int layer=0;layer<count;++layer){set(layer,"enabled",1);set(layer,"generator",5);set(layer,"level",layer==0?.8f:.5f);set(layer,"release",.2f);
  if(preset==0){set(layer,"vSlices",3);set(layer,"vPitch",3);set(layer,"vEffect",2);}
  else if(preset==1){set(layer,"vReverse",1);set(layer,"vStretch",1.6f);set(layer,"vFormant",-4);set(layer,"attack",.15f);set(layer,"release",1);}
  else if(preset==2){set(layer,"vSlices",1);set(layer,"vEffect",1);}
  else{set(layer,"vPitch",float(layer==1?4:layer==2?7:0));set(layer,"vStretch",1.25f);set(layer,"pan",layer==1?-.4f:layer==2?.4f:0);set(layer,"attack",.1f);set(layer,"release",.7f);}
 }
 state.replaceState(tree);for(int layer=0;layer<4;++layer)requestSample(layer<count?"vocal:0":"factory:"+juce::String(layer),layer);panicRequested.store(true);
}
