#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <semaphore.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>

class MockMutex {
 public:
  MOCK_METHOD(void, lock, ());
  MOCK_METHOD(void, unlock, ());
  MOCK_METHOD(bool, try_lock, ());
};

class TestSemaphoreMockMutex : public misc::semaphore<MockMutex, std::condition_variable_any>, public testing::Test {
 public:
    TestSemaphoreMockMutex() : misc::semaphore<MockMutex, std::condition_variable_any>{10} {}
};

TEST_F(TestSemaphoreMockMutex, Lock) {
    using namespace ::testing;

    EXPECT_CALL(m_mutex, lock);
    EXPECT_CALL(m_mutex, unlock);
    acquire(15);
    ASSERT_EQ(m_in_use, 15);

    EXPECT_CALL(m_mutex, lock);
    EXPECT_CALL(m_mutex, unlock);
    release(15);
    ASSERT_EQ(m_in_use, 0);

    EXPECT_CALL(m_mutex, lock);
    EXPECT_CALL(m_mutex, unlock);
    acquire(10);
    ASSERT_EQ(m_in_use, 10);

    EXPECT_CALL(m_mutex, lock);
    EXPECT_CALL(m_mutex, unlock);
    release(10);
    ASSERT_EQ(m_in_use, 0);

    EXPECT_CALL(m_mutex, try_lock).WillOnce(Return(false));
    ASSERT_FALSE(try_acquire(15));
    ASSERT_EQ(m_in_use, 0);

    EXPECT_CALL(m_mutex, try_lock).WillOnce(Return(true));
    EXPECT_CALL(m_mutex, unlock);
    ASSERT_TRUE(try_acquire(15));
    ASSERT_EQ(m_in_use, 15);

    EXPECT_CALL(m_mutex, try_lock).WillOnce(Return(true));
    EXPECT_CALL(m_mutex, unlock);
    ASSERT_FALSE(try_acquire(5));
    ASSERT_EQ(m_in_use, 15);

    EXPECT_CALL(m_mutex, lock);
    EXPECT_CALL(m_mutex, unlock);
    release(15);
    ASSERT_EQ(m_in_use, 0);
}



class TestSemaphore : public misc::semaphore<>, public testing::Test {
 public:
    TestSemaphore() : misc::semaphore<>{10} {}
};

TEST_F(TestSemaphore, Lock) {
    using namespace ::testing;


    {
    m_mutex.lock();
    const auto& f = std::async([&]{
        acquire(15);
    });

    EXPECT_EQ(f.wait_for(std::chrono::milliseconds{1}), std::future_status::timeout);
    m_mutex.unlock();
    EXPECT_EQ(f.wait_for(std::chrono::milliseconds{1}), std::future_status::ready);
    EXPECT_EQ(m_in_use, 15);
    }

    {
    const auto& f = std::async([&]{
        acquire(5);
    });

    EXPECT_EQ(f.wait_for(std::chrono::milliseconds{1}), std::future_status::timeout);
    release(15);
    EXPECT_EQ(f.wait_for(std::chrono::milliseconds{1}), std::future_status::ready);
    EXPECT_EQ(m_in_use, 5);
    }

    {
    const auto& f = std::async([&]{
        acquire(7);
    });

    EXPECT_EQ(f.wait_for(std::chrono::milliseconds{1}), std::future_status::timeout);
    release(5);
    EXPECT_EQ(f.wait_for(std::chrono::milliseconds{1}), std::future_status::ready);
    EXPECT_EQ(m_in_use, 7);
    }

    {
    auto f = std::async([&]{
        return try_acquire(7);
    });

    EXPECT_FALSE(f.get());
    EXPECT_EQ(m_in_use, 7);
    }

    {
    release(7);
    auto f = std::async([&]{
        return try_acquire(7);
    });

    EXPECT_TRUE(f.get());
    EXPECT_EQ(m_in_use, 7);
    }
}