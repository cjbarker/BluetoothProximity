#include <Windows.h>
#include <BluetoothAPIs.h>
#include <string>
#include <map>

using namespace std;

#ifndef __BLUETOOTHPROXY_H__
#define __BLUETOOTHPROXY_H__

class Guard : public CRITICAL_SECTION {
	public:
		Guard() {
			InitializeCriticalSection(this);
		}

		~Guard() {
			DeleteCriticalSection(this);
		}

	private:
		// disable copy and assignment of Guard
		Guard( Guard const&);
		Guard& operator=( Guard const&);
};

class BluetoothProxy {
  public:
		static BluetoothProxy * GetInstance();

		void FindDevices();
		void FindDevices(const BLUETOOTH_DEVICE_SEARCH_PARAMS&, BLUETOOTH_DEVICE_INFO&);
		void AddDevice(BLUETOOTH_DEVICE_INFO& deviceInfo);
		void AddDevice(const wstring&);
		void RemoveDevice(BLUETOOTH_DEVICE_INFO& deviceInfo);
		void RemoveDevice(const wstring&);
		BOOL IsProximityDevice(BLUETOOTH_DEVICE_INFO&);

  private:
		static BluetoothProxy * m_pInstance;	
		static Guard INIT_LOCK;

		HBLUETOOTH_DEVICE_FIND m_Device;	// bluetooth device
		map<wstring, int> m_DeviceMap;		// name of device and 1=exists/paired

		BOOL IsWorkstationLocked();
		DWORD HandleLastError(const char *msg);
		void HandleState(BLUETOOTH_DEVICE_INFO& deviceInfo);

		BluetoothProxy();
		BluetoothProxy(BluetoothProxy const&) {}				// prevent copy constructor use
		BluetoothProxy& operator=(BluetoothProxy const&){};		// prevent assignment operator use
};

#endif // __BLUETOOTHPROXY_H__