#ifndef THREADPOOL_H_
#define THREADPOOL_H_

#include <atomic>
#include <type_traits>
#include <vector>
#include <queue>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <utility>
#include <memory>
#include <stdexcept>
namespace tp{
    class Threadpool{
    private:
        typedef std::function<void()> task_type;
    public:
        explicit Threadpool(int n=0);
        ~Threadpool(){
            stop_.store(true);          //工作线程停止接受任务
            cond_.notify_all();
            for(auto &t:threads_){      //等待所有工作线程将任务队列里的任务执行完
                if(t.joinable())
                    t.join();
            }
        }
        template <typename Function,typename... Args>
        std::future<typename std::result_of<Function(Args...)>::type >  //std::result_of得到Function返回值的类型
        submit(Function&&,Args&&...);

    private:
        std::atomic<bool>        stop_;
        std::mutex               mtx_;
        std::condition_variable  cond_;
        std::queue<task_type>    tasks_;
        std::vector<std::thread> threads_;
    };
    Threadpool::Threadpool(int n) :stop_(false){
        int nthreads = n;
        if(nthreads<=0){                                            //得到可用硬件线程数（默认最小线程数=2）
            nthreads = std::thread::hardware_concurrency();
            if(nthreads==0)
                nthreads=2;
        }
        for(int i=0;i<nthreads;++i){
            threads_.push_back(std::thread(                         /*开启nthreads个线程*/
                    [this]{
                        while(!this->stop_){                        /*循环（若线程池处于工作状态，从任务队列中取出1个任务）*/
                            task_type task;
                            {
                                std::unique_lock<std::mutex> ulk(this->mtx_);

                                this->cond_.wait(ulk,[this]{ return this->stop_.load()||!this->tasks_.empty();}); /*先放弃锁，直到线程池开始析构或者任务队列里有任务*/
                                if(this->stop_&&this->tasks_.empty()) return;                                     /*若线程池开始析构，且任务队列里的任务全部执行完，结束工作线程*/
                                task = std::move(this->tasks_.front());                 /*从工作队列里取出1个任务*/
                                this->tasks_.pop();                                     /*调整工作队列*/
                            }
                            task();                                                     /*执行从工作队列中取出的任务*/
                        }
                    }
                    ));
        }
    }
    template <typename Function,typename... Args>
    std::future<typename std::result_of<Function(Args...)>::type >                      /*future的类型为Function函数对象的返回类型*/
    Threadpool::submit(Function && fcn, Args &&... args) {
        typedef typename std::result_of<Function(Args...)>::type return_type;
        typedef std::packaged_task<return_type ()>task;                                 /*包装函数，返回值类似function，可转换为futrue对象*/
        auto t = std::make_shared<task>(std::bind(std::forward<Function>(fcn),std::forward<Args>(args)...));
        auto ret = t->get_future();                                                     /*将packaged_task对象，转换为futrue对象*/
        {
            std::lock_guard<std::mutex> lg(mtx_);
            if(stop_.load())
                throw  std::runtime_error("thread pool has stoped!");
            tasks_.emplace([t]{(*t)();});                                               /*将packaged_task（类function对象）添加到任务队列*/
        }
        cond_.notify_one();                                                             /*通知1个工作线程，处理加入的任务*/
        return ret;
    }
}






#endif