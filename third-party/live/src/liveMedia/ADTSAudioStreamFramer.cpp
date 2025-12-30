/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 3 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
// "liveMedia"
// Copyright (c) 1996-2020 Live Networks, Inc.  All rights reserved.
// A filter that reads AAC audio frames, and outputs each frame with
// a preceding ADTS header.
// Implementation

#include "ADTSAudioStreamFramer.hh"

struct adtsHeader {
  unsigned int syncword;  //12 bit 'FFF'
  unsigned int id;        //1 bit  0 for MPEG-4，1 for MPEG-2
  unsigned int layer;     //2 bit '00'
  unsigned int protectionAbsent;  //1 bit 1: without crc，0: with crc
  unsigned int profile;           //1 bit AAC object type
  unsigned int samplingFreqIndex; //4 bit samplerate
  unsigned int privateBit;        //1 bit
  unsigned int channelCfg; //3 bit channel cnts
  unsigned int originalCopy;         //1 bit 
  unsigned int home;                  //1 bit 

  unsigned int copyrightIdentificationBit;   //1 bit
  unsigned int copyrightIdentificationStart; //1 bit
  unsigned int aacFrameLength;               //13 bit
  unsigned int adtsBufferFullness;           //11 bi

  unsigned int numberOfRawDataBlockInFrame; //2 bit
};

static unsigned const samplingFrequencyTable[16] = {
  96000, 88200, 64000, 48000,
  44100, 32000, 24000, 22050,
  16000, 12000, 11025, 8000,
  7350, 0, 0, 0
};

namespace {
static Boolean parseAdtsHeader(uint8_t* in, struct adtsHeader* res) {
  memset(res,0,sizeof(*res));
  
  if ((in[0] == 0xFF)&&((in[1] & 0xF0) == 0xF0)) {
    res->id = ((unsigned int) in[1] & 0x08) >> 3;
    res->layer = ((unsigned int) in[1] & 0x06) >> 1;
    res->protectionAbsent = (unsigned int) in[1] & 0x01;
    res->profile = ((unsigned int) in[2] & 0xc0) >> 6;
    res->samplingFreqIndex = ((unsigned int) in[2] & 0x3c) >> 2;
    res->privateBit = ((unsigned int) in[2] & 0x02) >> 1;
    res->channelCfg = ((((unsigned int) in[2] & 0x01) << 2) | (((unsigned int) in[3] & 0xc0) >> 6));
    res->originalCopy = ((unsigned int) in[3] & 0x20) >> 5;
    res->home = ((unsigned int) in[3] & 0x10) >> 4;
    res->copyrightIdentificationBit = ((unsigned int) in[3] & 0x08) >> 3;
    res->copyrightIdentificationStart = (unsigned int) in[3] & 0x04 >> 2;
    res->aacFrameLength = (((((unsigned int) in[3]) & 0x03) << 11) |
                            (((unsigned int)in[4] & 0xFF) << 3) |
                                ((unsigned int)in[5] & 0xE0) >> 5) ;
    res->adtsBufferFullness = (((unsigned int) in[5] & 0x1f) << 6 |
                                    ((unsigned int) in[6] & 0xfc) >> 2);
    res->numberOfRawDataBlockInFrame = ((unsigned int) in[6] & 0x03);

    return True;
  } else {
    return False;
  }
}
};

ADTSAudioStreamFramer* ADTSAudioStreamFramer
::createNew(UsageEnvironment& env, FramedSource* inputSource, u_int32_t sampleRate) {
  return new ADTSAudioStreamFramer(env, inputSource, sampleRate);
}

ADTSAudioStreamFramer
::ADTSAudioStreamFramer(UsageEnvironment& env, FramedSource* inputSource, u_int32_t sampleRate)
  : FramedFilter(env, inputSource) {
  fSampleRate = sampleRate;
  fPresentationTimeBase.tv_sec = 0;
  fPresentationTimeBase.tv_usec = 0;
  fNextPresentationTime.tv_sec = 0;
  fNextPresentationTime.tv_usec = 0;
  fPresentationTimeBaseSpecified.tv_sec = 0;
  fPresentationTimeBaseSpecified.tv_usec = 0;
}

