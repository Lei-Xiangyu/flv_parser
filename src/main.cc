#include "format_demux_flv.h"
#include <fstream>
#include "unistd.h"

int main_fun(int argc, char* argv[])
{
    std::ofstream file_handle_v;
    std::ofstream file_handle_a;

    file_handle_v.open(argv[2], std::ios::trunc | std::ios::binary);
    file_handle_a.open(argv[3], std::ios::trunc | std::ios::binary);

    FormatDemuxFlv s_flv_demuxer;
    MediaStreamInfo* p_stream = nullptr;
    uint32_t num_streams = 0;
    s_flv_demuxer.OpenSource(argv[1], &p_stream, &num_streams);

    MediaStreamPacket* p_pkt = nullptr;
    int ret_getpkt = 0;
    while((ret_getpkt = s_flv_demuxer.GetOnePacket(&p_pkt)) != 1)
    {
        if(ret_getpkt < 0)
        {
            printf("Failed to get packet from flv file!\n");
            break;
        }

        MediaStreamType meida_type;

        for(int i = 0; i < num_streams; i++)
        {
            if(p_stream[i].stream_id == p_pkt->stream_id)
            {
                meida_type = p_stream[i].media_type;
            }
        }

        if(meida_type == MediaStreamType::VEDIO)
        {
            file_handle_v.write((const char*)p_pkt->data, p_pkt->data_length);
        }
        else if (meida_type == MediaStreamType::AUDIO)
        {
            file_handle_a.write((const char*)p_pkt->data, p_pkt->data_length);
        }
        
        delete p_pkt;
        p_pkt = nullptr;
    }
    if(p_pkt)
    { 
        delete p_pkt;
        p_pkt = nullptr;
    }

    file_handle_v.close();
    file_handle_a.close();

    return 0;
}

int main(int argc, char* argv[])
{
    for(int i = 0; i < argc; i++)
    {
        printf("argv %d: %s\n", i, argv[i]);
    }

    main_fun(argc, argv);
    
    return 0;
}