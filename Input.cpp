#pragma comment(lib,"dxguid.lib")
#pragma comment(lib,"dinput8.lib")

#include "Input.h"
//#include <dinput.h>
//#define DIRECTINPUT_VERSION 0x0800

void Input::Initialize(WinApp* winApp)
{
	this->winApp = winApp;
	

	ComPtr<IDirectInput8> directinput = nullptr;
	result = DirectInput8Create(winApp->GetInstance(), DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&directinput, nullptr);
	assert(SUCCEEDED(result));

	ComPtr<IDirectInputDevice8> keybord;
	result = directinput->CreateDevice(GUID_SysKeyboard, &keyboard, NULL);
	assert(SUCCEEDED(result));

	result = keyboard->SetDataFormat(&c_dfDIKeyboard);
	assert(SUCCEEDED(result));

	result = keyboard->SetCooperativeLevel(winApp->GetHwnd(), DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);
	assert(SUCCEEDED(result));

}

void Input::Update()
{

	memcpy(keyPre, key, sizeof(key));

	keyboard->Acquire();
	keyboard->GetDeviceState(sizeof(key), key);

}

bool Input::PushKey(BYTE keyNumber)
{
	if (key[keyNumber])
	{
		return true;
	}

	return false;
}

bool Input::TriggerKey(BYTE keyNumber)
{
	if (keyPre[keyNumber] == 0 && key[keyNumber])
	{

		return true;
	}

	return false;
}

