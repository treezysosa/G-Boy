#pragma once
#include <vector>
#include <complex>
#include <cmath>
#include <numbers>
#include <stdexcept>
namespace gboy::vocal {
// In-place radix-2 FFT. Workspaces are allocated by the background renderer.
inline void fft(std::vector<std::complex<double>>& data,bool inverse=false){
 const size_t n=data.size();if(n<2||(n&(n-1)))throw std::invalid_argument("FFT size must be a power of two");
 for(size_t i=1,j=0;i<n;++i){size_t bit=n>>1;for(;j&bit;bit>>=1)j^=bit;j^=bit;if(i<j)std::swap(data[i],data[j]);}
 for(size_t length=2;length<=n;length<<=1){const double angle=(inverse?2:-2)*std::numbers::pi/double(length);const std::complex<double> step(std::cos(angle),std::sin(angle));
  for(size_t i=0;i<n;i+=length){std::complex<double> phase(1);for(size_t j=0;j<length/2;++j){const auto a=data[i+j],b=data[i+j+length/2]*phase;data[i+j]=a+b;data[i+j+length/2]=a-b;phase*=step;}}
 }
 if(inverse)for(auto& x:data)x/=double(n);
}
}
