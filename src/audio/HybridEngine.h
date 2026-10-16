#pragma once
#include "Engine.h"
#include "../synth/Oscillators.h"
#include "../granular/Grains.h"
#include "../effects/StateVariableFilter.h"
#include "../modulation/Matrix.h"
#include "../effects/EffectRack.h"
namespace gboy {
enum class Generator { Sample,Analog,Wavetable,FM,Granular,Vocal };
struct LayerParameters {
 bool enabled=true;Generator generator=Generator::Sample;
 float attack=.01f,decay=.15f,sustain=.7f,release=.25f;
 float level=.8f,pan=0,tune=0,cutoff=18000,resonance=.1f;
 int filter=0;
 float shape=.5f,ratio=2,position=.2f,grainSize=.08f,lfoRate=2,macro=0;
 std::array<ModRoute,4> routes{};
};
class LayerEngine {
 struct Voice {
  double pos=0,phase=0,modPhase=0,frequency=440,increment=.01,step=1;
  float env=0,velocity=0,releaseStep=0,pan=0,level=1,shape=.5f,position=.2f,cutoff=18000,leftPan=.70710678f,rightPan=.70710678f;
  double begin=0,end=0;bool sliced=false;
  int note=60,channel=1,stage=0,control=0;uint64_t age=0;
  Grains grains;std::array<StateVariableFilter,2> filter;
 };
 std::array<Voice,64> voices{};
 const Sample* sample=nullptr;const Wavetables* tables=nullptr;
 double rate=44100,lfoPhase=0;float smoothing=.03f,layerGain=0;uint64_t age=0;
 float lastVelocity=0;int lastNote=60,lastChannel=1;
 Generator lastGenerator=Generator::Sample;
 std::array<float,16> wheels{},bends{};
 void control(Voice& v,float lfo){
  const auto mods=evaluate(params.routes,{0,lfo,v.env,v.velocity,wheels[size_t(v.channel-1)],float(v.note-60)/60.f,params.macro});
  const float semitones=std::clamp(params.tune+mods[0]*12+bends[size_t(v.channel-1)]*2,-60.f,60.f);
  const double frequency=440*std::exp2((v.note-69+double(semitones))/12);
  const double targetIncrement=std::clamp(frequency/rate,0.000001,.45);
  const double targetStep=sample?sample->rate/rate*std::exp2(((v.sliced?0:v.note-sample->root)+double(semitones))/12):1;
  // Control updates every 16 samples; a 5ms smoother limits automation discontinuities.
  v.increment+=(targetIncrement-v.increment)*smoothing;v.step+=(targetStep-v.step)*smoothing;
  v.pan+=(std::clamp(params.pan+mods[3],-1.f,1.f)-v.pan)*smoothing;
  v.level+=(std::clamp(1+mods[2],0.f,2.f)-v.level)*smoothing;
  v.shape+=(std::clamp(params.shape+mods[5],0.f,1.f)-v.shape)*smoothing;
  v.position+=(std::clamp(params.position+mods[4],0.f,1.f)-v.position)*smoothing;
  const float cutoff=std::clamp(params.cutoff*std::exp2(std::clamp(mods[1]*4,-8.f,8.f)),20.f,float(rate*.45));
  v.cutoff+=(cutoff-v.cutoff)*smoothing;
  v.leftPan=std::sqrt((1-v.pan)*.5f);v.rightPan=std::sqrt((1+v.pan)*.5f);
  if(params.filter!=0){v.filter[0].configure(v.cutoff,params.resonance,rate);v.filter[1].copyCoefficientsFrom(v.filter[0]);}
 }
public:
 LayerParameters params;
 fx::Rack effects;
 std::array<float,4> effectModulation() const {float envelope=0;for(const auto& voice:voices)if(voice.stage)envelope=std::max(envelope,voice.env);const auto values=evaluate(params.routes,{0,float(std::sin(tau*lfoPhase)),envelope,lastVelocity,wheels[size_t(lastChannel-1)],float(lastNote-60)/60.f,params.macro});return {values[6],values[7],values[8],values[9]};}

