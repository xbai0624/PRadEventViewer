#ifndef PRAD_HV_SYSTEM_H
#define PRAD_HV_SYSTEM_H

#include "CAENHVWrapper.h"
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include "PRadException.h"
#include "datastruct.h"

class PRadEventViewer;

class PRadHVSystem
{
public:
    class CAEN_Board
    {
    public:
        std::string model;
        std::string desc;
        unsigned short slot;
        unsigned short nChan;
        unsigned short serNum;
        unsigned char fmwLSB;
        unsigned char fmwMSB;

        // constructor
        CAEN_Board() {};
        CAEN_Board(std::string m, std::string d, unsigned short s, unsigned short n,
                   unsigned short ser, unsigned char lsb, unsigned char msb)
        : model(m), desc(d), slot(s), nChan(n), serNum(ser), fmwLSB(lsb), fmwMSB(msb)
        {};
        CAEN_Board(char* m, char* d, unsigned short s, unsigned short n,
                   unsigned short ser, unsigned char lsb, unsigned char msb)
        : model(m), desc(d), slot(s), nChan(n), serNum(ser), fmwLSB(lsb), fmwMSB(msb)
        {};
     };

    class CAEN_Crate
    {
    public:
        unsigned char id;
        std::string name;
        std::string ip;
        CAENHV::CAENHV_SYSTEM_TYPE_t sysType;
        int linkType;
        std::string username;
        std::string password;
        int handle;
        bool mapped;
        std::vector<CAEN_Board> boardList;

        // constructor
        CAEN_Crate() {};
        CAEN_Crate(unsigned char i, std::string n, std::string p,
                   CAENHV:: CAENHV_SYSTEM_TYPE_t type, int link,
                   std::string user, std::string pwd)
        : id(i), name(n), ip(p), sysType(type), linkType(link),
          username(user), password(pwd), handle(-1), mapped(false) {};
//        void Initialize() throw(PRadException);
//        void DeInitialize() throw(PRadException);
        void clear() {handle = -1; mapped = false; boardList.clear();};
    };

    enum ShowErrorType
    {
        ShowError,
        ShowAll,
    };

    PRadHVSystem(PRadEventViewer *p);
    virtual ~PRadHVSystem();
    void AddCrate(const std::string &name,
                  const std::string &ip,
                  const unsigned char &id,
                  const CAENHV::CAENHV_SYSTEM_TYPE_t &type = CAENHV::SY1527,
                  const int &linkType = LINKTYPE_TCPIP,
                  const std::string &username = "admin",
                  const std::string &password = "admin");
    void Initialize();
    void DeInitialize();
    void Connect();
    void Disconnect();
    void StartMonitor();
    void StopMonitor();
    void SetPowerOn(bool &val);
    void SetPowerOn(ChannelAddress &config, bool &val);
    void SetVoltage(const char *name, ChannelAddress &config, float &val);
    void ReadVoltage();
    void PrintOut();

private:
    PRadEventViewer *console;
    std::vector<CAEN_Crate> crateList;
    volatile bool alive;
    std::thread queryThread;
    std::mutex locker;
    void getCrateMap(CAEN_Crate &crate);
    void heartBeat();
    void queryLoop();
    void checkStatus();
    void checkVoltage(const CAENHVData &hvData);
    void showError(const std::string &prefix, const int &err, ShowErrorType type = ShowError);
    void showChError(const char *n, const unsigned int &ebit);
    float getLimit(const char *name = "");
};

#endif