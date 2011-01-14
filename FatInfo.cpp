#include "precomp.h"
#include "fatinfo.h"
#include "hexwnd.h"
#include "hexdoc.h"
#include "utils.h"

// Don't put parentheses here.  We need the expressions evaluated in order.
#define BYTES_PER_FAT_ENTRY   fatType / 8
#define FAT_ENTRIES_PER_BYTE  8 / fatType


void FatDirEntry::Print()
{
    if (!name.Len())
        name = shortName;

    printf("%s  %s\n", GetStringInfo().c_str(), GetName().c_str());
}

wxString FatDirEntry::GetStringInfo()
{
    return
        modify.GetDate() + _T("  ") +
        modify.GetTime() + _T("  ") +
        wxString::Format(_T("%13s  %8X"), FormatDec((UINT64)size).c_str(), firstCluster);
}

void thFatTime::SetTime(const uint8 *raw, uint8 ms_raw /*= 0*/)
{
    USHORT tmp = raw[0] + ((USHORT)raw[1] << 8);
    wSecond = 2 * (tmp & 0x1F);    // bits 4-0
    wMinute = (tmp >>  5) & 0x3F;  // bits 10-5
    wHour   = (tmp >> 11) & 0x3F;  // bits 15-11
    wMilliseconds = ms_raw * 10;
    wSecond += wMilliseconds / 1000;
    wMilliseconds %= 1000;
}

void thFatTime::SetDate(const uint8 *raw)
{
   USHORT tmp = raw[0] + ((USHORT)raw[1] << 8);
   wDay   =  tmp        & 0x1F;  // bits 4-0
   wMonth = (tmp >>  5) & 0x0F;  // bits 8-5
   wYear  = 1980 + ((tmp >> 9) & 0x3F);  // bits 15-9
}

void FatDirEntry::Read(uint8 *buf, const FatInfo *fi)
{
    if (buf[0] == 0xE5) // deleted
    {
        char newname[8];
        memcpy(newname, buf, 8);
        newname[0] = '?';
        name8 = wxString(newname, wxConvLibc, 8).Strip();
    }
    else
        name8 = wxString((char*)buf, wxConvLibc, 8).Strip();
    name3 = wxString((char*)buf + 8, wxConvLibc, 3).Strip();
    attrib = buf[0x0B];
    reserved = buf[0x0C];

    if (reserved & 0x08)
        name8 = name8.Lower();
    if (reserved & 0x10)
        name3 = name3.Lower();
    shortName = name8;
    if (name3.Len())
        shortName += wxChar('.') + name3;

    create.SetTime(buf + 0x0E, buf[0x0D]);
    create.SetDate(buf + 0x10);

    access.SetDate(buf + 0x12);

    firstCluster = *(USHORT*)(buf + 0x1A);
    if (fi->fatType == 32)
        firstCluster += *(ULONG*)(buf + 0x14) << 16;

    modify.SetTime(buf + 0x16);
    modify.SetDate(buf + 0x18);

    size = *(ULONG*)(buf + 0x1C);
}


//*************************************************************************************************
//*************************************************************************************************
// FatInfo
//*************************************************************************************************
//*************************************************************************************************

FatInfo::FatInfo()
{
    fatType = 0;
    hbfatFile = INVALID_HANDLE_VALUE;
    hbfatMapping = NULL;
    bfat = NULL;
}

