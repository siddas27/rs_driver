/******************************************************************************
 * Copyright 2020 RoboSense All rights reserved.
 * Suteng Innovation Technology Co., Ltd. www.robosense.ai

 * This software is provided to you directly by RoboSense and might
 * only be used to access RoboSense LiDAR. Any compilation,
 * modification, exploration, reproduction and redistribution are
 * restricted without RoboSense's prior consent.

 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL ROBOSENSE BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *****************************************************************************/
#pragma once
#define RS16_PCAP_SLEEP_DURATION 1150
#define RS32_PCAP_SLEEP_DURATION 500
#define RSBP_PCAP_SLEEP_DURATION 500
#define RS128_PCAP_SLEEP_DURATION 95

#include "common/common_header.h"
#include "driver/decoder/decoder_base.hpp"
#include "msg/lidar_packet_msg.h"

using boost::asio::deadline_timer;
using boost::asio::ip::udp;
namespace robosense
{
  namespace lidar
  {
    const int RSLIDAR_PKT_LEN = 1248;

    enum InputState
    {
      INPUT_OK = 0,
      INPUT_TIMEOUT = 1,
      INPUT_ERROR = 2,
      INPUT_DIFOP = 4,
      INPUT_MSOP = 8,
      INPUT_EXIT = 16
    };

    class Input
    {
    public:
      Input(const LiDAR_TYPE &_lidar_type, const RSInput_Param &_input_param,
            const std::function<void(const Error &)> _excb) : lidar_type_(_lidar_type),
                                                              input_param_(_input_param),
                                                              excb_(_excb)
      {
        if (input_param_.read_pcap)
        {
          char errbuf[PCAP_ERRBUF_SIZE];
          if ((pcap_ = pcap_open_offline(input_param_.pcap_file_dir.c_str(), errbuf)) == NULL)
          {
            excb_(Error(ErrCode_PcapWrongDirectory));
            exit(-1);
          }
          else
          {
            std::stringstream msop_filter;
            std::stringstream difop_filter;
            msop_filter << "src host " << input_param_.device_ip << " && ";
            difop_filter << "src host " << input_param_.device_ip << " && ";
            msop_filter << "udp dst port " << input_param_.msop_port;
            pcap_compile(pcap_, &this->pcap_msop_filter_, msop_filter.str().c_str(), 1, 0xFFFFFFFF);
            difop_filter << "udp dst port " << input_param_.difop_port;
            pcap_compile(pcap_, &this->pcap_difop_filter_, difop_filter.str().c_str(), 1, 0xFFFFFFFF);
          }
        }
        else
        {
          setSocket("msop", input_param_.msop_port);
          setSocket("difop", input_param_.difop_port);
        }
      }
      ~Input()
      {
        if (!input_param_.read_pcap)
        {
          msop_thread_.start.store(false);
          difop_thread_.start.store(false);
          msop_thread_.m_thread->detach();
          difop_thread_.m_thread->detach();
        }
        else
        {
          pcap_thread_.start.store(false);
          pcap_thread_.m_thread->detach();
          this->input_param_.pcap_file_dir.clear();
          pcap_close(this->pcap_);
        }
      }
      inline void regRecvMsopCallback(const std::function<void(const LidarPacketMsg &)> callBack)
      {
        msop_cb_.push_back(callBack);
      }
      inline void regRecvDifopCallback(const std::function<void(const LidarPacketMsg &)> callBack)
      {
        difop_cb_.push_back(callBack);
      }
      inline void start()
      {
        if (!input_param_.read_pcap)
        {
          msop_thread_.start.store(true);
          difop_thread_.start.store(true);
          msop_thread_.m_thread.reset(new std::thread([this]() { getMsopPacket(); }));
          difop_thread_.m_thread.reset(new std::thread([this]() { getDifopPacket(); }));
        }
        else
        {
          pcap_thread_.start.store(true);
          pcap_thread_.m_thread.reset(new std::thread([this]() { getPcapPacket(); }));
        }
      }

    private:
      inline void checkDifopDeadline()
      {
        if (difop_deadline_->expires_at() <= deadline_timer::traits_type::now())
        {
          difop_sock_ptr_->cancel();
          difop_deadline_->expires_at(boost::posix_time::pos_infin);
        }
        difop_deadline_->async_wait(boost::bind(&Input::checkDifopDeadline, this));
      }

