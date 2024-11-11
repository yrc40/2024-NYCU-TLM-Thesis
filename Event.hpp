#ifndef EVENT_HPP
#define EVENT_HPP

class Event {
    public:
        Event(int time, int busID, int eventType, int oneOfID, bool direction);
        int getEventType();
        int getTime();
        int getBusID();

    private:
        int time;
        int busID;
        int eventType;
        int stopID;
        int lightID;
        bool direction;

};

#endif