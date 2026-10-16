#pragma once
#include "../audio/Engine.h"
#include "../synth/Oscillators.h"
namespace gboy {
inline float sampleAt(const std::vector<float>& data,double pos){
 if(data.size()<2)return 0;
 const double last=double(data.size()-1);pos=std::clamp(pos,0.,last);
 const size_t i=std::min(data.size()-2,size_t(pos));const float f=float(pos-double(i));return data[i]+(data[i+1]-data[i])*f;
}
class Grains {
 struct Grain { double age=0,position=0,length=1; };
 std::array<Grain,4> grains{};
 bool initial=true;
public:
 void reset(){grains={};initial=true;}
 std::array<float,2> process(const Sample* sample,double step,float position,float size,double rate){
  if(!sample||sample->left.size()<2||sample->left.size()!=sample->right.size())return {};
  if(initial){for(size_t i=0;i<4;++i){auto& g=grains[i];g.length=std::max(16.,double(size)*rate);g.age=g.length*double(i)/4.;g.position=double(position)*double(sample->left.size()-1)+g.age*step;}initial=false;}
  std::array<float,2> out{};
  for(auto& g:grains){
   if(g.age>=g.length){g.age=0;g.length=std::max(16.,double(size)*rate);g.position=std::clamp(double(position),0.,1.)*double(sample->left.size()-1);}
   const float window=float(.5-.5*std::cos(tau*g.age/g.length))*.5f;
   const double pos=wrap(g.position/double(sample->left.size()-1))*double(sample->left.size()-1);
   out[0]+=sampleAt(sample->left,pos)*window;out[1]+=sampleAt(sample->right,pos)*window;
   g.position+=step;g.age+=1;
  }
  return out;
 }
};
}
