#include "PluginEditor.h"
GBoyEditor::GBoyEditor(GBoyProcessor& processor):AudioProcessorEditor(processor),p(processor),keys(p.keyboard,juce::MidiKeyboardComponent::horizontalKeyboard),grid(p){
 setLookAndFeel(&look);look.setColour(juce::Slider::rotarySliderFillColourId,juce::Colour(0xff99e8ce));look.setColour(juce::TextButton::buttonColourId,juce::Colour(0xff4b386e));look.setColour(juce::TextButton::buttonOnColourId,juce::Colour(0xff24675f));
 for(size_t i=0;i<knobs.size();++i){auto& slider=knobs[i];slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);slider.setTextBoxStyle(juce::Slider::TextBoxBelow,false,84,20);slider.setNumDecimalPlacesToDisplay(i==6||i==7?1:3);addAndMakeVisible(slider);labels[i].setText(gboy::layerSpecs[i].label,juce::dontSendNotification);labels[i].setJustificationType(juce::Justification::centred);addAndMakeVisible(labels[i]);}
 for(int i=0;i<4;++i){auto& button=layerButtons[size_t(i)];button.setButtonText("LAYER "+juce::String(i+1));button.onClick=[this,i]{selectLayer(i);};addAndMakeVisible(button);}
 generator.addItemList({"Sampler","Virtual analog","Wavetable","FM","Granular","Vocal"},1);filter.addItemList({"Bypass","Low pass","Band pass","High pass"},1);
 for(auto* combo:{&generator,&filter,&patches,&hybridPatches})addAndMakeVisible(combo);
 addAndMakeVisible(enabled);engineLabel.setText("ENGINE",juce::dontSendNotification);filterLabel.setText("FILTER",juce::dontSendNotification);matrixLabel.setText("MOD MATRIX  |  SOURCE  >  VOICE TARGET  >  FX OVERRIDE  >  AMOUNT",juce::dontSendNotification);
 for(auto* label:{&engineLabel,&filterLabel,&matrixLabel,&masterLabel})addAndMakeVisible(label);
 masterLabel.setText("MASTER",juce::dontSendNotification);master.setSliderStyle(juce::Slider::LinearHorizontal);master.setTextBoxStyle(juce::Slider::TextBoxRight,false,60,22);addAndMakeVisible(master);masterAttachment=std::make_unique<SliderAttachment>(p.state,"volume",master);
 hybridPatches.addItemList({"Handheld Stack","Glacier Grains","Indigo Bass","FM Prism"},1);hybridPatches.setTextWhenNothingSelected("Hybrid patches...");hybridPatches.onChange=[this]{if(hybridPatches.getSelectedId()>0){p.loadHybridPreset(hybridPatches.getSelectedId()-1);hybridPatches.setSelectedId(0,juce::dontSendNotification);}};
 patches.addItemList({"Chip Pulse","Soft Keys","FM Bell","Arcade Drum"},1);patches.setTextWhenNothingSelected("Load factory sample...");patches.onChange=[this]{if(patches.getSelectedId()>0){p.requestSample("factory:"+juce::String(patches.getSelectedId()-1),selectedLayer);patches.setSelectedId(0,juce::dontSendNotification);}};
 for(size_t i=0;i<4;++i){modSources[i].addItemList({"Off","LFO","Envelope","Velocity","Mod wheel","Note","Macro"},1);modTargets[i].addItemList({"Pitch","Cutoff","Gain","Pan","Grain position","Shape / FM index"},1);modFxTargets[i].addItemList({"Voice destination","Layer FX mix","Layer FX amount","Master FX mix","Master FX amount"},1);addAndMakeVisible(modFxTargets[i]);modAmounts[i].setSliderStyle(juce::Slider::LinearHorizontal);modAmounts[i].setTextBoxStyle(juce::Slider::TextBoxRight,false,60,22);addAndMakeVisible(modSources[i]);addAndMakeVisible(modTargets[i]);addAndMakeVisible(modAmounts[i]);}
 for(auto* button:{&import,&save,&load,&preview,&panic,&mode})addAndMakeVisible(button);
 import.onClick=[this]{choose(0);};save.onClick=[this]{choose(1);};load.onClick=[this]{choose(2);};
 preview.onStateChange=[this]{if(preview.isDown())p.keyboard.noteOn(1,60,.85f);else p.keyboard.noteOff(1,60,0.f);};
 panic.onClick=[this]{for(int ch=1;ch<=16;++ch)p.keyboard.allNotesOff(ch);p.stopSequence();};mode.onClick=[this]{modern=!modern;repaint();};
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
 setupPhase4();
 addAndMakeVisible(keys);keys.setAvailableRange(36,96);keys.setKeyWidth(22);selectLayer(0);setPage(false);setResizable(true,true);setResizeLimits(960,900,1600,1200);setSize(1080,940);startTimerHz(30);
}
GBoyEditor::~GBoyEditor(){stopTimer();p.keyboard.noteOff(1,60,0);setLookAndFeel(nullptr);}
void GBoyEditor::selectLayer(int layer){
 // Destroy old attachments first, so rebinding never writes into the previous layer.
 for(auto& a:attachments)a.reset();enableAttachment.reset();generatorAttachment.reset();filterAttachment.reset();
 for(auto& a:sourceAttachments)a.reset();for(auto& a:targetAttachments)a.reset();for(auto& a:amountAttachments)a.reset();for(auto& a:fxTargetAttachments)a.reset();
 for(auto& attachment:vocalAttachments)attachment.reset();preserveAttachment.reset();reverseAttachment.reset();sliceAttachment.reset();effectAttachment.reset();
 selectedLayer=layer;
 for(size_t i=0;i<3;++i)vocalAttachments[i]=std::make_unique<SliderAttachment>(p.state,gboy::parameterId(layer,gboy::vocalSpecs[i].id),vocalKnobs[i]);
 preserveAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.state,gboy::parameterId(layer,"vPreserve"),preserve);reverseAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.state,gboy::parameterId(layer,"vReverse"),reverse);
 sliceAttachment=std::make_unique<ComboAttachment>(p.state,gboy::parameterId(layer,"vSlices"),sliceMode);effectAttachment=std::make_unique<ComboAttachment>(p.state,gboy::parameterId(layer,"vEffect"),vocalEffect);
 for(size_t i=0;i<knobs.size();++i)attachments[i]=std::make_unique<SliderAttachment>(p.state,gboy::parameterId(layer,gboy::layerSpecs[i].id),knobs[i]);
 enableAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.state,gboy::parameterId(layer,"enabled"),enabled);
 generatorAttachment=std::make_unique<ComboAttachment>(p.state,gboy::parameterId(layer,"generator"),generator);filterAttachment=std::make_unique<ComboAttachment>(p.state,gboy::parameterId(layer,"filter"),filter);
 for(int row=0;row<4;++row){auto i=size_t(row);sourceAttachments[i]=std::make_unique<ComboAttachment>(p.state,gboy::routeId(layer,row,"source"),modSources[i]);targetAttachments[i]=std::make_unique<ComboAttachment>(p.state,gboy::routeId(layer,row,"target"),modTargets[i]);fxTargetAttachments[i]=std::make_unique<ComboAttachment>(p.state,gboy::routeId(layer,row,"fxTarget"),modFxTargets[i]);amountAttachments[i]=std::make_unique<SliderAttachment>(p.state,gboy::routeId(layer,row,"amount"),modAmounts[i]);}
 for(int i=0;i<4;++i)layerButtons[size_t(i)].setToggleState(i==layer,juce::dontSendNotification);
 bindEffects();
 repaint();
}
void GBoyEditor::setPage(int next){
 page=next;const bool vocal=page==1;showVocals=vocal;for(auto& component:knobs)component.setVisible(page==0);for(auto& component:labels)component.setVisible(page==0);
 for(auto& component:modSources)component.setVisible(page==0);for(auto& component:modTargets)component.setVisible(page==0);for(auto& component:modFxTargets)component.setVisible(page==0);for(auto& component:modAmounts)component.setVisible(page==0);
 matrixLabel.setVisible(page==0);hybridPatches.setVisible(page==0);vocalPatches.setVisible(vocal);
 for(auto& component:vocalKnobs)component.setVisible(vocal);for(auto& component:vocalLabels)component.setVisible(vocal);
 for(juce::Component* component:std::initializer_list<juce::Component*>{&preserve,&reverse,&sliceMode,&vocalEffect,&sliceLabel,&effectLabel,&vocalHelp,&synthetic,&midiExport})component->setVisible(vocal);
 for(auto* component:{&generator,&filter,&patches})component->setVisible(page<2);
 engineLabel.setVisible(page<2);filterLabel.setVisible(page<2);import.setVisible(page<2);
 masterRack.setVisible(page==2);rackPresets.setVisible(page==2);rackSave.setVisible(page==2);rackLoad.setVisible(page==2);effectsHelp.setVisible(page==2);
 for(size_t i=0;i<4;++i){effectTypes[i].setVisible(page==2);effectBypass[i].setVisible(page==2);effectUp[i].setVisible(page==2);effectDown[i].setVisible(page==2);effectSlotLabels[i].setVisible(page==2);for(size_t field=0;field<4;++field){effectSliders[i][field].setVisible(page==2);effectLabels[i][field].setVisible(page==2);}}
 grid.setVisible(page==3);for(auto& component:seqFlags)component.setVisible(page==3);for(auto& component:seqChoices)component.setVisible(page==3);for(auto& component:seqControls)component.setVisible(page==3);for(auto& component:seqControlLabels)component.setVisible(page==3);for(auto& component:stepControls)component.setVisible(page==3);for(auto& component:stepLabels)component.setVisible(page==3);
 for(juce::Component* component:std::initializer_list<juce::Component*>{&stepEnabled,&seqPresets,&seqViewport,&seqExport,&seqRandom,&seqStateLabel,&stepTitle,&seqHelp})component->setVisible(page==3);
 effectsPage.setToggleState(page==2,juce::dontSendNotification);sequencePage.setToggleState(page==3,juce::dontSendNotification);
 hybridPage.setToggleState(page==0,juce::dontSendNotification);vocalPage.setToggleState(vocal,juce::dontSendNotification);repaint();
}
void GBoyEditor::paint(juce::Graphics& g){
 const float w=float(getWidth()),h=float(getHeight());g.fillAll(juce::Colour(modern?0xff181e2b:0xff35244f));g.setColour(juce::Colour(modern?0xff26334a:0xff756099));g.fillRoundedRectangle(12,12,w-24,h-24,40);
 g.setColour(juce::Colour(0xff151d29));g.fillRoundedRectangle(35,58,w-70,116,16);
 g.setColour(juce::Colour(0xffa5eac5));g.setFont(26);g.drawText("G BOY",40,14,170,38,juce::Justification::centredLeft);g.setFont(14);g.drawText("PHASE 04 / FX + SEQUENCER",225,20,300,28,juce::Justification::centredLeft);
 g.setFont(16);g.drawText("L"+juce::String(selectedLayer+1)+" / "+(p.isBusy(selectedLayer,false)?"RENDERING / LOADING...":p.status(selectedLayer)),55,65,int(w)-110,24,juce::Justification::centredLeft);
 juce::Path path;for(int i=0;i<128;++i){const float x=55+float(i)*(w-110)/127,y=126-p.waveform[size_t(selectedLayer)][size_t(i)].load()*28;if(i==0)path.startNewSubPath(x,y);else path.lineTo(x,y);}g.strokePath(path,juce::PathStrokeType(2));
 if(showVocals){const auto info=p.sliceInfo(selectedLayer);g.setColour(juce::Colour(0xffd7bfff));g.setFont(10);for(int i=0;i<info.count;++i){const float x=55+float(info.edges[size_t(i)])*(w-110);g.drawVerticalLine(int(x),94,153);g.drawText(juce::String(60+i),int(x)+2,137,28,16,juce::Justification::centredLeft);}g.setFont(16);g.drawText(juce::String(info.count)+" slices / "+juce::String(info.seconds,2)+" s   |   "+(info.count>0?"MIDI 60-"+juce::String(59+info.count):"Whole phrase, chromatic"),40,602,int(w)-80,28,juce::Justification::centredLeft);}

 g.setColour(juce::Colour(0xff1f453d));g.fillRect(55,158,int(w)-110,4);g.setColour(juce::Colour(0xffa5eac5));g.fillRect(55,158,int((w-110)*juce::jlimit(0.f,1.f,p.meter.load())),4);
}
void GBoyEditor::resized(){
 const int w=getWidth(),h=getHeight(),area=w-80,col=area/5;
 for(int i=0;i<4;++i)layerButtons[size_t(i)].setBounds(40+i*125,185,115,28);
 hybridPage.setBounds(w-400,20,80,28);vocalPage.setBounds(w-310,20,80,28);effectsPage.setBounds(w-220,20,80,28);sequencePage.setBounds(w-130,20,90,28);
 vocalPatches.setBounds(550,185,w-780,28);hybridPatches.setBounds(550,185,w-780,28);enabled.setBounds(w-220,185,180,28);
 engineLabel.setBounds(40,225,65,26);generator.setBounds(105,225,180,28);filterLabel.setBounds(300,225,60,26);filter.setBounds(360,225,150,28);
 patches.setBounds(525,225,w-820,28);import.setBounds(w-280,225,75,28);save.setBounds(w-195,225,70,28);load.setBounds(w-115,225,75,28);
 for(int i=0;i<15;++i){const int x=40+(i%5)*col,y=265+(i/5)*102;labels[size_t(i)].setBounds(x,y,col,22);knobs[size_t(i)].setBounds(x,y+22,col,80);}
 matrixLabel.setBounds(40,574,w-80,22);
 for(size_t i=0;i<4;++i){const int y=602+int(i)*32;modSources[i].setBounds(40,y,area/4-10,27);modTargets[i].setBounds(40+area/4,y,area/4-10,27);modFxTargets[i].setBounds(40+area*2/4,y,area/4-10,27);modAmounts[i].setBounds(40+area*3/4,y,area/4,27);}
 const int vocalColumn=area/3;
 for(int i=0;i<3;++i){vocalLabels[size_t(i)].setBounds(40+i*vocalColumn,290,vocalColumn,28);vocalKnobs[size_t(i)].setBounds(40+i*vocalColumn,320,vocalColumn,120);}
 preserve.setBounds(50,460,area/2-10,30);reverse.setBounds(50+area/2,460,area/2-10,30);
 sliceLabel.setBounds(50,515,170,30);sliceMode.setBounds(230,515,w-280,30);effectLabel.setBounds(50,559,170,30);vocalEffect.setBounds(230,559,w-280,30);
 synthetic.setBounds(50,643,area/2-15,32);midiExport.setBounds(50+area/2,643,area/2-15,32);vocalHelp.setBounds(50,682,w-100,50);
 rackPresets.setBounds(550,185,w-780,28);masterRack.setBounds(40,225,180,28);rackSave.setBounds(w-490,225,130,28);rackLoad.setBounds(w-350,225,130,28);
 for(int slot=0;slot<4;++slot){const int y=280+slot*95;effectSlotLabels[size_t(slot)].setBounds(40,y,75,25);effectTypes[size_t(slot)].setBounds(120,y,260,25);effectBypass[size_t(slot)].setBounds(400,y,120,25);effectUp[size_t(slot)].setBounds(w-155,y,50,25);effectDown[size_t(slot)].setBounds(w-100,y,50,25);for(int field=0;field<4;++field){const int x=40+field*area/4;effectLabels[size_t(slot)][size_t(field)].setBounds(x,y+27,area/4-10,20);effectSliders[size_t(slot)][size_t(field)].setBounds(x,y+48,area/4-10,26);}}
 effectsHelp.setBounds(40,680,w-80,45);
 seqPresets.setBounds(550,185,w-780,28);for(int i=0;i<5;++i)seqFlags[size_t(i)].setBounds(40+i*100,225,100,28);seqChoices[2].setBounds(550,225,w-800,28);
 seqChoices[0].setBounds(40,265,110,28);seqChoices[1].setBounds(160,265,100,28);seqChoices[4].setBounds(270,265,115,28);seqChoices[3].setBounds(395,265,180,28);seqViewport.setBounds(585,265,140,28);
 for(int i=0;i<5;++i){const int x=40+i*col;seqControlLabels[size_t(i)].setBounds(x,303,col-10,20);seqControls[size_t(i)].setBounds(x,324,col-10,28);stepLabels[size_t(i)].setBounds(x,578,col-10,20);stepControls[size_t(i)].setBounds(x,598,col-10,65);}
 grid.setBounds(40,365,w-80,190);stepTitle.setBounds(40,557,w-80,20);stepEnabled.setBounds(40,674,160,27);seqStateLabel.setBounds(220,674,w-300,27);seqRandom.setBounds(40,710,170,30);seqExport.setBounds(w-260,710,220,30);seqHelp.setBounds(230,706,w-520,34);
 preview.setBounds(40,746,115,30);panic.setBounds(165,746,115,30);masterLabel.setBounds(300,746,65,30);master.setBounds(370,746,w-650,30);mode.setBounds(w-240,746,200,30);keys.setBounds(40,h-115,w-80,80);
}
bool GBoyEditor::isInterestedInFileDrag(const juce::StringArray& files){return files.size()==1&&juce::File(files[0]).hasFileExtension("wav;aiff;aif;flac");}
void GBoyEditor::filesDropped(const juce::StringArray& files,int,int){if(isInterestedInFileDrag(files))p.requestSample(juce::File(files[0]).getFullPathName(),selectedLayer);}
void GBoyEditor::choose(int action){
 const int targetLayer=selectedLayer,targetRack=rackIndex();const bool saving=action==1||action==3||action==4||action==6;
 const juce::String title=action==0?"Import original sample":action==1?"Save G Boy preset":action==3?"Export slice MIDI":action==4?"Save effect rack":action==5?"Load effect rack":action==6?"Export pattern MIDI":"Load G Boy preset";
 chooser=std::make_unique<juce::FileChooser>(title,juce::File{},action==0?"*.wav;*.aif;*.aiff;*.flac":action==3||action==6?"*.mid":action==4||action==5?"*.gbfx":"*.gboy");
 int flags=saving?juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::warnAboutOverwriting:juce::FileBrowserComponent::openMode;flags|=juce::FileBrowserComponent::canSelectFiles;
 chooser->launchAsync(flags,[safe=juce::Component::SafePointer<GBoyEditor>(this),action,targetLayer,targetRack](const juce::FileChooser& fc){if(!safe)return;const auto file=fc.getResult();if(file==juce::File{})return;bool success=true;
  if(action==0)safe->p.requestSample(file.getFullPathName(),targetLayer);
  else if(action==1){juce::MemoryBlock block;safe->p.getStateInformation(block);success=file.withFileExtension("gboy").replaceWithData(block.getData(),block.getSize());}
  else if(action==3)success=safe->p.exportSlices(file.withFileExtension("mid"),targetLayer);
  else if(action==4)success=safe->p.saveRack(file.withFileExtension("gbfx"),targetRack);
  else if(action==5)success=safe->p.loadRack(file,targetRack);
  else if(action==6)success=safe->p.exportSequence(file.withFileExtension("mid"));
  else{juce::MemoryBlock block;success=file.getSize()<1024*1024&&file.loadFileAsData(block);if(success)safe->p.setStateInformation(block.getData(),int(block.getSize()));}
  if(!success)juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,"Operation unavailable","Check the file and destination. Slice export requires completed slicing; rack files must use the GBFX format.");
 });
}

