#pragma once
#include <juce_audio_formats/juce_audio_formats.h>
#include "Analysis.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <functional>
namespace gboy::library {
struct Entry {
 juce::String id,name,kind="sample",category="SYNTH",subcategory,description,tags,author,license,provenance,human="unspecified",language,tradition,region;
 juce::String libraryId,path,relative,preview,action;int factoryIndex=-1,root=60;bool favorite=false,missing=false,analysed=false;Analysis analysis;
};
struct Root {juce::String id,name,path;};
struct Snapshot {std::vector<Entry> entries;std::vector<Root> roots;juce::String status;uint64_t revision=0;bool busy=false;};
struct Query {juce::String text,category,kind,libraryId;bool favorites=false;juce::String similarTo;};
std::vector<Entry> search(const Snapshot&,const Query&);
std::vector<Entry> factoryEntries();
class LibraryManager {
public:
 explicit LibraryManager(juce::File home=defaultHome());~LibraryManager();
 static juce::File defaultHome();
 static std::shared_ptr<LibraryManager> shared(juce::File home=defaultHome());
 uint64_t revision() const;
 Snapshot snapshot() const;Entry find(const juce::String&) const;
 void importFiles(std::vector<juce::File>);void scanFolder(juce::File);void installPack(juce::File);
 void relocate(juce::String libraryId,juce::File folder);void rescan();
 void edit(juce::String id,juce::String category,juce::String tags,juce::String author,juce::String license,juce::String provenance,int root);
 void favorite(juce::String id,bool);void exportLibrary(juce::String libraryId,juce::File destination);
 bool waitUntilIdle(int milliseconds=30000) const;
private:
 juce::File home;mutable std::mutex mutex;mutable std::condition_variable changed;Snapshot view;std::deque<std::function<void()>> jobs;bool quitting=false;std::thread worker;
 // The worker alone owns the mutable index. Audio processing never calls this manager.
 Snapshot index;void enqueue(std::function<void()>);void run();void publish();void persist();void restore();
 void indexFolder(const Root&);void readPack(juce::File,bool install);Entry decode(juce::File,const Root&,juce::String relative,const Entry* metadata=nullptr);
};
}
