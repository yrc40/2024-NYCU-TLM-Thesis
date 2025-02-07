#include "System.hpp"
#include "toml.hpp"
#include <iomanip>
#include <sstream>

System::System() : route(mileageCmp()) {
    /* 初始化一個模擬系統時，自動設定 event set 的 mapping */
    eventSet[1] = [this](Event* e) { this->arriveAtStop(e); }; // 事件 1: 公車到站
    eventSet[2] = [this](Event* e) { this->deptFromStop(e); }; // 事件 2: 公車離站
    eventSet[3] = [this](Event* e) { this->arriveAtLight(e); }; // 事件 3: 公車到號誌化路口
    eventSet[4] = [this](Event* e) { this->deptFromLight(e); }; // 事件 4: 公車離開號誌化路口
}

const int System::getTmax() { return Tmax; }

optional<Stop*> System::getNextStop(int stopID) { return findNextStop(stopID); }

optional<Stop*> System::findNextStop(int stopID) { // 回傳 Optional: 代表可能有空值
    for (auto it = route.begin(); it != route.end(); ++it) { // 遍歷路線上的元素
        if (auto* stop = get_if<Stop*>(&*it)) { // 如果抓到一個 stop
            if ((*stop)->id == stopID) { // 若這個 stop 的編號是函式傳入的 id
                auto nextIt = next(it);
                while (nextIt != route.end()) { // 往後遍歷所有元素
                    if (auto* nextStop = get_if<Stop*>(&*nextIt)) { // 若抓到一個 stop，即為下一站
                        return *nextStop; // 回傳指向下站元素的指標
                    }
                    ++nextIt;
                }
                cout << "No next Stop found after stopID: " << stopID << endl; // 沒有找到下一站
                return nullopt; // 回傳空指標
            }
        }
    }
    return nullopt; // 若沒有找到目標車站 (stopID)，回傳空指標
}

/* 尋找路線上下一個元素 */
optional<variant<Stop*, Light*>> System::findNext(variant<Stop*, Light*> target) { // 回傳 Optional: 代表可能有空值
    auto it = route.find(target); // 找到目標元素的位置
    it++; // 位置 +1
    if (holds_alternative<Stop*>(*it)) { // 若為站點
        Stop* stop = std::get<Stop*>(*it);
        cout << "Find Next Stop ID: " << stop->id << endl;
        return *it; // 回傳指向元素的指標
    } else if (holds_alternative<Light*>(*it)) { // 若為號誌
        Light* light = std::get<Light*>(*it);
        cout << "Find Next Light ID: " << light->id << endl;
        return *it; // 回傳指向元素的指標
    }   

    return nullopt; // 若沒有下一個元素，回傳空指標

}

/* 計算部分績效的函數 */
void System::incrHeadwayDev(float t) {
    headwayDev += t; // 將績效值加上輸入值 t
}

string showTime(int time) {
    ostringstream oss;
    oss << setw(2) << setfill('0') << time / 3600 << ":"
        << setw(2) << setfill('0') << (time % 3600) / 60;
    return oss.str();
}

int System::time2Seconds(const string& timeStr) {
    int hours = stoi(timeStr.substr(0, 2));
    int minutes = stoi(timeStr.substr(2, 2));
    return hours * 3600 + minutes * 60;
}

/* 模擬系統讀取參數並生成必要元素的函式 */
void System::init() {
    /* 讀取設定檔 */
    try {
        auto config = toml::parse_file( "config.toml" ); // 讀取 general 設定檔

        /* 讀取站點參數及站點檔案 */
        this->stopDistAvg = config["stop"]["distAvg"].value<double>();
        this->stopDistSd = config["stop"]["distSd"].value<double>();
        this->setupStop(this->stopDistAvg.value(), this->stopDistSd.value());

        /* 讀取號誌參數及號誌描述檔 */
        this->signalDistAvg = config["signal"]["distAvg"].value<double>();
        this->signalDistSd = config["signal"]["distSd"].value<double>();
        this->setupSignal(this->signalDistAvg.value(), this->signalDistSd.value());

        /*讀取班表分佈參數並產生班表*/
        auto startTimeOpt = config["schedule"]["startTime"].value<string>();
        if (!startTimeOpt) throw runtime_error("錯誤: TOML 描述檔缺少 'schedule.startTime' 欄位");
        this->scheStart = time2Seconds(*startTimeOpt);

        this->shift = config["schedule"]["shift"].value<int>();
        this->scheAvg = config["schedule"]["avg"].value<double>();
        this->scheAvg.value() *= 60;
        this->scheSd = config["schedule"]["sd"].value<double>();
        this->scheSd.value() *= 60;
        this->setupSche(this->scheStart.value(), this->scheAvg.value(), this->scheSd.value(), this->shift.value());

    } catch (const toml::parse_error& e) {
        cerr << "設定檔讀取錯誤：" << e.what() << "\n";
        exit(1);
    }

    string line;

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

    /*for (int i = 0; i < sche.size(); i++) {
        cout << this->sche[i] << " ";
    }
    cout << endl;*/

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


}

