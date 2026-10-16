#pragma once
#include <cmath>
#include <algorithm>
#include <numbers>
namespace gboy {
class StateVariableFilter {
 float ic1=0,ic2=0,g=0,k=1,a1=1,a2=0,a3=0;
public:
 void reset(){ic1=ic2=0;}
 void configure(float cutoff,float resonance,double rate){
  g=std::tan(float(std::numbers::pi)*std::clamp(cutoff,20.f,float(rate*.45))/float(rate));
  k=2-1.9f*std::clamp(resonance,0.f,1.f);a1=1/(1+g*(g+k));a2=g*a1;a3=g*a2;
 }
 float process(float input,int mode){
  if(mode==0)return input;
  const float v3=input-ic2,v1=a1*ic1+a2*v3,v2=ic2+a2*ic1+a3*v3;
  ic1=2*v1-ic1;ic2=2*v2-ic2;
  // Clamp only corrupt runaway state; ordinary resonant output is left intact.
  if(!std::isfinite(ic1)||!std::isfinite(ic2)){reset();return 0;}
  return mode==1?v2:mode==2?v1:input-k*v1-v2;
 }
};
}
