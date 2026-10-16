#pragma once
#include "../audio/Engine.h"
#include <cmath>
namespace gboy::vocal {
// 0: phrase, 1: grid 8, 2: grid 16, 3: energy onsets. Maximum 32 slices.
inline void slice(Sample& sample,int mode){
 sample.sliceCount=0;sample.sliceEdges={};const size_t count=std::min(sample.left.size(),sample.right.size());if(count<2||mode==0)return;
 auto& edges=sample.sliceEdges;edges[0]=0;
 if(mode==1||mode==2){sample.sliceCount=int(std::min(count/2,size_t(mode==1?8:16)));for(int i=1;i<=sample.sliceCount;++i)edges[size_t(i)]=count*size_t(i)/size_t(sample.sliceCount);return;}
 const size_t window=std::max(size_t(8),size_t(sample.rate*.005));const size_t distance=std::max(window,size_t(sample.rate*.06));
 float previous=0,baseline=0;size_t last=0;sample.sliceCount=1;
 for(size_t pos=0;pos<count;pos+=window){double sum=0;const size_t end=std::min(count,pos+window);for(size_t i=pos;i<end;++i)sum+=(double(sample.left[i])*sample.left[i]+double(sample.right[i])*sample.right[i])*.5;
  const float rms=float(std::sqrt(sum/double(end-pos)));
  if(rms>.015f&&rms>std::max(previous*1.7f,baseline*2.f)&&pos>=last+distance&&sample.sliceCount<32&&count-pos>=distance){edges[size_t(sample.sliceCount++)]=pos;last=pos;}
  previous=rms;baseline=baseline*.9f+rms*.1f;
 }
 edges[size_t(sample.sliceCount)]=count;
}
}
