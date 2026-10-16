#pragma once
#include <array>
#include <string>
namespace gboy {
struct ParameterSpec {const char* id;const char* label;float low,high,initial,skew;};
inline constexpr std::array<ParameterSpec,15> layerSpecs{{
 {"attack","Attack (s)",.001f,5,.01f,.4f},{"decay","Decay (s)",.001f,5,.15f,.4f},
 {"sustain","Sustain",0,1,.7f,1},{"release","Release (s)",.001f,10,.25f,.4f},
 {"level","Layer level",0,1,.8f,1},{"pan","Pan",-1,1,0,1},{"tune","Tune (st)",-24,24,0,1},
 {"cutoff","Cutoff (Hz)",20,20000,18000,.25f},{"resonance","Resonance",0,1,.1f,1},
 {"shape","VA / WT / FM shape",0,1,.5f,1},{"ratio","FM ratio",.25f,8,2,.5f},
 {"position","Grain position",0,1,.2f,1},{"grainSize","Grain size (s)",.01f,.25f,.08f,.5f},
 {"lfoRate","LFO rate (Hz)",.01f,30,2,.4f},{"macro","Macro",0,1,0,1}
}};
inline constexpr std::array<ParameterSpec,3> vocalSpecs{{
 {"vPitch","Pitch (st)",-12,12,0,1},{"vStretch","Duration ratio",.5f,2,1,1},{"vFormant","Formant (st)",-12,12,0,1}
}};
inline std::string parameterId(int layer,const std::string& suffix){
 if(layer==0&&(suffix=="attack"||suffix=="decay"||suffix=="sustain"||suffix=="release"))return suffix;
 return "l"+std::to_string(layer+1)+"_"+suffix;
}
inline std::string routeId(int layer,int route,const std::string& suffix){return parameterId(layer,"r"+std::to_string(route+1)+"_"+suffix);}
}
