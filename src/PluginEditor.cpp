#include "PluginEditor.h"
GBoyEditor::GBoyEditor(GBoyProcessor& processor):AudioProcessorEditor(processor),p(processor),keys(p.keyboard,juce::MidiKeyboardComponent::horizontalKeyboard){
 setLookAndFeel(&look);look.setColour(juce::Slider::rotarySliderFillColourId,juce::Colour(0xff99e8ce));look.setColour(juce::TextButton::buttonColourId,juce::Colour(0xff4b386e));look.setColour(juce::TextButton::buttonOnColourId,juce::Colour(0xff24675f));
 for(size_t i=0;i<knobs.size();++i){auto& slider=knobs[i];slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);slider.setTextBoxStyle(juce::Slider::TextBoxBelow,false,84,20);slider.setNumDecimalPlacesToDisplay(i==6||i==7?1:3);addAndMakeVisible(slider);labels[i].setText(gboy::layerSpecs[i].label,juce::dontSendNotification);labels[i].setJustificationType(juce::Justification::centred);addAndMakeVisible(labels[i]);}
 for(int i=0;i<4;++i){auto& button=layerButtons[size_t(i)];button.setButtonText("LAYER "+juce::String(i+1));button.onClick=[this,i]{selectLayer(i);};addAndMakeVisible(button);}
 generator.addItemList({"Sampler","Virtual analog","Wavetable","FM","Granular","Vocal"},1);filter.addItemList({"Bypass","Low pass","Band pass","High pass"},1);
 for(auto* combo:{&generator,&filter,&patches,&hybridPatches})addAndMakeVisible(combo);
 addAndMakeVisible(enabled);engineLabel.setText("ENGINE",juce::dontSendNotification);filterLabel.setText("FILTER",juce::dontSendNotification);matrixLabel.setText("MOD MATRIX  |  SOURCE  >  DESTINATION  >  BIPOLAR AMOUNT",juce::dontSendNotification);
 for(auto* label:{&engineLabel,&filterLabel,&matrixLabel,&masterLabel})addAndMakeVisible(label);
 masterLabel.setText("MASTER",juce::dontSendNotification);master.setSliderStyle(juce::Slider::LinearHorizontal);master.setTextBoxStyle(juce::Slider::TextBoxRight,false,60,22);addAndMakeVisible(master);masterAttachment=std::make_unique<SliderAttachment>(p.state,"volume",master);
 hybridPatches.addItemList({"Handheld Stack","Glacier Grains","Indigo Bass","FM Prism"},1);hybridPatches.setTextWhenNothingSelected("Hybrid patches...");hybridPatches.onChange=[this]{if(hybridPatches.getSelectedId()>0){p.loadHybridPreset(hybridPatches.getSelectedId()-1);hybridPatches.setSelectedId(0,juce::dontSendNotification);}};
 patches.addItemList({"Chip Pulse","Soft Keys","FM Bell","Arcade Drum"},1);patches.setTextWhenNothingSelected("Load factory sample...");patches.onChange=[this]{if(patches.getSelectedId()>0){p.requestSample("factory:"+juce::String(patches.getSelectedId()-1),selectedLayer);patches.setSelectedId(0,juce::dontSendNotification);}};
 for(size_t i=0;i<4;++i){modSources[i].addItemList({"Off","LFO","Envelope","Velocity","Mod wheel","Note","Macro"},1);modTargets[i].addItemList({"Pitch","Cutoff","Gain","Pan","Grain position","Shape / FM index"},1);modAmounts[i].setSliderStyle(juce::Slider::LinearHorizontal);modAmounts[i].setTextBoxStyle(juce::Slider::TextBoxRight,false,60,22);addAndMakeVisible(modSources[i]);addAndMakeVisible(modTargets[i]);addAndMakeVisible(modAmounts[i]);}
 for(auto* button:{&import,&save,&load,&preview,&panic,&mode})addAndMakeVisible(button);
 import.onClick=[this]{choose(0);};save.onClick=[this]{choose(1);};load.onClick=[this]{choose(2);};
 preview.onStateChange=[this]{if(preview.isDown())p.keyboard.noteOn(1,60,.85f);else p.keyboard.noteOff(1,60,0.f);};
 panic.onClick=[this]{for(int ch=1;ch<=16;++ch)p.keyboard.allNotesOff(ch);p.panicRequested.store(true);};mode.onClick=[this]{modern=!modern;repaint();};
 for(size_t i=0;i<3;++i){vocalKnobs[i].setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);vocalKnobs[i].setTextBoxStyle(juce::Slider::TextBoxBelow,false,110,24);vocalKnobs[i].setNumDecimalPlacesToDisplay(3);vocalLabels[i].setText(gboy::vocalSpecs[i].label,juce::dontSendNotification);vocalLabels[i].setJustificationType(juce::Justification::centred);addAndMakeVisible(vocalKnobs[i]);addAndMakeVisible(vocalLabels[i]);}
 for(auto* button:{&synthetic,&midiExport,&hybridPage,&vocalPage})addAndMakeVisible(button);
 for(auto* button:{&preserve,&reverse})addAndMakeVisible(button);
 for(auto* combo:{&sliceMode,&vocalEffect,&vocalPatches})addAndMakeVisible(combo);
 for(auto* label:{&sliceLabel,&effectLabel,&vocalHelp})addAndMakeVisible(label);
 sliceMode.addItemList({"Whole phrase (chromatic)","8 grid slices / MIDI 60-67","16 grid slices / MIDI 60-75","Detect onsets / MIDI 60+"},1);
 vocalEffect.addItemList({"Clean","Telephone","Lo-fi resampling","Stutter / fixed 120 BPM"},1);
 sliceLabel.setText("SLICE MAPPING",juce::dontSendNotification);effectLabel.setText("VOCAL EFFECT",juce::dontSendNotification);
 vocalHelp.setText("Import your original recording or use the synthetic vowel source.\n"
                   "Choose the Vocal engine for MIDI slice playback. Edits render in the background.",juce::dontSendNotification);vocalHelp.setJustificationType(juce::Justification::centredLeft);
 vocalPatches.addItemList({"Pixel Chops","Reverse Aura","Radio Cuts","Vowel Harmony"},1);vocalPatches.setTextWhenNothingSelected("Vocal presets...");vocalPatches.onChange=[this]{if(vocalPatches.getSelectedId()>0){p.loadVocalPreset(vocalPatches.getSelectedId()-1);vocalPatches.setSelectedId(0,juce::dontSendNotification);}};
 hybridPage.onClick=[this]{setPage(false);};vocalPage.onClick=[this]{setPage(true);};synthetic.onClick=[this]{p.requestSample("vocal:0",selectedLayer);};midiExport.onClick=[this]{choose(3);};
 addAndMakeVisible(keys);keys.setAvailableRange(36,96);keys.setKeyWidth(22);selectLayer(0);setPage(false);setResizable(true,true);setResizeLimits(960,900,1600,1200);setSize(1080,940);startTimerHz(30);
}
GBoyEditor::~GBoyEditor(){stopTimer();p.keyboard.noteOff(1,60,0);setLookAndFeel(nullptr);}
void GBoyEditor::selectLayer(int layer){
 // Destroy old attachments first, so rebinding never writes into the previous layer.
 for(auto& a:attachments)a.reset();enableAttachment.reset();generatorAttachment.reset();filterAttachment.reset();
 for(auto& a:sourceAttachments)a.reset();for(auto& a:targetAttachments)a.reset();for(auto& a:amountAttachments)a.reset();
 for(auto& attachment:vocalAttachments)attachment.reset();preserveAttachment.reset();reverseAttachment.reset();sliceAttachment.reset();effectAttachment.reset();
 selectedLayer=layer;
 for(size_t i=0;i<3;++i)vocalAttachments[i]=std::make_unique<SliderAttachment>(p.state,gboy::parameterId(layer,gboy::vocalSpecs[i].id),vocalKnobs[i]);
 preserveAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.state,gboy::parameterId(layer,"vPreserve"),preserve);reverseAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.state,gboy::parameterId(layer,"vReverse"),reverse);
 sliceAttachment=std::make_unique<ComboAttachment>(p.state,gboy::parameterId(layer,"vSlices"),sliceMode);effectAttachment=std::make_unique<ComboAttachment>(p.state,gboy::parameterId(layer,"vEffect"),vocalEffect);
 for(size_t i=0;i<knobs.size();++i)attachments[i]=std::make_unique<SliderAttachment>(p.state,gboy::parameterId(layer,gboy::layerSpecs[i].id),knobs[i]);
 enableAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.state,gboy::parameterId(layer,"enabled"),enabled);
 generatorAttachment=std::make_unique<ComboAttachment>(p.state,gboy::parameterId(layer,"generator"),generator);filterAttachment=std::make_unique<ComboAttachment>(p.state,gboy::parameterId(layer,"filter"),filter);
 for(int row=0;row<4;++row){auto i=size_t(row);sourceAttachments[i]=std::make_unique<ComboAttachment>(p.state,gboy::routeId(layer,row,"source"),modSources[i]);targetAttachments[i]=std::make_unique<ComboAttachment>(p.state,gboy::routeId(layer,row,"target"),modTargets[i]);amountAttachments[i]=std::make_unique<SliderAttachment>(p.state,gboy::routeId(layer,row,"amount"),modAmounts[i]);}
 for(int i=0;i<4;++i)layerButtons[size_t(i)].setToggleState(i==layer,juce::dontSendNotification);
 repaint();
}
void GBoyEditor::setPage(bool vocal){
 showVocals=vocal;for(auto& component:knobs)component.setVisible(!vocal);for(auto& component:labels)component.setVisible(!vocal);
 for(auto& component:modSources)component.setVisible(!vocal);for(auto& component:modTargets)component.setVisible(!vocal);for(auto& component:modAmounts)component.setVisible(!vocal);
 matrixLabel.setVisible(!vocal);hybridPatches.setVisible(!vocal);vocalPatches.setVisible(vocal);
 for(auto& component:vocalKnobs)component.setVisible(vocal);for(auto& component:vocalLabels)component.setVisible(vocal);
 for(juce::Component* component:std::initializer_list<juce::Component*>{&preserve,&reverse,&sliceMode,&vocalEffect,&sliceLabel,&effectLabel,&vocalHelp,&synthetic,&midiExport})component->setVisible(vocal);
 hybridPage.setToggleState(!vocal,juce::dontSendNotification);vocalPage.setToggleState(vocal,juce::dontSendNotification);repaint();
}
void GBoyEditor::paint(juce::Graphics& g){
 const float w=float(getWidth()),h=float(getHeight());g.fillAll(juce::Colour(modern?0xff181e2b:0xff35244f));g.setColour(juce::Colour(modern?0xff26334a:0xff756099));g.fillRoundedRectangle(12,12,w-24,h-24,40);
 g.setColour(juce::Colour(0xff151d29));g.fillRoundedRectangle(35,58,w-70,116,16);
 g.setColour(juce::Colour(0xffa5eac5));g.setFont(26);g.drawText("G BOY",40,14,170,38,juce::Justification::centredLeft);g.setFont(14);g.drawText("PHASE 03 / VOCAL WORKSTATION",225,20,400,28,juce::Justification::centredLeft);
 g.setFont(16);g.drawText("L"+juce::String(selectedLayer+1)+" / "+(p.isBusy(selectedLayer)?"RENDERING / LOADING...":p.status(selectedLayer)),55,65,int(w)-110,24,juce::Justification::centredLeft);
 juce::Path path;for(int i=0;i<128;++i){const float x=55+float(i)*(w-110)/127,y=126-p.waveform[size_t(selectedLayer)][size_t(i)].load()*28;if(i==0)path.startNewSubPath(x,y);else path.lineTo(x,y);}g.strokePath(path,juce::PathStrokeType(2));
 if(showVocals){const auto info=p.sliceInfo(selectedLayer);g.setColour(juce::Colour(0xffd7bfff));g.setFont(10);for(int i=0;i<info.count;++i){const float x=55+float(info.edges[size_t(i)])*(w-110);g.drawVerticalLine(int(x),94,153);g.drawText(juce::String(60+i),int(x)+2,137,28,16,juce::Justification::centredLeft);}g.setFont(16);g.drawText(juce::String(info.count)+" slices / "+juce::String(info.seconds,2)+" s   |   "+(info.count>0?"MIDI 60-"+juce::String(59+info.count):"Whole phrase, chromatic"),40,602,int(w)-80,28,juce::Justification::centredLeft);}

 g.setColour(juce::Colour(0xff1f453d));g.fillRect(55,158,int(w)-110,4);g.setColour(juce::Colour(0xffa5eac5));g.fillRect(55,158,int((w-110)*juce::jlimit(0.f,1.f,p.meter.load())),4);
}
void GBoyEditor::resized(){
 const int w=getWidth(),h=getHeight(),area=w-80,col=area/5;
 for(int i=0;i<4;++i)layerButtons[size_t(i)].setBounds(40+i*125,185,115,28);
 hybridPage.setBounds(w-245,20,95,28);vocalPage.setBounds(w-140,20,100,28);
 vocalPatches.setBounds(550,185,w-780,28);hybridPatches.setBounds(550,185,w-780,28);enabled.setBounds(w-220,185,180,28);
 engineLabel.setBounds(40,225,65,26);generator.setBounds(105,225,180,28);filterLabel.setBounds(300,225,60,26);filter.setBounds(360,225,150,28);
 patches.setBounds(525,225,w-820,28);import.setBounds(w-280,225,75,28);save.setBounds(w-195,225,70,28);load.setBounds(w-115,225,75,28);
 for(int i=0;i<15;++i){const int x=40+(i%5)*col,y=265+(i/5)*102;labels[size_t(i)].setBounds(x,y,col,22);knobs[size_t(i)].setBounds(x,y+22,col,80);}
 matrixLabel.setBounds(40,574,w-80,22);
 for(size_t i=0;i<4;++i){const int y=602+int(i)*32;modSources[i].setBounds(40,y,area/3-10,27);modTargets[i].setBounds(40+area/3,y,area/3-10,27);modAmounts[i].setBounds(40+area*2/3,y,area/3,27);}
 const int vocalColumn=area/3;
 for(int i=0;i<3;++i){vocalLabels[size_t(i)].setBounds(40+i*vocalColumn,290,vocalColumn,28);vocalKnobs[size_t(i)].setBounds(40+i*vocalColumn,320,vocalColumn,120);}
 preserve.setBounds(50,460,area/2-10,30);reverse.setBounds(50+area/2,460,area/2-10,30);
 sliceLabel.setBounds(50,515,170,30);sliceMode.setBounds(230,515,w-280,30);effectLabel.setBounds(50,559,170,30);vocalEffect.setBounds(230,559,w-280,30);
 synthetic.setBounds(50,643,area/2-15,32);midiExport.setBounds(50+area/2,643,area/2-15,32);vocalHelp.setBounds(50,682,w-100,50);
 preview.setBounds(40,746,115,30);panic.setBounds(165,746,115,30);masterLabel.setBounds(300,746,65,30);master.setBounds(370,746,w-650,30);mode.setBounds(w-240,746,200,30);keys.setBounds(40,h-115,w-80,80);
}
bool GBoyEditor::isInterestedInFileDrag(const juce::StringArray& files){return files.size()==1&&juce::File(files[0]).hasFileExtension("wav;aiff;aif;flac");}
void GBoyEditor::filesDropped(const juce::StringArray& files,int,int){if(isInterestedInFileDrag(files))p.requestSample(juce::File(files[0]).getFullPathName(),selectedLayer);}
void GBoyEditor::choose(int action){
 const int targetLayer=selectedLayer;
 const bool saving=action==1||action==3;
 const juce::String title=action==0?"Import original sample into layer "+juce::String(targetLayer+1):action==1?"Save four-layer preset":action==3?"Export slice sequence (120 BPM)":"Load G Boy preset";
 chooser=std::make_unique<juce::FileChooser>(title,juce::File{},action==0?"*.wav;*.aif;*.aiff;*.flac":action==3?"*.mid":"*.gboy");
 int flags=saving?juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::warnAboutOverwriting:juce::FileBrowserComponent::openMode;flags|=juce::FileBrowserComponent::canSelectFiles;
 chooser->launchAsync(flags,[safe=juce::Component::SafePointer<GBoyEditor>(this),action,targetLayer](const juce::FileChooser& fc){if(!safe)return;const auto file=fc.getResult();if(file==juce::File{})return;
  if(action==0)safe->p.requestSample(file.getFullPathName(),targetLayer);
  else if(action==1){juce::MemoryBlock block;safe->p.getStateInformation(block);if(!file.withFileExtension("gboy").replaceWithData(block.getData(),block.getSize()))juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,"Save failed","Could not write preset.");}
  else if(action==3){if(!safe->p.exportSlices(file.withFileExtension("mid"),targetLayer))juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon,"MIDI export unavailable","Choose a slice mapping and wait for rendering to finish, then export to a writable folder.");}
  else{juce::MemoryBlock block;if(file.getSize()<1024*1024&&file.loadFileAsData(block))safe->p.setStateInformation(block.getData(),int(block.getSize()));}
 });
}
