#include "audio/HybridEngine.h"
#include <iostream>
#include <stdexcept>
#include <memory>
void require(bool ok,const char* text){if(!ok)throw std::runtime_error(text);}
double energy(const std::vector<float>& v){double sum=0;for(float x:v){require(std::isfinite(x)&&std::abs(x)<=1,"finite bounded samples");sum+=x*x;}return sum;}
int main(){
 auto sample=gboy::factory(1);
 for(double rate:{44100.,48000.,96000.,192000.})for(int mode=0;mode<5;++mode){
  auto e=std::make_unique<gboy::HybridEngine>();e->prepare(rate);auto& layer=e->layers[0];layer.setSample(&sample);layer.params.generator=gboy::Generator(mode);
  std::vector<float> l(8192),r(l.size());e->render(l.data(),r.data(),int(l.size()));require(energy(l)==0,"silence before note");
  e->noteOn(60,.8f);e->render(l.data(),r.data(),int(l.size()));require(energy(l)>.001,"each generator audible");
  e->noteOff(60);for(int i=0;i<30;++i)e->render(l.data(),r.data(),int(l.size()));require(energy(l)<1e-9,"each generator releases");
  for(int note=0;note<200;++note){e->noteOn(24+note%84,1);}
  require(layer.activeVoices()==64,"64 voices bounded");
  for(int filter=0;filter<4;++filter){layer.params.filter=filter;layer.params.resonance=1;layer.params.cutoff=20;e->render(l.data(),r.data(),int(l.size()));energy(l);layer.params.cutoff=float(rate*.45);e->render(l.data(),r.data(),int(l.size()));energy(l);}
  e->allOff();e->render(l.data(),r.data(),int(l.size()));require(energy(l)==0,"panic clears filters too");
 }
 // Four independently configured layers, independent enable and pan.
 auto e=std::make_unique<gboy::HybridEngine>();std::vector<float> l(48000),r(l.size());
 for(int i=0;i<4;++i){auto& layer=e->layers[size_t(i)];layer.params.enabled=true;layer.params.generator=gboy::Generator(i+1);layer.setSample(&sample);layer.params.tune=float(i*7);}
 e->noteOn(60,1);e->render(l.data(),r.data(),int(l.size()));require(energy(l)>.1,"four-layer mix");
 e->allOff();for(auto& layer:e->layers)layer.params.enabled=false;e->noteOn(60,1);e->render(l.data(),r.data(),int(l.size()));require(energy(l)==0,"disabled layers silent");
 auto& layer=e->layers[0];layer.params.enabled=true;layer.params.pan=-1;e->noteOn(60,1);e->render(l.data(),r.data(),int(l.size()));require(energy(l)>.01&&energy(r)==0,"hard left pan");
 // Stereo-short samples and no sample must be safe in granular mode.
 gboy::Sample tiny;tiny.left={0,1};tiny.right=tiny.left;layer.setSample(&tiny);layer.params.generator=gboy::Generator::Granular;e->noteOn(127,1);e->render(l.data(),r.data(),int(l.size()));energy(l);
 layer.setSample(nullptr);e->noteOn(60,1);e->render(l.data(),r.data(),int(l.size()));require(energy(l)==0,"missing sample safe");
 // Matrix sums routes, clamps amounts and preserves destination independence.
 std::array<gboy::ModRoute,4> routes{};routes[0]={gboy::ModSource::Wheel,gboy::ModTarget::Pitch,.5f};routes[1]={gboy::ModSource::Macro,gboy::ModTarget::Cutoff,-1};
 auto mods=gboy::evaluate(routes,{0,0,0,0,1,0,.25f});require(mods[0]==.5f&&mods[1]==-.25f&&mods[2]==0,"mod matrix routes");
 // End-to-end routing must alter the waveform, not merely the matrix values.
 auto render=[&](gboy::ModRoute route,float wheel,float macro){
  auto synth=std::make_unique<gboy::HybridEngine>();synth->prepare(48000);auto& lp=synth->layers[0].params;lp.generator=gboy::Generator::Analog;lp.shape=0;lp.routes[0]=route;lp.macro=macro;
  synth->wheel(1,wheel);synth->noteOn(69,1);std::vector<float> audio(12000),right(audio.size());synth->render(audio.data(),right.data(),int(audio.size()));return audio;
 };
 const auto dry=render({},0,0);
 auto crossings=[](const std::vector<float>& audio){int count=0;for(size_t i=6001;i<audio.size();++i)if(audio[i-1]<=0&&audio[i]>0)++count;return count;};
 require(std::abs(crossings(dry)-55)<=1,"A4 oscillator tuned to 440 Hz");
 auto octave=render({gboy::ModSource::Wheel,gboy::ModTarget::Pitch,1},1,0);require(std::abs(crossings(octave)-110)<=1,"wheel to pitch is one octave at full amount");
 auto filterEnergy=[](int mode,double frequency){gboy::StateVariableFilter filter;filter.configure(1000,.1f,48000);double sum=0;for(int i=0;i<24000;++i){float out=filter.process(float(std::sin(gboy::tau*frequency*i/48000)),mode);if(i>12000)sum+=out*out;}return sum;};
 require(filterEnergy(1,100)>filterEnergy(1,10000)*20,"low-pass attenuates high frequencies");
 require(filterEnergy(3,10000)>filterEnergy(3,100)*20,"high-pass attenuates low frequencies");
 require(filterEnergy(2,1000)>filterEnergy(2,100)*10&&filterEnergy(2,1000)>filterEnergy(2,10000)*10,"band-pass response centred at cutoff");
 for(int source=1;source<int(gboy::ModSource::Count);++source){auto modulated=render({gboy::ModSource(source),gboy::ModTarget::Pitch,.8f},1,1);double difference=0;for(size_t i=0;i<dry.size();++i)difference+=std::abs(dry[i]-modulated[i]);require(difference>1,"every modulation source reaches audio");}
 // Buffer partitioning must not change oscillators or modulation timing.
 auto a=std::make_unique<gboy::HybridEngine>(),b=std::make_unique<gboy::HybridEngine>();
 for(auto* synth:{a.get(),b.get()}){synth->prepare(48000);synth->layers[0].params.generator=gboy::Generator::FM;synth->layers[0].params.routes[0]={gboy::ModSource::Lfo,gboy::ModTarget::Shape,.5f};synth->noteOn(64,1);}
 std::vector<float> full(4096),fullR(4096),split(4096),splitR(4096);a->render(full.data(),fullR.data(),4096);
 for(int offset=0;offset<4096;){int count=std::min(37,4096-offset);b->render(split.data()+offset,splitR.data()+offset,count);offset+=count;}
 for(size_t i=0;i<full.size();++i)require(std::abs(full[i]-split[i])<1e-6f,"buffer partition invariant");
 std::cout<<"PASS hybrid: 5 generators x 4 rates, filters at extremes, release, voice bounds, layers, pan, missing/tiny samples, matrix\n";
}
