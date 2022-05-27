#include "msg_item.h"

#if __GNUC__ > 7
#include <cstddef>
  typedef size_t fortran_charlen_t;
#else
  typedef int fortran_charlen_t;
#endif

#include <string.h>

#include <unistd.h>
#include <getopt.h>

#include <cmath>
#include <stdexcept>


extern "C" {
  // --- Fortran routines ---

    void genmsk_128_90_(char msg0[], 
                        int* ichk, 
                        char msgsent[],
                        int* i4tone,
                        int* itype,
                        fortran_charlen_t, fortran_charlen_t);

}


static void tone2bitseq(const int* tone, int* outbitsec)
{
    int tmp[144];

    // flip polarity
    for(int i=0; i<144; i++)
    {
        tmp[i] = (tone[i] == 0)? 1: 0;
    }

    for(int i=0; i<144; i+=2)
    {
        tmp[i+0] = (tmp[i+0] == 0)? -1: 1;
        tmp[i+1] = (tmp[i+1] == 0)? +1: -1;
    }

    int tmp2[144];

    tmp2[0] = -1; // because of sync 01110010
    for(int i=1; i<144; i++)
    {
        tmp2[i] = tmp[i-1] * tmp2[i-1];
    }

    for(int i=0; i<144; i++)
    {
        outbitsec[i] = (tmp2[i] == -1)? 0 : 1;
    }

}


void MsgItem::initFromMessage(const char* msg)
{
    strcpy(buf, msg);
    reinitForStoredMessage();
}

void MsgItem::reinitForStoredMessage()
{
    int ichk = 0;
    int itype = 1;
    genmsk_128_90_(buf, &ichk, msgsent, i4tone, &itype, 37, 37);

    // check for some reason
    for(int j=0; j<144; j++)
    {
        const int ch = i4tone[j];
        if(ch!=0 && ch!=1)
        {
            throw std::runtime_error("wrong ch");
        }
    }

    tone2bitseq(i4tone, bitseq);
}


void MsgItem::calculateIQ(const int pp_len)
{
    std::vector<float> pp(pp_len);
    const float pi = 4 * std::atan(1.0f);
    for (int i = 0; i < pp_len; i++)
    {
        pp[i] = std::sin(pi * i / pp_len);
    }

    constexpr int N72 = 144 / 2;

    std::vector<float> tones(144);

    q_res.resize(72 * pp_len);
    i_res.resize(72 * pp_len);

    for(int i=0; i<144; i++)
    {
        tones[i] = (bitseq[i] == 0)? -1.0f : 1.0f;
    }

    // Q (half first)
    for (int j = 0; j < pp_len / 2; j++)
    {
        q_res[j] = tones[0] * pp[pp_len / 2 + j];
    }

    // Q
    for (int i = 1; i < N72 - 1; i++)
    {
        int base =  pp_len * i - pp_len / 2;
        for (int j = 0; j < pp_len; j++)
        {
            q_res[base + j] = tones[2 * i] * pp[j];
        }
    }

    // Q (half last)
    {
        int base = pp_len * N72 - pp_len / 2;
        for (int j = 0; j < pp_len / 2; j++)
        {
            q_res[base + j] = tones[0] * pp[j];
        }
    }

    // I
    for (int i = 0; i < N72; i++)
    {
        int base = pp_len * i;
        for (int j = 0; j < pp_len; j++)
        {
            i_res[base + j] = tones[2 * i + 1] * pp[j];
        }
    }
}
