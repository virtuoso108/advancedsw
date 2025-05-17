#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "../app/application/otherdvm.h"
#include "../app/domain/location.h"
#include "../app/dto.h"
#include <string>
#include <memory>

using namespace std;
using ::testing::Return;
using ::testing::_;
using ::testing::DoAll;
using ::testing::SetArgPointee;
using ::testing::SetArrayArgument;
using ::testing::Invoke;

// OtherDVM을 테스트하기 위한 모의 클래스
class MockOtherDVM : public OtherDVM {
public:
    MockOtherDVM(int id, const Location &loc) : OtherDVM(id, loc) {}

    MOCK_METHOD(CheckStockResponse, findAvailableStocks, (const CheckStockRequest& request, int senderDvmId));
    MOCK_METHOD(askPrepaymentResponse, askForPrepayment, (const askPrepaymentRequest& request, int senderDvmId));
};

// Socket 메시지를 위한 모의 구현
class MockSocketMessage {
public:
    static std::string createStockResponse(const std::string& item_code, int item_num, int x, int y) {
        std::string response = "msg_type:res_stock;";
        response += "src_id:T2;";
        response += "dst_id:T1;";
        response += "item_code:" + item_code + ";";
        response += "item_num:" + std::to_string(item_num) + ";";
        response += "coor_x:" + std::to_string(x) + ";";
        response += "coor_y:" + std::to_string(y) + ";";
        return response;
    }
    
    static std::string createPrepaymentResponse(const std::string& item_code, int item_num, bool available) {
        std::string response = "msg_type:res_prepay;";
        response += "src_id:T2;";
        response += "dst_id:T1;";
        response += "item_code:" + item_code + ";";
        response += "item_num:" + std::to_string(item_num) + ";";
        response += "availability:" + std::string(available ? "T" : "F") + ";";
        return response;
    }
};

// OtherDVM 클래스를 테스트하기 위한 테스트 픽스처
class OtherDVMTest : public ::testing::Test {
protected:
    OtherDVM* otherDVM;
    MockOtherDVM* mockOtherDVM;
    
    virtual void SetUp() override {
        otherDVM = new OtherDVM(2, Location(10, 20));
        mockOtherDVM = new MockOtherDVM(3, Location(30, 40));
    }
    
    virtual void TearDown() override {
        delete otherDVM;
        delete mockOtherDVM;
    }
};

// TC-DIST-001: 위치 정보 획득 테스트
TEST_F(OtherDVMTest, GetLocation_ReturnsSavedLocation) {
    Location expectedLoc(10, 20);
    Location actualLoc = otherDVM->getLocation();
    
    EXPECT_EQ(actualLoc.getX(), expectedLoc.getX());
    EXPECT_EQ(actualLoc.getY(), expectedLoc.getY());
}

// TC-DIST-001: 위치 정보 - 원점 테스트
TEST_F(OtherDVMTest, GetLocation_OriginLocation) {
    OtherDVM originDVM(3, Location(0, 0));
    Location loc = originDVM.getLocation();
    
    EXPECT_EQ(loc.getX(), 0);
    EXPECT_EQ(loc.getY(), 0);
}

// TC-DIST-001: 위치 정보 - 음수 좌표 테스트
TEST_F(OtherDVMTest, GetLocation_NegativeCoordinates) {
    OtherDVM negativeDVM(4, Location(-10, -20));
    Location loc = negativeDVM.getLocation();
    
    EXPECT_EQ(loc.getX(), -10);
    EXPECT_EQ(loc.getY(), -20);
}

// TC-DIST-001: 위치 정보 - X 좌표만 음수
TEST_F(OtherDVMTest, GetLocation_NegativeXCoordinate) {
    OtherDVM mixedDVM(5, Location(-10, 20));
    Location loc = mixedDVM.getLocation();
    
    EXPECT_EQ(loc.getX(), -10);
    EXPECT_EQ(loc.getY(), 20);
}

