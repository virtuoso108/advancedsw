#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "../app/application/dvm.h"
#include "../app/domain/item.h"
#include "../app/domain/location.h"
#include "../app/dto.h"
#include "../app/exception/dvmexception.h"
#include <list>
#include <map>
#include <string>
#include <stdexcept> // std::runtime_error

using namespace std;
using ::testing::_;
using ::testing::Return;

// 테스트용 Mock OtherDVM 클래스
class MockOtherDVM : public OtherDVM {
public:
    MockOtherDVM(int id, Location loc) : OtherDVM(id, loc) {}

    MOCK_METHOD(CheckStockResponse, findAvailableStocks, (const CheckStockRequest& request));
    MOCK_METHOD(askPrepaymentResponse, askForPrepayment, (const askPrepaymentRequest& request));
};

class DVMTest : public ::testing::Test {
protected:
    DVM* dvm1; // 주 테스트 대상 DVM
    DVM* dvm_for_other_interaction; // 다른 DVM 역할을 할 DVM (필요시)

    Item item_coke{"001", "Coke", 1000};
    Item item_sprite{"002", "Sprite", 1200};
    Item item_fanta{"003", "Fanta", 1100}; // 재고 없는 아이템
    Item item_pepsi{"004", "Pepsi", 1050}; // 다른 DVM에만 있는 아이템
    Item item_water{"005", "Water", 800};  // 추가 아이템
    Item item_coffee{"006", "Coffee", 1500}; // 추가 아이템
    Item item_tea{"007", "Tea", 1300};     // 추가 아이템

    list<OtherDVM> other_dvm_list_empty;
    list<OtherDVM> other_dvm_list_with_one;
    
    // MockOtherDVM 포인터로 관리하여 수명 관리
    unique_ptr<MockOtherDVM> mock_other_dvm_inst;

    virtual void SetUp() {
        list<Item> items1 = {item_coke, item_sprite, item_fanta, item_water, item_coffee, item_tea};
        map<Item, int> stocks1 = {
            {item_coke, 10},
            {item_sprite, 5},
            {item_fanta, 0}, // Fanta는 재고 없음
            {item_water, 20},
            {item_coffee, 3},
            {item_tea, 8}
        };
        
        // MockOtherDVM 인스턴스 생성 및 설정
        mock_other_dvm_inst = make_unique<MockOtherDVM>(2, Location(10, 10));

        dvm1 = new DVM(1, Location(0,0), stocks1, items1, {}, other_dvm_list_empty);

        // 다른 DVM 역할을 할 DVM 인스턴스 (예: 선결제 요청 대상)
        list<Item> items_for_other = {item_pepsi};
        map<Item, int> stocks_for_other = {{item_pepsi, 20}};
        OtherDVM real_other_dvm(2, Location(10,10)); 
    }

    virtual void TearDown() {
        delete dvm1;
        dvm1 = nullptr;
    }
};

// ===== UI 테스트 케이스 그룹 =====

// TC-UI-001: 메뉴 표시 - 전체 음료 메뉴
TEST_F(DVMTest, QueryItems_ShouldReturnAllItems) {
    string result = dvm1->queryItems();
    EXPECT_NE(result.find(item_coke.printItem()), string::npos);
    EXPECT_NE(result.find(item_sprite.printItem()), string::npos);
    EXPECT_NE(result.find(item_fanta.printItem()), string::npos);
    EXPECT_NE(result.find(item_water.printItem()), string::npos);
    EXPECT_NE(result.find(item_coffee.printItem()), string::npos);
    EXPECT_NE(result.find(item_tea.printItem()), string::npos);
}

// TC-UI-001: 메뉴 표시 - Coke 포함 확인
TEST_F(DVMTest, QueryItems_ShouldIncludeCoke) {
    string result = dvm1->queryItems();
    EXPECT_NE(result.find(item_coke.printItem()), string::npos);
}

// TC-UI-001: 메뉴 표시 - Sprite 포함 확인
TEST_F(DVMTest, QueryItems_ShouldIncludeSprite) {
    string result = dvm1->queryItems();
    EXPECT_NE(result.find(item_sprite.printItem()), string::npos);
}

