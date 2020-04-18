#include <gtest/gtest.h>
#include <sync/sync.h>
#include <sw_sync.h>
#include <fcntl.h>
#include <vector>
#include <string>
#include <cassert>
#include <iostream>
#include <unistd.h>
#include <thread>
#include <poll.h>
#include <mutex>
#include <algorithm>
#include <tuple>
#include <random>
#include <unordered_map>

// TODO: better stress tests?
// Handle more than 64 fd's simultaneously, i.e. fix sync_fence_info's 4k limit.
// Handle wraparound in timelines like nvidia.

using namespace std;

namespace {

// C++ wrapper class for sync timeline.
class SyncTimeline {
    int m_fd = -1;
    bool m_fdInitialized = false;
public:
    SyncTimeline(const SyncTimeline &) = delete;
    SyncTimeline& operator=(SyncTimeline&) = delete;
    SyncTimeline() noexcept {
        int fd = sw_sync_timeline_create();
        if (fd == -1)
            return;
        m_fdInitialized = true;
        m_fd = fd;
    }
    void destroy() {
        if (m_fdInitialized) {
            close(m_fd);
            m_fd = -1;
            m_fdInitialized = false;
        }
    }
    ~SyncTimeline() {
        destroy();
    }
    bool isValid() const {
        if (m_fdInitialized) {
            int status = fcntl(m_fd, F_GETFD, 0);
            if (status >= 0)
                return true;
            else
                return false;
        }
        else {
            return false;
        }
    }
    int getFd() const {
        return m_fd;
    }
    int inc(int val = 1) {
        return sw_sync_timeline_inc(m_fd, val);
    }
};

struct SyncPointInfo {
    std::string driverName;
    std::string objectName;
    uint64_t timeStampNs;
    int status; // 1 sig, 0 active, neg is err
};

// Wrapper class for sync fence.
class SyncFence {
    int m_fd = -1;
    bool m_fdInitialized = false;
    static int s_fenceCount;

