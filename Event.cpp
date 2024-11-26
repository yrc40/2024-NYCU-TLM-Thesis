#include "Event.hpp"

Event::Event(int time, int busID, int eventType, int oneOfID, bool direction)
    : time(time), busID(busID), eventType(eventType), direction(direction) {
        /* tmp assumption */
        stopID = oneOfID;
}

const int Event::getEventType() { return eventType; }

const int Event::getTime() { return time; }

const int Event::getBusID() { return busID; }

const int Event::getStopID() { return stopID; }

const bool Event::getDirection() { return direction; }