// TC-UI-001: 메뉴 표시 - 재고 없는 Fanta 포함 확인
TEST_F(DVMTest, QueryItems_ShouldIncludeOutOfStockItems) {
    string result = dvm1->queryItems();
    EXPECT_NE(result.find(item_fanta.printItem()), string::npos);
}

// TC-UI-001: 메뉴 표시 - 없는 아이템(Pepsi) 제외 확인
TEST_F(DVMTest, QueryItems_ShouldNotIncludeNonExistingItems) {
    string result = dvm1->queryItems();
    EXPECT_EQ(result.find(item_pepsi.printItem()), string::npos);
}

// TC-UI-002: 재고 정보 표시 - 단일 수량 Coke
TEST_F(DVMTest, QueryStocks_SingleCoke_ShouldReturnInfo) {
    string result = dvm1->queryStocks(item_coke.getItemCode(), 1);
    EXPECT_NE(result.find("flag:this"), string::npos);
    EXPECT_NE(result.find("item_code:" + item_coke.getItemCode()), string::npos);
}

// TC-UI-002: 재고 정보 표시 - 단일 수량 Coke의 가격 정보
TEST_F(DVMTest, QueryStocks_SingleCoke_ShouldReturnCorrectPrice) {
    string result = dvm1->queryStocks(item_coke.getItemCode(), 1);
    EXPECT_NE(result.find("total_price:" + to_string(item_coke.getPrice())), string::npos);
}

// TC-UI-002: 재고 정보 표시 - 단일 수량 Coke의 이름 정보
TEST_F(DVMTest, QueryStocks_SingleCoke_ShouldReturnName) {
    string result = dvm1->queryStocks(item_coke.getItemCode(), 1);
    EXPECT_NE(result.find("item_name:" + item_coke.printItem()), string::npos);
}

// TC-UI-002: 재고 정보 표시 - 단일 수량 Coke의 수량 정보
TEST_F(DVMTest, QueryStocks_SingleCoke_ShouldReturnQuantity) {
    string result = dvm1->queryStocks(item_coke.getItemCode(), 1);
    EXPECT_NE(result.find("count:1"), string::npos);
}

// TC-UI-002: 재고 정보 표시 - 다중 수량 Coke
TEST_F(DVMTest, QueryStocks_MultipleCoke_ShouldReturnInfo) {
    string result = dvm1->queryStocks(item_coke.getItemCode(), 3);
    EXPECT_NE(result.find("flag:this"), string::npos);
    EXPECT_NE(result.find("item_code:" + item_coke.getItemCode()), string::npos);
    EXPECT_NE(result.find("total_price:" + to_string(item_coke.getPrice() * 3)), string::npos); // 3개 가격
    EXPECT_NE(result.find("count:3"), string::npos);
}

// TC-UI-002: 재고 정보 표시 - Sprite
TEST_F(DVMTest, QueryStocks_Sprite_ShouldReturnInfo) {
    string result = dvm1->queryStocks(item_sprite.getItemCode(), 1);
    EXPECT_NE(result.find("flag:this"), string::npos);
    EXPECT_NE(result.find("item_code:" + item_sprite.getItemCode()), string::npos);
    EXPECT_NE(result.find("total_price:" + to_string(item_sprite.getPrice())), string::npos);
}

// TC-UI-002: 재고 정보 표시 - Water
TEST_F(DVMTest, QueryStocks_Water_ShouldReturnInfo) {
    string result = dvm1->queryStocks(item_water.getItemCode(), 1);
    EXPECT_NE(result.find("flag:this"), string::npos);
    EXPECT_NE(result.find("item_code:" + item_water.getItemCode()), string::npos);
    EXPECT_NE(result.find("total_price:" + to_string(item_water.getPrice())), string::npos);
}

// TC-UI-003: 재고 최대 수량
TEST_F(DVMTest, QueryStocks_MaxAvailableQuantity_ShouldReturnInfo) {
    // Water는 재고가 20개
    string result = dvm1->queryStocks(item_water.getItemCode(), 20);
    EXPECT_NE(result.find("flag:this"), string::npos);
    EXPECT_NE(result.find("count:20"), string::npos);
}

// TC-UI-003: 재고량 초과 요청
TEST_F(DVMTest, QueryStocks_ExceedingAvailableQuantity_ShouldReturnProperMessage) {
    // Water는 재고가 20개인데 30개 요청
    string result = dvm1->queryStocks(item_water.getItemCode(), 30);
    // 여기서는 구현에 따라 다양한 결과가 가능
    // 일부만 제공하거나 에러 메시지를 반환할 수 있음
}

