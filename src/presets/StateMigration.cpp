#include "StateMigration.h"
#include <unordered_map>
#include <cmath>
#include <cstdlib>
namespace gboy::presets {
juce::ValueTree migrateState(const juce::XmlElement& xml,const juce::ValueTree& defaults,const juce::AudioProcessorValueTreeState& parameters){
 if(!xml.hasTagName("GBOY"))return {};const int version=xml.getIntAttribute("version",1);if(version<1||version>stateVersion)return {};
 auto input=juce::ValueTree::fromXml(xml);if(!input.isValid()||input.getNumChildren()>4096)return {};
 auto migrated=defaults.createCopy();std::unordered_map<std::string,juce::ValueTree> nodes;for(int i=0;i<migrated.getNumChildren();++i){auto child=migrated.getChild(i);nodes.emplace(child.getProperty("id").toString().toStdString(),child);}
 for(int i=0;i<input.getNumChildren();++i){const auto child=input.getChild(i);const auto id=child.getProperty("id").toString();const auto found=nodes.find(id.toStdString());auto* parameter=parameters.getParameter(id);if(found==nodes.end()||!parameter||!child.hasProperty("value"))continue;const auto stored=child.getProperty("value");double number=0;if(stored.isString()){const auto text=stored.toString().trim();if(text.isEmpty())continue;const auto* begin=text.toRawUTF8();char* end=nullptr;number=std::strtod(begin,&end);if(end==begin||*end!='\0')continue;}else if(stored.isInt()||stored.isInt64()||stored.isDouble()||stored.isBool())number=double(stored);else continue;
  if(!std::isfinite(number))continue;const auto range=parameter->getNormalisableRange();const float bounded=float(std::clamp(number,double(range.start),double(range.end)));const float value=parameter->convertFrom0to1(parameter->convertTo0to1(bounded));if(std::isfinite(value))found->second.setProperty("value",value,nullptr);
 }
 migrated.setProperty("version",stateVersion,nullptr);return migrated;
}
}
