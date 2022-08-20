#include "format_demux_flv.h"
#include <fstream>
#include <stdio.h>
#include <cstring>

FormatDemuxFlv::FormatDemuxFlv()
{
    p_flv_data_      = nullptr;
    p_read_ptr_      = nullptr;
    p_streams_       = nullptr;
    asc_             = nullptr;
    sps_             = nullptr;
    pps_             = nullptr;
    streams_num_     = 0;
    stream_id_video_ = 0;
    stream_id_audio_ = 0;
    flv_file_length_ = 0;
    sps_len_         = 0;
    pps_len_         = 0;
    b_include_video_stream_ = false;
    b_include_audio_stream_ = false;
}

FormatDemuxFlv::~FormatDemuxFlv()
{
    if(p_flv_data_)
    {
        delete p_flv_data_;
        p_flv_data_ = nullptr;
        p_read_ptr_ = nullptr;
    }
    if(p_streams_)
    {
        delete[] p_streams_;
        p_streams_ = nullptr;
    }
    if(asc_)
    {
        delete asc_;
        asc_ = nullptr;
    }

    if(sps_)
    {
        delete sps_;
        sps_ = nullptr;
    }

    if(pps_)
    {
        delete pps_;
        pps_ = nullptr;
    }
}

int FormatDemuxFlv::GetFlvData(char* p_url)
{
    std::ifstream flv_file(p_url, std::ios::in | std::ios::binary);
    if(!flv_file.is_open())
    {
        printf("File url: %s, open failed!\n", p_url);
        return -1;
    }

    auto file_pos_begin = flv_file.tellg();
    flv_file.seekg(0, std::ios::end);
    auto file_pos_end = flv_file.tellg();
    flv_file.seekg(0, std::ios::beg);

    flv_file_length_ = file_pos_end - file_pos_begin;

    if(p_flv_data_)
    {
        delete p_flv_data_;
        p_flv_data_ = nullptr;
        p_read_ptr_ = nullptr;
    }
    p_flv_data_ = new unsigned char[flv_file_length_];

    if(flv_file.read((char*)p_flv_data_, flv_file_length_) < 0)
    {
        printf("File url: %s, read failed!\n", p_url);
        return -1;
    }
    p_read_ptr_ = p_flv_data_;
    flv_file.close();
    return 0;
}

int FormatDemuxFlv::ParseFlvHead()
{
    if(p_read_ptr_[0] != 'F' || p_read_ptr_[1] != 'L' || p_read_ptr_[2] != 'V')
    {
        printf("This file is not in flv format!\n");
        return -1;
    }
    p_read_ptr_ += 3;

    if(*p_read_ptr_ != 0x01)
    {
        printf("Only version 1 flv files are supported\n");
        return -1;
    }
    p_read_ptr_++;

    bool b_have_video = false;
    bool b_have_audio = false;
    bool b_creat_video_stream = false;
    bool b_creat_audio_stream = false;

    streams_num_ = 0;
    if((*p_read_ptr_) & 0x01)
    {
        b_have_video = true;
        (streams_num_)++;
    }
    if((*p_read_ptr_) & 0x04)
    {
        b_have_audio = true;
        (streams_num_)++;
    }
    p_read_ptr_++;

    if(streams_num_ == 0)
    {
        printf("flv has neither video nor audio streams\n");
        return -1;
    }

    if(p_streams_)
    {
        delete[] p_streams_;
        p_streams_ = nullptr;
    }

    p_streams_ = new MediaStreamInfo[streams_num_];
    for(int i = 0; i < streams_num_; i++)
    {
        if(b_have_video && (!b_creat_video_stream))
        {
            p_streams_[i].stream_id = i;
            p_streams_[i].media_type = MediaStreamType::VEDIO;
            stream_id_video_ = i;
            b_creat_video_stream = true;
            continue;
        }   

        if(b_have_audio && (!b_creat_audio_stream))
        {
            p_streams_[i].stream_id = i;
            p_streams_[i].media_type = MediaStreamType::AUDIO;
            stream_id_audio_ = i;
            b_creat_audio_stream = true;
            continue;
        }
    }

    b_include_video_stream_ = b_have_video;
    b_include_audio_stream_ = b_have_audio;

    if(p_read_ptr_[3] != 0x09)
    {
        printf("The length of the flv head is not 9\n");
        return -1;
    }
    p_read_ptr_ += 4;
    return 0;
}

