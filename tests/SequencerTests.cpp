#include "sequencer/Sequencer.h"
#include <iostream>
#include <stdexcept>
#include <vector>
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
struct Timed {int sample;gboy::seq::Event event;};
std::vector<Timed> render(gboy::seq::Settings settings,int count=48000){gboy::seq::Sequencer seq;seq.configure(settings);seq.input(60,1);seq.input(64,1);seq.input(67,1);std::vector<Timed> events;for(int i=0;i<count;++i){auto frame=seq.tick(double(i)/24000,true);for(int k=0;k<frame.count;++k)events.push_back({i,frame.values[size_t(k)]});}return events;}
int main(){
 gboy::seq::Settings settings;settings.enabled=true;for(auto& bank:settings.patterns)for(auto& step:bank){step.enabled=true;step.pitch=0;step.gate=.5f;}
 auto events=render(settings);std::vector<int> ons,offs;for(const auto& e:events)(e.event.on?ons:offs).push_back(e.sample);check(ons.size()==8,"eight sixteenth steps per second at 120 BPM");check(ons[1]==6000&&offs[0]==3000,"sample accurate step and gate timing");
 settings.swing=.4f;events=render(settings);ons.clear();for(const auto& e:events)if(e.event.on)ons.push_back(e.sample);check(std::abs(ons[1]-8400)<=1&&ons[2]==12000,"swing delays odd step and preserves pairs");
 settings.swing=0;settings.patterns[0][0].ratchet=4;events=render(settings,6000);ons.clear();for(const auto& e:events)if(e.event.on)ons.push_back(e.sample);check(ons.size()==4&&ons[1]==1500,"ratchet subdivisions");
 settings.patterns[0][0].ratchet=1;for(auto& step:settings.patterns[0])step.probability=0;events=render(settings);check(events.empty(),"probability zero skips notes");
 for(auto& step:settings.patterns[0]){step.probability=1;}
 settings.mode=1;events=render(settings,24000);std::vector<int> notes;for(const auto& e:events)if(e.event.on)notes.push_back(e.event.note);check(notes==std::vector<int>({60,64,67,60}),"ascending arp cycles held notes");
 settings.mode=2;events=render(settings,18000);notes.clear();for(const auto& e:events)if(e.event.on)notes.push_back(e.event.note);check(notes==std::vector<int>({67,64,60}),"descending arp");
 settings.mode=4;events=render(settings,6000);int count=0;for(const auto& e:events)count+=e.event.on;check(count==3,"chord emits held triad");
 gboy::seq::Sequencer seq;seq.configure(settings);auto first=seq.tick(0,true);check(first.count==3,"chord fallback triad");auto stopped=seq.tick(.01,false);check(stopped.count==3,"stop releases all generated notes");
 first=seq.tick(0,true);auto seek=seq.tick(-1,true);check(seek.count>=3,"transport seek releases previous chord");
 check(gboy::seq::scaleNote(61,0,1)==62,"major scale snapping");
 settings.mode=0;settings.chain=true;settings.length=16;seq.configure(settings);seq.tick(0,true);seq.tick(4,true);check(seq.bank()==1&&seq.step()==0,"pattern chaining");
 settings.mode=1;settings.latch=true;seq.configure(settings);seq.clearInput();seq.input(65,1);seq.input(65,0);auto latch=seq.tick(0,true);check(latch.count>=1&&latch.values[size_t(latch.count-1)].note==65,"arp latch holds released key");
 settings.mode=1;settings.latch=false;settings.chain=false;settings.octaves=2;seq.configure(settings);seq.clearInput();seq.input(60,1);seq.input(64,1);std::vector<int> four;for(int i=0;i<4;++i){auto frame=seq.tick(i*.25,true);for(int k=0;k<frame.count;++k)if(frame.values[size_t(k)].on)four.push_back(frame.values[size_t(k)].note);}check(four==std::vector<int>({60,64,72,76}),"four-note/octave arp does not repeat only one note");
 std::cout<<"PASS sequencer: timing, gate, swing, ratchets, probability, arps, chords, stop/seek, scale, chaining, latch\n";
}
