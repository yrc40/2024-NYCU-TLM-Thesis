#include "System.hpp"

System::System() : route(mileageCmp()) {}

System::System(string routeName) : routeName(routeName), route(mileageCmp()) {
    eventSet[1] = [this](Event* e) { this->arriveAtStop(e); };
    eventSet[2] = [this](Event* e) { this->deptFromStop(e); };
    eventSet[3] = [this](Event* e) { this->arriveAtLight(e); };
    eventSet[4] = [this](Event* e) { this->deptFromLight(e); };
    eventSet[5] = [this](Event* e) { this->paxArriveAtStop(e); };
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
        cout << "!!!!!!!!!!Stop ID: " << stop->id << endl;
    } else if (std::holds_alternative<Light*>(*it)) {
        Light* light = std::get<Light*>(*it);
        cout << "Light ID: " << light->id << endl;  // 假設 Light 有 getId()   
    }   
    return *it;
    /*if (it == this->route.end()) { 
        cout << "2222222\n";
        return nullopt;
    }

    it++;  
    if (it == this->route.end()) { 
        cout << "2222222\n";
        return nullopt;
    }

    return *it;
    cout << static_cast<int>(*it);*/

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

    /*for (const auto& element : route) {
        visit([](auto&& obj) {
            using T = decay_t<decltype(obj)>;
            if constexpr (is_same_v<T, Stop*>) {
                cout << "Stop ID: " << obj->id << endl;
            } else if constexpr (std::is_same_v<T, Light*>) {
                cout << "Light ID: " << obj->id << endl;
            }
        }, element);
    }*/

    Bus* newBus = new Bus(0, 5);
    fleet.push_back(newBus);

}

void System::arriveAtStop(Event* e) {
    cout << "Time: " << e->getTime() << "\n";
    cout << "New Event: Bus " << e->getBusID() << " arrive at stop " << e->getStopID() << "\n";

    /*assume drop rate is 0.5*/
    float dropRate = 0.5;

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
    int availableCapacity = static_cast<int>(bus->getCapacity() - bus->getPax() * dropRate);  
    int demand = stop->pax; 
    int boardPax = (demand > availableCapacity) ? availableCapacity : demand;

    bus->setPax(availableCapacity + boardPax);    
    stop->pax -= boardPax;
    cout << "Capacity: " << bus->getCapacity() << ", Current Volume: " << bus->getVol() << ", Boarded Pax: " << boardPax << endl;

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
    } //000000000000000000000000
    
    stop->lastArrive = e->getTime();

    bus->setDwell(bus->getDwell() - min(this->getTmax(), bus->getDwell()));

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
    if (!nextElement.has_value()) cout << "00000000000\n";
    if(nextElement.has_value()) {
        visit([&](auto* obj) {
            using T = decay_t<decltype(*obj)>;
            if constexpr (is_same_v<T, Stop>) {
                cout << "Next Stop ID: " << obj->id << endl;
                int dist =  obj->mileage - stop->mileage;
                int newTime = e->getTime() + dist / bus->getVol();
                cout << "New Time:" << newTime << "\n";
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
                int dist = stop->mileage - obj->mileage;
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
    
}

void System::deptFromLight(Event* e) {
   
}

void System::paxArriveAtStop(Event* e) {
   
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
    /*Event* currentEvent = eventList.top();
    cout << "curr " << "busID:" << currentEvent->getBusID() << "\n"
        << "stopID " << currentEvent->getStopID() << "\n";

    int stopID = currentEvent->getStopID();
    Stop* stop = nullptr;
    auto it = find_if(route.begin(), route.end(), [&](const variant<Stop*, Light*>& item) {
    if (auto* s = get_if<Stop*>(&item)) {
        // 比較 stopID，找出對應的 Stop
        if ((*s)->id == stopID) {
            stop = *s;
            return true;
        }
    }
        return false;
    });

    cout << "fordebug: " << stop->id <<"Name " << stop->stopName << endl;
    auto nextstop = findNext(stop);
    if (nextstop.has_value()) {  // 確認 nextstop 是否有值
    // 判斷下一個停靠點是 Stop* 還是 Light*
    if (auto stopPtr = std::get_if<Stop*>(&(*nextstop))) {
        cout << "Next stop is a Stop at address: " << (*stopPtr)->id << endl;
    } else if (auto lightPtr = std::get_if<Light*>(&(*nextstop))) {
        cout << "Next stop is a Light at address: " << *lightPtr << endl;
    }
} else {
    cout << "No next stop found." << endl;
}*/

    while(!eventList.empty()) {
        Event* currentEvent = eventList.top();

        int eventType = currentEvent->getEventType();
        auto it = eventSet.find(eventType);
        if (it != eventSet.end()) {
            it->second(currentEvent);
        } else {
            cout << "Unknown event type: " << eventType << endl;
        }
        // cout << eventList.size() << '\n';
        eventList.pop();
        // cout << eventList.size() << '\n';
    }


}
