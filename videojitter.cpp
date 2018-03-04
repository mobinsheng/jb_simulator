#include "videojitter.h"
#include "smooth.h"
#include <stdio.h>


VideoJitter::VideoJitter()
{
    total_frames = 0;
    fp_log = fopen("/home/mbs/jitter.csv","wb");
    fprintf(fp_log,"instant_delay,depth,instant_delay,delay\n");
}

VideoJitter::~VideoJitter(){
    fflush(fp_log);
    fclose(fp_log);
}

void VideoJitter::Push(FrameInfo *info){

    CalNetDelay(info);
    CalDataDelay(info);
    CalDepth2(info);
    Log();
    ++total_frames;
    prev_frame = *info;
}

void VideoJitter::Pop(FrameInfo *info){

}

bool VideoJitter::IsReadable(){

}

double VideoJitter::GetDepthSmoothFactor(){
    double factor = 0.995;
    // 计算当前的data delay，用来决定depth下降的速度
    int64_t diff = depth_info.depth - net_delay_info.delay;
    if(diff > 500){
        factor = 0.98;//0.96;
    }
    else if(diff > 400){
        factor = 0.985;//0.97;
    }
    else if(diff > 300){
        factor = 0.99;//0.98;
    }
    else if(diff > 200){
        factor = 0.995;//0.99;
    }
    else if(diff > 150){
        factor = 0.998;
    }
    else{
        factor = 0.999;//0.995;
    }
    AdjustDepthFactor(factor);
    return factor;
}

double VideoJitter::AdjustDepthFactor(double& factor){
    int trend = statistics_info.GetChangeTrend();
    switch (trend) {
    case StatisticsInfo::kChangeDownSlow:
        factor += 0.01;
        break;
    case StatisticsInfo::kChangeDownFast:
        factor -= 0.01;
        break;
    case StatisticsInfo::kChangeDownStable:
        break;
    case StatisticsInfo::kChangeUpSlow:
        factor +=0.01 ;
        break;
    case StatisticsInfo::kChangeUpFast:
        factor += 0.02;
        break;
    case StatisticsInfo::kChangeUpStable:
        break;
    case StatisticsInfo::kChangeStable:
        break;
    default:
        break;
    }
    return factor;
}

double VideoJitter::GetNetDelaySmoothFactor(){
    double factor = 0.995;
    int64_t diff = net_delay_info.delay - net_delay_info.instant_delay;
    if(diff > 500){
        factor = 0.99;//0.96;
    }
    else if(diff > 400){
        factor = 0.992;//0.97;
    }
    else if(diff > 300){
        factor = 0.994;//0.98;
    }
    else if(diff > 200){
        factor = 0.996;//0.99;
    }
    else if(diff > 150){
        factor = 0.998;
    }
    else{
        factor = 0.999;//0.995;
    }
    factor = 0.995;
    return factor;
}

void VideoJitter::CalNetDelay(FrameInfo *info){

    net_delay_info.prev_delay = net_delay_info.delay;
    if(total_frames > 0){
        int64_t lastDiff = prev_frame.recv_timestamp - prev_frame.send_timestamp;
        int64_t curDiff = info->recv_timestamp - info->send_timestamp;
        int64_t diff = curDiff - lastDiff;
        net_delay_info.instant_delay +=diff;
        if(net_delay_info.delay < net_delay_info.instant_delay){
            net_delay_info.delay = net_delay_info.instant_delay;
        }
        else{
            double factor = net_delay_info.factor;
            net_delay_info.delay = factor * net_delay_info.delay +
                    (1 - factor) * net_delay_info.instant_delay;
        }
    }
    else{
        net_delay_info.instant_delay = 0;
        net_delay_info.delay = 0;
    }
}

void VideoJitter::CalDataDelay(FrameInfo *info){

}

// 基本算法
void VideoJitter::CalDepth1(FrameInfo *info){
    depth_info.instant_depth = net_delay_info.delay;

    if(depth_info.depth < depth_info.instant_depth){
        depth_info.depth = depth_info.instant_depth;
    }
    else {
        double factor = GetDepthSmoothFactor();
        depth_info.depth = factor * depth_info.depth +
                (1 - factor) * depth_info.instant_depth;
    }
}

