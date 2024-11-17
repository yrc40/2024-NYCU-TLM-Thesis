#ifndef BUS_HPP
#define BUS_HPP

#include "Event.hpp"

using namespace std;

class Bus {
    public:
        Bus(int id);
        int getId();
        int getVol();
        int getPax();
        int getLoaction();
        int getDwell();
        const int getCapacity();

        void setVol(int v);
        void setPax(int p);
        void setLocation(int l);
        void setDwell(int d);

    private:
        int id;
        const int capacity = 60;
        int vol;
        int pax;
        int location;
        int dwell;
};

#endif