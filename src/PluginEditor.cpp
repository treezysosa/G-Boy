#include "PluginEditor.h"
GBoyEditor::GBoyEditor(GBoyProcessor& processor):AudioProcessorEditor(processor),p(processor),keys(p.keyboard,juce::MidiKeyboardComponent::horizontalKeyboard){
 setLookAndFeel(&look);look.setColour(juce::Slider::rotarySliderFillColourId,juce::Colour(0xff99e8ce));look.setColour(juce::TextButton::buttonColourId,juce::Colour(0xff4b386e));
 const char* ids[]={"attack","decay","sustain","release","volume"};
 for(size_t i=0;i<5;++i){auto& s=knobs[i];s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);s.setTextBoxStyle(juce::Slider::TextBoxBelow,false,80,20);addAndMakeVisible(s);labels[i].setText(juce::String(ids[i]).toUpperCase(),juce::dontSendNotification);labels[i].setJustificationType(juce::Justification::centred);addAndMakeVisible(labels[i]);attachments[i]=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.state,ids[i],s);}
 patches.addItemList({"Chip Pulse","Soft Keys","FM Bell","Arcade Drum"},1);patches.setSelectedId(1,juce::dontSendNotification);patches.onChange=[this]{p.requestSample("factory:"+juce::String(patches.getSelectedId()-1));};addAndMakeVisible(patches);
 for(auto* b:{&import,&save,&load,&preview,&panic,&mode})addAndMakeVisible(b);
 import.onClick=[this]{choose(0);};save.onClick=[this]{choose(1);};load.onClick=[this]{choose(2);};
 preview.onStateChange=[this]{if(preview.isDown())p.keyboard.noteOn(1,60,.85f);else p.keyboard.noteOff(1,60,0.f);};
 panic.onClick=[this]{for(int ch=1;ch<=16;++ch)p.keyboard.allNotesOff(ch);p.panicRequested.store(true);};mode.onClick=[this]{modern=!modern;repaint();};
 addAndMakeVisible(keys);keys.setAvailableRange(36,96);keys.setKeyWidth(22);setResizable(true,true);setResizeLimits(760,620,1400,1000);setSize(920,620);startTimerHz(30);
}
void GBoyEditor::paint(juce::Graphics& g){
 auto w=float(getWidth()),h=float(getHeight());g.fillAll(juce::Colour(modern?0xff181e2b:0xff35244f));
 g.setColour(juce::Colour(modern?0xff26334a:0xff756099));g.fillRoundedRectangle(12,12,w-24,h-24,50);
 g.setColour(juce::Colour(0xff151d29));g.fillRoundedRectangle(35,62,w-70,160,18);
 g.setColour(juce::Colour(0xffa5eac5));g.setFont(26);g.drawText("G BOY",40,18,180,40,juce::Justification::centredLeft);
 g.setFont(14);g.drawText("PHASE 01 / SAMPLE INSTRUMENT",220,24,400,25,juce::Justification::centredLeft);
 g.setFont(17);g.drawText(p.status(),55,75,int(w)-110,30,juce::Justification::centredLeft);
 juce::Path path;for(int i=0;i<128;++i){float x=55+float(i)*(w-110)/127,y=153-p.waveform[size_t(i)].load()*50;if(i==0)path.startNewSubPath(x,y);else path.lineTo(x,y);}g.strokePath(path,juce::PathStrokeType(2));
 g.setColour(juce::Colour(0xff1f453d));g.fillRect(55,199,int(w)-110,5);g.setColour(juce::Colour(0xffa5eac5));g.fillRect(55,199,int((w-110)*juce::jlimit(0.f,1.f,p.meter.load())),5);
}
void GBoyEditor::resized(){int w=getWidth(),h=getHeight();patches.setBounds(40,235,w-450,30);import.setBounds(w-395,235,105,30);save.setBounds(w-280,235,105,30);load.setBounds(w-165,235,105,30);
 int width=(w-80)/5;for(int i=0;i<5;++i){labels[size_t(i)].setBounds(40+i*width,280,width,22);knobs[size_t(i)].setBounds(40+i*width,302,width,110);}
 preview.setBounds(40,425,120,32);panic.setBounds(170,425,120,32);mode.setBounds(w-240,425,200,32);keys.setBounds(40, h-135,w-80,95);}
bool GBoyEditor::isInterestedInFileDrag(const juce::StringArray& files){return files.size()==1&&juce::File(files[0]).hasFileExtension("wav;aiff;aif;flac");}
void GBoyEditor::filesDropped(const juce::StringArray& files,int,int){if(isInterestedInFileDrag(files))p.requestSample(juce::File(files[0]).getFullPathName());}
void GBoyEditor::choose(int action){
 chooser=std::make_unique<juce::FileChooser>(action==0?"Import sample (root C4 / MIDI 60, max 30 seconds)":action==1?"Save G Boy preset":"Load G Boy preset",juce::File{},action==0?"*.wav;*.aif;*.aiff;*.flac":"*.gboy");
 int flags=action==1?juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::warnAboutOverwriting:juce::FileBrowserComponent::openMode;flags|=juce::FileBrowserComponent::canSelectFiles;
 chooser->launchAsync(flags,[safe=juce::Component::SafePointer<GBoyEditor>(this),action](const juce::FileChooser& fc){if(!safe)return;auto file=fc.getResult();if(file==juce::File{})return;
  if(action==0)safe->p.requestSample(file.getFullPathName());else if(action==1){juce::MemoryBlock block;safe->p.getStateInformation(block);if(!file.withFileExtension("gboy").replaceWithData(block.getData(),block.getSize()))juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,"Save failed","Could not write preset.");}
  else{juce::MemoryBlock block;if(file.getSize()<1024*1024&&file.loadFileAsData(block))safe->p.setStateInformation(block.getData(),int(block.getSize()));}
 });
}
