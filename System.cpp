#include "System.hpp"

System::System() : route(mileageCmp()) {}

System::System(string routeName) : routeName(routeName), route(mileageCmp()) {
    eventSet[1] = [this](Event* e) { this->arriveAtStop(e); };
    eventSet[2] = [this](Event* e) { this->deptFromStop(e); };
    eventSet[3] = [this](Event* e) { this->arriveAtLight(e); };
    eventSet[4] = [this](Event* e) { this->deptFromLight(e); };
}

const int System::getTmax() { return Tmax; }

optional<Stop*> System::getNextStop(int stopID) { return findNextStop(stopID); }

optional<Stop*> System::findNextStop(int stopID) {
    for (auto it = route.begin(); it != route.end(); ++it) {
        if (auto* stop = get_if<Stop*>(&*it)) {
            // cout << "Checking stop with ID: " << (*stop)->id << endl;
            if ((*stop)->id == stopID) {
                 cout << "Found stopID: " << stopID << ". Looking for the next stop." << endl;
                auto nextIt = next(it);
                while (nextIt != route.end()) {
                    if (auto* nextStop = get_if<Stop*>(&*nextIt)) {
                        // cout << "Found next stop with ID: " << (*nextStop)->id << endl;
                        return *nextStop;
                    }
                    ++nextIt;
                }
                cout << "No next Stop found after stopID: " << stopID << endl;
                return nullopt; // 已到達 `route` 的尾部
            }
        }
    }
    // cout << "Stop with ID " << stopID << " not found in route." << endl;
    return nullopt;
}

optional<variant<Stop*, Light*>> System::findNext(variant<Stop*, Light*> target) {
    auto it = route.find(target);
    it++;
    if (std::holds_alternative<Stop*>(*it)) {
        Stop* stop = std::get<Stop*>(*it);
        cout << "Find Next Stop ID: " << stop->id << endl;
    } else if (std::holds_alternative<Light*>(*it)) {
        Light* light = std::get<Light*>(*it);
        cout << "Find Next Light ID: " << light->id << endl;  
    }   
    return *it;

}

void System::incrHeadwayDev(int t) {
    headwayDev += t;
}

void System::init() {
    /*TODO: read parameter*/
    /*fleet、headway*/
    /*Stop, Light(assume 500/1)*/
    cout << "eneter init" << endl;
    ifstream file( "./data/" + routeName + ".csv");
    string line;

    getline(file, line);

    while (getline(file, line)) {
        stringstream ss(line);
        string field;
        string dummy;
        Stop* stop = new Stop;

        std::getline(ss, field, ',');
        stop->id = std::stoi(field);

        std::getline(ss, dummy, ',');
        std::getline(ss, field, ',');
        stop->direction = (field == "1");

        std::getline(ss, field, ',');
        stop->stopName = field;

        std::getline(ss, dummy, ',');
        std::getline(ss, dummy, ',');
        std::getline(ss, field, ',');
        stop->note = field;

        std::getline(ss, field, ',');
        stop->mileage = std::stoi(field);
        stop->pax = 0; 

        route.insert(stop);

        cout << stop->id << " " << stop->direction << " " << stop->stopName << " " << stop->mileage << "\n";
    }

    file.close(); 
    Light* light = new Light;

    // 初始化成員變數
    light->id = 1;
    light->mileage = 750;
    light->state = RED; // 修改預設狀態
    light->cycleTime = 60;
    light->offset = 10;

    light->plan.push_back({0, GREEN});
    light->plan.push_back({20, YELLOW});
    light->plan.push_back({30, RED});

    route.insert(light);
    for (const auto& element : route) {
        visit([](auto&& obj) {
            using T = decay_t<decltype(obj)>;
            if constexpr (is_same_v<T, Stop*>) {
                cout << "Stop ID: " << obj->id << endl;
            } else if constexpr (std::is_same_v<T, Light*>) {
                cout << "Light ID: " << obj->id << endl;
            }
        }, element);
    }

    Bus* newBus = new Bus(0, 5);
    fleet.push_back(newBus);

}

