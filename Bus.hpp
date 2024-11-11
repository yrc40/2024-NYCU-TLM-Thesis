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

        void setVol();
        void setPax();
        void setLocation();
        void setDwell();

    private:
        int id;
        const int capacity = 60;
        int vol;
        int pax;
        int location;
        int dwell;
};

#endif