#include "vocals/VocalRenderer.h"
#include "audio/HybridEngine.h"
#include <iostream>
#include <memory>
#include <stdexcept>
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
gboy::Sample tone(double rate,double duration,double frequency){gboy::Sample s;s.rate=rate;s.left.resize(size_t(rate*duration));s.right.resize(s.left.size());for(size_t i=0;i<s.left.size();++i)s.left[i]=s.right[i]=float(std::sin(gboy::tau*frequency*double(i)/rate))*.4f;return s;}
double energy(const std::vector<float>& audio){double value=0;for(float x:audio){check(std::isfinite(x)&&std::abs(x)<=1.001f,"finite bounded vocal output");value+=x*x;}return value;}
double frequency(const gboy::Sample& s){size_t a=s.left.size()/4,b=s.left.size()*3/4;int crossings=0;for(size_t i=a+1;i<b;++i)if(s.left[i-1]<=0&&s.left[i]>0)++crossings;return double(crossings)*s.rate/double(b-a);}
int main(){
 std::vector<std::complex<double>> fft(1024);fft[37]=1;const auto original=fft;gboy::vocal::fft(fft);gboy::vocal::fft(fft,true);for(size_t i=0;i<fft.size();++i)check(std::abs(fft[i]-original[i])<1e-10,"FFT roundtrip");
 for(double rate:{44100.,48000.,96000.,192000.}){
  auto input=tone(rate,.3,440);auto neutral=gboy::vocal::render(input,{});check(neutral.left==input.left,"neutral render exactly bypasses");
  gboy::vocal::Settings settings;settings.pitch=12;settings.preserve=false;auto up=gboy::vocal::render(input,settings);check(up.left.size()==input.left.size(),"pitch leaves duration unchanged");check(std::abs(frequency(up)-880)<20,"pitch up octave");energy(up.left);
  settings.pitch=-12;auto down=gboy::vocal::render(input,settings);check(std::abs(frequency(down)-220)<20,"pitch down octave");energy(down.left);
  settings.pitch=0;settings.stretch=1.6f;auto stretched=gboy::vocal::render(input,settings);check(std::abs(double(stretched.left.size())/input.left.size()-1.6)<.0001,"stretch duration");check(std::abs(frequency(stretched)-440)<15,"stretch preserves pitch");energy(stretched.left);
 }
 auto phrase=gboy::vocal::syntheticPhrase();check(energy(phrase.left)>1,"original synthetic vowel audio");
 gboy::vocal::Settings settings;settings.slicing=3;auto chopped=gboy::vocal::render(phrase,settings);check(chopped.sliceCount>=4&&chopped.sliceCount<=32,"detect phrase onsets");
 for(int mode:{1,2}){gboy::vocal::slice(chopped,mode);check(chopped.sliceCount==(mode==1?8:16),"grid slice count");for(int i=0;i<chopped.sliceCount;++i)check(chopped.sliceEdges[size_t(i+1)]>chopped.sliceEdges[size_t(i)],"increasing slice bounds");}
 settings={};settings.reverse=true;auto reversed=gboy::vocal::render(phrase,settings);check(reversed.left[10000]==phrase.left[phrase.left.size()-1-10000],"reverse source without compounding");
 settings={};settings.formant=7;auto shifted=gboy::vocal::render(phrase,settings);double difference=0;for(size_t i=0;i<shifted.left.size();++i)difference+=std::abs(shifted.left[i]-phrase.left[i]);check(difference>10,"formant processing changes audio");energy(shifted.left);
 settings.pitch=7;settings.preserve=true;auto preserved=gboy::vocal::render(phrase,settings);settings.preserve=false;auto unpreserved=gboy::vocal::render(phrase,settings);difference=0;for(size_t i=0;i<preserved.left.size();++i)difference+=std::abs(preserved.left[i]-unpreserved.left[i]);check(difference>10,"formant preservation changes pitched spectrum");
 for(int effect=1;effect<=3;++effect){settings={};settings.effect=effect;auto fx=gboy::vocal::render(phrase,settings);energy(fx.left);check(fx.left!=phrase.left,"vocal effect changes samples");}
 // Slice keys select their own region and end, rather than chromatically repitching the whole source.
 gboy::Sample signs;signs.rate=48000;signs.left.resize(4800,.4f);std::fill(signs.left.begin()+2400,signs.left.end(),-.4f);signs.right=signs.left;signs.sliceCount=2;signs.sliceEdges[0]=0;signs.sliceEdges[1]=2400;signs.sliceEdges[2]=4800;
 auto engine=std::make_unique<gboy::HybridEngine>();engine->prepare(48000);engine->layers[0].params.generator=gboy::Generator::Vocal;engine->layers[0].setSample(&signs);std::vector<float> l(6000),r(6000);
 engine->noteOn(60,1);engine->render(l.data(),r.data(),6000);check(l[1000]>0&&l[4000]==0,"first slice positive and ends");engine->allOff();engine->noteOn(61,1);engine->render(l.data(),r.data(),6000);check(l[1000]<0&&l[4000]==0,"second slice negative and ends");engine->noteOn(59,1);engine->render(l.data(),r.data(),6000);check(energy(l)==0,"outside slice keys silent");
 // Measure an actual spectral-envelope movement, not just a waveform difference.
 auto vowel=tone(48000,1,100);for(size_t i=0;i<vowel.left.size();++i){double x=0;for(int harmonic=1;harmonic<=60;++harmonic){const double z=(harmonic*100-800)/150.;x+=(.002+.1*std::exp(-.5*z*z))*std::sin(gboy::tau*harmonic*100*double(i)/48000);}vowel.left[i]=vowel.right[i]=float(x*.7);}
 auto spectralPeak=[](const gboy::Sample& sample){const size_t size=8192;std::vector<std::complex<double>> bins(size);const size_t start=(sample.left.size()-size)/2;for(size_t i=0;i<size;++i)bins[i]=double(sample.left[start+i])*(.5-.5*std::cos(gboy::tau*double(i)/size));gboy::vocal::fft(bins);size_t peak=1;for(size_t k=1;k<size/2;++k)if(std::abs(bins[k])>std::abs(bins[peak]))peak=k;return double(peak)*sample.rate/size;};
 settings={};settings.formant=12;auto raised=gboy::vocal::render(vowel,settings);check(spectralPeak(raised)>spectralPeak(vowel)*1.5,"formant shift moves spectral-envelope peak");
 settings={};settings.pitch=12;settings.preserve=true;auto kept=gboy::vocal::render(vowel,settings);settings.preserve=false;auto moved=gboy::vocal::render(vowel,settings);check(spectralPeak(moved)>spectralPeak(kept)*1.4,"preservation keeps envelope below unpreserved pitch-shifted formants");
 auto silent=tone(44100,.05,0);settings={};settings.pitch=5;settings.stretch=.5f;auto silence=gboy::vocal::render(silent,settings);check(energy(silence.left)==0,"silence remains silent during FFT processing");
 gboy::Sample tiny;tiny.left={0,.1f};tiny.right=tiny.left;settings={};settings.stretch=2;auto shortRender=gboy::vocal::render(tiny,settings);check(shortRender.left.size()==4,"very short input bounded");energy(shortRender.left);
 std::atomic<bool> cancel{true};bool stopped=false;try{settings={};settings.pitch=1;(void)gboy::vocal::render(phrase,settings,&cancel);}catch(const std::runtime_error&){stopped=true;}check(stopped,"render cancellation");
 std::cout<<"PASS vocals: FFT, pitch/time independence at four rates, reverse, formants, preservation, effects, grid/onset slices, MIDI mapping, cancellation\n";
}