int FormatDemuxFlv::GetStreamsInfo()
{
    unsigned char* p_parse_ptr = p_read_ptr_;
    bool b_get_video_stream_info = false;
    bool b_get_audio_stream_info = false;

    while(!(\
    (!b_include_video_stream_ || b_get_video_stream_info) \
    && (!b_include_audio_stream_ || b_get_audio_stream_info)))
    {
        p_parse_ptr += 4;

        uint32_t tag_length = 0;
        uint8_t* p_tag_length = (uint8_t*)&tag_length;
        for(int i = 0; i < 3; i++)
        {
            p_tag_length[i] = p_parse_ptr[3 - i];
        }

        if((*p_parse_ptr) == 0x12)
        {
            p_parse_ptr += (tag_length + 11);
            continue;
        }
        else if((*p_parse_ptr) == 0x08)
        {
            p_parse_ptr += 11;
            if((*p_parse_ptr & 0xF0) == 0xa0)
            {
                for(int i = 0; i < streams_num_; i++)
                {
                    if(p_streams_[i].media_type == MediaStreamType::AUDIO)
                    {
                        p_streams_[i].codec_type = CodecType::AAC;
                        break;
                    }
                }
            }
            else
            {
                printf("This audio encoding format is not supported");
                return -1;
            }
            p_parse_ptr += tag_length;
            b_get_audio_stream_info = true;
            continue;
        }
        else if((*p_parse_ptr) == 0x09)
        {
            p_parse_ptr += 11;
            if((*p_parse_ptr & 0x0F) == 0x07)
            {
                for(int i = 0; i < streams_num_; i++)
                {
                    if(p_streams_[i].media_type == MediaStreamType::VEDIO)
                    {
                        p_streams_[i].codec_type = CodecType::H264;
                        break;
                    }
                }
            }
            else
            {
                printf("This video encoding format is not supported");
                return -1;
            }
            p_parse_ptr += tag_length;
            b_get_video_stream_info = true;
            continue;
        }
    }
    return 0;
}

int FormatDemuxFlv::OpenSource(char* p_url, MediaStreamInfo** p_streams, uint32_t* p_streams_num)
{
    if(p_streams_num == nullptr)
    {
        return -1;
    }
    if(p_streams == nullptr)
    {
        return -1;
    }

    if(GetFlvData(p_url) < 0)
    {
        printf("Error getting flv data!\n");
        return -1;
    }

    if(ParseFlvHead() < 0)
    {
        printf("Error parsing flv head!\n");
        return -1;
    }

   if(GetStreamsInfo() < 0)
    {
        printf("Error getting stream info!\n");
        return -1;
    }

    *p_streams = p_streams_;
    *p_streams_num = streams_num_;

    return 0;
}

int FormatDemuxFlv::ParseTagCommonHead(MediaStreamPacket* p_pkt, uint32_t* p_tag_remaining_length)
{
    bool b_find_video_tag = false;
    bool b_find_audio_tag = false;
    uint32_t tag_length = 0;
    uint32_t tag_timestamp = 0;
    while((!b_find_video_tag) && (!b_find_audio_tag))
    {
        p_read_ptr_ += 4;

        uint8_t* p_tag_length = (uint8_t*)&tag_length;
        for(int i = 0; i < 3; i++)
        {
            p_tag_length[i] = p_read_ptr_[3 - i];
        }

        if(*p_read_ptr_ == 0x08)
        {
            b_find_audio_tag = true;
            p_pkt->stream_id = stream_id_audio_;
            break;
        }
        else if(*p_read_ptr_ == 0x09)
        {
            b_find_video_tag = true;
            p_pkt->stream_id = stream_id_video_;
            break;
        }
        p_read_ptr_ += (11 + tag_length);
    }

    uint8_t* p_tag_length = (uint8_t*)&tag_timestamp;
    if((p_read_ptr_[4] & p_read_ptr_[5] & p_read_ptr_[6]) == 0xff)
    {
        p_tag_length[3] = p_read_ptr_[7];
    }
    p_tag_length[0] = p_read_ptr_[6];
    p_tag_length[1] = p_read_ptr_[5];
    p_tag_length[2] = p_read_ptr_[4];

    p_pkt->pts = tag_timestamp;
    p_pkt->dts = tag_timestamp;
    p_read_ptr_ += 11;
    *p_tag_remaining_length = tag_length;
    return 0;
}

