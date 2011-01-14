#include <Windows.h>
#include <string.h>
#include <stdlib.h>
#include <tchar.h>

class RegConfig
{
public:
    RegConfig(LPCTSTR path, HKEY hive = HKEY_CURRENT_USER)
    {
        hKey = NULL;
        if (path)
            Open(path, hive);
    }
    ~RegConfig() { Close(); }

    int ReadInt(LPCTSTR name, int defVal, bool bSetIfEmpty = true)
    {
        DWORD type, cBytes = sizeof(DWORD), val;
        if (RegQueryValueEx(hKey, name, 0, &type, (LPBYTE)&val, &cBytes) != ERROR_SUCCESS)
        {
            if (bSetIfEmpty)
                WriteInt(name, defVal);
            return defVal;
        }
        if (type != REG_DWORD)
            return defVal;
        return (int)val;
    }

    void WriteInt(LPCTSTR name, int val)
    {
        RegSetValueEx(hKey, name, 0, REG_DWORD, (LPBYTE)&val, sizeof(DWORD));
    }

    bool ReadBool(LPCTSTR name, bool defVal, bool bSetIfEmpty = true)
    {
       return 0 != ReadInt(name, defVal, bSetIfEmpty);
    }
    void WriteBool(LPCTSTR name, bool val)
    {
       return WriteInt(name, val);
    }

    LPTSTR ReadString(LPCTSTR name, LPCTSTR defVal, bool bSetIfEmpty = true)
    {
        DWORD cBytes;
        if (RegQueryValueEx(hKey, name, 0, 0, 0, &cBytes) != ERROR_SUCCESS)
        {
            if (bSetIfEmpty && defVal)
                WriteString(name, defVal);
            return defVal ? _tcsdup(defVal) : NULL;
        }
        LPTSTR val = (LPTSTR)malloc(cBytes + 2);
        RegQueryValueEx(hKey, name, 0, 0, (LPBYTE)val, &cBytes);
        return val;
    }

    void WriteString(LPCSTR name, LPCTSTR val)
    {
        DWORD cBytes = _tcslen(val) * sizeof(TCHAR);
        RegSetValueEx(hKey, name, 0, REG_SZ, (LPBYTE)val, cBytes);
    }

    bool Open(LPCTSTR path, HKEY hive = HKEY_CURRENT_USER)
    {
        RegCreateKeyEx(hive, path, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, NULL);
        return true;
    }

    bool Close()
    {
        RegCloseKey(hKey);
        hKey = NULL;
        return true;
    }

    HKEY GetKey()
    {
       return hKey;
    }

protected:
    bool m_dirty;
    HKEY hKey;
};