void GBoyEditor::setupPhase4(){
 addAndMakeVisible(effectsPage);addAndMakeVisible(sequencePage);effectsPage.onClick=[this]{setPage(2);};sequencePage.onClick=[this]{setPage(3);};
 for(auto* component:{&rackSave,&rackLoad})addAndMakeVisible(component);addAndMakeVisible(masterRack);addAndMakeVisible(rackPresets);addAndMakeVisible(effectsHelp);
 masterRack.onClick=[this]{bindEffects();};rackSave.onClick=[this]{choose(4);};rackLoad.onClick=[this]{choose(5);};
 rackPresets.addItemList({"Init rack","Tape Dream","Crushed Cassette","Shimmer Cloud","Pump Delay","Factory Room"},1);rackPresets.setTextWhenNothingSelected("FX presets...");rackPresets.onChange=[this]{if(rackPresets.getSelectedId()>0){p.loadRackPreset(rackIndex(),rackPresets.getSelectedId()-1);rackPresets.setSelectedId(0,juce::dontSendNotification);updateEffectLabels();}};
 effectsHelp.setText("Four serial slots per layer and master. UP/DN changes order. Layer/master FX modulation is assigned in HYBRID > MOD MATRIX. Factory IR convolution uses an original sparse room impulse; ducking follows the musical clock.",juce::dontSendNotification);
 for(size_t slot=0;slot<4;++slot){
  juce::StringArray types;for(const auto& spec:gboy::fx::specs)types.add(spec.name);effectTypes[slot].addItemList(types,1);effectTypes[slot].onChange=[this]{updateEffectLabels();};effectBypass[slot].setButtonText("Bypass");effectSlotLabels[slot].setText("SLOT "+juce::String(int(slot)+1),juce::dontSendNotification);effectUp[slot].setButtonText("UP");effectDown[slot].setButtonText("DN");effectUp[slot].onClick=[this,slot]{p.moveEffect(rackIndex(),int(slot),-1);};effectDown[slot].onClick=[this,slot]{p.moveEffect(rackIndex(),int(slot),1);};
  addAndMakeVisible(effectTypes[slot]);addAndMakeVisible(effectBypass[slot]);addAndMakeVisible(effectSlotLabels[slot]);addAndMakeVisible(effectUp[slot]);addAndMakeVisible(effectDown[slot]);
  for(size_t field=0;field<4;++field){auto& slider=effectSliders[slot][field];slider.setSliderStyle(juce::Slider::LinearHorizontal);slider.setTextBoxStyle(juce::Slider::TextBoxRight,false,85,23);slider.textFromValueFunction=[this,slot,field](double normalized){if(field==0)return juce::String(normalized*100,1)+" %";const int type=juce::jlimit(0,int(gboy::fx::Type::Count)-1,effectTypes[slot].getSelectedId()-1);const float value=gboy::fx::physical(gboy::fx::Type(type),field-1,float(normalized));return juce::String(value,std::abs(value)>=100?0:2)+" "+gboy::fx::specs[size_t(type)].parameters[field-1].unit;};slider.valueFromTextFunction=[this,slot,field](const juce::String& text){const double value=text.getDoubleValue();if(field==0)return juce::jlimit(0.,1.,value/100);const int type=juce::jlimit(0,int(gboy::fx::Type::Count)-1,effectTypes[slot].getSelectedId()-1);const auto& spec=gboy::fx::specs[size_t(type)].parameters[field-1];return juce::jlimit(0.,1.,spec.log?std::log(std::max(double(spec.low),value)/spec.low)/std::log(spec.high/spec.low):(value-spec.low)/(spec.high-spec.low));};addAndMakeVisible(slider);addAndMakeVisible(effectLabels[slot][field]);}
 }
 const char* flagIds[]={"enabled","sync","run","latch","chain"};const char* flagNames[]={"Enable","Host sync","Run","Latch","Chain"};
 for(size_t i=0;i<5;++i){seqFlags[i].setButtonText(flagNames[i]);addAndMakeVisible(seqFlags[i]);seqFlagAttachments[i]=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.state,"seq_"+juce::String(flagIds[i]),seqFlags[i]);}
 seqChoices[0].addItemList({"16 steps","32 steps","64 steps"},1);seqChoices[1].addItemList({"Bank A","Bank B","Bank C","Bank D"},1);seqChoices[2].addItemList({"Step","Arp up","Arp down","Arp up-down","Chord"},1);seqChoices[3].addItemList({"Chromatic","Major","Natural minor","Dorian","Major pentatonic"},1);seqChoices[4].addItemList({"1/8 notes","1/16 notes","1/32 notes"},1);
 const char* choiceIds[]={"seq_length","seq_bank","seq_mode","seq_scale","seq_division"};for(size_t i=0;i<5;++i){addAndMakeVisible(seqChoices[i]);seqChoiceAttachments[i]=std::make_unique<ComboAttachment>(p.state,choiceIds[i],seqChoices[i]);}
 seqChoices[1].onChange=[this]{bindStep(selectedStep);grid.repaint();};seqChoices[0].onChange=[this]{const int pages=1<<juce::jlimit(0,2,seqChoices[0].getSelectedId()-1);if(seqViewport.getSelectedId()>pages)seqViewport.setSelectedId(1);};
 const char* names[]={"Tempo (BPM)","Root MIDI","Swing","Key (0=C)","Arp octaves"};for(size_t i=0;i<5;++i){seqControls[i].setSliderStyle(juce::Slider::LinearHorizontal);seqControls[i].setTextBoxStyle(juce::Slider::TextBoxRight,false,55,22);seqControls[i].setNumDecimalPlacesToDisplay(i==0||i==2?2:0);seqControlLabels[i].setText(names[i],juce::dontSendNotification);addAndMakeVisible(seqControls[i]);addAndMakeVisible(seqControlLabels[i]);seqControlAttachments[i]=std::make_unique<SliderAttachment>(p.state,gboy::seq::controls[i].id,seqControls[i]);}
 const char* stepNames[]={"Pitch offset","Velocity","Gate","Probability","Ratchets"};for(size_t i=0;i<5;++i){stepControls[i].setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);stepControls[i].setTextBoxStyle(juce::Slider::TextBoxBelow,false,80,22);stepControls[i].setNumDecimalPlacesToDisplay(i==0||i==4?0:2);stepLabels[i].setText(stepNames[i],juce::dontSendNotification);stepLabels[i].setJustificationType(juce::Justification::centred);addAndMakeVisible(stepControls[i]);addAndMakeVisible(stepLabels[i]);}
 addAndMakeVisible(grid);grid.onSelect=[this](int step){bindStep(step);};addAndMakeVisible(seqViewport);seqViewport.addItemList({"Steps 1-16","Steps 17-32","Steps 33-48","Steps 49-64"},1);seqViewport.setSelectedId(1,juce::dontSendNotification);seqViewport.onChange=[this]{grid.setPage(seqViewport.getSelectedId()-1);};
 addAndMakeVisible(seqPresets);seqPresets.addItemList({"Arcade Steps","Minor Arp","Trap Dice","Chord Pulse"},1);seqPresets.setTextWhenNothingSelected("Pattern presets...");seqPresets.onChange=[this]{if(seqPresets.getSelectedId()>0){p.loadSequencePreset(seqPresets.getSelectedId()-1);seqPresets.setSelectedId(0,juce::dontSendNotification);seqViewport.setSelectedId(1,juce::dontSendNotification);grid.setPage(0);}};
 addAndMakeVisible(stepEnabled);addAndMakeVisible(stepTitle);addAndMakeVisible(seqStateLabel);addAndMakeVisible(seqHelp);addAndMakeVisible(seqRandom);addAndMakeVisible(seqExport);seqRandom.onClick=[this]{p.randomizePattern(juce::jlimit(0,3,seqChoices[1].getSelectedId()-1));};seqExport.onClick=[this]{choose(6);};seqHelp.setText("Click/drag notes; right-click disables. Velocity bars below. Host sync overrides Run.",juce::dontSendNotification);bindStep(0);
}
void GBoyEditor::bindEffects(){
 for(auto& attachment:effectTypeAttachments)attachment.reset();for(auto& attachment:effectBypassAttachments)attachment.reset();for(auto& row:effectAttachments)for(auto& attachment:row)attachment.reset();
 const int rack=rackIndex();const char* fields[]={"mix","a","b","c"};for(int slot=0;slot<4;++slot){const auto i=size_t(slot);effectTypeAttachments[i]=std::make_unique<ComboAttachment>(p.state,gboy::fx::parameterId(rack,slot,"type"),effectTypes[i]);effectBypassAttachments[i]=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.state,gboy::fx::parameterId(rack,slot,"bypass"),effectBypass[i]);for(size_t field=0;field<4;++field)effectAttachments[i][field]=std::make_unique<SliderAttachment>(p.state,gboy::fx::parameterId(rack,slot,fields[field]),effectSliders[i][field]);}updateEffectLabels();
}
void GBoyEditor::updateEffectLabels(){
 for(size_t row=0;row<4;++row)modTargets[row].setEnabled(modFxTargets[row].getSelectedId()<=1);
 for(size_t slot=0;slot<4;++slot){const int type=juce::jlimit(0,int(gboy::fx::Type::Count)-1,effectTypes[slot].getSelectedId()-1);effectLabels[slot][0].setText("Wet / dry",juce::dontSendNotification);for(size_t field=0;field<4;++field){if(field>0)effectLabels[slot][field].setText(gboy::fx::specs[size_t(type)].parameters[field-1].name,juce::dontSendNotification);effectSliders[slot][field].setEnabled(type>0);effectSliders[slot][field].updateText();}effectUp[slot].setEnabled(slot>0);effectDown[slot].setEnabled(slot<3);}
}
void GBoyEditor::bindStep(int step){
 selectedStep=juce::jlimit(0,63,step);const int bank=juce::jlimit(0,3,seqChoices[1].getSelectedId()-1);for(auto& attachment:stepAttachments)attachment.reset();stepEnableAttachment.reset();const char* fields[]={"pitch","velocity","gate","probability","ratchet"};for(size_t i=0;i<5;++i)stepAttachments[i]=std::make_unique<SliderAttachment>(p.state,gboy::seq::stepId(bank,selectedStep,fields[i]),stepControls[i]);stepEnableAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.state,gboy::seq::stepId(bank,selectedStep,"enabled"),stepEnabled);stepTitle.setText("BANK "+juce::String::charToString(juce::juce_wchar('A'+bank))+" / STEP "+juce::String(selectedStep+1),juce::dontSendNotification);
}
void GBoyEditor::refreshSequence(){
 const int step=p.playingStep.load(),bank=p.playingBank.load();const auto text=step>=0?(p.hostSyncActive.load()?"HOST SYNC":"INTERNAL CLOCK")+juce::String(" / BANK ")+juce::String::charToString(juce::juce_wchar('A'+std::max(0,bank)))+" / STEP "+juce::String(step+1):juce::String("Enable + Run, or Host sync + DAW play");seqStateLabel.setText(text,juce::dontSendNotification);seqControls[0].setEnabled(!p.hostSyncActive.load());
}
