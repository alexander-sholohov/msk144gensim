//
// Author: Alexander Sholohov <ra9yer@yahoo.com>
//
// License: MIT
//

#include <string.h>

#include <unistd.h>
#include <getopt.h>


#include <string>
#include <vector>
#include <iostream>
#include <thread>
#include <sstream>
#include <cmath>

#include <chrono>
#include <thread>
#include <random>

#include "msg_item.h"

constexpr int NumMessagesMax = 3;

struct Context
{
    float center_freq = 1500.0f;
    int signal_level = 100;
    int noise_level = 5;
    int on_frames = 10;
    int off_frames = 20;
    bool use_throttle = false;
    int sample_rate = 12000;
    int num_messages = 1;
    bool numbered_messages = false;
    MsgItem messages[NumMessagesMax];
};


class SimpleRNG
{
public:
    SimpleRNG(): 
        _dev(),
        _rng(_dev()),
        _dist(-999, 999)
    {
    }

    float get_random() 
    {
        int v = _dist(_rng);
        return v / 1000.0f;
    }

private:
    std::random_device _dev;
    std::mt19937 _rng;
    std::uniform_int_distribution<std::mt19937::result_type> _dist;

};


//------------------------------------------------------------
static bool str2bool(const char* str)
{
    if (strcmp(str, "true") == 0)
        return true;

    if (strcmp(str, "1") == 0)
        return true;

    return false;
}




class NumberedMessage
{
public:
    void makeNextMessage()
    {
        char buf[80];
        sprintf(buf, "M_%05d", counter);
        msg_item.initFromMessage(buf);

        counter++;
        if(counter > 99999)
        {
            counter = 0;
        }
    }

    MsgItem& msgItem()
    {
        return msg_item;
    }

private:

    int counter = 0;
    MsgItem msg_item;
};

static void out_wav_16bit(const Context& ctx)
{
    // gen wav

    const int nsps = 6;
    const float baud = 2000.0f;
    const float sps = baud * nsps; // 12000.0f;
    const float two_pi = 8 * std::atan(1.0f);
    const float dphi0 = two_pi * (ctx.center_freq - 0.25f * baud) / sps;
    const float dphi1 = two_pi * (ctx.center_freq + 0.25f * baud) / sps;
    float phi = 0.0f;
    const int64_t frame_length_in_milliseconds = 144LL * 1000 / static_cast<int64_t>(baud);

    SimpleRNG rng;
    NumberedMessage numbered_msg;

    while(true)
    {
        for(int msg_idx=0; msg_idx<ctx.num_messages; msg_idx++)
        {
            const int* tones;
            if(ctx.numbered_messages)
            {
                numbered_msg.makeNextMessage();
                tones = numbered_msg.msgItem().i4tone;
            }
            else
            {
                tones = ctx.messages[msg_idx].i4tone;
            }

            for(int i=0; i<ctx.on_frames; i++)
            {
                for(int j=0; j<144; j++)
                {
                    const int ch = tones[j];
                    if(ch!=0 && ch!=1)
                    {
                        throw std::runtime_error("wrong ch");
                    }

                    const float dphi = (ch == 0)? dphi0 : dphi1;

                    for(int k=0; k<nsps; k++)
                    {
                        float v_signal = std::cos(phi) * ctx.signal_level;
                        float v_noise = rng.get_random() * ctx.noise_level;
                        int v = static_cast<int>(v_signal + v_noise);
                        char low = v & 0xff;
                        char high = v >> 8;
                        std::cout << low << high;

                        phi += dphi;
                        if(phi > two_pi)
                        {
                            phi -= two_pi;
                        }
                    }
                }

                if(ctx.use_throttle)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(frame_length_in_milliseconds));
                }
            }
        }

        // send silence
        for(int i=0; i<ctx.off_frames; i++)
        {
            for(int j=0; j<144; j++)
            {
                for(int k=0; k<nsps; k++)
                {
                    float v_signal = 0.0f;
                    float v_noise = rng.get_random() * ctx.noise_level;
                    int v = static_cast<int>(v_signal + v_noise);

                    char low = v & 0xff;
                    char high = v >> 8;
                    std::cout << low << high;
                }
            }

            if(ctx.use_throttle)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(frame_length_in_milliseconds));
            }
            
        }
    }

}