void System::arriveAtStop(Event* e) {
    cout << "Time: " << e->getTime() << "\n";
    cout << "New Event: Bus " << e->getBusID() << " arrive at stop " << e->getStopID() << "\n";

    /*assume drop rate is 0.5*/
    float dropRate = 0.5;
    float arriveRate = 0.5;

    int busID = e->getBusID();
    Bus* bus = nullptr;
    for (auto& b : fleet) {
        if (b->getId() == busID) {
            bus = b;
            break;
        }
    }
    cout << "dwell time = " << bus->getDwell() << "\n";
    
    if (!bus) {
        cout << "Error: Bus not found for ID " << busID << endl;
        return;
    }
    
    int stopID = e->getStopID(); 
    Stop* stop = nullptr;
    auto it = find_if(route.begin(), route.end(), [&](const variant<Stop*, Light*>& item) {
        if (auto* s = get_if<Stop*>(&item)) {
            if ((*s)->id == stopID) {
                stop = *s;
                return true;
            }
        }
        return false;
    });

    if (it != route.end()) {
        stop = get<Stop*>(*it);
    } else {
        cout << "Error: Stop not found for ID " << stopID << endl;
        return;
    }

    /*Update vol and location*/
    bus->setVol(0);
    bus->setLocation(stop->mileage);

    /*Deal with pax*/
    int paxRemain = static_cast<int>(bus->getPax() * dropRate);
    int availableCapacity = static_cast<int>(bus->getCapacity() - paxRemain);
    cout << "availableCapacity: " << availableCapacity << "\n";
    int demand = stop->pax + static_cast<int>((e->getTime() - stop->lastArrive) * arriveRate); 
    cout << "Demand: " << demand << "\n";
    int boardPax = (demand > availableCapacity) ? availableCapacity : demand;

    bus->setPax(paxRemain + boardPax);    
    stop->pax -= boardPax;
    cout << "Capacity: " << bus->getCapacity() << ", Current Passenger: " << bus->getPax() << ", Boarded Passenger: " << boardPax << endl;

    Bus* prevBus = nullptr; // Get distance from proceed bus
    for (auto& b : fleet) {
        if (b->getId() == busID - 1) {
            prevBus = b;
            break;
        }
    }
    if (prevBus) {
        this->incrHeadwayDev(pow(((e->getTime() - stop->lastArrive) - bus->getHeadway()) / bus->getHeadway(), 2)); //headway deviation
        cout << "Cumulative headway deviation" << headwayDev << "\n";
    }
    
    stop->lastArrive = e->getTime();

    /*New event*/
    auto itor = route.find(stop);
    if (itor != route.end() && itor == prev(route.end())) {
        cout << "Final stop in this route, terminate\n";
        return;   
    } else {
        cout << "Going to next element ...\n";
        Event* newEvent = new Event( //depart form stop
            e->getTime() + min(this->getTmax(), bus->getDwell()), 
            bus->getId(),
            2, 
            e->getStopID(), 
            e->getDirection()
        );
        eventList.push(newEvent); 
    }
    bus->setDwell(bus->getDwell() - min(this->getTmax(), bus->getDwell()));
    cout << "\n";
}

