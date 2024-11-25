#include "System.hpp"

System::System() : route(mileageCmp()) {}

System::System(string routeName) : routeName(routeName), route(mileageCmp()) {
    eventSet[1] = [this](Event* e) { this->arriveAtStop(e); };
    eventSet[2] = [this](Event* e) { this->deptFromStop(e); };
    eventSet[3] = [this](Event* e) { this->arriveAtLight(e); };
    eventSet[4] = [this](Event* e) { this->deptFromLight(e); };
    eventSet[5] = [this](Event* e) { this->paxArriveAtStop(e); };
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

    bus->setLocation(stop->mileage);
    
    int availableCapacity = static_cast<int>(bus->getCapacity() - bus->getPax() * dropRate);  
    int demand = stop->pax; 
    int boardPax = (demand > availableCapacity) ? availableCapacity : demand;

    bus->setPax(availableCapacity + boardPax);    
    stop->pax -= boardPax;

    cout << "Bus " << busID << " arrived at Stop " << stopID << endl;
    cout << "Capacity: " << bus->getCapacity() << ", Current Volume: " << bus->getVol() << ", Boarded Pax: " << boardPax << endl;

    Event* newEvent = new Event( //depart form stop
        e->getTime() + bus->getDwell(), 
        bus->getId(),
        2, 
        e->getStopID(), 
        e->getDirection()
    );

    eventList.push(newEvent);

}

void System::deptFromStop(Event* e) {
    /*Calculate scheme*/

    /*Find next event*/
}

void System::arriveAtLight(Event* e) {
    
}

void System::deptFromLight(Event* e) {
   
}

void System::paxArriveAtStop(Event* e) {
   
}