void System::setupStop(double avg, double sd) {
    /*讀取站點資訊檔案 (stops.csv) 並生成符合輸入分佈的站距*/
    string line;
    double current_distance = 0.0;
    random_device rd;
    mt19937 gen(rd());
    normal_distribution<> dist(avg, sd);
    this->stopAmount = 0;
    int id = 0;
    ifstream file( "./data/stops.csv");

    double tmpAvg, tmpSd;
    
    getline(file, line); // 跳過 csv 檔標頭
    while (getline(file, line)) { // 逐行讀取
        stringstream ss(line);
        string field;
        string dummy;
        Stop* stop = new Stop;

        getline(ss, field, ',');
        stop->id = id;

        getline(ss, field, ',');
        stop->stopName = field;

        for (int i = 0; i < 3; i++) {
            getline(ss, field, ',');
            tmpAvg = stod(field);
            getline(ss, field, ',');
            tmpSd = stod(field);
            stop->arrivalRate[i] = make_pair(tmpAvg, tmpSd);
        }

        for (int i = 0; i < 3; i++) {
            getline(ss, field, ',');
            tmpAvg = stod(field);
            getline(ss, field, ',');
            tmpSd = stod(field);
            stop->dropRate[i] = make_pair(tmpAvg, tmpSd);
        }

        if (stop->id == 0) {
            stop->mileage = 0;
        } else {
            double next_distance = std::max(0.0, dist(gen)); 
            current_distance += next_distance;
            stop->mileage = current_distance;
        }
        route.insert(stop);
        this->stopAmount++;
        id++;
    }
    file.close();
}

void System::setupSignal(double avg, double sd) {
    /*讀取號誌資訊檔案 (signals.csv) 並生成符合輸入分佈的站距*/
    string line;
    double current_distance = 0.0;
    int id;
    random_device rd;
    mt19937 gen(rd());
    ifstream file( "./data/signals.csv");

    double tmpAvg, tmpSd;
    getline(file, line); // 跳過 csv 檔標頭
    while (getline(file, line)) { // 逐行讀取
        stringstream ss(line);
        string field;
        string dummy;
        Light* light = new Light;

        getline(ss, field, ',');
        light->id = id;

        getline(ss, field, ',');
        light->lightName = field;

        getline(ss, field);
        light->plan.setPhase(field);

        normal_distribution<> dist(avg, sd);
        if (light->id == 0) {
            light->mileage = 0;
        } else {
            double next_distance = max(0.0, dist(gen)); 
            current_distance += next_distance;
            light->mileage = current_distance;
        }
        route.insert(light);
        id++;
    }
    file.close();
}