void System::deptFromStop(Event* e) {

    cout << "Time: " << e->getTime() << "\n";
    cout << "New Event: Bus " << e->getBusID() << " depart from stop " << e->getStopID() << "\n";

    int busID = e->getBusID();
    Bus* bus = nullptr;
    for (auto& b : fleet) {
        if (b->getId() == busID) {
            bus = b;
            break;
        }
    }
    
    if (!bus) {
        cout << "Error: Bus not found for ID " << busID << endl;
        return;
    }
    
    int stopID = e->getStopID(); 
    cout << "busID: " << e->getBusID() << endl;
    Stop* stop = nullptr;
    auto it = find_if(route.begin(), route.end(), [&](const variant<Stop*, Light*>& item) {
        if (auto* s = get_if<Stop*>(&item)) {
            if ((*s)->id == stopID) {
                stop = *s;
                return true;
            }
        }
        return false;
    });

    //find next stop
    auto nextStop = this->getNextStop(stopID);
    //cout << "1. Next stop is: " << nextStop.value()->stopName << "\n";

    /*Calculate scheme*/
    // 1. Calculate Tn,s
    
    float arriveRate = 0.5; //tmp assumption
    float dropRate = 0.5; //tmp assumption
    if (nextStop.has_value()) {
        cout << "Next stop is: " << nextStop.value()->stopName << "\n";
        int boardPax = min(nextStop.value()->pax + static_cast<int>(ceil(bus->getHeadway()*arriveRate)), static_cast<int>(bus->getCapacity() - bus->getPax() * dropRate)); //pax num
        int paxTime = static_cast<int>(boardPax * (bus->getPax() < 0.65 * bus->getCapacity() ? 2 : 2.7 * tan(bus->getPax() * (1 - dropRate) / bus->getCapacity())));
        int totaldwell = paxTime + bus->getDwell();
        cout << "total dwell time = " << totaldwell << "\n";
        
        Bus* prevBus = nullptr; // Get distance from proceed bus
        for (auto& b : fleet) {
            if (b->getId() == busID - 1) {
                prevBus = b;
                break;
            }
        }
    
        if (!prevBus) { //first car 
            cout << "The first bus should not follow other's volocity" << "\n";
            bus->setVol(Vavg);
            bus->setDwell(totaldwell);
            cout << "vol = " << bus->getVol() << " dwell time = " << bus->getDwell() << "\n";

        } else if (prevBus->getVol()) {
            float distance = prevBus->getLoaction() + prevBus->getVol() * (e->getTime() - prevBus->getLastStop()) - stop->mileage;
            float newVol = distance / (bus->getHeadway() + totaldwell);
            cout << "distance = " << distance << "new Vol = " << newVol << "\n";


            if(newVol < Vlow) {
                totaldwell += (distance / newVol) - (distance / Vavg);
                newVol = Vavg;
                bus->setVol(newVol);
                bus->setDwell(totaldwell);
            } else if (newVol > Vlimit) {
                prevBus->setDwell(prevBus->getDwell() + (distance / Vlimit) - (distance / newVol));
                newVol = Vlimit;
            }

            bus->setVol(newVol);
            bus->setDwell(totaldwell);

        } else {
            float distance = prevBus->getLoaction() - stop->mileage;
            float newVol = distance / (bus->getHeadway() + totaldwell);

            if(newVol < Vlow) {
                totaldwell += (distance / newVol) - (distance / Vavg);
                newVol = Vavg;
                bus->setVol(newVol);
                bus->setDwell(totaldwell);
            } else if (newVol > Vlimit) {
                prevBus->setDwell(prevBus->getDwell() + (distance / Vlimit) - (distance / newVol));
                newVol = Vlimit;
            }

            bus->setVol(newVol);
            bus->setDwell(totaldwell);
        }
    }

    /*Find next event*/
    auto nextElement = findNext(stop);
    if(nextElement.has_value()) {
        visit([&](auto* obj) {
            using T = decay_t<decltype(*obj)>;
            if constexpr (is_same_v<T, Stop>) {
                int dist =  obj->mileage - stop->mileage;
                int newTime = e->getTime() + dist / bus->getVol();
                Event* newEvent = new Event( //arrive at stop
                    newTime, 
                    bus->getId(),
                    1, 
                    obj->id, 
                    e->getDirection()
                );
                eventList.push(newEvent);

            } else if constexpr (is_same_v<T, Light>) {
                cout << "Next Light ID: " << obj->id << endl;
                int dist = obj->mileage - stop->mileage;
                int newTime = e->getTime() + dist / bus->getVol();
                Event* newEvent = new Event( //arrive at light
                    newTime, 
                    bus->getId(),
                    3, 
                    obj->id, 
                    e->getDirection()
                );
                eventList.push(newEvent);
            }
        }, nextElement.value());
    } else {
        cout << "Can't find next element or no next\n";
    }
    cout << "\n";
}