int FormatDemuxFlv::ParseAudioHeadAndData(MediaStreamPacket** p_pkts, uint32_t* p_tag_remaining_length)
{
    (*p_tag_remaining_length)--;
    p_read_ptr_++;

    if(*p_read_ptr_ == 0)
    {
        (*p_tag_remaining_length)--;
        p_read_ptr_++;
        if(asc_)
        {
            delete asc_;
            asc_ = nullptr;
        }
        asc_ = new uint8_t[*p_tag_remaining_length];
        memcpy(asc_, p_read_ptr_, *p_tag_remaining_length);
        delete *p_pkts;
        *p_pkts = nullptr;
        
    }
    else if(*p_read_ptr_ == 1)
    {
        (*p_tag_remaining_length)--;
        p_read_ptr_++;
        (*p_pkts)->data = new uint8_t[*p_tag_remaining_length + 7];
        (*p_pkts)->data_length = *p_tag_remaining_length + 7;
        uint8_t adts_head[7];

        adts_head[0] = 0xFF;
        adts_head[1] = 0xF0;
        adts_head[1] |= 0x01;  //protection_absent Indicates whether error_check() data is present or not.
        adts_head[2] = ((((asc_[0] & 0xF8) >> 3) - 1 ) & 0x03) << 6;
        adts_head[2] |= (((asc_[0] & 0x07) << 1) | ( (asc_[1] & 0x80) >> 7)) << 2;
        adts_head[2] |= (asc_[1] & 0x38) >> 5;
        adts_head[3] |= (asc_[1] & 0x38) << 3;
        adts_head[3] |= (static_cast<uint8_t>((*p_tag_remaining_length) >> 8) & 0x1F) >> 3;
        adts_head[4] = ((static_cast<uint8_t>((*p_tag_remaining_length) >> 8) & 0x1F) << 6) | (static_cast<uint8_t>((*p_tag_remaining_length) & 0xFF) >> 3);
        adts_head[5] = (static_cast<uint8_t>((*p_tag_remaining_length) & 0xFF) << 5);
        adts_head[6] = 0xFC;
        memcpy((*p_pkts)->data, adts_head, 7);
        memcpy((*p_pkts)->data + 7, p_read_ptr_, *p_tag_remaining_length);
    }
    return 0;
}

