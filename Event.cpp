#include "Event.hpp"

Event::Event(int time, int busID, int eventType, int oneOfID, bool direction)
    : time(time), busID(busID), eventType(eventType), direction(direction) {
        /* tmp assumption */
        lightID = oneOfID;
}

int Event::getEventType() { return eventType; }

int Event::getTime() { return time; }

int Event::getBusID() { return busID; }