ADTSAudioStreamFramer::~ADTSAudioStreamFramer() {
}

#define MILLION 1000000

struct timeval ADTSAudioStreamFramer::currentFramePlayTime() const {
  unsigned const numSamples = 1024;
  unsigned const freq = fSampleRate;

  // result is numSamples/freq
  unsigned const uSeconds = (freq == 0) ? 0
    : ((numSamples*2*MILLION)/freq + 1)/2; // rounds to nearest integer

  struct timeval result;
  result.tv_sec = uSeconds/MILLION;
  result.tv_usec = uSeconds%MILLION;
  return result;
}

void ADTSAudioStreamFramer::setPresentationTime(const struct timeval &presentationTime, const unsigned &durationInMicroseconds) {
  if (fPresentationTimeBase.tv_sec == 0 && fPresentationTimeBase.tv_usec == 0) {
    // Set to the current time:
    gettimeofday(&fPresentationTimeBase, NULL);
    fNextPresentationTime = fPresentationTimeBase;
  }

  auto sourcePresentationTimeSpecified = fInputSource->getPresentationTimeSpecified();
  if (sourcePresentationTimeSpecified.tv_sec == 0 && sourcePresentationTimeSpecified.tv_usec == 0) {
    fPresentationTime = fNextPresentationTime;
  }
  else {
    if (fPresentationTimeBaseSpecified.tv_sec == 0 && fPresentationTimeBaseSpecified.tv_usec == 0) {
      // Set to the current time:
      fPresentationTimeBaseSpecified = sourcePresentationTimeSpecified;
      fPresentationTime = fNextPresentationTime;
    }
    else {
      long long basePts = (long long)fPresentationTimeBase.tv_sec * MILLION + fPresentationTimeBase.tv_usec;
      long long sourceBasePts = (long long)fPresentationTimeBaseSpecified.tv_sec * MILLION + fPresentationTimeBaseSpecified.tv_usec;
      long long sourceCurPts = (long long)sourcePresentationTimeSpecified.tv_sec * MILLION + sourcePresentationTimeSpecified.tv_usec;

      sourceCurPts = basePts + (sourceCurPts - sourceBasePts);

      fPresentationTime.tv_sec = sourceCurPts / MILLION;
      fPresentationTime.tv_usec = sourceCurPts % MILLION;
    }
  }
}

void ADTSAudioStreamFramer::computeNextPresentationTime() {
  // Note that the presentation time for the next NAL unit will be different:
  struct timeval framePlayTime = currentFramePlayTime();
  fDurationInMicroseconds = framePlayTime.tv_sec*MILLION + framePlayTime.tv_usec;
  fNextPresentationTime.tv_usec += framePlayTime.tv_usec;
  fNextPresentationTime.tv_sec
    += framePlayTime.tv_sec + fNextPresentationTime.tv_usec/MILLION;
  fNextPresentationTime.tv_usec %= MILLION;
}

void ADTSAudioStreamFramer::doGetNextFrame() {
  fInputSource->getNextFrame(fTo, fMaxSize,
                 afterGettingFrame, this,
                 FramedSource::handleClosure, this);
}

void ADTSAudioStreamFramer
::afterGettingFrame(void* clientData, unsigned frameSize,
		    unsigned numTruncatedBytes,
		    struct timeval presentationTime,
		    unsigned durationInMicroseconds) {
  ADTSAudioStreamFramer* source = (ADTSAudioStreamFramer*)clientData;
  source->afterGettingFrame1(frameSize, numTruncatedBytes, presentationTime, durationInMicroseconds);
}

void ADTSAudioStreamFramer
::afterGettingFrame1(unsigned frameSize, unsigned numTruncatedBytes,
		     struct timeval presentationTime,
		     unsigned durationInMicroseconds) {
  fFrameSize = frameSize;

  // parser
  struct adtsHeader header;
  parseAdtsHeader(fTo, &header);

  if (header.samplingFreqIndex < 16) {
    fSampleRate = samplingFrequencyTable[header.samplingFreqIndex];
  }

  setPresentationTime(presentationTime, durationInMicroseconds);

  computeNextPresentationTime();

  // Complete delivery to the downstream object:
  fNumTruncatedBytes = numTruncatedBytes;
  afterGetting(this);
}