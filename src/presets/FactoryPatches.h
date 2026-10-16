#pragma once
#include "../audio/HybridEngine.h"
namespace gboy::presets {
inline std::array<LayerParameters,4> hybrid(int preset){
 std::array<LayerParameters,4> result{};const int count=preset==3?4:preset>=4?2:3;
 for(int layer=0;layer<4;++layer){auto& p=result[size_t(layer)];p.enabled=layer<count;if(!p.enabled)continue;p.level=layer==0?.7f:.4f;p.pan=layer==1?-.35f:layer==2?.35f:0;
  if(preset==0){p.generator=Generator(layer+1);p.tune=layer==1?12.f:0.f;p.filter=1;p.cutoff=layer==0?2800.f:7000.f;p.shape=.3f;}
  else if(preset==1){p.generator=layer==0?Generator::Granular:layer==1?Generator::Wavetable:Generator::FM;p.attack=.6f;p.release=1.5f;p.filter=1;p.cutoff=5000;p.lfoRate=.3f;p.routes[0]={ModSource::Lfo,layer==0?ModTarget::Position:ModTarget::Cutoff,.3f};}
  else if(preset==2){p.generator=layer==1?Generator::FM:Generator::Analog;p.tune=-12;p.filter=1;p.cutoff=700;p.resonance=.35f;p.shape=.65f;p.routes[0]={ModSource::Envelope,ModTarget::Cutoff,.7f};}
  else if(preset==3){p.generator=Generator::FM;p.ratio=float(1+layer*2);p.shape=.25f;p.decay=1.2f;p.sustain=.2f;p.release=1;p.tune=layer==3?12.f:0.f;}
  else if(preset==4){p.generator=layer==0?Generator::FM:Generator::Analog;p.ratio=1;p.shape=.12f;p.decay=1.5f;p.sustain=.25f;p.release=.8f;p.filter=1;p.cutoff=2400;p.tune=layer==1?12.f:0.f;}
  else if(preset==5){p.generator=Generator::Wavetable;p.shape=.35f+float(layer)*.3f;p.attack=1.2f;p.release=2.5f;p.filter=1;p.cutoff=2200;p.tune=layer==1?7.f:0.f;p.routes[0]={ModSource::Lfo,ModTarget::Cutoff,.15f};p.lfoRate=.23f;}
  else if(preset==6){p.generator=Generator::Analog;p.tune=-12;p.shape=layer==0?.75f:.35f;p.filter=1;p.cutoff=layer==0?550.f:1200.f;p.decay=.3f;p.sustain=.4f;p.routes[0]={ModSource::Envelope,ModTarget::Cutoff,.3f};}
  else if(preset==7){p.generator=layer==0?Generator::Analog:Generator::FM;p.shape=layer==0?.08f:.5f;p.ratio=3;p.tune=layer==1?12.f:0.f;p.decay=.22f;p.sustain=.25f;p.release=.15f;p.routes[0]={ModSource::Lfo,ModTarget::Pitch,.015f};p.lfoRate=6;}
  else if(preset==8){p.generator=Generator::FM;p.ratio=layer==0?1.41f:2.73f;p.shape=.5f;p.attack=.5f;p.release=2;p.tune=layer==1?7.f:0.f;p.filter=1;p.cutoff=4800;}
  else {p.generator=Generator::Wavetable;p.shape=.2f+float(layer)*.2f;p.attack=.2f;p.release=1.2f;p.tune=layer==1?.08f:-.08f;p.filter=1;p.cutoff=1800;p.routes[0]={ModSource::Lfo,ModTarget::Shape,.12f};p.lfoRate=.4f;}
  p.routes[3]={ModSource::Macro,ModTarget::Shape,.25f};
 }
 return result;
}
}
