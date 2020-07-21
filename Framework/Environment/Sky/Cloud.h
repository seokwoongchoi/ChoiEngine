#pragma once
class Cloud
{
public:
	explicit Cloud();
	~Cloud();
	Cloud(const Cloud&) = delete;
	Cloud& operator=(const Cloud&) = delete;

	void Render();
	void Update();
};

