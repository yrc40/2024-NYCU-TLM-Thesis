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

    cout << "Start simulation process...\n";
}

const int System::getTmax() { return this->Tmax.value(); }

optional<Stop*> System::getNextStop(int stopID) { return findNextStop(stopID); }

optional<Stop*> System::findNextStop(int stopID) {
/**
 * @brief 尋找路線中某個站牌 (`stopID`) 的下一站
 * 
 * 此函式會遍歷 `route` (路線) 集合，尋找 `stopID` 對應的 `Stop` 物件，
 * 並返回其下一個 `Stop` 物件的指標 (若有的話)。
 * 若 `stopID` 是路線中的最後一站，或 `stopID` 不在 `route` 中，則回傳 `nullopt`。
 * 
 * @param stopID 目標站牌的編號
 * @return optional<Stop*> 若找到下一站，則回傳 `Stop*`；否則回傳 `nullopt`
 */
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

optional<variant<Stop*, Light*>> System::findNext(variant<Stop*, Light*> target) {
/**
 * @brief 尋找 `route` (路線) 中某個目標 (`target`) 的下一個元素 (站點或號誌)
 * 
 * 此函式會在 `route` (set 容器) 中尋找 `target`，並回傳其後方的元素。
 * `route` 中的元素為 `variant<Stop*, Light*>`，可能是站點 (`Stop*`) 或號誌 (`Light*`)。
 * 若 `target` 為 `route` 中的最後一個元素，則回傳 `nullopt`。
 * 
 * @param target 目標元素 (`Stop*` 或 `Light*`)
 * @return optional<variant<Stop*, Light*>> 若找到下一個元素，則回傳 `Stop*` 或 `Light*`；否則回傳 `nullopt`
 */
    auto it = route.find(target); // 找到目標元素的位置
    it++; // 位置 +1
    if (holds_alternative<Stop*>(*it)) { // 若為站點
        Stop* stop = std::get<Stop*>(*it);
        return *it; // 回傳指向元素的指標
    } else if (holds_alternative<Light*>(*it)) { // 若為號誌
        Light* light = std::get<Light*>(*it);
        return *it; // 回傳指向元素的指標
    }   

    return nullopt; // 若沒有下一個元素，回傳空指標

}

void System::incrHeadwayDev(float t) {
/**
 * @brief 增加班距偏差 (Headway Deviation)
 * 
 * 此函式用來更新系統的班距偏差 (`headwayDev`)，表示班距與理想狀態的偏離程度。
 * `headwayDev` 為一個累積值，每次呼叫時會將輸入值 `t` 加到 `headwayDev` 上。
 * 
 * @param t 欲增加的班距偏差值 (正數表示偏差增加，負數表示偏差減少)
 */
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
    for (auto& element : this->route) { // 遍歷 route 中的元素
        visit([](auto&& obj) {
            using T = decay_t<decltype(obj)>;
            if constexpr (is_same_v<T, Stop*>) {
                cout << "Stop ID: " << obj->id << " " << obj->stopName << endl;
            } else if constexpr (is_same_v<T, Light*>) {
                cout << "Signal ID: " << obj->id <<  " " << obj->lightName << endl;
            }
        }, element);
    }
}

void System::init() {
/**
 * @brief 初始化系統並讀取設定檔 (config.toml)
 * 
 * 此函式從 `config.toml` 讀取各種設定，包括：
 * - 路線資訊
 * - 站點與號誌參數
 * - 班表資訊
 * - 速度與時間相關參數
 * 
 * 若設定檔缺少必要欄位，則會拋出錯誤並終止程式執行。
 */
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

        /* 讀取時間相關參數 */
        this->Tmax = config["time"]["Tmax"].value<int>();
        this->schemeThreshold = config["time"]["schemeThreshold"].value<double>();

    } catch (const toml::parse_error& e) {
        cerr << "設定檔讀取錯誤：" << e.what() << "\n";
        exit(1);
    }

    this->displayRoute();

}

