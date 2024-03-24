#include <iostream>
#include <wlanapi.h>
#include <windot11.h> // ����DOT11_SSID�ṹ
#include <objbase.h>
#include <wtypes.h>
#include <string>
#pragma comment(lib, "wlanapi.lib")
#pragma comment(lib, "ole32.lib")

class WiFiConnector {
public:
    WiFiConnector() {
        // ��ʼ��WLAN�ͻ���
        DWORD dwClientVersion = 2; // ָ���ͻ��˰汾��Windows Vista �����߰汾Ϊ2
        DWORD dwError = WlanOpenHandle(dwClientVersion, NULL, &dwNegotiatedVersion, &hClient);
        if (dwError != ERROR_SUCCESS) {
            throw std::runtime_error("�޷���ʼ��WLAN�ͻ��ˡ�");
        }

        // ö��WLAN�ӿ�
        WLAN_INTERFACE_INFO_LIST *pIfList = nullptr;
        dwError = WlanEnumInterfaces(hClient, NULL, &pIfList);
        if (dwError != ERROR_SUCCESS) {
            throw std::runtime_error("�޷�ö��WLAN�ӿڡ�");
        }

        // �������ֻ��һ��WLAN�ӿڣ�����ȡ��ӿ�GUID
        if (pIfList->dwNumberOfItems > 0) {
            pInterfaceInfo = new WLAN_INTERFACE_INFO(pIfList->InterfaceInfo[0]);
        } else {
            throw std::runtime_error("δ�ҵ�WLAN�ӿڡ�");
        }

        WlanFreeMemory(pIfList); // �ͷ�ö�ٽӿ��б�ռ�õ��ڴ�
    }

    ~WiFiConnector() {
        // �ͷ���Դ
        if (hClient != NULL) {
            WlanCloseHandle(hClient, NULL);
        }
        if (pInterfaceInfo != nullptr) {
            delete pInterfaceInfo;
        }
    }

    void Connect(const std::wstring &ssid, const std::wstring &password = L"") {
        std::wstring profileXml = ConstructProfileXML(ssid, password); // ���������ļ�XML

        // ����WIFI�����ļ�
        DWORD dwReasonCode = 0;
        DWORD dwError = WlanSetProfile(hClient, &pInterfaceInfo->InterfaceGuid, 0, profileXml.c_str(), NULL, TRUE, NULL, &dwReasonCode);
        if (dwError != ERROR_SUCCESS) {
            throw std::runtime_error("����WIFI�����ļ�ʧ�ܡ�");
        }

        // ���ӵ�WIFI
        WLAN_CONNECTION_PARAMETERS connectParams;
        ZeroMemory(&connectParams, sizeof(connectParams));
        connectParams.wlanConnectionMode = wlan_connection_mode_profile;
        connectParams.strProfile = ssid.c_str();
        connectParams.dot11BssType = dot11_BSS_type_any;
        dwError = WlanConnect(hClient, &pInterfaceInfo->InterfaceGuid, &connectParams, NULL);
        if (dwError != ERROR_SUCCESS) {
            throw std::runtime_error("����WIFIʧ�ܡ�");
        }
    }

private:
    HANDLE hClient = NULL;
    DWORD dwNegotiatedVersion;
    WLAN_INTERFACE_INFO *pInterfaceInfo = nullptr;

    std::wstring ConstructProfileXML(const std::wstring &ssid, const std::wstring &password) {
        // ����SSID�����빹��WLAN�����ļ���XML�ַ���
        std::wstring securityXml;
        if (!password.empty()) {
            // ��Ҫ�����WIFI
            securityXml = L"<authEncryption><authentication>WPA2PSK</authentication><encryption>AES</encryption><useOneX>false</useOneX></authEncryption>"
                L"<sharedKey><keyType>passPhrase</keyType><protected>false</protected><keyMaterial>" + password + L"</keyMaterial></sharedKey>";
        } else {
            // ����WIFI
            securityXml = L"<authEncryption><authentication>open</authentication><encryption>none</encryption><useOneX>false</useOneX></authEncryption>";
        }

        std::wstring profileXml = L"<?xml version=\"1.0\"?><WLANProfile xmlns=\"http://www.microsoft.com/networking/WLAN/profile/v1\">"
            L"<name>" + ssid + L"</name><SSIDConfig><SSID><name>" + ssid + L"</name></SSID></SSIDConfig>"
            L"<connectionType>ESS</connectionType><connectionMode>auto</connectionMode><MSM><security>" +
            securityXml + L"</security></MSM></WLANProfile>";

        return profileXml;
    }
};