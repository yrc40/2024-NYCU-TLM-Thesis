#ifndef BUS_HPP
#define BUS_HPP

#include "Event.hpp"
#include <utility>

using namespace std;

class Bus {
    public:
        /* Constructor */
        Bus(int id, int headway); // 給定 id 及 headway

        /* Getter */
        int getId(); // 取得公車 id
        double getVol(); // 取得公車當前速度
        int getPax(); // 取得公車當前乘客數
        int getLocation(); // 取得公車當前位置 (里程)
        int getDwell(); // 取得公車之設計置站時間
        int getStopDwell();
        int getLastGo(); // 取得公車上一次駛離站點或號誌的時間
        const int getCapacity(); // 取得公車容量
        const int getHeadway(); // 取得公車與上一班車的發車間距
        double getNextVol();
        double getArrivalRate();
        double getDropRate();

        /* Setter */
        void setVol(double v); // 設定速度
        void setPax(int p); // 設定乘客數
        void setLocation(int l); // 設定位置 (里程)
        void setDwell(int d); // 設定置站時間
        void setLastGo(int t); // 設定上一次駛離站點或號誌的時間
        void setNextVol(double v);
        void setStopDwell(int d);
        void setArrivalRate(double a);
        void setDropRate(double d);

        pair<int, bool> bunching = {0, 0}; // 連班記錄變數 { 第幾站, 連班與否 }

    private:
        int id; // 編號
        const int capacity = 60; // 容量
        double vol = 0; // 速度
        double nextVol = 0;
        int pax = 0; // 車上乘客
        int location = 0; // 位置
        int dwell = 0; // 累積置站時間
        int stopDwell = 0;
        int headway; // 發車間距
        int lastGo = 0; // 上一次駛離站點或號誌的時間
        double arrivalRate = 0;
        double dropRate = 0;
};

#endif