#include "Event.hpp"

Event::Event(int time, int busID, int eventType, int oneOfID, bool direction)
    : time(time), busID(busID), eventType(eventType), direction(direction) {
    if(eventType == 1 || eventType == 2) { // 若事件種類編號為 1 (公車到站) 或 2 (公車離站)
        stopID = oneOfID; // 記錄站點編號
    } else {
        lightID = oneOfID; // 記錄號誌編號
    }
}

const int Event::getEventType() { return eventType; }

const int Event::getTime() { return time; }

const int Event::getBusID() { return busID; }

const int Event::getStopID() { return stopID; }

const int Event::getLightID() { return lightID; }

const bool Event::getDirection() { return direction; }