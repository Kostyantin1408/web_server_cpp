#ifndef SOCKETLISTENER_TEST_HPP
#define SOCKETLISTENER_TEST_HPP

#include <gtest/gtest.h>
#include <server/SocketListener.hpp>
#include <sys/eventfd.h>
#include <fcntl.h>
#include <unistd.h>
#include <thread>

class SocketListenerTest : public ::testing::Test {
protected:
    SocketListener listener{{10, 0}};

    bool isNonBlocking(int fd) {
        int flags = fcntl(fd, F_GETFL, 0);
        return (flags & O_NONBLOCK) != 0;
    }
};

TEST_F(SocketListenerTest, MakeNonBlocking_SetsFdNonBlocking) {
    int pipefd[2];
    ASSERT_EQ(pipe(pipefd), 0);
    EXPECT_FALSE(isNonBlocking(pipefd[0]));

    listener.add_socket(pipefd[0], [](int){}, EPOLLIN);
    EXPECT_TRUE(isNonBlocking(pipefd[0]));

    listener.removeSocket(pipefd[0]);
    close(pipefd[0]);
    close(pipefd[1]);
}

TEST_F(SocketListenerTest, Run_InvokesCallbackOnEvent) {
    int efd = eventfd(0, EFD_NONBLOCK);
    ASSERT_GE(efd, 0);

    bool called = false;
    listener.add_socket(efd, [&](int fd) {
        uint64_t v;
        read(fd, &v, sizeof(v));
        called = true;
        listener.stop();
    }, EPOLLIN);

    std::thread runner([&]() { listener.run(); });

    uint64_t one = 1;
    EXPECT_EQ(write(efd, &one, sizeof(one)), sizeof(one));
    runner.join();

    EXPECT_TRUE(called);
    listener.removeSocket(efd);
    close(efd);
}

TEST_F(SocketListenerTest, RemoveSocket_DeregistersCallback) {
    int efd = eventfd(0, EFD_NONBLOCK);
    ASSERT_GE(efd, 0);

    bool called = false;
    listener.add_socket(efd, [&](int){ called = true; listener.stop(); }, EPOLLIN);
    listener.removeSocket(efd);

    std::thread runner([&]() { listener.run(); });
    uint64_t one = 1;
    write(efd, &one, sizeof(one));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    listener.stop();
    runner.join();

    EXPECT_FALSE(called);
    close(efd);
}

TEST_F(SocketListenerTest, AddInvalidFd_ThrowsRuntimeError) {
    int badfd = -1;
    EXPECT_THROW(listener.add_socket(badfd, [](int){}, EPOLLIN), std::runtime_error);
}

TEST_F(SocketListenerTest, Stop_WithoutRun_DoesNotCrash) {
    EXPECT_NO_THROW(listener.stop());
}

TEST_F(SocketListenerTest, Run_WithNoSockets_ExitsOnStop) {
    std::thread runner([&]() { listener.run(); });
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    listener.stop();
    EXPECT_NO_THROW(runner.join());
}

TEST_F(SocketListenerTest, HandlesEPOLLOUTEvent) {
    int pipefd[2];
    ASSERT_EQ(pipe(pipefd), 0);
    bool called = false;
    listener.add_socket(pipefd[1], [&](int){ called = true; listener.stop(); }, EPOLLOUT);

    std::thread runner([&]() { listener.run(); });
    runner.join();

    EXPECT_TRUE(called);
    listener.removeSocket(pipefd[1]);
    close(pipefd[0]);
    close(pipefd[1]);
}

TEST_F(SocketListenerTest, MultipleSockets_InvokesAllCallbacks) {
    int efd1 = eventfd(0, EFD_NONBLOCK);
    int efd2 = eventfd(0, EFD_NONBLOCK);
    ASSERT_GE(efd1, 0);
    ASSERT_GE(efd2, 0);

    int count = 0;
    listener.add_socket(efd1, [&](int fd) { uint64_t v; read(fd, &v, sizeof(v)); ++count; }, EPOLLIN);
    listener.add_socket(efd2, [&](int fd) { uint64_t v; read(fd, &v, sizeof(v)); ++count; listener.stop(); }, EPOLLIN);

    std::thread runner([&]() { listener.run(); });

    uint64_t one = 1;
    write(efd1, &one, sizeof(one));
    write(efd2, &one, sizeof(one));
    runner.join();

    EXPECT_EQ(count, 2);
    listener.removeSocket(efd1);
    listener.removeSocket(efd2);
    close(efd1);
    close(efd2);
}

TEST_F(SocketListenerTest, AddExistingSocket_ReplacesCallback) {
    int efd = eventfd(0, EFD_NONBLOCK);
    ASSERT_GE(efd, 0);

    bool firstCalled = false, secondCalled = false;
    listener.add_socket(efd, [&](int){ firstCalled = true; }, EPOLLIN);
    listener.add_socket(efd, [&](int){ secondCalled = true; listener.stop(); }, EPOLLIN);

    std::thread runner([&]() { listener.run(); });

    uint64_t one = 1;
    write(efd, &one, sizeof(one));
    runner.join();

    EXPECT_FALSE(firstCalled);
    EXPECT_TRUE(secondCalled);
    listener.removeSocket(efd);
    close(efd);
}

TEST_F(SocketListenerTest, RemovingNonexistentSocket_DoesNothing) {
    int fakefd = 9999;
    EXPECT_NO_THROW(listener.removeSocket(fakefd));
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

#endif // SOCKETLISTENER_TEST_HPP
