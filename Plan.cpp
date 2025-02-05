#include "Plan.hpp"
#include <regex>
#include <string>

Plan::Plan() {};

void Plan::setPhase(string config) {
    vector<string> segments;
    regex pattern(R"(/[^/]+/[^/]+/[^/]+/[^/]+,[^/]+/)");  // 正則表達式匹配 /.../.../.../.../

    sregex_iterator it(config.begin(), config.end(), pattern);
    sregex_iterator end;

    while (it != end) {
        segments.push_back(it->str());
        it++;
    }

    for (const auto& seg : segments) {
        cout << seg << endl;
        processSegment(seg);
    }
}

void Plan::processSegment(string segment) {
    vector<string> parts;
    string temp;
    stringstream ss(segment.substr(1, segment.size() - 2));  // 去掉開頭和結尾的 `/`

    while (getline(ss, temp, '/')) {  
        parts.push_back(temp);
    }

    if (parts.size() < 4) {
        throw runtime_error("時制資料格式錯誤，應該至少有 4 個部分\n");
    }

    this->time.push_back(time2Seconds(parts[0])); // 解析時間（轉換為秒）
    this->cycle.push_back(stoi(parts[1]));
    if (!this->offset.size()) {
        this->offset.push_back(stoi(parts[2]));
    } else {
        int cumulativeOffset = (this->offset[this->offset.size() - 1] +  stoi(parts[2])) % stoi(parts[1]);
        this->offset.push_back(cumulativeOffset);
    }
    
    // 解析 pair 部分
    if (parts.size() > 3) {
        stringstream pairStream(parts[3]);
        string pairItem;
        vector<pair<int, int>> tmp;
        while (getline(pairStream, pairItem, ',')) {
            int value = stoi(pairItem);
            if (tmp.empty()) {
                tmp.emplace_back(value, 0);
            } else {
                if (!tmp.back().second) {
                    if (value <= tmp.back().first)  
                        throw runtime_error("時相區間的結束時間必須大於起始時間\n");
                    tmp.back().second = value;
                } else {
                    tmp.emplace_back(value, 0);
                }  
                
            }
        }
        if (tmp.back().second == 0) 
            throw runtime_error("時相區間對輸入必須成對\n");
        getline(pairStream, pairItem);
        tmp.back().second = stoi(pairItem);

        for (auto& ele : tmp) {
            cout << "(" << ele.first << ", " << ele.second <<  ")\n";
        }
        this->phase.push_back(tmp);
    }

}

int Plan::time2Seconds(const string& timeStr) {
    int hours = stoi(timeStr.substr(0, 2));
    int minutes = stoi(timeStr.substr(2, 2));
    return hours * 3600 + minutes * 60;
}