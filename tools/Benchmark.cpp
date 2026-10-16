#include "presets/FactoryPatches.h"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <memory>
#include <vector>
int main(int argc,char** argv){std::ostream* output=&std::cout;std::ofstream file;if(argc>1){file.open(argv[1]);if(!file)return 2;output=&file;}*output<<"{\"scope\":\"core engine, not hardware or DAW certification\",\"measurements\":[";bool first=true;
 for(double rate:{44100.,48000.,96000.,192000.})for(int block:{64,256,1024}){
  auto engine=std::make_unique<gboy::HybridEngine>();engine->prepare(rate);auto sample=gboy::factory(1);const auto patch=gboy::presets::hybrid(0);for(size_t i=0;i<4;++i){engine->layers[i].params=patch[i];engine->layers[i].setSample(&sample);}engine->masterEffects.slots[0].params={gboy::fx::Type::PingPong,false,.2f,.5f,.4f,.6f};engine->masterEffects.slots[1].params={gboy::fx::Type::Reverb,false,.2f,.5f,.5f,.6f};for(int note=48;note<64;++note)engine->noteOn(note,.7f);
  std::vector<float> left(size_t(block),0),right(size_t(block),0);std::vector<double> durations;durations.reserve(100);for(int i=0;i<8;++i)engine->render(left.data(),right.data(),block);
  for(int i=0;i<100;++i){const auto start=std::chrono::steady_clock::now();engine->render(left.data(),right.data(),block);const auto end=std::chrono::steady_clock::now();durations.push_back(std::chrono::duration<double>(end-start).count());for(float x:left)if(!std::isfinite(x))return 1;}
  std::sort(durations.begin(),durations.end());if(!first)*output<<',';first=false;*output<<"{\"rate\":"<<rate<<",\"block\":"<<block<<",\"layers\":3,\"notes\":16,\"voices\":48,\"master_effects\":2,\"median_seconds\":"<<durations[50]<<",\"p95_seconds\":"<<durations[95]<<",\"max_seconds\":"<<durations.back()<<",\"median_budget_percent\":"<<durations[50]*rate/block*100<<'}';
 }*output<<"]}\n";return *output?0:2;}
