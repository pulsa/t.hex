#ifndef _BLOCKDEVS_H_
#define _BLOCKDEVS_H_

#include "defs.h"

class thBlockDevice
{
public:
    thBlockDevice(int driveNum, wxString name, wxString type, wxString path, THSIZE size = 0)
    {
        this->driveNum = driveNum;
        this->name = name;
        this->type = type;
        this->path = path;
        this->volume = -1;
        this->size = size;
    }

    // For drives with an MBR, we hope to find some partitions.
    // Use the DiskNumber member to specify a volume from the table.
    std::vector<DISK_EXTENT> Partitions;
    // For other drives (like CD & floppy), store an index into the volume table.
    int volume;

    wxString name, type, path;
    int driveNum;
    THSIZE size; // added 2007-08-14.  Will this work?  We'll find out...

    wxString GetDevPath();
};

class thVolume
{
public:
    thVolume(wxString name)
    {
        this->name = name;
        de.DiskNumber = -1;
        de.ExtentLength.QuadPart = 0;
    }

    wxString name; // "\\?\Volume{...}\"
    wxArrayString paths;
    DISK_EXTENT de;
    VOLUME_DISK_EXTENTS *pvde; // for future RAID support; not currently used.

    wxString GetPaths();        // "X:, C:\mnt\X"
    wxString GetDevPath();      // "\\.\X:"
    wxString GetRootPath();     // "X:\"
    BOOL GetVolumeInformation(wxString &label, wxString &fsys);

    THSIZE size() const { return de.ExtentLength.QuadPart; }
};

class thBlockDevices
{
public:
    thBlockDevices(); // do everything.

    std::vector<thVolume> volumes;
    std::vector<thBlockDevice> devs;

protected:
    void       GetVolumeNames();
    void       GetBlockDevs(const GUID *guid);
    wxString   GetStringProperty(HDEVINFO Devs, PSP_DEVINFO_DATA DevInfo, DWORD property);
    wxString   GetDeviceType(HDEVINFO Devs, PSP_DEVINFO_DATA DevInfo, DWORD property = 0);
    int        VolumeIndex(wxString volumeName);
    void       AddPartitions(thBlockDevice &dev, wxString path);

private:
    TCHAR *Buffer;
    DWORD dwSize;
    enum { BUFFER_SIZE = 10000 };
};

#endif // _BLOCKDEVS_H_
