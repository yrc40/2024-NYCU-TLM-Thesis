#ifndef PLAN_HPP
#define PLAN_HPP

#include<bits/stdc++.h>
using namespace std;

class Plan {
    public:
        Plan();
        void setPhase(string config);
        int calculateSignal(int time);
        int timeRemain(int index, int target);
        
    private:
        vector<int> time;
        vector<int> cycle;
        vector<int> offset;
        vector<vector<pair<int, int>>> phase;
        void processSegment(string seg);
        int time2Seconds(const string& timeStr);

};

#endif