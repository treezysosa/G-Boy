#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "audio/HybridEngine.h"
#include "audio/ParameterSpecs.h"
#include "vocals/VocalRenderer.h"
#include <thread>
#include <mutex>
class GBoyProcessor final : public juce::AudioProcessor, private juce::MidiKeyboardStateListener {
public:
 GBoyProcessor(); ~GBoyProcessor() override;
 void prepareToPlay(double,int) override;
 void releaseResources() override {}
 void processBlock(juce::AudioBuffer<float>&,juce::MidiBuffer&) override;
 bool isBusesLayoutSupported(const BusesLayout& b) const override {return b.getMainOutputChannelSet()==juce::AudioChannelSet::stereo();}
 juce::AudioProcessorEditor* createEditor() override;
 bool hasEditor() const override {return true;}
 const juce::String getName() const override {return "G Boy";}
 bool acceptsMidi() const override {return true;} bool producesMidi() const override {return false;}
 double getTailLengthSeconds() const override {return 10.;}
 int getNumPrograms() override {return 1;} int getCurrentProgram() override {return 0;}
 void setCurrentProgram(int) override {} const juce::String getProgramName(int) override {return "Patch";}
 void changeProgramName(int,const juce::String&) override {}
 void getStateInformation(juce::MemoryBlock&) override;
 void setStateInformation(const void*,int) override;
 struct SliceInfo { std::array<double,33> edges{};int count=0;double seconds=0; };
 bool isBusy(int layer,bool includePublication=true) const;
 SliceInfo sliceInfo(int layer) const;
 bool exportSlices(const juce::File&,int layer) const;
 void loadVocalPreset(int);
 void loadHybridPreset(int preset);
 void requestSample(juce::String source,int layer=0);
 juce::String status(int layer=0);
 juce::AudioProcessorValueTreeState state;
 juce::MidiKeyboardState keyboard;
 std::array<std::array<std::atomic<float>,128>,4> waveform{};
 std::atomic<float> meter{0};
 std::atomic<bool> panicRequested{false};
private:
 static juce::AudioProcessorValueTreeState::ParameterLayout layout();
 gboy::HybridEngine engine;
 struct SampleSlot {
  gboy::Sample* active=nullptr;
  std::atomic<gboy::Sample*> pending{nullptr},retired{nullptr};
  juce::String requested,source="factory:0",message="CHIP PULSE",originalSource="factory:0";bool hasRequest=false,rendering=false,sourceDirty=false;
  std::unique_ptr<gboy::Sample> original;gboy::vocal::Settings applied;SliceInfo info;
 };
 std::array<SampleSlot,4> slots;
 struct Handles {
  std::array<std::atomic<float>*,15> values{};
  std::array<std::atomic<float>*,3> vocalValues{};
  std::atomic<float>* preserve=nullptr;std::atomic<float>* reverse=nullptr;std::atomic<float>* slicing=nullptr;std::atomic<float>* effect=nullptr;
  std::atomic<float>* enabled=nullptr;std::atomic<float>* generator=nullptr;std::atomic<float>* filter=nullptr;
  std::array<std::array<std::atomic<float>*,3>,4> routes{};
 };
 std::array<Handles,4> handles;
 std::atomic<float>* master=nullptr;
 juce::ValueTree defaults;
 void readParameters();
 std::atomic<bool> quit{false};
 gboy::vocal::Settings vocalSettings(size_t layer) const;
 mutable std::mutex mutex;
 std::thread worker;
 struct KeyEvent { int note=60, channel=1; float velocity=0; };
 juce::AbstractFifo keyFifo{256}; std::array<KeyEvent,256> keyEvents{};
 void handleNoteOn(juce::MidiKeyboardState*,int,int,float) override;
 void handleNoteOff(juce::MidiKeyboardState*,int,int,float) override;
 void loadLoop();
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GBoyProcessor)
};