void System::setupStop(double avg, double sd) {
/**
 * @brief 讀取站點資訊檔案 (stops.csv) 並初始化站點資料
 *
 * 此函式會執行以下步驟：
 * 1. 讀取 `./data/stops.csv` 檔案，解析每個站點的資料。
 * 2. 生成符合 **常態分佈 (Normal Distribution)** 的站距 (mileage)。
 * 3. 設定站點名稱、乘客到站率 (`arrivalRate`)、乘客下車率 (`dropRate`)。
 * 4. 將解析出的 `Stop` 物件插入 `route` 容器內。
 *
 * @param avg 站距的平均值 (meters)
 * @param sd 站距的標準差 (meters)
 */
    /* 初始化變數 */
    string line;
    double current_distance = 0.0, next_distance; // 累積的總距離
    random_device rd;
    mt19937 gen(rd()); // 隨機數生成器
    normal_distribution<> dist(avg, sd); // 常態分佈，均值 avg，標準差 sd

    this->stopAmount = 0; // 記錄站點數量
    int id = 0; // 站點 ID

    ifstream file("./data/stops.csv"); // 開啟站點資訊檔案
    if (!file) {
        throw runtime_error("無法開啟./data/stops.csv\n");
    }

    double tmpAvg, tmpSd; // 暫存讀取的平均值與標準差

    getline(file, line); // 跳過 CSV 檔案的標題行

    /* 逐行讀取站點資料 */
    while (getline(file, line)) {
        stringstream ss(line); // 使用 stringstream 解析 CSV 行
        string field;
        Stop* stop = new Stop; // 創建新的 Stop 物件

        stop->id = id; // 設定站點 ID

        /* 讀取站點名稱 */
        getline(ss, field, ',');
        stop->stopName = field;

        /* 讀取乘客到站率 (arrivalRate) */
        for (int i = 0; i < 3; i++) {
            getline(ss, field, ','); // 讀取平均值
            tmpAvg = stod(field) / 3600; // 轉換為每秒
            getline(ss, field, ','); // 讀取標準差
            tmpSd = stod(field) / 3600; // 轉換為每秒
            stop->arrivalRate[i] = make_pair(tmpAvg, tmpSd); // 存入到站率
        }

        /* 讀取乘客下車率 (dropRate) */
        for (int i = 0; i < 3; i++) {
            getline(ss, field, ','); // 讀取平均值
            tmpAvg = stod(field);
            getline(ss, field, ','); // 讀取標準差
            tmpSd = stod(field);
            stop->dropRate[i] = make_pair(tmpAvg, tmpSd); // 存入下車率
        }

        /* 計算站點的里程數 (mileage) */
        if (stop->id == 0) {
            stop->mileage = 0; // 第一個站點的里程數為 0
        } else {
            next_distance = max(0.0, dist(gen)); // 生成符合常態分佈的距離，確保不小於 0
            current_distance += next_distance; // 累加站距
            stop->mileage = current_distance; // 設定站點的累積里程
        }

        /* 將站點加入路線容器 */
        while (route.insert(stop).second == false) {
            current_distance -= next_distance;
            next_distance = max(0.0, dist(gen)); 
            current_distance += next_distance;
            stop->mileage = current_distance;
            route.insert(stop);
        }
        this->stopAmount++; // 更新站點數量
        id++; // 站點 ID 遞增
    }

    file.close(); // 關閉 CSV 檔案
}

