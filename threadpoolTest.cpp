#include <mutex>
#include <vector>
#include <future>
#include <iostream>
#include <algorithm>
#include "Threadpool.h"
int main(){
    std::mutex mtx;
    try{
        tp::Threadpool tp1(5);
        std::vector<std::future<int>> v;
        std::vector<std::future<void>> v1;
        for(int i=0;i<10;++i){
            auto ans = tp1.submit([](int answer){return answer;},i);                        /*添加10个任务到任务队列*/
            v.push_back(std::move(ans));
        }
        for(int i=0;i<5;++i){
            auto ans = tp1.submit([&mtx](const std::string& str1,const std::string& str2){  /*添加5个任务到任务队列，使用流前要上锁*/
                std::lock_guard<std::mutex> lg(mtx);
                std::cout<<(str1+str2)<<std::endl;
                return ;
            },"hello",std::to_string(i));
        }
        for(int i=0;i<v.size();++i){
            //std::lock_guard<std::mutex>lg(mtx);
            std::cout<<v[i].get()<<std::endl;
        }
    }catch(std::exception& e){
        std::cout<<e.what()<<std::endl;
    }

    return 0;

}
