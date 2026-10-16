#include "audio/Engine.h"
#include <iostream>
#include <stdexcept>
void check(bool ok,const char* msg){if(!ok)throw std::runtime_error(msg);}
int main(){
 for(double rate:{44100.,48000.,96000.,192000.}) for(int block:{1,32,256,1024}) {
  auto sample=gboy::factory(0); gboy::Engine e; e.prepare(rate);e.setSample(&sample);
  std::vector<float> l(block),r(block);e.render(l.data(),r.data(),block);
  for(float x:l)check(x==0,"silence before MIDI");
  e.noteOn(60,1);double energy=0;
  for(int n=0;n<20000;n+=block){e.render(l.data(),r.data(),block);for(float x:l){check(std::isfinite(x)&&std::abs(x)<=1,"finite bounded output");energy+=x*x;}}
  check(energy>0.001,"MIDI produces audio");e.noteOff(60);
  for(int n=0;n<int(rate);n+=block)e.render(l.data(),r.data(),block);
  for(float x:l)check(x==0,"release reaches silence");
  for(int n=0;n<200;++n){e.noteOn(36+n%60,.8f);}
  e.render(l.data(),r.data(),block);
  e.allOff();e.render(l.data(),r.data(),block);for(float x:l)check(x==0,"panic");
 }
 for(int k=0;k<4;++k){auto s=gboy::factory(k);check(s.left.size()==132300,"factory length");double energy=0;for(auto x:s.left)energy+=x*x;check(energy>1,"factory audio");}
 std::cout<<"PASS: factory sounds, MIDI, ADSR release, 64-voice stealing, panic, 4 rates x 4 buffers\n";
}
