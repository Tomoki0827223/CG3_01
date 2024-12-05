#pragma once
#include <Windows.h>
#include <cstdint>
#include <wrl.h>
#include <WinUser.h>



class WinApp
{
public:
	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparm, LPARAM lparam);

	void Initialize();
	void Update();
	void Finalize();

	static const int32_t kClientWidth = 1280;
	static const int32_t kClientHeight = 720;

	HWND GetHwnd() const { return hwnd; }
	HINSTANCE GetInstance() const { return wc.hInstance; }

	bool ProsessMeassage();

private:

	HWND hwnd = nullptr;
	WNDCLASS wc{};
};

