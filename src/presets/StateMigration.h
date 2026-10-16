#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
namespace gboy::presets {
inline constexpr int stateVersion=6;
// Builds a fresh complete state before applying any edits. Unknown IDs are ignored;
// missing IDs retain defaults; non-finite values cannot enter the DSP.
juce::ValueTree migrateState(const juce::XmlElement&,const juce::ValueTree& defaults,const juce::AudioProcessorValueTreeState&);
}