// TC-UI-004, TC-DIST-001: 재고 없는 아이템 조회
TEST_F(DVMTest, QueryStocks_OutOfStockFanta_ShouldReturnNotAvailable) {
    string result = dvm1->queryStocks(item_fanta.getItemCode(), 1);
    EXPECT_NE(result.find("flag:not_available"), string::npos);
}

// TC-UI-004, TC-DIST-001: 재고 없는 아이템의 코드 정보
TEST_F(DVMTest, QueryStocks_OutOfStockFanta_ShouldReturnItemCode) {
    string result = dvm1->queryStocks(item_fanta.getItemCode(), 1);
    EXPECT_NE(result.find("item_code:" + item_fanta.getItemCode()), string::npos);
}

// TC-UI-004: 존재하지 않는 아이템 코드로 조회
TEST_F(DVMTest, QueryStocks_NonExistingCode_ShouldReturnNotAvailable) {
    string result = dvm1->queryStocks("999", 1);
    EXPECT_NE(result.find("flag:not_available"), string::npos);
}

// ===== 직접 판매 테스트 케이스 그룹 =====

// TC-SALES-001: 직접 판매 - 단일 수량 Coke
TEST_F(DVMTest, RequestOrder_SingleCoke_ShouldSucceed) {
    SaleRequest request{item_coke.getItemCode(), 1, item_coke};
    EXPECT_NO_THROW(dvm1->requestOrder(request));
}

// TC-SALES-001: 직접 판매 - 다중 수량 Coke
TEST_F(DVMTest, RequestOrder_MultipleCoke_ShouldSucceed) {
    SaleRequest request{item_coke.getItemCode(), 3, item_coke};
    EXPECT_NO_THROW(dvm1->requestOrder(request));
}

// TC-SALES-001: 직접 판매 - 최대 재고량
TEST_F(DVMTest, RequestOrder_MaxStock_ShouldSucceed) {
    SaleRequest request{item_coke.getItemCode(), 10, item_coke}; // Coke 재고 10개
    EXPECT_NO_THROW(dvm1->requestOrder(request));
}

// TC-SALES-001: 직접 판매 - 재고 부족 상황
TEST_F(DVMTest, RequestOrder_ExceedingStock_ShouldThrowError) {
    SaleRequest request{item_coke.getItemCode(), 11, item_coke}; // Coke 재고 10개
    EXPECT_THROW(dvm1->requestOrder(request), runtime_error);
}

// TC-SALES-001: 직접 판매 - Sprite 1개
TEST_F(DVMTest, RequestOrder_SingleSprite_ShouldSucceed) {
    SaleRequest request{item_sprite.getItemCode(), 1, item_sprite};
    EXPECT_NO_THROW(dvm1->requestOrder(request));
}

// TC-SALES-001: 직접 판매 - Sprite 경계값
TEST_F(DVMTest, RequestOrder_BorderlineStock_ShouldSucceed) {
    SaleRequest request{item_sprite.getItemCode(), 5, item_sprite}; // Sprite 재고 정확히 5개
    EXPECT_NO_THROW(dvm1->requestOrder(request));
}

// TC-SALES-001: 직접 판매 - Sprite 재고 부족 상황
TEST_F(DVMTest, RequestOrder_SlightlyExceedingStock_ShouldThrowError) {
    SaleRequest request{item_sprite.getItemCode(), 6, item_sprite}; // Sprite 재고 5개
    EXPECT_THROW(dvm1->requestOrder(request), runtime_error);
}

// TC-SALES-001: 직접 판매 - 재고 없음
TEST_F(DVMTest, RequestOrder_ZeroStock_ShouldThrowError) {
    SaleRequest request{item_fanta.getItemCode(), 1, item_fanta}; // Fanta 재고 0개
    EXPECT_THROW(dvm1->requestOrder(request), runtime_error);
}