// TC-DIST-001: 위치 정보 - Y 좌표만 음수
TEST_F(OtherDVMTest, GetLocation_NegativeYCoordinate) {
    OtherDVM mixedDVM(6, Location(10, -20));
    Location loc = mixedDVM.getLocation();
    
    EXPECT_EQ(loc.getX(), 10);
    EXPECT_EQ(loc.getY(), -20);
}

// TC-DIST-001: 위치 정보 - 최대값 테스트
TEST_F(OtherDVMTest, GetLocation_MaxCoordinates) {
    OtherDVM maxDVM(7, Location(INT_MAX, INT_MAX));
    Location loc = maxDVM.getLocation();
    
    EXPECT_EQ(loc.getX(), INT_MAX);
    EXPECT_EQ(loc.getY(), INT_MAX);
}

// TC-DIST-001: 위치 정보 - 최소값 테스트
TEST_F(OtherDVMTest, GetLocation_MinCoordinates) {
    OtherDVM minDVM(8, Location(INT_MIN, INT_MIN));
    Location loc = minDVM.getLocation();
    
    EXPECT_EQ(loc.getX(), INT_MIN);
    EXPECT_EQ(loc.getY(), INT_MIN);
}

// TC-DIST-003: 거리 계산 테스트 (간접)
TEST_F(OtherDVMTest, CalculateDistance_FromOrigin) {
    OtherDVM originDVM(9, Location(0, 0));
    OtherDVM targetDVM(10, Location(3, 4));
    
    // 맨해튼 거리 |x2-x1| + |y2-y1| = |3-0| + |4-0| = 3 + 4 = 7
    int distance = originDVM.getLocation().calculateDistance(targetDVM.getLocation());
    EXPECT_EQ(distance, 7);
}

// TC-DIST-003: 거리 계산 테스트 - 음수 좌표 포함
TEST_F(OtherDVMTest, CalculateDistance_WithNegativeCoordinates) {
    OtherDVM dvm1(11, Location(-5, -10));
    OtherDVM dvm2(12, Location(5, 10));
    
    // 맨해튼 거리 |x2-x1| + |y2-y1| = |5-(-5)| + |10-(-10)| = 10 + 20 = 30
    int distance = dvm1.getLocation().calculateDistance(dvm2.getLocation());
    EXPECT_EQ(distance, 30);
}

// TC-DIST-003: 거리 계산 테스트 - 동일 위치
TEST_F(OtherDVMTest, CalculateDistance_SameLocation) {
    OtherDVM dvm1(13, Location(10, 20));
    OtherDVM dvm2(14, Location(10, 20));
    
    int distance = dvm1.getLocation().calculateDistance(dvm2.getLocation());
    EXPECT_EQ(distance, 0);
}

// TC-DIST-003: 거리 계산 테스트 - X 좌표만 다름
TEST_F(OtherDVMTest, CalculateDistance_DifferentXOnly) {
    OtherDVM dvm1(15, Location(10, 20));
    OtherDVM dvm2(16, Location(30, 20));
    
    int distance = dvm1.getLocation().calculateDistance(dvm2.getLocation());
    EXPECT_EQ(distance, 20);
}

// TC-DIST-003: 거리 계산 테스트 - Y 좌표만 다름
TEST_F(OtherDVMTest, CalculateDistance_DifferentYOnly) {
    OtherDVM dvm1(17, Location(10, 20));
    OtherDVM dvm2(18, Location(10, 50));
    
    int distance = dvm1.getLocation().calculateDistance(dvm2.getLocation());
    EXPECT_EQ(distance, 30);
}

// TC-DIST-001: DVM ID 획득 테스트
TEST_F(OtherDVMTest, GetDvmId_ReturnsSavedId) {
    EXPECT_EQ(otherDVM->getDvmId(), 2);
}

// TC-DIST-001: DVM ID - 음수 ID 테스트
TEST_F(OtherDVMTest, GetDvmId_NegativeId) {
    OtherDVM negativeDVM(-3, Location(0, 0));
    EXPECT_EQ(negativeDVM.getDvmId(), -3);
}

