#ifndef clientService_hpp
#define clientService_hpp

#include <atomic>

extern const long int serverStartTime;
extern std::atomic<int> queryNumber;

void client_service(int socketDescriptor);

#endif