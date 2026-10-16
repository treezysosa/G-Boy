#pragma once
#include <array>
namespace gboy::seq {
struct Spec {const char* id;const char* name;float low,high,initial;};
inline constexpr std::array<Spec,6> controls{{{"seq_tempo","Tempo (BPM)",30,300,120},{"seq_root","Root MIDI",0,127,60},{"seq_swing","Swing",0,.49f,0},{"seq_key","Key",0,11,0},{"seq_octaves","Arp octaves",1,3,1},{"seq_seed","Seed",1,1000000,17}}};
inline constexpr std::array<const char*,6> stepFields{"enabled","pitch","velocity","gate","probability","ratchet"};
}
