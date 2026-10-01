#define NOMINMAX
#include <Windows.h>
#include <conio.h>
#include <chrono>
#include <iostream>
#include <random>
#include <string>
#include <sstream>
#include <thread>

using namespace std;

using Clock = chrono::steady_clock;

void waitThreeSeconds(const string& message)
{
    cout << message << '\n';
    for (int seconds = 3; seconds >= 1; --seconds)
    {
        cout << "\r" << seconds << " 秒后开始连点，请切换到需要连点的窗口。"
            << flush;
        Sleep(1000);
    }
    cout << "\r倒计时结束，连点开始。                         \n";
}

int makeRandomDelay(int baseDelay, mt19937& randomEngine)
{
    uniform_int_distribution<int> jitter(-3, 5);
    return max(1, baseDelay + jitter(randomEngine));
}

bool parseKeyName(string text, DWORD& vkCode, string& displayName)
{
    for (char& character : text)
    {
        if (character >= 'a' && character <= 'z')
        {
            character = static_cast<char>(character - 'a' + 'A');
        }
    }

    if (text.size() == 1 &&
        ((text[0] >= 'A' && text[0] <= 'Z') ||
            (text[0] >= '0' && text[0] <= '9')))
    {
        vkCode = static_cast<DWORD>(text[0]);
        displayName = string("键盘【") + text[0] + "】键";
        return true;
    }

    struct NamedKey
    {
        const char* name;
        DWORD code;
        const char* display;
    };

    const NamedKey namedKeys[] = {
        {"SHIFT", VK_SHIFT, "Shift"}, {"CTRL", VK_CONTROL, "Ctrl"},
        {"CONTROL", VK_CONTROL, "Ctrl"}, {"ALT", VK_MENU, "Alt"},
        {"SPACE", VK_SPACE, "空格"}, {"TAB", VK_TAB, "Tab"},
        {"ENTER", VK_RETURN, "Enter"}, {"UP", VK_UP, "方向上"},
        {"DOWN", VK_DOWN, "方向下"}, {"LEFT", VK_LEFT, "方向左"},
        {"RIGHT", VK_RIGHT, "方向右"}, {"F1", VK_F1, "F1"},
        {"F2", VK_F2, "F2"}, {"F3", VK_F3, "F3"},
        {"F4", VK_F4, "F4"}, {"F5", VK_F5, "F5"},
        {"F9", VK_F9, "F9"}, {"F10", VK_F10, "F10"},
        {"F11", VK_F11, "F11"}, {"F12", VK_F12, "F12"}
    };

    for (const NamedKey& key : namedKeys)
    {
        if (text == key.name)
        {
            vkCode = key.code;
            displayName = string("键盘【") + key.display + "】键";
            return true;
        }
    }

    return false;
}

