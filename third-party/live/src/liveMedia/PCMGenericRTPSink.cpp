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
// RTP sink for AAC ADTS audio
// Implementation

#include "PCMGenericRTPSink.hh"
#include "Locale.hh"
#include <ctype.h> // needed on some systems to define "tolower()"

namespace {
static char const* generic_rtpPayloadFormatName(u_int32_t type, u_int32_t rtpTimestampFrequency) {
  // g711a
  if (type == 19) {
    return "PCMA";
  }
  // g711u
  else if (type == 20) {
    return "PCMU";
  }
  // g726
  else if (type == 21) {
    if (rtpTimestampFrequency <= 16000) {
      return "G726-16";
    }
    else if (rtpTimestampFrequency <= 24000) {
      return "G726-24";
    }
    else if (rtpTimestampFrequency <= 32000) {
      return "G726-32";
    }
    else if (rtpTimestampFrequency <= 40000) {
      return "G726-40";
    }
    else {
      return "G726-40";
    }
  }
  // OPUS
  else if (type == 1007) {
    return "OPUS";
  }

  // FIXME.
  return "PCM";
}
};

PCMGenericRTPSink
::PCMGenericRTPSink(UsageEnvironment& env, Groupsock* RTPgs,
		      u_int8_t rtpPayloadFormat,
		      u_int32_t rtpTimestampFrequency,
		      unsigned numChannels,
		      signed type)
  : MultiFramedRTPSink(env, RTPgs, rtpPayloadFormat,
		       rtpTimestampFrequency,
		       generic_rtpPayloadFormatName(type, rtpTimestampFrequency), numChannels), 
  fAllowMultipleFramesPerPacket(False), fSetMBitOnNextPacket(False) {
  fSDPMediaTypeString = "audio";
  fSetMBitOnLastFrames = False;

  // Set up the "a=fmtp:" SDP line for this stream:
  char const* fmtpFmt =
    "a=fmtp:%d "
    "streamtype=%d\r\n";
  unsigned fmtpFmtSize = strlen(fmtpFmt)
    + 3 /* max char len */
    + 3 /* max char len */;
  char* fmtp = new char[fmtpFmtSize];
  sprintf(fmtp, fmtpFmt,
    rtpPayloadType(),
    strcmp(fSDPMediaTypeString, "video") == 0 ? 4 : 5);
  fFmtpSDPLine = strDup(fmtp);

  delete[] fmtp;
}

PCMGenericRTPSink::~PCMGenericRTPSink() {
  delete[] fFmtpSDPLine;
}

PCMGenericRTPSink*
PCMGenericRTPSink::createNew(UsageEnvironment& env, Groupsock* RTPgs,
			       u_int8_t rtpPayloadFormat,
			       u_int32_t rtpTimestampFrequency,
			       unsigned numChannels,
			       signed type) {
  return new PCMGenericRTPSink(env, RTPgs, rtpPayloadFormat,
				 rtpTimestampFrequency, numChannels, type);
}

void PCMGenericRTPSink::doSpecialFrameHandling(unsigned fragmentationOffset,
                  unsigned char* frameStart,
                  unsigned numBytesInFrame,
                  struct timeval framePresentationTime,
                  unsigned numRemainingBytes) {
  if (numRemainingBytes == 0) {
    // This packet contains the last (or only) fragment of the frame.
    // Set the RTP 'M' ('marker') bit, if appropriate:
    if (fSetMBitOnLastFrames) setMarkerBit();
  }

  if (fSetMBitOnNextPacket) {
    // An external object has asked for the 'M' bit to be set on the next packet:
    setMarkerBit();
    fSetMBitOnNextPacket = False;
  }

  // Important: Also call our base class's doSpecialFrameHandling(),
  // to set the packet's timestamp:
  MultiFramedRTPSink::doSpecialFrameHandling(fragmentationOffset,
                      frameStart, numBytesInFrame,
                      framePresentationTime,
                      numRemainingBytes);
}

Boolean PCMGenericRTPSink::
frameCanAppearAfterPacketStart(unsigned char const* /*frameStart*/,
              unsigned /*numBytesInFrame*/) const {
  return fAllowMultipleFramesPerPacket;
}

char const* PCMGenericRTPSink::sdpMediaType() const {
  return fSDPMediaTypeString;
}

char const* PCMGenericRTPSink::auxSDPLine() {
  return fFmtpSDPLine;
}
