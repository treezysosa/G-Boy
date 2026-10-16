#pragma once
#include "LibraryManager.h"
#include <juce_audio_utils/juce_audio_utils.h>
class GBoyProcessor;
class LibraryBrowser final:public juce::Component,private juce::ListBoxModel,private juce::Timer,public juce::FileDragAndDropTarget {
public:
 explicit LibraryBrowser(GBoyProcessor&,std::function<int()> layer);~LibraryBrowser() override;
 void resized() override;void paint(juce::Graphics&) override;
 bool isInterestedInFileDrag(const juce::StringArray&) override;void filesDropped(const juce::StringArray&,int,int) override;
private:
 GBoyProcessor& processor;std::function<int()> layer;gboy::library::Snapshot snapshot;std::vector<gboy::library::Entry> results;juce::String selectedId,similarId;uint64_t revision=~uint64_t(0);
 juce::TextEditor query,tags,author,license,provenance;juce::ComboBox category,kind,libraries,editCategory;juce::Slider root;
 juce::ListBox list;juce::ToggleButton favorites{"Favorites only"},favorite{"Favorite"};
 juce::TextButton load{"LOAD TO LAYER"},preview{"PREVIEW"},stop{"STOP PREVIEW"},similar{"SIMILAR"},clearSimilar{"CLEAR SIMILAR"},saveMetadata{"SAVE METADATA"},add{"IMPORT FILES"},folder{"ADD FOLDER"},install{"INSTALL PACK"},relocate{"RELOCATE"},rescan{"CHECK FILES"},exportPack{"EXPORT LIBRARY"},savePatch{"SAVE PATCH"};
 juce::Label details,status,help,tagLabel,authorLabel,licenseLabel,sourceLabel,rootLabel;std::unique_ptr<juce::FileChooser> chooser;
 int getNumRows() override{return int(results.size());}void paintListBoxItem(int,juce::Graphics&,int,int,bool) override;void selectedRowsChanged(int) override;void listBoxItemDoubleClicked(int,const juce::MouseEvent&) override;
 void timerCallback() override;void refresh(bool rebuildMenus=false);void selection();void choose(int);gboy::library::Entry selected() const;
};
