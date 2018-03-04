#ifndef VIDEOJITTER_H
#define VIDEOJITTER_H

#include <deque>
#include <pthread.h>
#include <time.h>
#include <stdint.h>
#include <memory>
#include <random>
#include <chrono>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include "random_generator.h"
#include "sync_queue.h"
#include "simple_lock.h"

using namespace std;

struct FrameInfo{
    int64_t send_timestamp;
    int64_t recv_timestamp;
};


struct NetDelayInfo{
    int64_t instant_delay;
    int64_t delay;
    int64_t prev_delay;
    double factor;
    deque<int64_t> smooth_queue;
    int64_t delay_sum;
    NetDelayInfo(){
        factor = 0.995;
        instant_delay = 0;
        delay = 0;
        prev_delay = 0;
        delay_sum = 0;
    }
};

#include "simple_array.h"
struct DepthInfo{
    int64_t instant_depth;
    int64_t depth;
    deque<int64_t> smooth_queue;
    int64_t depth_sum;
    deque<int64_t> smooth_queue2;
    int64_t depth_sum2;

    static const int kArraySize = 5;
    SimpleArray<double>* in_depth_array;
    double out_depth_array[kArraySize];



    DepthInfo(){
        instant_depth = 0;
        depth =0;
        depth_sum = 0;
        depth_sum2 = 0;
        in_depth_array = new SimpleArray<double>(kArraySize);
        memset(out_depth_array,0,sizeof(out_depth_array));
    }
    ~DepthInfo(){
        delete in_depth_array;
    }
};

struct DataDelayInfo{
    int64_t data_delay;

    DataDelayInfo(){
        data_delay = 0;
    }
};

struct StatisticsInfo{
    enum{
        kInvalid = -999,
        kRenderPeriod = 40, //  render的时间间隔（ms）
        kWinSize = 40, // 窗口（以帧为单位），在窗口之内计算，滑动窗口
        kSmallWinSize = 5, // 小窗口，窗口之内的小窗口，实时计算
    };

    enum{
        kPlane,
        kUp,
        kDown,

    };

    enum{
        kChangeDownSlow, // 缓慢下降
        kChangeDownFast, // 快速下降
        kChangeDownStable, // 平缓下降
        kChangeUpSlow, // 缓慢上升
        kChangeUpFast, // 快速上升
        kChangeUpStable, // 平缓上升
        kChangeStable, // 基本维持不变
    };

    int64_t cur_delay;
    int64_t prev_delay;
    int64_t min_delay;
    int64_t max_delay;
    int64_t avg_delay;
    int64_t delay_trend; // 变化趋势：下降还是上升
    double slope; // 窗口的斜率
    double prev_slope;
    double instant_slope;
    double prev_instant_slope;
    int64_t delay_sum;

    int64_t frame_count;

    deque<int64_t> delay_queue;

    StatisticsInfo(){

        Clean();
    }

    ~StatisticsInfo(){

    }

    int GetChangeTrend(){
        if(slope > 0.5){
            if(instant_slope > 0.5){
                return kChangeUpFast;
            }
            else if(instant_slope < -0.5){
                return kChangeUpSlow;
            }
            else{
                return kChangeUpStable;
            }
        }
        else if(slope < 0.5){
            if(instant_slope > 0.5){
                return kChangeDownSlow;
            }
            else if(instant_slope < -0.5){
                return kChangeDownFast;
            }
            else{
                return kChangeDownStable;
            }
        }
        else{
            return kChangeStable;
        }
    }

    void CollectStatistics(int64_t Delay){
        ++frame_count;

        delay_queue.push_back(Delay);
        if(delay_queue.size() > kWinSize){
            delay_queue.pop_front();
        }

        // smooth
        {
            prev_delay = cur_delay;
            cur_delay = Delay;
            if(min_delay == kInvalid){
                min_delay = Delay;
            }
            if(min_delay > Delay){
                min_delay = Delay;
            }
            if(max_delay == kInvalid){
                max_delay = Delay;
            }
            if(max_delay < Delay){
                max_delay = Delay;
            }
            delay_sum += Delay;
            avg_delay = delay_sum / frame_count;
            if(cur_delay > prev_delay + 10){
                delay_trend = kUp;
            }
            else if(cur_delay < prev_delay - 10){
                delay_trend = kDown;
            }

            prev_slope = slope;
            prev_instant_slope = instant_slope;

            size_t size =delay_queue.size();

            slope = (double)(delay_queue[size - 1] - delay_queue[0]) / (double)(size * kRenderPeriod);

            if(size >= kSmallWinSize){
                instant_slope = (double)(delay_queue[size - 1] - delay_queue[size - kSmallWinSize ]) / (double)(kSmallWinSize * kRenderPeriod);
            }
            else{
                instant_slope = (double)(delay_queue[size - 1] - delay_queue[0]) / (double)(size * kRenderPeriod);
            }
        }
        //====

    }



