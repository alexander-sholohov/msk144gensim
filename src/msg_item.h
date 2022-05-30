//
// Author: Alexander Sholokhov <ra9yer@yahoo.com>
//
// License: MIT
//

#pragma once

#include <vector>

struct MsgItem
{
    char buf[80] = {"HELLO"};
    int i4tone[144];
    int bitseq[144];
    char msgsent[200];
    std::vector<float> q_res;
    std::vector<float> i_res;

    void initFromMessage(const char* msg);
    void reinitForStoredMessage();

    void calculateIQ(const int pp_len);
};
