#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "presets/FactoryPatches.h"
#include "presets/StateMigration.h"
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
 for(int layer=0;layer<4;++layer)for(int route=0;route<4;++route)layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{gboy::routeId(layer,route,"fxTarget"),1},"Layer "+juce::String(layer+1)+" Route "+juce::String(route+1)+" FX override",juce::StringArray{"Voice destination","Layer FX mix","Layer FX amount","Master FX mix","Master FX amount"},0));
 juce::StringArray effects;for(const auto& spec:gboy::fx::specs)effects.add(spec.name);
 for(int rack=0;rack<5;++rack)for(int slot=0;slot<4;++slot){
  const auto title=(rack==4?juce::String("Master"):"Layer "+juce::String(rack+1))+" FX "+juce::String(slot+1)+" ";
  layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{gboy::fx::parameterId(rack,slot,"type"),1},title+"Type",effects,0));
  layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{gboy::fx::parameterId(rack,slot,"bypass"),1},title+"Bypass",false));
  for(const auto* field:{"mix","a","b","c"})layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{gboy::fx::parameterId(rack,slot,field),1},title+field,juce::NormalisableRange<float>(0,1),juce::String(field)=="mix"?.3f:.5f));
 }
 for(const auto* flag:{"enabled","sync","run","latch","chain"})layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"seq_"+juce::String(flag),1},"Sequence "+juce::String(flag),juce::String(flag)=="sync"));
 auto choice=[&](const char* id,const char* name,juce::StringArray values,int initial){layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{id,1},name,values,initial));};
 choice("seq_length","Sequence length",{"16","32","64"},0);choice("seq_bank","Sequence bank",{"A","B","C","D"},0);choice("seq_mode","Sequence mode",{"Step","Arp up","Arp down","Arp up-down","Chord"},0);choice("seq_scale","Sequence scale",{"Chromatic","Major","Natural minor","Dorian","Major pentatonic"},0);choice("seq_division","Step duration",{"1/8","1/16","1/32"},1);
 for(const auto& spec:gboy::seq::controls){const bool integer=juce::String(spec.id)=="seq_root"||juce::String(spec.id)=="seq_key"||juce::String(spec.id)=="seq_octaves"||juce::String(spec.id)=="seq_seed";layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{spec.id,1},spec.name,juce::NormalisableRange<float>(spec.low,spec.high,integer?1.f:0.f),spec.initial));}
 for(int bank=0;bank<4;++bank)for(int step=0;step<64;++step){const auto initial=gboy::seq::defaultStep(bank,step);const auto title="Bank "+juce::String(bank+1)+" Step "+juce::String(step+1)+" ";
  layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{gboy::seq::stepId(bank,step,"enabled"),1},title+"Enabled",initial.enabled));
  layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{gboy::seq::stepId(bank,step,"pitch"),1},title+"Pitch",juce::NormalisableRange<float>(-24,24,1),float(initial.pitch)));
  layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{gboy::seq::stepId(bank,step,"velocity"),1},title+"Velocity",juce::NormalisableRange<float>(0,1),initial.velocity));
  layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{gboy::seq::stepId(bank,step,"gate"),1},title+"Gate",juce::NormalisableRange<float>(.05f,.95f),initial.gate));
  layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{gboy::seq::stepId(bank,step,"probability"),1},title+"Probability",juce::NormalisableRange<float>(0,1),1.f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{gboy::seq::stepId(bank,step,"ratchet"),1},title+"Ratchet",juce::NormalisableRange<float>(1,4,1),1.f));
 }
 return layout;
}
GBoyProcessor::GBoyProcessor(juce::File libraryHome):AudioProcessor(BusesProperties().withOutput("Output",juce::AudioChannelSet::stereo(),true)),state(*this,nullptr,"GBOY",layout()),libraryOwner(gboy::library::LibraryManager::shared(libraryHome)),library(*libraryOwner){
 defaults=state.copyState();master=state.getRawParameterValue("volume");keyboard.addListener(this);
 for(int layer=0;layer<4;++layer){
  auto& h=handles[size_t(layer)];
  for(size_t i=0;i<h.values.size();++i)h.values[i]=state.getRawParameterValue(gboy::parameterId(layer,gboy::layerSpecs[i].id));
  for(size_t i=0;i<3;++i)h.vocalValues[i]=state.getRawParameterValue(gboy::parameterId(layer,gboy::vocalSpecs[i].id));
  h.preserve=state.getRawParameterValue(gboy::parameterId(layer,"vPreserve"));h.reverse=state.getRawParameterValue(gboy::parameterId(layer,"vReverse"));h.slicing=state.getRawParameterValue(gboy::parameterId(layer,"vSlices"));h.effect=state.getRawParameterValue(gboy::parameterId(layer,"vEffect"));
  h.enabled=state.getRawParameterValue(gboy::parameterId(layer,"enabled"));h.generator=state.getRawParameterValue(gboy::parameterId(layer,"generator"));h.filter=state.getRawParameterValue(gboy::parameterId(layer,"filter"));
  for(int route=0;route<4;++route){auto& row=h.routes[size_t(route)];row[0]=state.getRawParameterValue(gboy::routeId(layer,route,"source"));row[1]=state.getRawParameterValue(gboy::routeId(layer,route,"target"));row[2]=state.getRawParameterValue(gboy::routeId(layer,route,"amount"));row[3]=state.getRawParameterValue(gboy::routeId(layer,route,"fxTarget"));}
  auto& slot=slots[size_t(layer)];slot.source="factory:"+juce::String(layer);slot.originalSource=slot.source;slot.original=std::make_unique<gboy::Sample>(gboy::factory(layer));slot.active=new gboy::Sample(*slot.original);slot.info.seconds=double(slot.active->left.size())/slot.active->rate;engine.layers[size_t(layer)].setSample(slot.active);
  slot.message=juce::StringArray{"CHIP PULSE","SOFT KEYS","FM BELL","ARCADE DRUM"}[layer];
  for(size_t i=0;i<128;++i)waveform[size_t(layer)][i].store(slot.active->left[i*40]);
 }
 const char* fxFields[]={"type","bypass","mix","a","b","c"};for(int rack=0;rack<5;++rack)for(int slot=0;slot<4;++slot)for(size_t field=0;field<6;++field)fxHandles[size_t(rack)][size_t(slot)].values[field]=state.getRawParameterValue(gboy::fx::parameterId(rack,slot,fxFields[field]));
 const char* flags[]={"enabled","sync","run","latch","chain"};const char* choices[]={"seq_length","seq_bank","seq_mode","seq_scale","seq_division"};for(size_t i=0;i<5;++i){seqFlags[i]=state.getRawParameterValue("seq_"+juce::String(flags[i]));seqChoices[i]=state.getRawParameterValue(choices[i]);}for(size_t i=0;i<6;++i)seqControls[i]=state.getRawParameterValue(gboy::seq::controls[i].id);
 for(int bank=0;bank<4;++bank)for(int step=0;step<64;++step)for(size_t field=0;field<6;++field)stepHandles[size_t(bank)][size_t(step)][field]=state.getRawParameterValue(gboy::seq::stepId(bank,step,gboy::seq::stepFields[field]));
 worker=std::thread([this]{loadLoop();});
}
GBoyProcessor::~GBoyProcessor(){keyboard.removeListener(this);quit=true;worker.join();delete previewPending.exchange(nullptr);delete previewRetired.exchange(nullptr);delete previewActive;for(auto& slot:slots){delete slot.pending.exchange(nullptr);delete slot.retired.exchange(nullptr);delete slot.active;}}
void GBoyProcessor::prepareToPlay(double rate,int){processingRate=std::isfinite(rate)&&rate>0?rate:44100;engine.prepare(processingRate);previewEngine.prepare(processingRate);previewEngine.setSample(previewActive?&previewActive->sample:nullptr);for(size_t i=0;i<4;++i)engine.layers[i].setSample(slots[i].active);keyboard.reset();internalBeat=0;sequencer.clearInput();lastManualRun=false;}
void GBoyProcessor::requestSample(juce::String source,int layer,int root,juce::String assetId){if(layer<0||layer>3)return;std::lock_guard lock(mutex);auto& slot=slots[size_t(layer)];slot.requested=std::move(source);slot.requestedRoot=juce::jlimit(0,127,root);slot.requestedAsset=std::move(assetId);slot.hasRequest=true;slot.message="LOADING...";}
juce::String GBoyProcessor::status(int layer){std::lock_guard lock(mutex);return slots[size_t(juce::jlimit(0,3,layer))].message;}
void GBoyProcessor::readParameters(){
 engine.volume=master->load();
 for(size_t layer=0;layer<4;++layer){auto& p=engine.layers[layer].params;const auto& h=handles[layer];auto get=[&](size_t i){return h.values[i]->load();};
  p.enabled=h.enabled->load()>.5f;p.generator=gboy::Generator(juce::jlimit(0,5,int(h.generator->load())));p.filter=juce::jlimit(0,3,int(h.filter->load()));
  p.attack=get(0);p.decay=get(1);p.sustain=get(2);p.release=get(3);p.level=get(4);p.pan=get(5);p.tune=get(6);p.cutoff=get(7);p.resonance=get(8);p.shape=get(9);p.ratio=get(10);p.position=get(11);p.grainSize=get(12);p.lfoRate=get(13);p.macro=get(14);
  for(size_t route=0;route<4;++route){const auto& row=h.routes[route];p.routes[route]={gboy::ModSource(juce::jlimit(0,6,int(row[0]->load()))),gboy::ModTarget(row[3]->load()>.5f?5+juce::jlimit(1,4,int(row[3]->load())):juce::jlimit(0,5,int(row[1]->load()))),row[2]->load()};}
 }
 for(size_t rack=0;rack<5;++rack)for(size_t slot=0;slot<4;++slot){const auto& values=fxHandles[rack][slot].values;auto& target=rack==4?engine.masterEffects.slots[slot].params:engine.layers[rack].effects.slots[slot].params;target={gboy::fx::Type(juce::jlimit(0,int(gboy::fx::Type::Count)-1,int(values[0]->load()))),values[1]->load()>.5f,values[2]->load(),values[3]->load(),values[4]->load(),values[5]->load()};}
 const auto config=sequenceSettings();if(config.enabled!=sequencer.configuration().enabled)engine.allOff();sequencer.configure(config);

}
gboy::vocal::Settings GBoyProcessor::vocalSettings(size_t layer) const {
 const auto& h=handles[layer];gboy::vocal::Settings settings;
 settings.pitch=h.vocalValues[0]->load();settings.stretch=h.vocalValues[1]->load();settings.formant=h.vocalValues[2]->load();
 if(std::abs(settings.pitch)<.0001f)settings.pitch=0;if(std::abs(settings.stretch-1)<.0001f)settings.stretch=1;if(std::abs(settings.formant)<.0001f)settings.formant=0;
 settings.preserve=h.preserve->load()>.5f;settings.reverse=h.reverse->load()>.5f;settings.slicing=juce::jlimit(0,3,int(h.slicing->load()));settings.effect=juce::jlimit(0,3,int(h.effect->load()));return settings;
}
bool GBoyProcessor::isBusy(int layer,bool includePublication) const {
 if(layer<0||layer>3)return false;const auto index=size_t(layer);const auto desired=vocalSettings(index);std::lock_guard lock(mutex);const auto& slot=slots[index];
 return slot.hasRequest||slot.rendering||slot.sourceDirty||(includePublication&&slot.pending.load()!=nullptr)||!(desired==slot.applied);
}
GBoyProcessor::SliceInfo GBoyProcessor::sliceInfo(int layer) const {std::lock_guard lock(mutex);return slots[size_t(juce::jlimit(0,3,layer))].info;}
void GBoyProcessor::loadLoop(){
 juce::AudioFormatManager formats;formats.registerBasicFormats();
 std::array<gboy::vocal::Settings,4> observed{};
 std::array<std::chrono::steady_clock::time_point,4> changed{};
 while(!quit.load()){
  delete previewRetired.exchange(nullptr);juce::String previewId;uint64_t generation=0;{std::lock_guard lock(mutex);previewId=std::move(previewRequest);previewRequest.clear();generation=previewRequestGeneration;}
  if(previewId.isNotEmpty())buildPreview(previewId,generation);
  for(size_t layer=0;layer<slots.size()&&!quit.load();++layer){
   auto& slot=slots[layer];delete slot.retired.exchange(nullptr);
   const auto desired=vocalSettings(layer);const auto now=std::chrono::steady_clock::now();
   if(!(desired==observed[layer])){observed[layer]=desired;changed[layer]=now;}
   juce::String next,assetId;int root=60;bool load=false,render=false;
   {std::lock_guard lock(mutex);if(slot.hasRequest){next=slot.requested;root=slot.requestedRoot;assetId=slot.requestedAsset;slot.hasRequest=false;load=true;}render=load||((slot.sourceDirty||!(desired==slot.applied))&&now-changed[layer]>=std::chrono::milliseconds(150));if(render)slot.rendering=true;}
   if(!render)continue;
   try{
    if(load){
     if(assetId.isNotEmpty()){for(int attempt=0;attempt<250&&!quit.load()&&!library.waitUntilIdle(20);++attempt){}const auto entry=library.find(assetId);if(entry.id.isNotEmpty()&&juce::File(entry.path).existsAsFile())next=entry.path;}
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
     source->root=root;slot.original=std::move(source);slot.originalSource=next;slot.originalAsset=assetId;{std::lock_guard lock(mutex);slot.sourceDirty=true;}
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
     delete slot.pending.exchange(sample.release());slot.source=slot.originalSource;slot.asset=slot.originalAsset;slot.publishedRoot=slot.original->root;slot.applied=desired;slot.sourceDirty=false;slot.info=info;
     slot.message=slot.source=="vocal:0"?"SYNTHETIC VOWEL PHRASE":slot.source.startsWith("factory:")?juce::StringArray{"CHIP PULSE","SOFT KEYS","FM BELL","ARCADE DRUM"}[slot.source.substring(8).getIntValue()]:juce::File(slot.source).getFileName();
    }
    slot.rendering=false;
   }catch(const std::exception& error){std::lock_guard lock(mutex);slot.applied=desired;slot.sourceDirty=false;slot.rendering=false;slot.message=error.what();}
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
 }
}
gboy::seq::Settings GBoyProcessor::sequenceSettings() const {
 gboy::seq::Settings next;next.enabled=seqFlags[0]->load()>.5f;next.sync=seqFlags[1]->load()>.5f;next.run=seqFlags[2]->load()>.5f;next.latch=seqFlags[3]->load()>.5f;next.chain=seqFlags[4]->load()>.5f;
 next.length=16<<juce::jlimit(0,2,int(seqChoices[0]->load()));next.bank=juce::jlimit(0,3,int(seqChoices[1]->load()));next.mode=juce::jlimit(0,4,int(seqChoices[2]->load()));next.scale=juce::jlimit(0,4,int(seqChoices[3]->load()));next.division=.5/std::exp2(double(juce::jlimit(0,2,int(seqChoices[4]->load()))));
 next.tempo=seqControls[0]->load();next.root=int(seqControls[1]->load());next.swing=seqControls[2]->load();next.key=int(seqControls[3]->load());next.octaves=int(seqControls[4]->load());next.seed=uint32_t(seqControls[5]->load());
 for(size_t bank=0;bank<4;++bank)for(size_t step=0;step<64;++step){const auto& values=stepHandles[bank][step];next.patterns[bank][step]={values[0]->load()>.5f,int(values[1]->load()),values[2]->load(),values[3]->load(),values[4]->load(),int(values[5]->load())};}
 return next;
}
void GBoyProcessor::inputNote(int note,float velocity,int channel){sequencer.input(note,velocity,channel);if(!sequencer.configuration().enabled){if(velocity>0)engine.noteOn(note,velocity,channel);else engine.noteOff(note,channel);}}
void GBoyProcessor::applyEvents(const gboy::seq::Events& events){for(int i=0;i<events.count;++i){const auto& event=events.values[size_t(i)];if(event.on)engine.noteOn(event.note,event.velocity,event.channel);else engine.noteOff(event.note,event.channel);}}
void GBoyProcessor::renderSegment(float* left,float* right,int count,double beat){
 if(count<=0)return;const double increment=clockTempo/(60*processingRate);engine.tempo=clockTempo;engine.beat=beat;
 if(!sequencer.configuration().enabled||!clockRunning){applyEvents(sequencer.tick(beat,clockRunning));engine.render(left,right,count);return;}
 int cursor=0;for(int i=0;i<count;++i){const auto events=sequencer.tick(beat+i*increment,true);if(events.count){engine.render(left+cursor,right+cursor,i-cursor);applyEvents(events);cursor=i;}}engine.render(left+cursor,right+cursor,count-cursor);
}
void GBoyProcessor::processBlock(juce::AudioBuffer<float>& buffer,juce::MidiBuffer& midi){
 const auto callbackStart=juce::Time::getHighResolutionTicks();
 juce::ScopedNoDenormals noDenormals;
 for(size_t layer=0;layer<4;++layer){auto& slot=slots[layer];if(slot.retired.load()==nullptr){if(auto* sample=slot.pending.exchange(nullptr)){auto* old=slot.active;slot.active=sample;engine.layers[layer].setSample(sample);slot.retired.store(old);}}}
 readParameters();const auto& config=sequencer.configuration();clockTempo=config.tempo;clockBeat=internalBeat;clockRunning=config.run;
 bool useHost=false;
 if(config.sync){if(auto* playhead=getPlayHead()){if(auto position=playhead->getPosition()){if(auto bpm=position->getBpm())if(std::isfinite(*bpm)&&*bpm>=20&&*bpm<=400)clockTempo=*bpm;if(auto ppq=position->getPpqPosition()){if(std::isfinite(*ppq)&&std::abs(*ppq)<1e9){useHost=true;clockRunning=position->getIsPlaying();if(clockRunning)clockBeat=*ppq;}}}}}
 const bool manualRun=!useHost&&config.enabled&&config.run;if(manualRun&&!lastManualRun){internalBeat=clockBeat=0;}lastManualRun=manualRun;hostSyncActive.store(useHost);
 int start1,size1,start2,size2;keyFifo.prepareToRead(256,start1,size1,start2,size2);
 for(int i=0;i<size1;++i){const auto& key=keyEvents[size_t(start1+i)];inputNote(key.note,key.velocity,key.channel);}for(int i=0;i<size2;++i){const auto& key=keyEvents[size_t(start2+i)];inputNote(key.note,key.velocity,key.channel);}keyFifo.finishedRead(size1+size2);
 if(panicRequested.exchange(false)){engine.allOff();engine.resetEffects();sequencer.clearInput();}
 buffer.clear();if(buffer.getNumChannels()<2)return;
 int cursor=0;const double increment=clockTempo/(60*processingRate);
 for(const auto meta:midi){if(meta.numBytes>3)continue;const int at=juce::jlimit(cursor,buffer.getNumSamples(),meta.samplePosition);renderSegment(buffer.getWritePointer(0)+cursor,buffer.getWritePointer(1)+cursor,at-cursor,clockBeat+cursor*increment);cursor=at;
  const auto message=meta.getMessage();if(message.isNoteOn())inputNote(message.getNoteNumber(),message.getFloatVelocity(),message.getChannel());else if(message.isNoteOff())inputNote(message.getNoteNumber(),0,message.getChannel());else if(message.isPitchWheel())engine.bend(message.getChannel(),float(message.getPitchWheelValue()-8192)/8192.f);else if(message.isController()&&message.getControllerNumber()==1)engine.wheel(message.getChannel(),float(message.getControllerValue())/127);else if(message.isAllNotesOff()||message.isAllSoundOff()){sequencer.clearInput(message.getChannel());engine.allOff(message.getChannel());if(message.isAllSoundOff())engine.resetEffects();}
 }
 renderSegment(buffer.getWritePointer(0)+cursor,buffer.getWritePointer(1)+cursor,buffer.getNumSamples()-cursor,clockBeat+cursor*increment);internalBeat+=buffer.getNumSamples()*increment;
 mixPreview(buffer);
 if(buffer.getNumSamples()>0){const double seconds=juce::Time::highResolutionTicksToSeconds(juce::Time::getHighResolutionTicks()-callbackStart);const float load=float(seconds*processingRate/buffer.getNumSamples());cpuLoad.store(cpuLoad.load()*.95f+load*.05f);cpuPeak.store(std::max(cpuPeak.load(),load));}int count=0;for(const auto& layer:engine.layers)count+=layer.activeVoices();voiceCount.store(count);
 playingStep.store(sequencer.step());playingBank.store(sequencer.bank());meter.store(buffer.getMagnitude(0,buffer.getNumSamples()));midi.clear();
}
void GBoyProcessor::getStateInformation(juce::MemoryBlock& block){
 auto tree=state.copyState();tree.setProperty("version",gboy::presets::stateVersion,nullptr);
 {std::lock_guard lock(mutex);for(int i=0;i<4;++i){const auto& slot=slots[size_t(i)];const auto suffix=juce::String(i+1);tree.setProperty("sampleSource"+suffix,slot.source,nullptr);tree.setProperty("sampleAsset"+suffix,slot.asset,nullptr);tree.setProperty("sampleRoot"+suffix,slot.publishedRoot,nullptr);}}
 auto xml=tree.createXml();copyXmlToBinary(*xml,block);
}
void GBoyProcessor::setStateInformation(const void* data,int size){restoreState(data,size);}
bool GBoyProcessor::restoreState(const void* data,int size){
 if(!data||size<=0||size>1024*1024)return false;auto xml=getXmlFromBinary(data,size);if(!xml)return false;auto migrated=gboy::presets::migrateState(*xml,defaults,state);if(!migrated.isValid())return false;
 const auto input=juce::ValueTree::fromXml(*xml);state.replaceState(migrated);
 for(int i=0;i<4;++i){const auto suffix=juce::String(i+1);auto fallback=i==0?input.getProperty("sampleSource","factory:0").toString():"factory:"+juce::String(i);const double root=double(input.getProperty("sampleRoot"+suffix,60));requestSample(input.getProperty("sampleSource"+suffix,fallback).toString(),i,std::isfinite(root)?int(std::clamp(root,0.,127.)):60,input.getProperty("sampleAsset"+suffix).toString());}
 panicRequested.store(true);return true;
}
juce::AudioProcessorEditor* GBoyProcessor::createEditor(){return new GBoyEditor(*this);}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new GBoyProcessor();}

