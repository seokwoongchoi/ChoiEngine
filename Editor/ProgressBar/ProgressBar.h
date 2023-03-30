#pragma once
class ProgressBar
{
public:
	ProgressBar() :status("")
		, progress(0.0f)
		, title("Hold On...")
		, xMin(500.0f)
		, xMax(0.0f)
		, yMin(83.0f)
		, yMax(0.0f)
		, height(0.0f)
		, bVisible(false)
	{}
	~ProgressBar() = default;

	ProgressBar(const ProgressBar&) = delete;
	ProgressBar& operator=(const ProgressBar&) = delete;

	void Begin();
	void Render();

	void Read() ;
private:
	std::string title;
	float xMin;
	float xMax;
	float yMin;
	float yMax;
	float height;
	
	bool bVisible;
	string status;
	float progress;
	
};

