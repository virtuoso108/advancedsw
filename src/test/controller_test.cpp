#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../app/presentation/controller.h"
#include "../app/application/dvm.h"
#include "../app/domain/item.h"
#include "../app/domain/location.h"
#include "../app/dto.h"
#include <string>
#include <map>
#include <sstream>

using namespace std;
using ::testing::Return;
using ::testing::_;
using ::testing::AtLeast;
using ::testing::DoAll;
using ::testing::SetArgPointee;

// Controller 테스트를 위한 Mock DVM 클래스
class MockDVM : public DVM {
public:
    MockDVM(int id, Location loc, map<Item, int> stocks, list<Item> itemList) 
        : DVM(id, loc, stocks, itemList, {}, {}) {}
    
    MOCK_METHOD(string, queryItems, ());
    MOCK_METHOD(string, queryStocks, (string itemCode, int count));
    MOCK_METHOD(void, requestOrder, (SaleRequest request));
    MOCK_METHOD((pair<Location, string>), requestOrder, (int dvmId, SaleRequest request));
    MOCK_METHOD(bool, processPrepaidItem, (string certCode));
};

// Controller 클래스의 private 멤버에 접근하기 위한 래퍼 클래스
class TestableController : public Controller {
public:
    TestableController(DVM* dvm) : Controller(dvm) {
        dvmId = 1;
        location = Location(10, 20);
    }
    
    map<string, string> testParseStockResponse(const string &response) {
        return parseStockResponse(response);
    }
    
    string testHandleCheckStockRequest(const string &msg) {
        return handleCheckStockRequest(msg);
    }
    
    string testHandlePrepaymentRequest(const string &msg) {
        return handlePrepaymentRequest(msg);
    }
    
    using Controller::dvmId;
    using Controller::location;
};

class ControllerTest : public ::testing::Test {
protected:
    MockDVM* mockDvm;
    TestableController* controller;
    
    Item item_coke{"001", "Coke", 1000};
    Item item_sprite{"002", "Sprite", 1200};
    Item item_fanta{"003", "Fanta", 1100};
    
    virtual void SetUp() override {
        map<Item, int> stocks = {
            {item_coke, 10},
            {item_sprite, 5},
            {item_fanta, 0}
        };
        
        list<Item> items = {item_coke, item_sprite, item_fanta};
        
        mockDvm = new MockDVM(1, Location(10, 20), stocks, items);
        controller = new TestableController(mockDvm);
    }
    
    virtual void TearDown() override {
        delete controller;
        delete mockDvm;
    }
};

// ParseStockResponse 테스트 케이스들

TEST_F(ControllerTest, ParseStockResponse_ShouldParseFlag_WhenThisFlag) {
    string response = "flag:this;item_code:001;";
    auto result = controller->testParseStockResponse(response);
    EXPECT_EQ(result["flag"], "this");
}

TEST_F(ControllerTest, ParseStockResponse_ShouldParseItemCode_WhenThisFlag) {
    string response = "flag:this;item_code:001;";
    auto result = controller->testParseStockResponse(response);
    EXPECT_EQ(result["item_code"], "001");
}

TEST_F(ControllerTest, ParseStockResponse_ShouldParseItemName_WhenThisFlag) {
    string response = "flag:this;item_name:Coke;";
    auto result = controller->testParseStockResponse(response);
    EXPECT_EQ(result["item_name"], "Coke");
}

TEST_F(ControllerTest, ParseStockResponse_ShouldParseCount_WhenThisFlag) {
    string response = "flag:this;count:2;";
    auto result = controller->testParseStockResponse(response);
    EXPECT_EQ(result["count"], "2");
}

TEST_F(ControllerTest, ParseStockResponse_ShouldParseTotalPrice_WhenThisFlag) {
    string response = "flag:this;total_price:2000;";
    auto result = controller->testParseStockResponse(response);
    EXPECT_EQ(result["total_price"], "2000");
}

TEST_F(ControllerTest, ParseStockResponse_ShouldParseFlag_WhenOtherFlag) {
    string response = "flag:other;target:2;";
    auto result = controller->testParseStockResponse(response);
    EXPECT_EQ(result["flag"], "other");
}

TEST_F(ControllerTest, ParseStockResponse_ShouldParseTarget_WhenOtherFlag) {
    string response = "flag:other;target:2;";
    auto result = controller->testParseStockResponse(response);
    EXPECT_EQ(result["target"], "2");
}

TEST_F(ControllerTest, ParseStockResponse_ShouldParseCoordinates_WhenOtherFlag) {
    string response = "flag:other;x:10;y:20;";
    auto result = controller->testParseStockResponse(response);
    EXPECT_EQ(result["x"], "10");
    EXPECT_EQ(result["y"], "20");
}

