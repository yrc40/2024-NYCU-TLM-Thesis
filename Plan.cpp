#include "Plan.hpp"
#include <regex>
#include <string>

Plan::Plan() {};

void Plan::setPhase(string config) {
/**
 * @brief 設置號誌的時相配置
 * 
 * 這個函式負責解析號誌配置字串並提取相關的時相設定，然後針對每個時相進行處理。
 * 它使用正則表達式將配置字串中的不同時相段提取出來，並對每個段落進行處理。
 * 
 * @param config signals.csv 的號誌時制設定字串
 */
    vector<string> segments;
    regex pattern(R"(/[^/]+/[^/]+/[^/]+/[^/]+,[^/]+/)");  // 正則表達式匹配 /.../.../.../.../

    /* 使用 sregex_iterator 遍歷配置字串，並根據正則表達式提取匹配的時相 */
    sregex_iterator it(config.begin(), config.end(), pattern); 
    sregex_iterator end;

    while (it != end) {
        segments.push_back(it->str());
        it++;
    }

    /* 遍歷所有提取的時相並進行處理 */
    for (const auto& seg : segments) {
        processSegment(seg);   
    }
}

void Plan::processSegment(string segment) {
/**
 * @brief 處理號誌相位配置段
 * 
 * 這個函式會解析號誌相位配置段，將其拆分為時間、週期、偏移量和時相區間等部分，
 * 並將這些資訊儲存到 `time`、`cycle`、`offset` 和 `phase` 各個成員中。
 * 若遇到格式錯誤或無效數據，會拋出異常。
 * 
 * @param segment 來自配置的號誌相位段，格式為 "/時間/週期/偏移量/時相區間/"
 */
    vector<string> parts; // 存儲分割後的部分 (時間、週期、偏移量、時相區間)
    string temp;
    stringstream ss(segment.substr(1, segment.size() - 2));  // 去掉開頭和結尾的 `/`

    while (getline(ss, temp, '/')) {  
        parts.push_back(temp);
    }

    /* 檢查 parts 中的元素數量是否符合要求，應該至少有 4 個部分 */
    if (parts.size() < 4) {
        throw runtime_error("時制資料格式錯誤，應該至少有 4 個部分\n");
    }

    this->time.push_back(time2Seconds(parts[0])); // 解析時間（轉換為秒）
    this->cycle.push_back(stoi(parts[1])); // 解析並儲存週期

    /* 如果偏移量尚未初始化，則初始化第一個偏移量，否則，累積計算偏移量 */
    if (!this->offset.size()) {
        this->offset.push_back(stoi(parts[2]));
    } else {
        int cumulativeOffset = (this->offset[this->offset.size() - 1] +  stoi(parts[2])) % stoi(parts[1]);
        this->offset.push_back(cumulativeOffset);
    }
    
    // 解析 pair 部分
    if (parts.size() > 3) {
        stringstream pairStream(parts[3]);  // 用於處理時相區間的字串流
        string pairItem;
        vector<pair<int, int>> tmp;  // 存儲一組時相區間的起始和結束時間

        // 使用 ',' 分割時相區間，並處理每一組數據
        while (getline(pairStream, pairItem, ',')) {
            int value = stoi(pairItem);  // 將字串轉為整數

            
            if (tmp.empty()) { // 如果還沒有加入任何時間區間
                tmp.emplace_back(value, 0);  // 記錄第一個區間的起始時間，結束時間暫時設為 0
            } else {
                // 如果已有加入時間
                if (!tmp.back().second) { // 若最後一個區間的結束時間為 0 則需要做設定
                    if (value <= tmp.back().first)  
                        throw runtime_error("時相區間的結束時間必須大於起始時間\n");
                    tmp.back().second = value;  // 設定結束時間
                } else { // 開新的區間，並設定區間起始值
                    tmp.emplace_back(value, 0);  // 如果還沒有結束時間，則開始新的區間
                }
            }
        }
        
        // 如果最後一個區間沒有結束時間，則拋出錯誤
        if (tmp.back().second == 0) 
            throw runtime_error("時相區間對輸入必須成對\n");

        // 將處理好的時相區間推入 `phase` 容器
        this->phase.push_back(tmp);
    }

}

