#include "System.hpp"
#include "toml.hpp"
#include <iomanip>
#include <sstream>

System::System() : route(mileageCmp()) {
/**
 * @brief 模擬系統建構子，初始化事件處理設置並開始模擬過程
 * 
 * 這個建構子會初始化模擬系統的基本設定，並自動設定每一種類型事件對應的處理函式（即事件集）。
 * 它使用 `eventSet` 來映射事件類型與對應的事件處理函式，並且在初始化完成後開始模擬過程。
 * 
 */
    /* 初始化一個模擬系統時，自動設定 event set 的 mapping */
    eventSet[1] = [this](Event* e) { this->arriveAtStop(e); }; // 事件 1: 公車到站
    eventSet[2] = [this](Event* e) { this->deptFromStop(e); }; // 事件 2: 公車離站
    eventSet[3] = [this](Event* e) { this->arriveAtLight(e); }; // 事件 3: 公車到號誌化路口
    eventSet[4] = [this](Event* e) { this->deptFromLight(e); }; // 事件 4: 公車離開號誌化路口

    cout << "Start simulation process\n";
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

void System::incrHeadwayDev(float t) {
    headwayDev += t; // 將績效值加上輸入值 t
}

void System::printFormattedTime(int seconds) {
/**
 * @brief 格式化輸出時間（以 HH:MM:SS 方式顯示）
 * @param seconds 時間（以秒為單位）
 */   
    cout << setw(2) << setfill('0') << seconds / 3600 << ":" 
         << setw(2) << setfill('0') << (seconds % 3600) / 60 << ":" 
         << setw(2) << setfill('0') << seconds % 60;
}

void System::printEventDetails(Event* e) {
/**
 * @brief 印出公車事件的詳細資訊
 * 
 * 此函式會格式化並輸出事件資訊，包括時間、公車 ID、到達或離開的狀態，以及對應的地點（站牌或號誌）。
 * 
 * @param e 指向事件物件的指標，包含事件的時間、類型、公車 ID、站牌或號誌 ID
 * 
 * @note 事件類型 (`getEventType()`) 的對應關係：
 *       - 0: 公車到達站牌
 *       - 1: 公車離開站牌
 *       - 2: 公車到達號誌
 *       - 3: 公車離開號誌
 * 
 * @see System::printFormattedTime(int) 用於格式化時間輸出
 */
    const char* arrivalStatus[] = { " arrived at ", " departed from ", " arrived at ", " departed from " };
    const char* locationType[] = { "stop ", "stop ", "signal ", "signal "};

    cout << "\nTime: ";
    printFormattedTime(e->getTime());
    cout << "\n";

    cout << "New Event: Bus " << e->getBusID() 
         << arrivalStatus[e->getEventType() - 1]
         << locationType[e->getEventType() - 1];
    if (e->getEventType() == 1 || e->getEventType() == 2) {
        cout << e->getStopID() << "\n\n";
    } else {
        cout << e->getLightID() << "\n\n";
    }
}


int System::time2Seconds(const string& timeStr) {
/**
 * @brief 將時間字串轉換為對應的秒數
 * 
 * @param timeStr 時間字串，格式應為 "HHMM"（24 小時制）
 * @return int 轉換後的總秒數（從 00:00 開始計算）
 * @throws runtime_error 當輸入格式錯誤或長度不符時拋出異常
 */

    if (timeStr.length() != 4) {
        throw runtime_error("時間字串，格式應為 \"HHMM\"（24 小時制）");
    }
    int hours = stoi(timeStr.substr(0, 2));
    int minutes = stoi(timeStr.substr(2, 2));

    return hours * 3600 + minutes * 60;
}

pair<int, int> System::timeRange2Pair(const string& timeStr){
/**
 * @brief 解析時間範圍字串，例如 "1830-1900" 或 "1800"，輸入一個數字時為該小時內的 0 分至 59 分
 * 
 * @param  timeStr 時間字串，格式可以是 "HHMM-HHMM" 或 "HHMM"
 * @return pair<int, int> 解析後的時間範圍 (開始時間, 結束時間)
 * @throws runtime_error 當格式錯誤時拋出異常
 */
    size_t dashPos = timeStr.find('-'); // 查找 "-" 的位置

    if (dashPos == string::npos) { 
        // **[情況 1]** 沒有 "-"，代表單一小時，例如 "1800"
        
        // 確保時間字串長度為 4，且全為數字
        if (timeStr.size() != 4 || !all_of(timeStr.begin(), timeStr.end(), ::isdigit)) {
            throw runtime_error("時間格式錯誤: " + timeStr);
        }

        int time = stoi(timeStr); // 轉換為整數
        return {time, time + 59*60}; // 回傳 (開始時間, 結束時間) 一樣
    } else { 
        // **[情況 2]** 有 "-"，代表時間範圍，例如 "1830-1900"

        string startStr = timeStr.substr(0, dashPos);    // 擷取開始時間
        string endStr = timeStr.substr(dashPos + 1);     // 擷取結束時間

        // 檢查長度與格式
        if (startStr.size() != 4 || endStr.size() != 4 || 
            !all_of(startStr.begin(), startStr.end(), ::isdigit) ||
            !all_of(endStr.begin(), endStr.end(), ::isdigit)) {
            throw runtime_error("時間範圍格式錯誤: " + timeStr);
        }

        int startTime = time2Seconds(startStr); // 轉換開始時間
        int endTime = time2Seconds(endStr);     // 轉換結束時間

        // 檢查開始時間不能大於結束時間
        if (startTime > endTime) {
            throw runtime_error("時間範圍錯誤，開始時間不能大於結束時間: " + timeStr);
        }

        return {startTime, endTime}; // 回傳解析後的時間範圍
    }
}

void System::displayRoute() {
/**
 * @brief 顯示系統中的路線資訊
 * 
 * 遍歷 `this->route` 容器，並根據元素的類型 (`Stop*` 或 `Light*`) 顯示對應的 ID。
 * `route` 容器中存儲的是 `std::variant<Stop*, Light*>`，因此使用 `std::visit` 來處理不同類型的物件。
 */
    for (const auto& element : this->route) {
        visit([](auto&& obj) {
            using T = decay_t<decltype(obj)>;
            if constexpr (is_same_v<T, Stop*>) {
                cout << "Stop ID: " << obj->id << endl;
            } else if constexpr (is_same_v<T, Light*>) {
                cout << "Signal ID: " << obj->id << endl;
            }
        }, element);
    }
}

void System::init() {
    /* 讀取設定檔 */
    try {
        auto config = toml::parse_file( "config.toml" ); // 讀取 general 設定檔

        /* 讀取路線基本資料 */
        this->routeName = config["general"]["route"].value_or("Testcase");

        auto peakOpt = config["general"]["morningPeak"].value<string>();
        if (!peakOpt) throw runtime_error("錯誤: TOML 描述檔缺少 'general.morningPeak' 欄位");
        this->morningPeak = timeRange2Pair(*peakOpt);

        peakOpt = config["general"]["eveningPeak"].value<string>();
        if (!peakOpt) throw runtime_error("錯誤: TOML 描述檔缺少 'general.eveningPeak' 欄位");
        this->eveningPeak = timeRange2Pair(*peakOpt);
        

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

        /* 讀取速度相關參數 */
        this->Vavg = config["velocity"]["avg"].value<double>();
        this->Vsd = config["velocity"]["sd"].value<double>();
        this->Vlimit = config["velocity"]["limit"].value<double>();
        this->Vlow = config["velocity"]["low"].value<double>();

    } catch (const toml::parse_error& e) {
        cerr << "設定檔讀取錯誤：" << e.what() << "\n";
        exit(1);
    }

    this->displayRoute();

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
            tmpAvg = stod(field) / 3600;
            getline(ss, field, ',');
            tmpSd = stod(field) / 3600;
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
            double next_distance = max(0.0, dist(gen)); 
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
    /* 讀取號誌資訊檔案 (signals.csv) 並生成符合輸入分佈的站距 */
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

Bus* System::findPrevBus(Bus* target) {
/**
 * @brief 查找目標公車 (target) 在車隊中的前一輛公車
 * 
 * 這個函式會根據公車的位置 (`getLocation()`) 排序車隊 (`fleet`)，
 * 然後遍歷車隊尋找目標公車 (`target`) 前方的那輛公車。
 * 
 * @param target 目標公車 (欲查找前一輛公車的對象)
 * @return Bus* 若找到前一輛公車，則回傳該公車指標；若目標公車為第一輛，則回傳 nullptr
 */
    // 先依照公車位置小到大排序，確保順序正確
    sort(this->fleet.begin(), this->fleet.end(), [](Bus* a, Bus* b) {
        return a->getLocation() < b->getLocation();
    });

    // 遍歷車隊，找到第一輛比 `target` 位置更大的公車
    for (auto it = fleet.begin(); it != fleet.end(); ++it) {
        if ((*it)->getLocation() > target->getLocation()) {
               return *it; 
        }
    }
    // 若沒有找到比 `target` 位置更大的公車，回傳 nullptr
    return nullptr;
} 

double System::getArrivalRate(int time, Stop* stop) {
/**
 * @brief 根據時間與站點的到達率計算公車的隨機到達率
 * 
 * 此函式會依據給定的時間 (`time`)，判斷當前時段屬於
 * 早高峰、晚高峰或一般時段，並根據站點 (`stop`) 提供的
 * 平均到達率 (`arrivalRateAvg`) 和標準差 (`arrivalRateSd`)
 * 來產生一個符合常態分佈的隨機到達率。
 * 
 * @param time 當前時間（以秒為單位）
 * @param stop 指向 `Stop` 物件的指標，用於獲取該站點的到達率
 * @return double 計算出的隨機到達率，確保不小於 0
 * 
 * @note
 * - 早上尖峰 (`morningPeak`) 的到達率使用 `stop->arrivalRate[0]`
 * - 下午尖峰 (`eveningPeak`) 的到達率使用 `stop->arrivalRate[1]`
 * - 離峰時間使用 `stop->arrivalRate[2]`
 * - 使用 `std::normal_distribution` 來產生隨機變數
 */
    // 初始化隨機數生成器
    random_device rd;
    mt19937 gen(rd());

    double arrivalRateAvg, arrivalRateSd;

    // 根據當前時間選擇對應的到達率平均值與標準差
    if (time >= this->morningPeak.first && time <= this->morningPeak.second) {
        arrivalRateAvg = stop->arrivalRate[0].first;
        arrivalRateSd = stop->arrivalRate[0].second;    
    } else if (time >= this->eveningPeak.first && time <= this->eveningPeak.second) {
        arrivalRateAvg = stop->arrivalRate[1].first;
        arrivalRateSd = stop->arrivalRate[1].second;
    } else {
        arrivalRateAvg = stop->arrivalRate[2].first;
        arrivalRateSd = stop->arrivalRate[2].second;
    }

    // 使用常態分佈來生成隨機到達率
    normal_distribution<> dist(arrivalRateAvg, arrivalRateSd);

    // 確保回傳值不小於 0
    return max(0.0, dist(gen));
}

double System::getDropRate(int time, Stop* stop) {
/**
 * @brief 根據時間與站點的下車率計算公車的隨機下車率
 * 
 * 此函式會依據給定的時間 (`time`)，判斷當前時段屬於
 * 早高峰、晚高峰或一般時段，並根據站點 (`stop`) 提供的
 * 平均下車率 (`dropRateAvg`) 和標準差 (`dropRateSd`)
 * 來產生一個符合常態分佈的隨機下車率。
 * 
 * @param time 當前時間（以秒為單位）
 * @param stop 指向 `Stop` 物件的指標，用於獲取該站點的下車率
 * @return double 計算出的隨機下車率，確保不小於 0
 * 
 * @note
 * - 早上尖峰 (`morningPeak`) 的下車率使用 `stop->dropRate[0]`
 * - 早上尖峰 (`eveningPeak`) 的下車率使用 `stop->dropRate[1]`
 * - 離峰時間使用 `stop->dropRate[2]`
 * - 使用 `std::normal_distribution` 來產生隨機變數
 */
    // 初始化隨機數生成器
    random_device rd;
    mt19937 gen(rd());

    double dropRateAvg, dropRateSd;

    // 根據當前時間選擇對應的下車率平均值與標準差
    if (time >= this->morningPeak.first && time <= this->morningPeak.second) {
        dropRateAvg = stop->dropRate[0].first;
        dropRateSd = stop->dropRate[0].second;    
    } else if (time >= this->eveningPeak.first && time <= this->eveningPeak.second) {
        dropRateAvg = stop->dropRate[1].first;
        dropRateSd = stop->dropRate[1].second;
    } else {
        dropRateAvg = stop->dropRate[2].first;
        dropRateSd = stop->dropRate[2].second;
    }

    // 使用常態分佈來生成隨機下車率
    normal_distribution<> dist(dropRateAvg, dropRateSd);

    // 確保回傳值不小於 0
    return max(0.0, dist(gen));
}

Bus* System::findBus(int id) {
/**
 * @brief 根據公車 ID 在車隊中尋找對應的公車物件
 * 
 * 此函式會遍歷 `fleet` 容器，尋找 `id` 相匹配的 `Bus` 物件，
 * 若找到則回傳該 `Bus` 指標，否則拋出 `runtime_error` 異常。
 * 
 * @param id 要查找的公車 ID
 * @return Bus* 指向對應 ID 的 `Bus` 物件指標
 * 
 * @throws runtime_error 若找不到對應 ID 的公車則拋出異常
 * 
 * @note
 * - `fleet` 是一個 `vector<Bus*>`，存放所有公車的指標
 * - 使用 `for (auto& b : this->fleet)` 來遍歷 `fleet`
 * - 若找不到符合條件的公車，則拋出錯誤訊息
 */
    for (auto& b : this->fleet) {
        if (b->getId() == id) {
            return b;
        }
    }
    
    throw runtime_error("找不到 id = " + to_string(id) + " 的公車\n");
}

Stop* System::findStop(int id) {
/**
 * @brief 根據站點 ID 在路線中尋找對應的站點物件
 * 
 * 此函式會遍歷 `route` 容器，嘗試尋找 `id` 相匹配的 `Stop` 物件，
 * 若找到則回傳該 `Stop` 指標，若找不到則輸出錯誤訊息並拋出異常。
 * 
 * @param id 要查找的站點 ID
 * @return Stop* 指向對應 ID 的 `Stop` 物件指標
 * 
 * @throws std::runtime_error 若找不到對應 ID 的站點則拋出異常
 * 
 * @note
 * - `route` 是一個 `vector<variant<Stop*, Light*>>`，存放 `Stop*` 或 `Light*`
 * - 使用 `find_if` 搭配 `get_if` 來判斷 `variant` 內的型別是否為 `Stop*`
 */
    // 使用 find_if 來搜尋符合條件的 Stop*
    auto it = find_if(route.begin(), route.end(), [&](const variant<Stop*, Light*>& item) {
        if (auto* s = get_if<Stop*>(&item)) { // 確認 item 是否為 Stop*
            return (*s)->id == id;
        }
        return false;
    });

    // 如果找到符合的站點，則回傳對應的 Stop*
    if (it != route.end()) {
        return get<Stop*>(*it);
    } 
    
    // 若找不到則拋出異常
    throw runtime_error("找不到 ID = " + to_string(id) + " 的站點");
}

Light* System::findSignal(int id) {
/**
 * @brief 根據站點 ID 在路線中尋找對應的號誌物件
 * 
 * 此函式會遍歷 `route` 容器，嘗試尋找 `id` 相匹配的 `Light` 物件，
 * 若找到則回傳該 `Light` 指標，若找不到則輸出錯誤訊息並拋出異常。
 * 
 * @param id 要查找的站點 ID
 * @return Light* 指向對應 ID 的 `Light` 物件指標
 * 
 * @throws std::runtime_error 若找不到對應 ID 的站點則拋出異常
 * 
 * @note
 * - `route` 是一個 `vector<variant<Stop*, Light*>>`，存放 `Stop*` 或 `Light*`
 * - 使用 `find_if` 搭配 `get_if` 來判斷 `variant` 內的型別是否為 `Light*`
 */
    // 使用 find_if 來搜尋符合條件的 Light*
    auto it = find_if(route.begin(), route.end(), [&](const variant<Stop*, Light*>& item) {
        if (auto* s = get_if<Light*>(&item)) { // 確認 item 是否為 Light*
            return (*s)->id == id;
        }
        return false;
    });

    // 如果找到符合的站點，則回傳對應的 Light*
    if (it != route.end()) {
        return get<Light*>(*it);
    } 
    
    // 若找不到則拋出異常
    throw runtime_error("找不到 ID = " + to_string(id) + " 的號誌");
}

int System::handlingPax(Bus* bus, Stop* stop, int time, double dropRate) {
/**
 * @brief 處理公車在停靠站時的乘客上下車過程
 * 
 * 根據到達率 (arrivalRate) 和下車率 (dropRate)，計算公車在停靠站時的乘客變動，並更新相關數值。
 * 這個函式會處理以下幾個步驟：
 * 1. 計算需要下車的乘客數量 (dropPax)。
 * 2. 計算車上剩餘的乘客數量 (paxRemain)。
 * 3. 計算停靠站需求 (demand) 和公車可用的剩餘容量 (availableCapacity)。
 * 4. 根據需求和容量計算上車的乘客數量 (boardPax)。
 * 5. 更新公車上的乘客數量 (setPax)，並減少停靠站的需求。
 * 6. 更新公車的停留時間 (setDwell)，即根據上車乘客數量設定。
 * 
 * @param bus 當前處理的公車對象。
 * @param stop 當前停靠的站點對象。
 * @param arrivalRate 到達率。
 * @param dropRate 下車率。
 *
 * @return dwellTime 乘客上下車花費之時

 */
    int paxRemain, dropPax, demand, availableCapacity, boardPax, timePassed;
    timePassed = (stop->lastArrive >=0) ? (time - stop->lastArrive) : bus->getHeadway();
    dropPax = min(bus->getPax(), static_cast<int>(timePassed * dropRate));
    paxRemain = bus->getPax() - dropPax;
    demand = stop->pax;
 
    availableCapacity = static_cast<int>(bus->getCapacity() - paxRemain);
    cout << "availableCapacity: " << availableCapacity << "\n";

    cout << "Demand: " << demand << "\n";
    boardPax = (demand > availableCapacity) ? availableCapacity : demand;
    int dwellTime = static_cast<int>(boardPax * (bus->getPax() < 0.65 * bus->getCapacity() ? 2 : 2.7));

    bus->setPax(paxRemain + boardPax);    
    stop->pax -= boardPax;
    cout << "Capacity: " << bus->getCapacity() << ", Current Passenger: " << bus->getPax() << ", Boarded Passenger: " << boardPax << endl;

    return dwellTime;
}

void System::sortedFleet() {
/**
 * @brief 根據公車的位置對車隊進行排序
 * 
 * 這個函式會根據每輛公車的地理位置 (`getLocation()`) 來對車隊 (`fleet`) 進行排序，
 * 排序的順序是從大到小，也就是說，位置較靠前的公車會排在前面。
 *
 */
    sort(fleet.begin(), fleet.end(), [](Bus* a, Bus* b) {
        return a->getLocation() > b->getLocation(); 
    });
}

void System::eventPerformance(Event* e, Stop* stop, Bus* bus) {
/**
 * @brief 處理公車事件的執行結果並計算與前一輛公車抵達的時間差異
 * 
 * 此函式會檢查當前公車 (bus) 相對於上一輛公車 (prevBus) 的行駛時間差，並根據這些資訊進行相應的輸出與統計。
 * 它會顯示當前時間與上一輛公車的抵達時間之間的時間差，並計算偏差量。
 * 同時更新停靠站的上一輛公車抵達時間。
 * 
 * @param e 公車事件 (Event)，包含當前事件的時間與相關資料
 * @param stop 停靠站 (Stop)，儲存該站點的各項資訊，包括上一輛公車的抵達時間
 * @param bus 公車 (Bus)，需要被檢查的公車，並計算與前一輛公車的抵達時間差
 */
    Bus* prevBus = this->findPrevBus(bus);
    if (prevBus) {
        cout << "Now: "; 
        this->printFormattedTime(e->getTime());
        cout << ", last arrive time: ";
        this->printFormattedTime(stop->lastArrive); 
        cout << ", scheduled headeay: " << bus->getHeadway() / 60 << " min\n";
        
        cout << "headway deviation: " << abs(static_cast<float>((e->getTime() - stop->lastArrive) - bus->getHeadway())) << " seconds\n";
        this->incrHeadwayDev(pow(static_cast<float>((e->getTime() - stop->lastArrive) - bus->getHeadway()) / static_cast<float>(bus->getHeadway()), 2)); //headway deviation
        cout << "Cumulative headway deviation: " << this->headwayDev << "\n";
    }
    stop->lastArrive = e->getTime();
}

void System::arriveAtStop(Event* e) {
    /* 事件說明 */
    this->printEventDetails(e);

    /* 取得事件元素 */
    Bus* bus = this->findBus(e->getBusID());
    Stop* stop = this->findStop(e->getStopID());
    
    /* 取得當前當站的到達率及下車率 */
    double arrivalRate, dropRate;
    arrivalRate = stop->id ? bus->getArrivalRate() : this->getArrivalRate(e->getTime(), stop);
    dropRate = stop->id ? bus->getDropRate() : this->getDropRate(e->getTime(), stop);

    /* 更新公車狀態 */
    bus->setVol(0);
    bus->setLocation(stop->mileage);
    this->sortedFleet();

    /* 更新站點狀態 */
    if (stop->lastArrive >= 0) {
        stop->pax += bus->getHeadway() * arrivalRate;
    } else {
        stop->pax += static_cast<int>((e->getTime() - stop->lastArrive) * arrivalRate);
    }

    /* 處理乘客上下車 */
    cout << "Processing Passengers alighting and boarding...\n";
    int dwellTime = this->handlingPax(bus, stop, e->getTime(), dropRate);

    /* 計算績效值 */
    this->eventPerformance(e, stop, bus);

    /* 建立新事件 */
    auto itor = route.find(stop);
    if (itor != route.end() && itor == prev(route.end())) {
        cout << "抵達終點站\n\n";
        return;   
    } else {
        cout << "繼續前往下一元素 ...\n";
        Event* newEvent = new Event( //depart form stop
            e->getTime() + min(this->getTmax(), max(bus->getDwell(), dwellTime)), 
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
    /* 事件說明 */
    this->printEventDetails(e);

    /* 取得事件所需之元素 */
    auto bus = this->findBus(e->getBusID());
    auto stop = this->findStop(e->getStopID());

    /* 取得當前當站的到達率及下車率 */
    auto arrivalRate = bus->getArrivalRate();
    bus->setArrivalRate(arrivalRate);
    auto dropRate = bus->getDropRate();
    bus->setDropRate(dropRate);

    /* 取得平均速度分佈與上下限 */
    random_device rd;
    mt19937 gen(rd());
    normal_distribution<> dist(this->Vavg.value(), this->Vsd.value());
    double Vavg = max(0.0, dist(gen)) / 3.6;
    double Vlimit = this->Vlimit.value() / 3.6;
    double Vlow = this->Vlow.value() / 3.6;

    /* 更新公車狀態 */
    bus->setLastGo(e->getTime());

    /* 計算行駛速度(策略一：置站優先) */
    auto nextStop = this->getNextStop(stop->id);
    if (nextStop.has_value()) {
        cout << "Next stop is: " << nextStop.value()->id << " " << nextStop.value()->stopName << "\n";
        int boardPax = min(nextStop.value()->pax + static_cast<int>(ceil(bus->getHeadway()*arrivalRate)), static_cast<int>(bus->getCapacity() - (bus->getPax() * dropRate))); //pax num
        int paxTime = static_cast<int>(boardPax * (bus->getPax() < 0.65 * bus->getCapacity() ? 2 : 2.7));
        int totaldwell = paxTime + bus->getDwell();
        cout << "total dwell time = " << totaldwell << "\n";

        Bus* prevBus = this->findPrevBus(bus);
        if (!prevBus) { //first car 
            cout << "The first bus should not follow other's volocity" << "\n";
            bus->setVol(Vavg);
            bus->setDwell(totaldwell);
            cout << "vol = " << bus->getVol() * 3.6 << " kph, dwell time = " << bus->getDwell() << "\n";

        } else {
            double distance, newVol;
            if (prevBus->getVol()) {
                distance = prevBus->getLocation() + prevBus->getVol() * (e->getTime() - prevBus->getLastGo()) - stop->mileage;
                newVol = distance / (bus->getHeadway() + totaldwell);
            } else {
                distance = prevBus->getLocation() - stop->mileage;
                newVol = distance / (bus->getHeadway() + totaldwell);
            }
        
            if ((distance / Vavg) < bus->getHeadway() * 0.75) {
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

        } 
    }

    /* 產生新事件 */
    if (stop->id == this->stopAmount - 1) return;
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
        throw runtime_error("找不到路線中下一個元素");
    }
    cout << "\n";
}

void System::arriveAtLight(Event* e) {
    /* 事件說明 */
    this->printEventDetails(e);

    /*Find target*/
    Bus* bus = this->findBus(e->getBusID());
    Light* light = this->findSignal(e->getLightID());
    
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
            light->id, 
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
    this->printEventDetails(e);
    
    Bus* bus = this->findBus(e->getBusID());
    Light* light = this->findSignal(e->getLightID());

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
/**
 * @brief 模擬系統事件處理流程
 * 
 * 這個函式會不斷從事件列表 (`eventList`) 取出最高優先級的事件，並根據事件類型執行相對應的處理函式。
 * 當事件處理完後，該事件會從 `eventList` 移除，直到 `eventList` 為空，模擬才會結束。
 * 
 * @throws std::runtime_error 如果遇到未知的事件類型，則拋出異常。
 */
    while(!eventList.empty()) {
        Event* currentEvent = eventList.top();
        int eventType = currentEvent->getEventType();
        auto it = eventSet.find(eventType);
        if (it != eventSet.end()) {
            it->second(currentEvent);
        } else {
            throw runtime_error("未知的事件種類: " + to_string(eventType));
        }
        eventList.pop();
    }
}

void System::performance() {
    cout << ">>> Performance <<<\n";
    cout << "There were " << fleet.size() << " bus run today.\n";
    cout << "Each line consists of " << this->stopAmount << " stop.\n"; 
    cout << "Total heawdway deviation: " << this->headwayDev / 1;
    cout << "\nAvg headway deviation: " << this->headwayDev /(fleet.size() - 1);
}

void  System::showRoute() {
    for (const auto& element : this->route) {
        visit([](auto&& obj) {
            using T = decay_t<decltype(obj)>;
            if constexpr (is_same_v<T, Stop*>) {
                cout << "Stop ID: " << obj->id << "\n";
            } else if constexpr (std::is_same_v<T, Light*>) {
                cout << "Light ID: " << obj->id << endl;
            }
        }, element);
    }
}