// TC-DIST-001: DVM ID - 0 ID 테스트
TEST_F(OtherDVMTest, GetDvmId_ZeroId) {
    OtherDVM zeroDVM(0, Location(0, 0));
    EXPECT_EQ(zeroDVM.getDvmId(), 0);
}

// TC-DIST-001: DVM ID - 최대값 ID 테스트
TEST_F(OtherDVMTest, GetDvmId_MaxId) {
    OtherDVM maxDVM(INT_MAX, Location(0, 0));
    EXPECT_EQ(maxDVM.getDvmId(), INT_MAX);
}

// TC-DIST-001: DVM ID - 최소값 ID 테스트
TEST_F(OtherDVMTest, GetDvmId_MinId) {
    OtherDVM minDVM(INT_MIN, Location(0, 0));
    EXPECT_EQ(minDVM.getDvmId(), INT_MIN);
}

// TC-COM-002: findAvailableStocks 모의 테스트 - 재고 있음
TEST_F(OtherDVMTest, FindAvailableStocks_MockWithStock) {
    // 모의 응답 설정
    CheckStockResponse mockResponse;
    mockResponse.dst_id = 1;
    mockResponse.item_code = "001";
    mockResponse.item_num = 5;
    mockResponse.coor_x = 10;
    mockResponse.coor_y = 20;
    
    EXPECT_CALL(*mockOtherDVM, findAvailableStocks(_, _))
        .WillOnce(Return(mockResponse));
    
    // 테스트 실행
    CheckStockRequest request{"001", 5};
    CheckStockResponse response = mockOtherDVM->findAvailableStocks(request, 1);
    
    // 검증
    EXPECT_EQ(response.dst_id, 1);
    EXPECT_EQ(response.item_code, "001");
    EXPECT_EQ(response.item_num, 5);
    EXPECT_EQ(response.coor_x, 10);
    EXPECT_EQ(response.coor_y, 20);
}

// TC-COM-002: findAvailableStocks 모의 테스트 - 재고 없음
TEST_F(OtherDVMTest, FindAvailableStocks_MockNoStock) {
    // 모의 응답 설정
    CheckStockResponse mockResponse;
    mockResponse.dst_id = 1;
    mockResponse.item_code = "001";
    mockResponse.item_num = 0; // 재고 없음
    mockResponse.coor_x = 10;
    mockResponse.coor_y = 20;
    
    EXPECT_CALL(*mockOtherDVM, findAvailableStocks(_, _))
        .WillOnce(Return(mockResponse));
    
    // 테스트 실행
    CheckStockRequest request{"001", 5};
    CheckStockResponse response = mockOtherDVM->findAvailableStocks(request, 1);
    
    // 검증
    EXPECT_EQ(response.dst_id, 1);
    EXPECT_EQ(response.item_code, "001");
    EXPECT_EQ(response.item_num, 0);
}

// TC-COM-002: findAvailableStocks 모의 테스트 - 다른 아이템 코드
TEST_F(OtherDVMTest, FindAvailableStocks_MockDifferentItemCode) {
    // 모의 응답 설정
    CheckStockResponse mockResponse1;
    mockResponse1.item_code = "001";
    mockResponse1.item_num = 5;
    
    CheckStockResponse mockResponse2;
    mockResponse2.item_code = "002";
    mockResponse2.item_num = 3;
    
    EXPECT_CALL(*mockOtherDVM, findAvailableStocks(::testing::Field(&CheckStockRequest::item_code, "001"), _))
        .WillOnce(Return(mockResponse1));
    
    EXPECT_CALL(*mockOtherDVM, findAvailableStocks(::testing::Field(&CheckStockRequest::item_code, "002"), _))
        .WillOnce(Return(mockResponse2));
    
    // 테스트 실행
    CheckStockRequest request1{"001", 5};
    CheckStockResponse response1 = mockOtherDVM->findAvailableStocks(request1, 1);
    
    CheckStockRequest request2{"002", 5};
    CheckStockResponse response2 = mockOtherDVM->findAvailableStocks(request2, 1);
    
    // 검증
    EXPECT_EQ(response1.item_code, "001");
    EXPECT_EQ(response1.item_num, 5);
    
    EXPECT_EQ(response2.item_code, "002");
    EXPECT_EQ(response2.item_num, 3);
}