static void out_iq_8bit(Context& ctx)
{
    const int symbols_per_second = 2000;
    const int sdr_sample_rate = ctx.sample_rate; // was 2048000
    const int samples_per_symbol = sdr_sample_rate / symbols_per_second;
    const int pp_len = samples_per_symbol * 2;

    const int64_t frame_length_in_milliseconds = 144LL * 1000 / static_cast<int64_t>(symbols_per_second);

    // pp_len = 2048
    // 144 iq len = 294912

    SimpleRNG rng;
    NumberedMessage numbered_msg;

    if (pp_len < 12 || pp_len % 2 != 0)
    {
        std::ostringstream info;
        info << "pp_len must be more than 12 and must be even." << std::endl
            << "sdr_sample_rate = " << sdr_sample_rate << std::endl
            << "pp_len = " << pp_len << std::endl
            << "pp_len * 144 = " << pp_len * 144  << std::endl;

        throw std::runtime_error(info.str());
    }


    for(int idx=0; idx<ctx.num_messages; idx++)
    {
        ctx.messages[idx].calculateIQ(pp_len);
    }

    constexpr int N72 = 144 / 2;

    while(true)
    {
        for(int msg_idx=0; msg_idx<ctx.num_messages; msg_idx++)
        {
            float* i_res;
            float* q_res;

            if(ctx.numbered_messages)
            {
                numbered_msg.makeNextMessage();
                numbered_msg.msgItem().calculateIQ(pp_len);

                i_res = &numbered_msg.msgItem().i_res[0];
                q_res = &numbered_msg.msgItem().q_res[0];
            }
            else
            {
                i_res = &ctx.messages[msg_idx].i_res[0];
                q_res = &ctx.messages[msg_idx].q_res[0];
            }

            for(int i=0; i<ctx.on_frames; i++)
            {
                for (int i = 0; i < N72 * pp_len; i++)
                {
                    float i_noise = rng.get_random() * ctx.noise_level;
                    float q_noise = rng.get_random() * ctx.noise_level;

                    float i_signal = i_res[i] * ctx.signal_level;
                    float q_signal = q_res[i] * ctx.signal_level;

                    char i_ch = static_cast<char>(i_noise + i_signal);
                    char q_ch = static_cast<char>(q_noise + q_signal);

                    std::cout << i_ch << q_ch;
                }

                if(ctx.use_throttle)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(frame_length_in_milliseconds));
                }
            }
        }

        // send silence
        for(int i=0; i<ctx.off_frames; i++)
        {
            for (int i = 0; i < N72 * pp_len; i++)
            {
                float i_noise = rng.get_random() * ctx.noise_level;
                float q_noise = rng.get_random() * ctx.noise_level;

                char i_ch = static_cast<char>(i_noise);
                char q_ch = static_cast<char>(q_noise);

                std::cout << i_ch << q_ch;
            }

            if(ctx.use_throttle)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(frame_length_in_milliseconds));
            }
        }

    }


}



//-------------------------------------------------------------------
static void usage(void)
{
    std::ostringstream buf;
    buf << "\nmsk144gensim - msk144 generator. Produces infinite stdout audio stream - 16 bits signed, 12000 samples per second, mono.\n\n";
    buf << "Usage:\t[--message= Message to send ]\n";
    buf << "\t[--message2= 2nd message to send]\n";
    buf << "\t[--message3= 3rd message to send]\n";
    buf << "\t[--center-freq= Center frequency (default: 1500)]\n";
    buf << "\t[--signal-level= max signal level (default: 100)]\n";
    buf << "\t[--noise-level= max signal level (default: 5)]\n";
    buf << "\t[--on-frames= ON frmaes (default: 10)]\n";
    buf << "\t[--off-frames= ON frames (default: 20)]\n";
    buf << "\t[--mode= 1-wav output; 2-IQ output (default: 1)]\n";
    buf << "\t[--sample-rate= sample rate for output stream (default: 12000)]\n";
    buf << "\t[--use-throttle= emulate real time output using sleep (default: false)]\n";
    buf << "\t[--show-only Print only text and bits to be send. (default: false)]\n";
    buf << "\t[--help this text]\n";

    std::cout << buf.str() << std::endl;

    exit(1);
}