bool FatInfo::Init(HexWnd *hw, int forceType /*= 0*/)
{
    if (bfat)                               UnmapViewOfFile(bfat);
    if (hbfatMapping != NULL)               CloseHandle(hbfatMapping);
    if (hbfatFile != INVALID_HANDLE_VALUE)  CloseHandle(hbfatFile);
    hbfatFile = INVALID_HANDLE_VALUE;
    hbfatMapping = NULL;
    bfat = NULL;

    if (hw == m_hw)
        return true;
    m_hw = hw;
    if (!hw)
        return false;

    HexDoc *doc = hw->doc;

    doc->Read(0x03, 8, (uint8*)oemName);
    doc->Read(0x36, 8, (uint8*)fsName);
    oemName[8] = fsName[8] = 0;
    fatType = forceType;
    if (fatType == 0)
    {
        if (!strncmp(fsName, "FAT12", 5))
            fatType = 12;
        else if (!strncmp(fsName, "FAT16", 5))
            fatType = 16;
        else 
        {
            doc->Read(0x52, 6, (uint8*)fsName);
            if (!strncmp(fsName, "FAT32", 5))
                fatType = 32;
        }
    }

    if (fatType != 12 && fatType != 16 && fatType != 32)
       return false; // I don't know this filesystem

    bytesPerSector = doc->Read16(0x0B);
    sectorsPerCluster = doc->GetAt(0x0D);
    reservedSectors = doc->Read16(0x0E);
    fatCount = doc->GetAt(0x10);
    rootEntries = doc->Read16(0x11);
    totalSectors = doc->Read16(0x13);
    if (!totalSectors)
        totalSectors = doc->Read32(0x20);
    mediaDescriptor = doc->GetAt(0x15);
    sectorsPerFat = doc->Read16(0x16); // valid only for fat12/16
    sectorsPerTrack = doc->Read16(0x18);
    headCount = doc->Read16(0x1A);
    hiddenSectors = doc->Read32(0x1C);

    if (fatType == 32)
    {
        sectorsPerFat = doc->Read32(0x24);
        rootCluster = doc->Read32(0x2C);
    }
    else
        rootCluster = 0;

    fatSize = sectorsPerFat * bytesPerSector;
    totalClusters = wxMin(totalSectors / sectorsPerCluster, fatSize * FAT_ENTRIES_PER_BYTE);
    fatStart = reservedSectors * bytesPerSector;
    dataStart = fatStart + (fatCount * fatSize);
    clusterSize = sectorsPerCluster * bytesPerSector;

    if (fatType == 12 || fatType == 16)
    {
        rootStart = dataStart;
        dataStart += RoundUp(rootEntries, 32) * 32;
    }
    else
    {
       rootStart = BytePos(AREA_DATA, rootCluster);
    }

    return true;
}

int FatInfo::HitTest(THSIZE byte, THSIZE &offset)
{
    if (m_hw == NULL || fatType == 0)
        return AREA_INVALID;

   if (byte < bytesPerSector) // first sector is boot sector
       return AREA_BS; // offset undefined
   if (byte < fatStart)
       return AREA_RESERVED; // offset undefined
   if (byte < fatStart + fatSize)
   {
       offset = (byte - fatStart) * FAT_ENTRIES_PER_BYTE; // offset = fat entry
       return AREA_FAT1;
   }
   if (fatCount == 2 && byte < fatStart + 2 * fatSize)
   {
       offset = (byte - fatSize - fatStart) * FAT_ENTRIES_PER_BYTE; // offset = fat entry
       return AREA_FAT2;
   }
   if (byte < dataStart) // root directory for FAT12/16
   {
       offset = (byte - fatStart - fatSize * fatCount) / 32; // offset = dir entry
       return AREA_ROOT;
   }

   offset = (byte - dataStart) / clusterSize + 2; // offset = cluster number
   return AREA_DATA;
}

THSIZE FatInfo::BytePos(int area, THSIZE offset)
{
    if (m_hw == NULL || fatType == 0)
        return 0;

    switch (area)
    {
    case AREA_FAT1:
       offset = wxMax(2, wxMin(offset, totalClusters));
       return fatStart + offset * BYTES_PER_FAT_ENTRY;
    case AREA_FAT2:
       offset = wxMax(2, wxMin(offset, totalClusters));
       return fatStart + fatSize + offset * BYTES_PER_FAT_ENTRY;
    case AREA_DATA:
        if (offset == 0)
            return rootStart;
       offset = wxMax(2, wxMin(offset, totalClusters));
       return dataStart + (THSIZE)(offset - 2) * clusterSize;
    case AREA_ROOT:
       return rootStart + (THSIZE)offset * 32; // offset = dir entry
    case AREA_BS:
    case AREA_RESERVED:
    case AREA_INVALID:
    default:
        return 0;
    }
}

