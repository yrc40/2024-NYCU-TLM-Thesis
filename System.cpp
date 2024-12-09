#include "System.hpp"
#include <iomanip>
#include <sstream>

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
                 // cout << "Found stopID: " << stopID << ". Looking for the next stop." << endl;
                auto nextIt = next(it);
                while (nextIt != route.end()) {
                    if (auto* nextStop = get_if<Stop*>(&*nextIt)) {
                        // cout << "Found next stop with ID: " << (*nextStop)->id << endl;
                        return *nextStop;
                    }
                    ++nextIt;
                }
                cout << "No next Stop found after stopID: " << stopID << endl;
                return nullopt;
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

void System::incrHeadwayDev(float t) {
    headwayDev += t;
}

string showTime(int time) {
    ostringstream oss;
    oss << setw(2) << setfill('0') << time / 3600 << ":"
        << setw(2) << setfill('0') << (time % 3600) / 60;
    return oss.str();
}

void System::init() {
    /*TODO: read parameter*/
    string line;
    /*Read Stops*/
    ifstream file( "./data/" + routeName + ".csv");
    
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

        //if (stop->id == 1 || stop->id == 2 || stop->id == 3) 
        route.insert(stop);
    }
    file.close();

    /*Read Lights*/
    
    ifstream file2( "./data/light307.csv");
    getline(file2, line);
    string field;
    while (getline(file2, line)) {

        stringstream ss(line);
        Light* light = new Light;
        string field;

        std::getline(ss, field, ',');
        light->id = stoi(field);

        std::getline(ss, field, ',');
        light->lightName = field;

        std::getline(ss, field, ',');
        light->mileage = stoi(field);

        //if (light->id == 1 || light->id == 2 || light->id == 3 || light->id == 4) 
        route.insert(light);
    }
    file2.close();

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

    /*Read get on rate*/
    ifstream file3("./data/getOn.csv");
    getline(file3, line);
    while (getline(file3, line)) {
        
        stringstream ss(line);
        string field;
        vector<float> item;

        std::getline(ss, field, ',');
        std::getline(ss, field, ',');
        item.push_back(stof(field) / 3600); //18

        std::getline(ss, field, ',');
        item.push_back(stof(field) / 3600); // 19

        std::getline(ss, field, ',');
        item.push_back(stof(field) / 3600); //17

        getOn.push_back(item);
    }
    file3.close();

    /*Read get off rate*/
    ifstream file4("./data/getOff.csv");
    getline(file4, line);
    while (getline(file4, line)) {
        
        stringstream ss(line);
        string field;
        vector<float> item;

        std::getline(ss, field, ',');
        std::getline(ss, field, ',');
        item.push_back(stof(field) / 3600); //18

        std::getline(ss, field, ',');
        item.push_back(stof(field) / 3600); // 19

        std::getline(ss, field, ',');
        item.push_back(stof(field) / 3600); //17

        getOff.push_back(item);
    }
    file4.close();

    /*Read fleet and first departure*/
    for(int i = 0; i < sche.size(); i++) {
        if(i == 0) {
            Bus* newBus = new Bus(0, 300);
            // newBus->setPax((300 * getOn[0][0]) > newBus->getCapacity() ? newBus->getCapacity() : 300 * getOn[0][0]);
            fleet.push_back(newBus);

        } else {
            int hdwy = sche[i] - sche[i-1];
            Bus* newBus = new Bus(i, hdwy);
            // newBus->setPax((hdwy * getOn[0][0]) > newBus->getCapacity() ? newBus->getCapacity() : hdwy * getOn[0][0]);
            fleet.push_back(newBus);
        }
        Event* newEvent = new Event( 
            sche[i], //time
            i, //bus
            1, //event
            1, //stopid
            1
        );
        eventList.push(newEvent);
    }

}

void System::readSche(int trial) {

    ifstream file5("./data/sche.csv");
    string line;
    for(int i=0; i<=trial; i++) {
        getline(file5, line);
    }

    int time, hours, minutes;
    stringstream ss(line);
    string field;

    while (std::getline(ss, field, ',')) {
        string ans = "";
        for (auto &c : field) {
            if(c < '0' || c > '9') {
                continue;
            } else {
                ans += c;
            }
        }

        try {
            time = stoi(ans);
            sche.push_back(time);
        } catch (const std::invalid_argument& e) {
            std::cerr << "\nInvalid argument: " << e.what() << std::endl;
        } catch (const std::out_of_range& e) {
            std::cerr << "Out of range: " << e.what() << std::endl;
        }
    }
    file5.close();
}

