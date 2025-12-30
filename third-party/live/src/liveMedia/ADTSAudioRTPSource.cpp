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
// AAC ADTS Audio RTP Sources
// Implementation

#include "ADTSAudioRTPSource.hh"

ADTSAudioRTPSource*
ADTSAudioRTPSource::createNew(UsageEnvironment& env,
			      Groupsock* RTPgs,
			      unsigned char rtpPayloadFormat,
			      unsigned rtpTimestampFrequency) {
  return new ADTSAudioRTPSource(env, RTPgs, rtpPayloadFormat,
				rtpTimestampFrequency);
}

ADTSAudioRTPSource::ADTSAudioRTPSource(UsageEnvironment& env,
				       Groupsock* rtpGS,
				       unsigned char rtpPayloadFormat,
				       unsigned rtpTimestampFrequency)
  : MultiFramedRTPSource(env, rtpGS,
			 rtpPayloadFormat, rtpTimestampFrequency) {
}

ADTSAudioRTPSource::~ADTSAudioRTPSource() {
}

Boolean ADTSAudioRTPSource
::processSpecialHeader(BufferedPacket* packet,
		       unsigned& resultSpecialHeaderSize) {
  unsigned packetSize = packet->dataSize();

  // There's a 2-byte payload header at the beginning:
  if (packetSize < 7) return False;
  resultSpecialHeaderSize = 7;

  return True;
}

char const* ADTSAudioRTPSource::MIMEtype() const {
  return "audio/aac";
}