bool triggerSendInput(DWORD vkCode)
{
    INPUT input[2] = {};

    if (vkCode == VK_LBUTTON ||
        vkCode == VK_RBUTTON ||
        vkCode == VK_MBUTTON)
    {
        input[0].type = INPUT_MOUSE;
        input[1].type = INPUT_MOUSE;

        if (vkCode == VK_LBUTTON)
        {
            input[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
            input[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
        }
        else if (vkCode == VK_RBUTTON)
        {
            input[0].mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
            input[1].mi.dwFlags = MOUSEEVENTF_RIGHTUP;
        }
        else
        {
            input[0].mi.dwFlags = MOUSEEVENTF_MIDDLEDOWN;
            input[1].mi.dwFlags = MOUSEEVENTF_MIDDLEUP;
        }
    }
    else
    {
        // 使用扫描码发送键盘事件，兼容更多依赖物理键位的程序。
        // MAPVK_VK_TO_VSC_EX 会在高字节标记 E0/E1 扩展键。
        UINT scanCode = MapVirtualKeyW(vkCode, MAPVK_VK_TO_VSC_EX);
        if (scanCode == 0)
        {
            return false;
        }

        DWORD scanFlags = KEYEVENTF_SCANCODE;
        if ((scanCode & 0xFF00U) != 0)
        {
            scanFlags |= KEYEVENTF_EXTENDEDKEY;
        }

        input[0].type = INPUT_KEYBOARD;
        input[0].ki.wVk = 0;
        input[0].ki.wScan = static_cast<WORD>(scanCode & 0xFFU);
        input[0].ki.dwFlags = scanFlags;

        input[1].type = INPUT_KEYBOARD;
        input[1].ki.wVk = 0;
        input[1].ki.wScan = static_cast<WORD>(scanCode & 0xFFU);
        input[1].ki.dwFlags = scanFlags | KEYEVENTF_KEYUP;
    }

    return SendInput(2, input, sizeof(INPUT)) == 2;
}

void prepareConsoleInput(int hotkey)
{
    while (GetAsyncKeyState(hotkey) & 0x8000)
    {
        Sleep(10);
    }

    HANDLE consoleInput = GetStdHandle(STD_INPUT_HANDLE);
    FlushConsoleInputBuffer(consoleInput);
}

// 返回 false 表示用户按了 Esc 取消
bool readInputLine(string& text)
{
    text.clear();

    while (true)
    {
        int key = _getch();

        if (key == 0 || key == 224)
        {
            _getch();
            continue;
        }

        if (key == 27) // Esc
        {
            cout << "\n已取消。\n";
            return false;
        }

        if (key == '\r') // Enter
        {
            cout << '\n';
            return true;
        }

        if (key == '\b') // Backspace
        {
            if (!text.empty())
            {
                text.pop_back();
                cout << "\b \b" << flush;
            }

            continue;
        }

        // 这里只接受可显示的 ASCII 字符
        if (key >= 32 && key <= 126 && text.size() < 32)
        {
            text.push_back(static_cast<char>(key));
            cout << static_cast<char>(key) << flush;
        }
    }
}

// 必须整行都是一个合法整数。
// 例如 "20" 可以，"20abc" 不可以。
bool parseInteger(const string& text, int& value)
{
    istringstream input(text);

    if (!(input >> value))
    {
        return false;
    }

    input >> ws;
    return input.eof();
}

int main()
{
    SetConsoleOutputCP(CP_UTF8);

    random_device seed;
    mt19937 randomEngine(seed());

    cout << "========================================\n";
    cout << " 欢迎使用 C++ 字母 / 鼠标连点程序\n";
    cout << "========================================\n";
    cout << "F8: 开始 / 暂停连点\n";
    cout << "F7: 调整连点间隔时间\n";
    cout << "F6: 切换连点目标\n";
    cout << "ESC: 主循环中退出；输入时取消\n";
    cout << "修改设置时，请将焦点切回程序窗口。\n";
    cout << "按 F8 后有 3 秒时间切换到需要连点的窗口。\n";
    cout << "----------------------------------------\n";

    bool isRunning = false;
    int delaytime = 10;
    DWORD activeKey = VK_LBUTTON;
    string activeKeyName = "鼠标左键";

    // 记录下一次连点时间，让较长间隔也不会阻塞快捷键检测
    Clock::time_point nextClickTime = Clock::now();

    while (true)
    {
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000)
        {
            cout << ">>> 程序已退出\n";
            break;
        }

        // F8：启动或暂停
        if (GetAsyncKeyState(VK_F8) & 0x8000)
        {
            prepareConsoleInput(VK_F8);

            if (isRunning)
            {
                isRunning = false;
                cout << ">>> 连点已停止\n";
            }
            else
            {
                waitThreeSeconds(">>> 连点准备启动");
                isRunning = true;
                nextClickTime = Clock::now();
                cout << ">>> 当前目标：" << activeKeyName
                    << "，间隔：" << delaytime << " ms\n";
            }
        }

        // F7：调整间隔
        if (GetAsyncKeyState(VK_F7) & 0x8000)
        {
            bool wasRunning = isRunning;
            isRunning = false;

            prepareConsoleInput(VK_F7);

            cout << "\n[调速模式] 连点已暂停。当前间隔："
                << delaytime << " ms\n";
            cout << "请输入新的间隔，范围 1～60000 毫秒："
                << flush;

            string text;
            int tempDelay = 0;

            if (readInputLine(text))
            {
                if (parseInteger(text, tempDelay) &&
                    tempDelay >= 1 &&
                    tempDelay <= 60000)
                {
                    delaytime = tempDelay;

                    cout << "间隔已调整为 "
                        << delaytime << " ms\n";
                }
                else
                {
                    cout << "请输入 1～60000 的整数，"
                        << "此次保持原间隔。\n";
                }
            }

            isRunning = wasRunning;
            nextClickTime = Clock::now();

            if (isRunning)
            {
                waitThreeSeconds("设置完成，连点准备恢复");
                nextClickTime = Clock::now();
            }

            // 输入期间可能按过其他快捷键，
            // 不在本轮继续处理它们。
            continue;
        }

        // F6：切换目标
        if (GetAsyncKeyState(VK_F6) & 0x8000)
        {
            bool wasRunning = isRunning;
            isRunning = false;

            prepareConsoleInput(VK_F6);

            cout << "\n[切换目标] 连点已暂停。\n";
            cout << "[1] 鼠标左键\n";
            cout << "[2] 鼠标右键\n";
            cout << "[3] 鼠标中键\n";
            cout << "[4] 键盘键\n";
            cout << "[5] 空格键\n";
            cout << "请输入编号：" << flush;

            string text;
            int choice = 0;

            if (readInputLine(text))
            {
                if (!parseInteger(text, choice))
                {
                    cout << "编号无效，未更改目标。\n";
                }
                else if (choice == 1)
                {
                    activeKey = VK_LBUTTON;
                    activeKeyName = "鼠标左键";
                }
                else if (choice == 2)
                {
                    activeKey = VK_RBUTTON;
                    activeKeyName = "鼠标右键";
                }
                else if (choice == 3)
                {
                    activeKey = VK_MBUTTON;
                    activeKeyName = "鼠标中键";
                }
                else if (choice == 5)
                {
                    activeKey = VK_SPACE;
                    activeKeyName = "键盘【空格】键";
                }
                else if (choice == 4)
                {
                    cout << "请输入键名："
                        << flush;

                    string keyText;

                    if (readInputLine(keyText))
                    {
                        DWORD newKey = 0;
                        string newKeyName;

                        if (parseKeyName(keyText, newKey, newKeyName))
                        {
                            activeKey = newKey;
                            activeKeyName = newKeyName;
                        }
                        else
                        {
                            cout << "不支持这个键名；F6、F7、F8 是程序快捷键，"
                                << "不能设为目标。\n";
                        }
                    }
                }
                else
                {
                    cout << "不存在这个选项，未更改目标。\n";
                }
            }

            cout << "当前目标：" << activeKeyName << '\n';

            isRunning = wasRunning;
            nextClickTime = Clock::now();

            if (isRunning)
            {
                waitThreeSeconds("设置完成，连点准备恢复");
                nextClickTime = Clock::now();
            }

            continue;
        }

        // 到达指定时间后，执行一次连点
        if (isRunning)
        {
            Clock::time_point now = Clock::now();

            if (now >= nextClickTime)
            {
                if (!triggerSendInput(activeKey))
                {
                    isRunning = false;
                    cout << ">>> 发送输入失败，连点已暂停。\n";
                }

                int actualDelay = makeRandomDelay(delaytime, randomEngine);
                nextClickTime = Clock::now() +
                    chrono::milliseconds(actualDelay);
            }

            // 较长的剩余时间先休眠，最后约 1 ms 主动让出时间片，
            // 无需修改 Windows 的全局计时器精度。
            Clock::time_point currentTime = Clock::now();
            if (nextClickTime > currentTime + chrono::milliseconds(2))
            {
                this_thread::sleep_until(
                    nextClickTime - chrono::milliseconds(1));
            }
            else
            {
                this_thread::yield();
            }
        }
        else
        {
            Sleep(20);
        }
    }

    return 0;
}
