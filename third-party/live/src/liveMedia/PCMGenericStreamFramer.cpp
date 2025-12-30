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
// A filter that reads PCM Generic frames, and outputs each frame with
// Implementation

#include "PCMGenericStreamFramer.hh"

PCMGenericStreamFramer* PCMGenericStreamFramer
::createNew(UsageEnvironment& env, FramedSource* inputSource, u_int32_t sampleRate) {
  return new PCMGenericStreamFramer(env, inputSource, sampleRate);
}

PCMGenericStreamFramer
::PCMGenericStreamFramer(UsageEnvironment& env, FramedSource* inputSource, u_int32_t sampleRate)
  : FramedFilter(env, inputSource) {
  fSampleRate = sampleRate;
  fPresentationTimeBase.tv_sec = 0;
  fPresentationTimeBase.tv_usec = 0;
  fNextPresentationTime.tv_sec = 0;
  fNextPresentationTime.tv_usec = 0;
  fPresentationTimeBaseSpecified.tv_sec = 0;
  fPresentationTimeBaseSpecified.tv_usec = 0;
}

PCMGenericStreamFramer::~PCMGenericStreamFramer() {
}

#define MILLION 1000000

struct timeval PCMGenericStreamFramer::currentFramePlayTime() const {
  unsigned const numSamples = 1024; // FIXME.
  unsigned const freq = fSampleRate;

  // result is numSamples/freq
  unsigned const uSeconds = (freq == 0) ? 0
    : ((numSamples*2*MILLION)/freq + 1)/2; // rounds to nearest integer

  struct timeval result;
  result.tv_sec = uSeconds/MILLION;
  result.tv_usec = uSeconds%MILLION;
  return result;
}

void PCMGenericStreamFramer::setPresentationTime(const struct timeval &presentationTime, const unsigned &durationInMicroseconds) {
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

void PCMGenericStreamFramer::computeNextPresentationTime() {
  // Note that the presentation time for the next NAL unit will be different:
  struct timeval framePlayTime = currentFramePlayTime();
  fDurationInMicroseconds = framePlayTime.tv_sec*MILLION + framePlayTime.tv_usec;
  fNextPresentationTime.tv_usec += framePlayTime.tv_usec;
  fNextPresentationTime.tv_sec
    += framePlayTime.tv_sec + fNextPresentationTime.tv_usec/MILLION;
  fNextPresentationTime.tv_usec %= MILLION;
}

void PCMGenericStreamFramer::doGetNextFrame() {
  fInputSource->getNextFrame(fTo, fMaxSize,
                 afterGettingFrame, this,
                 FramedSource::handleClosure, this);
}

void PCMGenericStreamFramer
::afterGettingFrame(void* clientData, unsigned frameSize,
		    unsigned numTruncatedBytes,
		    struct timeval presentationTime,
		    unsigned durationInMicroseconds) {
  PCMGenericStreamFramer* source = (PCMGenericStreamFramer*)clientData;
  source->afterGettingFrame1(frameSize, numTruncatedBytes, presentationTime, durationInMicroseconds);
}

void PCMGenericStreamFramer
::afterGettingFrame1(unsigned frameSize, unsigned numTruncatedBytes,
		     struct timeval presentationTime,
		     unsigned durationInMicroseconds) {
  fFrameSize = frameSize;

  setPresentationTime(presentationTime, durationInMicroseconds);

  computeNextPresentationTime();

  // Complete delivery to the downstream object:
  fNumTruncatedBytes = numTruncatedBytes;
  afterGetting(this);
}
