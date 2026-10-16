#pragma once
#include "../PluginProcessor.h"
class StepGrid final:public juce::Component {
 GBoyProcessor& processor;int page=0,selected=0;
 float read(int bank,int step,const char* field) const {return processor.state.getRawParameterValue(gboy::seq::stepId(bank,step,field))->load();}
 void write(int bank,int step,const char* field,float value){auto* parameter=processor.state.getParameter(gboy::seq::stepId(bank,step,field));parameter->setValueNotifyingHost(parameter->convertTo0to1(value));}
 void edit(const juce::MouseEvent& event){
  const int bank=int(processor.state.getRawParameterValue("seq_bank")->load()),length=16<<int(processor.state.getRawParameterValue("seq_length")->load());
  const float width=float(getWidth()-32)/16;const int column=juce::jlimit(0,15,int((event.position.x-32)/width)),step=page*16+column;if(event.position.x<32||step>=length)return;selected=step;if(onSelect)onSelect(step);
  if(event.mods.isRightButtonDown()){write(bank,step,"enabled",read(bank,step,"enabled")>.5f?0.f:1.f);}
  else if(event.position.y>=getHeight()-22)write(bank,step,"velocity",juce::jlimit(.05f,1.f,1-(event.position.y-float(getHeight()-22))/22));
  else{const int pitch=juce::jlimit(-12,12,12-int(event.position.y/(float(getHeight()-22)/25)));write(bank,step,"pitch",float(pitch));write(bank,step,"enabled",1);}
  repaint();
 }
public:
 std::function<void(int)> onSelect;
 explicit StepGrid(GBoyProcessor& p):processor(p){}
 void setPage(int next){page=juce::jlimit(0,3,next);selected=page*16;if(onSelect)onSelect(selected);repaint();}
 void mouseDown(const juce::MouseEvent& event) override {edit(event);}
 void mouseDrag(const juce::MouseEvent& event) override {if(event.mods.isLeftButtonDown())edit(event);}
 void paint(juce::Graphics& g) override {
  g.fillAll(juce::Colour(0xff15212a));const int bank=int(processor.state.getRawParameterValue("seq_bank")->load()),length=16<<int(processor.state.getRawParameterValue("seq_length")->load());
  const float width=float(getWidth()-32)/16,row=float(getHeight()-22)/25;
  g.setFont(10);for(int i=0;i<25;++i){const int pitch=12-i;g.setColour(juce::Colour(pitch%12==0?0xff386354:0xff243740));g.drawHorizontalLine(int(i*row),32,float(getWidth()));if(pitch%6==0){g.setColour(juce::Colours::lightgrey);g.drawText(juce::String(pitch),0,int(i*row),30,int(row)+2,juce::Justification::centredRight);}}
  for(int column=0;column<16;++column){const int step=page*16+column;const float x=32+column*width;g.setColour(juce::Colour(column%4==0?0xff667789:0xff33434c));g.drawVerticalLine(int(x),0,float(getHeight()));
   if(step>=length){g.setColour(juce::Colour(0xaa101010));g.fillRect(x,0.f,width,float(getHeight()));continue;}
   if(processor.playingBank.load()==bank&&processor.playingStep.load()==step){g.setColour(juce::Colour(0x4448ffac));g.fillRect(x,0.f,width,float(getHeight()));}
   if(read(bank,step,"enabled")>.5f){const int pitch=juce::jlimit(-12,12,int(read(bank,step,"pitch")));g.setColour(juce::Colour(step==selected?0xffd8b5ff:0xff97edc9));g.fillRoundedRectangle(x+2,(12-pitch)*row+1,width-4,std::max(2.f,row-1),2);const float velocity=read(bank,step,"velocity");g.fillRect(x+2,float(getHeight()-2)-velocity*17,width-4,velocity*17);}
   g.setColour(juce::Colours::white);g.drawText(juce::String(step+1),int(x),getHeight()-22,int(width),16,juce::Justification::centred);
   if(step==selected){g.setColour(juce::Colour(0xffd8b5ff));g.drawRect(juce::Rectangle<float>(x,0,width,float(getHeight())),1.5f);}
  }
 }
};
