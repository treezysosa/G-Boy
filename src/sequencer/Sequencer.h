#pragma once
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
namespace gboy::seq {
struct Step {bool enabled=true;int pitch=0;float velocity=.8f,gate=.65f,probability=1;int ratchet=1;};
inline Step defaultStep(int bank,int step){constexpr int pitches[8]={0,7,12,7,0,3,10,7};Step value;value.enabled=step%4!=3;value.pitch=pitches[(step+bank*2)%8];value.velocity=step%4==0?.9f:.65f;return value;}
struct Settings {
 bool enabled=false,sync=true,run=false,latch=false,chain=false;
 int length=16,bank=0,mode=0,root=60,key=0,scale=0,octaves=1;
 double tempo=120,division=.25;float swing=0;uint32_t seed=17;
 std::array<std::array<Step,64>,4> patterns{};
 Settings(){for(int bankIndex=0;bankIndex<4;++bankIndex)for(int step=0;step<64;++step)patterns[size_t(bankIndex)][size_t(step)]=defaultStep(bankIndex,step);}
};
struct Note {int note=60,channel=1;float velocity=.8f;};
struct Event {int note=60,channel=1;float velocity=0;bool on=false;};
struct Events {std::array<Event,32> values{};int count=0;void push(Event event){if(count<int(values.size()))values[size_t(count++)]=event;}};
inline uint32_t hash(uint32_t value){value^=value>>16;value*=0x7feb352du;value^=value>>15;value*=0x846ca68bu;return value^(value>>16);}
inline int scaleNote(int note,int key,int scale){
 note=std::clamp(note,0,127);if(scale==0)return note;
 constexpr std::array<uint16_t,5> masks{0xfff,0x0ab5,0x05ad,0x06ad,0x0295}; // chromatic, major, natural minor, dorian, major pentatonic
 const auto mask=masks[size_t(std::clamp(scale,0,4))];
 auto allowed=[&](int value){const int pc=((value-key)%12+12)%12;return (mask&(1<<pc))!=0;};
 for(int distance=0;distance<12;++distance){if(note+distance<=127&&allowed(note+distance))return note+distance;if(note-distance>=0&&allowed(note-distance))return note-distance;}
 return note;
}
inline std::string stepId(int bank,int step,const std::string& field){return "seq_b"+std::to_string(bank+1)+"_s"+std::to_string(step+1)+"_"+field;}
class Sequencer {
 Settings settings;
 std::array<std::array<float,128>,16> held{};
 std::array<std::array<bool,128>,16> physical{};
 std::array<Note,8> sounding{};int soundingCount=0;
 int64_t lastStep=std::numeric_limits<int64_t>::min();int lastSub=-1,currentIndex=-1,currentBank=-1;
 double previousBeat=0,offBeat=0;bool wasRunning=false,restart=false,passes=true;
 int keyCount() const {int count=0;for(const auto& channel:physical)for(bool value:channel)count+=value;return count;}
 void release(Events& events){for(int i=0;i<soundingCount;++i)events.push({sounding[size_t(i)].note,sounding[size_t(i)].channel,0,false});soundingCount=0;}
 int notes(std::array<Note,48>& out) const {
  int count=0;for(int note=0;note<128&&count<16;++note)for(int channel=0;channel<16&&count<16;++channel)if(held[size_t(channel)][size_t(note)]>0)out[size_t(count++)]={note,channel+1,held[size_t(channel)][size_t(note)]};
  const int base=count;for(int octave=1;octave<settings.octaves;++octave)for(int i=0;i<base&&count<48;++i){auto value=out[size_t(i)];value.note+=octave*12;if(value.note<=127)out[size_t(count++)]=value;}
  std::sort(out.begin(),out.begin()+count,[](const auto& a,const auto& b){return a.note<b.note||(a.note==b.note&&a.channel<b.channel);});return count;
 }
public:
 void configure(const Settings& next){
  if(settings.enabled!=next.enabled||settings.mode!=next.mode||settings.length!=next.length||settings.bank!=next.bank||settings.chain!=next.chain||settings.root!=next.root||settings.scale!=next.scale||settings.key!=next.key||settings.division!=next.division)restart=true;
  if(settings.latch&&!next.latch)for(size_t c=0;c<16;++c)for(size_t n=0;n<128;++n)if(!physical[c][n])held[c][n]=0;
  settings=next;
 }
 const Settings& configuration() const {return settings;}
 void input(int note,float velocity,int channel=1){
  if(note<0||note>127||channel<1||channel>16)return;
  if(velocity>0){if(settings.latch&&keyCount()==0)for(auto& values:held)values.fill(0);physical[size_t(channel-1)][size_t(note)]=true;held[size_t(channel-1)][size_t(note)]=velocity;}
  else{physical[size_t(channel-1)][size_t(note)]=false;if(!settings.latch)held[size_t(channel-1)][size_t(note)]=0;}
 }
 void clearInput(int channel=0){for(int i=0;i<16;++i)if(channel==0||channel==i+1){held[size_t(i)].fill(0);physical[size_t(i)].fill(false);}restart=true;}
 int step() const {return currentIndex;}int bank() const {return currentBank;}
 Events tick(double beat,bool running){
  Events events;
  if(!settings.enabled||!running){release(events);wasRunning=false;lastStep=std::numeric_limits<int64_t>::min();lastSub=-1;currentIndex=currentBank=-1;restart=false;return events;}
  if(restart||!wasRunning||beat<previousBeat-1e-7||beat-previousBeat>settings.division*2){release(events);lastStep=std::numeric_limits<int64_t>::min();lastSub=-1;restart=false;}
  wasRunning=true;previousBeat=beat;
  if(soundingCount&&beat+1e-9>=offBeat)release(events);
  const double division=settings.division,pair=std::floor(beat/(division*2)),pairStart=pair*division*2,first=division*(1+std::clamp(double(settings.swing),0.,.49));
  const bool odd=beat-pairStart>=first;const int64_t absolute=int64_t(pair)*2+(odd?1:0);const double begin=pairStart+(odd?first:0),duration=odd?division*2-first:first;
  const int index=int((absolute%settings.length+settings.length)%settings.length),cycle=int(std::floor(double(absolute)/settings.length));
  const int bank=settings.chain?((settings.bank+cycle)%4+4)%4:settings.bank;const auto& step=settings.patterns[size_t(bank)][size_t(index)];
  const int ratchet=std::clamp(step.ratchet,1,4);const double subDuration=duration/ratchet;const int sub=std::clamp(int(std::floor((beat-begin)/subDuration+1e-8)),0,ratchet-1);
  currentIndex=index;currentBank=bank;
  if(absolute==lastStep&&sub==lastSub)return events;
  release(events);
  if(absolute!=lastStep){const auto random=hash(settings.seed^uint32_t(absolute)^uint32_t(bank*73856093));passes=double(random)/4294967296.<std::clamp(double(step.probability),0.,1.);}
  lastStep=absolute;lastSub=sub;
  const double subBegin=begin+sub*subDuration;offBeat=subBegin+std::clamp(double(step.gate),.05,.95)*subDuration;
  if(!step.enabled||!passes||step.velocity<=0||beat>=offBeat)return events;
  std::array<Note,48> heldNotes{};const int count=notes(heldNotes);
  auto emit=[&](Note note){note.note=scaleNote(note.note+step.pitch,settings.key,settings.scale);note.velocity*=step.velocity;sounding[size_t(soundingCount++)]=note;events.push({note.note,note.channel,note.velocity,true});};
  const uint64_t sequence=uint64_t(absolute<0?-absolute:absolute)*uint64_t(ratchet)+uint64_t(sub);
  if(settings.mode==0){emit(count?heldNotes[0]:Note{settings.root,1,1});}
  else if(settings.mode==4){if(count){for(int i=0;i<std::min(count,8);++i)emit(heldNotes[size_t(i)]);}else for(int interval:{0,4,7})emit({std::min(127,settings.root+interval),1,1});}
  else if(count){int selected=0;if(settings.mode==1)selected=int(sequence%uint64_t(count));else if(settings.mode==2)selected=count-1-int(sequence%uint64_t(count));else{const int period=std::max(1,count*2-2),position=int(sequence%uint64_t(period));selected=position<count?position:period-position;}emit(heldNotes[size_t(selected)]);}
  return events;
 }
};
}
