#pragma once

class GameTimer {
public:
	GameTimer();

	float TotalTime()const;
	float DeltaTime()const;
	bool IsStoped();

	void Reset();
	void Start();
	void Stop();
	void Tick();

private:
	double mSecondsPerCount;
	double mDeltaTime;

	__int64 mBaseTime;
	__int64 mPauseTime;
	__int64 mStopTime;
	__int64 mPrevTime;
	__int64 mCurrentTime;

	bool isStoped;

};