void System::setupSignal(double avg, double sd) {
/**
 * @brief 讀取號誌資訊檔案 (signals.csv) 並初始化號誌資料
 *
 * 此函式會執行以下步驟：
 * 1. 讀取 `./data/signals.csv` 檔案，解析每個號誌的資料。
 * 2. 生成符合 **常態分佈 (Normal Distribution)** 的號誌距離 (mileage)。
 * 3. 設定號誌名稱 (`lightName`)、時相資訊 (`plan.setPhase`)。
 * 4. 確保號誌的 `mileage` 值不與其他站點或號誌重疊。
 *
 * @param avg 號誌間距的平均值 (meters)
 * @param sd 號誌間距的標準差 (meters)
 */
    /* 初始化變數 */
    string line;
    double current_distance = 0.0, next_distance; // 累積的總距離
    int id = 0; // 號誌 ID
    random_device rd;
    mt19937 gen(rd()); // 隨機數生成器
    ifstream file("./data/signals.csv"); // 開啟號誌資訊檔案

    /* 檢查檔案是否成功開啟 */
    if (!file) {
        throw runtime_error("Can't open ./data/signals.csv"); // 若無法開啟檔案，拋出例外
    }

    getline(file, line); // 跳過 CSV 檔案的標題行

    /* 逐行讀取號誌資料 */
    while (getline(file, line)) {
        stringstream ss(line); // 使用 stringstream 解析 CSV 行
        string field;
        Light* light = new Light; // 創建新的 Light 物件 (代表一個號誌)

        /* 讀取號誌 ID */
        getline(ss, field, ',');
        light->id = id;

        /* 讀取號誌名稱 */
        getline(ss, field, ',');
        light->lightName = field;

        /* 讀取並設定號誌時相 (phase) */
        getline(ss, field);
        light->plan.setPhase(field);

        /* 計算號誌的里程數 (mileage) */
        normal_distribution<> dist(avg, sd); // 產生符合常態分佈的號誌距離
        next_distance = max(0.0, dist(gen)); // 確保距離不小於 0
        current_distance += next_distance; // 累計距離
        light->mileage = current_distance; // 設定號誌的里程數

        /* 確保號誌的 mileage 不與其他站點/號誌重疊 */
        while (route.insert(light).second == false) { // 若 `insert` 失敗 (代表已有相同里程的站點或號誌)
            current_distance -= next_distance; // 回退上次的距離變更
            next_distance = max(0.0, dist(gen)); // 重新產生新的距離
            current_distance += next_distance; // 更新累積距離
            light->mileage = current_distance; // 設定新的里程數
            route.insert(light); // 嘗試再次插入
        }

        id++; // 號誌 ID 遞增
    }

    file.close(); // 關閉 CSV 檔案
}

