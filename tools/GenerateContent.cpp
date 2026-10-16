#include "audio/Engine.h"
#include <filesystem>
#include <fstream>
#include <string>
void word(std::ostream& out,uint32_t x,int bytes){for(int i=0;i<bytes;++i)out.put(char((x>>(8*i))&255));}
void wav(const std::filesystem::path& path,const std::vector<float>& l,const std::vector<float>& r,int rate){
 std::ofstream out(path,std::ios::binary);auto bytes=uint32_t(l.size()*4);out.write("RIFF",4);word(out,36+bytes,4);out.write("WAVEfmt ",8);word(out,16,4);word(out,1,2);word(out,2,2);word(out,uint32_t(rate),4);word(out,uint32_t(rate*4),4);word(out,4,2);word(out,16,2);out.write("data",4);word(out,bytes,4);
 for(size_t i=0;i<l.size();++i)for(float x:{l[i],r[i]})word(out,uint16_t(int16_t(std::clamp(x,-1.f,1.f)*32767)),2);
 if(!out)throw std::runtime_error("WAV write failed");
}
int main(int argc,char** argv){auto dest=std::filesystem::path(argc>1?argv[1]:"factory-content/samples");std::filesystem::create_directories(dest);const char* names[]={"Chip Pulse","Soft Keys","FM Bell","Arcade Drum"};
 for(int k=0;k<4;++k){auto s=gboy::factory(k);wav(dest/(std::string(names[k])+".wav"),s.left,s.right,44100);}
 auto s=gboy::factory(1);gboy::Engine engine;engine.prepare(48000);engine.setSample(&s);std::vector<float> l(48000*3),r(l.size());for(int n:{60,64,67})engine.noteOn(n,.8f);engine.render(l.data(),r.data(),48000);for(int n:{60,64,67})engine.noteOff(n);engine.render(l.data()+48000,r.data()+48000,96000);wav(dest/"../Phase1-Demo.wav",l,r,48000);
}