int FatInfo::ReadDir(int cluster)
{
   if (!m_hw || !fatType)
      return -1;

   THSIZE bytepos = BytePos(AREA_DATA, cluster);
   if (cluster == 0)
      bytepos = BytePos(AREA_ROOT, 0);
   uint8 buf[32];
   HexDoc *doc = m_hw->doc;
   FatDirEntry de;
   int count = 0;
   wchar_t lfn[256] = {0};

   while (1)
   {
      doc->Read(bytepos, 32, buf);
      bytepos += 32;
      if (buf[0] == 0)
         break;
      if (buf[0x0B] == 0x0F) // attrib byte 0x0F signifies LFN
      {
         int pos = ((buf[0] & 0x1F) - 1) * 13;
         if (pos < 0) // badness
         {
            PRINTF(_T("Invalid LFN skipped at 0x%I64X.\n"), bytepos - 32);
            continue;
         }
         for (int i = 0; i < 5; i++)
            lfn[pos + i] = *(USHORT*)(buf + 1 + 2 * i);
         for (int i = 0; i < 6; i++)
            lfn[pos + i + 5] = *(USHORT*)(buf + 0x0E + 2 * i);
         for (int i = 0; i < 2; i++)
            lfn[pos + i + 11] = *(USHORT*)(buf + 0x1C + 2 * i);
      }
      else // the real directory entry, not a long name
      {
         // read in a normal directory entry
         de.Read(buf, this);
         de.name = lfn;
         if (buf[0] != 0xE5) // deleted?
            de.Print();
         count++;
         de.Clear();
         memset(lfn, 0, sizeof(lfn));
      }
   }
   if (de.name8.Len())
      de.Print();

   return count;
}

void FatInfo::BuildBackwardFAT()
{
    if (!m_hw || !fatType)
        return;

    if (wxMessageBox(_T("Build backward FAT32?"), _T("T.Hex"), wxYES_NO, m_hw) != wxYES)
        return;

    HANDLE hFile = CreateFile(_T("backward.fat"), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, 0);
    DWORD memSize = totalClusters * sizeof(DWORD);
    SetFilePointer(hFile, memSize, 0, FILE_BEGIN);
    SetEndOfFile(hFile);
    HANDLE hMapping = CreateFileMapping(hFile, 0, PAGE_READWRITE, 0, 0, NULL);
    // Say good-bye to your address space...
    DWORD *bfat = (DWORD*)MapViewOfFile(hMapping, FILE_MAP_WRITE, 0, 0, memSize);

    memset(bfat, 0, memSize); // Clear everything first.

    if (fatType == 12)
    {
        for (int cluster = 2; cluster * BYTES_PER_FAT_ENTRY < fatSize; cluster++)
        {
            int next = GetFatEntry(cluster);
            if (next >= 2 && next < totalClusters)
                bfat[next] = cluster;
        }
    }
    else // FAT16 OR FAT32
    {
        int blocksize = 128 * 1024;
        int cluster = 0, next;
        for (int offset = 0; offset < fatSize; offset += blocksize)
        {
            if (fatSize - offset < blocksize)
                blocksize = fatSize - offset;

            const uint8 *buf = m_hw->doc->Load(fatStart + offset, blocksize);
            if (!buf)
            {
                wxString msg;
                msg.Printf(_T("Couldn't read %d bytes from doc at 0x%I64X"), blocksize, offset);
                PRINTF(_T("%s\n"), msg);
                wxMessageBox(msg);
                break;
            }

            for (int runner = 0; runner < blocksize; runner += BYTES_PER_FAT_ENTRY, cluster++)
            {
                if (cluster < 2)
                    continue;
                if (fatType == 32)
                    next = *(int*)(buf + runner);
                else
                    next = (int)*(USHORT*)(buf + runner);
                if (next >= 2 && next < totalClusters)
                    bfat[next] = cluster;
            }
        }
    } // end for FAT16 and FAT32

    UnmapViewOfFile(bfat);
    CloseHandle(hMapping);
    CloseHandle(hFile);
}

