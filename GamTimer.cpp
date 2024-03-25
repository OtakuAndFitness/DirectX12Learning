#include <Windows.h>
#include "GameTimer.h"

GameTimer::GameTimer() : mSecondsPerCount(0.0), mDeltaTime(-1.0), mBaseTime(0), mPauseTime(0), mStopTime(0), mPrevTime(0), mCurrentTime(0), isStoped(false) {
	__int64 countsPerSec;
	QueryPerformanceFrequency((LARGE_INTEGER*)&countsPerSec);
	mSecondsPerCount = 1.0 / (double)countsPerSec;
}

void GameTimer::Tick() {
	if (isStoped) {
		mDeltaTime = 0.0;
		return;
	}

	__int64 currentTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&currentTime);
	mCurrentTime = currentTime;

	mDeltaTime = (mCurrentTime - mPrevTime) * mSecondsPerCount;

	mPrevTime = mCurrentTime;

	if (mDeltaTime < 0.0) {
		mDeltaTime = 0;
	}
}

float GameTimer::DeltaTime()const {
	return (float)mDeltaTime;
}

void GameTimer::Reset() {
	__int64 currTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

	mBaseTime = currTime;
	mPrevTime = currTime;
	mStopTime = 0;
	isStoped = false;
}

void GameTimer::Stop() {
	if (!isStoped) {
		__int64 currTime;
		QueryPerformanceCounter((LARGE_INTEGER*)&currTime);
		mStopTime = currTime;
		isStoped = true;
	}
}

void GameTimer::Start() {
	__int64 startTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&startTime);
	if (isStoped) {
		mPauseTime += (startTime - mStopTime);
		mPrevTime = startTime;
		mStopTime = 0;
		isStoped = false;
	}

}

float GameTimer::TotalTime()const {
	if (isStoped) {
		return (float)((mStopTime - mPauseTime - mBaseTime) * mSecondsPerCount);
	}
	else {
		return (float)((mCurrentTime - mPauseTime - mBaseTime) * mSecondsPerCount);
	}
}

bool GameTimer::IsStoped() {
	return isStoped;
}