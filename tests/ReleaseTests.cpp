#include "PluginProcessor.h"
#include "presets/StateMigration.h"
#include <atomic>
#include <cstdlib>
#include <new>
#include <memory>
#include <iostream>
#include <stdexcept>
#if defined(_MSC_VER)
#include <malloc.h>
#define GBOY_NOINLINE __declspec(noinline)
#else
#define GBOY_NOINLINE __attribute__((noinline))
#endif
namespace {thread_local bool tracking=false;thread_local int allocations=0,deallocations=0;}
void* operator new(std::size_t n){if(tracking)++allocations;if(auto* result=std::malloc(n?n:1))return result;throw std::bad_alloc();}
void* operator new[](std::size_t n){return ::operator new(n);}
GBOY_NOINLINE void operator delete(void* p) noexcept {if(tracking&&p)++deallocations;std::free(p);}
GBOY_NOINLINE void operator delete[](void* p) noexcept {::operator delete(p);}
GBOY_NOINLINE void operator delete(void* p,std::size_t) noexcept {::operator delete(p);}
GBOY_NOINLINE void operator delete[](void* p,std::size_t) noexcept {::operator delete(p);}
void* operator new(std::size_t n,std::align_val_t alignment){if(tracking)++allocations;
#if defined(_MSC_VER)
 if(auto* p=_aligned_malloc(n?n:1,size_t(alignment)))return p;
#else
 void* p=nullptr;if(posix_memalign(&p,size_t(alignment),n?n:1)==0)return p;
#endif
 throw std::bad_alloc();}