    void Clean(){
        cur_delay = kInvalid;
        prev_delay = kInvalid;
        min_delay = kInvalid;
        max_delay = kInvalid;
        avg_delay = kInvalid;
        delay_trend = kPlane; // 变化趋势：下降还是上升
        slope = 0;
        delay_sum = 0;
        instant_slope = 0;
        prev_slope = 0;
        prev_instant_slope = 0;

        frame_count = 0;
    }
};

class VideoJitter
{
public:
    VideoJitter();

    ~VideoJitter();

    void Push(FrameInfo* info);

    void Pop(FrameInfo* info);

    bool IsReadable();

    void CalNetDelay(FrameInfo* info);

    void CalDataDelay(FrameInfo* info);

    void CalDepth1(FrameInfo* info); // 最基本的算法

    void CalDepth2(FrameInfo* info); // 在CalDepth1的基础上用求平均值的方法进行平滑

    void CalDepth3(FrameInfo* info); // 在CalDepth2的基础上再次使用求平均值的方法平滑

    double GetDepthSmoothFactor();

    double AdjustDepthFactor(double& factor);

    double GetNetDelaySmoothFactor();

    void Log();

private:
    FrameInfo prev_frame;

    NetDelayInfo net_delay_info;
    DataDelayInfo data_delay_info;
    DepthInfo depth_info;
    StatisticsInfo statistics_info;

    int64_t total_frames;

    FILE* fp_log;
};

//===========================


class Transmission{
private:

    SyncQueue<FrameInfo*> frame_queue;
    RandomGenerator random_generator;

    int64_t total_frames;

    int64_t max_frames;

    SimpleLock lock;

public:
    VideoJitter jitter;

    Transmission(){

        total_frames = 0;
        max_frames = 400;

        random_generator.SetMode(RandomGenerator::kNormalDistribution);
        random_generator.SetNormalDistributionParam(700,400);

    }

    void SetRandomMode(int mode){
        random_generator.SetMode(mode);
    }

    void SetRandomDistributionParam(int low ,int up){
        random_generator.SetRandomDistributionParam(low,up);
    }

    void SetNormalDistributionParam(int avg,int mse){
        random_generator.SetNormalDistributionParam(avg,mse);
    }

    int64_t GetRandomNum(){
        return random_generator.RandomNum();
    }


    void SetMaxFrames(int64_t maxFrames){
        max_frames = maxFrames;
    }

    bool IsProcessFinish(){
        lock.Lock();
        bool v = total_frames >= max_frames;
        lock.Unlock();
        return v;
    }

    void SendFrame(FrameInfo* info){
        frame_queue.Push(info);
        lock.Lock();
        ++total_frames;
        lock.Unlock();
    }

    FrameInfo* RecvFrame(){
        return frame_queue.Pop();
    }

    void AdjustRandom(){
        lock.Lock();
        if(total_frames == 50){
            SetRandomMode(0);
            SetNormalDistributionParam(700,300);
        }
        if(total_frames == 100){
            SetRandomMode(0);
            SetNormalDistributionParam(500,200);
        }

        if(total_frames == 150){
            SetRandomMode(1);
            SetRandomDistributionParam(400,1000);
        }
        if(total_frames == 200){
            SetRandomMode(1);
            SetRandomDistributionParam(500,700);
        }

        if(total_frames == 250){
            SetRandomMode(0);
            SetNormalDistributionParam(700,300);
        }
        if(total_frames == 300){
            SetRandomMode(0);
            SetNormalDistributionParam(800,200);
        }

        if(total_frames == 350){
            SetRandomMode(1);
            SetRandomDistributionParam(500,1000);
        }
        if(total_frames == 400){
            SetRandomMode(1);
            SetRandomDistributionParam(500,700);
        }


        if(total_frames == 550){
            SetNormalDistributionParam(600,400);
            SetRandomMode(0);
        }

        if(total_frames == 800){
            SetNormalDistributionParam(700,300);
            SetRandomMode(0);
        }

        lock.Unlock();
    }
};

//===============================

void RunSimulator(int frames);

#endif // VIDEOJITTER_H