void System::arriveAtLight(Event* e) {
    /*Announcement*/
    cout << "Time: " << e->getTime() << "\n";
    cout << "Bus " << e->getBusID() << " arrive at light " << e->getLightID() << "\n";

    /*Find target*/
    int busID = e->getBusID();
    Bus* bus = nullptr;
    for (auto& b : fleet) {
        if (b->getId() == busID) {
            bus = b;
            break;
        }
    }
    
    if (!bus) {
        cout << "Error: Bus not found for ID " << busID << endl;
        return;
    }
    
    int lightID = e->getLightID();
    Light* light = nullptr;
    auto it = find_if(route.begin(), route.end(), [&](const variant<Stop*, Light*>& item) {
        if (auto* l = get_if<Light*>(&item)) {
                return true;
        }
        return false;
    });

    if (it != route.end()) {
        light = get<Light*>(*it);
    } else {
        cout << "Error: Stop not found for ID " << lightID << endl;
        return;
    }

    /*Update light status*/
    int remainder = (e->getTime() - light->offset) % light->cycleTime;
    int wait = 0;
    for (auto it = light->plan.begin(); it != light->plan.end() - 1; it++) {
        if (remainder >= it->first && remainder < (it + 1)->first) {
            light->state =  it->second;
            wait = (it + 1)->first - remainder;
        }
    }

    /*New Event*/
    if (light->state == GREEN) {
        cout << "Now is GREEN, just go through...\n";
        auto nextElement = findNext(light);
        if(nextElement.has_value()) {
            visit([&](auto* obj) {
                using T = decay_t<decltype(*obj)>;
                if constexpr (is_same_v<T, Stop>) {
                    int dist =  obj->mileage - light->mileage;
                    int newTime = e->getTime() + dist / bus->getVol();
                    Event* newEvent = new Event( //arrive at stop
                        newTime, 
                        bus->getId(),
                        1, 
                        obj->id, 
                        e->getDirection()
                    );
                    eventList.push(newEvent);

                } else if constexpr (is_same_v<T, Light>) {
                    cout << "Next Light ID: " << obj->id << endl;
                    int dist = obj->mileage - light->mileage;
                    int newTime = e->getTime() + dist / bus->getVol();
                    Event* newEvent = new Event( //arrive at light
                        newTime, 
                        bus->getId(),
                        3, 
                        obj->id, 
                        e->getDirection()
                    );
                    eventList.push(newEvent);
                }
            }, nextElement.value());
        } else {
            cout << "Can't find next element or no next\n";
        }
    } else {
        cout << "Now is RED, wait for " << wait << " second...\n";
        Event* newEvent = new Event( //dept form light
            e->getTime() + wait, 
            bus->getId(),
            4, 
            lightID, 
            e->getDirection()
        );
        eventList.push(newEvent);
    }
    
}

void System::deptFromLight(Event* e) {
    /*Announcement*/
    cout << "Time: " << e->getTime() << "\n";
    cout << "Bus " << e->getBusID() << " dept form light " << e->getLightID() << "\n";

    /*Find target*/
    int busID = e->getBusID();
    Bus* bus = nullptr;
    for (auto& b : fleet) {
        if (b->getId() == busID) {
            bus = b;
            break;
        }
    }
    
    if (!bus) {
        cout << "Error: Bus not found for ID " << busID << endl;
        return;
    }
    
    int lightID = e->getLightID();
    Light* light = nullptr;
    auto it = find_if(route.begin(), route.end(), [&](const variant<Stop*, Light*>& item) {
        if (auto* l = get_if<Light*>(&item)) {
                return true;
        }
        return false;
    });

    if (it != route.end()) {
        light = get<Light*>(*it);
    } else {
        cout << "Error: Stop not found for ID " << lightID << endl;
        return;
    }

    /*New event*/
    auto nextElement = findNext(light);
    if(nextElement.has_value()) {
        visit([&](auto* obj) {
            using T = decay_t<decltype(*obj)>;
            if constexpr (is_same_v<T, Stop>) {
                int dist =  obj->mileage - light->mileage;
                int newTime = e->getTime() + dist / bus->getVol();
                Event* newEvent = new Event( //arrive at stop
                    newTime, 
                    bus->getId(),
                    1, 
                    obj->id, 
                    e->getDirection()
                );
                eventList.push(newEvent);

            } else if constexpr (is_same_v<T, Light>) {
                cout << "Next Light ID: " << obj->id << endl;
                int dist = obj->mileage - light->mileage;
                int newTime = e->getTime() + dist / bus->getVol();
                Event* newEvent = new Event( //arrive at light
                    newTime, 
                    bus->getId(),
                    3, 
                    obj->id, 
                    e->getDirection()
                );
                eventList.push(newEvent);
            }
        }, nextElement.value());
    } else {
        cout << "Can't find next element or no next\n";
    }
}  


void System::simulation() {
    Event* newEvent = new Event( //arrive at light
        0, //time
        0, //bus
        2, //event
        2, //stopid
        0
    );
    eventList.push(newEvent);

    while(!eventList.empty()) {
        Event* currentEvent = eventList.top();
        cout << eventList.size() << "\n";
        int eventType = currentEvent->getEventType();
        auto it = eventSet.find(eventType);
        if (it != eventSet.end()) {
            it->second(currentEvent);
        } else {
            cout << "Unknown event type: " << eventType << endl;
        }
        eventList.pop();
        cout << eventList.size() << "\n";
    }


}