    void setFd(int fd) {
        m_fd = fd;
        m_fdInitialized = true;
    }
    void clearFd() {
        m_fd = -1;
        m_fdInitialized = false;
    }
public:
    bool isValid() const {
        if (m_fdInitialized) {
            int status = fcntl(m_fd, F_GETFD, 0);
            if (status >= 0)
                return true;
            else
                return false;
        }
        else {
            return false;
        }
    }
    SyncFence& operator=(SyncFence &&rhs) noexcept {
        destroy();
        if (rhs.isValid()) {
            setFd(rhs.getFd());
            rhs.clearFd();
        }
        return *this;
    }
    SyncFence(SyncFence &&fence) noexcept {
        if (fence.isValid()) {
            setFd(fence.getFd());
            fence.clearFd();
        }
    }
    SyncFence(const SyncFence &fence) noexcept {
        // This is ok, as sync fences are immutable after construction, so a dup
        // is basically the same thing as a copy.
        if (fence.isValid()) {
            int fd = dup(fence.getFd());
            if (fd == -1)
                return;
            setFd(fd);
        }
    }
    SyncFence(const SyncTimeline &timeline,
              int value,
              const char *name = nullptr) noexcept {
        std::string autoName = "allocFence";
        autoName += s_fenceCount;
        s_fenceCount++;
        int fd = sw_sync_fence_create(timeline.getFd(), name ? name : autoName.c_str(), value);
        if (fd == -1)
            return;
        setFd(fd);
    }
    SyncFence(const SyncFence &a, const SyncFence &b, const char *name = nullptr) noexcept {
        std::string autoName = "mergeFence";
        autoName += s_fenceCount;
        s_fenceCount++;
        int fd = sync_merge(name ? name : autoName.c_str(), a.getFd(), b.getFd());
        if (fd == -1)
            return;
        setFd(fd);
    }
    SyncFence(const vector<SyncFence> &sources) noexcept {
        assert(sources.size());
        SyncFence temp(*begin(sources));
        for (auto itr = ++begin(sources); itr != end(sources); ++itr) {
            temp = SyncFence(*itr, temp);
        }
        if (temp.isValid()) {
            setFd(temp.getFd());
            temp.clearFd();
        }
    }
    void destroy() {
        if (isValid()) {
            close(m_fd);
            clearFd();
        }
    }
    ~SyncFence() {
        destroy();
    }
    int getFd() const {
        return m_fd;
    }
    int wait(int timeout = -1) {
        return sync_wait(m_fd, timeout);
    }
    vector<SyncPointInfo> getInfo() const {
        struct sync_pt_info *pointInfo = nullptr;
        vector<SyncPointInfo> fenceInfo;
        sync_fence_info_data *info = sync_fence_info(getFd());
        if (!info) {
            return fenceInfo;
        }
        while ((pointInfo = sync_pt_info(info, pointInfo))) {
            fenceInfo.push_back(SyncPointInfo{
                pointInfo->driver_name,
                pointInfo->obj_name,
                pointInfo->timestamp_ns,
                pointInfo->status});
        }
        sync_fence_info_free(info);
        return fenceInfo;
    }
    int getSize() const {
        return getInfo().size();
    }
    int getSignaledCount() const {
        return countWithStatus(1);
    }
    int getActiveCount() const {
        return countWithStatus(0);
    }
    int getErrorCount() const {
        return countWithStatus(-1);
    }
private:
    int countWithStatus(int status) const {
        int count = 0;
        for (auto &info : getInfo()) {
            if (info.status == status) {
                count++;
            }
        }
        return count;
    }
};

int SyncFence::s_fenceCount = 0;

TEST(AllocTest, Timeline) {
    SyncTimeline timeline;
    ASSERT_TRUE(timeline.isValid());
}

TEST(AllocTest, Fence) {
    SyncTimeline timeline;
    ASSERT_TRUE(timeline.isValid());

    SyncFence fence(timeline, 1);
    ASSERT_TRUE(fence.isValid());
}

TEST(AllocTest, FenceNegative) {
    int timeline = sw_sync_timeline_create();
    ASSERT_GT(timeline, 0);

    // bad fd.
    ASSERT_LT(sw_sync_fence_create(-1, "fence", 1), 0);

    // No name - segfaults in user space.
    // Maybe we should be friendlier here?
    /*
    ASSERT_LT(sw_sync_fence_create(timeline, nullptr, 1), 0);
    */
    close(timeline);
}

TEST(FenceTest, OneTimelineWait) {
    SyncTimeline timeline;
    ASSERT_TRUE(timeline.isValid());

    SyncFence fence(timeline, 5);
    ASSERT_TRUE(fence.isValid());

    // Wait on fence until timeout.
    ASSERT_EQ(fence.wait(0), -1);
    ASSERT_EQ(errno, ETIME);

    // Advance timeline from 0 -> 1
    ASSERT_EQ(timeline.inc(1), 0);

    // Wait on fence until timeout.
    ASSERT_EQ(fence.wait(0), -1);
    ASSERT_EQ(errno, ETIME);

    // Signal the fence.
    ASSERT_EQ(timeline.inc(4), 0);

    // Wait successfully.
    ASSERT_EQ(fence.wait(0), 0);

    // Go even futher, and confirm wait still succeeds.
    ASSERT_EQ(timeline.inc(10), 0);
    ASSERT_EQ(fence.wait(0), 0);
}

TEST(FenceTest, OneTimelinePoll) {
    SyncTimeline timeline;
    ASSERT_TRUE(timeline.isValid());

    SyncFence fence(timeline, 100);
    ASSERT_TRUE(fence.isValid());

    fd_set set;
    FD_ZERO(&set);
    FD_SET(fence.getFd(), &set);

    // Poll the fence, and wait till timeout.
    timeval time = {0};
    ASSERT_EQ(select(fence.getFd() + 1, &set, nullptr, nullptr, &time), 0);

    // Advance the timeline.
    timeline.inc(100);
    timeline.inc(100);

    // Select should return that the fd is read for reading.
    FD_ZERO(&set);
    FD_SET(fence.getFd(), &set);

    ASSERT_EQ(select(fence.getFd() + 1, &set, nullptr, nullptr, &time), 1);
    ASSERT_TRUE(FD_ISSET(fence.getFd(), &set));
}

TEST(FenceTest, OneTimelineMerge) {
    SyncTimeline timeline;
    ASSERT_TRUE(timeline.isValid());

    // create fence a,b,c and then merge them all into fence d.
    SyncFence a(timeline, 1), b(timeline, 2), c(timeline, 3);
    ASSERT_TRUE(a.isValid());
    ASSERT_TRUE(b.isValid());
    ASSERT_TRUE(c.isValid());

    SyncFence d({a,b,c});
    ASSERT_TRUE(d.isValid());

    // confirm all fences have one active point (even d).
    ASSERT_EQ(a.getActiveCount(), 1);
    ASSERT_EQ(b.getActiveCount(), 1);
    ASSERT_EQ(c.getActiveCount(), 1);
    ASSERT_EQ(d.getActiveCount(), 1);

    // confirm that d is not signaled until the max of a,b,c
    timeline.inc(1);
    ASSERT_EQ(a.getSignaledCount(), 1);
    ASSERT_EQ(d.getActiveCount(), 1);

    timeline.inc(1);
    ASSERT_EQ(b.getSignaledCount(), 1);
    ASSERT_EQ(d.getActiveCount(), 1);

    timeline.inc(1);
    ASSERT_EQ(c.getSignaledCount(), 1);
    ASSERT_EQ(d.getActiveCount(), 0);
    ASSERT_EQ(d.getSignaledCount(), 1);
}

TEST(FenceTest, MergeSameFence) {
    SyncTimeline timeline;
    ASSERT_TRUE(timeline.isValid());

    SyncFence fence(timeline, 5);
    ASSERT_TRUE(fence.isValid());

    SyncFence selfMergeFence(fence, fence);
    ASSERT_TRUE(selfMergeFence.isValid());

    ASSERT_EQ(selfMergeFence.getSignaledCount(), 0);

    timeline.inc(5);
    ASSERT_EQ(selfMergeFence.getSignaledCount(), 1);
}

TEST(FenceTest, PollOnDestroyedTimeline) {
    SyncTimeline timeline;
    ASSERT_TRUE(timeline.isValid());

    SyncFence fenceSig(timeline, 100);
    SyncFence fenceKill(timeline, 200);

    // Spawn a thread to wait on a fence when the timeline is killed.
    thread waitThread{
        [&]() {
            ASSERT_EQ(timeline.inc(100), 0);

            // Wait on the fd.
            struct pollfd fds;
            fds.fd = fenceKill.getFd();
            fds.events = POLLIN | POLLERR;
            ASSERT_EQ(poll(&fds, 1, 0), 0);
        }
    };

    // Wait for the thread to spool up.
    fenceSig.wait();

    // Kill the timeline.
    timeline.destroy();

    // wait for the thread to clean up.
    waitThread.join();
}

TEST(FenceTest, MultiTimelineWait) {
    SyncTimeline timelineA, timelineB, timelineC;

    SyncFence fenceA(timelineA, 5);
    SyncFence fenceB(timelineB, 5);
    SyncFence fenceC(timelineC, 5);

    // Make a larger fence using 3 other fences from different timelines.
    SyncFence mergedFence({fenceA, fenceB, fenceC});
    ASSERT_TRUE(mergedFence.isValid());

    // Confirm fence isn't signaled
    ASSERT_EQ(mergedFence.getActiveCount(), 3);
    ASSERT_EQ(mergedFence.wait(0), -1);
    ASSERT_EQ(errno, ETIME);

    timelineA.inc(5);
    ASSERT_EQ(mergedFence.getActiveCount(), 2);
    ASSERT_EQ(mergedFence.getSignaledCount(), 1);

    timelineB.inc(5);
    ASSERT_EQ(mergedFence.getActiveCount(), 1);
    ASSERT_EQ(mergedFence.getSignaledCount(), 2);

    timelineC.inc(5);
    ASSERT_EQ(mergedFence.getActiveCount(), 0);
    ASSERT_EQ(mergedFence.getSignaledCount(), 3);

    // confirm you can successfully wait.
    ASSERT_EQ(mergedFence.wait(100), 0);
}

TEST(StressTest, TwoThreadsSharedTimeline) {
    const int iterations = 1 << 16;
    int counter = 0;
    SyncTimeline timeline;
    ASSERT_TRUE(timeline.isValid());

    // Use a single timeline to synchronize two threads
    // hammmering on the same counter.
    auto threadMain = [&](int threadId) {
        for (int i = 0; i < iterations; i++) {
            SyncFence fence(timeline, i * 2 + threadId);
            ASSERT_TRUE(fence.isValid());

            // Wait on the prior thread to complete.
            ASSERT_EQ(fence.wait(), 0);

            // Confirm the previous thread's writes are visible and then inc.
            ASSERT_EQ(counter, i * 2 + threadId);
            counter++;

            // Kick off the other thread.
            ASSERT_EQ(timeline.inc(), 0);
        }
    };

    thread a{threadMain, 0};
    thread b{threadMain, 1};
    a.join();
    b.join();

    // make sure the threads did not trample on one another.
    ASSERT_EQ(counter, iterations * 2);
}

class ConsumerStressTest : public ::testing::TestWithParam<int> {};

TEST_P(ConsumerStressTest, MultiProducerSingleConsumer) {
    mutex lock;
    int counter = 0;
    int iterations = 1 << 12;

    vector<SyncTimeline> producerTimelines(GetParam());
    vector<thread> threads;
    SyncTimeline consumerTimeline;

    // Producer threads run this lambda.
    auto threadMain = [&](int threadId) {
        for (int i = 0; i < iterations; i++) {
            SyncFence fence(consumerTimeline, i);
            ASSERT_TRUE(fence.isValid());

            // Wait for the consumer to finish. Use alternate
            // means of waiting on the fence.
            if ((iterations + threadId) % 8 != 0) {
                ASSERT_EQ(fence.wait(), 0);
            }
            else {
                while (fence.getSignaledCount() != 1) {
                    ASSERT_EQ(fence.getErrorCount(), 0);
                }
            }

            // Every producer increments the counter, the consumer checks + erases it.
            lock.lock();
            counter++;
            lock.unlock();

            ASSERT_EQ(producerTimelines[threadId].inc(), 0);
        }
    };

    for (int i = 0; i < GetParam(); i++) {
        threads.push_back(thread{threadMain, i});
    }

    // Consumer thread runs this loop.
    for (int i = 1; i <= iterations; i++) {
        // Create a fence representing all producers final timelines.
        vector<SyncFence> fences;
        for (auto& timeline : producerTimelines) {
            fences.push_back(SyncFence(timeline, i));
        }
        SyncFence mergeFence(fences);
        ASSERT_TRUE(mergeFence.isValid());

        // Make sure we see an increment from every producer thread. Vary
        // the means by which we wait.
        if (iterations % 8 != 0) {
            ASSERT_EQ(mergeFence.wait(), 0);
        }
        else {
            while (mergeFence.getSignaledCount() != mergeFence.getSize()) {
                ASSERT_EQ(mergeFence.getErrorCount(), 0);
            }
        }
        ASSERT_EQ(counter, GetParam()*i);

        // Release the producer threads.
        ASSERT_EQ(consumerTimeline.inc(), 0);
    }

    for_each(begin(threads), end(threads), [](thread& thread) { thread.join(); });
}
INSTANTIATE_TEST_CASE_P(
    ParameterizedStressTest,
    ConsumerStressTest,
    ::testing::Values(2,4,16));

class MergeStressTest : public ::testing::TestWithParam<tuple<int, int>> {};

template <typename K, typename V> using dict = unordered_map<K,V>;

TEST_P(MergeStressTest, RandomMerge) {
    int timelineCount = get<0>(GetParam());
    int mergeCount = get<1>(GetParam());

    vector<SyncTimeline> timelines(timelineCount);

    default_random_engine generator;
    uniform_int_distribution<int> timelineDist(0, timelines.size()-1);
    uniform_int_distribution<int> syncPointDist(0, numeric_limits<int>::max());

    SyncFence fence(timelines[0], 0);
    ASSERT_TRUE(fence.isValid());

    unordered_map<int, int> fenceMap;
    fenceMap.insert(make_pair(0, 0));

    // Randomly create syncpoints out of a fixed set of timelines, and merge them together.
    for (int i = 0; i < mergeCount; i++) {

        // Generate syncpoint.
        int timelineOffset = timelineDist(generator);
        const SyncTimeline& timeline = timelines[timelineOffset];
        int syncPoint = syncPointDist(generator);

        // Keep track of the latest syncpoint in each timeline.
        auto itr = fenceMap.find(timelineOffset);
        if (itr == end(fenceMap)) {
            fenceMap.insert(make_pair(timelineOffset, syncPoint));
        }
        else {
            int oldSyncPoint = itr->second;
            fenceMap.erase(itr);
            fenceMap.insert(make_pair(timelineOffset, max(syncPoint, oldSyncPoint)));
        }

        // Merge.
        fence = SyncFence(fence, SyncFence(timeline, syncPoint));
        ASSERT_TRUE(fence.isValid());
    }

    // Confirm our map matches the fence.
    ASSERT_EQ(fence.getSize(), fenceMap.size());

    // Trigger the merged fence.
    for (auto& item: fenceMap) {
        ASSERT_EQ(fence.wait(0), -1);
        ASSERT_EQ(errno, ETIME);

        // Increment the timeline to the last syncpoint.
        timelines[item.first].inc(item.second);
    }

    // Check that the fence is triggered.
    ASSERT_EQ(fence.wait(0), 0);
}

INSTANTIATE_TEST_CASE_P(
    ParameterizedMergeStressTest,
    MergeStressTest,
    ::testing::Combine(::testing::Values(16,32), ::testing::Values(32, 1024, 1024*32)));

}

