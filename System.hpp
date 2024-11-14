#ifndef SYSTEM_HPP
#define SYSTEM_HPP

#include "Event.hpp"
#include "Bus.hpp"
#include<bits/stdc++.h>

using namespace std;

enum TrafficLight { RED, YELLOW, GREEN };

struct Light {
    int id;
    int mileage;
    TrafficLight state = GREEN;
    int cycleTime;
    int red;
    int green;
    int yellow;
};

struct Stop {
    int id;
    string stopName;
    int mileage;
    int pax;
    string note;
};

struct eventCmp {
    bool operator()(Event* a, Event* b) { return a->getTime() > b->getTime(); }
};

struct mileageCmp {
    bool operator()(const variant<Stop*, Light*>& a, const variant<Stop*, Light*>& b) const {
        return visit([](const auto* obj1, const auto* obj2) {
            return obj1->mileage > obj2->mileage; 
        }, a, b);
    }
};

class System {
    public:
        System();
        System(string routeName);
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
        priority_queue<variant<Stop*, Light*>, vector<variant<Stop*, Light*>>, mileageCmp> route; //decltype
        map<int, function<void(Event*)>> eventSet;
        int headway;
        string routeName;
        
};

#endif