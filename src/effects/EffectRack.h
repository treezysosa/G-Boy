#pragma once
#include "EffectSpecs.h"
#include "StateVariableFilter.h"
#include "../synth/Oscillators.h"
#include <vector>
#include <cstdint>
namespace gboy::fx {
class DelayLine {
 std::vector<float> data;size_t position=0,valid=0;
public:
 void prepare(double rate){data.assign(size_t(rate*2)+8,0);reset();}
 void reset(){position=valid=0;}
 float read(double delay) const {
  if(data.empty()||delay<1||delay>double(valid)||delay>=double(data.size()-2))return 0;
  double index=double(position)-delay;if(index<0)index+=double(data.size());const size_t a=size_t(index),b=(a+1)%data.size();return data[a]+float(index-double(a))*(data[b]-data[a]);
 }
 void push(float value){data[position]=value;position=(position+1)%data.size();valid=std::min(valid+1,data.size());}
};
class Slot {
 double rate=44100,phase=0,shiftPhase=0,holdPhase=0;
 Type lastType=Type::Off;float mix=0,smooth=.002f;
 std::array<float,3> current{.5f,.5f,.5f},value{};
 std::array<DelayLine,2> line;
 std::array<StateVariableFilter,2> low;
 std::array<std::array<float,4>,2> allpass{};
 std::array<float,2> feedback{},held{},previousHeld{};
 std::array<size_t,64> irDelay{};std::array<float,64> irGain{};
 float detector=0,slowDetector=0,compressGain=1,eqB0=1,eqB1=0,eqB2=0,eqA1=0,eqA2=0;
 std::array<std::array<float,2>,2> eqState{};
 uint32_t random=1777;int control=0;
 float noise(){random=random*1664525u+1013904223u;return float(double(random)/4294967295.*2-1);}
 void configure(){
  for(size_t i=0;i<3;++i)value[i]=physical(params.type,i,current[i]);
  float cutoff=18000;
  switch(params.type){
   case Type::EQ:{const float frequency=std::clamp(value[0],20.f,float(rate*.45)),A=std::pow(10.f,value[1]/40),w=float(tau)*frequency/float(rate),alpha=std::sin(w)/(2*value[2]),norm=1+alpha/A;eqB0=(1+alpha*A)/norm;eqB1=-2*std::cos(w)/norm;eqB2=(1-alpha*A)/norm;eqA1=eqB1;eqA2=(1-alpha/A)/norm;break;}
   case Type::Saturation:cutoff=value[1];break;
   case Type::Tape:case Type::Delay:case Type::PingPong:case Type::Reverb:case Type::Convolution:case Type::Resonator:cutoff=value[2];break;
   case Type::Vinyl:cutoff=18000-14000*value[2];break;
   case Type::Shimmer:cutoff=7000;break;
   case Type::FilterSweep:cutoff=value[0]*std::exp2(value[1]*float(std::sin(tau*phase)));break;
   default:break;
  }
  for(auto& filter:low)filter.configure(cutoff,params.type==Type::FilterSweep?.4f:0.f,rate);
  if(params.type==Type::Convolution){uint32_t seed=72;for(size_t i=0;i<64;++i){seed=seed*1664525u+1013904223u;const double time=(double(i)+1)/64*value[0]*.001;irDelay[i]=size_t(std::max(1.,time*rate));irGain[i]=float((double(seed)/4294967295.*2-1)*std::exp(-time/(value[1]*.001))*.18);}}
 }
 std::array<float,2> shifted(float pitch){
  const double window=rate*.04;shiftPhase=wrap(shiftPhase+(double(pitch)-1)/window);
  const double other=wrap(shiftPhase+.5),weight=.5-.5*std::cos(tau*shiftPhase);
  std::array<float,2> out{};for(size_t channel=0;channel<2;++channel)out[channel]=float(line[channel].read(2+(1-shiftPhase)*window)*weight+line[channel].read(2+(1-other)*window)*(1-weight));return out;
 }
public:
 Parameters params;
 void prepare(double sampleRate){rate=sampleRate;smooth=float(1-std::exp(-1/(rate*.01)));for(auto& delay:line)delay.prepare(rate);reset();}
 void reset(){for(auto& delay:line)delay.reset();for(auto& filter:low)filter.reset();allpass={};feedback={};held={};previousHeld={};eqState={};detector=slowDetector=0;compressGain=1;phase=shiftPhase=holdPhase=0;mix=0;control=0;lastType=params.type;current={params.a,params.b,params.c};}
 void process(float* left,float* right,int count,double bpm=120,double beat=0,float modMix=0,float modAmount=0){
  if(lastType!=params.type)reset();
  if(params.type==Type::Off)return;
  const float targetMix=params.bypass?0:std::clamp(params.mix+modMix,0.f,1.f);
  if(targetMix==0&&mix<1e-6f){if(mix!=0)reset();return;}
  const double beatIncrement=bpm/(60*rate);
  for(int i=0;i<count;++i){
   mix+=(targetMix-mix)*smooth;current[0]+=(std::clamp(params.a+modAmount,0.f,1.f)-current[0])*smooth;current[1]+=(params.b-current[1])*smooth;current[2]+=(params.c-current[2])*smooth;
   if(control--<=0){configure();control=31;}
   const float l=left[i],r=right[i];float x=l,y=r;double frequency=1;
   switch(params.type){
    case Type::Off:break;
    case Type::EQ:{std::array<float,2> input{l,r};for(size_t c=0;c<2;++c){auto& z=eqState[c];const float out=eqB0*input[c]+z[0];z[0]=eqB1*input[c]-eqA1*out+z[1];z[1]=eqB2*input[c]-eqA2*out;input[c]=out;}x=input[0];y=input[1];break;}
    case Type::Compressor:case Type::Limiter:{const float drive=params.type==Type::Limiter?db(value[2]):1;const float peak=std::max(std::abs(l),std::abs(r))*drive;const float release=std::exp(-1/(value[params.type==Type::Limiter?1:2]*.001f*float(rate)));detector=peak>detector?peak+(detector-peak)*float(std::exp(-1/(.003*rate))):peak+(detector-peak)*release;const float threshold=db(value[0]);float desired=1;if(detector>threshold)desired=params.type==Type::Limiter?threshold/detector:std::pow(detector/threshold,1/value[1]-1);compressGain+=(desired-compressGain)*(desired<compressGain?.2f:1-release);x=l*drive*compressGain;y=r*drive*compressGain;if(params.type==Type::Limiter){x=std::clamp(x,-threshold,threshold);y=std::clamp(y,-threshold,threshold);}break;}
    case Type::Saturation:{const float drive=db(value[0]),bias=std::tanh(value[2]*drive);x=low[0].process(std::tanh((l+value[2])*drive)-bias,1);y=low[1].process(std::tanh((r+value[2])*drive)-bias,1);break;}
    case Type::Distortion:{const float drive=db(value[0]);auto distort=[&](float v){const float a=std::clamp(v*drive,-value[1],value[1]);const float b=float(std::asin(std::sin(double(v*drive)))*2/std::numbers::pi);return a+(b-a)*value[2];};x=distort(l);y=distort(r);break;}
    case Type::Bitcrusher:{const float levels=std::exp2(std::round(value[0])-1),drive=db(value[1]);x=std::round((l*drive+noise()*value[2]/levels)*levels)/levels;y=std::round((r*drive+noise()*value[2]/levels)*levels)/levels;break;}
    case Type::RateReduce:{holdPhase+=std::min(double(value[0])/rate,1.);if(holdPhase>=1){holdPhase-=1;previousHeld=held;held={l*db(value[2]),r*db(value[2])};}x=held[0]*(1-value[1])+float(previousHeld[0]+holdPhase*(held[0]-previousHeld[0]))*value[1];y=held[1]*(1-value[1])+float(previousHeld[1]+holdPhase*(held[1]-previousHeld[1]))*value[1];break;}
    case Type::Tape:{frequency=.3;const double delay=2+rate*.001*value[1]*(1+std::sin(tau*phase));x=low[0].process(std::tanh(line[0].read(delay)*db(value[0])),1);y=low[1].process(std::tanh(line[1].read(delay)*db(value[0])),1);line[0].push(l);line[1].push(r);break;}
    case Type::Vinyl:{const float crack=noise()>1-value[1]*.002f?noise()*.2f:0;const float hiss=noise()*value[0];x=low[0].process(l+hiss+crack,1);y=low[1].process(r+hiss+crack,1);break;}
    case Type::Chorus:case Type::Flanger:{frequency=value[0];const double base=params.type==Type::Chorus?.015:.004,depth=value[1]*.001;const float dl=line[0].read((base+depth*std::sin(tau*phase))*rate),dr=line[1].read((base+depth*std::sin(tau*(phase+.25)))*rate);line[0].push(l+dl*value[2]);line[1].push(r+dr*value[2]);x=dl;y=dr;break;}
    case Type::Phaser:{frequency=value[0];const double cutoff=std::clamp(600*std::exp2(value[1]*2*std::sin(tau*phase)),30.,rate*.4),g=std::tan(std::numbers::pi*cutoff/rate);const float a=float((1-g)/(1+g));std::array<float,2> frame{l+feedback[0]*value[2],r+feedback[1]*value[2]};for(size_t c=0;c<2;++c){for(auto& z:allpass[c]){const float out=-a*frame[c]+z;z=frame[c]+a*out;frame[c]=out;}feedback[c]=frame[c];}x=(l+frame[0])*.5f;y=(r+frame[1])*.5f;break;}
    case Type::Tremolo:{frequency=value[0];const float sine=float(std::sin(tau*phase)),wave=sine*(1-value[2])+(sine>=0?1.f:-1.f)*value[2],gain=1-value[1]*(wave*.5f+.5f);x=l*gain;y=r*gain;break;}
    case Type::AutoPan:{frequency=value[0];const float pan=std::clamp(float(std::sin(tau*phase))*value[1]+value[2],-1.f,1.f);x=l*std::sqrt(1-pan);y=r*std::sqrt(1+pan);break;}
    case Type::Delay:case Type::PingPong:{const double time=value[0]*rate*.001;const float dl=low[0].process(line[0].read(time),1),dr=low[1].process(line[1].read(time),1);if(params.type==Type::PingPong){line[0].push(l+dr*value[1]);line[1].push(r+dl*value[1]);}else{line[0].push(l+dl*value[1]);line[1].push(r+dr*value[1]);}x=dl;y=dr;break;}
    case Type::Reverb:case Type::Shimmer:{const double size=(params.type==Type::Shimmer?value[1]:value[0])*rate*.001;float dl=0,dr=0;for(double tap:{1.,1.37,1.79,2.13}){dl+=line[0].read(size*tap)*.25f;dr+=line[1].read(size*(tap+.13))*.25f;}dl=low[0].process(dl,1);dr=low[1].process(dr,1);if(params.type==Type::Shimmer){const auto shiftedFrame=shifted(2);dl=dl*(1-value[0]*.3f)+shiftedFrame[0]*value[0]*.3f;dr=dr*(1-value[0]*.3f)+shiftedFrame[1]*value[0]*.3f;}const float decay=params.type==Type::Shimmer?value[2]:value[1];line[0].push(l+(dl*.7f+dr*.3f)*decay);line[1].push(r+(dr*.7f-dl*.3f)*decay);x=dl;y=dr;break;}
    case Type::Convolution:{float dl=0,dr=0;for(size_t tap=0;tap<64;++tap){dl+=line[0].read(double(irDelay[tap]))*irGain[tap];dr+=line[1].read(double(irDelay[tap]+tap%3))*irGain[tap];}line[0].push(l);line[1].push(r);x=low[0].process(dl,1);y=low[1].process(dr,1);break;}
    case Type::FilterSweep:frequency=value[2];x=low[0].process(l,1);y=low[1].process(r,1);break;
    case Type::Resonator:{const double delay=rate/value[0];const float dl=low[0].process(line[0].read(delay),1),dr=low[1].process(line[1].read(delay),1);line[0].push(l+dl*value[1]);line[1].push(r+dr*value[1]);x=dl;y=dr;break;}
    case Type::RingMod:frequency=value[0];x=l*(value[2]+(1-value[2])*float(std::sin(tau*phase)));y=r*(value[2]+(1-value[2])*float(std::sin(tau*(phase+value[1]*.5))));break;
    case Type::Transient:{const float peak=std::max(std::abs(l),std::abs(r));detector+=(peak-detector)*float(1-std::exp(-1/(rate*.003)));slowDetector+=(peak-slowDetector)*float(1-std::exp(-1/(rate*.05)));const float attack=std::max(0.f,detector-slowDetector)/(slowDetector+.01f),sustain=std::max(0.f,slowDetector-detector)/(slowDetector+.01f),gain=std::clamp(1+value[2]*(value[0]*attack+value[1]*sustain),0.f,4.f);x=l*gain;y=r*gain;break;}
    case Type::StereoWidth:{const float mid=(l+r)*.5f*db(value[1]),side=(l-r)*.5f*value[0]*db(value[2]);x=mid+side;y=mid-side;break;}
    case Type::Duck:{const double cycles=std::exp2(std::round(std::log2(value[0]))),fraction=wrap((beat+double(i)*beatIncrement)*cycles);const float gain=1-value[1]*(1-float(std::pow(fraction,value[2])));x=l*gain;y=r*gain;break;}
    case Type::PitchMod:{frequency=value[2];const auto frame=shifted(std::exp2((value[0]+value[1]*float(std::sin(tau*phase)))/12));line[0].push(l);line[1].push(r);x=frame[0];y=frame[1];break;}
    case Type::Count:break;
   }
   phase=wrap(phase+frequency/rate);
   if(!std::isfinite(x)||!std::isfinite(y)){x=y=0;reset();}
   x=std::clamp(x,-8.f,8.f);y=std::clamp(y,-8.f,8.f);left[i]=l+(x-l)*mix;right[i]=r+(y-r)*mix;
  }
 }
};
class Rack {
public:
 std::array<Slot,4> slots;
 void prepare(double rate){for(auto& slot:slots)slot.prepare(rate);}
 void reset(){for(auto& slot:slots)slot.reset();}
 void process(float* left,float* right,int count,double bpm=120,double beat=0,float mixMod=0,float amountMod=0){for(auto& slot:slots)slot.process(left,right,count,bpm,beat,mixMod,amountMod);}
};
}
