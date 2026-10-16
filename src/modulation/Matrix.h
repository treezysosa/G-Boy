#pragma once
#include <array>
#include <algorithm>
namespace gboy {
enum class ModSource { Off,Lfo,Envelope,Velocity,Wheel,Note,Macro,Count };
enum class ModTarget { Pitch,Cutoff,Gain,Pan,Position,Shape,Count };
struct ModRoute { ModSource source=ModSource::Off; ModTarget target=ModTarget::Pitch; float amount=0; };
using ModValues=std::array<float,size_t(ModTarget::Count)>;
inline ModValues evaluate(const std::array<ModRoute,4>& routes,const std::array<float,size_t(ModSource::Count)>& sources){
 ModValues out{};
 for(const auto& route:routes){auto s=size_t(route.source),d=size_t(route.target);if(s>0&&s<sources.size()&&d<out.size())out[d]+=sources[s]*std::clamp(route.amount,-1.f,1.f);}
 return out;
}
}
