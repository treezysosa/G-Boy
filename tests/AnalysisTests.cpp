#include "library/Analysis.h"
#include "presets/FactoryPatches.h"
#include <iostream>
#include <memory>
#include <stdexcept>
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
int main(){try{
 for(double rate:{44100.,48000.,96000.,192000.}){std::vector<float> tone(size_t(rate*1.5));for(size_t i=size_t(rate*.1);i<tone.size()-size_t(rate*.1);++i)tone[i]=float(.5*std::sin(6.283185307179586*220*double(i)/rate));auto a=gboy::library::analyse(tone.data(),nullptr,tone.size(),rate);check(a.root==57&&std::abs(a.pitchHz-220)<2,"pitch root detection at four rates");check(a.trimStart>=size_t(rate*.099)&&a.trimEnd<=tone.size()-size_t(rate*.099),"silence bounds");check(a.peak>.49&&a.rms>.3,"signal statistics");auto same=gboy::library::distance(a,a);check(same==0,"identical similarity distance");}
 std::vector<float> pulses(48000*6);for(size_t start=0;start<pulses.size();start+=24000)for(size_t i=0;i<2400&&start+i<pulses.size();++i)pulses[start+i]=float(std::sin(6.283185307179586*130*double(i)/48000)*std::exp(-double(i)/300));auto rhythm=gboy::library::analyse(pulses.data(),nullptr,pulses.size(),48000);check(rhythm.transients.size()>=10&&std::abs(rhythm.bpm-120)<3,"transients and tempo suggestion");
 std::vector<float> silence(8192);auto zero=gboy::library::analyse(silence.data(),nullptr,silence.size(),44100);check(zero.root==-1&&zero.bpm==0&&zero.trimStart==silence.size(),"silence has no invented pitch or tempo");check(gboy::library::analyse(nullptr,nullptr,0,44100).seconds==0,"empty input");
 for(int index=0;index<10;++index){auto engine=std::make_unique<gboy::HybridEngine>();engine->prepare(48000);const auto recipe=gboy::presets::hybrid(index);auto sample=gboy::factory(1);for(size_t layer=0;layer<4;++layer){engine->layers[layer].params=recipe[layer];engine->layers[layer].setSample(&sample);}std::vector<float> left(48000),right(48000);engine->noteOn(60,.8f);engine->render(left.data(),right.data(),int(left.size()));double energy=0;for(auto x:left){check(std::isfinite(x),"factory patches finite");energy+=x*x;}check(energy>.01,"all expanded factory patches audible");}
 std::cout<<"PASS analysis: pitch x 4 rates, silence bounds, statistics, onset/BPM, similarity, silence, 10 audible factory patches\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
