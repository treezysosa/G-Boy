#include "audio/HybridEngine.h"
#include "sequencer/Sequencer.h"
#include <atomic>
#include <cstdlib>
#include <new>
#include <iostream>
#include <memory>
#if defined(_MSC_VER)
#define NOINLINE __declspec(noinline)
#else
#define NOINLINE __attribute__((noinline))
#endif
static std::atomic<bool> tracking{false};
static std::atomic<int> allocations{0};
void* operator new(std::size_t n){if(tracking.load())++allocations;if(void* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){return ::operator new(n);}
NOINLINE void operator delete(void* p) noexcept {std::free(p);}
NOINLINE void operator delete[](void* p) noexcept {std::free(p);}
NOINLINE void operator delete(void* p,std::size_t) noexcept {std::free(p);}
NOINLINE void operator delete[](void* p,std::size_t) noexcept {std::free(p);}
int main(){
 auto e=std::make_unique<gboy::HybridEngine>();auto sample=gboy::factory(1);e->prepare(48000);
 std::array<float,513> l{},r{};
 for(auto& layer:e->layers){layer.params.enabled=true;layer.setSample(&sample);layer.params.filter=1;layer.params.routes[0]={gboy::ModSource::Lfo,gboy::ModTarget::Cutoff,.5f};}
 tracking=true;
 for(int mode=0;mode<6;++mode){for(auto& layer:e->layers)layer.params.generator=gboy::Generator(mode);
  for(int n=0;n<80;++n)e->noteOn(48+n%24,.8f);
  for(int n=0;n<8;++n)e->render(l.data(),r.data(),513);
  e->wheel(1,.5f);e->bend(1,1);e->noteOff(60);e->allOff();
 }
 for(int type=1;type<int(gboy::fx::Type::Count);++type){for(auto& layer:e->layers)for(auto& slot:layer.effects.slots)slot.params={gboy::fx::Type(type),false,.3f,.5f,.5f,.5f};for(auto& slot:e->masterEffects.slots)slot.params={gboy::fx::Type(type),false,.3f,.5f,.5f,.5f};e->noteOn(60,.8f);e->render(l.data(),r.data(),513);e->allOff();}
 gboy::seq::Sequencer sequencer;gboy::seq::Settings config;config.enabled=true;config.mode=4;config.octaves=3;for(auto& bank:config.patterns)for(auto& step:bank)step.ratchet=4;sequencer.configure(config);for(int n=0;n<16;++n)sequencer.input(48+n,1);for(int n=0;n<48000;++n){const auto events=sequencer.tick(double(n)/24000,true);for(int i=0;i<events.count;++i){const auto& event=events.values[size_t(i)];if(event.on)e->noteOn(event.note,event.velocity,event.channel);else e->noteOff(event.note,event.channel);}}
 tracking=false;
 if(allocations.load()!=0){std::cerr<<"render allocated "<<allocations<<" times\n";return 1;}
 std::cout<<"PASS: no allocations during four-layer note allocation, stealing, all six generators, filtering, modulation and render, plus all twenty active FX slots across all effect types\n";
}