// 在基本算法上平滑
void VideoJitter::CalDepth2(FrameInfo *info){
    CalDepth1(info);

    depth_info.smooth_queue.push_back(depth_info.depth);
    depth_info.depth_sum += depth_info.depth;
    if(depth_info.smooth_queue.size() > 15){

        depth_info.depth_sum -= depth_info.smooth_queue[0];
        depth_info.smooth_queue.pop_front();
    }
    depth_info.depth = depth_info.depth_sum / depth_info.smooth_queue.size();

    if(depth_info.depth < depth_info.instant_depth){
        depth_info.depth = depth_info.instant_depth;
    }

}

// 在基本算法上再次平滑
void VideoJitter::CalDepth3(FrameInfo *info){
    CalDepth1(info);


    depth_info.smooth_queue.push_back(depth_info.depth);
    depth_info.depth_sum += depth_info.depth;
    if(depth_info.smooth_queue.size() > 15){

        depth_info.depth_sum -= depth_info.smooth_queue[0];
        depth_info.smooth_queue.pop_front();
    }
    depth_info.depth = depth_info.depth_sum / depth_info.smooth_queue.size();

    depth_info.smooth_queue2.push_back(depth_info.depth);
    depth_info.depth_sum2 += depth_info.depth;
    if(depth_info.smooth_queue2.size() > 15){

        depth_info.depth_sum2 -= depth_info.smooth_queue2[0];
        depth_info.smooth_queue2.pop_front();
    }
    depth_info.depth = depth_info.depth_sum2 / depth_info.smooth_queue2.size();


    //===
    /*depth_info.in_depth_array->Push(depth_info.depth);
    size_t size = depth_info.in_depth_array->Size();
    if(size >= DepthInfo::kArraySize){
        depth_info.in_depth_array->Pop();
    }
    cubicSmooth5(depth_info.in_depth_array->Data(),depth_info.out_depth_array,size);
    depth_info.depth = depth_info.out_depth_array[size -1];*/
    //==

    if(depth_info.depth < depth_info.instant_depth){
        depth_info.depth = depth_info.instant_depth;
    }
}

void VideoJitter::Log(){
    fprintf(fp_log,"%lld,%lld,%lld,%lld\n",depth_info.instant_depth,depth_info.depth,net_delay_info.instant_delay,net_delay_info.delay);
}

//============================================
FrameInfo* CreateFrameInfo(){
    static int64_t duration = 0;
    static int64_t start_time = clock() / CLOCKS_PER_SEC;

    duration += 50;
    FrameInfo* info = new FrameInfo;
    info->send_timestamp = start_time + duration;
    return info;
}


void* SendThread(void* param){
    Transmission* trans = (Transmission*)param;
    while (true) {
        FrameInfo* info = CreateFrameInfo();
        trans->SendFrame(info);
        if(trans->IsProcessFinish()){
            break;
        }
        usleep(10 );
    }
    return 0;
}

static void* RecvThread(void* param){
    Transmission* trans = (Transmission*)param;
    while(true){
        FrameInfo* info = trans->RecvFrame();

        trans->AdjustRandom();
        int num = 0;
        while(num <= 0){
            num = trans->GetRandomNum();
        }
        info->recv_timestamp = info->send_timestamp + 40 + num;

        trans->jitter.Push(info);
        delete info;

        if(trans->IsProcessFinish()){
            break;
        }

    }
}

static void* RenderThread(void* param){
    Transmission* trans = (Transmission*)param;
    while(true){
        FrameInfo* info = trans->RecvFrame();

        trans->AdjustRandom();
        info->recv_timestamp = info->send_timestamp + trans->GetRandomNum();

        trans->jitter.Push(info);
        delete info;

        if(trans->IsProcessFinish()){
            break;
        }

    }
}

void RunSimulator(int frames){
    pthread_t t1,t2;
    Transmission transmission;
    int mode = 0;//0,RandomGenerator::kRandomDistribution(1)
    transmission.SetRandomMode(mode);
    transmission.SetRandomDistributionParam(0,1000);
    transmission.SetNormalDistributionParam(700,300);
    transmission.SetMaxFrames(frames);

    pthread_create(&t1,NULL,SendThread,&transmission);
    pthread_create(&t2,NULL,RecvThread,&transmission);

    pthread_join(t1,NULL);
    pthread_join(t2,NULL);
}
