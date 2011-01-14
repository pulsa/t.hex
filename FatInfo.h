#pragma once

#ifndef _FATINFO_H_
#define _FATINFO_H_

#include "defs.h"

class HexWnd;
class HexDoc;
class FatInfo;

class thFatTime : public SYSTEMTIME
{
public:
    bool IsFull();
    thFatTime() { Clear(); }

    void Clear()
    {
        wYear = wMonth = wDay = wHour = wMinute = wSecond = wMilliseconds = 0;
    }

    BOOL ToLocalFileTime(FILETIME *pft) {
        FILETIME tmp;
        return SystemTimeToFileTime(this, &tmp) &&
            FileTimeToLocalFileTime(&tmp, pft);
    }

    void SetDate(const uint8* raw);
    void SetTime(const uint8* raw, uint8 ms_raw = 0);

    wxString GetDate() { return wxString::Format(_T("%04d-%02d-%02d"), wYear, wMonth, wDay); }
    wxString GetTime() { return wxString::Format(_T("%02d:%02d:%02d"), wHour, wMinute, wSecond); }
};


class FatDirEntry
{
public:
    wxString name, name8, name3, shortName;
    BYTE attrib;
    BYTE reserved;

    thFatTime create, modify, access;
    int firstCluster;
    ULONG size;

    void Clear()
    {
        name = name8 = name3 = wxEmptyString;
        create.Clear();
        modify.Clear();
        access.Clear();
    }

    wxString GetStringInfo();
    wxString GetName() { return (name.Len() ? name : shortName); }
    void Print();
    void Read(uint8* buf, const FatInfo *fi);
};

enum {
    FCI_FIRST  = 0x01,
    FCI_LAST   = 0x02,
    FCI_PREV   = 0x04,
    FCI_NEXT   = 0x08,
    FCI_BEFORE = 0x10,
    FCI_AFTER  = 0x20,
    FCI_COUNT  = 0x40,
    FCI_CONTIG = 0x80,
};

typedef struct {
    int mask;
    int first, last;   // first and last cluster of file or directory
    int prev,  next;   // immediate neighbors
    int before, after; // where are we in the chain
    int count;         // total clusters in file
    int contig[2];     // number of contiguous clusters before and after [cluster]
} FAT_CHAIN_INFO;

class FatInfo
{
public:
    FatInfo();
    bool Init(HexWnd *hw, int forceType = 0);

    enum {AREA_INVALID = 0, AREA_BS, AREA_RESERVED, AREA_FAT1, AREA_FAT2, AREA_ROOT, AREA_DATA};
    int HitTest(THSIZE byte, THSIZE &offset); // returns the area

    THSIZE BytePos(int area, THSIZE offset);

    int ReadDir(int cluster);

    bool Ok() { return m_hw && fatType; }
    bool IsValidCluster(int cluster) { return (cluster >= 2 && cluster < totalClusters) ||
        cluster == rootCluster; }  //! Does this break anything?  2009-08-21

    int GetFatEntry(int cluster);
    int NextCluster(int cluster, bool track = true); // look up from FAT 1
    int PrevCluster(int cluster, bool track = true); // look up from backward FAT
    void BuildBackwardFAT();
    //int FirstClusterOfObject(int cluster);
    bool GetFatInfo(int cluster, FAT_CHAIN_INFO *fci);
    bool IsDirectory(int cluster);

    bool AreaHasClusters(int area) {
        return area == AREA_FAT1 || area == AREA_FAT2 || area == AREA_DATA ||
            (area == AREA_ROOT && fatType == 32);
    }

    int GotoPath(wxString path);
    int FindFile(int cluster, wxString name);

    int fatType; // 0 = invalid, 12, 16, or 32

    // These are read directly from the boot sector.
    char oemName[9];
    char fsName[9];
    int bytesPerSector;
    int sectorsPerCluster;
    int reservedSectors;
    int fatCount;
    ULONG totalSectors;
    BYTE mediaDescriptor;
    int sectorsPerTrack;
    int headCount;
    int hiddenSectors;
    int sectorsPerFat;
    int rootCluster;
    int rootEntries;

    // These are calculated from the above values.
    int totalClusters;
    int clusterSize;
    THSIZE fatStart, fatSize, rootStart, dataStart;
    //int bytesPerFatEntryNum, bytesPerFatEntryDenom;

private:
    HexWnd *m_hw;

    HANDLE hbfatFile;
    HANDLE hbfatMapping;
    DWORD *bfat;
};

#endif // _FATINFO_H_
