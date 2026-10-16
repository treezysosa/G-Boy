#pragma once
#include "Fft.h"
#include "Slices.h"
#include "../granular/Grains.h"
#include "../effects/StateVariableFilter.h"
#include <atomic>
namespace gboy::vocal {
struct Settings {
 float pitch=0,stretch=1,formant=0;
 bool preserve=true,reverse=false;int slicing=0,effect=0;
 bool operator==(const Settings&) const = default;
};
inline std::vector<float> phaseVocoder(const std::vector<float>& input,double rate,double stretch,double envelopeRatio,const std::atomic<bool>* cancel,double pitchRatio=1){
 const size_t n=rate>96000?4096:rate>48000?2048:1024,hop=n/8,bins=n/2+1;
 const size_t length=size_t(std::ceil(double(input.size())*stretch));
 std::vector<double> output(length+n*4),normal(output.size()),previous(bins),phase(bins),envelope(bins),magnitude(bins),window(n);
 std::vector<std::complex<double>> spectrum(n),cepstrum(n);
 for(size_t i=0;i<n;++i)window[i]=.5-.5*std::cos(2*std::numbers::pi*double(i)/double(n));
 const int64_t last=int64_t(input.size())+int64_t(n);
 bool first=true;
 for(int64_t start=-int64_t(n);start<last;start+=int64_t(hop)){
  if(cancel&&cancel->load())throw std::runtime_error("Vocal render cancelled");
  for(size_t i=0;i<n;++i){const auto index=start+int64_t(i);spectrum[i]=index>=0&&index<int64_t(input.size())?double(input[size_t(index)])*window[i]:0.;}
  fft(spectrum);
  for(size_t k=0;k<bins;++k){magnitude[k]=std::abs(spectrum[k]);cepstrum[k]=std::log(std::max(1e-10,magnitude[k]));}
  if(std::abs(envelopeRatio-1)>1e-6){
   for(size_t k=bins;k<n;++k){cepstrum[k]=cepstrum[n-k];}
   fft(cepstrum,true);
   const size_t lifter=std::clamp(size_t(rate*.0015),size_t(8),n/8);
   for(size_t k=lifter+1;k<n-lifter;++k)cepstrum[k]=0;
   fft(cepstrum);for(size_t k=0;k<bins;++k)envelope[k]=std::exp(std::clamp(cepstrum[k].real(),-23.,23.));
  }
  const int64_t outStart=int64_t(std::llround((double(start)+double(n)/2)*stretch)-int64_t(n/2));
  const int64_t nextStart=int64_t(std::llround((double(start+int64_t(hop))+double(n)/2)*stretch)-int64_t(n/2));
  const double outHop=double(nextStart-outStart);
  for(size_t k=0;k<bins;++k){
   const double angle=std::arg(spectrum[k]),expected=2*std::numbers::pi*double(k)*double(hop)/double(n);
   double deviation=angle-previous[k]-expected;deviation-=2*std::numbers::pi*std::round(deviation/(2*std::numbers::pi));
   if(first)phase[k]=angle;else phase[k]+=(expected+deviation)*outHop/double(hop);
   previous[k]=angle;double gain=1;
   if(std::abs(envelopeRatio-1)>1e-6){const double index=std::clamp(double(k)/envelopeRatio,0.,double(bins-1));const size_t a=std::min(bins-2,size_t(index));const double f=index-double(a);const double target=envelope[a]+(envelope[a+1]-envelope[a])*f;gain=std::clamp(target/std::max(1e-10,envelope[k]),.05,20.);}
   spectrum[k]=std::polar(double(k)>double(n)/(2*std::max(1.,pitchRatio))?0.:magnitude[k]*gain,phase[k]);
  }
  spectrum[0]={spectrum[0].real(),0};spectrum[n/2]={spectrum[n/2].real(),0};
  for(size_t k=bins;k<n;++k){spectrum[k]=std::conj(spectrum[n-k]);}
  fft(spectrum,true);
  for(size_t i=0;i<n;++i){const int64_t pos=outStart+int64_t(i);if(pos>=0&&pos<int64_t(output.size())){output[size_t(pos)]+=spectrum[i].real()*window[i];normal[size_t(pos)]+=window[i]*window[i];}}
  first=false;
 }
 std::vector<float> result(length);for(size_t i=0;i<length;++i)result[i]=normal[i]>1e-8?float(output[i]/normal[i]):0;
 return result;
}
inline Sample render(const Sample& original,Settings settings,const std::atomic<bool>* cancel=nullptr){
 if(double(original.left.size())>original.rate*30||original.left.size()!=original.right.size()||original.left.size()<2||!std::isfinite(original.rate)||original.rate<8000||original.rate>192000)throw std::invalid_argument("Invalid vocal source");
 settings.pitch=std::isfinite(settings.pitch)?std::clamp(settings.pitch,-12.f,12.f):0;settings.stretch=std::isfinite(settings.stretch)?std::clamp(settings.stretch,.5f,2.f):1;settings.formant=std::isfinite(settings.formant)?std::clamp(settings.formant,-12.f,12.f):0;
 const double pitch=std::exp2(double(settings.pitch)/12),formant=std::exp2(double(settings.formant)/12);
 const double envelope=settings.preserve?formant/pitch:formant;
 const size_t length=std::max(size_t(2),size_t(std::llround(double(original.left.size())*settings.stretch)));
 Sample out;out.rate=original.rate;out.root=original.root;
 if(settings.pitch==0&&settings.stretch==1&&settings.formant==0){out.left=original.left;out.right=original.right;}
 else{
  for(int channel=0;channel<2;++channel){const auto& source=channel==0?original.left:original.right;auto stretched=phaseVocoder(source,original.rate,double(settings.stretch)*pitch,envelope,cancel,pitch);auto& data=channel==0?out.left:out.right;data.resize(length);
   for(size_t i=0;i<length;++i)data[i]=sampleAt(stretched,double(i)*pitch);
  }
 }
 if(settings.reverse){std::reverse(out.left.begin(),out.left.end());std::reverse(out.right.begin(),out.right.end());}
 if(settings.effect==1){
  for(auto* data:{&out.left,&out.right}){StateVariableFilter high,low;high.configure(350,.1f,out.rate);low.configure(3000,.1f,out.rate);for(float& value:*data)value=std::tanh(low.process(high.process(value,3),1)*2)*.65f;}
 }else if(settings.effect==2){
  for(auto* data:{&out.left,&out.right}){float held=0;for(size_t i=0;i<data->size();++i){if(i%6==0)held=std::round((*data)[i]*32)/32;(*data)[i]=held;}}
 }else if(settings.effect==3){
  const size_t beat=std::max(size_t(2),size_t(out.rate*.25)),repeat=std::max(size_t(2),size_t(out.rate*.0625));
  for(auto* data:{&out.left,&out.right}){const auto input=*data;for(size_t i=0;i<data->size();++i){const size_t pos=i%repeat,base=(i/beat)*beat;const float fade=float(std::min({1.,double(pos)/std::max(1.,out.rate*.002),double(repeat-1-pos)/std::max(1.,out.rate*.002)}));(*data)[i]=input[std::min(input.size()-1,base+pos)]*fade;}}
 }
 float peak=0;for(auto* data:{&out.left,&out.right})for(float& x:*data){if(!std::isfinite(x))x=0;peak=std::max(peak,std::abs(x));}
 if(peak>1)for(auto* data:{&out.left,&out.right})for(float& x:*data)x*=.98f/peak;
 // Edge ramps only for processed material; neutral renders preserve exact source samples.
 if(settings.pitch!=0||settings.stretch!=1||settings.formant!=0||settings.reverse||settings.effect!=0){const size_t fade=std::min(length/2,size_t(out.rate*.003));for(auto* data:{&out.left,&out.right})for(size_t i=0;i<fade;++i){(*data)[i]*=float(i)/float(fade);(*data)[data->size()-1-i]*=float(i)/float(fade);}}
 slice(out,settings.slicing);return out;
}
inline Sample syntheticPhrase(){
 Sample sample;sample.rate=44100;sample.left.resize(141120);sample.right.resize(sample.left.size());
 constexpr double formants[4][3]={{700,1200,2500},{350,1900,2600},{450,800,2400},{600,1500,2800}};
 for(size_t i=0;i<sample.left.size();++i){const double t=double(i)/sample.rate,local=std::fmod(t,.8);const int vowel=std::min(3,int(t/.8));double x=0;
  for(int harmonic=1;harmonic<=40;++harmonic){const double frequency=261.625565*harmonic;double amplitude=.08/double(harmonic);for(int band=0;band<3;++band){const double z=(frequency-formants[vowel][band])/(band==0?130.:220.);amplitude+=std::exp(-.5*z*z)*.12/(1+band*.5);}x+=amplitude*std::sin(2*std::numbers::pi*frequency*t);}
  const double gate=std::clamp(std::min(local/.02,(.68-local)/.05),0.,1.);sample.left[i]=sample.right[i]=float(x*gate*.65);
 }
 return sample;
}
}