 void prepare(double r,const Wavetables& wt){rate=r;tables=&wt;effects.prepare(rate);smoothing=float(1-std::exp(-16/(.005*rate)));reset();wheels={};bends={};}
 void reset(){for(auto& v:voices)v={};lfoPhase=0;layerGain=0;lastGenerator=params.generator;}
 void setSample(const Sample* s){sample=s;reset();}
 void wheel(int channel,float value){if(channel>=1&&channel<=16)wheels[size_t(channel-1)]=std::clamp(value,0.f,1.f);}
 void bend(int channel,float value){if(channel>=1&&channel<=16)bends[size_t(channel-1)]=std::clamp(value,-1.f,1.f);}
 int activeVoices() const {int n=0;for(const auto& v:voices)n+=v.stage!=0;return n;}
 void noteOn(int note,float velocity,int channel=1){
  if(!params.enabled||channel<1||channel>16)return;
  if((params.generator==Generator::Sample||params.generator==Generator::Granular||params.generator==Generator::Vocal)&&(!sample||sample->left.size()<2||sample->left.size()!=sample->right.size()))return;
  if(params.generator==Generator::Vocal&&sample->sliceCount>0&&(note<60||note>=60+sample->sliceCount))return;
  if(lastGenerator!=params.generator){reset();}
  lastVelocity=velocity;lastNote=note;lastChannel=channel;
  auto* selected=&voices[0];
  for(auto& v:voices){if(v.stage==0){selected=&v;break;}if(v.age<selected->age)selected=&v;}
  *selected={};auto& v=*selected;v.note=std::clamp(note,0,127);v.channel=channel;v.velocity=std::clamp(velocity,0.f,1.f);v.stage=1;v.age=++age;
  v.end=sample?double(sample->left.size()):0;
  if(params.generator==Generator::Vocal&&sample->sliceCount>0){const size_t index=size_t(note-60);v.begin=double(sample->sliceEdges[index]);v.end=double(sample->sliceEdges[index+1]);v.pos=v.begin;v.sliced=true;}
  v.frequency=440*std::exp2((v.note-69+double(params.tune))/12);v.increment=std::clamp(v.frequency/rate,.000001,.45);
  v.step=sample?sample->rate/rate*std::exp2(((v.sliced?0:v.note-sample->root)+double(params.tune))/12):1;
  v.pan=params.pan;v.cutoff=params.cutoff;v.shape=params.shape;v.position=params.position;
 }
 void noteOff(int note,int channel=1){for(auto& v:voices)if(v.stage&&v.note==note&&v.channel==channel&&v.stage!=4){v.stage=4;v.releaseStep=v.env/(std::max(.001f,params.release)*float(rate));}}
 void allOff(int channel=0){for(auto& v:voices)if(channel==0||v.channel==channel)v.stage=0;}
 void render(float* l,float* r,int count){
  if(lastGenerator!=params.generator)reset();
  if(!params.enabled)allOff();
  const float gainCoefficient=float(1-std::exp(-1/(.01*rate)));
  const float aStep=1/(std::max(.001f,params.attack)*float(rate));
  const float dStep=(1-params.sustain)/(std::max(.001f,params.decay)*float(rate));
  for(int i=0;i<count;++i){
   float left=0,right=0;const float lfo=float(std::sin(tau*lfoPhase));lfoPhase=wrap(lfoPhase+std::clamp(double(params.lfoRate),.01,30.)/rate);
   for(auto& v:voices){
    if(!v.stage)continue;
    if(v.stage==1){v.env+=aStep;if(v.env>=1){v.env=1;v.stage=2;}}
    else if(v.stage==2){v.env-=dStep;if(v.env<=params.sustain){v.env=params.sustain;v.stage=3;}}
    else if(v.stage==3)v.env+=(params.sustain-v.env)*gainCoefficient;
    else{v.env-=v.releaseStep;if(v.env<=0){v.stage=0;continue;}}
    if(v.control--<=0){control(v,lfo);v.control=15;}
    float x=0,y=0;
    switch(params.generator){
     case Generator::Vocal:
     case Generator::Sample:
      if(!sample||v.pos+1>=v.end){v.stage=0;continue;}
      x=sampleAt(sample->left,v.pos);y=sampleAt(sample->right,v.pos);
      if(v.sliced){const double fade=std::max(1.,std::min(sample->rate*.003,(v.end-v.begin)*.25));const float edge=float(std::clamp(std::min((v.pos-v.begin)/fade,(v.end-1-v.pos)/fade),0.,1.));x*=edge;y*=edge;}
      v.pos+=v.step;break;
     case Generator::Analog:x=y=analog(v.phase,v.increment,v.shape);break;
     case Generator::Wavetable:x=y=tables?tables->read(v.phase,v.increment,v.shape):0;break;
     case Generator::FM:{
      // Two sine operators; ratio is clamped below Nyquist. FM is not oversampled.
      const double ratio=std::clamp(double(params.ratio),.25,std::min(8.,.45/v.increment));
      x=y=float(std::sin(tau*v.phase+double(v.shape)*6*std::sin(tau*v.modPhase)));
      v.modPhase=wrap(v.modPhase+v.increment*ratio);break;
     }
     case Generator::Granular:{auto frame=v.grains.process(sample,v.step,v.position,std::clamp(params.grainSize,.01f,.25f),rate);x=frame[0];y=frame[1];break;}
    }
    v.phase=wrap(v.phase+v.increment);
    x=v.filter[0].process(x,params.filter);y=v.filter[1].process(y,params.filter);
    const float amplitude=v.env*v.velocity*v.level*.16f;
    left+=x*amplitude*v.leftPan;right+=y*amplitude*v.rightPan;
   }
   layerGain+=(std::clamp(params.level,0.f,1.f)-layerGain)*gainCoefficient;
   l[i]=left*layerGain;r[i]=right*layerGain;
  }
 }
};
class HybridEngine {
 // Tables precede layers and outlive all references to them.
 Wavetables tables;
 float gain=0,gainCoefficient=.002f;double renderRate=44100;
 std::array<float,256> scratchL{},scratchR{};
public:
 std::array<LayerEngine,4> layers;
 float volume=.65f;fx::Rack masterEffects;double tempo=120,beat=0;
 HybridEngine(){for(size_t i=1;i<4;++i)layers[i].params.enabled=false;prepare(44100);}
 void prepare(double rate){renderRate=rate;masterEffects.prepare(rate);gain=0;gainCoefficient=float(1-std::exp(-1/(.01*rate)));for(auto& layer:layers)layer.prepare(rate,tables);}
 void resetEffects(){masterEffects.reset();for(auto& layer:layers)layer.effects.reset();}
 void noteOn(int note,float velocity,int channel=1){for(auto& layer:layers)layer.noteOn(note,velocity,channel);}
 void noteOff(int note,int channel=1){for(auto& layer:layers)layer.noteOff(note,channel);}
 void allOff(int channel=0){for(auto& layer:layers)layer.allOff(channel);}
 void wheel(int channel,float value){for(auto& layer:layers)layer.wheel(channel,value);}
 void bend(int channel,float value){for(auto& layer:layers)layer.bend(channel,value);}
 void render(float* l,float* r,int count){
  for(int offset=0;offset<count;offset+=256){const int n=std::min(256,count-offset);
   std::fill_n(l+offset,n,0.f);std::fill_n(r+offset,n,0.f);
   float masterMixMod=0,masterAmountMod=0;
   for(auto& layer:layers){layer.render(scratchL.data(),scratchR.data(),n);const auto mod=layer.effectModulation();layer.effects.process(scratchL.data(),scratchR.data(),n,tempo,beat,mod[0],mod[1]);masterMixMod+=mod[2];masterAmountMod+=mod[3];for(int i=0;i<n;++i){l[offset+i]+=scratchL[size_t(i)];r[offset+i]+=scratchR[size_t(i)];}}
   for(int i=0;i<n;++i){gain+=(volume-gain)*gainCoefficient;l[offset+i]*=gain;r[offset+i]*=gain;}
   masterEffects.process(l+offset,r+offset,n,tempo,beat,std::clamp(masterMixMod,-1.f,1.f),std::clamp(masterAmountMod,-1.f,1.f));
   for(int i=0;i<n;++i){l[offset+i]=std::tanh(l[offset+i]);r[offset+i]=std::tanh(r[offset+i]);}
   beat+=double(n)*tempo/(60*renderRate);
  }
 }
};
}