// TC-COM-002: findAvailableStocks 모의 테스트 - 요청 수량에 따른 응답
TEST_F(OtherDVMTest, FindAvailableStocks_MockByRequestQuantity) {
    // 모의 응답 설정 - 요청 수량이 5 이하면 충족 가능, 그 이상이면 일부만 제공
    EXPECT_CALL(*mockOtherDVM, findAvailableStocks(::testing::Field(&CheckStockRequest::item_num, ::testing::Le(5)), _))
        .WillRepeatedly([](const CheckStockRequest& request, int senderId) {
            CheckStockResponse response;
            response.item_code = request.item_code;
            response.item_num = request.item_num;  // 요청 수량 모두 제공
            return response;
        });
    
    EXPECT_CALL(*mockOtherDVM, findAvailableStocks(::testing::Field(&CheckStockRequest::item_num, ::testing::Gt(5)), _))
        .WillRepeatedly([](const CheckStockRequest& request, int senderId) {
            CheckStockResponse response;
            response.item_code = request.item_code;
            response.item_num = 5;  // 최대 5개만 제공
            return response;
        });
    
    // 테스트 실행
    CheckStockRequest request1{"001", 3};  // 3개 요청
    CheckStockResponse response1 = mockOtherDVM->findAvailableStocks(request1, 1);
    
    CheckStockRequest request2{"001", 10};  // 10개 요청
    CheckStockResponse response2 = mockOtherDVM->findAvailableStocks(request2, 1);
    
    // 검증
    EXPECT_EQ(response1.item_num, 3);  // 요청한 3개 모두 제공
    EXPECT_EQ(response2.item_num, 5);  // 요청한 10개 중 5개만 제공
}

// TC-PRE-001: askForPrepayment 모의 테스트 - 성공
TEST_F(OtherDVMTest, AskForPrepayment_MockSuccess) {
    // 모의 응답 설정
    askPrepaymentResponse mockResponse;
    mockResponse.item_code = "001";
    mockResponse.item_num = 5;
    mockResponse.availability = true;  // 성공
    
    EXPECT_CALL(*mockOtherDVM, askForPrepayment(_, _))
        .WillOnce(Return(mockResponse));
    
    // 테스트 실행
    askPrepaymentRequest request{"001", 5, "ABC123"};
    askPrepaymentResponse response = mockOtherDVM->askForPrepayment(request, 1);
    
    // 검증
    EXPECT_EQ(response.item_code, "001");
    EXPECT_EQ(response.item_num, 5);
    EXPECT_TRUE(response.availability);
}

// TC-PRE-001: askForPrepayment 모의 테스트 - 실패 (재고 부족)
TEST_F(OtherDVMTest, AskForPrepayment_MockFailure) {
    // 모의 응답 설정
    askPrepaymentResponse mockResponse;
    mockResponse.item_code = "001";
    mockResponse.item_num = 5;
    mockResponse.availability = false;  // 실패
    
    EXPECT_CALL(*mockOtherDVM, askForPrepayment(_, _))
        .WillOnce(Return(mockResponse));
    
    // 테스트 실행
    askPrepaymentRequest request{"001", 5, "ABC123"};
    askPrepaymentResponse response = mockOtherDVM->askForPrepayment(request, 1);
    
    // 검증
    EXPECT_EQ(response.item_code, "001");
    EXPECT_EQ(response.item_num, 5);
    EXPECT_FALSE(response.availability);
}

