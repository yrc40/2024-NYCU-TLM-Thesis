#ifndef EVENT_HPP
#define EVENT_HPP

class Event {
    public:
        /* Constructor */
        Event(int time, int busID, int eventType, int oneOfID, bool direction); 
        // 給定發生時間, 車輛編號, 事件種類代碼, 號誌或站點 id, 方向

        /* Getter */
        const int getEventType(); // 取得事件種類代碼
        const int getTime(); // 取得發生時間
        const int getBusID(); // 取得公車編號
        const int getStopID(); // 取得站點編號
        const bool getDirection(); // 取得方向
        const int getLightID(); // 取得號誌編號

    private:
        int time; // 發生時間
        int busID; // 公車編號
        int eventType; // 事件種類代碼
        int stopID; // 站點編號
        int lightID; // 號誌編號
        bool direction; // 方向

};

#endif