void System::setupSche(int startTime, double avg, double sd, int shift) {
/**
 * @brief 初始化班表 (Schedule) 並生成車輛與事件
 *
 * 此函式會執行以下步驟：
 * 1. 透過 `startTime` 設定首班車時間 (`currentTime`)。
 * 2. 根據 **常態分佈 (Normal Distribution)** 產生車輛發車間距 (`hdwy`)。
 * 3. 根據 `shift` 參數決定發車班次，並建立:
 *    - **班表 (`sche`)**
 *    - **車輛 (`fleet`)**
 *    - **對應事件 (`eventList`)**
 *
 * @param startTime 首班車時間 (秒)
 * @param avg 發車間距的平均值 (秒)
 * @param sd 發車間距的標準差 (秒)
 * @param shift 總發車班次數
 */
    int currentTime = startTime, hdwy = 0; // 當前時間 (currentTime) 與發車間距 (hdwy)
    random_device rd;
    mt19937 gen(rd()); // 隨機數生成器
    normal_distribution<> dist(avg, sd); // 產生符合常態分佈的發車間距

    /* 根據班次數量 (shift) 進行迴圈 */
    for (int i = 0; i < this->shift.value(); i++) {
        hdwy = abs(dist(gen)); // 產生隨機發車間距 (取絕對值避免負數)
        
        if (i > 0) { // 從第二班車開始，將發車間距加到當前時間
            currentTime += hdwy;
        }

        this->sche.push_back(currentTime); // 記錄發車時間

        /* 創建新巴士物件 */
        Bus* newBus = new Bus(i, hdwy); // `i` 為車輛 ID, `hdwy` 為該車的發車間距
        fleet.push_back(newBus); // 加入車隊

        /* 創建事件物件 (代表該班車的發車事件) */
        Event* newEvent = new Event( 
            this->sche[i], // 發車時間
            i, // 車輛 ID
            1, // 事件類型 (1 代表發車)
            0, // 停靠站 ID (0 代表起點站)
            1  // 路線方向
        );
        eventList.push(newEvent); // 將事件加入事件列表
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
/**
 * @brief 處理公車到達站點的事件，並更新相關的車輛與站點狀態
 *
 * 當公車抵達一個站點時，這個函式會：
 * 1. 顯示事件的詳細資訊。
 * 2. 根據事件獲得相關的公車與站點物件。
 * 3. 計算並更新公車與站點的狀態。
 * 4. 處理乘客的上下車。
 * 5. 根據上下車處理的結果更新站點與車輛的績效。
 * 6. 設定並推送下一個事件。
 *
 * @param e 當前的事件物件，代表公車到達某站點
 */
    /* 事件說明 */
    this->printEventDetails(e);  // 顯示事件的詳細資訊 (例如時間、車輛、站點等)

    /* 取得事件元素 */
    Bus* bus = this->findBus(e->getBusID());  // 根據事件中的車輛 ID 取得對應的公車物件
    Stop* stop = this->findStop(e->getStopID());  // 根據事件中的站點 ID 取得對應的站點物件

    /* 取得當前當站的到達率及下車率 */
    double arrivalRate, dropRate;
    // 若站點 ID 不為 0，則使用公車的到達率與下車率；若為 0，則使用依據上個離站事件時間計算的到達率與下車率
    arrivalRate = stop->id ? bus->getArrivalRate() : this->getArrivalRate(e->getTime(), stop);
    dropRate = stop->id ? bus->getDropRate() : this->getDropRate(e->getTime(), stop);

    /* 更新公車狀態 */
    bus->setVol(0);  // 設定車輛速度為 0，代表公車在站點停等
    bus->setLocation(stop->mileage);  // 更新公車的位置為當前站點的里程
    this->sortedFleet();  // 重新排序車隊，確保車輛狀態正確

    /* 更新站點狀態 */
    if (stop->lastArrive >= 0) {  // 若站點有上一班車的到達時間
        stop->pax += static_cast<int>((e->getTime() - stop->lastArrive) * arrivalRate);  // 根據事件時間差與到達率計算新增乘客數
    } else {  // 若站點沒有上一班車的到達時間
        stop->pax += bus->getHeadway() * arrivalRate;  // 根據發車間距與到達率計算新增乘客數
    }

    /* 處理乘客上下車 */
    cout << "Processing Passengers alighting and boarding...\n";
    int dwellTime = this->handlingPax(bus, stop, e->getTime(), dropRate);  // 處理上下車，並返回停留時間 (dwellTime)

    /* 計算績效值 */
    this->eventPerformance(e, stop, bus);  // 計算並更新績效指標

    /* 建立新事件 */
    auto itor = route.find(stop);  // 找到當前站點在路線中的位置
    if (itor != route.end() && itor == prev(route.end())) {  // 若為終點站
        cout << "Arrive at terminal\n\n";  // 顯示已經抵達終點站
        return;   // 結束當前事件，無需再建立新事件
    } else {
        cout << "Continue to next stop...\n";
        // 創建新的事件，表示從當前站點出發
        Event* newEvent = new Event(
            e->getTime() + min(this->getTmax(), max(bus->getDwell(), dwellTime)),  // 新事件的時間為當前時間 + 停留時間
            bus->getId(),  // 車輛 ID
            2,  // 事件類型為 2 (離站)
            e->getStopID(),  // 當前站點 ID
            e->getDirection()  // 當前方向
        );
        eventList.push(newEvent);  // 將新事件推送到事件列表
    }

    bus->setDwell(bus->getDwell() - min(this->getTmax(), bus->getDwell()));  // 更新公車的停留時間，考慮最大允許停留時間 (Tmax)
    cout << "\n";  // 換行顯示
}

void System::deptFromStop(Event* e) {
/**
 * @brief 處理公車離開站點的事件，並更新相關的車輛與站點狀態
 *
 * 當公車離開站點時，這個函式會：
 * 1. 顯示事件的詳細資訊。
 * 2. 根據事件獲得相關的公車與站點物件。
 * 3. 更新公車的到達率與下車率。
 * 4. 根據設定的分佈來計算公車的行駛速度及停留時間。
 * 5. 計算並設定公車的行駛速度，可能也會根據前一班車的速度來調整。
 * 6. 根據速度計算結果的不同情況調整公車速度與停留時間。
 * 7. 設定下一個事件的時間與站點。
 *
 * @param e 當前的事件物件，代表公車離開某站點
 */
   /* 事件說明 */
    this->printEventDetails(e);  // 顯示事件的詳細資訊 (例如時間、車輛、站點等)

    /* 取得事件所需之元素 */
    auto bus = this->findBus(e->getBusID());  // 根據事件中的車輛 ID 查找對應的公車物件
    auto stop = this->findStop(e->getStopID());  // 根據事件中的站點 ID 查找對應的站點物件

    /* 取得當前當站的到達率及下車率 */
    auto arrivalRate = bus->getArrivalRate();  // 取得公車的到達率
    bus->setArrivalRate(arrivalRate);  // 更新公車的到達率
    auto dropRate = bus->getDropRate();  // 取得公車的下車率
    bus->setDropRate(dropRate);  // 更新公車的下車率

    /* 取得平均速度分佈與上下限 */
    random_device rd;
    mt19937 gen(rd());
    normal_distribution<> dist(this->Vavg.value(), this->Vsd.value());
    double Vavg = max(0.0, dist(gen)) / 3.6;  // 計算公車行駛的平均速度 (單位：m/s)
    double Vlimit = this->Vlimit.value() / 3.6;  // 設定行駛速度的上限 (單位：m/s)
    double Vlow = this->Vlow.value() / 3.6;  // 設定行駛速度的下限 (單位：m/s)

    /* 更新公車狀態 */
    bus->setLastGo(e->getTime());  // 設定公車的最後離站時間為當前事件的時間

    /* 計算行駛速度(策略一：置站優先) */
    auto nextStop = this->getNextStop(stop->id);  // 取得當前站點的下一站
    if (nextStop.has_value()) {
        cout << "Next stop is: " << nextStop.value()->id << " " << nextStop.value()->stopName << "\n";

        // 計算上車的乘客數量
        int boardPax = min(nextStop.value()->pax + static_cast<int>(ceil(bus->getHeadway()*arrivalRate)), 
                           static_cast<int>(bus->getCapacity() - (bus->getPax() * dropRate))); // 上車乘客數量
        int paxTime = static_cast<int>(boardPax * (bus->getPax() < 0.65 * bus->getCapacity() ? 2 : 2.7));  // 計算上下車的時間
        int totaldwell = paxTime + bus->getDwell();  // 計算總停留時間
        cout << "total dwell time = " << totaldwell << "\n";

        // 設定公車的行駛速度與停留時間
        Bus* prevBus = this->findPrevBus(bus);  // 找出前一班車
        if (!prevBus) {  // 第一班車
            cout << "The first bus should not follow other's velocity" << "\n";
            bus->setVol(Vavg);  // 設定速度為平均速度
            bus->setDwell(totaldwell);  // 設定停留時間
            cout << "vol = " << bus->getVol() * 3.6 << " kph, dwell time = " << bus->getDwell() << "\n";
        } else {
            double distance, newVol;
            // 取得前車距離
            if (prevBus->getVol()) {
                distance = prevBus->getLocation() + prevBus->getVol() * (e->getTime() - prevBus->getLastGo()) - stop->mileage;
                newVol = distance / (bus->getHeadway() + totaldwell);  // 計算新速度
            } else {
                distance = prevBus->getLocation() - stop->mileage;
                newVol = distance / (bus->getHeadway() + totaldwell);  // 計算新速度
            }
        
            // 如果公車的行駛速度過慢，則恢復到平均速度
            if ((distance / Vavg) < bus->getHeadway() * this->schemeThreshold.value()) {
                newVol = Vavg;
                if (bus->bunching.second) cout << "recovered the bunching problem successfully in " << stop->id - bus->bunching.first << "stops.\n";
                bus->bunching = make_pair(stop->id, 0);
                cout << "No bunching, just run with avg speed.\n";
            } else {
                bus->bunching = make_pair(stop->id, 1);  // 設定為可能發生連班
                cout << "There's might be bus bunching, use the given scheme\n";
            }
            
            // 如果速度太低，進行調整
            if(newVol < Vlow) {
                cout << "Yes it's too close\n";
                cout << "distance: " << distance << "\n";
                cout << "paxTime: " << paxTime << "\n";
                cout << "totalDwell: " << totaldwell << "\n";
                totaldwell += (distance / newVol) - (distance / Vavg);  // 調整總停留時間
                newVol = Vavg;  // 恢復到平均速度
                bus->setVol(newVol);
                bus->setDwell(totaldwell);
            } else if (newVol > Vlimit) {  // 如果速度過快，則限制速度
                cout << "Yes it's too far\n";
                cout << "distance: " << distance << "\n";
                cout << "paxTime: " << paxTime << "\n";
                cout << "totalDwell: " << totaldwell << "\n";
                cout << "hdwy: " << bus->getHeadway() << "\n";
                prevBus->setDwell(prevBus->getDwell() + (distance / Vlimit) - (distance / newVol));  // 調整前一班車的停留時間
                newVol = Vlimit;  // 限制速度為上限
            }

            bus->setVol(newVol);  // 設定新速度
            bus->setDwell(totaldwell);  // 設定新停留時間
            cout << "distance = " << distance << " new Vol = " << newVol * 3.6 << " kph\n";
        } 
    }

    /* 產生新事件 */
    if (stop->id == this->stopAmount - 1) return;  // 如果是終點站，結束事件
    auto nextElement = findNext(stop);  // 找到下一個元素 (站點或號誌)
    if(nextElement.has_value()) {
        // 訪問並建立新事件
        visit([&](auto* obj) {
            using T = decay_t<decltype(*obj)>;
            if constexpr (is_same_v<T, Stop>) {
                // 如果是站點，計算並建立到達該站點的事件
                int dist =  obj->mileage - stop->mileage;
                int newTime = e->getTime() + dist / bus->getVol();
                Event* newEvent = new Event( //arrive at stop
                    newTime, 
                    bus->getId(),
                    1, 
                    obj->id, 
                    e->getDirection()
                );
                eventList.push(newEvent);  // 將新事件加入事件列表
            } else if constexpr (is_same_v<T, Light>) {
                // 如果是號誌，計算並建立到達該號誌的事件
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
                eventList.push(newEvent);  // 將新事件加入事件列表
            }
        }, nextElement.value());
    } else {
        throw runtime_error("找不到路線中下一個元素");  // 如果找不到下一個元素，拋出異常
    }
    cout << "\n";  // 換行顯示
}

void System::arriveAtLight(Event* e) {
/**
 * @brief 處理公車到達號誌的事件，並根據號誌顯示紅綠燈狀態，更新公車狀態。
 *
 * 當公車到達號誌時，這個函式會：
 * 1. 顯示事件的詳細資訊。
 * 2. 根據事件獲得相關的公車與號誌物件。
 * 3. 更新公車的行駛狀態，包括位置與速度。
 * 4. 計算當前號誌的燈號，如果為綠燈則公車繼續行駛，若為紅燈則等待。
 * 5. 根據燈號設定新事件，抵達下一元素或離開號誌。
 *
 * @param e 當前的事件物件，代表公車到達號誌
 */
   /* 事件說明 */
    this->printEventDetails(e);  // 顯示事件的詳細資訊 (例如時間、車輛、號誌等)

    /* 取得事件所需之元素 */
    auto bus = this->findBus(e->getBusID());  // 根據事件中的車輛 ID 查找對應的公車物件
    auto light = this->findSignal(e->getLightID());  // 根據事件中的號誌 ID 查找對應的號誌物件

    /* 更新公車狀態 */
    bus->setLocation(light->mileage);  // 設定公車的當前位置為號誌的位置
    bus->setNextVol(bus->getVol());  // 保存公車的當前速度
    bus->setVol(0.0);  // 設定公車的行駛速度為 0

    /* 計算號誌燈號 */
    int timeRemain = light->plan.calculateSignal(e->getTime());  // 根據事件時間計算剩餘的紅綠燈時間

    /* 根據燈號進行處理 */
    if (timeRemain == 0) {  // 若燈號為綠燈
        cout << "Now is GREEN, just go through...\n";
        bus->setVol(bus->getNextVol());  // 恢復公車的行駛速度
    } else {  // 若燈號為紅燈
        cout << "Now is RED, wait for " << timeRemain <<" seconds...\n\n";
        // 創建新的事件表示等待紅燈
        Event* newEvent = new Event( // 從號誌出發
            e->getTime() + timeRemain,  // 設定新的事件時間為當前時間加上等待時間
            bus->getId(),  // 使用當前公車的 ID
            4,  // 事件類型為 4，表示離開號誌
            light->id,  // 號誌的 ID
            e->getDirection()  // 設定公車的行駛方向
        );
        eventList.push(newEvent);  // 將新的事件加入事件列表
        return;  
    }

    /* 處理下一元素（站點或號誌） */
    auto nextElement = findNext(light);  // 找到下一個元素（站點或號誌）
    if(nextElement.has_value()) {
        // 訪問並處理下一個元素，建立新事件
        visit([&](auto* obj) {
            using T = decay_t<decltype(*obj)>;
            if constexpr (is_same_v<T, Stop>) {  // 如果是站點
                cout << "Next Stop ID: " << obj->id << endl;
                int dist =  obj->mileage - light->mileage;  // 計算從號誌到站點的距離
                int newTime = e->getTime() + dist / bus->getVol();  // 計算到達該站點的時間
                Event* newEvent = new Event( // 到達站點事件
                    newTime, 
                    bus->getId(),
                    1,  // 事件類型為 1，表示到達站點
                    obj->id, 
                    e->getDirection()
                );
                eventList.push(newEvent);  // 將新事件加入事件列表

            } else if constexpr (is_same_v<T, Light>) {  // 如果是號誌
                cout << "Next Light ID: " << obj->id << endl;
                int dist = obj->mileage - light->mileage;  // 計算從當前號誌到下一號誌的距離
                int newTime = e->getTime() + dist / bus->getVol();  // 計算到達下一號誌的時間
                Event* newEvent = new Event( // 到達號誌事件
                    newTime, 
                    bus->getId(),
                    3,  // 事件類型為 3，表示到達號誌
                    obj->id, 
                    e->getDirection()
                );
                eventList.push(newEvent);  // 將新事件加入事件列表
            }
        }, nextElement.value());  // 處理下一元素
    } else {
        cout << "Can't find next element or no next\n";  // 如果找不到下一個元素或沒有下一元素
    }
    
    cout << "\n";  // 換行顯示
}

void System::deptFromLight(Event* e) {
/**
 * @brief 處理公車離開號誌的事件，並根據號誌燈的位置與公車的速度計算到達下一個目的地的時間。
 * 
 * 當公車離開號誌燈時，這個函式會：
 * 1. 顯示事件的詳細資訊。
 * 2. 根據事件獲取相關的公車與號誌物件。
 * 3. 更新公車的狀態，包括設定公車位置、速度與最近一次出發時間。
 * 4. 生成新的事件，指向下一個目的地（站點或號誌燈）。
 *
 * @param e 當前的事件物件，代表公車從號誌燈出發
 */
    /* 顯示事件詳細資訊 */
    this->printEventDetails(e); 

    /* 取得事件所需的物件 */
    auto bus = this->findBus(e->getBusID());  // 根據事件中的車輛 ID 查找對應的公車物件
    auto light = this->findSignal(e->getLightID());  // 根據事件中的號誌燈 ID 查找對應的號誌燈物件

    /* 更新公車狀態 */
    bus->setLocation(light->mileage);  // 設定公車的位置為號誌燈的位置
    bus->setVol(bus->getNextVol());  // 設定公車的速度為上次設定的速度
    bus->setLastGo(e->getTime());  // 設定公車的最近一次出發時間為當前事件的時間

    /* 產生新事件 */
    auto nextElement = findNext(light);  // 查找號誌燈後的下一個元素（可能是站點或號誌燈）
    if (nextElement.has_value()) {  // 如果找到下一個元素
        visit([&](auto* obj) {  // 遍歷下一個元素
            using T = decay_t<decltype(*obj)>;
            
            // 如果下一個元素是站點
            if constexpr (is_same_v<T, Stop>) {
                int dist = obj->mileage - light->mileage;  // 計算從號誌燈到下一站的距離
                int newTime = e->getTime() + dist / bus->getVol();  // 計算到達下一站的時間
                Event* newEvent = new Event(  // 創建一個新的到站事件
                    newTime, 
                    bus->getId(),
                    1,  // 事件類型為「到站」
                    obj->id,  // 站點 ID
                    e->getDirection()  // 方向
                );
                eventList.push(newEvent);  // 將新事件加入事件列表

            // 如果下一個元素是號誌燈
            } else if constexpr (is_same_v<T, Light>) {
                cout << "Next Light ID: " << obj->id << endl;  // 顯示下一個號誌燈的 ID
                int dist = obj->mileage - light->mileage;  // 計算從當前號誌燈到下一號誌燈的距離
                int newTime = e->getTime() + dist / bus->getVol();  // 計算到達下一號誌燈的時間
                Event* newEvent = new Event(  // 創建一個新的到號誌燈事件
                    newTime, 
                    bus->getId(),
                    3,  // 事件類型為「到達號誌」
                    obj->id,  // 號誌燈 ID
                    e->getDirection()  // 方向
                );
                eventList.push(newEvent);  // 將新事件加入事件列表
            }
        }, nextElement.value());  // 呼叫訪問函式並處理下一個元素
    } else {
        cout << "Can't find next element or no next\n";  // 如果找不到下一個元素，顯示錯誤訊息
    }
    cout << "\n";  // 換行
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