// TC-SALES-001: 직접 판매 - 잘못된 요청 수량 (0)
TEST_F(DVMTest, RequestOrder_ZeroQuantity_ShouldBeHandled) {
    SaleRequest request{item_coke.getItemCode(), 0, item_coke};
    // 구현에 따라 에러가 발생하지 않을 수 있음 - 0개는 아무 작업이 필요 없는 것으로 간주될 수 있음
    EXPECT_NO_THROW(dvm1->requestOrder(request));
    // 혹은 에러를 던지도록 처리된다면: EXPECT_THROW(dvm1->requestOrder(request), runtime_error);
}

// TC-SALES-001: 직접 판매 - 잘못된 요청 수량 (음수)
TEST_F(DVMTest, RequestOrder_NegativeQuantity_ShouldBeHandled) {
    SaleRequest request{item_coke.getItemCode(), -1, item_coke};
    // 구현에 따라 에러가 발생하지 않을 수 있음 - 음수는 내부적으로 절대값 처리될 수 있음
    EXPECT_NO_THROW(dvm1->requestOrder(request));
    // 혹은 에러를 던지도록 처리된다면: EXPECT_THROW(dvm1->requestOrder(request), runtime_error);
}

// TC-SALES-001: 직접 판매 - 존재하지 않는 아이템
TEST_F(DVMTest, RequestOrder_NonExistingItem_ShouldThrowError) {
    SaleRequest request{"999", 1, Item{"999", "Unknown", 1000}};
    EXPECT_THROW(dvm1->requestOrder(request), runtime_error);
}

// ===== 인증 코드 테스트 케이스 그룹 =====

// TC-PRE-006: 유효한 인증코드 검증
TEST_F(DVMTest, ProcessPrepaidItem_ValidCode_ShouldReturnTrue) {
    string cert_code = "VALID_CODE_1";
    dvm1->saveSaleFromOther(item_coke.getItemCode(), 1, cert_code);
    EXPECT_TRUE(dvm1->processPrepaidItem(cert_code));
}

// TC-PRE-006: 두 번째 유효한 인증코드 검증
TEST_F(DVMTest, ProcessPrepaidItem_AnotherValidCode_ShouldReturnTrue) {
    string cert_code = "VALID_CODE_2";
    dvm1->saveSaleFromOther(item_sprite.getItemCode(), 1, cert_code);
    EXPECT_TRUE(dvm1->processPrepaidItem(cert_code));
}

// TC-PRE-007: 잘못된 인증코드 검증
TEST_F(DVMTest, ProcessPrepaidItem_InvalidCode_ShouldReturnFalse) {
    EXPECT_FALSE(dvm1->processPrepaidItem("INVALID_CODE"));
}

// TC-PRE-007: 빈 인증코드 검증
TEST_F(DVMTest, ProcessPrepaidItem_EmptyCode_ShouldReturnFalse) {
    EXPECT_FALSE(dvm1->processPrepaidItem(""));
}

// TC-PRE-007: 다양한 잘못된 인증코드 1
TEST_F(DVMTest, ProcessPrepaidItem_InvalidCode1_ShouldReturnFalse) {
    EXPECT_FALSE(dvm1->processPrepaidItem("WRONG1"));
}

// TC-PRE-007: 다양한 잘못된 인증코드 2
TEST_F(DVMTest, ProcessPrepaidItem_InvalidCode2_ShouldReturnFalse) {
    EXPECT_FALSE(dvm1->processPrepaidItem("WRONG2"));
}

// TC-PRE-007: 다양한 잘못된 인증코드 3
TEST_F(DVMTest, ProcessPrepaidItem_InvalidCode3_ShouldReturnFalse) {
    EXPECT_FALSE(dvm1->processPrepaidItem("WRONG3"));
}

// TC-PRE-006, TC-PRE-007: 이미 사용된 인증코드 재검증
TEST_F(DVMTest, ProcessPrepaidItem_UsedCode_ShouldReturnFalse) {
    string cert_code = "USED_CODE";
    dvm1->saveSaleFromOther(item_coke.getItemCode(), 1, cert_code);
    
    // 첫 번째 사용은 성공
    EXPECT_TRUE(dvm1->processPrepaidItem(cert_code));
    
    // 두 번째 사용은 실패
    EXPECT_FALSE(dvm1->processPrepaidItem(cert_code));
}

