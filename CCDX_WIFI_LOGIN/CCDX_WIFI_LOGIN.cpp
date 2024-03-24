/**
 * @file CCDX_WIFI_LOGIN.cpp
 * @date 24.03.2024
 * @author RMSHE
 *
 * < GasSensorOS >
 * Copyright(C) 2024 RMSHE. All rights reserved.
 *
 * This program is free software : you can redistribute it and /or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.If not, see < https://www.gnu.org/licenses/>.
 *
 * Electronic Mail : asdfghjkl851@outlook.com
 */

#include "WebRequests.hpp"
#include "CSV_Operations.hpp"
#include "WiFiConnector.hpp"

int main() {
    CSV_Operations csv_op;

    // Windows系统中连接到CCDX-WIFI信号;
    try {
        WiFiConnector connector;
        // connector.Connect(L"MySSID", L"MyPassword"); // 连接到需要密码的WIFI
        connector.Connect(L"ccdx-wifi"); // 连接到公共WIFI
    } catch (const std::runtime_error &e) {
        std::wcerr << L"出现错误: " << e.what() << std::endl;
    }

    while (true) {
        // 从CSV文件读取账户信息
        std::vector<std::vector<std::string>> accountInfo = csv_op.readFromCSV("./AccountInfo.csv");

        // 如果账户信息为空，则提示用户输入并保存到CSV文件中
        if (accountInfo.empty()) {
            std::cout << "[未设置账号信息] 您只需要输入一次用户名和密码我们便可永久记住。\n" << std::endl;

            std::string username, password;

            std::cout << "请输入用户名: ";
            std::cin >> username;

            std::cout << "请输入密码: ";
            std::cin >> password;

            accountInfo.push_back({ username, password });

            // 保存用户名和密码到CSV
            csv_op.writeToCSV(accountInfo, "./AccountInfo.csv");
        }

        // 使用账户信息创建Web请求对象
        WebRequests webReq(accountInfo[0][0], accountInfo[0][1]);

        // 尝试登录并获取响应
        auto response = webReq.login();

        // 检查登录是否成功, 如果登录失败则打印错误信息;
        if (response[0].first == "result" && response[0].second == "false") {
            std::cout << "[登录失败] 错误信息:\n";
            for (const auto &[key, value] : response) { std::cout << key << ": " << value << '\n'; }

            std::cout << "\n是否需要重置密码?(Y/N): ";
            std::string cmd; std::cin >> cmd;

            if (cmd == "Y" || cmd == "y") csv_op.deletePath("./AccountInfo.csv");

            system("cls");
        } else {
            break;
        }
    }

    return 0;
}