void System::setupSche(int startTime, double avg, double sd, int shift) {
    int currentTime = startTime, hdwy = 0;
    random_device rd;
    mt19937 gen(rd());
    normal_distribution<> dist(avg, sd);


    for(int i = 0; i < this->shift.value(); i++) {
        hdwy = abs(dist(gen));
        if(i > 0) {
            currentTime += hdwy;
        }
        this->sche.push_back(currentTime);

        Bus* newBus = new Bus(i, hdwy);
        fleet.push_back(newBus);

        Event* newEvent = new Event( 
            this->sche[i], //time
            i, //bus
            1, //event
            0, //stopid
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
    float arriveRate = getOn[(e->getStopID())][0];
    float dropRate = getOff[(e->getStopID())][0];

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
        float arriveRate = getOn[(nextStop.value()->id)][0];
        float dropRate = getOff[(nextStop.value()->id)][0];
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
            if ((distance / Vavg) < bus->getHeadway() * 0.75) { // || (distance / Vavg) > bus->getHeadway() * 1.5
                newVol = Vavg;
                if (bus->bunching.second) cout << "recovered the bunching problem successfully in " << stop->id - bus->bunching.first << "stops.\n";
                bus->bunching = make_pair(stop->id, 0);
                cout << "No bunching, just run with avg speed.\n";
            } else {
                bus->bunching = make_pair(stop->id, 1);
                cout << "There's might be bus bunching, use the given scheme\n";
            }
            
            if(newVol < Vlow) {
                cout << "Yes it's too close\n";
                cout << "distance: " << distance << "\n";
                cout << "paxTime: " << paxTime << "\n";
                cout << "totalDwell: " << totaldwell << "\n";
                totaldwell += (distance / newVol) - (distance / Vavg);
                newVol = Vavg;
                bus->setVol(newVol);
                bus->setDwell(totaldwell);
            } else if (newVol > Vlimit) {
                cout << "Yes it's too far\n";
                cout << "distance: " << distance << "\n";
                cout << "paxTime: " << paxTime << "\n";
                cout << "totalDwell: " << totaldwell << "\n";
                cout << "hdwy: " << bus->getHeadway() << "\n";
                prevBus->setDwell(prevBus->getDwell() + (distance / Vlimit) - (distance / newVol));
                newVol = Vlimit;
            }

            bus->setVol(newVol);
            bus->setDwell(totaldwell);
            cout << "distance = " << distance << " new Vol = " << newVol * 3.6 << " kph\n";

        } else {
            float distance = prevBus->getLocation() - stop->mileage;
            float newVol = distance / (bus->getHeadway() + totaldwell);

            if ((distance / Vavg) < bus->getHeadway() * 0.75 ) { //|| (distance / Vavg) > bus->getHeadway() * 1.5)
                //cout << "Yes it's too far\n";
                newVol = Vavg;
                if (bus->bunching.second) cout << "recovered the bunching problem successfully in " << stop->id - bus->bunching.first << "stops.\n";
                bus->bunching = make_pair(stop->id, 0);
                cout << "No bunching, just run with avg speed.\n";
            } else {
                 //cout << "Yes it's too close\n";
                bus->bunching = make_pair(stop->id, 1);
                cout << "There's might be bus bunching, use the given scheme\n";
            }

            if(newVol < Vlow) {
                cout << "Yes it's too close\n";
                cout << newVol << "\n";
                cout << "distance: " << distance << "\n";
                cout << "paxTime: " << paxTime << "\n";
                cout << "totalDwell: " << totaldwell << "\n";
                totaldwell += (distance / newVol) - (distance / Vavg);
                newVol = Vavg;
                bus->setVol(newVol);
                bus->setDwell(totaldwell);
            } else if (newVol > Vlimit) {
                cout << "Yes it's too far\n";
                cout << "distance: " << distance << "\n";
                cout << "paxTime: " << paxTime << "\n";
                cout << "totalDwell: " << totaldwell << "\n";
                cout << "hdwy: " << bus->getHeadway() << "\n";
                prevBus->setDwell(prevBus->getDwell() + (distance / Vlimit) - (distance / newVol));
                newVol = Vlimit;
            }

            bus->setVol(newVol);
            bus->setDwell(totaldwell);
            cout << "distance = " << distance << " new Vol = " << newVol * 3.6 << " kph\n";
        }
    }

    /*Find next event*/
    if (stop->id == this->stopAmount) return;
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

void  System::showRoute() {
     for (const auto& element : this->route) {
        visit([](auto&& obj) {
            using T = decay_t<decltype(obj)>;
            if constexpr (is_same_v<T, Stop*>) {
                cout << "Stop ID: " << obj->id << endl;
            } else if constexpr (std::is_same_v<T, Light*>) {
                cout << "Light ID: " << obj->id << endl;
            }
        }, element);
    }
}
