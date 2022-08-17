#pragma once
#include <memory>
#include <vector>

enum class MediaStreamType
{
    VEDIO,
    AUDIO,
};

enum class CodecType
{
    RAWDATA,
    H264,
    AAC,
};

struct MediaStreamInfo
{
    uint32_t        stream_id;
    MediaStreamType media_type;
    CodecType       codec_type;
};


struct MediaStreamPacket
{
    uint32_t stream_id;
    uint8_t* data;
    uint32_t data_length;
    uint64_t pts;
    uint64_t dts;
    /*
    if(avc)
    {
        flags & 0x0001 ? key_frame : no
    }
    */
    uint32_t flags = 0;
};

// struct FlvHead
// {
//     uint8_t sig_f;
//     uint8_t sig_l;
//     uint8_t sig_v;
//     uint8_t version;
//     uint8_t flag;
//     uint32_t data_offset;
// };

// struct FlvTagCommonHead
// {
//     uint8_t flag;
//     uint32_t data_size;
//     uint32_t timestamp;
//     uint32_t id;
// };

// struct FlvTagAudioCommonHead
// {
//     uint8_t flag;
// };

// struct FlvTagAudioAacData
// {
//     uint8_t aac_data_type;
//     uint8_t* data; // type = 1 ? rawdata : AudioSpecificConfig
// };

// struct FlvTagVideoCommonHead
// {
//     uint8_t flag;
// };

// struct FlvTagVideoAvcData
// {
//     uint8_t type;
//     uint32_t cts;
//     uint8_t* data;
// };

class FormatDemuxFlv
{
public:
    FormatDemuxFlv();
    ~FormatDemuxFlv();
public:
    int OpenSource(char* p_url, MediaStreamInfo** p_streams, uint32_t* p_streams_num);
    int GetOnePacket(MediaStreamPacket** p_pkts);
private:
    int GetFlvData(char* p_url);
    int ParseFlvHead();
    int GetStreamsInfo();
    int ParseTagCommonHead(MediaStreamPacket* p_pkt, uint32_t* p_tag_remaining_length);
    int ParseAudioHeadAndData(MediaStreamPacket** p_pkts, uint32_t* p_tag_remaining_length);
    int ParsevideoHeadAndData(MediaStreamPacket** p_pkts, uint32_t* p_tag_remaining_length);
private:
    unsigned char*   p_flv_data_;
    unsigned char*   p_read_ptr_;
    uint64_t         flv_file_length_;
    bool             b_include_video_stream_;
    bool             b_include_audio_stream_;
    uint32_t         stream_id_video_;
    uint32_t         stream_id_audio_;
    MediaStreamInfo* p_streams_;
    uint32_t         streams_num_;

    uint8_t*              asc_;
    std::vector<uint8_t*> sps_vec_;
    std::vector<uint8_t*> pps_vec_;
};