#ifndef _AUTOLOCK_HEADER_H_
#define _AUTOLOCK_HEADER_H_

#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>

#define AUTOLOCK(mtx) std::lock_guard<std::mutex> lg(mtx)

#define AUTOLOCK_1(mtx) std::lock_guard<std::mutex> lg_1(mtx)
#define AUTOLOCK_2(mtx) std::lock_guard<std::mutex> lg_2(mtx)
#define AUTOLOCK_3(mtx) std::lock_guard<std::mutex> lg_3(mtx)

#define WAIT(cond, mtx) std::unique_lock<std::mutex> ul(mtx); cond.wait(ul)
#define WAIT_TILL_COND(cond, mtx, pred) std::unique_lock<std::mutex> ul(mtx); cond.wait(ul, pred)
#define WAIT_MINUTES(cond, mtx, mins) std::unique_lock<std::mutex> ul(mtx); cond.wait_for(ul, std::chrono::minutes(mins))
#define WAIT_SECONDS(cond, mtx, secs) std::unique_lock<std::mutex> ul(mtx); cond.wait_for(ul, std::chrono::seconds(secs))
#define WAIT_MILLISEC(cond, mtx, msec) std::unique_lock<std::mutex> ul(mtx); cond.wait_for(ul, std::chrono::milliseconds(msec))
#define WAIT_MICROSEC(cond, mtx, csec) std::unique_lock<std::mutex> ul(mtx); cond.wait_for(ul, std::chrono::microseconds(csec))

#define WAIT_MINUTES_TILL_COND(cond, mtx, mins, pred) std::unique_lock<std::mutex> ul(mtx); cond.wait_for(ul, std::chrono::minutes(mins), pred)
#define WAIT_SECONDS_TILL_COND(cond, mtx, secs, pred) std::unique_lock<std::mutex> ul(mtx); cond.wait_for(ul, std::chrono::seconds(secs), pred)
#define WAIT_MILLISEC_TILL_COND(cond, mtx, msec, pred) std::unique_lock<std::mutex> ul(mtx); cond.wait_for(ul, std::chrono::milliseconds(msec), pred)
#define WAIT_MICROSEC_TILL_COND(cond, mtx, csec, pred) std::unique_lock<std::mutex> ul(mtx); cond.wait_for(ul, std::chrono::microseconds(csec), pred)

#define WAKEUP_ONE(cond) cond.notify_one()
#define WAKEUP_ALL(cond) cond.notify_all()

#define WAIT_TO_EXIT(thread) if (thread.joinable()) thread.join()

#endif

