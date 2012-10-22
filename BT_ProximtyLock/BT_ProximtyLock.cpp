// BT_ProximtyLock.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "btproxy.h"
#include <iostream>

Guard BluetoothProxy::INIT_LOCK;	// definition for Singleton's lock it's initialized before main() is invoked
BluetoothProxy* BluetoothProxy::m_pInstance = NULL;		// SINGLETON

BLUETOOTH_DEVICE_SEARCH_PARAMS SEARCH_PARAMS = {
	sizeof(BLUETOOTH_DEVICE_SEARCH_PARAMS),
	1,
	0, 
	1,	// returns unknown devices
	1,
	1,
	15,
	NULL
};

BLUETOOTH_DEVICE_INFO DEVICE_INFO =  {
	sizeof(BLUETOOTH_DEVICE_INFO),
	0
};

BluetoothProxy::BluetoothProxy() 
{
	m_Device = NULL;
}

BluetoothProxy * BluetoothProxy::GetInstance()
{
      if (m_pInstance == NULL)
      {
            EnterCriticalSection(&INIT_LOCK);
            if (m_pInstance == NULL) {
                  m_pInstance = new BluetoothProxy();
            }
            LeaveCriticalSection(&INIT_LOCK);
      }
      return m_pInstance;
}

/*
TODO Registery Settings
1) Read for Map device settings (stored as comma separate string)
2) If not exists create registery setting
3) Any time device added need to add to registery setting
4) Any time device removed need to remove from registery setting
http://msdn.microsoft.com/en-us/library/windows/desktop/ms724875(v=vs.85).aspx
*/
DWORD BluetoothProxy::HandleLastError(const char *msg) 
{
    DWORD errCode = GetLastError();

	LPTSTR errorText = NULL;

	FormatMessage(
	   // use system message tables to retrieve error text
	   FORMAT_MESSAGE_FROM_SYSTEM
	   // allocate buffer on local heap for error text
	   |FORMAT_MESSAGE_ALLOCATE_BUFFER
	   // Important! will fail otherwise, since we're not 
	   // (and CANNOT) pass insertion parameters
	   |FORMAT_MESSAGE_IGNORE_INSERTS,  
	   NULL,    // unused with FORMAT_MESSAGE_FROM_SYSTEM
	   errCode,
	   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
	   (LPTSTR)&errorText,  // output 
	   0, // minimum size for output buffer
	   NULL);   // arguments - see note 

	if ( errorText != NULL)
	{
	   wcout << msg << ": " << errorText << endl;
	   // release memory allocated by FormatMessage()
	   LocalFree(errorText);
	   errorText = NULL;
	}

	return errCode;
}

void BluetoothProxy::AddDevice(BLUETOOTH_DEVICE_INFO& deviceInfo)
{
	const wstring name(deviceInfo.szName);
	this->AddDevice(name);
}

void BluetoothProxy::AddDevice(const wstring& deviceName)
{
	map<wstring, int>::iterator it;
	it = m_DeviceMap.find(deviceName);

	if (it == m_DeviceMap.end())
    {
        EnterCriticalSection(&INIT_LOCK);
		it = m_DeviceMap.find(deviceName);
        if (it == m_DeviceMap.end()) {
			m_DeviceMap.insert(pair<wstring, int>(deviceName, 0));
        }
        LeaveCriticalSection(&INIT_LOCK);
    }
}

void BluetoothProxy::RemoveDevice(BLUETOOTH_DEVICE_INFO& deviceInfo)
{
	const wstring name(deviceInfo.szName);
	this->RemoveDevice(name);
}

void BluetoothProxy::RemoveDevice(const wstring& deviceName)
{
	map<wstring, int>::iterator it;
	it = m_DeviceMap.find(deviceName);

	if (it != m_DeviceMap.end()) 
	{
		EnterCriticalSection(&INIT_LOCK);
		it = m_DeviceMap.find(deviceName);
		if (it != m_DeviceMap.end()) {
			m_DeviceMap.erase(it);
		}
		LeaveCriticalSection(&INIT_LOCK);
	}
}