int FatInfo::GetFatEntry(int cluster)
{
    int tmp = 0;
    if (fatType == 32) {
        tmp = m_hw->doc->Read32(fatStart + cluster * BYTES_PER_FAT_ENTRY);
        if (tmp >= 0x0FFFFFF0 && tmp <= 0x0FFFFFF6) // reserved
            tmp = 0;
    }
    else if (fatType == 16) {
        tmp = m_hw->doc->Read16(fatStart + cluster * BYTES_PER_FAT_ENTRY);
        if (tmp >= 0xFFF0 && tmp < 0xFFF6) // reserved
            tmp = 0;
    }
    else if (fatType == 12) {
        tmp = m_hw->doc->Read16(fatStart + cluster * BYTES_PER_FAT_ENTRY);
        if (cluster & 1)
            tmp >>= 4;
        tmp &= 0xFFF;
        if (tmp >= 0x0FF0 && tmp <= 0xFF6) // reserved
            tmp = 0;
    }
    return tmp;
}

int FatInfo::NextCluster(int cluster, bool track /*= true*/) // look up from FAT 1
{
    if (!Ok()) return -1;
    if (cluster < 2 || cluster >= totalClusters)
        return -1;

    if (!track)
        return Add(cluster, 1, 0x0FFFFFEF);
    return GetFatEntry(cluster);
}

int FatInfo::PrevCluster(int cluster, bool track /*= true*/) // look up from backward FAT
{
    if (!Ok()) return -1;

    if (!track)
        return Subtract(cluster, 1);

    if (!bfat)
    {
        hbfatFile = CreateFile(_T("backward.fat"), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);
        if (hbfatFile == INVALID_HANDLE_VALUE)
        {
            PRINTF(_T("Couldn't open backward.fat.  Code %d\n"), GetLastError());
            return -1;
        }
        DWORD memSize = totalClusters * sizeof(DWORD);
        hbfatMapping = CreateFileMapping(hbfatFile, 0, PAGE_READONLY, 0, 0, NULL);
        if (hbfatMapping == NULL)
        {
            PRINTF(_T("Couldn't create file mapping for backward.fat.  Code %d\n"), GetLastError());
            CloseHandle(hbfatFile);
            hbfatFile = INVALID_HANDLE_VALUE;
            return -1;
        }
        // Say good-bye to your address space...
        bfat = (DWORD*)MapViewOfFile(hbfatMapping, FILE_MAP_READ, 0, 0, memSize);
        if (!bfat)
        {
            PRINTF(_T("Couldn't map view of file.  Code %d\n"), GetLastError());
            CloseHandle(hbfatFile);
            CloseHandle(hbfatMapping);
            hbfatFile = INVALID_HANDLE_VALUE;
            hbfatMapping = NULL;
        }
    }

    if (!bfat)
        return -1;

    if (cluster < 2 || cluster >= totalClusters)
        return -1;

    return bfat[cluster];
}

bool FatInfo::IsDirectory(int cluster)
{
    if (!Ok()) return false;

    if (!IsValidCluster(cluster))
        return false;

    if (cluster == rootCluster)
        return true;
    
    uint8 buf[32];
    if (!m_hw->doc->Read(BytePos(AREA_DATA, cluster), 32, buf))
        return false;
    if (memcmp(buf, ".          ", 11))
        return false;
    if ((buf[0x0B] & 0x58) != 0x10)
        return false;

    return true; // If it didn't fail any test, it's probably a directory.
}

