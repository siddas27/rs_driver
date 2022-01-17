/*********************************************************************************************************************
Copyright (c) 2020 RoboSense
All rights reserved

By downloading, copying, installing or using the software you agree to this license. If you do not agree to this
license, do not download, install, copy or use the software.

License Agreement
For RoboSense LiDAR SDK Library
(3-clause BSD License)

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following
disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following
disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the names of the RoboSense, nor Suteng Innovation Technology, nor the names of other contributors may be used
to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
PECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*********************************************************************************************************************/

#pragma once
#include <rs_driver/driver/decoder/decoder_mech.hpp>

namespace robosense
{
namespace lidar
{

#pragma pack(push, 1)

typedef struct
{
  uint8_t id[2];
  uint16_t azimuth;
  RSChannel channels[32];
} RSHELIOSMsopBlock;

typedef struct
{
  uint8_t id[4];
  uint16_t protocol_version;
  uint8_t reserved1[14];
  RSTimestampUTC timestamp;
  uint8_t lidar_type;
  uint8_t reserved2[7];
  RSTemperature temp;
  uint8_t reserved3[2];
} RSHELIOSMsopHeader;

typedef struct
{
  RSHELIOSMsopHeader header;
  RSHELIOSMsopBlock blocks[12];
  unsigned int index;
  uint16_t tail;
} RSHELIOSMsopPkt;

typedef struct
{
  uint8_t id[8];
  uint16_t rpm;
  RSEthNetV2 eth;
  RSFOV fov;
  uint8_t reserved1[2];
  uint16_t phase_lock_angle;
  RSVersionV2 version;
  uint8_t reserved2[229];
  RSSN sn;
  uint16_t zero_cali;
  uint8_t return_mode;
  RSTimeInfo time_info;
  RSStatusV2 status;
  uint8_t reserved3[5];
  RSDiagnoV2 diagno;
  uint8_t gprmc[86];
  RSCalibrationAngle vert_angle_cali[32];
  RSCalibrationAngle horiz_angle_cali[32];
  uint8_t reserved4[586];
  uint16_t tail;
} RSHELIOSDifopPkt;

#pragma pack(pop)

class DecoderRSHELIOS : public DecoderMech
{
public:
  virtual void decodeDifopPkt(const uint8_t* pkt, size_t size);
  virtual bool decodeMsopPkt(const uint8_t* pkt, size_t size);
  virtual ~DecoderRSHELIOS() = default;

  explicit DecoderRSHELIOS(const RSDecoderParam& param, 
      const std::function<void(const Error&)>& excb);

#ifndef UNIT_TEST
protected:
#endif

  static RSDecoderMechConstParam& getConstParam();
  static RSEchoMode getEchoMode(uint8_t mode);

