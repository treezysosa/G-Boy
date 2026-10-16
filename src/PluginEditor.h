#pragma once
#include "PluginProcessor.h"
#include "sequencer/StepGrid.h"
#include "library/BrowserComponent.h"
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
 std::array<juce::Slider,3> vocalKnobs;std::array<juce::Label,3> vocalLabels;
 std::array<std::unique_ptr<SliderAttachment>,3> vocalAttachments;
 juce::ToggleButton preserve{"Preserve formants when pitching"},reverse{"Reverse phrase"};
 std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> preserveAttachment,reverseAttachment;
 juce::ComboBox sliceMode,vocalEffect,vocalPatches;
 juce::Label sliceLabel,effectLabel,vocalHelp;
 std::unique_ptr<ComboAttachment> sliceAttachment,effectAttachment;
 juce::TextButton synthetic{"SYNTHETIC VOWEL SOURCE"},midiExport{"EXPORT SLICE MIDI"},hybridPage{"HYBRID"},vocalPage{"VOCALS"};
 bool showVocals=false;int page=0;void setPage(int);
 juce::TextButton effectsPage{"FX"},sequencePage{"SEQ"},browserPage{"BROWSER"};
 std::unique_ptr<LibraryBrowser> browser;
 juce::ToggleButton masterRack{"Master rack"};juce::ComboBox rackPresets;
 juce::TextButton rackSave{"SAVE RACK"},rackLoad{"LOAD RACK"};
 std::array<juce::ComboBox,4> effectTypes;
 std::array<juce::ToggleButton,4> effectBypass;
 std::array<juce::TextButton,4> effectUp,effectDown;
 std::array<juce::Label,4> effectSlotLabels;
 std::array<std::array<juce::Slider,4>,4> effectSliders;
 std::array<std::array<juce::Label,4>,4> effectLabels;
 std::array<std::unique_ptr<ComboAttachment>,4> effectTypeAttachments;
 std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>,4> effectBypassAttachments;
 std::array<std::array<std::unique_ptr<SliderAttachment>,4>,4> effectAttachments;
 juce::Label effectsHelp;
 void setupPhase4();
 void bindEffects();void updateEffectLabels();int rackIndex() const {return masterRack.getToggleState()?4:selectedLayer;}
 StepGrid grid;
 std::array<juce::ToggleButton,5> seqFlags;
 std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>,5> seqFlagAttachments;
 std::array<juce::ComboBox,5> seqChoices;
 std::array<std::unique_ptr<ComboAttachment>,5> seqChoiceAttachments;
 std::array<juce::Slider,5> seqControls;
 std::array<juce::Label,5> seqControlLabels;
 std::array<std::unique_ptr<SliderAttachment>,5> seqControlAttachments;
 std::array<juce::Slider,5> stepControls;
 std::array<juce::Label,5> stepLabels;
 std::array<std::unique_ptr<SliderAttachment>,5> stepAttachments;
 juce::ToggleButton stepEnabled{"Step enabled"};
 std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> stepEnableAttachment;
 juce::ComboBox seqPresets,seqViewport;
 juce::TextButton seqExport{"EXPORT PATTERN MIDI"},seqRandom{"RANDOMIZE BANK"};
 juce::Label seqStateLabel,stepTitle,seqHelp;int selectedStep=0;
 void bindStep(int);void refreshSequence();
 juce::Slider master;juce::Label performanceLabel;juce::Label masterLabel;std::unique_ptr<SliderAttachment> masterAttachment;
 std::array<juce::TextButton,4> layerButtons;
 juce::ToggleButton enabled{"Layer enabled"};
 std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> enableAttachment;
 juce::ComboBox generator,filter,patches,hybridPatches;
 juce::Label engineLabel,filterLabel,matrixLabel;
 std::unique_ptr<ComboAttachment> generatorAttachment,filterAttachment;
 std::array<juce::ComboBox,4> modSources,modTargets,modFxTargets;
 std::array<juce::Slider,4> modAmounts;
 std::array<std::unique_ptr<ComboAttachment>,4> sourceAttachments,targetAttachments,fxTargetAttachments;
 std::array<std::unique_ptr<SliderAttachment>,4> amountAttachments;
 juce::TextButton import{"IMPORT"},save{"SAVE"},load{"LOAD"},preview{"A / PLAY"},panic{"B / STOP"},mode{"RETRO / MODERN"};
 std::unique_ptr<juce::FileChooser> chooser;bool modern=false;int selectedLayer=0;
 void selectLayer(int);void choose(int);void timerCallback() override {performanceLabel.setText("CPU "+juce::String(p.cpuLoad.load()*100,1)+"% / "+juce::String(p.voiceCount.load())+" voices",juce::dontSendNotification);updateEffectLabels();refreshSequence();grid.repaint();repaint();}
};
