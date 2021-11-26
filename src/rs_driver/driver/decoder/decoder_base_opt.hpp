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
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*********************************************************************************************************************/

#pragma once

#include <rs_driver/driver/decoder/set_member.hpp>
#include <rs_driver/driver/driver_param.h>
#include <rs_driver/utility/time.h>

#include <arpa/inet.h>
#include <fstream>
#include <cmath>
#include <algorithm>
#include <functional>
#include <chrono>

namespace robosense
{
namespace lidar
{

#pragma pack(push, 1)
typedef struct
{
  // identity
  uint64_t MSOP_ID;
  uint64_t DIFOP_ID;
  uint64_t BLOCK_ID;

  // duration
  uint32_t PKT_RATE; // to be deleted
  uint16_t BLOCKS_PER_PKT;
  uint16_t BLOCKS_PER_FRAME;
  uint16_t CHANNELS_PER_BLOCK;
  uint16_t LASER_NUM;

  // firing
  float DSR_TOFFSET; // to be deleted
  float FIRING_FREQUENCY; // to be deleted

  // distance
  float DIS_RESOLUTION;

  // lens center
  float RX;
  float RY;
  float RZ;
} LidarConstantParameter;

typedef struct
{
  uint8_t year;
  uint8_t month;
  uint8_t day;
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
  uint16_t ms;
  uint16_t us;
} RSTimestampYMD;

typedef struct 
{
    uint8_t sec[6];
    uint8_t ss[4];
} RSTimestampUTC2;

typedef struct
{
  uint8_t sec[6];
  uint32_t us;
} RSTimestampUTC;

typedef struct
{
  uint8_t sync_mode;
  uint8_t sync_sts;
  RSTimestampUTC timestamp;
} RSTimeInfo;

typedef struct
{
  uint8_t tt[2];
} RsTemp;

typedef struct
{
  uint8_t id[8];
//  uint64_t id;
  uint8_t reserved_1[12];
  RSTimestampYMD timestamp;
  uint8_t lidar_type;
  uint8_t reserved_2[7];
  RsTemp temp;
  //uint16_t temp_raw;
  uint8_t reserved_3[2];
} RSMsopHeader;

typedef struct
{
  uint8_t id[4];
//  uint32_t id;
  uint16_t protocol_version;
  uint8_t reserved_1;
  uint8_t wave_mode;
  RsTemp temp;
#if 0
  uint8_t temp_low;
  uint8_t temp_high;
#endif
  RSTimestampUTC timestamp;
  uint8_t reserved_2[10];
  uint8_t lidar_type;
  uint8_t reserved_3[49];
} RSMsopHeaderNew;

typedef struct
{
  uint8_t lidar_ip[4];
  uint8_t host_ip[4];
  uint8_t mac_addr[6];
  uint16_t local_port;
  uint16_t dest_port;
  uint16_t port3;
  uint16_t port4;
} RSEthNet;

typedef struct
{
  uint8_t lidar_ip[4];
  uint8_t dest_ip[4];
  uint8_t mac_addr[6];
  uint16_t msop_port;
  uint16_t reserve_1;
  uint16_t difop_port;
  uint16_t reserve_2;
} RSEthNetNew;

typedef struct
{
  uint16_t start_angle;
  uint16_t end_angle;
} RSFOV;

typedef struct
{
  uint8_t sign;
  uint16_t value;
} RSCalibrationAngle;

typedef struct
{
  uint16_t distance;
  uint8_t intensity;
} RSChannel;

typedef struct
{
  uint8_t top_ver[5];
  uint8_t bottom_ver[5];
} RSVersion;

typedef struct
{
  uint8_t top_firmware_ver[5];
  uint8_t bot_firmware_ver[5];
  uint8_t bot_soft_ver[5];
  uint8_t motor_firmware_ver[5];
  uint8_t hw_ver[3];
} RSVersionNew;

typedef struct
{
  uint8_t num[6];
} RSSn;

typedef struct
{
  uint8_t device_current[3];
  uint8_t main_current[3];
  uint16_t vol_12v;
  uint16_t vol_sim_1v8;
  uint16_t vol_dig_3v3;
  uint16_t vol_sim_3v3;
  uint16_t vol_dig_5v4;
  uint16_t vol_sim_5v;
  uint16_t vol_ejc_5v;
  uint16_t vol_recv_5v;
  uint16_t vol_apd;
} RSStatus;

typedef struct
{
  uint16_t device_current;
  uint16_t vol_fpga;
  uint16_t vol_12v;
  uint16_t vol_dig_5v4;
  uint16_t vol_sim_5v;
  uint16_t vol_apd;
  uint8_t reserved[12];
} RSStatusNew;

typedef struct
{
  uint8_t reserved_1[9];
  uint16_t checksum;
  uint16_t manc_err1;
  uint16_t manc_err2;
  uint8_t gps_status;
  uint16_t temperature1;
  uint16_t temperature2;
  uint16_t temperature3;
  uint16_t temperature4;
  uint16_t temperature5;
  uint8_t reserved_2[5];
  uint16_t cur_rpm;
  uint8_t reserved_3[7];
} RSDiagno;

typedef struct
{
  uint16_t bot_fpga_temperature;
  uint16_t recv_A_temperature;
  uint16_t recv_B_temperature;
  uint16_t main_fpga_temperature;
  uint16_t main_fpga_core_temperature;
  uint16_t real_rpm;
  uint8_t lane_up;
  uint16_t lane_up_cnt;
  uint16_t main_status;
  uint8_t gps_status;
  uint8_t reserved[22];
} RSDiagnoNew;

#pragma pack(pop)

inline uint64_t calcTimeUTCWithNs(const RSTimestampUTC2* tsUtc)
{
  // sec
  uint64_t sec = 0;
  for (int i = 0; i < 6; i++)
  {
    sec <<= 8;
    sec += tsUtc->sec[i];
  }

  uint64_t ns = 0;
  for (int i = 0; i < 4; i++)
  {
    ns <<= 8;
    ns += tsUtc->ss[i]; 
  }

  return (sec * 1000000 + ns/1000);
}

inline uint64_t calcTimeUTCWithUs(const RSTimestampUTC2* tsUtc)
{
  // sec
  uint64_t sec = 0;
  for (int i = 0; i < 6; i++)
  {
    sec <<= 8;
    sec += tsUtc->sec[i];
  }

  uint64_t us = 0;
  for (int i = 0; i < 4; i++)
  {
    us <<= 8;
    us += tsUtc->ss[i]; 
  }

  return (sec * 1000000 + us);
}

inline uint64_t calcTimeUTCWithMs(const RSTimestampUTC2* tsUtc)
{
  // sec
  uint64_t sec = 0;
  for (int i = 0; i < 6; i++)
  {
    sec <<= 8;
    sec += tsUtc->sec[i];
  }

  // ms
  uint64_t ms = tsUtc->ss[0]; 
  ms <<= 8;
  ms += tsUtc->ss[1];

  // us
  uint64_t us = tsUtc->ss[2]; 
  us <<= 8;
  us += tsUtc->ss[3];

  return (sec * 1000000 + ms * 1000 + us);
}

inline uint64_t calcTimeYMD(const RSTimestampYMD* tsYmd)
{
  std::tm stm;
  memset(&stm, 0, sizeof(stm));

  // since 2000 in robosense YMD, and since 1900 in struct tm
  stm.tm_year = tsYmd->year + 2000 - 1900; 
  // since 1 in robosense YMD, and since 0 in struct tm
  stm.tm_mon = tsYmd->month - 1; 
  // since 1 in both robosense YMD and struct tm
  stm.tm_mday = tsYmd->day; 
  stm.tm_hour = tsYmd->hour;
  stm.tm_min = tsYmd->minute;
  stm.tm_sec = tsYmd->second;
  time_t sec = std::mktime(&stm);

  uint64_t ms = ntohs(tsYmd->ms);
  uint64_t us = ntohs(tsYmd->us);

#if 0
  std::cout << "tm_year:" << stm.tm_year 
    << ", tm_mon:" << stm.tm_mon 
    << ", ms:" << ms 
    << ", us:" << us 
    << std::endl;
#endif

  return (sec * 1000000 + ms * 1000 + us);
}

inline uint64_t calcTimeHost(void)
{
  std::chrono::system_clock::time_point t = std::chrono::system_clock::now();
  std::chrono::system_clock::duration t_s = t.time_since_epoch();

  std::chrono::duration<uint64_t, std::ratio<1l, 1000000l>> t_us = 
  std::chrono::duration_cast<std::chrono::duration<uint64_t, std::ratio<1l, 1000000l>>>(t_s);
  return t_us.count();
}

#define RS_TEMP_RESOLUTION 0.0625f

inline int16_t calcTemp(const RsTemp* tmp)
{
  // | lsb | padding | neg | msb |
  // |  5  |    3    |  1  |  7  | (in bits)
  uint8_t lsb = tmp->tt[0] >> 3;
  uint8_t msb = tmp->tt[1] & 0x7F;
  uint8_t neg = tmp->tt[1] & 0x80;

  int16_t t = ((uint16_t)msb << 5) + lsb;
  if (neg) t = -t;

  return t;
}

class ScanBlock
{
public:
  ScanBlock(uint16_t start, uint16_t end)
  {
    start_ = start % 36000;
    end_ = (end <= 36000) ? end : (end % 36000);
    cross_zero_ = (start_ > end_);
  }

