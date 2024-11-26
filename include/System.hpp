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
    bool direction;
    string stopName;
    int mileage;
    int pax;
    string note;
    int lastArrive;
};

struct eventCmp {
    bool operator()(Event* a, Event* b) { return a->getTime() > b->getTime(); }
};

struct mileageCmp {
    bool operator()(const variant<Stop*, Light*>& a, const variant<Stop*, Light*>& b) const {
        return visit([](const auto* obj1, const auto* obj2) {
            return obj1->mileage < obj2->mileage; 
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

        /*Func*/
        void incrHeadwayDev(int dev);
        optional<Stop*> getNextStop(int stopID);

        /*getter*/
        const int getTmax();

    private:
        /*Paramemter*/
        string routeName;
        int Tmax = 3 * 60;
        float Vavg = 40 / 3.6;
        float Vlimit = 50 / 3.6;
        float Vlow = 15 / 3.6;

        /*Variable*/
        int headwayDev = 0;

        /*Data Structure*/
        vector<Bus*> fleet;
        priority_queue<Event*, vector<Event*>, eventCmp> eventList;
        set<variant<Stop*, Light*>, mileageCmp> route;
        map<int, function<void(Event*)>> eventSet;

        /*Function*/
        optional<Stop*> findNextStop(int stopID);
        optional<variant<Stop*, Light*>> findNext(variant<Stop*, Light*> target);
        
};

#endif