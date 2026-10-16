#include "effects/EffectRack.h"
#include <iostream>
#include <stdexcept>
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
int main(){
 for(double rate:{44100.,48000.,96000.,192000.}){
  gboy::fx::Slot slot;slot.prepare(rate);std::vector<float> l(8192),r(8192);
  for(int type=0;type<int(gboy::fx::Type::Count);++type){slot.params={gboy::fx::Type(type),false,.8f,.6f,.65f,.4f};slot.reset();
   for(int block=0;block<5;++block){for(size_t i=0;i<l.size();++i){l[i]=float(std::sin(gboy::tau*220*(double(i)+block*8192)/rate))*.3f;r[i]=l[i]*.7f;}slot.process(l.data(),r.data(),int(l.size()));for(size_t i=0;i<l.size();++i)check(std::isfinite(l[i])&&std::isfinite(r[i])&&std::abs(l[i])<=8,"all effect outputs finite and bounded");}
  }
 }
 gboy::fx::Slot slot;slot.prepare(48000);std::vector<float> l(48000),r(48000);for(size_t i=0;i<l.size();++i)l[i]=r[i]=float(std::sin(gboy::tau*1000*double(i)/48000))*.7f;const auto dry=l;
 slot.process(l.data(),r.data(),int(l.size()));check(l==dry,"off is bit-exact bypass");
 slot.params={gboy::fx::Type::Distortion,true,1,1,.2f,0};slot.reset();slot.process(l.data(),r.data(),int(l.size()));check(l==dry,"bypassed slot is dry");
 slot.params={gboy::fx::Type::Delay,false,1,0,0,1};slot.reset();std::fill(l.begin(),l.end(),0);std::fill(r.begin(),r.end(),0);slot.process(l.data(),r.data(),int(l.size()));l[0]=1;r[0]=0;slot.process(l.data(),r.data(),int(l.size()));check(std::abs(l[480])>.1f,"delay impulse arrives at 10ms");check(std::abs(l[100])<1e-6,"delay has no early wet output");
 slot.params={gboy::fx::Type::Limiter,false,1,.5f,.5f,1};slot.reset();std::fill(l.begin(),l.end(),2);std::fill(r.begin(),r.end(),2);slot.process(l.data(),r.data(),int(l.size()));for(size_t i=10000;i<l.size();++i)check(std::abs(l[i])<=gboy::fx::db(-12)+.001,"limiter ceiling");
 slot.params={gboy::fx::Type::StereoWidth,false,1,0,.5f,.5f};slot.reset();for(size_t i=0;i<l.size();++i){l[i]=dry[i];r[i]=-dry[i];}slot.process(l.data(),r.data(),int(l.size()));for(size_t i=10000;i<l.size();++i)check(std::abs(l[i]-r[i])<1e-4,"zero width is mono");
 slot.params={gboy::fx::Type::Convolution,false,1,.5f,.5f,1};slot.reset();std::fill(l.begin(),l.end(),0);std::fill(r.begin(),r.end(),0);slot.process(l.data(),r.data(),int(l.size()));l[0]=r[0]=1;slot.process(l.data(),r.data(),int(l.size()));double tail=0;for(size_t i=480;i<l.size();++i)tail+=l[i]*l[i];check(tail>.001,"factory IR convolution has an impulse tail");
 std::cout<<"PASS effects: 27 choices x 4 rates, bypass, delay timing, limiter ceiling, stereo width, convolution impulse\n";
}
