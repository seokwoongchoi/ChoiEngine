#include "stdafx.h"

#include "Core/Engine.h"

#include "./Core/Window.h"


int APIENTRY WinMain
(
	HINSTANCE hInstance,
	HINSTANCE prevInstance,
	LPSTR lpszCmdParam,
	int nCmdShow
)
{

	{
		Window::Create(hInstance, L"D3D11Game", 1280, 720, false);
		Window::Show();
		D3DDesc desc;
		desc.AppName = Window::AppName;
		desc.Instance = Window::Instance;
		desc.bFullScreen = Window::IsFullScreen;
		desc.bVsync = false;
		desc.Handle = Window::Handle;
		desc.Width = Window::GetWidth();
		desc.Height = Window::GetHeight();
	
		D3D::SetDesc(desc);
	}


	{
		Thread::Create();
		Time::Create();
		Time::Get()->Start();
		Keyboard::Create();
		Mouse::Create();
	}
	
	auto engine = std::make_unique<Engine>();


	engine->Load();
	Window::Resize = [&engine](const uint& width, const uint& height)
	{
		engine->Resize(width, height);
	};
	MSG msg = { 0 };
	
	while (true)
	{

		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				break;
			}

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			//Update
			{
				Keyboard::Get()->Update();
				Mouse::Get()->Update();
				Time::Get()->Update();
				engine->Update();
			}
			
			engine->Begin();
			engine->Render();
			engine->Present();
		}

	}

	Mouse::Delete();
	Keyboard::Delete();
	Time::Delete();
	Thread::Delete();
	Window::Destroy();
	return 0;
}