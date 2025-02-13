#ifndef SYSTEM_HPP
#define SYSTEM_HPP

#include "Event.hpp"
#include "Bus.hpp"
#include "Plan.hpp"
#include<bits/stdc++.h>

using namespace std;

/* Signal state */
enum TrafficLight { RED, YELLOW, GREEN }; // 紅燈, 黃燈, 綠燈

/* Data Structure of Signal */
struct Light {
    int id; //編號
    int mileage = 0; //位置 (里程)
    string lightName; //號誌化路口名稱
    TrafficLight state = GREEN; //燈號
    int cycleTime; // 週期
    //vector<pair<int, TrafficLight>> plan; // 時制計畫
    int offset; // 和前一號誌的起始時間偏差
    Plan plan;
};

/* Data Structure of Stop */
struct Stop {
    int id; // 編號
    bool direction; // 方向
    string stopName; // 站點名稱
    int mileage; // 位置 (里程)
    int pax = 0; // 站上乘客數
    string note; // 站點備註
    int lastArrive = -1; // 上輛車抵達的時間
    array<pair<double, double>, 3> arrivalRate;
    array<pair<double, double>, 3> dropRate;
};

/* Comparator */
struct eventCmp {
    /* 為了使 Event 物件在 Event List (priority queue) 中能按照發生先後順序排列而設計之比較器 */
    bool operator()(Event* a, Event* b) { return a->getTime() > b->getTime(); }
};

struct mileageCmp {
     /* 為了使 Stop 及 Light 物件在 Route (set) 中能按照里程順序排列而設計之比較器 */
    bool operator()(const variant<Stop*, Light*>& a, const variant<Stop*, Light*>& b) const {
        return visit([](const auto* obj1, const auto* obj2) {
            return obj1->mileage < obj2->mileage; 
        }, a, b);
    }
};

class System {
    public:
        /* Constructor */
        System(); 

        /* Simulation */
        void init(); // 初始化函數
        void simulation(); // 模擬函數
        void performance(); // 計算績效函數
        void readSche(int trial); // 讀取班表函數

        /*Func*/
        void incrHeadwayDev(float dev); // 部分績效計算
        optional<Stop*> getNextStop(int stopID); // 取得下一站點函數

        /*getter*/
        const int getTmax(); // 取得最大置站時間

    private:
        /*Paramemter*/
        string routeName; // 路線名稱
        int Tmax = 3 * 60; // 最大置站時間 (秒)
        //double Vavg = 25 / 3.6; // 平均速度
        //double Vsd =  7 / 3.6;
        //double Vlimit = 40 / 3.6; // 速限
        //double Vlow = 15 / 3.6; // 最低容許速度
        int stopAmount;
        optional<double> stopDistAvg;
        optional<double> stopDistSd;
        optional<double> signalDistAvg;
        optional<double> signalDistSd;
        optional<int> scheStart;
        optional<int> shift;
        optional<double> scheAvg;
        optional<double> scheSd;
        pair<int, int> morningPeak;
        pair<int, int> eveningPeak;
        optional<double> Vavg;
        optional<double> Vsd;
        optional<double> Vlimit;
        optional<double> Vlow;
        

        /*Variable*/
        float headwayDev = 0; // 績效值: headeay deviation

        /*Data Structures*/
        vector<Bus*> fleet; // 車隊
        priority_queue<Event*, vector<Event*>, eventCmp> eventList; // 事件列表
        set<variant<Stop*, Light*>, mileageCmp> route; // 路線 (號誌 + 站點)
        map<int, function<void(Event*)>> eventSet; // 事件種類集合
        vector<vector<float>> getOn; // 乘客到達率
        vector<vector<float>> getOff; // 乘客下車率
        vector<int> sche; // 班表

        /*Function*/
        optional<Stop*> findNextStop(int stopID); // 取得下一站點函數
        optional<variant<Stop*, Light*>> findNext(variant<Stop*, Light*> target); // 取得路線上下一物件
        void printFormattedTime(int time); // 顯示時間函數
        void printEventDetails(Event* e);
        void showRoute(); // 印出路線上的元素
        void setupStop(double avg, double sd);
        void setupSignal(double avg, double sd);
        void setupSche(int start, double avg, double sd, int shift);
        int time2Seconds(const string& timeStr); 
        pair<int, int> timeRange2Pair(const string& timeRange);
        void displayRoute();
        double getArrivalRate(int time, Stop* stop);
        double getDropRate(int time, Stop* stop);
        Bus* findPrevBus(Bus* target);
        Bus* findBus(int id);
        Stop* findStop(int id);
        Light* findSignal(int id);
        int handlingPax(Bus* bus, Stop* stop, int time, double drop);
        void sortedFleet();
        void eventPerformance(Event* e, Stop* stop, Bus* bus);  

        /* Event */
        void arriveAtStop(Event* e); // 抵達站點事件
        void deptFromStop(Event* e); // 離開站點事件
        void arriveAtLight(Event* e); // 抵達號誌化路口事件
        void deptFromLight(Event* e); // 離開號誌化路口事件

        
};

#endif