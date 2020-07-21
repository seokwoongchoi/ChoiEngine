#pragma once
#include "Framework.h"

struct Progress
{
	Progress() { Clear(); }

	void Clear()
	{
		status.clear();
		jobsDone = 0;
		jobCount = 0;
		bLoading = false;

	}
	std::string status;

	int jobsDone;
	int jobCount;
	bool bLoading;

};
class ProgressReport final
{
public:
	static constexpr int Model = 0;
public:
	static ProgressReport& Get()
	{
		static ProgressReport instance;
		return instance;

	}
	const std::string GetStatus(const int& progressID)
	{
		return reports[progressID].status;
	}

	const float GetPercentage(const int& progressID)
	{
		float job = static_cast<float>(reports[progressID].jobsDone);
		float count = static_cast<float>(reports[progressID].jobCount);

		return job / count;

	}

	bool IsLoading(const int& progressID)
	{
		return reports[progressID].bLoading;
	}
	void SetStatus(const int& progressID, const std::string& status)
	{
		reports[progressID].status = status;
	}

	void SetJobsDone(const int& progressID, const int& jobsDone)
	{
		auto& report = reports[progressID];
		report.jobsDone = jobsDone;

		if (report.jobsDone >= report.jobCount)
			report.bLoading = false;
	}

	void SetJobCount(const int& progressID, const int& jobCount)
	{
		auto& report = reports[progressID];

		report.jobCount = jobCount;
		report.bLoading = true; //load½ÃÀÛ
	}
	void SetIsLoading(const int& progressID, const bool& bLoading)
	{
		reports[progressID].bLoading = bLoading;

	}

	void IncrementJobsDone(const int& progressID)
	{
		auto& report = reports[progressID];

		 report.jobsDone++ ;

		if (report.jobsDone >= report.jobCount)
			report.bLoading = false;

	}

	void Reset(const int& progressID)
	{
		reports[progressID].Clear();
	}
private:
	std::map<int, Progress> reports;
private:
	ProgressReport()
	{
		reports[Model] = Progress();
	}
	~ProgressReport() = default;

	ProgressReport(const ProgressReport&) = delete;
	ProgressReport& operator=(const ProgressReport&) = delete;
};