int Plan::time2Seconds(const string& timeStr) {
/**
 * @brief 將時間字串轉換為秒數
 * 
 * 這個函式會解析時間字串 (格式為 HHMM)，並將其轉換為以秒為單位的時間。
 * 
 * @param timeStr 時間字串，格式為 "HHMM" (兩位數小時 + 兩位數分鐘)
 * @return int 轉換後的總秒數
 * @throws std::out_of_range 若 `timeStr` 長度不足 4，可能會導致 `substr` 失敗並拋出異常
 */
    if (timeStr.length() < 4) {
        throw runtime_error("時間字串 " + timeStr + " 長度必須大於 4\n");
    }
    int hours = stoi(timeStr.substr(0, 2));
    int minutes = stoi(timeStr.substr(2, 2));
    return hours * 3600 + minutes * 60;
}

int Plan::calculateSignal(int timeStamp) {
/**
 * @brief 計算當前時間戳 (timeStamp) 對應的號誌燈狀態
 * 
 * 此函式根據時間表 (`this->time`) 找出當前 `timeStamp` 所屬的時段，
 * 然後計算當前時間在該時段的週期內對應的時相 (phase)。
 * 若目標時間落在綠燈區間內，則返回 0，否則計算距離下一個綠燈的時間。
 * 
 * @param timeStamp 當前時間戳 (單位：秒)
 * @return int 若當前時相為綠燈，則回傳 0；否則回傳距離下一個綠燈的秒數
 * @throws std::runtime_error 若找不到對應的時相，則拋出錯誤
 */
    // 遍歷所有時段，使用 views::enumerate 取得索引與對應時間
    for (auto [index, value] : views::enumerate(this->time)) {
        // 計算下一個時段的索引，若已經是最後一個則回到 0 (表示跨日的時間表)
        int nextIndex = (static_cast<size_t>(index) == this->time.size() - 1) ? 0 : (index + 1);

        // 計算下一個時段的起始時間，若為第一個時段則需加上一天的秒數 (86400)
        int nextTime = nextIndex ? this->time[nextIndex] : this->time[nextIndex] + 86400;

        // 判斷當前時間是否落在該時段內
        if (timeStamp >= value && timeStamp <= nextTime) {
            
            
            int startTime = timeStamp - this->offset[index]; // 計算相對於該時段的起始時間
            int cycleTime = this->cycle[index]; // 取得該時段的週期時間 (秒)

            /* 確保 `startTime` 為正數，若為負數則補足一個週期*/
            while (startTime < 0) {
                startTime += cycleTime;
            }

            /* 計算當前時間在該時段的週期內對應的時間點 */
            int target = startTime % cycleTime;

            /* 遍歷該時段的時相區間，取得索引與 pair<int, int> */
            for (auto&& t : views::enumerate(this->phase[index])) {
                auto pairIndex = get<0>(t);  // 取得索引
                auto& pairValue = get<1>(t); // 取得時相範圍 pair<int, int>

                /* 若當前時間落在綠燈區間內，回傳 0 (表示為綠燈) */
                if (target >= pairValue.first && target <= pairValue.second) {
                    return 0;
                } 
                // 若為紅燈，計算距離下一次綠燈的秒數
                else if (static_cast<size_t>(pairIndex) == this->phase[index].size() - 1) {
                    int timeRemain = this->timeRemain(index, target);
                    return timeRemain;
                }
            }
        }
    }

    throw runtime_error("找不到對應的時相\n");
}

int Plan::timeRemain(int index, int target) {
/**
 * @brief 計算距離下一個綠燈開始的剩餘時間
 * 
 * 此函式根據當前號誌時相 (`phase[index]`) 計算距離下一個綠燈的時間 (秒)。
 * 若當前時間 (`target`) 落在紅燈時段，則尋找下一個最近的綠燈開始時間。
 * 
 * @param index 目前時段的索引
 * @param target 當前時間 (以該時段週期計算出的相對時間)
 * @return int 距離下一個綠燈的時間 (秒)
 */
    vector<int> remain; // 用來儲存不同綠燈開始時間的倒數時間
    for (auto& p : this->phase[index]) { // 遍歷當前時段的所有時相區間
        if (target < p.first) {
            // 若當前時間 target 在該時相開始時間之前，計算剩餘時間
            remain.push_back(p.first - target);
        } else {
            // 若 target 已經超過該時相的起始時間，則計算下一次週期的時間
            remain.push_back(this->cycle[index] - target + p.first);
        }
    }
    // 找出最小的剩餘時間，即最近的綠燈開始時間
    sort(remain.begin(), remain.end());
    
    return remain[0];
}