  bool in(uint16_t angle)
  {
    if (cross_zero_)
      return (angle >= start_) || (angle < end_);
    else
      return (angle >= start_) && (angle < end_);
  }

  uint16_t start()
  {
    return start_;
  }

  uint16_t end()
  {
    return end_;
  }

private:
  uint16_t start_;
  uint16_t end_;
  bool cross_zero_;
};

class DistanceBlock
{
public:
  DistanceBlock (float min, float max, float usr_min, float usr_max)
    : min_((usr_min > min) ? usr_min : min), max_((usr_max < max) ? usr_max : max)
  {
  }

  bool in(float distance)
  {
    return ((min_ <= distance) && (distance <= max_));
  }

  float min()
  {
    return min_;
  }

  float max()
  {
    return max_;
  }

private:

  float min_;
  float max_;
};


class ChanAngles
{
public:
  
  int loadFromFile(const std::string& angle_path)
  {
    int ret = loadFromFile (angle_path, vert_angles_, horiz_angles_);
    if (ret < 0)
      return ret;

    genUserChan(vert_angles_, user_chans_);

    return 0;
  }

  int loadFromDifop(const RSCalibrationAngle vert_angle_arr[], 
      const RSCalibrationAngle horiz_angle_arr[], size_t size)
  {
    int ret = 
      loadFromDifop (vert_angle_arr, horiz_angle_arr, size, vert_angles_, horiz_angles_);
    if (ret < 0)
      return ret;

    genUserChan(vert_angles_, user_chans_);
    return 0;
  }

