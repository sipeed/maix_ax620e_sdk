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

#include "ADTSAudioRTPSink.hh"
#include "Locale.hh"
#include <ctype.h> // needed on some systems to define "tolower()"

static unsigned const samplingFrequencyTable[16] = {
  96000, 88200, 64000, 48000,
  44100, 32000, 24000, 22050,
  16000, 12000, 11025, 8000,
  7350, 0, 0, 0
};

ADTSAudioRTPSink
::ADTSAudioRTPSink(UsageEnvironment& env, Groupsock* RTPgs,
		      u_int8_t rtpPayloadFormat,
		      u_int32_t rtpTimestampFrequency,
		      unsigned numChannels,
		      signed aacType)
  : MultiFramedRTPSink(env, RTPgs, rtpPayloadFormat,
		       rtpTimestampFrequency, "MPEG4-GENERIC", numChannels) {
  fSDPMediaTypeString = "audio";
  fMPEG4Mode = "AAC-hbr";

  u_int8_t samplingFrequencyIndex = 0;
  u_int8_t profile = (aacType <= 0) ? 0 : aacType - 1;
  u_int8_t channelConfiguration = numChannels;
  for (uint8_t i = 0 ; i < sizeof(samplingFrequencyTable)/sizeof(samplingFrequencyTable[0]); i++) {
    if (samplingFrequencyTable[i] == rtpTimestampFrequency) {
      samplingFrequencyIndex = i;
      break;
    }
  }

  // "configStr" should be a 4-character hexadecimal string for a 2-byte value
  char configStr[5] = {0};
  sprintf(configStr, "%02x%02x", (uint8_t)((profile + 1) << 3)|(samplingFrequencyIndex >> 1),
          (uint8_t)((samplingFrequencyIndex << 7)|(channelConfiguration << 3)));

  // Set up the "a=fmtp:" SDP line for this stream:
  char const* fmtpFmt =
    "a=fmtp:%d "
    "streamtype=%d;profile-level-id=1;"
    "mode=%s;sizelength=13;indexlength=3;indexdeltalength=3;"
    "config=%s\r\n";
  unsigned fmtpFmtSize = strlen(fmtpFmt)
    + 3 /* max char len */
    + 3 /* max char len */
    + strlen(fMPEG4Mode)
    + strlen(configStr);
  char* fmtp = new char[fmtpFmtSize];
  sprintf(fmtp, fmtpFmt,
    rtpPayloadType(),
    strcmp(fSDPMediaTypeString, "video") == 0 ? 4 : 5,
    fMPEG4Mode,
    configStr);
  fFmtpSDPLine = strDup(fmtp);

  delete[] fmtp;
}

ADTSAudioRTPSink::~ADTSAudioRTPSink() {
  delete[] fFmtpSDPLine;
}

ADTSAudioRTPSink*
ADTSAudioRTPSink::createNew(UsageEnvironment& env, Groupsock* RTPgs,
			       u_int8_t rtpPayloadFormat,
			       u_int32_t rtpTimestampFrequency,
			       unsigned numChannels,
			       signed aacType) {
  return new ADTSAudioRTPSink(env, RTPgs, rtpPayloadFormat,
				 rtpTimestampFrequency, numChannels, aacType);
}

Boolean ADTSAudioRTPSink
::frameCanAppearAfterPacketStart(unsigned char const* /*frameStart*/,
                                 unsigned /*numBytesInFrame*/) const {
  // (For now) allow at most 1 frame in a single packet:
  return False;
}

void ADTSAudioRTPSink
::doSpecialFrameHandling(unsigned fragmentationOffset,
			 unsigned char* frameStart,
			 unsigned numBytesInFrame,
			 struct timeval framePresentationTime,
			 unsigned numRemainingBytes) {
  // Set the 2-byte "payload header", as defined in RFC 3640.
  unsigned char headers[4];

  if (numRemainingBytes == 0) {
    // This packet contains the last (or only) fragment of the frame.
    // Set the RTP 'M' ('marker') bit:
    setMarkerBit();
  }

  headers[0] = 0x00;
  headers[1] = 0x10;
  headers[2] = (numBytesInFrame & 0x1FE0) >> 5;
  headers[3] = (numBytesInFrame & 0x1F) << 3;

  setSpecialHeaderBytes(headers, sizeof headers);

  // Important: Also call our base class's doSpecialFrameHandling(),
  // to set the packet's timestamp:
  MultiFramedRTPSink::doSpecialFrameHandling(fragmentationOffset,
					     frameStart, numBytesInFrame,
					     framePresentationTime,
					     numRemainingBytes);
}

unsigned ADTSAudioRTPSink::specialHeaderSize() const {
  return 4;
}

char const* ADTSAudioRTPSink::sdpMediaType() const {
  return fSDPMediaTypeString;
}

char const* ADTSAudioRTPSink::auxSDPLine() {
  return fFmtpSDPLine;
}
