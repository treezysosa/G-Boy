#include "audio/HybridEngine.h"
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
 tracking=false;
 if(allocations.load()!=0){std::cerr<<"render allocated "<<allocations<<" times\n";return 1;}
 std::cout<<"PASS: no allocations during four-layer note allocation, stealing, all six generators, filtering, modulation and render\n";
}
