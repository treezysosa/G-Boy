#pragma once
#include <array>
#include <cmath>
#include <algorithm>
#include <string>
namespace gboy::fx {
enum class Type {Off,EQ,Compressor,Limiter,Saturation,Distortion,Bitcrusher,RateReduce,Tape,Vinyl,Chorus,Flanger,Phaser,Tremolo,AutoPan,Delay,PingPong,Reverb,Convolution,Shimmer,FilterSweep,Resonator,RingMod,Transient,StereoWidth,Duck,PitchMod,Count};
struct Scale {const char* name;float low,high;bool log;const char* unit;};
struct Spec {const char* name;std::array<Scale,3> parameters;};
inline constexpr Scale percent{"Amount",0,1,false,""};
inline constexpr std::array<Spec,size_t(Type::Count)> specs{{
 {"Off",{{percent,percent,percent}}},
 {"Parametric EQ",{{{"Frequency",20,20000,true,"Hz"},{"Gain",-18,18,false,"dB"},{"Q",.25f,8,true,""}}}},
 {"Compressor",{{{"Threshold",-48,0,false,"dB"},{"Ratio",1,20,false,":1"},{"Release",10,500,true,"ms"}}}},
 {"Limiter",{{{"Ceiling",-24,0,false,"dB"},{"Release",10,500,true,"ms"},{"Drive",0,18,false,"dB"}}}},
 {"Saturation",{{{"Drive",0,24,false,"dB"},{"Tone",300,18000,true,"Hz"},{"Bias",-.3f,.3f,false,""}}}},
 {"Distortion",{{{"Drive",0,36,false,"dB"},{"Clip",.05f,1,false,""},{"Fold",0,1,false,""}}}},
 {"Bitcrusher",{{{"Bits",2,16,false,"bits"},{"Drive",0,12,false,"dB"},{"Dither",0,1,false,""}}}},
 {"Rate reduction",{{{"Rate",200,48000,true,"Hz"},{"Interpolation",0,1,false,""},{"Drive",0,12,false,"dB"}}}},
 {"Tape",{{{"Drive",0,18,false,"dB"},{"Wow",0,8,false,"ms"},{"Tone",2000,18000,true,"Hz"}}}},
 {"Vinyl noise",{{{"Hiss",0,.02f,false,""},{"Crackles",0,1,false,""},{"Wear",0,1,false,""}}}},
 {"Chorus",{{{"Rate",.05f,5,true,"Hz"},{"Depth",0,10,false,"ms"},{"Feedback",0,.5f,false,""}}}},
 {"Flanger",{{{"Rate",.05f,5,true,"Hz"},{"Depth",.05f,3,false,"ms"},{"Feedback",0,.85f,false,""}}}},
 {"Phaser",{{{"Rate",.05f,5,true,"Hz"},{"Depth",0,1,false,""},{"Feedback",0,.7f,false,""}}}},
 {"Tremolo",{{{"Rate",.05f,20,true,"Hz"},{"Depth",0,1,false,""},{"Shape",0,1,false,""}}}},
 {"Auto pan",{{{"Rate",.05f,20,true,"Hz"},{"Depth",0,1,false,""},{"Centre",-1,1,false,""}}}},
 {"Stereo delay",{{{"Time",10,1500,true,"ms"},{"Feedback",0,.85f,false,""},{"Tone",300,18000,true,"Hz"}}}},
 {"Ping pong delay",{{{"Time",10,1500,true,"ms"},{"Feedback",0,.85f,false,""},{"Tone",300,18000,true,"Hz"}}}},
 {"Algorithmic reverb",{{{"Size",20,150,true,"ms"},{"Decay",.1f,.94f,false,""},{"Tone",300,18000,true,"Hz"}}}},
 {"Factory IR convolution",{{{"IR spread",10,350,true,"ms"},{"Decay",20,400,true,"ms"},{"Tone",300,18000,true,"Hz"}}}},
 {"Shimmer reverb",{{{"Shimmer",0,1,false,""},{"Size",20,150,true,"ms"},{"Decay",0,.9f,false,""}}}},
 {"Filter sweep",{{{"Cutoff",20,20000,true,"Hz"},{"Depth",0,4,false,"oct"},{"Rate",.05f,8,true,"Hz"}}}},
 {"Resonator",{{{"Frequency",40,2000,true,"Hz"},{"Feedback",0,.95f,false,""},{"Tone",300,18000,true,"Hz"}}}},
 {"Ring modulation",{{{"Frequency",.5f,2000,true,"Hz"},{"Stereo phase",0,1,false,""},{"Bias",0,1,false,""}}}},
 {"Transient shaper",{{{"Attack",-1,1,false,""},{"Sustain",-1,1,false,""},{"Sensitivity",.1f,4,false,""}}}},
 {"Stereo width",{{{"Width",0,2,false,""},{"Mid gain",-6,6,false,"dB"},{"Side gain",-6,6,false,"dB"}}}},
 {"Rhythmic ducking",{{{"Cycles / beat",.25f,4,true,""},{"Depth",0,1,false,""},{"Recovery",.2f,4,false,""}}}},
 {"Pitch modulation",{{{"Pitch",-12,12,false,"st"},{"Vibrato",0,2,false,"st"},{"Rate",.1f,8,true,"Hz"}}}}
}};
inline float physical(Type type,size_t parameter,float normalized){const auto& spec=specs[size_t(type)].parameters[parameter];const float x=std::clamp(normalized,0.f,1.f);return spec.log?spec.low*std::pow(spec.high/spec.low,x):spec.low+(spec.high-spec.low)*x;}
inline float db(float value){return std::pow(10.f,value/20);}
inline std::string parameterId(int rack,int slot,const std::string& name){return "fx"+std::to_string(rack+1)+"_s"+std::to_string(slot+1)+"_"+name;}
struct Parameters {Type type=Type::Off;bool bypass=false;float mix=.3f,a=.5f,b=.5f,c=.5f;};
}