  size_t chanSize()
  {
    return vert_angles_.size();
  }

  uint16_t toUserChan(uint16_t chan)
  {
    return user_chans_[chan];
  }

  int32_t horizAdjust(uint16_t chan, int32_t horiz)
  {
    return (horiz + horiz_angles_[chan]);
  }

  int32_t vertAdjust(uint16_t chan)
  {
    return vert_angles_[chan];
  }

  void narrow ()
  {
  }

  static
  void genUserChan(const std::vector<int32_t>& vert_angles, std::vector<uint16_t>& user_chans)
  {
    user_chans.resize(vert_angles.size());

    for (size_t i = 0; i < vert_angles.size(); i++)
    {
      int32_t angle = vert_angles[i];
      uint16_t chan = 0;

      for (size_t j = 0; j < vert_angles.size(); j++)
      {
        if (vert_angles[j] < angle)
        {
          chan++;
        }
      }

      user_chans[i] = chan;
    }
  }

  static int loadFromFile(const std::string& angle_path, 
      std::vector<int32_t>& vert_angles, std::vector<int32_t>& horiz_angles)
  {
    vert_angles.clear();
    horiz_angles.clear();

    std::ifstream fd(angle_path.c_str(), std::ios::in);
    if (!fd.is_open())
    {
      std::cout << "fail to open angle file:" << angle_path << std::endl;
      return -1;
    }

    std::string line;
    while (std::getline(fd, line))
    {
      size_t pos_comma = 0;
      float vert = std::stof(line, &pos_comma);
      float horiz = std::stof(line.substr(pos_comma+1));

      vert_angles.emplace_back(static_cast<int32_t>(vert * 100));
      horiz_angles.emplace_back(static_cast<int32_t>(horiz * 100));
    }

    fd.close();
    return 0;
  }

  static int loadFromDifop(const RSCalibrationAngle* vert_angle_arr, 
      const RSCalibrationAngle* horiz_angle_arr, size_t size, 
      std::vector<int32_t>& vert_angles, std::vector<int32_t>& horiz_angles)
  {
    vert_angles.clear();
    horiz_angles.clear();

    for (size_t i = 0; i < size; i++)
    {
      const RSCalibrationAngle& vert = vert_angle_arr[i];
      const RSCalibrationAngle& horiz = horiz_angle_arr[i];
      int32_t v;

      if (vert.sign == 0xFF)
        break;

      v = ntohs(vert.value);
      if (vert.sign != 0) v = -v;
      vert_angles.emplace_back(v);

      v = ntohs(horiz.value);
      if (horiz.sign != 0) v = -v;
      horiz_angles.emplace_back(v);

#if 0
      if (type == LidarType::RS32)
      {
        this->vert_angle_list_[i] *= 0.1f;
        this->hori_angle_list_[i] *= 0.1f;
      }
#endif
    }

    return ((vert_angles.size() > 0) ? 0 : -1);
  }

private:
  std::vector<int32_t> vert_angles_;
  std::vector<int32_t> horiz_angles_;
  std::vector<uint16_t> user_chans_;
};

class Trigon
{
public:

  Trigon()
  {
    static bool init_ = false;

    if (!init_)
    {
      init_ = true;

      uint16_t RS_ONE_ROUND = 36000;
      float RS_ANGLE_RESOLUTION = 0.01; 

      for (size_t i = 0; i < RS_ONE_ROUND; i++)
      {
        double rads = static_cast<double>(i) * RS_ANGLE_RESOLUTION;
        rads = rads * M_PI / 180;

        sins_.emplace_back((float)std::sin(rads));
        coss_.emplace_back((float)std::cos(rads));
      }
    }
  }

  void print()
  {
      uint16_t RS_ONE_ROUND = 36000;
      for (size_t i = 0; i < RS_ONE_ROUND; i++)
      {
        std::cout << i << "\t" << sins_[i] << "\t" << coss_[i] << std::endl;
      }
  }

  float cos(uint16_t angle)
  {
    return coss_[angle];
  }

  float sin(uint16_t angle)
  {
    return sins_[angle];
  }

private:
  std::vector<float> sins_;
  std::vector<float> coss_;
};

}  // namespace lidar
}  // namespace robosense