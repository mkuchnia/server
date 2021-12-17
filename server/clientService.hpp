#ifndef clientService_hpp
#define clientService_hpp

#include <atomic>
#include <map>
#include <mutex>

using namespace std;

extern const long int serverStartTime;
extern atomic<int> queryNumber;
extern atomic<int> connectionNumber;
extern map<long long, int> incrementMap;
extern mutex mtxIncrementMap;

void client_service(int socketDescriptor);

#endif