#pragma once
#include <array>
#include <algorithm>
#include <cmath>
#include <numbers>
namespace gboy {
inline constexpr double tau=2*std::numbers::pi;
inline double wrap(double phase) { return phase-std::floor(phase); }
inline float polyBlep(double t,double dt) {
 if(t<dt){const double x=t/dt;return float(x+x-x*x-1);}
 if(t>1-dt){const double x=(t-1)/dt;return float(x*x+x+x+1);}
 return 0;
}
// Immutable tables, generated before audio starts. Mip levels retain 1,2,4,8,16,32 harmonics.
class Wavetables {
 static constexpr int size=1024;
 using Table=std::array<float,size+1>;
 std::array<std::array<Table,3>,6> tables{};
public:
 Wavetables(){
  for(int mip=0;mip<6;++mip)for(int frame=0;frame<3;++frame){
   auto& table=tables[size_t(mip)][size_t(frame)];
   for(int i=0;i<size;++i){double p=tau*double(i)/size,x=0;
    for(int h=1;h<=(1<<mip);++h){
     if(frame==0&&h>1)break;
     if(frame==2&&h%2==0)continue;
     x+=std::sin(p*h)/(frame==0?1.:double(h));
    }
    table[size_t(i)]=float(x*(frame==0?1.:.65));
   }
   table[size]=table[0];
  }
 }
 float read(double phase,double increment,float position) const {
  int mip=0;while(mip<5 && (1<<(mip+1))*increment<.45)++mip;
  const float frame=std::clamp(position,0.f,1.f)*2;const int a=std::min(1,int(frame));const float mix=frame-float(a);
  const double index=wrap(phase)*size;const auto i=size_t(index);const float f=float(index-double(i));
  auto get=[&](int k){const auto& t=tables[size_t(mip)][size_t(k)];return t[i]+f*(t[i+1]-t[i]);};
  return get(a)+(get(a+1)-get(a))*mix;
 }
};
inline float analog(double phase,double dt,float shape){
 const float sine=float(std::sin(tau*phase));
 const float saw=float(2*phase-1)-polyBlep(phase,dt);
 const float pulse=(phase<.5?1.f:-1.f)+polyBlep(phase,dt)-polyBlep(wrap(phase+.5),dt);
 const float x=std::clamp(shape,0.f,1.f)*2;
 return x<1?sine+(saw-sine)*x:saw+(pulse-saw)*(x-1);
}
}
