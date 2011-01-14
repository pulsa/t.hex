#include "precomp.h"
#include "blockdevs.h"

//! TO DO: get device sizes and block sizes

wxString NoBackslash(wxString in)
{
   return in.EndsWith(_T("\\")) ? in.RemoveLast() : in;
}

wxString YesBackslash(wxString in)
{
   return in.EndsWith(_T("\\")) ? in : (in + _T("\\"));
}

thBlockDevices::thBlockDevices()
{
   Buffer = new TCHAR[BUFFER_SIZE];
   GetVolumeNames();
   GetBlockDevs(&GUID_DEVINTERFACE_DISK);
   GetBlockDevs(&GUID_DEVINTERFACE_VOLUME);
   // This works out nicely because everything we can read either is a disk and has an MBR,
   // or is a volume and doesn't.
   delete [] Buffer;
   Buffer = NULL;
}

void thBlockDevices::GetBlockDevs(const GUID *guid)
{
   DWORD devIndex;
   
   HDEVINFO hDevs = SetupDiGetClassDevs(guid, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
   wxString name, type, path;

   SP_DEVINFO_DATA devInfo = { sizeof(devInfo) };
   SP_DEVICE_INTERFACE_DATA devInt = { sizeof(devInt) };
   for (devIndex = 0; SetupDiEnumDeviceInterfaces(hDevs, NULL, guid, devIndex, &devInt); devIndex++)
   {
      SP_DEVICE_INTERFACE_DETAIL_DATA *pspdidd = (SP_DEVICE_INTERFACE_DETAIL_DATA*)Buffer;
      pspdidd->cbSize = sizeof(*pspdidd); // 5 Bytes!
      if (!SetupDiGetDeviceInterfaceDetail(hDevs, &devInt, pspdidd, BUFFER_SIZE, &dwSize, &devInfo))
         continue;
      path = pspdidd->DevicePath;
      if (!path.Len())
         continue;

      type = GetDeviceType(hDevs, &devInfo);
      if (!type.Len())
         continue; // we don't recognize the type
      name = GetStringProperty(hDevs, &devInfo, SPDRP_FRIENDLYNAME);
      if (!name.Len())
         name = GetStringProperty(hDevs, &devInfo, SPDRP_DEVICEDESC);

      // Get device number and size
      THSIZE size = 0;
      int devnum = -1;

      // open the disk, cdrom or floppy
      HANDLE hDrive = CreateFile(path, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, NULL, NULL);
      if ( hDrive != INVALID_HANDLE_VALUE )
      {
         DWORD cBytes = 0;
         if (type != _T("Floppy")) // Why don't floppy drives know they're empty?  Damn annoying...
         {
            // This will probably only work with hard disks.
            DISK_GEOMETRY g;
            if (DeviceIoControl(hDrive, IOCTL_DISK_GET_DRIVE_GEOMETRY,
                                NULL, 0, &g, sizeof(DISK_GEOMETRY), &cBytes, 0))
               size = (THSIZE)g.BytesPerSector * g.SectorsPerTrack * g.TracksPerCylinder * g.Cylinders.QuadPart;
         }
         // get its device number
         STORAGE_DEVICE_NUMBER sdn;
         if (DeviceIoControl(hDrive, IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0, &sdn, sizeof(sdn), &cBytes, NULL))
            devnum = sdn.DeviceNumber;
         // sdn.DeviceType is not terribly helpful -- HD and FD both show FILE_DEVICE_DISK
         CloseHandle(hDrive);
      }

      thBlockDevice dev(devnum, name, type, path, size);

#if _WIN32_WINNT >= 0x0500
      if (GetVolumeNameForVolumeMountPoint(path + _T("\\"), Buffer, BUFFER_SIZE))
         dev.volume = VolumeIndex(Buffer);  // device has no MBR
      else // maybe the device has partitions.  Let's find out.
         AddPartitions(dev, path);
#endif

      devs.push_back(dev);
   }
   SetupDiDestroyDeviceInfoList(hDevs);
}

void thBlockDevices::AddPartitions(thBlockDevice &dev, wxString path)
{
   HANDLE hDisk = CreateFile(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
   if (hDisk != INVALID_HANDLE_VALUE)
   {
      //extern char *GetDriveSerialNumber(HANDLE hDrive);
      //char *sernum = GetDriveSerialNumber(hDisk); // needs GENERIC_WRITE access?!?
      //if (sernum)
      //   printf(" Serial number: %s\n", sernum);
      DRIVE_LAYOUT_INFORMATION *pdli = (DRIVE_LAYOUT_INFORMATION*)Buffer;
      if (DeviceIoControl(hDisk, IOCTL_DISK_GET_DRIVE_LAYOUT,
         NULL, 0, pdli, BUFFER_SIZE, &dwSize, NULL))
      {
         for (size_t p = 0; p < pdli->PartitionCount; p++)
         {
            DISK_EXTENT part = {-1, 0, 0};
            for (size_t n = 0; n < volumes.size(); n++)
            {
               DISK_EXTENT &de = volumes[n].de;
               PARTITION_INFORMATION &pi = pdli->PartitionEntry[p];
               if (de.DiskNumber == dev.driveNum &&
                  de.StartingOffset.QuadPart == pi.StartingOffset.QuadPart &&
                  de.ExtentLength.QuadPart == pi.PartitionLength.QuadPart)
               {
                  part = de;
                  part.DiskNumber = n;
                  break;
               }
            }
            dev.Partitions.push_back(part);
         }
      }
      else
         printf("IOCTL_DISK_GET_DRIVE_LAYOUT error %d.\n", GetLastError());
      CloseHandle(hDisk);
   }
   else
      printf("  Volume error: %d\n", GetLastError());
}

wxString thBlockDevices::GetDeviceType(HDEVINFO Devs, PSP_DEVINFO_DATA DevInfo, DWORD property /*= 0*/)
{
   if (property == 0)
   {
      wxString type = GetDeviceType(Devs, DevInfo, SPDRP_HARDWAREID);
      if (!type.Len())
         type = GetDeviceType(Devs, DevInfo, SPDRP_COMPATIBLEIDS);
      return type;
   }

   GetStringProperty(Devs, DevInfo, property);

   for (TCHAR *id = Buffer; id[0] && (id < Buffer + dwSize); id += _tcslen(id) + 1)
   {
      if (!_tcsnicmp(id, _T("USBSTOR"), 7))
         return _T("USB Disk");
      if (!_tcsicmp(id, _T("GenDisk")))
         return _T("Disk");
      if (!_tcsicmp(id, _T("GenCdRom")))
         return _T("CD/DVD");
      if (!_tcsicmp(id, _T("GenFloppyDisk")))
         return _T("Floppy");
   }
   return wxEmptyString;
}

wxString thBlockDevices::GetStringProperty(HDEVINFO Devs, PSP_DEVINFO_DATA DevInfo, DWORD property)
{
   dwSize = 0;
   DevInfo->cbSize = sizeof(SP_DEVINFO_DATA);
   if (!SetupDiGetDeviceRegistryProperty(Devs, DevInfo, property, 0,
           (BYTE*)Buffer, BUFFER_SIZE, &dwSize))
       return wxEmptyString;
   return wxString(Buffer, dwSize);
}

void thBlockDevices::GetVolumeNames()
{
   // GetVolumePathNamesForVolumeName() was added in XP.
   // There may be a way to do this in Win2k by enumerating something.
#if _WIN32_WINNT >= 0x0501
   HANDLE hVol = FindFirstVolume(Buffer, BUFFER_SIZE);
   if (hVol == INVALID_HANDLE_VALUE)
      return;

   do {
      wxString name = Buffer;
      wxString tpath = NoBackslash(name);
      thVolume volume(name);

      bool bIsFloppy = false;

      GetVolumePathNamesForVolumeName(name, Buffer, BUFFER_SIZE, &dwSize);
      // dwSize is wrong.  It looks like the number of WCHARS in ASCII mode.
      for (TCHAR *path = Buffer; path[0]; path += _tcslen(path) + 1)
      {
         volume.paths.Add(path);

         wxString dosdev;
         QueryDosDevice(NoBackslash(path), wxStringBuffer(dosdev, 200), 200);
         if (dosdev.StartsWith(_T("\\Device\\Floppy")))
            bIsFloppy = true;
      }

      // Try to get paritition info for this volume.
      // Since we use FILE_READ_ATTRIBUTES here instead of GENERIC_READ, this
      // doesn't cause floppy drives to make that horrible noise.
      // IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS fails because there's no MBR on a floppy disk.
      //DWORD access = (bIsFloppy ? FILE_READ_ATTRIBUTES : GENERIC_READ);
      if (!bIsFloppy)
      {
         HANDLE hDisk = CreateFile(tpath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                 NULL, OPEN_EXISTING, 0, 0);
         if (hDisk != INVALID_HANDLE_VALUE)
         {
            VOLUME_DISK_EXTENTS *pvde = (VOLUME_DISK_EXTENTS*)Buffer;
            GET_LENGTH_INFORMATION len;
            if (DeviceIoControl(hDisk, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
                              NULL, 0, pvde, BUFFER_SIZE, &dwSize, NULL) &&
                  pvde->NumberOfDiskExtents)
            {
               volume.de = pvde->Extents[0];
               //! to do: add all DISK_EXTENTs in the case of a RAID
            }
            else if (DeviceIoControl(hDisk, IOCTL_DISK_GET_LENGTH_INFO,
                                     NULL, 0, &len, sizeof(len), &dwSize, NULL))
            {
               volume.de.StartingOffset.QuadPart = 0;
               volume.de.ExtentLength = len.Length;
            }

            CloseHandle(hDisk);
         }
      }

      volumes.push_back(volume);
   } while (FindNextVolume(hVol, Buffer, BUFFER_SIZE));
   FindVolumeClose(hVol);
#endif
}

int thBlockDevices::VolumeIndex(wxString volumeName)
{
   for (size_t n = 0; n < volumes.size(); n++)
   {
      if (volumes[n].name == volumeName)
         return n;
   }
   return -1;
}

wxString thVolume::GetPaths()
{
    if (!paths.GetCount())
       return wxEmptyString;
    wxString ret = paths[0];
    for (size_t n = 1; n < paths.GetCount(); n++)
       ret += _T(", ") + NoBackslash(paths[n]);
    return ret;
}

wxString thVolume::GetDevPath()
{
    // assume the first path is the drive letter.  Safe?
    if (paths.GetCount())
       return _T("\\\\.\\") + NoBackslash(paths[0]);
    return NoBackslash(name);
}

wxString thVolume::GetRootPath()
{
   return YesBackslash(paths.GetCount() ? paths[0] : name);
}

wxString thBlockDevice::GetDevPath()
{
   return wxString::Format(_T("\\\\.\\PhysicalDrive%d"), driveNum); // close enough, probably
}

BOOL thVolume::GetVolumeInformation(wxString &label, wxString &fsys)
{
    return ::GetVolumeInformation(GetRootPath(),
        wxStringBuffer(label, MAX_PATH), MAX_PATH,
        NULL, NULL, NULL,
        wxStringBuffer(fsys, MAX_PATH), MAX_PATH);
}