void GBoyProcessor::handleNoteOn(juce::MidiKeyboardState*,int channel,int note,float velocity){int a,n,b,m;keyFifo.prepareToWrite(1,a,n,b,m);if(n){keyEvents[size_t(a)]={note,channel,velocity};keyFifo.finishedWrite(1);}else panicRequested.store(true);}
void GBoyProcessor::handleNoteOff(juce::MidiKeyboardState* k,int channel,int note,float){handleNoteOn(k,channel,note,0);}

void GBoyProcessor::loadHybridPreset(int preset){
 auto tree=defaults.createCopy();const auto recipe=gboy::presets::hybrid(juce::jlimit(0,9,preset));
 auto set=[&](int layer,const std::string& suffix,float value){auto node=tree.getChildWithProperty("id",juce::String(gboy::parameterId(layer,suffix)));if(node.isValid())node.setProperty("value",value,nullptr);};
 for(int layer=0;layer<4;++layer){const auto& p=recipe[size_t(layer)];set(layer,"enabled",p.enabled?1.f:0.f);set(layer,"generator",float(p.generator));set(layer,"filter",float(p.filter));const float values[]={p.attack,p.decay,p.sustain,p.release,p.level,p.pan,p.tune,p.cutoff,p.resonance,p.shape,p.ratio,p.position,p.grainSize,p.lfoRate,p.macro};for(size_t i=0;i<15;++i)set(layer,gboy::layerSpecs[i].id,values[i]);for(int row=0;row<4;++row){const auto& route=p.routes[size_t(row)];set(layer,"r"+std::to_string(row+1)+"_source",float(route.source));set(layer,"r"+std::to_string(row+1)+"_target",float(route.target));set(layer,"r"+std::to_string(row+1)+"_amount",route.amount);}}
 state.replaceState(tree);for(int layer=0;layer<4;++layer)requestSample("factory:"+juce::String(layer),layer);panicRequested.store(true);
}
bool GBoyProcessor::exportSlices(const juce::File& file,int layer) const {
 if(isBusy(layer,false))return false;const auto info=sliceInfo(layer);if(info.count<1)return false;
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

void GBoyProcessor::stopSequence(){for(const auto* id:{"seq_enabled","seq_run"})if(auto* parameter=state.getParameter(id))parameter->setValueNotifyingHost(0);panicRequested.store(true);}
void GBoyProcessor::loadRackPreset(int rack,int preset){
 if(rack<0||rack>4)return;std::array<gboy::fx::Parameters,4> slots{};
 using Type=gboy::fx::Type;
 if(preset==1){slots[0]={Type::Tape,false,.4f,.25f,.2f,.65f};slots[1]={Type::Chorus,false,.25f,.25f,.35f,.2f};slots[2]={Type::Reverb,false,.25f,.65f,.65f,.75f};}
 else if(preset==2){slots[0]={Type::Saturation,false,.4f,.5f,.6f,.5f};slots[1]={Type::Bitcrusher,false,.3f,.45f,0,0};}
 else if(preset==3){slots[0]={Type::Shimmer,false,.35f,.7f,.8f,.65f};slots[1]={Type::Delay,false,.25f,.5f,.4f,.75f};}
 else if(preset==4){slots[0]={Type::PingPong,false,.35f,.65f,.6f,.7f};slots[1]={Type::Duck,false,1,.5f,.8f,.4f};slots[2]={Type::Limiter,false,1,.95f,.4f,0};}
 else if(preset==5){slots[0]={Type::Convolution,false,.4f,.65f,.6f,.8f};slots[1]={Type::Compressor,false,.6f,.7f,.1f,.4f};}
 const char* fields[]={"type","bypass","mix","a","b","c"};
 for(int slot=0;slot<4;++slot){const auto& config=slots[size_t(slot)];const float values[]={float(config.type),config.bypass?1.f:0.f,config.mix,config.a,config.b,config.c};for(size_t i=0;i<6;++i)if(auto* parameter=state.getParameter(gboy::fx::parameterId(rack,slot,fields[i])))parameter->setValueNotifyingHost(parameter->convertTo0to1(values[i]));}
}
void GBoyProcessor::moveEffect(int rack,int slot,int direction){
 const int other=slot+direction;if(rack<0||rack>4||slot<0||slot>3||other<0||other>3)return;const char* fields[]={"type","bypass","mix","a","b","c"};
 std::array<float,6> a{},b{};for(size_t i=0;i<6;++i){a[i]=fxHandles[size_t(rack)][size_t(slot)].values[i]->load();b[i]=fxHandles[size_t(rack)][size_t(other)].values[i]->load();}
 for(size_t i=0;i<6;++i){auto* first=state.getParameter(gboy::fx::parameterId(rack,slot,fields[i]));auto* second=state.getParameter(gboy::fx::parameterId(rack,other,fields[i]));first->setValueNotifyingHost(first->convertTo0to1(b[i]));second->setValueNotifyingHost(second->convertTo0to1(a[i]));}
}
bool GBoyProcessor::saveRack(const juce::File& file,int rack) const {
 if(rack<0||rack>4)return false;juce::XmlElement root("GBFX");root.setAttribute("version",1);const char* fields[]={"type","bypass","mix","a","b","c"};
 for(int slot=0;slot<4;++slot){auto* child=root.createNewChildElement("SLOT");child->setAttribute("index",slot);for(size_t field=0;field<6;++field)child->setAttribute(fields[field],double(fxHandles[size_t(rack)][size_t(slot)].values[field]->load()));}
 return file.replaceWithText(root.toString());
}
bool GBoyProcessor::loadRack(const juce::File& file,int rack){
 if(rack<0||rack>4||file.getSize()>1024*1024)return false;auto xml=juce::XmlDocument::parse(file);if(!xml||!xml->hasTagName("GBFX")||xml->getIntAttribute("version")!=1||xml->getNumChildElements()!=4)return false;
 const char* fields[]={"type","bypass","mix","a","b","c"};std::array<std::array<float,6>,4> values{};std::array<bool,4> found{};
 for(auto* child=xml->getFirstChildElement();child;child=child->getNextElement()){const int index=child->getIntAttribute("index",-1);if(!child->hasTagName("SLOT")||index<0||index>3||found[size_t(index)])return false;found[size_t(index)]=true;for(size_t field=0;field<6;++field){if(!child->hasAttribute(fields[field]))return false;const float value=float(child->getDoubleAttribute(fields[field]));if(!std::isfinite(value)||value<0||value>(field==0?float(int(gboy::fx::Type::Count)-1):1.f))return false;values[size_t(index)][field]=value;}}
 for(int slot=0;slot<4;++slot)for(size_t field=0;field<6;++field){auto* parameter=state.getParameter(gboy::fx::parameterId(rack,slot,fields[field]));parameter->setValueNotifyingHost(parameter->convertTo0to1(values[size_t(slot)][field]));}return true;
}
void GBoyProcessor::loadSequencePreset(int preset){
 auto set=[&](const std::string& id,float value){if(auto* parameter=state.getParameter(id))parameter->setValueNotifyingHost(parameter->convertTo0to1(value));};
 set("seq_mode",preset==1?1.f:preset==3?4.f:0.f);set("seq_length",0);set("seq_division",1);set("seq_swing",preset==2?.2f:0.f);set("seq_scale",preset==1?2.f:1.f);set("seq_chain",0);set("seq_bank",0);
 for(int bank=0;bank<4;++bank)for(int index=0;index<64;++index){auto step=gboy::seq::defaultStep(bank,index);if(preset==1){step.pitch=0;step.gate=.4f;step.enabled=true;}else if(preset==2){step.pitch=index%4==0?0:7;step.enabled=index%8!=5;step.probability=index%4==0?1.f:.7f;step.ratchet=index%8==6?3:1;step.gate=.3f;}else if(preset==3){step.pitch=index%8<4?0:5;step.enabled=index%4==0;step.gate=.9f;}const float values[]={step.enabled?1.f:0.f,float(step.pitch),step.velocity,step.gate,step.probability,float(step.ratchet)};for(size_t field=0;field<6;++field)set(gboy::seq::stepId(bank,index,gboy::seq::stepFields[field]),values[field]);}
}
void GBoyProcessor::randomizePattern(int bank){
 if(bank<0||bank>3)return;auto set=[&](const std::string& id,float value){if(auto* parameter=state.getParameter(id))parameter->setValueNotifyingHost(parameter->convertTo0to1(value));};uint32_t seed=uint32_t(seqControls[5]->load())+1;if(seed>1000000)seed=1;set("seq_seed",float(seed));auto random=[&](){seed=gboy::seq::hash(seed+0x9e3779b9u);return float(double(seed)/4294967295.);};
 for(int index=0;index<64;++index){const float values[]={(index==0||random()>.3f)?1.f:0.f,float(int(random()*25)-12),.4f+random()*.6f,.2f+random()*.7f,.6f+random()*.4f,float(1+int(random()*3.99f))};for(size_t field=0;field<6;++field)set(gboy::seq::stepId(bank,index,gboy::seq::stepFields[field]),values[field]);}
}
bool GBoyProcessor::exportSequence(const juce::File& file) const {
 auto settings=sequenceSettings();settings.enabled=true;settings.sync=false;settings.run=true;gboy::seq::Sequencer exportEngine;exportEngine.configure(settings);if(settings.mode>=1&&settings.mode<=3)for(int interval:{0,4,7})exportEngine.input(std::min(127,settings.root+interval),1);
 juce::MidiMessageSequence track;auto tempo=juce::MidiMessage::tempoMetaEvent(int(60000000/settings.tempo));tempo.setTimeStamp(0);track.addEvent(tempo);
 auto append=[&](const gboy::seq::Events& events,int tick){for(int i=0;i<events.count;++i){const auto& event=events.values[size_t(i)];auto message=event.on?juce::MidiMessage::noteOn(event.channel,event.note,event.velocity):juce::MidiMessage::noteOff(event.channel,event.note);message.setTimeStamp(tick);track.addEvent(message);}};
 const double beats=settings.length*settings.division*(settings.chain?4:1);const int ticks=int(std::llround(beats*960));for(int tick=0;tick<ticks;++tick)append(exportEngine.tick(double(tick)/960,true),tick);append(exportEngine.tick(beats,false),ticks);track.updateMatchedPairs();
 juce::MidiFile midi;midi.setTicksPerQuarterNote(960);midi.addTrack(track);auto output=file.createOutputStream();if(!output||!output->setPosition(0)||output->truncate().failed())return false;return midi.writeTo(*output);
}

double GBoyProcessor::getTailLengthSeconds() const {
 auto rackTail=[&](size_t rack){double total=0;for(const auto& slot:fxHandles[rack]){const auto& p=slot.values;const auto type=gboy::fx::Type(juce::jlimit(0,int(gboy::fx::Type::Count)-1,int(p[0]->load())));if(type==gboy::fx::Type::Off||p[1]->load()>.5f)continue;const float a=gboy::fx::physical(type,0,p[3]->load()),b=gboy::fx::physical(type,1,p[4]->load()),c=gboy::fx::physical(type,2,p[5]->load());double delay=0,feedback=0;
  if(type==gboy::fx::Type::Delay||type==gboy::fx::Type::PingPong){delay=a*.001;feedback=b;}
  else if(type==gboy::fx::Type::Reverb){delay=a*.0023;feedback=b;}
  else if(type==gboy::fx::Type::Shimmer){delay=b*.0023+.04;feedback=c;}
  else if(type==gboy::fx::Type::Resonator){delay=1/a;feedback=b;}
  else if(type==gboy::fx::Type::Chorus||type==gboy::fx::Type::Flanger){delay=.03;feedback=c;}
  if(delay>0)total+=delay*(1+std::log(1e-5)/std::log(std::clamp(feedback,.001,.95)));
  else total+=type==gboy::fx::Type::Convolution?a*.001+.2:type==gboy::fx::Type::EQ||type==gboy::fx::Type::FilterSweep?2.:.1;
 }return total;};
 double layers=0;for(size_t layer=0;layer<4;++layer)layers=std::max(layers,double(handles[layer].values[3]->load())+rackTail(layer));return std::clamp(layers+rackTail(4)+1,1.,600.);
}

bool GBoyProcessor::loadLibraryEntry(const juce::String& id,int layer){
 const auto entry=library.find(id);if(entry.id.isEmpty()||entry.missing)return false;
 if(entry.action=="sample"){requestSample(entry.factoryIndex==4?"vocal:0":"factory:"+juce::String(entry.factoryIndex),layer,entry.root);return true;}
 if(entry.action=="hybrid"){loadHybridPreset(entry.factoryIndex);return true;}
 if(entry.action=="vocal"){loadVocalPreset(entry.factoryIndex);return true;}
 if(entry.action=="rack"){loadRackPreset(layer,entry.factoryIndex);return true;}
 if(entry.action=="pattern"){loadSequencePreset(entry.factoryIndex);return true;}
 if(entry.action!="file")return false;
 if(entry.kind=="sample"){requestSample(entry.path,layer,entry.root,entry.id);return true;}
 if(entry.kind=="rack")return loadRack(juce::File(entry.path),layer);
 juce::MemoryBlock block;const juce::File file(entry.path);if(file.getSize()>1024*1024||!file.loadFileAsData(block))return false;auto xml=getXmlFromBinary(block.getData(),int(block.getSize()));if(!xml||!xml->hasTagName("GBOY"))return false;
 // Pack exports use relative references; bind them to this installed library's stable IDs.
 const auto snapshot=library.snapshot();for(int i=1;i<=4;++i){const auto suffix=juce::String(i);const auto relative=xml->getStringAttribute("sampleRelative"+suffix);if(relative.isNotEmpty()){bool found=false;for(const auto& sample:snapshot.entries)if(sample.libraryId==entry.libraryId&&sample.relative==relative&&sample.kind=="sample"){xml->setAttribute("sampleSource"+suffix,sample.path);xml->setAttribute("sampleAsset"+suffix,sample.id);found=true;break;}if(!found)return false;}}
 copyXmlToBinary(*xml,block);return restoreState(block.getData(),int(block.getSize()));
}
bool GBoyProcessor::saveLibraryPatch(juce::File file){juce::MemoryBlock block;getStateInformation(block);file=file.withFileExtension("gboy");if(!file.replaceWithData(block.getData(),block.getSize()))return false;library.importFiles({file});return true;}
void GBoyProcessor::previewLibraryEntry(const juce::String& id){std::lock_guard lock(mutex);previewRequest=id;previewRequestGeneration=++previewGeneration;previewBusy.store(true);previewStop.store(true);}
void GBoyProcessor::stopLibraryPreview(){std::lock_guard lock(mutex);previewRequest.clear();++previewGeneration;delete previewPending.exchange(nullptr);previewStop.store(true);previewBusy.store(false);}
void GBoyProcessor::buildPreview(const juce::String& id,uint64_t generation){
 try{const auto entry=library.find(id);if(entry.id.isEmpty())throw std::runtime_error("No preview entry");auto sample=std::make_unique<gboy::Sample>();
  if(entry.action=="sample")*sample=entry.factoryIndex==4?gboy::vocal::syntheticPhrase():gboy::factory(entry.factoryIndex);
  else if(entry.action=="hybrid"){
   auto renderer=std::make_unique<gboy::HybridEngine>();renderer->prepare(44100);const auto recipe=gboy::presets::hybrid(entry.factoryIndex);std::array<gboy::Sample,4> sources;for(size_t i=0;i<4;++i){sources[i]=gboy::factory(int(i));renderer->layers[i].setSample(&sources[i]);renderer->layers[i].params=recipe[i];}sample->left.resize(44100*3);sample->right.resize(sample->left.size());renderer->noteOn(60,.85f);renderer->render(sample->left.data(),sample->right.data(),44100);renderer->noteOff(60);renderer->render(sample->left.data()+44100,sample->right.data()+44100,44100*2);
  }else if(entry.action=="vocal"){
   auto phrase=gboy::vocal::syntheticPhrase();gboy::vocal::Settings settings;if(entry.factoryIndex==0){settings.slicing=3;settings.pitch=3;settings.effect=2;}else if(entry.factoryIndex==1){settings.reverse=true;settings.stretch=1.6f;settings.formant=-4;}else if(entry.factoryIndex==2){settings.slicing=1;settings.effect=1;}else settings.stretch=1.25f;*sample=gboy::vocal::render(phrase,settings,&quit);
  }else{
   juce::File file(entry.path);if(entry.kind!="sample"){const auto snapshot=library.snapshot();juce::String folder;for(const auto& root:snapshot.roots)if(root.id==entry.libraryId)folder=root.path;if(entry.preview.isEmpty()||folder.isEmpty())throw std::runtime_error("No supplied preview");file=juce::File(folder).getChildFile(entry.preview);}
   juce::AudioFormatManager formats;formats.registerBasicFormats();std::unique_ptr<juce::AudioFormatReader> reader(formats.createReaderFor(file));if(!reader||!std::isfinite(reader->sampleRate)||reader->sampleRate<8000||reader->sampleRate>192000||reader->lengthInSamples<2)throw std::runtime_error("Invalid preview");const int frames=int(std::min(reader->lengthInSamples,juce::int64(reader->sampleRate*3)));juce::AudioBuffer<float> buffer(2,frames);if(!reader->read(&buffer,0,frames,0,true,true))throw std::runtime_error("Cannot decode preview");sample->rate=reader->sampleRate;sample->left.assign(buffer.getReadPointer(0),buffer.getReadPointer(0)+frames);const auto* right=buffer.getReadPointer(reader->numChannels>1?1:0);sample->right.assign(right,right+frames);
  }
  const size_t count=std::min(sample->left.size(),size_t(sample->rate*3));sample->left.resize(count);sample->right.resize(count);sample->root=entry.kind=="sample"?entry.root:60;double peak=0;for(size_t i=0;i<count;++i){for(auto* data:{&sample->left,&sample->right}){auto& value=(*data)[i];value=std::isfinite(value)?value:0;peak=std::max(peak,double(std::abs(value)));}}
  const float gain=float(std::min(8.,.7/std::max(.001,peak)));for(size_t i=0;i<count;++i){const float fade=float(std::min(1.,double(count-i)/(sample->rate*.04)));sample->left[i]*=gain*fade;sample->right[i]*=gain*fade;}
  std::lock_guard lock(mutex);if(generation==previewGeneration&&!quit){delete previewPending.exchange(new PreviewBuffer{std::move(*sample),generation});previewBusy.store(false);}
 }catch(...){std::lock_guard lock(mutex);if(generation==previewGeneration)previewBusy.store(false);}
}
void GBoyProcessor::mixPreview(juce::AudioBuffer<float>& buffer){
 if(isNonRealtime()){++previewGeneration;previewBusy.store(false);previewEngine.allOff();if(previewRetired.load()==nullptr)if(auto* canceled=previewPending.exchange(nullptr))previewRetired.store(canceled);return;}
 if(previewStop.exchange(false))previewEngine.allOff();if(previewRetired.load()==nullptr)if(auto* next=previewPending.exchange(nullptr)){if(next->generation!=previewGeneration.load()){previewRetired.store(next);return;}auto* old=previewActive;previewActive=next;previewEngine.setSample(&next->sample);previewRetired.store(old);previewEngine.params.volume=.8f;previewEngine.params.attack=.003f;previewEngine.params.release=.04f;previewEngine.params.sustain=1;previewEngine.noteOn(next->sample.root,1);}
 std::array<float,256> left{},right{};for(int offset=0;offset<buffer.getNumSamples();offset+=256){const int count=std::min(256,buffer.getNumSamples()-offset);previewEngine.render(left.data(),right.data(),count);for(int i=0;i<count;++i){buffer.addSample(0,offset+i,left[size_t(i)]);buffer.addSample(1,offset+i,right[size_t(i)]);}}
}