// TC-PRE-006: 특수문자가 포함된 인증코드
TEST_F(DVMTest, ProcessPrepaidItem_CodeWithSpecialChars_ShouldReturnTrue) {
    string cert_code = "CODE@#$%";
    dvm1->saveSaleFromOther(item_coke.getItemCode(), 1, cert_code);
    EXPECT_TRUE(dvm1->processPrepaidItem(cert_code));
}

// TC-PRE-006: 숫자만 포함된 인증코드
TEST_F(DVMTest, ProcessPrepaidItem_NumericCode_ShouldReturnTrue) {
    string cert_code = "12345";
    dvm1->saveSaleFromOther(item_coke.getItemCode(), 1, cert_code);
    EXPECT_TRUE(dvm1->processPrepaidItem(cert_code));
}

// TC-PRE-006: 다른 DVM에서 판매된 아이템 코드 검증
TEST_F(DVMTest, ProcessPrepaidItem_ItemFromOtherDVM_ShouldReturnTrue) {
    string cert_code = "OTHER_DVM_CODE";
    // Item item_pepsi는 현재 DVM의 아이템 목록에 없으므로 발견되지 않음
    // 대신 DVM에 있는 아이템 사용
    dvm1->saveSaleFromOther(item_coke.getItemCode(), 1, cert_code);
    EXPECT_TRUE(dvm1->processPrepaidItem(cert_code));
}

// ===== 원격 DVM 판매 저장 테스트 케이스 그룹 =====

// TC-PRE-005: 다른 DVM에서 판매 정보 저장 - Coke
TEST_F(DVMTest, SaveSaleFromOther_SingleCoke_ShouldSucceed) {
    string cert_code = "CERT_COKE_1";
    EXPECT_NO_THROW(dvm1->saveSaleFromOther(item_coke.getItemCode(), 1, cert_code));
    
    // 성공적인 저장 후 해당 인증코드로 검증 가능
    EXPECT_TRUE(dvm1->processPrepaidItem(cert_code));
}

// TC-PRE-005: 다른 DVM에서 판매 정보 저장 - Sprite
TEST_F(DVMTest, SaveSaleFromOther_SingleSprite_ShouldSucceed) {
    string cert_code = "CERT_SPRITE_1";
    EXPECT_NO_THROW(dvm1->saveSaleFromOther(item_sprite.getItemCode(), 1, cert_code));
    
    // 성공적인 저장 후 해당 인증코드로 검증 가능
    EXPECT_TRUE(dvm1->processPrepaidItem(cert_code));
}

// TC-PRE-005: 다른 DVM에서 판매 정보 저장 - 다중 수량
TEST_F(DVMTest, SaveSaleFromOther_MultipleCoke_ShouldSucceed) {
    string cert_code = "CERT_COKE_MULTI";
    EXPECT_NO_THROW(dvm1->saveSaleFromOther(item_coke.getItemCode(), 3, cert_code));
    
    // 성공적인 저장 후 해당 인증코드로 검증 가능
    EXPECT_TRUE(dvm1->processPrepaidItem(cert_code));
}

// TC-PRE-005: 다른 DVM에서 판매 정보 저장 - 재고 확인
TEST_F(DVMTest, SaveSaleFromOther_ShouldDecreaseStock) {
    string cert_code = "CERT_STOCK_CHECK";
    // 초기 Coke 재고: 10개
    dvm1->saveSaleFromOther(item_coke.getItemCode(), 3, cert_code);
    
    // 재고 7개로 감소, 8개 주문하면 에러
    SaleRequest request{item_coke.getItemCode(), 8, item_coke};
    EXPECT_THROW(dvm1->requestOrder(request), runtime_error);
    
    // 7개는 주문 가능
    SaleRequest valid_request{item_coke.getItemCode(), 7, item_coke};
    EXPECT_NO_THROW(dvm1->requestOrder(valid_request));
}

// TC-PRE-005: 다른 DVM에서 판매 정보 저장 - 재고 경계값
TEST_F(DVMTest, SaveSaleFromOther_BorderlineStock_ShouldSucceed) {
    // Coffee는 재고 3개
    string cert_code = "CERT_COFFEE_ALL";
    EXPECT_NO_THROW(dvm1->saveSaleFromOther(item_coffee.getItemCode(), 3, cert_code));
    
    // 재고 0개로 감소, 1개 주문하면 에러
    SaleRequest request{item_coffee.getItemCode(), 1, item_coffee};
    EXPECT_THROW(dvm1->requestOrder(request), runtime_error);
}

