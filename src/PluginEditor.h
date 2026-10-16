#pragma once
#include "PluginProcessor.h"
class GBoyEditor final:public juce::AudioProcessorEditor,private juce::Timer,public juce::FileDragAndDropTarget {
public:
 explicit GBoyEditor(GBoyProcessor&);~GBoyEditor() override {stopTimer();setLookAndFeel(nullptr);}
 void paint(juce::Graphics&) override;void resized() override;
 bool isInterestedInFileDrag(const juce::StringArray&) override;
 void filesDropped(const juce::StringArray&,int,int) override;
private:
 GBoyProcessor& p;juce::LookAndFeel_V4 look;
 juce::MidiKeyboardComponent keys;
 std::array<juce::Slider,5> knobs;std::array<juce::Label,5> labels;
 std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>,5> attachments;
 juce::ComboBox patches;juce::TextButton import{"IMPORT"},save{"SAVE"},load{"LOAD"},preview{"A / PLAY"},panic{"B / STOP"},mode{"RETRO / MODERN"};
 std::unique_ptr<juce::FileChooser> chooser;bool modern=false;
 void choose(int action);void timerCallback() override {repaint();}
};
