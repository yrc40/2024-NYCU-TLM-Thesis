#ifndef BUS_HPP
#define BUS_HPP

#include "Event.hpp"

using namespace std;

class Bus {
    public:
        Bus(int id, int headway);
        int getId();
        float getVol();
        int getPax();
        int getLoaction();
        int getDwell();
        int getLastGo();
        const int getCapacity();
        const int getHeadway();

        void setVol(float v);
        void setPax(int p);
        void setLocation(int l);
        void setDwell(int d);
        void setLastGo(int t);

    private:
        int id;
        const int capacity = 60;
        float vol = 0;
        int pax = 0;
        int location = 0;
        int dwell = 0;
        int headway;
        int lastGo = 0;
};

#endif