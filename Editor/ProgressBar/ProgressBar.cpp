#include "stdafx.h"
#include "ProgressBar.h"
#include "ProgressReport.h"


void ProgressBar::Begin()
{
	ProgressReport& report = ProgressReport::Get();
	bool bLoadingModel = report.IsLoading(ProgressReport::Model);//현재 로딩중인지 체크
	if (bLoadingModel)
	{
		progress = report.GetPercentage(ProgressReport::Model);
		status = report.GetStatus(ProgressReport::Model);
	}
	bVisible = bLoadingModel;
	
}

void ProgressBar::Render()
{
	if (!bVisible)
		return;
	

	ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoScrollbar | 	ImGuiWindowFlags_NoMove;
		
	ImGui::Begin("ProgressBar",nullptr, windowFlags);
	ImGui::SetWindowFocus();

	ImGui::PushItemWidth(500.0f - ImGui::GetStyle().WindowPadding.x*2.0f);

	ImGui::ProgressBar(progress, ImVec2(0, 0));
	ImGui::Text(status.c_str());
	ImGui::PopItemWidth();
	ImGui::End();
}

void ProgressBar::Read()
{
}