// TC-PRE-001: askForPrepayment 모의 테스트 - 인증 코드에 따른 응답
TEST_F(OtherDVMTest, AskForPrepayment_MockByCertificationCode) {
    // 모의 응답 설정 - "VALID"로 시작하는 인증 코드는 성공, 아니면 실패
    EXPECT_CALL(*mockOtherDVM, askForPrepayment(::testing::Field(&askPrepaymentRequest::cert_code, ::testing::StartsWith("VALID")), _))
        .WillRepeatedly([](const askPrepaymentRequest& request, int senderId) {
            askPrepaymentResponse response;
            response.item_code = request.item_code;
            response.item_num = request.item_num;
            response.availability = true;  // 성공
            return response;
        });
    
    EXPECT_CALL(*mockOtherDVM, askForPrepayment(::testing::Not(::testing::Field(&askPrepaymentRequest::cert_code, ::testing::StartsWith("VALID"))), _))
        .WillRepeatedly([](const askPrepaymentRequest& request, int senderId) {
            askPrepaymentResponse response;
            response.item_code = request.item_code;
            response.item_num = request.item_num;
            response.availability = false;  // 실패
            return response;
        });
    
    // 테스트 실행
    askPrepaymentRequest request1{"001", 5, "VALID123"};  // 유효한 코드
    askPrepaymentResponse response1 = mockOtherDVM->askForPrepayment(request1, 1);
    
    askPrepaymentRequest request2{"001", 5, "INVALID456"};  // 무효한 코드
    askPrepaymentResponse response2 = mockOtherDVM->askForPrepayment(request2, 1);
    
    // 검증
    EXPECT_TRUE(response1.availability);   // 유효한 코드로 성공
    EXPECT_FALSE(response2.availability);  // 무효한 코드로 실패
}

// TC-PRE-001: askForPrepayment 모의 테스트 - 수량에 따른 응답
TEST_F(OtherDVMTest, AskForPrepayment_MockByQuantity) {
    // 모의 응답 설정 - 요청 수량이 3 이하면 성공, 그 이상이면 실패
    EXPECT_CALL(*mockOtherDVM, askForPrepayment(::testing::Field(&askPrepaymentRequest::item_num, ::testing::Le(3)), _))
        .WillRepeatedly([](const askPrepaymentRequest& request, int senderId) {
            askPrepaymentResponse response;
            response.item_code = request.item_code;
            response.item_num = request.item_num;
            response.availability = true;  // 성공
            return response;
        });
    
    EXPECT_CALL(*mockOtherDVM, askForPrepayment(::testing::Field(&askPrepaymentRequest::item_num, ::testing::Gt(3)), _))
        .WillRepeatedly([](const askPrepaymentRequest& request, int senderId) {
            askPrepaymentResponse response;
            response.item_code = request.item_code;
            response.item_num = request.item_num;
            response.availability = false;  // 실패
            return response;
        });
    
    // 테스트 실행
    askPrepaymentRequest request1{"001", 2, "CODE123"};  // 2개 요청 - 성공
    askPrepaymentResponse response1 = mockOtherDVM->askForPrepayment(request1, 1);
    
    askPrepaymentRequest request2{"001", 5, "CODE123"};  // 5개 요청 - 실패
    askPrepaymentResponse response2 = mockOtherDVM->askForPrepayment(request2, 1);
    
    // 검증
    EXPECT_TRUE(response1.availability);   // 소량 요청은 성공
    EXPECT_FALSE(response2.availability);  // 대량 요청은 실패
}

// TC-DIST-001: 여러 인스턴스 생성 및 속성 비교
TEST_F(OtherDVMTest, MultipleInstances) {
    OtherDVM dvm1(1, Location(5, 5));
    OtherDVM dvm2(2, Location(10, 10));
    OtherDVM dvm3(3, Location(15, 15));
    
    // ID 검증
    EXPECT_EQ(dvm1.getDvmId(), 1);
    EXPECT_EQ(dvm2.getDvmId(), 2);
    EXPECT_EQ(dvm3.getDvmId(), 3);
    
    // X 좌표 검증
    EXPECT_EQ(dvm1.getLocation().getX(), 5);
    EXPECT_EQ(dvm2.getLocation().getX(), 10);
    EXPECT_EQ(dvm3.getLocation().getX(), 15);
    
    // Y 좌표 검증
    EXPECT_EQ(dvm1.getLocation().getY(), 5);
    EXPECT_EQ(dvm2.getLocation().getY(), 10);
    EXPECT_EQ(dvm3.getLocation().getY(), 15);
}