      inline void checkMsopDeadline()
      {
        if (msop_deadline_->expires_at() <= deadline_timer::traits_type::now())
        {
          msop_sock_ptr_->cancel();
          msop_deadline_->expires_at(boost::posix_time::pos_infin);
        }
        msop_deadline_->async_wait(boost::bind(&Input::checkMsopDeadline, this));
      }
      static void handleReceive(
          const boost::system::error_code &ec, std::size_t length,
          boost::system::error_code *out_ec, std::size_t *out_length)
      {
        *out_ec = ec;
        *out_length = length;
      }
      inline void setSocket(const std::string &pkt_type, const uint16_t &port)
      {
        if (pkt_type == "msop")
        {
          try
          {
            msop_sock_ptr_.reset(new udp::socket(msop_io_service_, udp::endpoint(udp::v4(), port)));
            msop_deadline_.reset(new deadline_timer(msop_io_service_));
          }
          catch (...)
          {
            excb_(ErrCode_MsopPortBuzy);
            exit(-1);
          }
          msop_deadline_->expires_at(boost::posix_time::pos_infin);
          checkMsopDeadline();
        }
        else if (pkt_type == "difop")
        {
          try
          {
            difop_sock_ptr_.reset(new udp::socket(difop_io_service_, udp::endpoint(udp::v4(), port)));
            difop_deadline_.reset(new deadline_timer(difop_io_service_));
          }
          catch (...)
          {
            excb_(ErrCode_DifopPortBuzy);
            exit(-1);
          }
          difop_deadline_->expires_at(boost::posix_time::pos_infin);
          checkDifopDeadline();
        }
      }
      inline void getPcapPacket()
      {
        while (pcap_thread_.start.load())
        {
          int ret;
          struct pcap_pkthdr *header;
          const u_char *pkt_data;
          switch (lidar_type_)
          {
          case LiDAR_TYPE::RS16:
            usleep(RS16_PCAP_SLEEP_DURATION);
            break;
          case LiDAR_TYPE::RS32:
            usleep(RS32_PCAP_SLEEP_DURATION);
            break;
          case LiDAR_TYPE::RSBP:
            usleep(RSBP_PCAP_SLEEP_DURATION);
            break;
          case LiDAR_TYPE::RS128:
            usleep(RS128_PCAP_SLEEP_DURATION);
            break;
          default:
            break;
          }
          if ((ret = pcap_next_ex(pcap_, &header, &pkt_data)) >= 0)
          {
            if (!input_param_.device_ip.empty() && (0 != pcap_offline_filter(&pcap_msop_filter_, header, pkt_data)))
            {
              LidarPacketMsg msg;
              memcpy(msg.packet.data(), pkt_data + 42, RSLIDAR_PKT_LEN);
              for (auto &iter : msop_cb_)
              {
                iter(msg);
              }
            }
            else if (!input_param_.device_ip.empty() && (0 != pcap_offline_filter(&pcap_difop_filter_, header, pkt_data)))
            {
              LidarPacketMsg msg;
              memcpy(msg.packet.data(), pkt_data + 42, RSLIDAR_PKT_LEN);
              for (auto &iter : difop_cb_)
              {
                iter(msg);
              }
            }
            else
            {
              continue;
            }
          }
          else
          {
            sleep(1);
            excb_(ErrCode_PcapFinished);
            if (input_param_.pcap_repeat)
            {
              excb_(ErrCode_PcapRepeat);
              char errbuf[PCAP_ERRBUF_SIZE];
              pcap_ = pcap_open_offline(input_param_.pcap_file_dir.c_str(), errbuf);
            }
            else
            {
              excb_(ErrCode_PcapExit);
              break;
            }
          }
        }
      }
      void getMsopPacket()
      {
        while (msop_thread_.start.load())
        {
          char *precv_buffer = (char *)malloc(RSLIDAR_PKT_LEN);
          msop_deadline_->expires_from_now(boost::posix_time::seconds(1));
          boost::system::error_code ec = boost::asio::error::would_block;
          std::size_t ret = 0;

          msop_sock_ptr_->async_receive(boost::asio::buffer(precv_buffer, RSLIDAR_PKT_LEN),
                                        boost::bind(&Input::handleReceive, _1, _2, &ec, &ret));
          do
          {
            msop_io_service_.run_one();
          } while (ec == boost::asio::error::would_block);
          if (ec)
          {
            free(precv_buffer);
            excb_(ErrCode_MsopPktTimeout);
            continue;
          }
          if (ret < RSLIDAR_PKT_LEN)
          {
            free(precv_buffer);
            excb_(ErrCode_MsopPktIncomplete);
            continue;
          }
          LidarPacketMsg msg;
          memcpy(msg.packet.data(), precv_buffer, RSLIDAR_PKT_LEN);
          for (auto &iter : msop_cb_)
          {
            iter(msg);
          }
          free(precv_buffer);
        }
      }
      void getDifopPacket()
      {
        while (difop_thread_.start.load())
        {
          char *precv_buffer = (char *)malloc(RSLIDAR_PKT_LEN);

          difop_deadline_->expires_from_now(boost::posix_time::seconds(2));
          boost::system::error_code ec = boost::asio::error::would_block;
          std::size_t ret = 0;

          difop_sock_ptr_->async_receive(boost::asio::buffer(precv_buffer, RSLIDAR_PKT_LEN),
                                         boost::bind(&Input::handleReceive, _1, _2, &ec, &ret));
          do
          {
            difop_io_service_.run_one();
          } while (ec == boost::asio::error::would_block);
          if (ec)
          {
            free(precv_buffer);
            excb_(ErrCode_DifopPktTimeout);
            continue;
          }
          if (ret < RSLIDAR_PKT_LEN)
          {
            free(precv_buffer);
            excb_(ErrCode_DifopPktIncomplete);
            continue;
          }
          LidarPacketMsg msg;
          memcpy(msg.packet.data(), precv_buffer, RSLIDAR_PKT_LEN);
          for (auto &iter : difop_cb_)
          {
            iter(msg);
          }
          free(precv_buffer);
        }
      }

    private:
      RSInput_Param input_param_;
      LiDAR_TYPE lidar_type_;
      std::function<void(const Error &)> excb_;
      /* pcap file parse */
      pcap_t *pcap_;
      bpf_program pcap_msop_filter_;
      bpf_program pcap_difop_filter_;
      /* live socket */
      std::unique_ptr<udp::socket> msop_sock_ptr_;
      std::unique_ptr<udp::socket> difop_sock_ptr_;
      std::unique_ptr<deadline_timer> msop_deadline_;
      std::unique_ptr<deadline_timer> difop_deadline_;
      Thread msop_thread_;
      Thread difop_thread_;
      Thread pcap_thread_;
      boost::asio::io_service msop_io_service_;
      boost::asio::io_service difop_io_service_;
      std::vector<std::function<void(const LidarPacketMsg &)>> difop_cb_;
      std::vector<std::function<void(const LidarPacketMsg &)>> msop_cb_;
    };
  } // namespace lidar
} // namespace robosense