int FormatDemuxFlv::ParsevideoHeadAndData(MediaStreamPacket** p_pkts, uint32_t* p_tag_remaining_length)
{
    bool b_key_frame = false;
    if(((*p_read_ptr_) & 0xf0) == 0x10)
    {
        b_key_frame = true;
    }

    (*p_tag_remaining_length)--;
    p_read_ptr_++;

    if(*p_read_ptr_ == 0)
    {
        p_read_ptr_ += 4;
        (*p_tag_remaining_length) -= 4;

        uint8_t spsnum = p_read_ptr_[5] & 0x1F;
        std::size_t offset = 6;
        for(uint8_t i = 0; i < spsnum; i++)
        {
            uint16_t spssize = ((uint16_t)(p_read_ptr_[offset]) << 8) | (uint16_t)p_read_ptr_[offset + 1];
            if(i == spsnum - 1)
            {
                uint8_t* sps_mem = new uint8_t[spssize];
                sps_len_ = spssize;
                memcpy(sps_mem, p_read_ptr_ + offset + 2, spssize);
                sps_ = sps_mem;
            }
            offset += (2 + spssize);
        }
        uint8_t ppsnum = p_read_ptr_[offset++];
        for(uint8_t i = 0; i < ppsnum; i++)
        {
            uint16_t ppssize = ((uint16_t)(p_read_ptr_[offset]) << 8) | (uint16_t)p_read_ptr_[offset + 1];
            if(i == ppsnum - 1)
            {
                uint8_t* pps_mem = new uint8_t[ppssize];
                pps_len_ = ppssize;
                memcpy(pps_mem, p_read_ptr_ + offset + 2, ppssize);
                pps_ = pps_mem;
            }
            offset += (2 + ppssize);
        }
        delete *p_pkts;
        *p_pkts = nullptr;
    }
    else if(*p_read_ptr_ == 1)
    {
        uint32_t cts = 0;
        uint8_t* p_cts = (uint8_t*)&cts;
        for(int i = 0; i < 3; i++)
        {
            p_cts[i] = p_read_ptr_[3 - i];
        }
        p_read_ptr_ += 4;
        (*p_tag_remaining_length) -= 4;

        (*p_pkts)->data_length = 0;
        uint8_t start_code[4] = {0x00,0x00,0x00,0x01};
        std::map<uint32_t, uint8_t*> mem_map;
        while(*p_tag_remaining_length > 0)
        {
            uint8_t* nalu_mem = nullptr;
            uint32_t nalu_mem_size = 0;
            uint32_t naluSize = 0;
            naluSize |= static_cast<uint32_t>(p_read_ptr_[0]) << 24;
            naluSize |= static_cast<uint32_t>(p_read_ptr_[1]) << 16;
            naluSize |= static_cast<uint32_t>(p_read_ptr_[2]) << 8;
            naluSize |= static_cast<uint32_t>(p_read_ptr_[3]);
            p_read_ptr_ += 4;
            (*p_tag_remaining_length) -= 4;
            uint32_t offset_pkt = 0;
            if((p_read_ptr_[0] & 0x1f) == 0x05)
            {
                nalu_mem_size += naluSize + 4 * 3 + sps_len_ + pps_len_;
                nalu_mem       = new uint8_t[nalu_mem_size];

                memcpy(nalu_mem + offset_pkt, start_code, 4);
                offset_pkt += 4;
                memcpy(nalu_mem + offset_pkt, sps_, sps_len_);
                offset_pkt += sps_len_;

                memcpy(nalu_mem + offset_pkt, start_code, 4);
                offset_pkt += 4;
                memcpy(nalu_mem + offset_pkt, pps_, pps_len_);
                offset_pkt += pps_len_;

                memcpy(nalu_mem + offset_pkt, start_code, 4);
                offset_pkt += 4;
                memcpy(nalu_mem + offset_pkt, p_read_ptr_, naluSize);
            }
            else// if((p_read_ptr_[0] & 0x1f) == 0x01)
            {
                nalu_mem_size += naluSize + 4;
                nalu_mem       = new uint8_t[nalu_mem_size];
                memcpy(nalu_mem + offset_pkt, start_code, 4);
                offset_pkt += 4;
                memcpy(nalu_mem + offset_pkt, p_read_ptr_, naluSize);
            }
            p_read_ptr_ += naluSize;
            (*p_tag_remaining_length) -= naluSize;
            if(nalu_mem)
            {
                mem_map.emplace(std::make_pair(nalu_mem_size, nalu_mem));
            }
        }
        if(mem_map.size() > 0)
        {
            for(auto itr = mem_map.begin(); itr != mem_map.end(); itr++)
            {
                (*p_pkts)->data_length += itr->first;
            }
            (*p_pkts)->data = new uint8_t[(*p_pkts)->data_length];
            uint32_t offset_allmem = 0;
            for(auto itr = mem_map.begin(); itr != mem_map.end(); itr++)
            {
                memcpy((*p_pkts)->data + offset_allmem, itr->second, itr->first);
                offset_allmem += itr->first;
            }
        }
        else
        {
            printf("Video tag has no nalu!\n");
        }

        for(auto itr = mem_map.begin(); itr != mem_map.end();)
        {
            delete itr->second;
            itr = mem_map.erase(itr);
        }
    }
    return 0;
}

int FormatDemuxFlv::GetOnePacket(MediaStreamPacket** p_pkts)
{
    if(p_pkts == nullptr)
    {
        return -1;
    }

    bool b_get_pkt = false;
    MediaStreamPacket* p_pkt = new MediaStreamPacket();
    while (!b_get_pkt)
    {
        if((p_read_ptr_ - p_flv_data_) + 4 >= flv_file_length_)
        {
            return 1;
        }
        uint32_t tag_remaining_length = 0;
        ParseTagCommonHead(p_pkt, &tag_remaining_length);

        if(p_pkt->stream_id == stream_id_audio_)
        {
            ParseAudioHeadAndData(&p_pkt, &tag_remaining_length);
        }
        else if(p_pkt->stream_id == stream_id_video_)
        {
            ParsevideoHeadAndData(&p_pkt, &tag_remaining_length);
        }

        p_read_ptr_ += tag_remaining_length;
        if(p_pkt)
        {
            b_get_pkt = true;
        }
        else
        {
            p_pkt = new MediaStreamPacket();
        }
    }
    
    *p_pkts = p_pkt;

    return 0;
}