#pragma once
#include <array>
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstdint>
namespace gboy {
struct Sample { std::vector<float> left, right; double rate=44100; int root=60; };
struct Parameters { float attack=.01f, decay=.15f, sustain=.7f, release=.25f, volume=.65f; };
class Engine {
 struct Voice { double pos=0, step=1; float env=0, velocity=0, releaseStep=0; int note=-1, channel=1, stage=0; uint64_t age=0; };
 std::array<Voice,64> voices{};
 const Sample* sample=nullptr; double rate=44100; uint64_t age=0; float gain=0;
public:
 Parameters params;
 void prepare(double r) { rate=r; reset(); }
 void reset() { for(auto& v:voices) v={}; gain=0; }
 void setSample(const Sample* s) { reset(); sample=s; }
 void noteOn(int note,float vel,int channel=1) {
  if(!sample || sample->left.size()<2) return;
  Voice* chosen=&voices[0];
  for(auto& v:voices) { if(v.stage==0) { chosen=&v; break; } if(v.age<chosen->age) chosen=&v; }
  *chosen={0, std::pow(2.,(note-sample->root)/12.)*sample->rate/rate,0,vel,0,note,channel,1,++age};
 }
 void noteOff(int note,int channel=1) { for(auto& v:voices) if(v.stage && v.note==note && v.channel==channel && v.stage!=4) { v.stage=4; v.releaseStep=v.env/(std::max(.001f,params.release)*float(rate)); } }
 void allOff(int channel=0) { for(auto& v:voices) if(!channel || v.channel==channel) v.stage=0; }
 void render(float* l,float* r,int n) {
  for(int i=0;i<n;++i) {
   float a=0,b=0;
   for(auto& v:voices) {
    if(!v.stage || !sample) continue;
    auto j=static_cast<size_t>(v.pos);
    if(j+1>=sample->left.size()) { v.stage=0; continue; }
    if(v.stage==1) { v.env+=1.f/(std::max(.001f,params.attack)*float(rate)); if(v.env>=1) {v.env=1;v.stage=2;} }
    else if(v.stage==2) { v.env-=(1-params.sustain)/(std::max(.001f,params.decay)*float(rate)); if(v.env<=params.sustain) {v.env=params.sustain;v.stage=3;} }
    else if(v.stage==3) v.env=params.sustain;
    else { v.env-=v.releaseStep; if(v.env<=0) {v.stage=0;continue;} }
    float f=float(v.pos-double(j)), amp=v.env*v.velocity*.16f;
    a+=(sample->left[j]+f*(sample->left[j+1]-sample->left[j]))*amp;
    b+=(sample->right[j]+f*(sample->right[j+1]-sample->right[j]))*amp;
    v.pos+=v.step;
   }
   gain+=(params.volume-gain)*float(1-std::exp(-1/(.01*rate)));
   l[i]=std::tanh(a*gain); r[i]=std::tanh(b*gain);
  }
 }
};
inline Sample factory(int kind) {
 Sample s; s.left.resize(44100*3); s.right.resize(s.left.size());
 uint32_t noise=42;
 for(size_t i=0;i<s.left.size();++i) {
  double t=double(i)/s.rate, p=261.625565*t;
  noise=noise*1664525u+1013904223u;
  double x=0;
  switch(kind) {
   case 0: x=(std::fmod(p,1.)<.25?.7:-.7)*std::exp(-t*1.4);break;
   case 1: x=(std::sin(2*3.141592653589793*p)+.25*std::sin(2*3.141592653589793*p*2.003))*std::exp(-t*.8);break;
   case 2: x=std::sin(2*3.141592653589793*p+2*std::sin(2*3.141592653589793*p*2))*std::exp(-t*2);break;
   default: x=(double(noise)/4294967295.*2-1)*std::exp(-t*22)+std::sin(2*3.141592653589793*(65*t+8*(1-std::exp(-t*30))))*std::exp(-t*12);break;
  }
  float fade=float(std::min(1.,t/.003)*std::min(1.,(3-t)/.02));
  s.left[i]=s.right[i]=float(x)*.7f*fade;
 }
 return s;
}
}