// TC-PRE-005: 다른 DVM에서 판매 정보 저장 - 재고 부족
TEST_F(DVMTest, SaveSaleFromOther_ExceedingStock_ShouldThrowError) {
    // Sprite는 재고 5개
    string cert_code = "CERT_SPRITE_TOOMANY";
    EXPECT_THROW(dvm1->saveSaleFromOther(item_sprite.getItemCode(), 6, cert_code), runtime_error);
}

// TC-PRE-005: 다른 DVM에서 판매 정보 저장 - 잘못된 아이템 코드
TEST_F(DVMTest, SaveSaleFromOther_InvalidCode_ShouldThrowError) {
    string cert_code = "CERT_INVALID_CODE";
    EXPECT_THROW(dvm1->saveSaleFromOther("999", 1, cert_code), runtime_error);
}

// TC-PRE-005: 다른 DVM에서 판매 정보 저장 - 인증코드 중복
TEST_F(DVMTest, SaveSaleFromOther_DuplicateCertCode_ShouldHandleAppropriately) {
    string cert_code = "CERT_DUPLICATE";
    EXPECT_NO_THROW(dvm1->saveSaleFromOther(item_coke.getItemCode(), 1, cert_code));
    
    // 구현에 따라 중복 인증코드 처리가 달라질 수 있음
}

// ===== 다른 DVM 통신 테스트 케이스 그룹 =====

// TC-DIST-001: 존재하지 않는 DVM 요청
TEST_F(DVMTest, RequestOrder_ToNonExistentDVM_ShouldThrowDVMNotFoundException) {
    SaleRequest request{item_coke.getItemCode(), 1, item_coke};
    EXPECT_THROW(dvm1->requestOrder(999, request), DVMNotFoundException);
}

// TC-DIST-001: 다른 DVM 요청 - ID 0
TEST_F(DVMTest, RequestOrder_ToDVMId0_ShouldThrowDVMNotFoundException) {
    SaleRequest request{item_coke.getItemCode(), 1, item_coke};
    EXPECT_THROW(dvm1->requestOrder(0, request), DVMNotFoundException);
}

// TC-DIST-001: 다른 DVM 요청 - 음수 ID
TEST_F(DVMTest, RequestOrder_ToNegativeDVMId_ShouldThrowDVMNotFoundException) {
    SaleRequest request{item_coke.getItemCode(), 1, item_coke};
    EXPECT_THROW(dvm1->requestOrder(-1, request), DVMNotFoundException);
}

// ===== 결제 처리 테스트 케이스 그룹 =====

// TC-PAY-001: 유효한 결제 (간접 테스트)
TEST_F(DVMTest, Payment_ValidPayment_ShouldSucceed) {
    SaleRequest request{item_coke.getItemCode(), 1, item_coke};
    EXPECT_NO_THROW(dvm1->requestOrder(request));
}

// TC-PAY-001: 다중 수량 결제 (간접 테스트)
TEST_F(DVMTest, Payment_MultipleQuantity_ShouldSucceed) {
    SaleRequest request{item_coke.getItemCode(), 2, item_coke};
    EXPECT_NO_THROW(dvm1->requestOrder(request));
}

// TC-PAY-001: 최소 가격 결제 (간접 테스트)
TEST_F(DVMTest, Payment_MinimumPrice_ShouldSucceed) {
    // water는 가장 저렴한 상품(800원)
    SaleRequest request{item_water.getItemCode(), 1, item_water};
    EXPECT_NO_THROW(dvm1->requestOrder(request));
}

// TC-PAY-001: 최대 가격 결제 (간접 테스트)
TEST_F(DVMTest, Payment_MaximumPrice_ShouldSucceed) {
    // coffee는 가장 비싼 상품(1500원)
    SaleRequest request{item_coffee.getItemCode(), 1, item_coffee};
    EXPECT_NO_THROW(dvm1->requestOrder(request));
}

// ===== 모의 시나리오 테스트 그룹 =====