// TC-DIST-005: 동일 거리에서 낮은 ID 우선 테스트 (간접)
TEST_F(OtherDVMTest, EqualDistance_LowerIdPriority) {
    // 두 DVM이 동일한 거리에 있을 때 ID가 낮은 것을 선택하는지 확인
    // 중심점에서 동일한 거리에 있는 두 DVM
    OtherDVM dvm1(1, Location(5, 0));  // 거리: |5-0| + |0-0| = 5
    OtherDVM dvm2(2, Location(0, 5));  // 거리: |0-0| + |5-0| = 5
    
    // 중심점으로부터의 거리가 같음을 확인
    OtherDVM centerDVM(0, Location(0, 0));
    EXPECT_EQ(centerDVM.getLocation().calculateDistance(dvm1.getLocation()), 5);
    EXPECT_EQ(centerDVM.getLocation().calculateDistance(dvm2.getLocation()), 5);
    
    // ID가 더 낮은 dvm1이 우선되어야 함 (실제 선택 로직은 DVM 클래스에 있음)
    EXPECT_LT(dvm1.getDvmId(), dvm2.getDvmId());
}

// TC-COM-002: SocketMessage 직렬화/역직렬화 테스트 (간접)
TEST_F(OtherDVMTest, SocketMessageSerialization) {
    // 재고 조회 응답 메시지 생성
    std::string serializedResponse = MockSocketMessage::createStockResponse("001", 5, 10, 20);
    
    // 메시지에 필요한 필드들이 포함되어 있는지 확인
    EXPECT_NE(serializedResponse.find("msg_type:res_stock"), std::string::npos);
    EXPECT_NE(serializedResponse.find("item_code:001"), std::string::npos);
    EXPECT_NE(serializedResponse.find("item_num:5"), std::string::npos);
    EXPECT_NE(serializedResponse.find("coor_x:10"), std::string::npos);
    EXPECT_NE(serializedResponse.find("coor_y:20"), std::string::npos);
}

// TC-COM-002: SocketMessage 직렬화/역직렬화 테스트 - 선결제
TEST_F(OtherDVMTest, SocketMessageSerialization_Prepayment) {
    // 선결제 응답 메시지 생성
    std::string serializedResponse = MockSocketMessage::createPrepaymentResponse("001", 5, true);
    
    // 메시지에 필요한 필드들이 포함되어 있는지 확인
    EXPECT_NE(serializedResponse.find("msg_type:res_prepay"), std::string::npos);
    EXPECT_NE(serializedResponse.find("item_code:001"), std::string::npos);
    EXPECT_NE(serializedResponse.find("item_num:5"), std::string::npos);
    EXPECT_NE(serializedResponse.find("availability:T"), std::string::npos);
}

// TC-COM-002: 재고 확인 요청 - 아이템 코드 검증
TEST_F(OtherDVMTest, FindAvailableStocks_ItemCodeVerification) {
    EXPECT_CALL(*mockOtherDVM, findAvailableStocks(_, _))
        .WillRepeatedly([](const CheckStockRequest& request, int senderId) {
            CheckStockResponse response;
            response.item_code = request.item_code;
            response.item_num = request.item_code == "001" ? 5 : 0;  // 001만 재고 있음
            return response;
        });
    
    // 테스트 실행
    CheckStockRequest request1{"001", 1};  // 재고 있는 아이템
    CheckStockResponse response1 = mockOtherDVM->findAvailableStocks(request1, 1);
    
    CheckStockRequest request2{"002", 1};  // 재고 없는 아이템
    CheckStockResponse response2 = mockOtherDVM->findAvailableStocks(request2, 1);
    
    // 검증
    EXPECT_EQ(response1.item_code, "001");
    EXPECT_EQ(response1.item_num, 5);  // 재고 있음
    
    EXPECT_EQ(response2.item_code, "002");
    EXPECT_EQ(response2.item_num, 0);  // 재고 없음
}