TEST_F(ControllerTest, ParseStockResponse_ShouldParseFlag_WhenNotAvailable) {
    string response = "flag:not_available;item_code:003;";
    auto result = controller->testParseStockResponse(response);
    EXPECT_EQ(result["flag"], "not_available");
}

TEST_F(ControllerTest, ParseStockResponse_ShouldReturnEmpty_WhenEmptyInput) {
    string response = "";
    auto result = controller->testParseStockResponse(response);
    EXPECT_TRUE(result.empty());
}

TEST_F(ControllerTest, ParseStockResponse_ShouldReturnEmpty_WhenOnlySeparators) {
    string response = ";;;";
    auto result = controller->testParseStockResponse(response);
    EXPECT_TRUE(result.empty());
}

TEST_F(ControllerTest, ParseStockResponse_ShouldHandleInvalidFormat) {
    string response = "flag=this;item_code=001;";
    auto result = controller->testParseStockResponse(response);
    EXPECT_TRUE(result.empty());
}

TEST_F(ControllerTest, ParseStockResponse_ShouldHandleMultipleColons) {
    string response = "flag:this:extra;item_code:001;";
    auto result = controller->testParseStockResponse(response);
    EXPECT_EQ(result["flag"], "this:extra");
}

TEST_F(ControllerTest, ParseStockResponse_ShouldPreserveSpaces) {
    string response = "flag: this ;item_code: 001 ;";
    auto result = controller->testParseStockResponse(response);
    EXPECT_EQ(result["flag"], " this ");
    EXPECT_EQ(result["item_code"], " 001 ");
}

TEST_F(ControllerTest, ParseStockResponse_ShouldHandleEmptyValues) {
    string response = "flag:;item_code:;empty:;";
    auto result = controller->testParseStockResponse(response);
    EXPECT_EQ(result["flag"], "");
    EXPECT_EQ(result["item_code"], "");
    EXPECT_EQ(result["empty"], "");
}

TEST_F(ControllerTest, ParseStockResponse_ShouldHandleSpecialCharacters) {
    string response = "flag:this;item_name:Special!@#$%^&*();";
    auto result = controller->testParseStockResponse(response);
    EXPECT_EQ(result["item_name"], "Special!@#$%^&*()");
}

TEST_F(ControllerTest, ParseStockResponse_ShouldHandleKoreanCharacters) {
    string response = "flag:this;item_name:콜라;";
    auto result = controller->testParseStockResponse(response);
    EXPECT_EQ(result["item_name"], "콜라");
}

TEST_F(ControllerTest, ParseStockResponse_ShouldHandleDuplicateKeys) {
    string response = "flag:this;flag:other;";
    auto result = controller->testParseStockResponse(response);
    EXPECT_EQ(result["flag"], "other");
}

// HandleCheckStockRequest 테스트 케이스들

TEST_F(ControllerTest, HandleCheckStockRequest_ShouldIncludeMessageType) {
    string request = "msg_type:req_stock;item_code:001;item_num:2;src_id:2;";
    string response = controller->testHandleCheckStockRequest(request);
    EXPECT_NE(response.find("msg_type:resp_stock"), string::npos);
}

TEST_F(ControllerTest, HandleCheckStockRequest_ShouldIncludeSourceId) {
    string request = "msg_type:req_stock;item_code:001;item_num:2;src_id:2;";
    string response = controller->testHandleCheckStockRequest(request);
    EXPECT_NE(response.find("src_id:1"), string::npos);
}

TEST_F(ControllerTest, HandleCheckStockRequest_ShouldIncludeDestinationId) {
    string request = "msg_type:req_stock;item_code:001;item_num:2;src_id:2;";
    string response = controller->testHandleCheckStockRequest(request);
    EXPECT_NE(response.find("dst_id:2"), string::npos);
}

TEST_F(ControllerTest, HandleCheckStockRequest_ShouldIncludeItemCode) {
    string request = "msg_type:req_stock;item_code:001;item_num:2;src_id:2;";
    string response = controller->testHandleCheckStockRequest(request);
    EXPECT_NE(response.find("item_code:001"), string::npos);
}

TEST_F(ControllerTest, HandleCheckStockRequest_ShouldIncludeCoordinates) {
    string request = "msg_type:req_stock;item_code:001;item_num:2;src_id:2;";
    string response = controller->testHandleCheckStockRequest(request);
    EXPECT_NE(response.find("coor_x:10"), string::npos);
    EXPECT_NE(response.find("coor_y:20"), string::npos);
}

