#ifndef EVENT_HPP
#define EVENT_HPP

class Event {
    public:
        Event(int time, int busID, int eventType, int oneOfID, bool direction);
        const int getEventType();
        const int getTime();
        const int getBusID();
        const int getStopID();
        const bool getDirection();

    private:
        int time;
        int busID;
        int eventType;
        int stopID;
        int lightID;
        bool direction;

};

#endif