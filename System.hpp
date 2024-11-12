#ifndef SYSTEM_HPP
#define SYSTEM_HPP

#include "Event.hpp"
#include "Bus.hpp"
#include<bits/stdc++.h>

using namespace std;

enum TrafficLight { RED, YELLOW, GREEN };

struct eventCmp {
    bool operator()(Event* a, Event* b) { return a->getTime() > b->getTime(); }
};

struct Light {
    int id;
    int location;
    TrafficLight state = GREEN;
    int cycleTime;
    int red;
    int green;
    int yellow;
};

struct Stop {
    int id;
    int location;
    int pax;
};

class System {
    public:
        System();
        void init(); //also setup event
        void simulation(); //also trigger event
        void performance();

        void arriveAtStop(Event* e);
        void deptFromStop(Event* e);
        void arriveAtLight(Event* e);
        void deptFromLight(Event* e);
        void paxArriveAtStop(Event* e);

    private:
        vector<Bus*> fleet;
        priority_queue<Event*, vector<Event*>, eventCmp> eventList;
        vector<variant<Stop*, Light*>> route;
        map<int, function<void(Event*)>> eventSet;
        int headway;
        
};

#endif