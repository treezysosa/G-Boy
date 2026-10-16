#include "audio/Engine.h"
#include "audio/HybridEngine.h"
#include "vocals/VocalRenderer.h"
#include "sequencer/Sequencer.h"
#include "presets/FactoryPatches.h"
#include <filesystem>
#include <memory>
#include <fstream>
#include <string>
void word(std::ostream& out,uint32_t x,int bytes){for(int i=0;i<bytes;++i)out.put(char((x>>(8*i))&255));}
void wav(const std::filesystem::path& path,const std::vector<float>& l,const std::vector<float>& r,int rate){
 std::ofstream out(path,std::ios::binary);auto bytes=uint32_t(l.size()*4);out.write("RIFF",4);word(out,36+bytes,4);out.write("WAVEfmt ",8);word(out,16,4);word(out,1,2);word(out,2,2);word(out,uint32_t(rate),4);word(out,uint32_t(rate*4),4);word(out,4,2);word(out,16,2);out.write("data",4);word(out,bytes,4);
 for(size_t i=0;i<l.size();++i)for(float x:{l[i],r[i]})word(out,uint16_t(int16_t(std::clamp(x,-1.f,1.f)*32767)),2);
 if(!out)throw std::runtime_error("WAV write failed");
}
int main(int argc,char** argv){auto dest=std::filesystem::path(argc>1?argv[1]:"factory-content/samples");std::filesystem::create_directories(dest);const char* names[]={"Chip Pulse","Soft Keys","FM Bell","Arcade Drum"};
 for(int k=0;k<4;++k){auto s=gboy::factory(k);wav(dest/(std::string(names[k])+".wav"),s.left,s.right,44100);}
 auto s=gboy::factory(1);gboy::Engine engine;engine.prepare(48000);engine.setSample(&s);std::vector<float> l(48000*3),r(l.size());for(int n:{60,64,67})engine.noteOn(n,.8f);engine.render(l.data(),r.data(),48000);for(int n:{60,64,67})engine.noteOff(n);engine.render(l.data()+48000,r.data()+48000,96000);wav(dest/"../Phase1-Demo.wav",l,r,48000);
 gboy::HybridEngine hybrid;hybrid.prepare(48000);
 for(int layer=0;layer<4;++layer){auto& engine=hybrid.layers[size_t(layer)];engine.setSample(&s);engine.params.enabled=true;engine.params.generator=gboy::Generator(layer+1);engine.params.level=.5f;engine.params.attack=.1f;engine.params.release=.7f;engine.params.filter=1;engine.params.cutoff=5000;engine.params.tune=layer==1?12.f:0.f;engine.params.routes[0]={gboy::ModSource::Lfo,gboy::ModTarget::Shape,.3f};}
 for(int note:{60,64,67}){hybrid.noteOn(note,.8f);}
 hybrid.render(l.data(),r.data(),48000);
 for(int note:{60,64,67}){hybrid.noteOff(note);}
 hybrid.render(l.data()+48000,r.data()+48000,96000);wav(dest/"../Phase2-Demo.wav",l,r,48000);

 auto phrase=gboy::vocal::syntheticPhrase();wav(dest/"Synthetic Vowel Phrase.wav",phrase.left,phrase.right,44100);
 gboy::vocal::Settings vocal;vocal.pitch=4;vocal.stretch=1.25f;vocal.formant=-2;vocal.slicing=1;vocal.effect=1;
 auto processed=gboy::vocal::render(phrase,vocal);wav(dest/"../Phase3-Demo.wav",processed.left,processed.right,44100);

 gboy::HybridEngine arrangement;arrangement.prepare(48000);arrangement.layers[0].params.generator=gboy::Generator::Analog;arrangement.layers[0].params.shape=.35f;arrangement.layers[0].params.filter=1;arrangement.layers[0].params.cutoff=4500;arrangement.layers[1].params.enabled=true;arrangement.layers[1].params.generator=gboy::Generator::FM;arrangement.layers[1].params.level=.25f;
 arrangement.masterEffects.slots[0].params={gboy::fx::Type::PingPong,false,.25f,.6f,.5f,.8f};arrangement.masterEffects.slots[1].params={gboy::fx::Type::Reverb,false,.2f,.7f,.5f,.7f};
 gboy::seq::Sequencer pattern;gboy::seq::Settings patternSettings;patternSettings.enabled=true;patternSettings.root=48;patternSettings.scale=1;pattern.configure(patternSettings);
 l.assign(48000*6,0);r.assign(l.size(),0);int cursor=0;
 for(int frame=0;frame<48000*4;++frame){const auto events=pattern.tick(double(frame)/24000,true);if(events.count){arrangement.render(l.data()+cursor,r.data()+cursor,frame-cursor);for(int i=0;i<events.count;++i){const auto& event=events.values[size_t(i)];if(event.on)arrangement.noteOn(event.note,event.velocity,event.channel);else arrangement.noteOff(event.note,event.channel);}cursor=frame;}}
 arrangement.render(l.data()+cursor,r.data()+cursor,48000*4-cursor);const auto stopEvents=pattern.tick(8,false);for(int i=0;i<stopEvents.count;++i)arrangement.noteOff(stopEvents.values[size_t(i)].note,stopEvents.values[size_t(i)].channel);arrangement.render(l.data()+48000*4,r.data()+48000*4,48000*2);wav(dest/"../Phase4-Demo.wav",l,r,48000);
 gboy::fx::Slot room;room.params={gboy::fx::Type::Convolution,false,1,.6f,.6f,.8f};room.prepare(48000);l.assign(24000,0);r.assign(24000,0);room.process(l.data(),r.data(),24000);l[0]=r[0]=1;room.process(l.data(),r.data(),24000);wav(dest/"Factory Sparse Room IR.wav",l,r,48000);

 const char* expandedNames[]={"Quiet Storm Keys","Velvet Night Pad","Cassette Bass","Neon Arcade","Haunted Glass","Bedroom Haze"};
 for(int patch=4;patch<10;++patch){auto renderer=std::make_unique<gboy::HybridEngine>();renderer->prepare(48000);const auto recipe=gboy::presets::hybrid(patch);for(size_t layer=0;layer<4;++layer){renderer->layers[layer].params=recipe[layer];renderer->layers[layer].setSample(&s);}l.assign(48000*4,0);r.assign(l.size(),0);renderer->noteOn(60,.85f);renderer->render(l.data(),r.data(),48000*2);renderer->noteOff(60);renderer->render(l.data()+48000*2,r.data()+48000*2,48000*2);wav(dest/(std::string(expandedNames[patch-4])+".wav"),l,r,48000);}

}
