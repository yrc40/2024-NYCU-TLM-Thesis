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
            if ((*stop)->id == stopID) {
                auto nextIt = next(it);
                while (nextIt != route.end()) {
                    if (auto* nextStop = get_if<Stop*>(&*nextIt)) {
                        return *nextStop;
                    }
                    ++nextIt;
                }
            }
        }
    }
    return nullopt;
}

optional<variant<Stop*, Light*>> System::findNext(variant<Stop*, Light*> target) {
    auto it = this->route.find(target);
    if (it == this->route.end() || ++it == this->route.end()) {
        return nullopt; 
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

}

void System::arriveAtStop(Event* e) {

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
            return (*s)->id == stopID;
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

    cout << "Bus " << busID << " arrived at Stop " << stopID << endl;
    cout << "Capacity: " << bus->getCapacity() << ", Current Volume: " << bus->getVol() << ", Boarded Pax: " << boardPax << endl;

    /*New event*/
    Event* newEvent = new Event( //depart form stop
        e->getTime() + bus->getDwell(), 
        bus->getId(),
        2, 
        e->getStopID(), 
        e->getDirection()
    );

    this->incrHeadwayDev(e->getTime() - stop->lastArrive); //headway deviation
    stop->lastArrive = e->getTime();

    eventList.push(newEvent);
    eventList.pop();

    bus->setDwell(bus->getDwell() - min(this->getTmax(), bus->getDwell()));

}

void System::deptFromStop(Event* e) {

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
            return (*s)->id == stopID;
        }
        return false;
    });

    //find next stop
    auto nextStop = this->getNextStop(stopID);


    /*Calculate scheme*/
    // 1. Calculate Tn,s
    float arriveRate = 0.5; //tmp assumption
    float dropRate = 0.5; //tmp assumption
    if (nextStop.has_value()) {
        int boardPax = min(nextStop.value()->pax + static_cast<int>(ceil(bus->getHeadway()*arriveRate)), static_cast<int>(bus->getCapacity() - bus->getPax() * dropRate)); //pax num
        float paxTime = boardPax * (bus->getPax() < 0.65 * bus->getCapacity() ? 2 : 2.7 * tan(bus->getPax() * (1 - dropRate) / bus->getCapacity()));
        int totaldwell = paxTime + bus->getDwell();

        /*Get distance from proceed bus*/
        Bus* prevBus = nullptr;
        for (auto& b : fleet) {
            if (b->getId() == busID) {
                prevBus = b;
                break;
            }
        }
    
        if (!prevBus) { //first car 
            bus->setVol(Vavg);
            bus->setDwell(totaldwell);

        } else if (prevBus->getVol()) {
            int distance = prevBus->getLoaction() + prevBus->getVol() * (e->getTime() - prevBus->getLastStop()) - stop->mileage;
            int newVol = static_cast<int>(distance / (bus->getHeadway() + totaldwell));

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
            int distance = prevBus->getLoaction() - stop->mileage;
            int newVol = static_cast<int>(distance / (bus->getHeadway() + totaldwell));

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
            cout << "Next Stop ID: " << obj->id << endl;
            int dist = stop->mileage - obj->mileage;
            int newTime = e->getTime() + dist / bus->getVol();
            Event* newEvent = new Event( //arrive at stop
                newTime, 
                bus->getId(),
                1, 
                obj->id, 
                e->getDirection()
            );
            eventList.push(newEvent);
            eventList.pop();

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
            eventList.pop();
        }
    }, nextElement.value());
    }

}

void System::arriveAtLight(Event* e) {
    
}

void System::deptFromLight(Event* e) {
   
}

void System::paxArriveAtStop(Event* e) {
   
}