TEST_F(ControllerTest, HandleCheckStockRequest_ShouldHandleNoStock) {
    string request = "msg_type:req_stock;item_code:003;item_num:1;src_id:2;";
    string response = controller->testHandleCheckStockRequest(request);
    EXPECT_NE(response.find("item_code:003"), string::npos);
}

TEST_F(ControllerTest, HandleCheckStockRequest_ShouldHandleInvalidFormat) {
    string request = "wrong_format";
    string response = controller->testHandleCheckStockRequest(request);
    EXPECT_NE(response.find("msg_type:resp_stock"), string::npos);
}

TEST_F(ControllerTest, HandleCheckStockRequest_ShouldHandleMissingFields) {
    string request = "msg_type:req_stock;";
    string response = controller->testHandleCheckStockRequest(request);
    EXPECT_NE(response.find("msg_type:resp_stock"), string::npos);
}

// HandlePrepaymentRequest 테스트 케이스들

TEST_F(ControllerTest, HandlePrepaymentRequest_ShouldIncludeMessageType) {
    string request = "msg_type:req_prepay;item_code:001;item_num:2;cert_code:ABC12;src_id:2;";
    string response = controller->testHandlePrepaymentRequest(request);
    EXPECT_NE(response.find("msg_type:resp_prepay"), string::npos);
}

TEST_F(ControllerTest, HandlePrepaymentRequest_ShouldIncludeItemCode) {
    string request = "msg_type:req_prepay;item_code:001;item_num:2;cert_code:ABC12;src_id:2;";
    string response = controller->testHandlePrepaymentRequest(request);
    EXPECT_NE(response.find("item_code:001"), string::npos);
}

TEST_F(ControllerTest, HandlePrepaymentRequest_ShouldIncludeItemNumber) {
    string request = "msg_type:req_prepay;item_code:001;item_num:2;cert_code:ABC12;src_id:2;";
    string response = controller->testHandlePrepaymentRequest(request);
    EXPECT_NE(response.find("item_num:2"), string::npos);
}

TEST_F(ControllerTest, HandlePrepaymentRequest_ShouldHandleNoStock) {
    string request = "msg_type:req_prepay;item_code:003;item_num:1;cert_code:ABC12;src_id:2;";
    string response = controller->testHandlePrepaymentRequest(request);
    EXPECT_NE(response.find("availability:F"), string::npos);
}

TEST_F(ControllerTest, HandlePrepaymentRequest_ShouldHandleInvalidFormat) {
    string request = "wrong_format";
    string response = controller->testHandlePrepaymentRequest(request);
    EXPECT_NE(response.find("msg_type:resp_prepay"), string::npos);
    EXPECT_NE(response.find("availability:F"), string::npos);
}

TEST_F(ControllerTest, HandlePrepaymentRequest_ShouldHandleMissingFields) {
    string request = "msg_type:req_prepay;";
    string response = controller->testHandlePrepaymentRequest(request);
    EXPECT_NE(response.find("msg_type:resp_prepay"), string::npos);
    EXPECT_NE(response.find("availability:F"), string::npos);
}

// 경계값 테스트 케이스들

TEST_F(ControllerTest, ParseStockResponse_ShouldHandleLongValues) {
    string longValue(1000, 'a');  // 1000자 길이의 문자열
    string response = "flag:this;item_name:" + longValue + ";";
    auto result = controller->testParseStockResponse(response);
    EXPECT_EQ(result["item_name"], longValue);
}

TEST_F(ControllerTest, ParseStockResponse_ShouldHandleNumericBoundaries) {
    string response = "count:-1;total_price:2147483647;";  // INT_MAX 값 사용
    auto result = controller->testParseStockResponse(response);
    EXPECT_EQ(result["count"], "-1");
    EXPECT_EQ(result["total_price"], "2147483647");
}

TEST_F(ControllerTest, HandleCheckStockRequest_ShouldHandleBoundaryQuantities) {
    vector<string> quantities = {"0", "-1", "999999"};
    for (const auto& qty : quantities) {
        string request = "msg_type:req_stock;item_code:001;item_num:" + qty + ";src_id:2;";
        string response = controller->testHandleCheckStockRequest(request);
        EXPECT_NE(response.find("msg_type:resp_stock"), string::npos);
    }
}

TEST_F(ControllerTest, HandlePrepaymentRequest_ShouldHandleBoundaryQuantities) {
    vector<string> quantities = {"0", "-1", "999999"};
    for (const auto& qty : quantities) {
        string request = "msg_type:req_prepay;item_code:001;item_num:" + qty + ";cert_code:ABC12;src_id:2;";
        string response = controller->testHandlePrepaymentRequest(request);
        EXPECT_NE(response.find("msg_type:resp_prepay"), string::npos);
    }
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
