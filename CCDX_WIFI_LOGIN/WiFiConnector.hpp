#include <iostream>
#include <wlanapi.h>
#include <windot11.h> // 对于DOT11_SSID结构
#include <objbase.h>
#include <wtypes.h>
#include <string>
#pragma comment(lib, "wlanapi.lib")
#pragma comment(lib, "ole32.lib")

class WiFiConnector {
public:
    WiFiConnector() {
        // 初始化WLAN客户端
        DWORD dwClientVersion = 2; // 指定客户端版本，Windows Vista 及更高版本为2
        DWORD dwError = WlanOpenHandle(dwClientVersion, NULL, &dwNegotiatedVersion, &hClient);
        if (dwError != ERROR_SUCCESS) {
            throw std::runtime_error("无法初始化WLAN客户端。");
        }

        // 枚举WLAN接口
        WLAN_INTERFACE_INFO_LIST *pIfList = nullptr;
        dwError = WlanEnumInterfaces(hClient, NULL, &pIfList);
        if (dwError != ERROR_SUCCESS) {
            throw std::runtime_error("无法枚举WLAN接口。");
        }

        // 这里假设只有一个WLAN接口，并获取其接口GUID
        if (pIfList->dwNumberOfItems > 0) {
            pInterfaceInfo = new WLAN_INTERFACE_INFO(pIfList->InterfaceInfo[0]);
        } else {
            throw std::runtime_error("未找到WLAN接口。");
        }

        WlanFreeMemory(pIfList); // 释放枚举接口列表占用的内存
    }

    ~WiFiConnector() {
        // 释放资源
        if (hClient != NULL) {
            WlanCloseHandle(hClient, NULL);
        }
        if (pInterfaceInfo != nullptr) {
            delete pInterfaceInfo;
        }
    }

    void Connect(const std::wstring &ssid, const std::wstring &password = L"") {
        std::wstring profileXml = ConstructProfileXML(ssid, password); // 构造配置文件XML

        // 设置WIFI配置文件
        DWORD dwReasonCode = 0;
        DWORD dwError = WlanSetProfile(hClient, &pInterfaceInfo->InterfaceGuid, 0, profileXml.c_str(), NULL, TRUE, NULL, &dwReasonCode);
        if (dwError != ERROR_SUCCESS) {
            throw std::runtime_error("设置WIFI配置文件失败。");
        }

        // 连接到WIFI
        WLAN_CONNECTION_PARAMETERS connectParams;
        ZeroMemory(&connectParams, sizeof(connectParams));
        connectParams.wlanConnectionMode = wlan_connection_mode_profile;
        connectParams.strProfile = ssid.c_str();
        connectParams.dot11BssType = dot11_BSS_type_any;
        dwError = WlanConnect(hClient, &pInterfaceInfo->InterfaceGuid, &connectParams, NULL);
        if (dwError != ERROR_SUCCESS) {
            throw std::runtime_error("连接WIFI失败。");
        }
    }

    // 检查Windows系统是否已经连接到指定SSID的WIFI: true表示已连接到指定SSID的WIFI；如果不匹配或查询失败，则返回false;
    bool IsConnectedToSSID(const std::wstring &targetSSID) {
        WLAN_CONNECTION_ATTRIBUTES *pConnectInfo = nullptr;
        DWORD dwDataSize = 0;
        DWORD dwError = WlanQueryInterface(hClient, &pInterfaceInfo->InterfaceGuid,
                                           wlan_intf_opcode_current_connection, NULL,
                                           &dwDataSize, (void **)&pConnectInfo, NULL);
        if (dwError != ERROR_SUCCESS) {
            // 查询接口失败
            return false;
        }

        // 使用辅助函数获取当前连接的SSID字符串
        std::wstring currentSSID = SSIDToString(pConnectInfo->wlanAssociationAttributes.dot11Ssid.ucSSID,
                                                pConnectInfo->wlanAssociationAttributes.dot11Ssid.uSSIDLength);

        // 检查SSID是否匹配
        bool isConnected = (currentSSID == targetSSID);

        // 释放查询到的内存
        WlanFreeMemory(pConnectInfo);

        return isConnected;
    }

private:
    HANDLE hClient = NULL;
    DWORD dwNegotiatedVersion;
    WLAN_INTERFACE_INFO *pInterfaceInfo = nullptr;

    std::wstring ConstructProfileXML(const std::wstring &ssid, const std::wstring &password) {
        // 根据SSID和密码构造WLAN配置文件的XML字符串
        std::wstring securityXml;
        if (!password.empty()) {
            // 需要密码的WIFI
            securityXml = L"<authEncryption><authentication>WPA2PSK</authentication><encryption>AES</encryption><useOneX>false</useOneX></authEncryption>"
                L"<sharedKey><keyType>passPhrase</keyType><protected>false</protected><keyMaterial>" + password + L"</keyMaterial></sharedKey>";
        } else {
            // 公共WIFI
            securityXml = L"<authEncryption><authentication>open</authentication><encryption>none</encryption><useOneX>false</useOneX></authEncryption>";
        }

        std::wstring profileXml = L"<?xml version=\"1.0\"?><WLANProfile xmlns=\"http://www.microsoft.com/networking/WLAN/profile/v1\">"
            L"<name>" + ssid + L"</name><SSIDConfig><SSID><name>" + ssid + L"</name></SSID></SSIDConfig>"
            L"<connectionType>ESS</connectionType><connectionMode>auto</connectionMode><MSM><security>" +
            securityXml + L"</security></MSM></WLANProfile>";

        return profileXml;
    }

    // 将SSID的字节序列转换为wstring
    std::wstring SSIDToString(const UCHAR *ssid, ULONG ssidLength) {
        std::wstring ssidStr;
        for (ULONG i = 0; i < ssidLength; ++i) {
            if (ssid[i] != '\0') { // 忽略空字符
                ssidStr += static_cast<wchar_t>(ssid[i]);
            }
        }
        return ssidStr;
    }
};