  template <typename T_BlockDiff>
  bool internDecodeMsopPkt(const uint8_t* pkt, size_t size);
};

inline RSDecoderMechConstParam& DecoderRSHELIOS::getConstParam()
{
  static RSDecoderMechConstParam param = 
  {
    1248 // msop len
      , 1248 // difop len
      , 4 // msop id len
      , 8 // difop id len
      , {0x55, 0xAA, 0x05, 0x5A} // msop id
    , {0xA5, 0xFF, 0x00, 0x5A, 0x11, 0x11, 0x55, 0x55} // difop id
    , {0xFF, 0xEE} // block id
    , 12 // blocks per packet
      , 32 // channels per block
      , 0.4f // distance min
      , 200.0f // distance max
      , 0.0025f // distance resolution
      , 0.0625f // temperature resolution

      // lens center
      , 0.03498f // RX
      , -0.015f // RY
      , 0.0f // RZ
  };

  INIT_ONLY_ONCE();

  float blk_ts = 55.56f;
  float firing_tss[] = 
  {
    0.00f,  1.57f,  3.15f,  4.72f,  6.30f,  7.87f,  9.45f, 11.36f, 
    13.26f, 15.17f, 17.08f, 18.99f, 20.56f, 22.14f, 23.71f, 25.29f,
    26.53f, 29.01f, 27.77f, 30.25f, 31.49f, 33.98f, 32.73f, 35.22f, 
    36.46f, 37.70f, 38.94f, 40.18f, 41.42f, 42.67f, 43.91f, 45.15f
  };

  param.BLOCK_DURATION = blk_ts / 1000000;
  for (uint16_t i = 0; i < sizeof(firing_tss)/sizeof(firing_tss[0]); i++)
  {
    param.CHAN_TSS[i] = (double)firing_tss[i] / 1000000;
    param.CHAN_AZIS[i] = firing_tss[i] / blk_ts;
  }

  return param;
}

inline RSEchoMode DecoderRSHELIOS::getEchoMode(uint8_t mode)
{
  switch (mode)
  {
    case 0x00: // dual return
      return RSEchoMode::ECHO_DUAL;
    case 0x04: // strongest return
    case 0x05: // last return
    case 0x06: // nearest return
    default:
      return RSEchoMode::ECHO_SINGLE;
  }
}

inline DecoderRSHELIOS::DecoderRSHELIOS(const RSDecoderParam& param,
      const std::function<void(const Error&)>& excb)
  : DecoderMech(getConstParam(), param, excb)
{
}

inline void DecoderRSHELIOS::decodeDifopPkt(const uint8_t* packet, size_t size)
{
  const RSHELIOSDifopPkt& pkt = *(const RSHELIOSDifopPkt*)(packet);
  this->template decodeDifopCommon<RSHELIOSDifopPkt>(pkt);

  this->echo_mode_ = getEchoMode (pkt.return_mode);
  this->split_blks_per_frame_ = (this->echo_mode_ == RSEchoMode::ECHO_DUAL) ? 
    (this->blks_per_frame_ << 1) : this->blks_per_frame_;
}

inline bool DecoderRSHELIOS::decodeMsopPkt(const uint8_t* pkt, size_t size)
{
  if (this->echo_mode_ == RSEchoMode::ECHO_SINGLE)
  {
    return internDecodeMsopPkt<SingleReturnBlockDiff<RSHELIOSMsopPkt>>(pkt, size);
  }
  else
  {
    return internDecodeMsopPkt<DualReturnBlockDiff<RSHELIOSMsopPkt>>(pkt, size);
  }
}

template <typename T_BlockDiff>
inline bool DecoderRSHELIOS::internDecodeMsopPkt(const uint8_t* packet, size_t size)
{
  const RSHELIOSMsopPkt& pkt = *(const RSHELIOSMsopPkt*)(packet);
  bool ret = false;

  this->temperature_ = parseTempInLe(&(pkt.header.temp)) * this->const_param_.TEMPERATURE_RES;

  double pkt_ts = 0;
  if (this->param_.use_lidar_clock)
  {
    pkt_ts = parseTimeUTCWithUs(&pkt.header.timestamp) * 1e-6;
  }
  else
  {
    uint64_t ts = getTimeHost();

    // roll back to first block to approach lidar ts as near as possible.
    pkt_ts = ts * 1e-6 - this->getPacketDuration();

    if (this->write_pkt_ts_)
    {
      createTimeUTCWithUs (ts, (RSTimestampUTC*)&pkt.header.timestamp);
    }
  }

  T_BlockDiff diff(pkt, this->const_param_.BLOCKS_PER_PKT, this->mech_const_param_.BLOCK_DURATION);

  double block_ts = pkt_ts;
  for (uint16_t blk = 0; blk < this->const_param_.BLOCKS_PER_PKT; blk++)
  {
    const RSHELIOSMsopBlock& block = pkt.blocks[blk];

    if (memcmp(this->const_param_.BLOCK_ID, block.id, 2) != 0)
    {
      this->excb_(Error(ERRCODE_WRONGPKTHEADER));
      break;
    }

    int32_t block_az = ntohs(block.azimuth);
    block_ts += diff.ts(blk);
    int32_t block_azi_diff = diff.azimuth(blk);

    if (this->split_strategy_->newBlock(block_az))
    {
      this->cb_split_frame_(this->height_, this->prev_point_ts_);
      ret = true;
    }

    for (uint16_t chan = 0; chan < this->const_param_.CHANNELS_PER_BLOCK; chan++)
    {
      const RSChannel& channel = block.channels[chan]; 

      double chan_ts = block_ts + this->mech_const_param_.CHAN_TSS[chan];
      int32_t angle_horiz = block_az + 
        (int32_t)((float)block_azi_diff * this->mech_const_param_.CHAN_AZIS[chan]);

      int32_t angle_vert = this->chan_angles_.vertAdjust(chan);
      int32_t angle_horiz_final = this->chan_angles_.horizAdjust(chan, angle_horiz);
      float distance = ntohs(channel.distance) * this->const_param_.DISTANCE_RES;

      if (this->distance_section_.in(distance) && this->scan_section_.in(angle_horiz_final))
      {
        RSPoint point;
        point.x =  distance * COS(angle_vert) * COS(angle_horiz_final) + this->mech_const_param_.RX * COS(angle_horiz);
        point.y = -distance * COS(angle_vert) * SIN(angle_horiz_final) - this->mech_const_param_.RX * SIN(angle_horiz);
        point.z =  distance * SIN(angle_vert) + this->mech_const_param_.RZ;
        point.intensity = channel.intensity;
        point.timestamp = chan_ts;
        point.ring = this->chan_angles_.toUserChan(chan);

        this->cb_new_point_(point);
      }
      else if (!this->param_.dense_points)
      {
        RSPoint point;
        point.x = NAN;
        point.y = NAN;
        point.z = NAN;
        point.intensity = 0;
        point.timestamp = chan_ts;
        point.ring = this->chan_angles_.toUserChan(chan);

        this->cb_new_point_(point);
      }

      this->prev_point_ts_ = chan_ts;
    }
  }

  this->prev_pkt_ts_ = pkt_ts;
  return ret;
}

}  // namespace lidar
}  // namespace robosense
