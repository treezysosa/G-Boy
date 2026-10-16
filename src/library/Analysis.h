#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>
#include <string>
namespace gboy::library {
struct Analysis {
 double seconds=0,rms=0,peak=0,brightness=0,pitchHz=0,pitchConfidence=0,bpm=0,tempoConfidence=0;
 int root=-1;size_t trimStart=0,trimEnd=0;std::vector<size_t> transients;std::array<float,64> waveform{};std::vector<std::string> tags;
};
// Offline, bounded analysis of at most 30 seconds. Pitch/tempo are suggestions, not authoritative musical labels.
inline Analysis analyse(const float* left,const float* right,size_t count,double rate){
 Analysis a;if(!left||count<2||!std::isfinite(rate)||rate<8000||rate>192000)return a;
 count=std::min(count,size_t(rate*30));a.seconds=double(count)/rate;double energy=0;size_t crossings=0;
 auto mono=[&](size_t i){const double x=(double(left[i])+double(right?right[i]:left[i]))*.5;return std::isfinite(x)?std::clamp(x,-1.,1.):0.;};
 double previous=0;for(size_t i=0;i<count;++i){const double x=mono(i);energy+=x*x;a.peak=std::max(a.peak,std::abs(x));crossings+=(x>=0)!=(previous>=0);previous=x;}a.rms=std::sqrt(energy/double(count));a.brightness=std::min(1.,double(crossings)/a.seconds/10000);
 const double threshold=std::max(.0001,a.peak*.0031623);a.trimStart=0;a.trimEnd=count;while(a.trimStart<count&&std::abs(mono(a.trimStart))<threshold)++a.trimStart;while(a.trimEnd>a.trimStart&&std::abs(mono(a.trimEnd-1))<threshold)--a.trimEnd;
 for(size_t bin=0;bin<64;++bin){double peak=0;for(size_t i=count*bin/64;i<count*(bin+1)/64;++i){const double x=mono(i);if(std::abs(x)>std::abs(peak))peak=x;}a.waveform[bin]=float(peak);}
 if(a.rms<1e-5){a.tags={"silence"};return a;}
 // Several decimated autocorrelation windows reduce dependence on attacks and padded silence.
 const size_t stride=std::max(size_t(1),size_t(rate/8000));const double reduced=rate/double(stride);const size_t window=2048;
 std::array<double,window> signal{};double bestConfidence=0,bestFrequency=0;
 for(int frame=0;frame<4;++frame){const size_t offset=a.trimStart+size_t(frame)*size_t(rate*.15);if(offset+(window-1)*stride>=a.trimEnd)break;
  double mean=0;for(size_t i=0;i<window;++i)mean+=signal[i]=mono(offset+i*stride);mean/=window;for(auto& x:signal)x-=mean;
  const int minimum=int(reduced/1200),maximum=std::min(600,int(reduced/45));std::array<double,601> correlation{};double best=0;
  for(int lag=std::max(2,minimum);lag<=maximum;++lag){double xy=0,xx=0,yy=0;for(size_t i=0;i<window-size_t(lag);++i){const double x=signal[i],y=signal[i+size_t(lag)];xy+=x*y;xx+=x*x;yy+=y*y;}correlation[size_t(lag)]=xy/std::sqrt(std::max(1e-20,xx*yy));best=std::max(best,correlation[size_t(lag)]);}
  for(int lag=std::max(3,minimum+1);lag<maximum;++lag)if(correlation[size_t(lag)]>=best*.97&&correlation[size_t(lag)]>correlation[size_t(lag-1)]&&correlation[size_t(lag)]>=correlation[size_t(lag+1)]){const double c=correlation[size_t(lag)];if(c>bestConfidence){const double denominator=correlation[size_t(lag-1)]-2*c+correlation[size_t(lag+1)];const double delta=std::abs(denominator)>1e-12?.5*(correlation[size_t(lag-1)]-correlation[size_t(lag+1)])/denominator:0;bestConfidence=c;bestFrequency=reduced/(lag+std::clamp(delta,-.5,.5));}break;}
 }
 a.pitchConfidence=bestConfidence;if(bestConfidence>.75){a.pitchHz=bestFrequency;a.root=std::clamp(int(std::lround(69+12*std::log2(bestFrequency/440))),0,127);}
 const size_t hop=std::max(size_t(1),size_t(rate*.01)),frames=(count+hop-1)/hop;std::vector<double> envelopes(frames),flux(frames);double maxFlux=0;
 for(size_t frame=0;frame<frames;++frame){double sum=0;const size_t end=std::min(count,(frame+1)*hop);for(size_t i=frame*hop;i<end;++i){const double x=mono(i);sum+=x*x;}envelopes[frame]=std::sqrt(sum/double(end-frame*hop));if(frame){flux[frame]=std::max(0.,envelopes[frame]-envelopes[frame-1]);maxFlux=std::max(maxFlux,flux[frame]);}}
 for(size_t frame=1;frame+1<frames;++frame)if(flux[frame]>.12*maxFlux&&flux[frame]>.005&&flux[frame]>=flux[frame-1]&&flux[frame]>flux[frame+1]&&(a.transients.empty()||frame*hop-a.transients.back()>size_t(rate*.07)))a.transients.push_back(frame*hop);
 if(frames>200&&a.transients.size()>=4){double best=0;int bestLag=0;const int minimum=int(60*rate/(180*double(hop))),maximum=int(60*rate/(60*double(hop)));
  for(int lag=minimum;lag<=maximum&&lag<int(frames);++lag){double xy=0,xx=0,yy=0;for(size_t i=size_t(lag);i<frames;++i){xy+=flux[i]*flux[i-size_t(lag)];xx+=flux[i]*flux[i];yy+=flux[i-size_t(lag)]*flux[i-size_t(lag)];}const double score=xy/std::sqrt(std::max(1e-20,xx*yy));if(score>best+.01){best=score;bestLag=lag;}}
  a.tempoConfidence=best;if(best>.35&&bestLag)a.bpm=60*rate/(double(hop)*bestLag);
 }
 a.tags.push_back(a.root>=0?"tonal":"unpitched");a.tags.push_back(a.seconds<1?"short":"long");a.tags.push_back(a.brightness>.12?"bright":"dark");if(a.transients.size()>2)a.tags.push_back("transient-rich");if(a.rms<.02)a.tags.push_back("quiet");return a;
}
inline double distance(const Analysis& a,const Analysis& b){return std::abs(std::log2((a.seconds+.1)/(b.seconds+.1)))*.2+std::abs(a.rms-b.rms)*2+std::abs(a.brightness-b.brightness)*3+(a.root>=0&&b.root>=0?std::abs(a.root-b.root)/24.:.5);}
}
