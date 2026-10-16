#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "audio/Engine.h"
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
 void requestSample(juce::String source);
 juce::String status();
 juce::AudioProcessorValueTreeState state;
 juce::MidiKeyboardState keyboard;
 std::array<std::atomic<float>,128> waveform{};
 std::atomic<float> meter{0};
 std::atomic<bool> panicRequested{false};
private:
 static juce::AudioProcessorValueTreeState::ParameterLayout layout();
 gboy::Engine engine;
 gboy::Sample* active=nullptr;
 std::atomic<gboy::Sample*> pending{nullptr}, retired{nullptr};
 std::atomic<bool> quit{false};
 std::mutex mutex; juce::String requested, source="factory:0", message="CHIP PULSE"; bool hasRequest=false;
 std::thread worker;
 struct KeyEvent { int note=60, channel=1; float velocity=0; };
 juce::AbstractFifo keyFifo{256}; std::array<KeyEvent,256> keyEvents{};
 void handleNoteOn(juce::MidiKeyboardState*,int,int,float) override;
 void handleNoteOff(juce::MidiKeyboardState*,int,int,float) override;
 void loadLoop();
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GBoyProcessor)
};
