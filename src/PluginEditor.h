#pragma once
#include "PluginProcessor.h"
class GBoyEditor final:public juce::AudioProcessorEditor,private juce::Timer,public juce::FileDragAndDropTarget {
public:
 explicit GBoyEditor(GBoyProcessor&);
 ~GBoyEditor() override;
 void paint(juce::Graphics&) override;void resized() override;
 bool isInterestedInFileDrag(const juce::StringArray&) override;
 void filesDropped(const juce::StringArray&,int,int) override;
private:
 using SliderAttachment=juce::AudioProcessorValueTreeState::SliderAttachment;
 using ComboAttachment=juce::AudioProcessorValueTreeState::ComboBoxAttachment;
 GBoyProcessor& p;juce::LookAndFeel_V4 look;
 juce::MidiKeyboardComponent keys;
 std::array<juce::Slider,15> knobs;std::array<juce::Label,15> labels;
 std::array<std::unique_ptr<SliderAttachment>,15> attachments;
 juce::Slider master;juce::Label masterLabel;std::unique_ptr<SliderAttachment> masterAttachment;
 std::array<juce::TextButton,4> layerButtons;
 juce::ToggleButton enabled{"Layer enabled"};
 std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> enableAttachment;
 juce::ComboBox generator,filter,patches,hybridPatches;
 juce::Label engineLabel,filterLabel,matrixLabel;
 std::unique_ptr<ComboAttachment> generatorAttachment,filterAttachment;
 std::array<juce::ComboBox,4> modSources,modTargets;
 std::array<juce::Slider,4> modAmounts;
 std::array<std::unique_ptr<ComboAttachment>,4> sourceAttachments,targetAttachments;
 std::array<std::unique_ptr<SliderAttachment>,4> amountAttachments;
 juce::TextButton import{"IMPORT"},save{"SAVE"},load{"LOAD"},preview{"A / PLAY"},panic{"B / STOP"},mode{"RETRO / MODERN"};
 std::unique_ptr<juce::FileChooser> chooser;bool modern=false;int selectedLayer=0;
 void selectLayer(int);void choose(int);void timerCallback() override {repaint();}
};