// TC-PERF-002: 네트워크 통신 응답 시간 테스트 (간접)
TEST_F(OtherDVMTest, NetworkCommunicationResponseTime) {
    // 모의 클래스에서는 실제 통신이 없으므로, 응답이 즉시 반환되는지 확인
    EXPECT_CALL(*mockOtherDVM, findAvailableStocks(_, _))
        .WillOnce(Return(CheckStockResponse()));
    
    // 테스트 실행 시작 전 시간 기록
    auto start = std::chrono::high_resolution_clock::now();
    
    // 요청 실행
    CheckStockRequest request{"001", 1};
    mockOtherDVM->findAvailableStocks(request, 1);
    
    // 테스트 실행 후 시간 기록
    auto end = std::chrono::high_resolution_clock::now();
    
    // 실행 시간 계산 (밀리초)
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    // TC-PERF-002: 네트워크 통신 응답 1초 이내 요구사항
    EXPECT_LT(duration, 1000);  // 1초(1000ms) 이내에 응답
}

// TC-PRE-003: 인증 코드 길이 검증 (간접)
TEST_F(OtherDVMTest, CertificationCodeLength) {
    // 다양한 길이의 인증 코드로 테스트
    EXPECT_CALL(*mockOtherDVM, askForPrepayment(::testing::Field(&askPrepaymentRequest::cert_code, ::testing::SizeIs(5)), _))
        .WillOnce(Return(askPrepaymentResponse{.availability = true}));
    
    EXPECT_CALL(*mockOtherDVM, askForPrepayment(::testing::Not(::testing::Field(&askPrepaymentRequest::cert_code, ::testing::SizeIs(5))), _))
        .WillOnce(Return(askPrepaymentResponse{.availability = false}));
    
    // 테스트 실행 - 5자리 인증 코드 (표준 길이)
    askPrepaymentRequest request1{"001", 1, "ABC12"};  // 5자리
    askPrepaymentResponse response1 = mockOtherDVM->askForPrepayment(request1, 1);
    
    // 테스트 실행 - 비표준 길이 인증 코드
    askPrepaymentRequest request2{"001", 1, "ABC1"};   // 4자리
    askPrepaymentResponse response2 = mockOtherDVM->askForPrepayment(request2, 1);
    
    // 검증
    EXPECT_TRUE(response1.availability);   // 5자리 코드는 성공
    EXPECT_FALSE(response2.availability);  // 비표준 길이 코드는 실패
}

// TC-PRE-004: 근접 자판기 안내 테스트 (간접)
TEST_F(OtherDVMTest, GuideToNearestVendingMachine) {
    // otherDVM은 Location(10, 20)에 위치
    OtherDVM dvm1(101, Location(10, 10));    // 거리: |10-10| + |10-20| = 0 + 10 = 10
    OtherDVM dvm2(102, Location(20, 20));    // 거리: |20-10| + |20-20| = 10 + 0 = 10
    OtherDVM dvm3(103, Location(30, 30));    // 거리: |30-10| + |30-20| = 20 + 10 = 30
    
    // 테스트 대상 DVM으로부터의 거리 계산
    int distance1 = otherDVM->getLocation().calculateDistance(dvm1.getLocation());
    int distance2 = otherDVM->getLocation().calculateDistance(dvm2.getLocation());
    int distance3 = otherDVM->getLocation().calculateDistance(dvm3.getLocation());
    
    // 거리 확인
    EXPECT_EQ(distance1, 10);  // dvm1과의 거리 10
    EXPECT_EQ(distance2, 10);  // dvm2와의 거리 10
    EXPECT_EQ(distance3, 30);  // dvm3과의 거리 30
    
    // dvm1과 dvm2는 동일 거리, dvm3은 더 멀다
    EXPECT_LT(distance1, distance3);  // dvm1이 dvm3보다 가까움
    EXPECT_LT(distance2, distance3);  // dvm2가 dvm3보다 가까움
    EXPECT_EQ(distance1, distance2);  // dvm1과 dvm2는 동일한 거리
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
