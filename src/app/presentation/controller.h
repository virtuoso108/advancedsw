#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <map>
#include "../domain/location.h"
#include "../domain/item.h"
#include "../application/dvm.h"
#include <string>
#include <iostream>
#include <regex>

using namespace std;

// TestableController 전방 선언
class TestableController;

class Controller {
private:
    Location location;
    std::map<Item, int> stocks;
    DVM* dvm;
    
    //추가
    int dvmId;
    map<string, string> parseStockResponse(const string &response);
    string handleCheckStockRequest(const string &msg);
    string handlePrepaymentRequest(const string &msg);
public:
    Controller(DVM* dvm);
    ~Controller();
    void run();
    int displayMenu();
    void handleMenuSelection(int choice);
    void handleBeverageSelection();
    void handlePrepaidPurchase();

    //추가
    void runServer();
    
    // TestableController가 Controller의 private 멤버에 접근할 수 있도록 설정
    friend class TestableController;
};
#endif