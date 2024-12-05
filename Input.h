#pragma once

#include <Windows.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <cassert>
#include "WinApp.h"
#include <wrl.h>

class Input
{
public:

	template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

	void Initialize(WinApp* winApp);

	void Update();

	bool PushKey(BYTE keyNumber);

	bool TriggerKey(BYTE keyNumber);

private:

	BYTE keyPre[256] = {};
	BYTE key[256] = {};

	ComPtr<IDirectInputDevice8> keyboard;

	IDirectInput8* directInput = nullptr;

	HRESULT result;

	WinApp* winApp = nullptr;
};