void* operator new[](std::size_t n,std::align_val_t alignment){return ::operator new(n,alignment);}
GBOY_NOINLINE void operator delete(void* p,std::align_val_t) noexcept {if(tracking&&p)++deallocations;
#if defined(_MSC_VER)
 _aligned_free(p);
#else
 std::free(p);
#endif
}
GBOY_NOINLINE void operator delete[](void* p,std::align_val_t alignment) noexcept {::operator delete(p,alignment);}
GBOY_NOINLINE void operator delete(void* p,std::size_t,std::align_val_t alignment) noexcept {::operator delete(p,alignment);}
GBOY_NOINLINE void operator delete[](void* p,std::size_t,std::align_val_t alignment) noexcept {::operator delete(p,alignment);}
namespace {
void expect(bool value,const char* message){if(!value)throw std::runtime_error(message);}
void parameter(GBoyProcessor& p,const juce::String& id,float value){auto* target=p.state.getParameter(id);expect(target!=nullptr,"release-test parameter exists");target->setValueNotifyingHost(target->convertTo0to1(value));}
void settle(GBoyProcessor& p){juce::AudioBuffer<float> buffer(2,128);juce::MidiBuffer midi;for(int i=0;i<3000;++i){p.processBlock(buffer,midi);bool busy=false;for(int layer=0;layer<4;++layer)busy|=p.isBusy(layer);if(!busy)return;std::this_thread::sleep_for(std::chrono::milliseconds(2));}throw std::runtime_error("release sample load timeout");}
struct Profile {juce::File home=juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("gboy-release-"+juce::Uuid().toString());~Profile(){home.deleteRecursively();}};
void writeJson(const juce::File& file,const juce::var& value){file.getParentDirectory().createDirectory();expect(file.replaceWithText(juce::JSON::toString(value,true)),"write release report");}
}
int runReleaseTests(const juce::File& reportDirectory){
 juce::ScopedJuceInitialiser_GUI gui;Profile profile;auto p=std::make_unique<GBoyProcessor>(profile.home);p->setRateAndBufferSizeDetails(48000,256);p->prepareToPlay(48000,256);
 for(int version=1;version<=5;++version){const auto file=juce::File(GBOY_FIXTURE_DIR).getChildFile("state-v"+juce::String(version)+".xml");auto xml=juce::XmlDocument::parse(file);expect(xml!=nullptr,"read versioned fixture");juce::MemoryBlock bytes;juce::AudioProcessor::copyXmlToBinary(*xml,bytes);expect(p->restoreState(bytes.getData(),int(bytes.getSize())),"migrate historical state");settle(*p);juce::MemoryBlock saved;p->getStateInformation(saved);auto current=juce::AudioProcessor::getXmlFromBinary(saved.getData(),int(saved.getSize()));expect(current&&current->getIntAttribute("version")==6&&current->getIntAttribute("sampleRoot1")==57,"migration retains root and writes version 6");if(version==1)expect(std::abs(p->state.getRawParameterValue("attack")->load()-.123f)<1e-4,"v1 envelope migration");if(version<4)expect(p->state.getRawParameterValue("seq_enabled")->load()==0&&p->state.getRawParameterValue("fx5_s1_type")->load()==0,"old states retain neutral new systems");if(version==4)expect(p->state.getRawParameterValue("seq_b1_s1_pitch")->load()==12,"v4 step migration");if(version==5)expect(std::abs(p->state.getRawParameterValue("volume")->load()-.42f)<1e-4,"v5 volume migration");}
 juce::XmlElement malformed("GBOY");malformed.setAttribute("version",6);auto* bad=malformed.createNewChildElement("PARAM");bad->setAttribute("id","volume");bad->setAttribute("value","nan");auto* huge=malformed.createNewChildElement("PARAM");huge->setAttribute("id","l1_cutoff");huge->setAttribute("value","1e300");juce::MemoryBlock bytes;juce::AudioProcessor::copyXmlToBinary(malformed,bytes);expect(p->restoreState(bytes.getData(),int(bytes.getSize())),"sanitized state accepted");expect(std::abs(p->state.getRawParameterValue("volume")->load()-.65f)<1e-4&&p->state.getRawParameterValue("l1_cutoff")->load()==20000,"non-finite rejected and finite values clamped");malformed.setAttribute("version",999);juce::AudioProcessor::copyXmlToBinary(malformed,bytes);expect(!p->restoreState(bytes.getData(),int(bytes.getSize())),"future version rejected");expect(!p->restoreState(nullptr,32),"null state rejected");for(int size=1;size<512;size+=7){juce::MemoryBlock noise(size_t(size),true);auto* data=static_cast<uint8_t*>(noise.getData());for(int i=0;i<size;++i)data[i]=uint8_t(i*17+size);expect(!p->restoreState(data,size),"random invalid state rejected");}
 p->loadHybridPreset(0);settle(*p);p->loadRackPreset(4,1);
 // Allocation tracking is local to the audio thread, including aligned C++ allocations/deletions.
 // MIDI construction, preparation, state editing and worker allocations are outside this scope.
 for(double rate:{44100.,48000.,96000.,192000.})for(int block:{1,32,256,1024}){p->prepareToPlay(rate,block);juce::AudioBuffer<float> audio(2,block);juce::MidiBuffer midi;midi.ensureSize(8192);for(int i=0;i<8;++i)p->processBlock(audio,midi);for(int i=0;i<16;++i)midi.addEvent(juce::MidiMessage::noteOn(1,48+i,.8f),0);allocations=deallocations=0;tracking=true;for(int i=0;i<8;++i)p->processBlock(audio,midi);tracking=false;expect(allocations==0&&deallocations==0,"full callback C++ allocation/deallocation check");for(int channel=0;channel<2;++channel)for(int frame=0;frame<block;++frame)expect(std::isfinite(audio.getSample(channel,frame)),"callback matrix finite");}
 // Simultaneous parameter edits, file requests and publication while rendering.
 p->prepareToPlay(48000,128);std::atomic<bool> done{false},finite{true};std::atomic<int> callbacks{0},audioAllocations{0};std::thread audioThread([&]{juce::AudioBuffer<float> audio(2,128);juce::MidiBuffer midi;midi.ensureSize(8192);midi.addEvent(juce::MidiMessage::noteOn(1,60,.8f),0);for(int i=0;i<16;++i)p->processBlock(audio,midi);allocations=deallocations=0;tracking=true;do{p->processBlock(audio,midi);for(int i=0;i<128;++i)if(!std::isfinite(audio.getSample(0,i)))finite=false;++callbacks;}while(!done.load()||callbacks.load()<128);tracking=false;audioAllocations=allocations+deallocations;});
 for(int i=0;i<40;++i){p->requestSample("factory:"+juce::String(i%4),i%4);parameter(*p,"l1_cutoff",float(100+i*300));parameter(*p,"l1_pan",float(i%11)/5-1);std::this_thread::sleep_for(std::chrono::milliseconds(2));}done=true;audioThread.join();expect(finite&&callbacks>0&&audioAllocations==0,"concurrent loading/editing remains finite without C++ audio allocations");settle(*p);
 // Render every engine page at two window sizes. This is automated GUI coverage, not a DAW usability review.
 reportDirectory.createDirectory();{std::unique_ptr<juce::AudioProcessorEditor> editor(p->createEditor());for(int page=0;page<5;++page){const char* buttons[]={"HYBRID","VOCALS","FX","SEQ","BROWSER"};for(int i=0;i<editor->getNumChildComponents();++i)if(auto* button=dynamic_cast<juce::TextButton*>(editor->getChildComponent(i)))if(button->getButtonText()==buttons[page]&&button->onClick)button->onClick();for(int size=0;size<2;++size){editor->setSize(size?1600:960,size?1200:900);auto image=editor->createComponentSnapshot(editor->getLocalBounds());expect(image.isValid(),"editor snapshot valid");juce::PNGImageFormat png;auto stream=reportDirectory.getChildFile("UI-"+juce::String(buttons[page])+"-"+juce::String(size)+".png").createOutputStream();expect(stream&&png.writeImageToStream(image,*stream),"save GUI snapshot");}}}
 juce::var report(new juce::DynamicObject());auto* out=report.getDynamicObject();out->setProperty("state_versions_tested","1,2,3,4,5,6");out->setProperty("cpp_callback_allocations",0);out->setProperty("cpp_callback_deallocations",0);out->setProperty("concurrent_callbacks",callbacks.load());out->setProperty("callback_smoothed_budget_percent",p->cpuLoad.load()*100);out->setProperty("callback_peak_budget_percent",p->cpuPeak.load()*100);out->setProperty("gui_snapshots",10);out->setProperty("scope","C++ operators instrumented; C malloc, OS locks and real DAW/hardware behavior are not certified");writeJson(reportDirectory.getChildFile("processor-report.json"),report);
 std::cout<<"PASS release: v1-v6 state/malformed inputs, 4 rates x 4 blocks, full-callback C++ allocation tracking, concurrent loader publication, 10 GUI snapshots\n";p.reset();return 0;
}
int exportFactoryPresets(const juce::File& destination){
 juce::ScopedJuceInitialiser_GUI gui;Profile profile;auto p=std::make_unique<GBoyProcessor>(profile.home);p->prepareToPlay(48000,128);p->setNonRealtime(true);auto manifest=juce::JSON::parse(destination.getChildFile("gboy-library.json").loadFileAsString());expect(manifest.isObject()&&manifest["entries"].isArray(),"factory pack manifest exists");manifest.getDynamicObject()->setProperty("id","gboy-original-factory-0.6");const auto folder=destination.getChildFile("native-presets");folder.createDirectory();const auto factory=gboy::library::factoryEntries();for(const auto& entry:factory){if(entry.action!="hybrid"&&entry.action!="vocal")continue;if(entry.action=="hybrid")p->loadHybridPreset(entry.factoryIndex);else p->loadVocalPreset(entry.factoryIndex);settle(*p);const auto file=folder.getChildFile(entry.name+".gboy");juce::MemoryBlock state;p->getStateInformation(state);expect(file.replaceWithData(state.getData(),state.getSize()),"export native preset");juce::AudioBuffer<float> render(2,48000*3);juce::AudioBuffer<float> block(2,128);juce::MidiBuffer midi;p->panicRequested=true;for(int offset=0;offset<render.getNumSamples();offset+=128){if(offset==0)for(int note:{60,64,67})midi.addEvent(juce::MidiMessage::noteOn(1,note,.8f),0);if(offset==48000)for(int note:{60,64,67})midi.addEvent(juce::MidiMessage::noteOff(1,note),0);p->processBlock(block,midi);for(int c=0;c<2;++c)render.copyFrom(c,offset,block,c,0,128);}auto preview=juce::File(file.getFullPathName()+".preview.wav");juce::WavAudioFormat format;auto stream=preview.createOutputStream();expect(stream!=nullptr,"preview output");std::unique_ptr<juce::AudioFormatWriter> writer(format.createWriterFor(stream.get(),48000,2,24,{},0));expect(writer!=nullptr,"preview writer");stream.release();expect(writer->writeFromAudioSampleBuffer(render,0,render.getNumSamples()),"export audible preview");writer.reset();juce::var metadata(new juce::DynamicObject());auto* object=metadata.getDynamicObject();object->setProperty("name",entry.name);object->setProperty("file","native-presets/"+file.getFileName());object->setProperty("preview","native-presets/"+preview.getFileName());object->setProperty("kind","patch");object->setProperty("category",entry.category);object->setProperty("tags",entry.tags);object->setProperty("author",entry.author);object->setProperty("license",entry.license);object->setProperty("provenance",entry.provenance);object->setProperty("human","synthetic");manifest["entries"].getArray()->add(metadata);}
 writeJson(destination.getChildFile("gboy-library.json"),manifest);std::cout<<"Exported 14 native factory presets with rendered previews\n";p.reset();return 0;
}