bool FatInfo::GetFatInfo(int cluster, FAT_CHAIN_INFO *pfci)
{
    if (!Ok()) return false;
    if (!pfci)  return false;

    int first = 0, last = 0, prev = 0, next = 0, before = 0, after = 0, count = 1, contig = 1;
    int tmp, current;
    bool inFirstBlock = true;

    if (!IsValidCluster(cluster))
        return false;

    tmp = GetFatEntry(cluster);
    if (tmp < 2) // says this cluster is unused
    {
        memset(pfci, 0, sizeof(FAT_CHAIN_INFO));
        return true;
    }

    if (FLAGS(pfci->mask, FCI_PREV))
        pfci->prev = PrevCluster(cluster);

    if (FLAGS(pfci->mask, FCI_NEXT))
        pfci->next = NextCluster(cluster);

    if (pfci->mask & (FCI_FIRST | FCI_BEFORE | FCI_COUNT)) // find first cluster
    {
        current = cluster;
        while (IsValidCluster(tmp = PrevCluster(current)))
        {
            if (inFirstBlock && tmp == current - 1)
                pfci->contig[0]++;
            else
                inFirstBlock = false;
            current = tmp;
            before++;
        }
        pfci->first = current;
        pfci->before = before;
    }

    inFirstBlock = true;
    if (pfci->mask & (FCI_LAST | FCI_AFTER | FCI_COUNT)) // find last cluster
    {
        current = cluster;
        while (IsValidCluster(tmp = NextCluster(current)))
        {
            if (inFirstBlock && tmp == current + 1)
                pfci->contig[1]++;
            else
                inFirstBlock = false;
            current = tmp;
            after++;
        }
        pfci->last = current;
        pfci->after = after;
        pfci->count = before + after + 1;
    }

    return true;
}

int FatInfo::GotoPath(wxString path)
{
    wxFileName fn(path);
    if (fn.HasVolume())
        fn.SetVolume(ZSTR);
    wxArrayString dirs = fn.GetDirs();
    
    int cluster = 0;
    for (size_t n = 0; n < dirs.GetCount(); n++)
    {
        cluster = FindFile(cluster, dirs[n]);
        if (!IsValidCluster(cluster))
            return -1;
    }

    if (cluster > 0)
    {
        cluster = FindFile(cluster, fn.GetFullName());
        if (!IsValidCluster(cluster))
            return -1;
    }
    return cluster;
}

int FatInfo::FindFile(int cluster, wxString name)
{
   if (!Ok()) return -1;

   THSIZE bytepos = BytePos(AREA_DATA, cluster);  // handles data cluster 0 as root directory.
   uint8 buf[32];
   HexDoc *doc = m_hw->doc;
   FatDirEntry de;
   wchar_t lfn[256] = {0};

   while (1)
   {
      if (!doc->Read(bytepos, 32, buf))
          return -1;
      bytepos += 32;
      if (buf[0] == 0)
         break;
      if (buf[0] == 0xE5) // deleted?
          continue;
      if (buf[0x0B] == 0x0F) // attrib byte 0x0F signifies LFN
      {
         int pos = ((buf[0] & 0x1F) - 1) * 13;
         if (pos < 0) // badness
         {
            PRINTF(_T("Invalid LFN skipped at 0x%I64X.\n"), bytepos - 32);
            continue;
         }
         for (int i = 0; i < 5; i++)
            lfn[pos + i] = *(USHORT*)(buf + 1 + 2 * i);
         for (int i = 0; i < 6; i++)
            lfn[pos + i + 5] = *(USHORT*)(buf + 0x0E + 2 * i);
         for (int i = 0; i < 2; i++)
            lfn[pos + i + 11] = *(USHORT*)(buf + 0x1C + 2 * i);
      }
      else // the real directory entry, not a long name
      {
         // read in a normal directory entry
         de.Read(buf, this);
         de.name = lfn;
         if (!name.CmpNoCase(de.name) || !name.CmpNoCase(de.shortName))
             return de.firstCluster;
         de.Clear();
         memset(lfn, 0, sizeof(lfn));
      }
   }

   return -1; // not found
}