BOOL BluetoothProxy::IsProximityDevice(BLUETOOTH_DEVICE_INFO& deviceInfo)
{
	BOOL exists = FALSE;

	if (m_DeviceMap.empty()) {
		return exists;
	}

	map<wstring, int>::iterator it;
	it = m_DeviceMap.find(deviceInfo.szName);

	EnterCriticalSection(&INIT_LOCK);
	it = m_DeviceMap.find(deviceInfo.szName);
	if (it != m_DeviceMap.end()) {
		exists = true;
	}
	LeaveCriticalSection(&INIT_LOCK);

	return exists;
}

void BluetoothProxy::HandleState(BLUETOOTH_DEVICE_INFO& deviceInfo) 
{
	if (deviceInfo.fConnected) {
		// check if machine is locked if use show unlocked screen
	}
	else {
		// check if machine is unlocked if yes lock screen
	}
}

void BluetoothProxy::FindDevices() 
{
	FindDevices(SEARCH_PARAMS, DEVICE_INFO);
}

void BluetoothProxy::FindDevices(const BLUETOOTH_DEVICE_SEARCH_PARAMS& searchParams, BLUETOOTH_DEVICE_INFO& deviceInfo) 
{
	/*
	if (searchParams == NULL) {
		cout << "No search params provided - unable to find bluetooth devices." << endl;
		return;
	}
	*/

	this->m_Device = BluetoothFindFirstDevice(&searchParams, &deviceInfo);

	if (this->m_Device == NULL) {
		HandleLastError("Unable to find Bluetooth Devices");
		return;
	}

	do {
		if (!this->IsProximityDevice(deviceInfo)) {
			continue;
		}
		
		cout << "BT device is on proximity list." << endl;

		BOOL locked = this->IsWorkstationLocked();
		BOOL callLockScreen = false;

		if(locked && deviceInfo.fAuthenticated) {
			callLockScreen = true;
		}
		else if (!locked && !deviceInfo.fAuthenticated) {
			callLockScreen = true;
		}
		else {
			;;	// all is well
		}

		if (callLockScreen) {
			cout << "Proximity change detected - Lock screen will be invoked" << endl;
			if (!LockWorkStation()) {
				cout << "Failed to lock workstation" << endl;
			}
		}

	} while(BluetoothFindNextDevice(m_Device, &deviceInfo));
}

BOOL BluetoothProxy::IsWorkstationLocked() 
{
	BOOL locked = false;

	HDESK hdesk;
	int rtn;
	
	hdesk = OpenDesktop( _T("default"), 0, FALSE, DESKTOP_SWITCHDESKTOP);

	if (hdesk != NULL)
	{
		rtn = SwitchDesktop(hdesk);
		if (rtn == 0) {
			locked = true;
		}
		else {
			locked = false;
		}
		CloseDesktop(hdesk);
	}
	else {
		HandleLastError("Could not access the desktop");
		locked = false;
	}

	return locked;
}

int _tmain(int argc, _TCHAR* argv[])
{
	cout << "Testing BlueTooth Lock" << endl;

	BluetoothProxy *proxy = BluetoothProxy::GetInstance();

	if (proxy == NULL) {
		cout << "Unable to get BluetoothProxy instance." << endl;
		return 1;
	}

	proxy->AddDevice(_T("Nexus S 4G"));	// For testing

	while (TRUE) {
		proxy->FindDevices();
		Sleep(10000);
	}

	return 0;
}


/*
Should run as service
1) Get callback when any bluetooth action triggered (device paired, disconnected, etc.)
2) When callback received check if previous device was paired if yes and no longer connected lock machine
3) Need to store previously paired device identifier to determine when searching for devices in area if it's user's actual device or someone elses

TODO register callbacks for when device attempts to connect or disconnect
See - L2CAP & SCO callback functions
*/