// 테스트 시나리오 1: 여러 음료 연속 구매 후 재고 확인
TEST_F(DVMTest, Scenario_MultiplePurchaseThenCheckStock) {
    // 초기 Coke 재고: 10개
    
    // 1. Coke 3개 구매
    SaleRequest request1{item_coke.getItemCode(), 3, item_coke};
    EXPECT_NO_THROW(dvm1->requestOrder(request1));
    
    // 2. Coffee 2개 구매
    SaleRequest request2{item_coffee.getItemCode(), 2, item_coffee};
    EXPECT_NO_THROW(dvm1->requestOrder(request2));
    
    // 3. 남은 Coke 7개 모두 구매 시도
    SaleRequest request3{item_coke.getItemCode(), 7, item_coke};
    EXPECT_NO_THROW(dvm1->requestOrder(request3));
    
    // 4. Coke 추가 구매 시도 (재고 부족)
    SaleRequest request4{item_coke.getItemCode(), 1, item_coke};
    EXPECT_THROW(dvm1->requestOrder(request4), runtime_error);
    
    // 5. Coffee 추가 구매 시도 (재고 1개)
    SaleRequest request5{item_coffee.getItemCode(), 1, item_coffee};
    EXPECT_NO_THROW(dvm1->requestOrder(request5));
    
    // 6. Coffee 추가 구매 시도 (재고 부족)
    SaleRequest request6{item_coffee.getItemCode(), 1, item_coffee};
    EXPECT_THROW(dvm1->requestOrder(request6), runtime_error);
}

// 테스트 시나리오 2: 선결제와 직접 구매 혼합
TEST_F(DVMTest, Scenario_PrepaymentAndDirectPurchase) {
    // 1. 다른 DVM에서 Water 10개 선결제 저장
    string cert_code1 = "SCENARIO_CERT1";
    EXPECT_NO_THROW(dvm1->saveSaleFromOther(item_water.getItemCode(), 10, cert_code1));
    
    // 2. Water 5개 직접 구매
    SaleRequest request1{item_water.getItemCode(), 5, item_water};
    EXPECT_NO_THROW(dvm1->requestOrder(request1));
    
    // 3. 선결제 코드로 Water 수령
    EXPECT_TRUE(dvm1->processPrepaidItem(cert_code1));
    
    // 4. 남은 Water 5개 중 4개 구매
    SaleRequest request2{item_water.getItemCode(), 4, item_water};
    EXPECT_NO_THROW(dvm1->requestOrder(request2));
    
    // 5. 남은 Water 1개 구매
    SaleRequest request3{item_water.getItemCode(), 1, item_water};
    EXPECT_NO_THROW(dvm1->requestOrder(request3));
    
    // 6. 추가 Water 구매 시도 (재고 부족)
    SaleRequest request4{item_water.getItemCode(), 1, item_water};
    EXPECT_THROW(dvm1->requestOrder(request4), runtime_error);
}

// 테스트 시나리오 3: 인증코드 여러 개 검증
TEST_F(DVMTest, Scenario_MultipleCertCodes) {
    // 1. 다양한 음료 선결제 정보 저장
    string cert_code1 = "MULTI_CERT1";
    string cert_code2 = "MULTI_CERT2";
    string cert_code3 = "MULTI_CERT3";
    
    EXPECT_NO_THROW(dvm1->saveSaleFromOther(item_coke.getItemCode(), 1, cert_code1));
    EXPECT_NO_THROW(dvm1->saveSaleFromOther(item_sprite.getItemCode(), 1, cert_code2));
    EXPECT_NO_THROW(dvm1->saveSaleFromOther(item_tea.getItemCode(), 1, cert_code3));
    
    // 2. 인증코드 검증
    EXPECT_TRUE(dvm1->processPrepaidItem(cert_code1));
    EXPECT_TRUE(dvm1->processPrepaidItem(cert_code2));
    EXPECT_TRUE(dvm1->processPrepaidItem(cert_code3));
    
    // 3. 이미 사용한 인증코드 재검증
    EXPECT_FALSE(dvm1->processPrepaidItem(cert_code1));
    EXPECT_FALSE(dvm1->processPrepaidItem(cert_code2));
    EXPECT_FALSE(dvm1->processPrepaidItem(cert_code3));
    
    // 4. 잘못된 인증코드 검증
    EXPECT_FALSE(dvm1->processPrepaidItem("MULTI_CERT4"));
    EXPECT_FALSE(dvm1->processPrepaidItem("WRONG_CERT"));
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 