//------------------------------------------------------------
int main(int argc, char** argv)
{

    static struct option long_options[] = {
            {"help", no_argument, 0, 0}, // 0
            {"message", required_argument, 0, 0}, // 1
            {"center-freq", required_argument, 0, 0}, // 2
            {"signal-level", required_argument, 0, 0}, // 3
            {"on-frames", required_argument, 0, 0}, // 4
            {"off-frames", required_argument, 0, 0}, // 5
            {"show-only", no_argument, 0, 0}, // 6
            {"mode", required_argument, 0, 0}, // 7
            {"use-throttle", required_argument, 0, 0}, // 8
            {"sample-rate", required_argument, 0, 0}, // 9
            {"noise-level", required_argument, 0, 0}, // 10
            {"num-messages", required_argument, 0, 0}, // 11
            {"message2", required_argument, 0, 0}, // 12
            {"message3", required_argument, 0, 0}, // 13
            {"numbered-messages", required_argument, 0, 0}, // 14
            {0, 0, 0, 0}
    };

    Context ctx;
    bool show_only = false;
    int mode = 1; 
    strcpy(ctx.messages[0].buf, "HELLO");
    strcpy(ctx.messages[1].buf, "HELLO2");
    strcpy(ctx.messages[2].buf, "HELLO3");


    while (true)
    {
        int option_index = 0;

        int c = getopt_long(argc, argv, "", long_options, &option_index);

        if (c == -1)
            break;

        if (c == 0)
        {
            switch (option_index)
            {
            case 0:
                usage();
                break;
            case 1:
                strcpy(ctx.messages[0].buf, optarg);
                break;
            case 2:
                ctx.center_freq = atof(optarg);
                break;
            case 3:
                ctx.signal_level = atoi(optarg);
                break;
            case 4:
                ctx.on_frames = atoi(optarg);
                break;
            case 5:
                ctx.off_frames = atoi(optarg);
                break;
            case 6:
                show_only = true; //str2bool(optarg);
                break;
            case 7:
                mode = atoi(optarg);
                break;
            case 8:
                ctx.use_throttle = str2bool(optarg);
                break;
            case 9:
                ctx.sample_rate = atoi(optarg);
                break;
            case 10:
                ctx.noise_level = atoi(optarg);
                break;
            case 11:
                ctx.num_messages = atoi(optarg);
                if(ctx.num_messages > NumMessagesMax) { ctx.num_messages = NumMessagesMax; }
                if(ctx.num_messages < 0) { ctx.num_messages = 0; }
                break;
            case 12:
                strcpy(ctx.messages[1].buf, optarg);
                break;
            case 13:
                strcpy(ctx.messages[2].buf, optarg);
                break;
            case 14:
                ctx.numbered_messages = str2bool(optarg);
                break;

            default:
                usage();
            }
        }
    }

    for(int idx=0; idx<ctx.num_messages; idx++)
    {
        ctx.messages[idx].reinitForStoredMessage();
    }

    if(show_only)
    {
        for(int idx=0; idx<ctx.num_messages; idx++)
        {
            std::cout << "msg id = " << idx << std::endl;
            std::cout << "msg sent: '" << ctx.messages[idx].msgsent << "'" << std::endl;
            std::cout << "i4tone[40] = " << ctx.messages[idx].i4tone[40] << std::endl;
            std::cout << "i4tone: ";
            for(int i=0; i<144; i++)
            {
                std::cout << ctx.messages[idx].i4tone[i];
            }
            std::cout << std::endl;
            std::cout << "bitseq: ";
            for(int i=0; i<144; i++)
            {
                std::cout << ctx.messages[idx].bitseq[i];
            }

            std::cout << std::endl;
        }

        std::cout << "signal_level:" << ctx.signal_level << std::endl;
        std::cout << "noise_level:" << ctx.noise_level << std::endl;
        std::cout << "mode:" << mode << std::endl;
        std::cout << "use_throttle:" << ctx.use_throttle << std::endl;
        std::cout << "sample_rate:" << ctx.sample_rate << std::endl;
        return 0;
    }


    if(mode == 1)
    {
        out_wav_16bit(ctx);
    }
    else if(mode == 2)
    {
        out_iq_8bit(ctx);
    }
    else 
    {
        throw std::runtime_error("Wrong mode");
    }

    std::cout << "Done." << std::endl;

    return 0;
}