void System::arriveAtStop(Event* e) {
    cout << "Time: " << std::setw(2) << std::setfill('0') << e->getTime() / 3600 << ":" <<
        std::setw(2) << std::setfill('0') << (e->getTime() % 3600) / 60 << ":" << 
        std::setw(2) << std::setfill('0') << e->getTime() % 60<< "\n";
    cout << "New Event: Bus " << e->getBusID() << " arrive at stop " << e->getStopID() << "\n";

    /*Get arrive rate and drop rate*/
    float arriveRate = getOn[(e->getStopID()) - 1][0];
    float dropRate = getOff[(e->getStopID()) - 1][0];

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
    sort(fleet.begin(), fleet.end(), [](Bus* a, Bus* b) {
        return a->getLocation() > b->getLocation(); 
    });

    cout << "mileage = " << bus->getLocation() << "\n"; 

    /*Deal with pax*/
    int paxRemain;

    int demand;
    if (stop->lastArrive == -1) {
        paxRemain = (static_cast<int>(bus->getHeadway() * dropRate) > bus->getPax()) ? 0 : (bus->getPax() - bus->getHeadway() * dropRate);
        stop->pax += 300 * arriveRate;
        demand = stop->pax;
    } else {
        paxRemain = (static_cast<int>(bus->getHeadway() * dropRate) > bus->getPax()) ? 0 : (bus->getPax() - bus->getHeadway() * dropRate);
        stop->pax += static_cast<int>((e->getTime() - stop->lastArrive) * arriveRate);
        demand = stop->pax; 
    }

    int availableCapacity = static_cast<int>(bus->getCapacity() - paxRemain);
    cout << "availableCapacity: " << availableCapacity << "\n";

    cout << "Demand: " << demand << "\n";
    int boardPax = (demand > availableCapacity) ? availableCapacity : demand;
    bus->setDwell(boardPax * 2);

    bus->setPax(paxRemain + boardPax);    
    stop->pax -= boardPax;
    cout << "Capacity: " << bus->getCapacity() << ", Current Passenger: " << bus->getPax() << ", Boarded Passenger: " << boardPax << endl;

    Bus* prevBus = nullptr;
    for (auto it = fleet.begin(); it != fleet.end(); ++it) {
        if ((*it)->getId() == busID) {
            if (it != fleet.begin()) {
                prevBus = *(it - 1); 
            } else {
                break;
            }
        }
    }

    // Get distance from proceed bus
    if (prevBus) {
        cout << "Time: " << e->getTime() << ", last arrive time: " << stop->lastArrive <<
            ", scheduled headeay: " << bus->getHeadway() / 60 << " min\n";
        
        cout << "headway deviation: " << abs(static_cast<float>((e->getTime() - stop->lastArrive) - bus->getHeadway())) << " seconds\n";
        this->incrHeadwayDev(pow(static_cast<float>((e->getTime() - stop->lastArrive) - bus->getHeadway()) / static_cast<float>(bus->getHeadway()), 2)); //headway deviation
        cout << "Cumulative headway deviation: " << headwayDev << "\n";
    }

    stop->lastArrive = e->getTime();

    /*New event*/
    auto itor = route.find(stop);
    if (itor != route.end() && itor == prev(route.end())) {
        cout << "Final stop in this route, terminate\n\n";
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
    cout << "Time: " << std::setw(2) << std::setfill('0') << e->getTime() / 3600 << ":" <<
        std::setw(2) << std::setfill('0') << (e->getTime() % 3600) / 60 << ":" << 
        std::setw(2) << std::setfill('0') << e->getTime() % 60<< "\n";
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

    /*Update bus status*/
    bus->setLastGo(e->getTime());
    cout << "mileage = " << bus->getLocation() << "\n";

    //find next stop
    auto nextStop = this->getNextStop(stopID);

    /*Calculate scheme*/
    if (nextStop.has_value()) {
        float arriveRate = getOn[(nextStop.value()->id) - 1][0];
        float dropRate = getOff[(nextStop.value()->id) - 1][0];
        cout << "Next stop is: " << nextStop.value()->id << " " << nextStop.value()->stopName << "\n";
        int boardPax = min(nextStop.value()->pax + static_cast<int>(ceil(bus->getHeadway()*arriveRate)), static_cast<int>(bus->getCapacity() - (bus->getPax() * dropRate))); //pax num
        int paxTime = static_cast<int>(boardPax * (bus->getPax() < 0.65 * bus->getCapacity() ? 2 : 2.7));
        int totaldwell = paxTime + bus->getDwell();
        cout << "total dwell time = " << totaldwell << "\n";
        
        Bus* prevBus = nullptr;
        for (auto it = fleet.begin(); it != fleet.end(); ++it) {
            if ((*it)->getId() == busID) {
                if (it != fleet.begin()) {
                    prevBus = *(it - 1); 
                } else {
                    break;
                }
            }
        }
    
        if (!prevBus) { //first car 
            cout << "The first bus should not follow other's volocity" << "\n";
            bus->setVol(Vavg);
            bus->setDwell(totaldwell);
            cout << "vol = " << bus->getVol() * 3.6 << " kph, dwell time = " << bus->getDwell() << "\n";

        } else if (prevBus->getVol()) {
            float distance = prevBus->getLocation() + prevBus->getVol() * (e->getTime() - prevBus->getLastGo()) - stop->mileage;
            float newVol = distance / (bus->getHeadway() + totaldwell);
            if ((distance / Vavg) < bus->getHeadway() * 0.75) { //
                newVol = Vavg;
                if (bus->bunching.second) cout << "recovered the bunching problem successfully in " << stop->id - bus->bunching.first << "stops.\n";
                bus->bunching = make_pair(stop->id, 0);
                cout << "No bunching, just run with avg speed.\n";
            } else {
                bus->bunching = make_pair(stop->id, 1);
                cout << "There's might be bus bunching, use the given scheme\n";
            }
            
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
            cout << "distance = " << distance << " new Vol = " << newVol * 3.6 << " kph\n";

        } else {
            float distance = prevBus->getLocation() - stop->mileage;
            float newVol = distance / (bus->getHeadway() + totaldwell);

            if ((distance / Vavg) < bus->getHeadway() * 0.75) { //
                newVol = Vavg;
                if (bus->bunching.second) cout << "recovered the bunching problem successfully in " << stop->id - bus->bunching.first << "stops.\n";
                bus->bunching = make_pair(stop->id, 0);
                cout << "No bunching, just run with avg speed.\n";
            } else {
                bus->bunching = make_pair(stop->id, 1);
                cout << "There's might be bus bunching, use the given scheme\n";
            }

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
            cout << "distance = " << distance << " new Vol = " << newVol * 3.6 << " kph\n";
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
    // cout << "!!!!!" << stop->pax << "!!!!\n";
    cout << "\n";
}

void System::arriveAtLight(Event* e) {
    /*Announcement*/
    cout << "Time: " << std::setw(2) << std::setfill('0') << e->getTime() / 3600 << ":" <<
        std::setw(2) << std::setfill('0') << (e->getTime() % 3600) / 60 << ":" << 
        std::setw(2) << std::setfill('0') << e->getTime() % 60<< "\n";
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
        if (auto* s = get_if<Light*>(&item)) {
            if ((*s)->id == lightID) {
                light = *s;
                return true;
            }
        }
        return false;
    });
    bus->setLocation(light->mileage);
    cout << "mileage = " << bus->getLocation() << "\n";

    /*New Event*/
    int randNoise = rand() % 2;
    int wait = 0;
    if (randNoise == 0) {
        cout << "Now is GREEN, just go through...\n";
    } else {
        int redTime = rand() % 2;
        wait = 10 * (redTime + 1);
        cout << "Now is RED, wait for " << wait <<" seconds...\n\n";
        Event* newEvent = new Event( //dept form light
            e->getTime() + wait, 
            bus->getId(),
            4, 
            lightID, 
            e->getDirection()
        );
        eventList.push(newEvent);
        return;
    }

    auto nextElement = findNext(light);
    if(nextElement.has_value()) {
        visit([&](auto* obj) {
            using T = decay_t<decltype(*obj)>;
            if constexpr (is_same_v<T, Stop>) {
                cout << "Next Stop ID: " << obj->id << endl;
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
    
    cout << "\n";
}

void System::deptFromLight(Event* e) {
    /*Announcement*/
    cout << "Time: " << std::setw(2) << std::setfill('0') << e->getTime() / 3600 << ":" <<
        std::setw(2) << std::setfill('0') << (e->getTime() % 3600) / 60 << ":" << 
        std::setw(2) << std::setfill('0') << e->getTime() % 60<< "\n";
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
        if (auto* s = get_if<Light*>(&item)) {
            if ((*s)->id == lightID) {
                light = *s;
                return true;
            }
        }
        return false;
    });

    /*Updat bus status*/
    bus->setLocation(light->mileage);
    cout << "mileage = " << bus->getLocation() << "\n";
    bus->setLastGo(e->getTime());

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
    cout << "\n";
}  


void System::simulation() {

    cout << fleet.size() << "\n";

    while(!eventList.empty()) {
        Event* currentEvent = eventList.top();
        int eventType = currentEvent->getEventType();
        auto it = eventSet.find(eventType);
        if (it != eventSet.end()) {
            it->second(currentEvent);
        } else {
            cout << "Unknown event type: " << eventType << endl;
        }
        eventList.pop();
    }
}

void System::performance() {
    cout << ">>> Performance <<<\n";
    cout << "There were " << fleet.size() << " bus run today.\n";
    cout << "Each line consists of 31 stop.\n"; 
    cout << "Total heawdway deviation: " << this->headwayDev / 1;
    cout << "\nAvg headway deviation: " << this->headwayDev /(fleet.size() - 1);
}
